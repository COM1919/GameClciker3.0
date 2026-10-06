'use strict';
/**
 * GC Lua 轻量扫描器
 * 不依赖完整 Lua 解析器，提供 tokenizer 与结构性检查所需的基础能力。
 * 所有位置信息均为 0 基（line / col），与 VSCode 的 Position 对齐。
 */

const KEYWORDS = new Set([
  'and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for', 'function', 'goto',
  'if', 'in', 'local', 'nil', 'not', 'or', 'repeat', 'return', 'then', 'true', 'until', 'while'
]);

const isNameStart = (c) => /[A-Za-z_]/.test(c);
const isNamePart = (c) => /[A-Za-z0-9_]/.test(c);
const isDigit = (c) => /[0-9]/.test(c);

/**
 * 词法分析
 * @param {string} text
 * @returns {{tokens: Array, errors: Array}}
 */
function tokenize(text) {
  const tokens = [];
  const errors = [];
  let i = 0;
  let line = 0;
  let lineStart = 0;
  const len = text.length;

  const push = (type, value, start, end, l, c) => {
    tokens.push({ type, value, start, end, line: l, col: c });
  };

  const colOf = (pos, lStart) => pos - lStart;

  while (i < len) {
    const ch = text[i];

    // 换行
    if (ch === '\n') {
      i++;
      line++;
      lineStart = i;
      continue;
    }
    if (ch === '\r') {
      i++;
      continue;
    }
    // 空白
    if (ch === ' ' || ch === '\t' || ch === '\f' || ch === '\v') {
      i++;
      continue;
    }

    const start = i;
    const sLine = line;
    const sCol = colOf(start, lineStart);

    // 注释
    if (ch === '-' && text[i + 1] === '-') {
      // 长注释 --[[ ... ]] / --[==[ ... ]==]
      const longOpen = tryLongBracket(text, i + 2);
      if (longOpen) {
        const end = findLongBracketEnd(text, longOpen.level, longOpen.contentStart);
        if (end < 0) {
          errors.push({ message: '未闭合的块注释（缺少 `]]`）', start, end: len, line: sLine, col: sCol, code: 'unterminated-comment' });
          i = len;
        } else {
          push('comment', text.slice(start, end), start, end, sLine, sCol);
          ({ line, lineStart } = advance(text, start, end, line, lineStart));
          i = end;
        }
      } else {
        // 行注释
        let end = text.indexOf('\n', i);
        if (end < 0) end = len;
        push('comment', text.slice(start, end), start, end, sLine, sCol);
        i = end;
      }
      continue;
    }

    // 长字符串 [[ ... ]]
    const longStr = tryLongBracket(text, i);
    if (longStr) {
      const end = findLongBracketEnd(text, longStr.level, longStr.contentStart);
      if (end < 0) {
        errors.push({ message: '未闭合的长字符串（缺少 `]]`）', start, end: len, line: sLine, col: sCol, code: 'unterminated-string' });
        i = len;
      } else {
        push('string', text.slice(start, end), start, end, sLine, sCol);
        ({ line, lineStart } = advance(text, start, end, line, lineStart));
        i = end;
      }
      continue;
    }

    // 字符串
    if (ch === '"' || ch === "'") {
      const quote = ch;
      let j = i + 1;
      let closed = false;
      while (j < len) {
        const cj = text[j];
        if (cj === '\\') { j += 2; continue; }
        if (cj === '\n') break;
        if (cj === quote) { closed = true; j++; break; }
        j++;
      }
      if (!closed) {
        errors.push({ message: `未闭合的字符串（缺少 \`${quote}\`）`, start, end: j, line: sLine, col: sCol, code: 'unterminated-string' });
      }
      const end = Math.min(j, len);
      push('string', text.slice(start, end), start, end, sLine, sCol);
      ({ line, lineStart } = advance(text, start, end, line, lineStart));
      i = end;
      continue;
    }

    // 数字
    if (isDigit(ch) || (ch === '.' && isDigit(text[i + 1]))) {
      let j = i;
      if (ch === '0' && (text[i + 1] === 'x' || text[i + 1] === 'X')) {
        j = i + 2;
        while (j < len && /[0-9a-fA-F.]/.test(text[j])) j++;
      } else {
        while (j < len && /[0-9.eE+\-]/.test(text[j])) {
          // 避免吃掉后续的 .. 连接符
          if (text[j] === '.' && text[j + 1] === '.') break;
          if ((text[j] === '+' || text[j] === '-') && !/[eE]/.test(text[j - 1])) break;
          j++;
        }
      }
      push('number', text.slice(i, j), i, j, sLine, sCol);
      i = j;
      continue;
    }

    // 名称 / 关键字
    if (isNameStart(ch)) {
      let j = i + 1;
      while (j < len && isNamePart(text[j])) j++;
      const word = text.slice(i, j);
      push(KEYWORDS.has(word) ? 'keyword' : 'name', word, i, j, sLine, sCol);
      i = j;
      continue;
    }

    // 多字符运算符
    const three = text.substr(i, 3);
    const two = text.substr(i, 2);
    if (three === '...') {
      push('op', '...', i, i + 3, sLine, sCol);
      i += 3;
      continue;
    }
    if (['==', '~=', '<=', '>=', '..', '::'].includes(two)) {
      push('op', two, i, i + 2, sLine, sCol);
      i += 2;
      continue;
    }

    // 单字符
    const type = '()[]{}'.includes(ch) ? 'punct' : 'op';
    push(type, ch, i, i + 1, sLine, sCol);
    i++;
  }

  return { tokens, errors };
}

