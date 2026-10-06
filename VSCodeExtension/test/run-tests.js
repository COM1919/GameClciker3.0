'use strict';
/**
 * GC Lua 扩展核心逻辑自测（纯 Node，不依赖 vscode）
 * 运行：node test/run-tests.js
 */

const path = require('path');
const fs = require('fs');

// 让 require('vscode') 返回极简替身，以便测试依赖 vscode API 的纯逻辑
const Module = require('module');
const originalLoad = Module._load;
Module._load = function (request, parent, isMain) {
  if (request === 'vscode') return require('./vscode-mock');
  return originalLoad.call(this, request, parent, isMain);
};

const scanner = require('../src/luaScanner');
const { computeDiagnostics } = require('../src/diagnostics');
const api = require('../data/api');

let passed = 0;
let failed = 0;
const failures = [];

function check(name, condition, detail) {
  if (condition) {
    passed++;
    console.log(`  ✓ ${name}`);
  } else {
    failed++;
    failures.push(`${name}${detail ? ' -> ' + detail : ''}`);
    console.log(`  ✗ ${name}${detail ? ' -> ' + detail : ''}`);
  }
}

function codes(text, options) {
  return computeDiagnostics(text, options).map(d => d.code);
}

function hasCode(text, code, options) {
  return codes(text, options).includes(code);
}

console.log('\n[1] JSON 配置文件校验');
for (const f of ['package.json', 'language-configuration.json', 'syntaxes/gclua.tmLanguage.json', 'snippets/gclua.json', 'themes/vs2026-dark.json', 'themes/dark_modern.json']) {
  const p = path.join(__dirname, '..', f);
  try {
    JSON.parse(fs.readFileSync(p, 'utf8'));
    check(`${f} 是合法 JSON`, true);
  } catch (e) {
    check(`${f} 是合法 JSON`, false, e.message);
  }
}

console.log('\n[2] 词法分析');
{
  const { tokens, errors } = scanner.tokenize('local a = 1 -- 注释\nlocal s = "字符串(有括号)"');
  check('无词法错误', errors.length === 0);
  check('包含注释 token', tokens.some(t => t.type === 'comment'));
  check('字符串被识别', tokens.some(t => t.type === 'string'));
  const mask = scanner.buildCodeMask('local s = "a(b)"', tokens);
  check('掩码中字符串内括号被屏蔽', !mask.includes('a(b)'));
}
{
  const { errors } = scanner.tokenize('local s = "未闭合');
  check('识别未闭合字符串', errors.some(e => e.code === 'unterminated-string'));
}

console.log('\n[3] 结构性诊断');
check('多余 end', hasCode('end', 'unexpected-end'));
check('未闭合 function', hasCode('function main()\n  PrintLog("x", 1)\n', 'unclosed-block'));
check('未闭合左括号', hasCode('Text("a"\n', 'unclosed-bracket'));
check('括号不匹配', hasCode('Text(]\n', 'mismatched-bracket'));
check('repeat 应以 until 结束', hasCode('repeat\n  Sleep(1)\nend\n', 'mismatched-end'));

console.log('\n[4] 语义诊断');
check('未定义函数', hasCode('FooBar()\n', 'undefined-function'));
check('参数过少', hasCode('SetKeyClick()\n', 'argument-count'));
check('参数过多', hasCode('Text("a", "b")\n', 'argument-count'));
check('弃用 API', hasCode('Helpmarker("x")\n', 'deprecated-api'));
check('移除的库', hasCode('io.open("a")\n', 'removed-library'));
check('移除的全局函数', hasCode('dofile("a.lua")\n', 'removed-function'));
check('移除的 os 函数', hasCode('os.execute("x")\n', 'removed-os'));
check('未知 KEY 常量', hasCode('local k = KEY.NOPE\n', 'unknown-key'));
check('未知 SC 成员', hasCode('SetKeyUp(1, SC.FOO)\n', 'unknown-sc'));
check('未知 GK 成员', hasCode('GetKeyDown(1, false, GK.FOO)\n', 'unknown-gk'));
check('未知 minecraft 成员', hasCode('minecraft.chat.Foo()\n', 'unknown-member'));
check('BeginChild 不成对', hasCode('BeginChild("a", 0, 0)\n', 'unbalanced-pair'));

console.log('\n[5] 误报控制');
check('注释内调用不应报错', codes('-- FooBar()\n').length === 0);
check('字符串内调用不应报错', !hasCode('local s = "FooBar()"\n', 'undefined-function'));
check('小写未定义函数不报错（视为用户函数）', !hasCode('myFunc()\n', 'undefined-function'));
check('已定义函数不报错', codes('function FooBar()\nend\nFooBar()\n', { topLevelCode: 'off' }).length === 0);
check('赋值的变量调用不报错', !hasCode('Foo = 1\nFoo()\n', 'undefined-function'));
check('方法调用 obj:Method() 不报错', !hasCode('local obj = {}\nobj:Method(1)\n', 'undefined-function'));
check('方法调用而不定义不报未定义', !hasCode('self:Update()\n', 'undefined-function'));

