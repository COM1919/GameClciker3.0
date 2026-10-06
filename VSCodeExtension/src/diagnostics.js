'use strict';
/**
 * GC Lua 诊断引擎（纯逻辑，便于单元测试）
 * 返回与 VSCode 解耦的诊断描述，由 extension.js 转换为 vscode.Diagnostic。
 */

const scanner = require('./luaScanner');
const api = require('../data/api');

const SEVERITY = { error: 'error', warning: 'warning', information: 'information', hint: 'hint' };

/** 计算行起始偏移，用于 offset -> line/col */
function computeLineStarts(text) {
  const starts = [0];
  for (let i = 0; i < text.length; i++) {
    if (text[i] === '\n') starts.push(i + 1);
  }
  return starts;
}

/** offset -> {line, character}（0 基） */
function offsetToPos(lineStarts, offset) {
  let lo = 0;
  let hi = lineStarts.length - 1;
  while (lo < hi) {
    const mid = Math.ceil((lo + hi) / 2);
    if (lineStarts[mid] <= offset) lo = mid;
    else hi = mid - 1;
  }
  return { line: lo, character: offset - lineStarts[lo] };
}

/**
 * 计算一个调用表达式的实参个数
 * @param {string} mask 代码掩码
 * @param {number} openIdx '(' 的索引
 * @returns {{count:number, end:number}|null}
 */
function countArguments(mask, openIdx) {
  if (mask[openIdx] !== '(') return null;
  let depth = 0;
  let count = 0;
  let hasContent = false;
  let i = openIdx + 1;
  for (; i < mask.length; i++) {
    const c = mask[i];
    if (c === '(' || c === '[' || c === '{') { depth++; hasContent = true; continue; }
    if (c === ')' || c === ']' || c === '}') {
      if (depth === 0 && c === ')') {
        return { count: hasContent ? count + 1 : 0, end: i };
      }
      depth--;
      continue;
    }
    if (c === ',' && depth === 0) { count++; continue; }
    if (!/\s/.test(c)) hasContent = true;
  }
  return null;
}

/**
 * 从块的起始 token（function / if / do / repeat）找到匹配的 end / until 的 token 索引。
 * 采用与 scanner.checkBlockPairs 一致的 opener 集合（while/for 的块以其内部的 do 表示）。
 */
function findBlockEndIndex(tokens, openerIdx) {
  const openers = { function: 1, if: 1, do: 1, repeat: 1 };
  let depth = 0;
  for (let i = openerIdx; i < tokens.length; i++) {
    const t = tokens[i];
    if (t.type !== 'keyword') continue;
    if (openers[t.value]) depth++;
    else if (t.value === 'end' || t.value === 'until') {
      depth--;
      if (depth <= 0) return i;
    }
  }
  return -1;
}

/** 提取一次调用的实参，每个实参为一组 token（忽略分隔逗号） */
function collectArgumentTokens(tokens, openIdx) {
  const args = [];
  let cur = [];
  let depth = 0;
  for (let k = openIdx + 1; k < tokens.length; k++) {
    const t = tokens[k];
    if (t.value === '(' || t.value === '[' || t.value === '{') { depth++; cur.push(t); continue; }
    if (t.value === ')' || t.value === ']' || t.value === '}') {
      if (depth === 0 && t.value === ')') { if (cur.length) args.push(cur); return args; }
      depth--; cur.push(t); continue;
    }
    if (t.value === ',' && depth === 0) { args.push(cur); cur = []; continue; }
    cur.push(t);
  }
  if (cur.length) args.push(cur);
  return args;
}

/** 判断单个 token 的字面量类型；非字面量返回 null */
function literalType(token) {
  if (!token) return null;
  if (token.type === 'string') return 'string';
  if (token.type === 'number') return 'number';
  if (token.type === 'keyword' && (token.value === 'true' || token.value === 'false')) return 'boolean';
  if (token.value === '{') return 'table';
  if (token.value === 'nil') return 'nil';
  return null;
}

/** 把 API 参数类型描述映射为可校验的字面量类型集合；无法判定（联合类型/任意类型）返回 null */
function expectedLiteralTypes(typeStr) {
  if (!typeStr) return null;
  if (typeStr.includes('|') || typeStr.includes('任意')) return null;
  if (typeStr.includes('字符串')) return ['string'];
  if (typeStr.includes('Bool')) return ['boolean'];
  if (typeStr.includes('整数') || typeStr.includes('浮点') || typeStr.includes('小数') || typeStr.includes('数字')) return ['number'];
  if (typeStr.includes('table')) return ['table'];
  return null;
}

const TYPE_LABEL = { string: '字符串', number: '数字', boolean: '布尔值', table: '表(table)', nil: 'nil' };