/** 尝试匹配长括号 [[ / [=[ ... */
function tryLongBracket(text, pos) {
  if (text[pos] !== '[') return null;
  let j = pos + 1;
  let level = 0;
  while (text[j] === '=') { level++; j++; }
  if (text[j] !== '[') return null;
  return { level, contentStart: j + 1 };
}

/** 查找长括号结束位置 */
function findLongBracketEnd(text, level, from) {
  const closer = ']' + '='.repeat(level) + ']';
  const idx = text.indexOf(closer, from);
  return idx < 0 ? -1 : idx + closer.length;
}

/** 推进行计数 */
function advance(text, start, end, line, lineStart) {
  for (let k = start; k < end; k++) {
    if (text[k] === '\n') {
      line++;
      lineStart = k + 1;
    }
  }
  return { line, lineStart };
}

/**
 * 生成“代码掩码”：把注释与字符串内容替换为空格（保留换行），
 * 使得基于正则的扫描不会命中注释与字符串中的内容。
 * 返回与原文等长的字符串。
 */
function buildCodeMask(text, tokens) {
  const chars = text.split('');
  for (const t of tokens) {
    if (t.type === 'comment') {
      for (let k = t.start; k < t.end; k++) {
        if (chars[k] !== '\n') chars[k] = ' ';
      }
    } else if (t.type === 'string') {
      // 保留引号本身，内容替换为空格，便于识别字符串参数位置
      for (let k = t.start; k < t.end; k++) {
        const c = chars[k];
        if (c !== '\n' && c !== '"' && c !== "'" && c !== '[' && c !== ']') chars[k] = ' ';
      }
    }
  }
  return chars.join('');
}

/**
 * 收集脚本中定义的函数名（全局与局部函数）
 * @returns {Set<string>}
 */
function collectDefinedFunctions(tokens) {
  const defined = new Set();
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (t.type === 'keyword' && t.value === 'function') {
      // function a.b.c / function a:b
      let j = i + 1;
      const parts = [];
      while (j < tokens.length && tokens[j].type === 'name') {
        parts.push(tokens[j].value);
        j++;
        if (tokens[j] && tokens[j].value === '.') { j++; continue; }
        if (tokens[j] && tokens[j].value === ':') { j++; continue; }
        break;
      }
      if (parts.length) {
        defined.add(parts[0]);
        defined.add(parts[parts.length - 1]);
      }
    }
  }
  return defined;
}

/**
 * 收集被赋值的名称（用于把局部/全局变量视为“已定义”）
 * @returns {Set<string>}
 */
function collectAssignedNames(tokens) {
  const names = new Set();
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (t.type === 'keyword' && t.value === 'local') {
      let j = i + 1;
      while (j < tokens.length && tokens[j].type === 'name') {
        names.add(tokens[j].value);
        j++;
        if (tokens[j] && tokens[j].value === ',') { j++; continue; }
        break;
      }
    }
    if (t.type === 'name' && tokens[i + 1] && tokens[i + 1].value === '=') {
      // 简单赋值目标（不含 . / [ 的成员赋值）
      const prev = tokens[i - 1];
      if (!prev || (prev.value !== '.' && prev.value !== ':' && prev.value !== 'local')) {
        names.add(t.value);
      }
    }
  }
  return names;
}

/**
 * 构建函数作用域列表
 * 遍历 token 流，跟踪 function ... end 块，记录函数名与字符偏移范围。
 * 用于判断某个偏移量是否在特定函数（如 render）内。
 * @returns {Array<{name:string, start:number, end:number}>} start/end 为字符偏移
 */