console.log('\n[6] 真实示例脚本');
{
  const sample = fs.readFileSync(
    path.join(__dirname, '..', '..', 'x64', 'Release', 'GameClicker', 'Script', 'temp', 'Test', 'script.lua'),
    'utf8'
  );
  const diags = computeDiagnostics(sample, {});
  check('示例脚本无诊断', diags.length === 0, JSON.stringify(diags.map(d => d.message)));
}

console.log('\n[7] 数据完整性');
check('API 数量 >= 70', api.GC_API_NAMES.length >= 70, `实际 ${api.GC_API_NAMES.length}`);
check('KEY 常量包含 A/Esc/Space', 'A' in api.KEY_CONSTANTS && 'VK_ESCAPE' in api.KEY_CONSTANTS && 'VK_SPACE' in api.KEY_CONSTANTS);
check('SC 值为 0/1/2', api.SC_CONSTANTS.MOUSE_EVENT.value === 0 && api.SC_CONSTANTS.SEND_INPUT.value === 1 && api.SC_CONSTANTS.POST_MESSAGE.value === 2);
check('GK 值为 1/3', api.GK_CONSTANTS.HOOK.value === 1 && api.GK_CONSTANTS.BOTH.value === 3);
check('GK 不包含 API（未注册）', !('API' in api.GK_CONSTANTS));

console.log('\n[8] 智能补全上下文');
{
  const { itemsForContext } = require('../src/completionProvider');
  const labelOf = i => (typeof i.label === 'string' ? i.label : i.label.label);
  const keyItems = itemsForContext('local k = KEY.', false);
  check('KEY. 触发常量补全', keyItems.length === Object.keys(api.KEY_CONSTANTS).length);
  const scItems = itemsForContext('SetKeyUp(1, SC.', false);
  check('SC. 触发常量补全', scItems.length === 3);
  const chatItems = itemsForContext('minecraft.chat.', false);
  check('minecraft.chat. 触发成员补全', chatItems.length === 2);
  // 补全项的 insertText 必须与「只替换光标处标识符」的范围策略一致（不再重复/丢失前缀）
  const insertOf = i => (typeof i.insertText === 'string' ? i.insertText : (i.insertText && i.insertText.value) || '');
  check('KEY 补全只插入键名（不重复 KEY. 前缀）', keyItems.every(i => insertOf(i) === labelOf(i)));
  check('SC 补全只插入成员名（不重复 SC. 前缀）', scItems.every(i => insertOf(i) === labelOf(i)));
  check('minecraft.chat 补全只插入成员名', chatItems.every(i => {
    const t = insertOf(i);
    return t.startsWith(labelOf(i)) && !t.includes('minecraft');
  }));
  // 补全必须始终包含「全部」API（不因所在函数而隐藏），只对上下文不可用项做标注
  const descOf = i => (typeof i.label === 'string' ? '' : (i.label.description || ''));
  const allNames = api.GC_API_NAMES;
  const nonUiNames = api.GC_FUNCTIONS.filter(f => f.category !== 'gui').map(f => f.name);
  const uiNames = api.GC_FUNCTIONS.filter(f => f.category === 'gui').map(f => f.name);

  const outsideItems = itemsForContext('Print', false);
  check('非 render 上下文包含全部 API', allNames.every(n => outsideItems.some(i => labelOf(i) === n)));
  check('非 render 下 UI API 被标注「仅 render 可用」', uiNames.every(n => {
    const it = outsideItems.find(i => labelOf(i) === n);
    return !!it && descOf(it).includes('仅 render 可用');
  }));

  const renderItems = itemsForContext('Button', true);
  check('render 上下文包含全部 API', allNames.every(n => renderItems.some(i => labelOf(i) === n)));
  check('render 下非 UI API 被标注「render 内不可用」', nonUiNames.every(n => {
    const it = renderItems.find(i => labelOf(i) === n);
    return !!it && descOf(it).includes('render 内不可用');
  }));
}

console.log('\n[9] 参数提示上下文');
{
  const { findCallContext } = require('../src/signatureProvider');
  const t1 = 'SetKeyClick(KEY.SPACE, ';
  const c1 = findCallContext(t1, t1.length);
  check('识别调用名', c1 && c1.name === 'SetKeyClick', JSON.stringify(c1));
  check('当前参数索引为 1', c1 && c1.activeParameter === 1);

  const t2 = 'Text("a", ("b"), ';
  const c2 = findCallContext(t2, t2.length);
  check('嵌套括号内的参数索引正确（第 3 个参数）', c2 && c2.activeParameter === 2, JSON.stringify(c2));

  const t3 = 'SliderInt("x", 1, 0, 100, ';
  const c3 = findCallContext(t3, t3.length);
  check('第 5 个参数索引为 4', c3 && c3.activeParameter === 4);

  check('非调用处返回 null', findCallContext('local a = 1', 11) === null);
}