/** 脚本生命周期函数：允许定义后不被显式调用 */
const LIFECYCLE_NAMES = new Set(['initScript', 'main', 'render', 'saveConfig']);

/** 未使用的局部变量 */
function checkUnusedLocals(tokens, add, level) {
  const locals = [];
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && t.value === 'local')) continue;
    let j = i + 1;
    // local function name —— 交由 checkUnusedFunctions 处理，避免重复
    if (tokens[j] && tokens[j].type === 'keyword' && tokens[j].value === 'function') continue;
    // local a, b, c
    while (j < tokens.length && tokens[j].type === 'name') {
      locals.push(tokens[j]);
      j++;
      if (tokens[j] && tokens[j].value === ',') { j++; continue; }
      break;
    }
  }
  for (const tok of locals) {
    if (tok.value.startsWith('_')) continue;
    let used = false;
    for (const t of tokens) {
      if (t.type === 'name' && t.value === tok.value && t.start !== tok.start) { used = true; break; }
    }
    if (!used) {
      add(tok.start, tok.end, level, `局部变量 \`${tok.value}\` 已声明但从未使用，可以删除。`, 'unused-local');
    }
  }
}

/** 未被调用的函数 */
function checkUnusedFunctions(tokens, add, level) {
  const defs = [];
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && t.value === 'function')) continue;
    let j = i + 1;
    let nameTok = null;
    while (j < tokens.length && tokens[j].type === 'name') {
      nameTok = tokens[j];
      j++;
      if (tokens[j] && (tokens[j].value === '.' || tokens[j].value === ':')) { j++; continue; }
      break;
    }
    if (nameTok) defs.push(nameTok);
  }
  for (const tok of defs) {
    const name = tok.value;
    if (LIFECYCLE_NAMES.has(name) || name.startsWith('_')) continue;
    let used = false;
    for (const t of tokens) {
      if (t.type === 'name' && t.value === name && t.start !== tok.start) { used = true; break; }
    }
    if (!used) {
      add(tok.start, tok.end, level,
        `函数 \`${name}\` 定义后从未被调用（GC 只会自动调用 initScript / main / render / saveConfig）。`,
        'unused-function');
    }
  }
}