function buildFunctionScopes(tokens) {
  const scopes = [];
  const stack = [];
  const openers = { function: 'function', if: 'if', do: 'do', repeat: 'repeat' };

  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i];
    if (t.type !== 'keyword') continue;

    if (openers[t.value]) {
      if (t.value === 'function') {
        let j = i + 1;
        const parts = [];
        while (j < tokens.length && tokens[j].type === 'name') {
          parts.push(tokens[j].value);
          j++;
          if (tokens[j] && tokens[j].value === '.') { j++; continue; }
          if (tokens[j] && tokens[j].value === ':') { j++; continue; }
          break;
        }
        const name = parts.length ? parts[parts.length - 1] : '';
        stack.push({ kind: 'function', name, startOff: t.start });
      } else {
        stack.push({ kind: openers[t.value], startOff: t.start });
      }
    } else if (t.value === 'end') {
      if (stack.length > 0) {
        const top = stack[stack.length - 1];
        if (top.kind === 'function') {
          scopes.push({ name: top.name, start: top.startOff, end: t.end });
        }
        stack.pop();
      }
    } else if (t.value === 'until') {
      if (stack.length > 0 && stack[stack.length - 1].kind === 'repeat') {
        stack.pop();
      }
    }
  }
  return scopes;
}

/**
 * 结构性检查：块关键字配对（function/if/do/repeat ... end/until）
 * @returns {Array<{message, start, end, line, col, severity, code}>}
 */
function checkBlockPairs(tokens) {
  const diags = [];
  const stack = [];
  const openers = { function: 'function', if: 'if', do: 'do', repeat: 'repeat' };

  for (const t of tokens) {
    if (t.type !== 'keyword') continue;
    if (openers[t.value]) {
      stack.push({ kind: openers[t.value], token: t });
    } else if (t.value === 'end') {
      if (stack.length === 0) {
        diags.push({ message: '多余的 `end`，找不到与之匹配的 `function` / `if` / `do`', token: t, code: 'unexpected-end' });
      } else {
        const top = stack[stack.length - 1];
        if (top.kind === 'repeat') {
          diags.push({ message: `\`repeat\` 块应当以 \`until\` 结束，而非 \`end\``, token: t, code: 'mismatched-end' });
          stack.pop();
        } else {
          stack.pop();
        }
      }
    } else if (t.value === 'until') {
      if (stack.length === 0) {
        diags.push({ message: '多余的 `until`，找不到与之匹配的 `repeat`', token: t, code: 'unexpected-until' });
      } else {
        const top = stack[stack.length - 1];
        if (top.kind !== 'repeat') {
          diags.push({ message: `\`until\` 只能用于结束 \`repeat\` 块；当前未闭合的是 \`${top.kind}\``, token: t, code: 'mismatched-until' });
        } else {
          stack.pop();
        }
      }
    }
  }

  for (const item of stack) {
    const label = item.kind === 'function' ? '函数' : item.kind === 'if' ? 'if 语句' : item.kind === 'do' ? 'do 块' : 'repeat 块';
    diags.push({
      message: `未闭合的 ${label}（缺少 \`${item.kind === 'repeat' ? 'until' : 'end'}\`）`,
      token: item.token,
      code: 'unclosed-block'
    });
  }
  return diags;
}

/**
 * 括号配对检查
 * @returns {Array}
 */
function checkBrackets(tokens) {
  const diags = [];
  const pairs = { ')': '(', ']': '[', '}': '{' };
  const stack = [];
  for (const t of tokens) {
    if (t.type !== 'punct') continue;
    if (t.value === '(' || t.value === '[' || t.value === '{') {
      stack.push(t);
    } else if (pairs[t.value]) {
      if (stack.length === 0) {
        diags.push({ message: `多余的 \`${t.value}\`，找不到与之匹配的 \`${pairs[t.value]}\``, token: t, code: 'unexpected-bracket' });
      } else {
        const top = stack[stack.length - 1];
        if (top.value !== pairs[t.value]) {
          diags.push({ message: `括号不匹配：\`${top.value}\` 需要对应的 \`${pairs[top.value] ? Object.keys(pairs).find(k => pairs[k] === top.value) : ''}\`，却遇到 \`${t.value}\``, token: t, code: 'mismatched-bracket' });
          stack.pop();
        } else {
          stack.pop();
        }
      }
    }
  }
  for (const t of stack) {
    const closer = t.value === '(' ? ')' : t.value === '[' ? ']' : '}';
    diags.push({ message: `未闭合的 \`${t.value}\`（缺少 \`${closer}\`）`, token: t, code: 'unclosed-bracket' });
  }
  return diags;
}

module.exports = {
  KEYWORDS,
  tokenize,
  buildCodeMask,
  collectDefinedFunctions,
  collectAssignedNames,
  buildFunctionScopes,
  checkBlockPairs,
  checkBrackets
};