console.log('\n[10] 语法定义引用校验');
{
  const grammar = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'syntaxes', 'gclua.tmLanguage.json'), 'utf8'));
  const repoKeys = new Set(Object.keys(grammar.repository || {}));
  const missing = [];
  const walk = (node) => {
    if (Array.isArray(node)) { node.forEach(walk); return; }
    if (node && typeof node === 'object') {
      if (typeof node.include === 'string' && node.include.startsWith('#')) {
        const key = node.include.slice(1);
        if (!repoKeys.has(key)) missing.push(key);
      }
      Object.values(node).forEach(walk);
    }
  };
  walk(grammar.patterns);
  walk(grammar.repository);
  check('所有 include 引用均存在', missing.length === 0, missing.join(', '));
  check('存在 gc-api 规则', repoKeys.has('gc-api'));
  check('scopeName 为 source.gclua', grammar.scopeName === 'source.gclua');
}

console.log('\n[11] 上下文 API 限制（render / main 函数作用域）');
{
  check('render 内调用非 UI API 报警', hasCode('function render()\n  PrintLog("x", 1)\nend\n', 'non-ui-in-render'));
  check('render 内调用 UI API 不报警', !hasCode('function render()\n  Text("x")\nend\n', 'non-ui-in-render'));
  check('main 内调用 UI API 报警', hasCode('function main()\n  Text("x")\nend\n', 'ui-in-non-render'));
  check('main 内调用非 UI API 不报警', !hasCode('function main()\n  PrintLog("x", 1)\nend\n', 'ui-in-non-render'));
  check('全局作用域调用 UI API 报警', hasCode('Text("x")\n', 'ui-in-non-render'));
  check('initScript 内调用 UI API 报警', hasCode('function initScript()\n  Button("x", 1, 1)\nend\n', 'ui-in-non-render'));
  check('saveConfig 内调用 UI API 报警', hasCode('function saveConfig()\n  Checkbox("x", true)\nend\n', 'ui-in-non-render'));
  check('自定义函数内调用 UI API 报警', hasCode('function myFunc()\n  SliderInt("x", 1, 0, 10, "%d")\nend\n', 'ui-in-non-render'));
  check('关闭 contextScope 后不报警', !hasCode('function main()\n  Text("x")\nend\n', 'ui-in-non-render', { contextScope: 'off' }));
  check('render 内 system 类 API 报警', hasCode('function render()\n  MessageBeep(1)\nend\n', 'non-ui-in-render'));
  check('main 内 system 类 API 不报警', !hasCode('function main()\n  MessageBeep(1)\nend\n', 'ui-in-non-render'));
}

console.log('\n[12] 函数作用域识别（buildFunctionScopes）');
{
  const { buildFunctionScopes } = scanner;
  const src = 'function render()\n  Text("x")\nend\nfunction main()\n  PrintLog("x", 1)\nend\n';
  const { tokens } = scanner.tokenize(src);
  const scopes = buildFunctionScopes(tokens);
  check('识别出 2 个函数作用域', scopes.length === 2, JSON.stringify(scopes.map(s => s.name)));
  check('包含 render 作用域', scopes.some(s => s.name === 'render'));
  check('包含 main 作用域', scopes.some(s => s.name === 'main'));
  const renderScope = scopes.find(s => s.name === 'render');
  check('render 作用域范围正确', renderScope.start < renderScope.end);
}

console.log('\n[13] 语法高亮 API 覆盖完整性');
{
  const grammar = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'syntaxes', 'gclua.tmLanguage.json'), 'utf8'));
  const apiPattern = (grammar.repository['gc-api'].patterns || []).map(p => p.match).join('|');
  const nsPattern = (grammar.repository['gc-namespace'].patterns || []).map(p => p.match).join('|');
  const deprecatedPattern = (grammar.repository['deprecated'].patterns || []).map(p => p.match).join('|');
  const unavailable = new Set(Object.keys(api.GC_DEPRECATED));

  const flatApis = api.GC_FUNCTIONS
    .map(f => f.name)
    .filter(n => !n.startsWith('minecraft.'))
    .filter(n => !unavailable.has(n));
  const missingHighlight = flatApis.filter(n => !apiPattern.includes(n));
  check('全部平铺 API 均在语法高亮 gc-api 规则中', missingHighlight.length === 0, missingHighlight.join(', '));

  const nsApis = api.GC_FUNCTIONS.map(f => f.name).filter(n => n.startsWith('minecraft.')).map(n => n.split('.')[2]);
  const missingNs = nsApis.filter(n => !apiPattern.includes(n) && !nsPattern.includes(n));
  check('minecraft.chat 成员均在语法高亮中', missingNs.length === 0, missingNs.join(', '));

  check('IsCloseUI 在弃用高亮规则中', deprecatedPattern.includes('IsCloseUI'));
  check('load 在弃用高亮规则中', deprecatedPattern.includes('load'));
  check('IsCloseUI 不在 gc-api 高亮规则中', !apiPattern.includes('IsCloseUI'));
}