/** 可疑逻辑：= 与 == 混淆、死循环、除零、自我赋值、恒真/恒假、空块、重复定义 */
function checkSuspicious(tokens, mask, add, level) {
  const COND_KEYWORDS = new Set(['if', 'elseif', 'while']);
  const STOP_KEYWORDS = new Set(['then', 'do', 'end', 'function', 'local', 'return', 'repeat', 'else', 'elseif']);

  // (a) 条件中误用赋值符号 `=`
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    const isCond = t.type === 'keyword' && COND_KEYWORDS.has(t.value);
    const isUntil = t.type === 'keyword' && t.value === 'until';
    if (!isCond && !isUntil) continue;
    const limit = Math.min(tokens.length, i + 60);
    for (let j = i + 1; j < limit; j++) {
      const u = tokens[j];
      if (isUntil) {
        // until 的条件一般与 until 在同一行
        if (u.line !== t.line) break;
      } else if (u.type === 'keyword' && STOP_KEYWORDS.has(u.value)) {
        break;
      }
      if (u.value === '=') {
        add(u.start, u.end, level, '条件中用了赋值符号 `=`，是否想写比较运算 `==`？', 'assignment-in-condition');
        break;
      }
    }
  }

  // (b) while true 死循环（块内没有 break / return）
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && t.value === 'while')) continue;
    const cond = tokens[i + 1];
    if (!(cond && cond.type === 'keyword' && cond.value === 'true')) continue;
    let doIdx = -1;
    for (let j = i + 1; j < tokens.length && j < i + 60; j++) {
      const u = tokens[j];
      if (u.type === 'keyword' && u.value === 'do') { doIdx = j; break; }
      if (u.type === 'keyword' && (u.value === 'end' || u.value === 'function')) break;
    }
    if (doIdx < 0) continue;
    const endIdx = findBlockEndIndex(tokens, doIdx);
    const scanEnd = endIdx < 0 ? tokens.length : endIdx;
    let hasExit = false;
    for (let k = doIdx + 1; k < scanEnd; k++) {
      const u = tokens[k];
      if (u.type === 'keyword' && (u.value === 'break' || u.value === 'return')) { hasExit = true; break; }
    }
    if (!hasExit) {
      add(t.start, t.end, level, '`while true` 循环内没有 `break` 或 `return`，会一直执行无法退出（死循环）。', 'infinite-loop');
    }
  }

  // (c) for 循环步长为 0
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && t.value === 'for')) continue;
    let eq = -1;
    let isGenericFor = false;
    for (let j = i + 1; j < tokens.length && j < i + 80; j++) {
      const u = tokens[j];
      if (u.type === 'keyword' && u.value === 'in') { isGenericFor = true; break; }
      if (u.type === 'keyword' && (u.value === 'do' || u.value === 'end' || u.value === 'function')) break;
      if (u.value === '=' && eq < 0) eq = j;
    }
    if (isGenericFor || eq < 0) continue;
    const segs = [];
    let cur = [];
    let depth = 0;
    let foundDo = false;
    for (let k = eq + 1; k < tokens.length; k++) {
      const u = tokens[k];
      if (depth === 0 && u.type === 'keyword' && u.value === 'do') { foundDo = true; break; }
      if (u.value === '(' || u.value === '[' || u.value === '{') depth++;
      else if (u.value === ')' || u.value === ']' || u.value === '}') depth--;
      else if (u.value === ',' && depth === 0) { segs.push(cur); cur = []; continue; }
      cur.push(u);
    }
    if (!foundDo) continue;
    if (cur.length) segs.push(cur);
    if (segs.length >= 3 && segs[2].length === 1) {
      const s = segs[2][0];
      if (s.type === 'number' && Number(s.value) === 0) {
        add(s.start, s.end, level, '`for` 循环步长为 0，将造成死循环（无法推进）。', 'zero-step-loop');
      }
    }
  }

  // (d) 除零 / 对 0 取模
  {
    const divZeroRe = /[/%]\s*0(?![.\dxXa-fA-F])/g;
    let dz;
    while ((dz = divZeroRe.exec(mask))) {
      add(dz.index, dz.index + dz[0].length, level, '对 `0` 做除法或取模，结果会是无穷或 NaN。', 'divide-by-zero');
    }
  }

  // (e) 自我赋值 x = x
  for (let i = 0; i < tokens.length; i++) {
    const lhs = tokens[i];
    if (lhs.type !== 'name') continue;
    const eq = tokens[i + 1];
    const rhs = tokens[i + 2];
    if (!(eq && eq.value === '=' && rhs && rhs.type === 'name' && rhs.value === lhs.value)) continue;
    const prev = tokens[i - 1];
    if (prev && (prev.value === '.' || prev.value === ':')) continue;
    const after = tokens[i + 3];
    if (after && (after.type === 'op' || after.value === '(' || after.value === '[' || after.value === '.' || after.value === ':')) continue;
    add(lhs.start, rhs.end, level, `\`${lhs.value}\` 赋值给自己，没有任何效果。`, 'self-assignment');
  }

  // (f) 重复定义全局函数
  {
    const seen = new Set();
    for (let i = 0; i < tokens.length; i++) {
      const t = tokens[i];
      if (!(t.type === 'keyword' && t.value === 'function')) continue;
      const prev = tokens[i - 1];
      if (prev && prev.type === 'keyword' && prev.value === 'local') continue;
      let j = i + 1;
      let nameTok = null;
      while (j < tokens.length && tokens[j].type === 'name') {
        nameTok = tokens[j];
        j++;
        if (tokens[j] && (tokens[j].value === '.' || tokens[j].value === ':')) { j++; continue; }
        break;
      }
      if (!nameTok) continue;
      if (seen.has(nameTok.value)) {
        add(nameTok.start, nameTok.end, level, `函数 \`${nameTok.value}\` 重复定义，后一个会覆盖前一个。`, 'duplicate-function');
      } else {
        seen.add(nameTok.value);
      }
    }
  }
}

/** GC API 调用实参的字面量类型校验 */
function checkArgTypes(tokens, apiMap, add, level) {
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (t.type !== 'name') continue;
    let name = t.value;
    let j = i + 1;
    while (tokens[j] && tokens[j].value === '.' && tokens[j + 1] && tokens[j + 1].type === 'name') {
      name += '.' + tokens[j + 1].value;
      j += 2;
    }
    if (!tokens[j] || tokens[j].value !== '(') continue;
    const def = apiMap.get(name);
    if (!def || !def.params || def.params.length === 0) continue;
    const args = collectArgumentTokens(tokens, j);
    const n = Math.min(args.length, def.params.length);
    for (let k = 0; k < n; k++) {
      const expected = expectedLiteralTypes(def.params[k].type);
      if (!expected || !args[k].length) continue;
      const first = args[k][0];
      const actual = literalType(first);
      if (!actual || actual === 'nil') continue;
      if (!expected.includes(actual)) {
        add(first.start, first.end, level,
          `参数 \`${def.params[k].name}\` 期望 ${def.params[k].type}，但传入了${TYPE_LABEL[actual]}字面量。`,
          'argument-type');
      }
    }
  }
}

/**
 * 收集脚本中“已知的名称”。
 * declared：真正的局部声明（local 变量 / 函数参数 / for 变量 / local function），用于“缺少 local”检查
 * all：declared + 赋值目标 + 函数定义名，用于“未定义变量”检查
 */
function collectKnownNames(tokens) {
  const declared = new Set();
  const all = new Set();
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    // local 声明
    if (t.type === 'keyword' && t.value === 'local') {
      let j = i + 1;
      if (tokens[j] && tokens[j].type === 'keyword' && tokens[j].value === 'function') {
        if (tokens[j + 1] && tokens[j + 1].type === 'name') { declared.add(tokens[j + 1].value); all.add(tokens[j + 1].value); }
        continue;
      }
      while (j < tokens.length && tokens[j].type === 'name') {
        declared.add(tokens[j].value);
        all.add(tokens[j].value);
        j++;
        if (tokens[j] && tokens[j].value === ',') { j++; continue; }
        break;
      }
      continue;
    }
    // 函数定义（含参数）
    if (t.type === 'keyword' && t.value === 'function') {
      let j = i + 1;
      while (j < tokens.length && tokens[j].type === 'name') {
        all.add(tokens[j].value);
        j++;
        if (tokens[j] && (tokens[j].value === '.' || tokens[j].value === ':')) { j++; continue; }
        break;
      }
      let k = j;
      while (k < tokens.length && tokens[k].value !== '(') k++;
      if (tokens[k] && tokens[k].value === '(') {
        let d = 0;
        for (let m = k + 1; m < tokens.length; m++) {
          const u = tokens[m];
          if (u.value === '(') d++;
          else if (u.value === ')') { if (d === 0) break; d--; }
          else if (u.type === 'name' && d === 0) {
            const p = tokens[m - 1];
            if (p && (p.value === '(' || p.value === ',')) { declared.add(u.value); all.add(u.value); }
          }
        }
      }
      continue;
    }
    // for 循环变量
    if (t.type === 'keyword' && t.value === 'for') {
      for (let j = i + 1; j < tokens.length; j++) {
        const u = tokens[j];
        if (u.type === 'keyword' && (u.value === 'do' || u.value === 'in')) break;
        if (u.type === 'name') { declared.add(u.value); all.add(u.value); }
      }
      continue;
    }
    // 赋值目标
    if (t.type === 'name') {
      const prev = tokens[i - 1];
      const next = tokens[i + 1];
      if (next && next.value === '=' && (!prev || (prev.value !== '.' && prev.value !== ':' && prev.value !== 'local'))) {
        all.add(t.value);
      }
    }
  }
  return { declared, all };
}

/** 冗余代码：恒真/恒假条件、恒真导致的冗余分支、空代码块 */
function checkRedundant(tokens, add, level) {
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && (t.value === 'if' || t.value === 'elseif'))) continue;
    const c = tokens[i + 1];
    const after = tokens[i + 2];
    if (!(c && c.type === 'keyword' && (c.value === 'true' || c.value === 'false'))) continue;
    if (!(after && after.type === 'keyword' && after.value === 'then')) continue;
    const cond = c.value;
    add(t.start, c.end, level,
      `条件恒为 \`${cond}\`，该判断没有实际意义${cond === 'true' ? '（`else` 分支永远不会执行）' : '（`then` 分支永远不会执行）'}。`,
      'redundant-condition');
    if (cond === 'true') {
      const endIdx = findBlockEndIndex(tokens, i);
      if (endIdx > 0) {
        const elseTok = tokens.slice(i + 3, endIdx).find(u => u.type === 'keyword' && u.value === 'else');
        if (elseTok) {
          add(elseTok.start, elseTok.end, level, '该 `else` 分支永远不会执行（条件恒为 `true`）。', 'redundant-branch');
        }
      }
    }
  }

  for (let i = 0; i < tokens.length - 1; i++) {
    const t = tokens[i];
    const n = tokens[i + 1];
    if (t.type === 'keyword' && (t.value === 'then' || t.value === 'do') &&
      n.type === 'keyword' && n.value === 'end') {
      add(t.start, n.end, level, `代码块为空：\`${t.value}\` 与 \`end\` 之间没有任何内容。`, 'empty-block');
    }
  }
}