console.log('\n[14] API 签名与运行时（ScriptAPI.h）一致性');
{
  const byName = n => api.GC_FUNCTIONS.find(f => f.name === n);
  check('DrawSpiltLine 支持可选长度参数', byName('DrawSpiltLine').params.length === 1 && byName('DrawSpiltLine').params[0].optional === true);
  check('DrawCircleFilled 需要 4 个参数', byName('DrawCircleFilled').params.length === 4);
  check('IsCloseUI 标记为禁用', !!api.GC_DEPRECATED.IsCloseUI);
  check('load 标记为已移除', !!api.GC_DEPRECATED.load);
  check('DrawSpiltLine(300) 不报参数错误', !hasCode('DrawSpiltLine(300)\n', 'argument-count'));
  check('DrawSpiltLine() 不报参数错误', !hasCode('DrawSpiltLine()\n', 'argument-count'));
  check('DrawCircleFilled 缺参数报错', hasCode('DrawCircleFilled(1, 2, 3)\n', 'argument-count'));
  check('DrawCircleFilled(1,2,3,4) 不报错', !hasCode('DrawCircleFilled(1, 2, 3, 4)\n', 'argument-count'));
  check('IsCloseUI 调用触发弃用警告', hasCode('IsCloseUI()\n', 'deprecated-api'));
  check('load 调用触发已移除错误', hasCode('load("x")\n', 'removed-function'));
}

console.log('\n[15] VS2026 配色与主题贡献');
{
  const pkg = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'package.json'), 'utf8'));
  check('贡献了颜色主题', Array.isArray(pkg.contributes.themes) && pkg.contributes.themes.length >= 1);
  const themeEntry = (pkg.contributes.themes || [])[0] || {};
  check('主题指向 vs2026-dark.json', themeEntry.path === './themes/vs2026-dark.json');
  const theme = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'themes', 'vs2026-dark.json'), 'utf8'));
  check('主题名为 GC Lua — VS2026 C++', theme.name === 'GC Lua — VS2026 C++');
  const ruleFor = sc => (theme.tokenColors || []).find(r => [].concat(r.scope).includes(sc));
  const fg = sc => { const r = ruleFor(sc); return r ? String(r.settings.foreground).toLowerCase() : null; };
  check('关键字为 VS C++ 蓝 #569cd6', fg('keyword') === '#569cd6');
  check('字符串为 VS C++ 暗橙 #d69d85', fg('string') === '#d69d85');
  check('注释为 VS C++ 绿 #57a64a', fg('comment') === '#57a64a');
  check('类型为 VS C++ 青 #4ec9b0', fg('entity.name.type') === '#4ec9b0');
  check('函数为 VS C++ 黄 #dcdcaa', fg('entity.name.function') === '#dcdcaa');
  check('数字为 VS C++ 浅绿 #b5cea8', fg('constant.numeric') === '#b5cea8');

  const cds = pkg.contributes.configurationDefaults || {};
  const tcc = cds['editor.tokenColorCustomizations'];
  check('提供了 tokenColorCustomizations 默认值', !!(tcc && Array.isArray(tcc.textMateRules)));
  const scopes = (tcc.textMateRules || []).flatMap(r => [].concat(r.scope));
  check('GC API 作用域已配色', scopes.includes('support.function.gc.gclua'));
  check('KEY/SC/GK 常量作用域已配色', scopes.includes('constant.other.key.gclua') && scopes.includes('constant.other.clickmode.gclua'));
  check('弃用作用域已配色', scopes.includes('invalid.deprecated.gclua'));
  check('字符串作用域已配色为 VS C++ 暗橙 #d69d85', tcc.textMateRules.some(r => [].concat(r.scope).includes('string.quoted.double.gclua') && String(r.settings.foreground).toLowerCase() === '#d69d85'));
  check('标点/访问符作用域已配色为关键字蓝 #569cd6', tcc.textMateRules.some(r => [].concat(r.scope).includes('punctuation.section.gclua') && String(r.settings.foreground).toLowerCase() === '#569cd6'));
  const themeForOps = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'themes', 'vs2026-dark.json'), 'utf8'));
  check('主题中运算符为关键字蓝 #569cd6', themeForOps.tokenColors.some(r => [].concat(r.scope).includes('keyword.operator') && String(r.settings.foreground).toLowerCase() === '#569cd6'));
  check('主题中不为裸 punctuation 统一着色', !themeForOps.tokenColors.some(r => [].concat(r.scope).includes('punctuation')));

  const cmds = (pkg.contributes.commands || []).map(c => c.command);
  check('提供「设为 GC Lua 语言」命令', cmds.includes('gclua.setLanguage'));
  check('GK 高亮不含未注册的 API', !JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'syntaxes', 'gclua.tmLanguage.json'), 'utf8')).repository['gc-constant'].patterns.some(p => String(p.match).includes('GK\\.(HOOK|API')));
}