/** 不可达代码：return / break 之后同一代码块内仍有语句 */
function checkUnreachable(tokens, add, level) {
  const BLOCK_END = new Set(['end', 'else', 'elseif', 'until']);
  const CONTINUE_TOKENS = new Set([
    ',', '(', '[', '{', '..', '+', '-', '*', '/', '%', '^',
    '==', '~=', '<=', '>=', '<', '>', '=', 'and', 'or'
  ]);
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && (t.value === 'return' || t.value === 'break'))) continue;
    let prev = t;
    for (let j = i + 1; j < tokens.length; j++) {
      const u = tokens[j];
      if (u.type === 'keyword' && BLOCK_END.has(u.value)) break;
      if (u.line > prev.line && !CONTINUE_TOKENS.has(prev.value)) {
        add(u.start, u.end, level,
          `\`${t.value}\` 之后的语句永远不会执行（不可达代码）。`, 'unreachable-code');
        break;
      }
      prev = u;
    }
  }
}

/** 未定义变量（提示）：读取了既未声明也未赋值的名称 */
function checkUndefinedVariables(tokens, add, level, knownNames) {
  const IGNORE = new Set(['self', '_ENV', '_G', '_VERSION', 'arg']);
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (t.type !== 'name') continue;
    if (IGNORE.has(t.value) || knownNames.has(t.value)) continue;
    const prev = tokens[i - 1];
    const next = tokens[i + 1];
    if (prev && (prev.value === '.' || prev.value === ':')) continue;          // 成员访问
    if (prev && prev.type === 'keyword' && prev.value === 'function') continue; // 函数名定义
    if (prev && prev.value === 'goto') continue;
    if (next && (next.value === '=' || next.value === '(' ||
      next.value === '[' || next.value === '.' || next.value === ':' || next.value === '::')) continue;
    add(t.start, t.end, level,
      `变量 \`${t.value}\` 未定义：请检查拼写，或先用 \`local\` 声明。`, 'undefined-variable');
  }
}

/** 变量定义缺少 local（函数体内对非局部名赋值） */
function checkMissingLocal(tokens, add, level, declaredLocals, functionScopes) {
  const isInFunction = (off) => functionScopes.some(s => off >= s.start && off <= s.end);
  const reported = new Set();
  let braceDepth = 0;
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    // 跟踪表构造函数深度：`{ ... }` 内的 `key = value` 是表字段，不是变量定义
    if (t.value === '{') { braceDepth++; continue; }
    if (t.value === '}') { if (braceDepth > 0) braceDepth--; continue; }
    if (braceDepth > 0) continue;
    if (t.type !== 'name') continue;
    const prev = tokens[i - 1];
    const next = tokens[i + 1];
    if (!(next && next.value === '=')) continue;
    if (prev && (prev.value === '.' || prev.value === ':' || prev.value === 'local')) continue;
    if (!isInFunction(t.start)) continue;              // 顶层赋值由“顶层执行”检查负责
    if (declaredLocals.has(t.value)) continue;
    if (reported.has(t.value)) continue;
    reported.add(t.value);
    add(t.start, t.end, level,
      `\`${t.value}\` 不是局部变量：定义变量时应使用 \`local\`（如 \`local ${t.value} = ...\`），避免创建全局变量。`,
      'missing-local');
  }
}

/** 缺少入口函数（仅在文件已定义至少一个入口函数时检查，避免对普通 Lua 文件误报） */
function checkMissingEntry(tokens, add, level) {
  const ENTRY = ['initScript', 'main', 'render', 'saveConfig'];
  const defined = new Set();
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && t.value === 'function')) continue;
    const nt = tokens[i + 1];
    if (nt && nt.type === 'name' && ENTRY.includes(nt.value)) defined.add(nt.value);
  }
  if (defined.size === 0) return;
  const missing = ENTRY.filter(n => !defined.has(n));
  if (missing.length === 0) return;
  const first = tokens[0];
  add(first ? first.start : 0, first ? first.end : 1, level,
    `缺少入口函数：${missing.map(m => '`' + m + '`').join('、')}。GC 脚本应定义 initScript / main / render / saveConfig 四个入口函数。`,
    'missing-entry');
}

/** 重复定义入口函数（错误） */
function checkDuplicateEntry(tokens, add, level) {
  const ENTRY = new Set(['initScript', 'main', 'render', 'saveConfig']);
  const seen = new Set();
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (!(t.type === 'keyword' && t.value === 'function')) continue;
    const nt = tokens[i + 1];
    if (!(nt && nt.type === 'name' && ENTRY.has(nt.value))) continue;
    if (seen.has(nt.value)) {
      add(nt.start, nt.end, level, `入口函数 \`${nt.value}\` 重复定义，只允许定义一次。`, 'duplicate-entry');
    } else {
      seen.add(nt.value);
    }
  }
}

/** 顶层可执行代码（错误）：GC 脚本不允许自动执行的 chunk */
function checkTopLevelExecution(tokens, add, level, functionScopes) {
  const isInFunction = (off) => functionScopes.some(s => off >= s.start && off <= s.end);
  const STMT_START = new Set([';', 'then', 'do', 'else', 'end', 'repeat', 'until']);
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (isInFunction(t.start)) continue;
    if (t.type !== 'keyword' && t.type !== 'name') continue;
    const prev = tokens[i - 1];
    const next = tokens[i + 1];
    // 顶层控制流
    if (t.type === 'keyword' && (t.value === 'if' || t.value === 'for' ||
      t.value === 'while' || t.value === 'repeat' || t.value === 'do')) {
      add(t.start, t.end, level,
        `GC 脚本不允许在顶层编写可执行代码（\`${t.value}\` 语句）；请把逻辑放进入口函数里。`, 'top-level-execution');
      continue;
    }
    if (t.type !== 'name') continue;
    const memberAccess = prev && (prev.value === '.' || prev.value === ':');
    const funcDef = prev && prev.type === 'keyword' && (prev.value === 'function' || prev.value === 'local');
    const atStart = !prev || prev.type === 'keyword' || STMT_START.has(prev.value);
    if (memberAccess || funcDef || !atStart) continue;
    if (next && next.value === '(') {
      add(t.start, t.end, level,
        `GC 脚本不允许在顶层直接调用 \`${t.value}()\`（会自动执行）；请把它移到入口函数中。`, 'top-level-execution');
    } else if (next && next.value === '=') {
      add(t.start, t.end, level,
        `GC 脚本不允许在顶层执行赋值语句；请使用 \`local\` 声明，或在入口函数中赋值。`, 'top-level-execution');
    }
  }
}

/**
 * 主入口
 * @param {string} text 文档全文
 * @param {object} options { enable, undefinedFunction, argumentCount, deprecated }
 * @returns {Array<{range:{start:{line,character},end:{line,character}}, severity:string, message:string, code:string}>}
 */