console.log('\n[16] 语法正则的 Oniguruma 兼容性');
{
  const grammar = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'syntaxes', 'gclua.tmLanguage.json'), 'utf8'));

  // Oniguruma（VSCode 的 TextMate 引擎）不允许把量词作用在断言组上，如 (?=\s*\()?，否则整份语法编译失败、全部无高亮
  const quantAfterAssert = /(\(\?[=!](?:\\.|[^()\\])*\))[?*+]/;
  const offenders = [];
  const walk = (node, p) => {
    if (Array.isArray(node)) { node.forEach((n, i) => walk(n, p + '[' + i + ']')); return; }
    if (node && typeof node === 'object') {
      for (const k of ['match', 'begin', 'end', 'while']) {
        if (typeof node[k] === 'string' && quantAfterAssert.test(node[k])) offenders.push(p + '.' + k + ' => ' + node[k]);
      }
      for (const k of Object.keys(node)) walk(node[k], p + '.' + k);
    }
  };
  walk(grammar, 'grammar');
  check('不存在「对断言组施加量词」的非法正则', offenders.length === 0, offenders.join(' | '));

  // 弃用规则必须排在函数调用规则之前，否则 dofile/print 等会被当作普通函数名而丢失失效标记
  const inc = grammar.patterns.map(p => p.include);
  check('deprecated 规则排在 functions 规则之前',
    inc.indexOf('#deprecated') >= 0 && inc.indexOf('#deprecated') < inc.indexOf('#functions'), inc.join(','));

  // 语法整体结构自检：patterns 中的每条 include 都能在 repository 中找到
  const repo = new Set(Object.keys(grammar.repository || {}));
  check('顶层 patterns 的 include 均存在', inc.every(i => repo.has(String(i).replace('#', ''))));
}

console.log('\n[17] 关键字补全与 preselect');
{
  const { itemsForContext } = require('../src/completionProvider');
  const labelOf = i => (typeof i.label === 'string' ? i.label : i.label.label);
  const descOf = i => (typeof i.label === 'string' ? '' : (i.label.description || ''));

  // 关键字补全：end / do / local / nil / true / false / and / or / not 等均已提供
  const defaultItems = itemsForContext('', false);
  const kwLabels = defaultItems
    .filter(i => i.kind === 14 /* Keyword */)
    .map(labelOf);
  check('关键字补全包含 end', kwLabels.includes('end'));
  check('关键字补全包含 do', kwLabels.includes('do'));
  check('关键字补全包含 local', kwLabels.includes('local'));
  check('关键字补全包含 function', kwLabels.includes('function'));
  check('关键字补全包含 if', kwLabels.includes('if'));
  check('关键字补全包含 for', kwLabels.includes('for'));
  check('关键字补全包含 while', kwLabels.includes('while'));
  check('关键字补全包含 return', kwLabels.includes('return'));
  check('关键字补全包含 break', kwLabels.includes('break'));
  check('关键字补全包含 nil/true/false', kwLabels.includes('nil') && kwLabels.includes('true') && kwLabels.includes('false'));

  // 块关键字 snippet 都以 end 结尾（自动配 end）
  const insertOf = i => (typeof i.insertText === 'string' ? i.insertText : (i.insertText && i.insertText.value) || '');
  const functionSnippet = defaultItems.find(i => labelOf(i) === 'function' && i.kind === 14);
  check('function snippet 以 end 结尾', insertOf(functionSnippet).trim().endsWith('end'));
  const ifSnippet = defaultItems.find(i => labelOf(i) === 'if' && i.kind === 14);
  check('if snippet 以 end 结尾', insertOf(ifSnippet).trim().endsWith('end'));
  const forSnippet = defaultItems.find(i => labelOf(i) === 'for' && i.kind === 14);
  check('for snippet 以 end 结尾', insertOf(forSnippet).trim().endsWith('end'));
  const doSnippet = defaultItems.find(i => labelOf(i) === 'do' && i.kind === 14);
  check('do snippet 以 end 结尾', insertOf(doSnippet).trim().endsWith('end'));

  // 分类标签：每个 API 补全项都有类别标签（功能 / UI / 系统 / Lua / 命名空间）
  const apiItems = defaultItems.filter(i => i.kind === 2 /* Function */);
  check('API 补全项都有分类标签', apiItems.every(i => {
    const d = descOf(i);
    return d.includes('功能') || d.includes('UI') || d.includes('系统') || d.includes('命名空间') || d.includes('Lua');
  }));

  // 排序分组：可用 API 排在不可用 API 之前
  const printItems = itemsForContext('Print', false);
  const availableSort = printItems.find(i => labelOf(i) === 'PrintLog');
  const uiItems = printItems.filter(i => {
    const d = descOf(i);
    return d.includes('仅 render 可用');
  });
  if (availableSort && uiItems.length > 0) {
    check('可用 API 排在不可用 API 之前', availableSort.sortText < uiItems[0].sortText,
      `${availableSort.sortText} vs ${uiItems[0].sortText}`);
  } else {
    check('可用 API 排在不可用 API 之前', false, '找不到测试对象');
  }

  // preselect：模拟 provideCompletionItems 的 preselect 逻辑
  // 输入 Print 时应 preselect PrintLog（前缀匹配，且不是不可用项）
  function simulatePreselect(linePrefix, inRender) {
    const items = itemsForContext(linePrefix, inRender);
    const prefixMatch = /([A-Za-z_][A-Za-z0-9_]*)$/.exec(linePrefix);
    const prefix = prefixMatch ? prefixMatch[1].toLowerCase() : '';
    if (!prefix) return null;
    let preselected = false;
    for (const item of items) {
      if (preselected) break;
      const label = typeof item.label === 'string' ? item.label : item.label.label;
      const desc = typeof item.label === 'string' ? '' : (item.label.description || '');
      const unavailable = desc.includes('不可用') || desc.includes('仅 render');
      if (!unavailable && label.toLowerCase() === prefix) { item.preselect = true; preselected = true; }
    }
    if (!preselected) {
      for (const item of items) {
        if (preselected) break;
        const label = typeof item.label === 'string' ? item.label : item.label.label;
        const desc = typeof item.label === 'string' ? '' : (item.label.description || '');
        const unavailable = desc.includes('不可用') || desc.includes('仅 render');
        if (!unavailable && label.toLowerCase().startsWith(prefix)) { item.preselect = true; preselected = true; }
      }
    }
    return items.find(i => i.preselect === true) || null;
  }

  const ps1 = simulatePreselect('Print', false);
  check('输入 Print 时 preselect PrintLog', ps1 && labelOf(ps1) === 'PrintLog', ps1 ? labelOf(ps1) : 'null');

  const ps2 = simulatePreselect('Button', true);
  check('render 内输入 Button 时 preselect Button', ps2 && labelOf(ps2) === 'Button', ps2 ? labelOf(ps2) : 'null');

  const ps3 = simulatePreselect('Text', false);
  check('非 render 内输入 Text 时不 preselect 不可用项', !ps3 || !descOf(ps3).includes('仅 render'));
}

console.log('\n[18] GK 常量校验消息不含已移除的 API');
{
  const diags = computeDiagnostics('GetKeyDown(1, false, GK.FOO)\n', {});
  const gkDiag = diags.find(d => d.code === 'unknown-gk');
  check('GK 未知成员报错', !!gkDiag);
  check('GK 消息不含 API', gkDiag ? !gkDiag.message.includes('`API`') : false, gkDiag ? gkDiag.message : '');
  check('GK 消息含 HOOK 和 BOTH', gkDiag ? (gkDiag.message.includes('HOOK') && gkDiag.message.includes('BOTH')) : false);
}

console.log('\n[19] 未使用的变量 / 函数');
{
  check('未使用的局部变量报警', hasCode('local unusedVar = 1\n', 'unused-local'));
  check('被使用的局部变量不报警', !hasCode('local x = 1\nPrintLog(tostring(x), 1)\n', 'unused-local'));
  check('下划线开头的局部变量被忽略', !hasCode('local _tmp = 1\n', 'unused-local'));
  check('未调用的函数报警', hasCode('local function helper()\nend\n', 'unused-function'));
  check('被调用的函数不报警', !hasCode('function foo()\nend\nfoo()\n', 'unused-function'));
  check('生命周期函数被忽略', !hasCode('function main()\nend\nfunction render()\nend\n', 'unused-function'));
  check('关闭 unused 后不报警', !hasCode('local unusedVar = 1\n', 'unused-local', { unused: 'off' }));
  // local function 只报一次（不重复报 unused-local）
  const dup = computeDiagnostics('local function helper()\nend\n', {}).filter(d => d.code === 'unused-local' || d.code === 'unused-function');
  check('local function 只报一次未被调用', dup.length === 1 && dup[0].code === 'unused-function', JSON.stringify(dup.map(d => d.code)));
}