function computeDiagnostics(text, options) {
  const opts = Object.assign(
    {
      enable: true, undefinedFunction: 'hint', undefinedVariable: 'hint',
      argumentCount: 'warning', deprecated: true, contextScope: 'warning',
      unused: 'hint', redundant: 'hint', suspicious: 'warning',
      entry: 'warning', missingLocal: 'warning', topLevelCode: 'error', duplicateEntry: 'error',
      typeCheck: 'warning'
    },
    options || {}
  );
  if (!opts.enable) return [];

  const lineStarts = computeLineStarts(text);
  const { tokens, errors } = scanner.tokenize(text);
  const mask = scanner.buildCodeMask(text, tokens);
  const diags = [];

  const makeRange = (startOff, endOff) => ({
    start: offsetToPos(lineStarts, startOff),
    end: offsetToPos(lineStarts, endOff)
  });

  const add = (startOff, endOff, severity, message, code) => {
    if (severity === 'off' || !SEVERITY[severity]) return;
    diags.push({ range: makeRange(startOff, endOff), severity, message, code });
  };

  // ---------- 1. 词法错误（未闭合字符串/注释） ----------
  for (const e of errors) {
    add(e.start, Math.max(e.end, e.start + 1), 'error', e.message, e.code);
  }

  // ---------- 2. 块关键字配对 ----------
  for (const d of scanner.checkBlockPairs(tokens)) {
    add(d.token.start, d.token.end, 'error', d.message, d.code);
  }

  // ---------- 3. 括号配对 ----------
  for (const d of scanner.checkBrackets(tokens)) {
    add(d.token.start, d.token.end, 'error', d.message, d.code);
  }

  // 语义检查依赖的集合
  const definedFns = scanner.collectDefinedFunctions(tokens);
  const assigned = scanner.collectAssignedNames(tokens);
  const apiMap = new Map(api.GC_FUNCTIONS.map(f => [f.name, f]));
  const globalSet = new Set([...api.GC_API_NAMES, ...api.LUA_STDLIB.globals, ...Object.keys(api.LUA_STDLIB.libraries)]);

  // 构建函数作用域，用于判断 API 调用是否在 render 函数内
  const functionScopes = scanner.buildFunctionScopes(tokens);
  const isInRender = (offset) => functionScopes.some(s => s.name === 'render' && offset >= s.start && offset <= s.end);

  // ---------- 4. 被移除的标准库 ----------
  {
    const removedLibRe = /\b(io|package|debug)\s*\./g;
    let m;
    while ((m = removedLibRe.exec(mask))) {
      add(m.index, m.index + m[1].length, 'error', `\`${m[1]}\` 库已被 GC 脚本沙箱整体移除，无法使用。`, 'removed-library');
    }
    const removedFnRe = /\b(dofile|loadfile|loadstring|load|collectgarbage)\s*\(/g;
    while ((m = removedFnRe.exec(mask))) {
      add(m.index, m.index + m[1].length, 'error', `\`${m[1]}\` 已被 GC 脚本沙箱移除，调用会报错。`, 'removed-function');
    }
    const osDangerRe = /\bos\.(execute|remove|rename|exit|tmpname|getenv)\b/g;
    while ((m = osDangerRe.exec(mask))) {
      add(m.index, m.index + m[0].length, 'error', `\`${m[0]}\` 已被沙箱移除，无法使用。`, 'removed-os');
    }
  }

  // ---------- 5. 调用扫描（API 参数数量、未定义函数、弃用、常量表） ----------
  const callRe = /([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)\s*\(/g;
  let m;
  while ((m = callRe.exec(mask))) {
    const rawName = m[1];
    const nameStart = m.index;
    const openIdx = m.index + m[1].length + (m[0].length - m[1].length - 1);
    const nameEnd = nameStart + rawName.length;

    // 关键字引导的定义/语句，跳过
    if (scanner.KEYWORDS.has(rawName)) continue;

    const args = countArguments(mask, openIdx);

    // 5a. GC API 参数数量
    if (apiMap.has(rawName) && args) {
      const def = apiMap.get(rawName);
      const required = def.params.filter(p => !p.optional).length;
      const max = def.params.length;
      if (args.count < required) {
        add(nameStart, nameEnd, opts.argumentCount, `\`${rawName}\` 参数过少：需要 ${required} 个参数，实际传入 ${args.count} 个。`, 'argument-count');
      } else if (args.count > max) {
        add(nameStart, nameEnd, opts.argumentCount, `\`${rawName}\` 参数过多：最多 ${max} 个参数，实际传入 ${args.count} 个。`, 'argument-count');
      }
    }

    // 5b. 弃用 API
    if (opts.deprecated && api.GC_DEPRECATED[rawName]) {
      const dep = api.GC_DEPRECATED[rawName];
      const fix = dep.replacement ? ` 建议改用 \`${dep.replacement}\`。` : '';
      add(nameStart, nameEnd, 'warning', `\`${rawName}\` 已弃用：${dep.reason}${fix}`, 'deprecated-api');
    }

    // 5c. 未定义函数（仅检查首字母大写、且非成员/方法调用的名称，降低误报）
    const isMember = rawName.includes('.');
    const isMethodCall = nameStart > 0 && mask[nameStart - 1] === ':';
    const isPascal = /^[A-Z]/.test(rawName);
    if (!isMember && !isMethodCall && isPascal && !apiMap.has(rawName) && !globalSet.has(rawName) &&
      !definedFns.has(rawName) && !assigned.has(rawName) && !api.GC_DEPRECATED[rawName]) {
      add(nameStart, nameEnd, opts.undefinedFunction, `未定义的函数 \`${rawName}\`。GC Lua 没有该 API，请检查拼写。`, 'undefined-function');
    }

    // 5d. minecraft 命名空间
    if (rawName.startsWith('minecraft.')) {
      const parts = rawName.split('.');
      if (parts[1] !== 'chat') {
        add(nameStart, nameEnd, 'warning', `\`minecraft\` 命名空间下没有 \`${parts[1]}\`，仅提供 \`chat\`。`, 'unknown-namespace');
      } else if (parts[2] && !['OpenReader', 'GetNewMsg'].includes(parts[2])) {
        add(nameStart, nameEnd, 'warning', `\`minecraft.chat\` 没有成员 \`${parts[2]}\`，可用成员：\`OpenReader\`、\`GetNewMsg\`。`, 'unknown-member');
      }
    }

    // 5e. 上下文 API 限制：render 函数内只能用 UI 系列，非 render 函数内不能用 UI 系列
    if (opts.contextScope !== 'off' && apiMap.has(rawName)) {
      const def = apiMap.get(rawName);
      const inRender = isInRender(nameStart);
      if (inRender && def.category !== 'gui') {
        add(nameStart, nameEnd, opts.contextScope,
          `\`${rawName}\` 不是 UI 系列 API，不应在 render 函数中使用。render 函数中只能使用 UI 系列的 API（如 Button/Text/Slider 等）。`,
          'non-ui-in-render');
      }
      if (!inRender && def.category === 'gui') {
        add(nameStart, nameEnd, opts.contextScope,
          `\`${rawName}\` 是 UI 系列 API，只能在 render 函数中使用。main 函数和其他函数中不允许使用 UI 系列的 API。`,
          'ui-in-non-render');
      }
    }
  }

  // ---------- 6. 常量表成员校验（KEY. / SC. / GK.） ----------
  {
    const keyRe = /\bKEY\.([A-Za-z0-9_]+)/g;
    let km;
    while ((km = keyRe.exec(mask))) {
      if (!Object.prototype.hasOwnProperty.call(api.KEY_CONSTANTS, km[1])) {
        add(km.index, km.index + km[0].length, 'warning', `\`KEY\` 表不存在键 \`${km[1]}\`。仅支持 A~Z、0~9 与 \`VK_*\` 常量。`, 'unknown-key');
      }
    }
    const scRe = /\bSC\.([A-Za-z0-9_]+)/g;
    let sm;
    while ((sm = scRe.exec(mask))) {
      if (!Object.prototype.hasOwnProperty.call(api.SC_CONSTANTS, sm[1])) {
        add(sm.index, sm.index + sm[0].length, 'warning', `\`SC\` 表不存在成员 \`${sm[1]}\`。可用：\`MOUSE_EVENT\`、\`SEND_INPUT\`、\`POST_MESSAGE\`。`, 'unknown-sc');
      }
    }
    const gkRe = /\bGK\.([A-Za-z0-9_]+)/g;
    let gm;
    while ((gm = gkRe.exec(mask))) {
      if (!Object.prototype.hasOwnProperty.call(api.GK_CONSTANTS, gm[1])) {
        add(gm.index, gm.index + gm[0].length, 'warning', `\`GK\` 表不存在成员 \`${gm[1]}\`。可用：\`HOOK\`、\`BOTH\`。`, 'unknown-gk');
      }
    }
  }

  // ---------- 7. 括号成对 API（BeginChild/EndChild、BeginGroup/EndGroup） ----------
  {
    const pairDefs = [
      { start: 'BeginChild', end: 'EndChild' },
      { start: 'BeginGroup', end: 'EndGroup' }
    ];
    for (const p of pairDefs) {
      const startCount = (mask.match(new RegExp(`\\b${p.start}\\s*\\(`, 'g')) || []).length;
      const endCount = (mask.match(new RegExp(`\\b${p.end}\\s*\\(`, 'g')) || []).length;
      if (startCount !== endCount) {
        const first = mask.search(new RegExp(`\\b${startCount > endCount ? p.start : p.end}\\b`));
        if (first >= 0) {
          add(first, first + (startCount > endCount ? p.start : p.end).length, 'warning',
            `\`${p.start}\` 与 \`${p.end}\` 必须成对出现：当前 ${p.start} 出现 ${startCount} 次，${p.end} 出现 ${endCount} 次。`, 'unbalanced-pair');
        }
      }
    }
  }

  // ---------- 8. 未使用的局部变量 / 函数 ----------
  if (SEVERITY[opts.unused]) {
    checkUnusedLocals(tokens, add, opts.unused);
    checkUnusedFunctions(tokens, add, opts.unused);
  }

  // ---------- 9. 冗余代码 ----------
  if (SEVERITY[opts.redundant]) {
    checkRedundant(tokens, add, opts.redundant);
    checkUnreachable(tokens, add, opts.redundant);
  }

  // ---------- 10. 可疑逻辑 ----------
  if (SEVERITY[opts.suspicious]) {
    checkSuspicious(tokens, mask, add, opts.suspicious);
  }

  // ---------- 11. 实参类型校验 ----------
  if (SEVERITY[opts.typeCheck]) {
    checkArgTypes(tokens, apiMap, add, opts.typeCheck);
  }

  // ---------- 12. 入口函数（缺失 / 重复） ----------
  if (SEVERITY[opts.entry]) {
    checkMissingEntry(tokens, add, opts.entry);
  }
  if (SEVERITY[opts.duplicateEntry]) {
    checkDuplicateEntry(tokens, add, opts.duplicateEntry);
  }

  // ---------- 13. 顶层可执行代码（不允许自动执行的 chunk） ----------
  if (SEVERITY[opts.topLevelCode]) {
    checkTopLevelExecution(tokens, add, opts.topLevelCode, functionScopes);
  }

  // ---------- 14. 未定义变量 / 缺少 local ----------
  {
    const { declared: declaredLocals, all: knownNames } = collectKnownNames(tokens);
    for (const n of api.GC_API_NAMES) knownNames.add(n);
    for (const n of api.LUA_STDLIB.globals) knownNames.add(n);
    for (const lib of Object.keys(api.LUA_STDLIB.libraries)) knownNames.add(lib);
    knownNames.add('minecraft');
    for (const n of ['KEY', 'SC', 'GK']) knownNames.add(n);
    if (SEVERITY[opts.undefinedVariable]) {
      checkUndefinedVariables(tokens, add, opts.undefinedVariable, knownNames);
    }
    if (SEVERITY[opts.missingLocal]) {
      checkMissingLocal(tokens, add, opts.missingLocal, declaredLocals, functionScopes);
    }
  }

  return diags;
}

module.exports = { computeDiagnostics, countArguments, offsetToPos, computeLineStarts };