console.log('\n[20] 可疑逻辑检查');
{
  check('条件中误用 = 报警', hasCode('if x = 1 then\nend\n', 'assignment-in-condition'));
  check('正常比较 == 不报警', !hasCode('if x == 1 then\nPrintLog("a", 1)\nend\n', 'assignment-in-condition'));
  check('while true 无 break 报警', hasCode('while true do\nSleep(1)\nend\n', 'infinite-loop'));
  check('while true 有 break 不报警', !hasCode('while true do\nbreak\nend\n', 'infinite-loop'));
  check('for 步长为 0 报警', hasCode('for i = 1, 10, 0 do\nend\n', 'zero-step-loop'));
  check('for 步长正常不报警', !hasCode('for i = 1, 10, 2 do\nPrintLog(tostring(i), 1)\nend\n', 'zero-step-loop'));
  check('除零报警', hasCode('local b = a / 0\n', 'divide-by-zero'));
  check('取模零报警', hasCode('local b = a % 0\n', 'divide-by-zero'));
  check('正常除法不报警', !hasCode('local b = a / 2\n', 'divide-by-zero'));
  check('小数除法不报警', !hasCode('local b = a / 0.5\n', 'divide-by-zero'));
  check('自我赋值报警', hasCode('local x = x\n', 'self-assignment'));
  check('自增式赋值不报警', !hasCode('x = x + 1\n', 'self-assignment'));
  check('恒真条件报警', hasCode('if true then\nPrintLog("a", 1)\nend\n', 'redundant-condition'));
  check('空代码块报警', hasCode('if x then\nend\n', 'empty-block'));
  check('重复定义函数报警', hasCode('function foo()\nend\nfunction foo()\nend\n', 'duplicate-function'));
  check('关闭 suspicious 后不报警', !hasCode('if x = 1 then\nend\n', 'assignment-in-condition', { suspicious: 'off' }));
}

console.log('\n[21] API 实参类型校验');
{
  check('字符串传给整数参数报警', hasCode('Sleep("abc")\n', 'argument-type'));
  check('正确整数参数不报警', !hasCode('Sleep(100)\n', 'argument-type'));
  check('字符串传给按键参数报警', hasCode('SetKeyDown("A")\n', 'argument-type'));
  check('常量表达式不误报', !hasCode('SetKeyDown(KEY.A)\n', 'argument-type'));
  check('正确字符串参数不报警', !hasCode('Text("hello")\n', 'argument-type'));
  check('数字传给字符串参数报警', hasCode('PrintLog(123, 1)\n', 'argument-type'));
  check('变量实参不误报', !hasCode('local s = "x"\nSleep(s)\n', 'argument-type'));
  check('关闭 typeCheck 后不报警', !hasCode('Sleep("abc")\n', 'argument-type', { typeCheck: 'off' }));
}

console.log('\n[22] 语言注册（lua + gclua 双支持）');
{
  const pkg = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'package.json'), 'utf8'));
  const events = pkg.activationEvents || [];
  check('激活事件包含 onLanguage:gclua', events.includes('onLanguage:gclua'));
  check('激活事件包含 onLanguage:lua', events.includes('onLanguage:lua'));
  const grammars = pkg.contributes.grammars || [];
  check('语法绑定包含 lua 语言', grammars.some(g => g.language === 'lua'));
  check('语法绑定包含 gclua 语言', grammars.some(g => g.language === 'gclua'));
  const ext = fs.readFileSync(path.join(__dirname, '..', 'src', 'extension.js'), 'utf8');
  check('extension.js 同时注册 lua 与 gclua', /LANGUAGE_IDS\s*=\s*\[[^\]]*'gclua'[^\]]*'lua'[^\]]*\]/.test(ext));
  const diagCfg = (pkg.contributes.configuration.properties || {});
  check('提供 unused 诊断配置', !!diagCfg['gclua.diagnostics.unused']);
  check('提供 suspicious 诊断配置', !!diagCfg['gclua.diagnostics.suspicious']);
  check('提供 typeCheck 诊断配置', !!diagCfg['gclua.diagnostics.typeCheck']);
}

console.log('\n[23] 语法增强（not / 入口函数 / .. / # 标签）');
{
  const grammar = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'syntaxes', 'gclua.tmLanguage.json'), 'utf8'));
  const kwMatches = (grammar.repository['keywords'].patterns || []).map(p => p.match);
  check('not 已注册高亮', kwMatches.some(m => /\bnot\b/.test(m)));
  check('and / or 已注册高亮', kwMatches.some(m => /\band\b/.test(m) && /\bor\b/.test(m)));
  const fnPatterns = grammar.repository['functions'].patterns || [];
  check('入口函数有专门 scope', fnPatterns.some(p => p.name === 'entity.name.function.entry.gclua'));
  const opNames = (grammar.repository['operators'].patterns || []).map(p => p.name);
  check('.. 连接符有专门 scope', opNames.includes('keyword.operator.concat.gclua'));
  check('字符串不再对 # 特殊着色（无 # 标签 include）', !JSON.stringify(grammar.repository['strings']).includes('#string-tag'));
  check('字符串外 # 仍作为长度运算符着色', (grammar.repository['operators'].patterns || []).some(p => /#/.test(p.match || '')));
  check('不存在遗留的 string-tag 规则', !grammar.repository['string-tag']);
  check('字符串引号有专门的定义 scope', JSON.stringify(grammar.repository['strings']).includes('punctuation.definition.string.begin.gclua'));
  const opPatterns = grammar.repository['operators'].patterns || [];
  check('vararg ... 有专门 scope', opPatterns.some(p => p.name === 'keyword.operator.vararg.gclua'));
  const punctNames = (grammar.repository['punctuation'].patterns || []).map(p => p.name);
  check('访问符 . 有专门 scope', punctNames.includes('punctuation.accessor.gclua'));
  check('逗号/分号与括号有 scope', punctNames.includes('punctuation.separator.gclua') && punctNames.includes('punctuation.section.gclua'));
}

console.log('\n[24] 扩展诊断（入口函数 / 顶层执行 / 未定义变量 / 缺少 local / 冗余 / 不可达）');
{
  check('未定义变量报提示', hasCode('local t = tostring(undefVar)\n', 'undefined-variable'));
  check('已声明变量不报', !hasCode('local x = 1\nlocal t = tostring(x)\n', 'undefined-variable'));
  check('函数内未用 local 赋值报警告', hasCode('function main()\n  counter = 1\nend\n', 'missing-local'));
  check('表构造字段不报缺少local', !hasCode('function main()\n  local t = { a = 1, b = 2, c = 3 }\n  PrintLog(tostring(t), 1)\nend\n', 'missing-local'));
  check('嵌套表构造字段不报缺少local', !hasCode('function main()\n  local t = { outer = { x = 1, y = 2 }, z = 3 }\n  PrintLog(tostring(t), 1)\nend\n', 'missing-local'));
  check('表构造外的真实漏写仍报', hasCode('function main()\n  local t = { a = 1 }\n  realGlobal = 2\nend\n', 'missing-local'));
  check('使用 local 不报警', !hasCode('function main()\n  local counter = 1\n  PrintLog(tostring(counter), 1)\nend\n', 'missing-local'));
  check('顶层函数调用报错', hasCode('PrintLog("x", 1)\n', 'top-level-execution'));
  check('顶层 if 报错', hasCode('if true then\nend\n', 'top-level-execution'));
  check('顶层 local 声明不报', !hasCode('local a = 1\nfunction main()\n  PrintLog(tostring(a), 1)\nend\n', 'top-level-execution'));
  check('函数内调用不报顶层执行', !hasCode('function main()\n  PrintLog("x", 1)\nend\n', 'top-level-execution'));
  check('缺少入口函数报警', hasCode('function main()\nend\nfunction render()\nend\n', 'missing-entry'));
  check('四个入口齐全不报', !hasCode('function initScript()\nend\nfunction main()\nend\nfunction render()\nend\nfunction saveConfig()\nend\n', 'missing-entry'));
  check('无入口函数不报（普通 Lua 文件）', !hasCode('local function helper()\nend\nlocal r = helper()\n', 'missing-entry'));
  check('重复定义入口函数报错', hasCode('function main()\nend\nfunction main()\nend\n', 'duplicate-entry'));
  check('恒真条件报冗余', hasCode('function main()\n  if true then\n    PrintLog("a", 1)\n  end\nend\n', 'redundant-condition'));
  check('恒真条件的 else 报冗余分支', hasCode('function main()\n  if true then\n    PrintLog("a", 1)\n  else\n    PrintLog("b", 1)\n  end\nend\n', 'redundant-branch'));
  check('空代码块报冗余', hasCode('function main()\n  if IsOpenGui() then\n  end\nend\n', 'empty-block'));
  check('return 之后代码不可达', hasCode('function main()\n  return\n  PrintLog("x", 1)\nend\n', 'unreachable-code'));
  check('return 是最后语句不报', !hasCode('function main()\n  PrintLog("x", 1)\n  return\nend\n', 'unreachable-code'));
  check('关闭 redundant 后不报', !hasCode('function main()\n  if true then\n    PrintLog("a", 1)\n  end\nend\n', 'redundant-condition', { redundant: 'off' }));
  check('关闭 topLevelCode 后不报', !hasCode('PrintLog("x", 1)\n', 'top-level-execution', { topLevelCode: 'off' }));
}

console.log('\n[25] 事件函数 CodeLens');
{
  const { GCLuaCodeLensProvider } = require('../src/codeLensProvider');
  const provider = new GCLuaCodeLensProvider();
  const src = 'function initScript()\nend\nfunction main()\nend\nfunction render()\nend\nfunction saveConfig()\nend\nfunction helper()\nend\n';
  const doc = { getText: () => src };
  const lenses = provider.provideCodeLenses(doc);
  check('为 4 个入口函数生成 CodeLens', lenses.length === 4, `实际 ${lenses.length}`);
  check('CodeLens 标题包含“事件函数”', lenses.every(l => l.command && String(l.command.title).includes('事件函数')));
  check('CodeLens 命令为 gclua.showEntryInfo', lenses.every(l => l.command && l.command.command === 'gclua.showEntryInfo'));
  check('非入口函数不生成 CodeLens', !lenses.some(l => l.command && String(l.command.arguments || []).includes('helper')));
}

console.log('\n================ 测试结果 ================');
console.log(`通过：${passed}    失败：${failed}`);
if (failed > 0) {
  console.log('\n失败项：');
  failures.forEach(f => console.log('  - ' + f));
  process.exit(1);
}
process.exit(0);