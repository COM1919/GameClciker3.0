'use strict';
/**
 * GC Lua 智能补全提供者
 * 覆盖：GC API 函数、KEY/SC/GK 常量、minecraft 命名空间、Lua 标准库、生命周期函数、关键字与代码片段。
 */

const vscode = require('vscode');
const api = require('../data/api');
const fmt = require('./docFormatter');
const scanner = require('./luaScanner');

/** 关键字补全（含块结构自动配 end） */
const KEYWORD_ITEMS = [
  ['if', 'if ${1:condition} then\n\t${2}\nend', 'if 语句'],
  ['elseif', 'elseif ${1:condition} then\n\t${2}', 'elseif 分支'],
  ['else', 'else\n\t${1}', 'else 分支'],
  ['end', 'end', '结束块'],
  ['for', 'for ${1:i} = ${2:1}, ${3:10} do\n\t${4}\nend', '数值 for 循环'],
  ['forin', 'for ${1:k}, ${2:v} in pairs(${3:table}) do\n\t${4}\nend', '遍历 for 循环'],
  ['while', 'while ${1:condition} do\n\t${2}\nend', 'while 循环'],
  ['repeat', 'repeat\n\t${1}\nuntil ${2:condition}', 'repeat 循环'],
  ['do', 'do\n\t${1}\nend', 'do 块'],
  ['function', 'function ${1:name}(${2})\n\t${3}\nend', '函数定义'],
  ['localfunction', 'local function ${1:name}(${2})\n\t${3}\nend', '局部函数定义'],
  ['local', 'local ${1:name} = ${2:value}', '局部变量声明'],
  ['return', 'return ${1}', '返回'],
  ['break', 'break', '跳出循环'],
  ['goto', 'goto ${1:label}', '跳转'],
  ['nil', 'nil', '空值'],
  ['true', 'true', '真'],
  ['false', 'false', '假'],
  ['and', 'and', '逻辑与'],
  ['or', 'or', '逻辑或'],
  ['not', 'not', '逻辑非'],
  ['in', 'in', 'in 关键字']
];

/** 生命周期函数补全 */
function lifecycleItems() {
  return Object.keys(api.LIFECYCLE).map(name => {
    const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Method);
    item.detail = '生命周期函数';
    item.documentation = new vscode.MarkdownString(api.LIFECYCLE[name]);
    item.insertText = new vscode.SnippetString(`function ${name}()\n\t\${1}\nend`);
    item.sortText = '0a' + name;
    item.label = { label: name, description: '生命周期' };
    return item;
  });
}

/** API 分类中文标签 */
function categoryLabel(def) {
  switch (def.category) {
    case 'gui': return 'UI';
    case 'system': return '系统';
    case 'functional': return '功能';
    default: return '';
  }
}

/** 由 API 定义构造补全项（contextTag 用于标注该 API 在当前函数上下文中是否可用） */
function apiCompletion(def, contextTag, sortPrefix) {
  const item = new vscode.CompletionItem(def.name, vscode.CompletionItemKind.Function);
  item.detail = def.signature;
  item.documentation = new vscode.MarkdownString(fmt.formatDoc(def));
  item.insertText = buildSnippet(def);
  item.sortText = (sortPrefix || '1a') + def.name;
  const tags = [categoryLabel(def)];
  if (def.restricted) tags.push('受限');
  if (contextTag) tags.push(contextTag);
  item.label = { label: def.name, description: tags.join(' · ') };
  return item;
}

/** 构造函数调用的 snippet（name 可指定实际插入名，默认用 API 全名） */
function buildSnippet(def, name) {
  const fn = name || def.name;
  if (!def.params || def.params.length === 0) {
    return new vscode.SnippetString(`${fn}()`);
  }
  const args = def.params.map((p, i) => `\${${i + 1}:${p.name}}`).join(', ');
  return new vscode.SnippetString(`${fn}(${args})`);
}

/**
 * 由命名空间成员的 API 构造补全项。
 * 只插入成员名（如 `OpenReader`），保留文档中已输入的命名空间前缀（如 `minecraft.chat.`），
 * 从而避免把前缀一起替换导致的重复/丢失。
 */
function memberCompletion(def) {
  const member = def.name.split('.').pop();
  const item = new vscode.CompletionItem(member, vscode.CompletionItemKind.Function);
  item.detail = def.signature;
  item.documentation = new vscode.MarkdownString(fmt.formatDoc(def));
  item.insertText = buildSnippet(def, member);
  item.sortText = '1b' + member;
  item.label = { label: member, description: categoryLabel(def) };
  return item;
}

/** KEY 常量补全（只插入键名，`KEY.` 前缀已在文档中） */
function keyItems() {
  return Object.keys(api.KEY_CONSTANTS).map(k => {
    const item = new vscode.CompletionItem(k, vscode.CompletionItemKind.Constant);
    item.detail = `KEY.${k} = ${api.KEY_CONSTANTS[k]}`;
    item.insertText = k;
    item.sortText = '3a' + k;
    item.label = { label: k, description: 'KEY' };
    return item;
  });
}

/** SC / GK 常量补全（只插入成员名，表名前缀已在文档中） */
function tableConstantItems(tableName, table) {
  return Object.keys(table).map(k => {
    const entry = table[k];
    const item = new vscode.CompletionItem(k, vscode.CompletionItemKind.EnumMember);
    item.detail = `${tableName}.${k} = ${entry.value}`;
    item.documentation = new vscode.MarkdownString(entry.desc);
    item.insertText = k;
    item.sortText = '3a' + k;
    item.label = { label: k, description: tableName };
    return item;
  });
}

/** Lua 标准库补全 */
function stdlibItems() {
  const items = [];
  for (const g of api.LUA_STDLIB.globals) {
    const item = new vscode.CompletionItem(g, vscode.CompletionItemKind.Function);
    item.detail = 'Lua 标准库';
    item.insertText = new vscode.SnippetString(`${g}(\${1})`);
    item.sortText = '4a' + g;
    item.label = { label: g, description: 'Lua' };
    items.push(item);
  }
  for (const [lib, members] of Object.entries(api.LUA_STDLIB.libraries)) {
    const mod = new vscode.CompletionItem(lib, vscode.CompletionItemKind.Module);
    mod.detail = 'Lua 标准库';
    mod.sortText = '4b' + lib;
    mod.label = { label: lib, description: 'Lua' };
    items.push(mod);
    for (const mem of members) {
      const item = new vscode.CompletionItem(mem, vscode.CompletionItemKind.Method);
      item.detail = `${lib}.${mem}`;
      item.insertText = `${mem}`;
      item.sortText = '4b' + lib + mem;
      item.label = { label: mem, description: `${lib}` };
      items.push(item);
    }
  }
  return items;
}

/** 关键字与代码片段补全 */
function keywordItems() {
  return KEYWORD_ITEMS.map(([label, snippet, desc]) => {
    const item = new vscode.CompletionItem(label, vscode.CompletionItemKind.Keyword);
    item.insertText = new vscode.SnippetString(snippet);
    item.sortText = '5a' + label;
    item.label = { label: label, description: desc };
    return item;
  });
}

/**
 * 根据已输入的前缀判断补全上下文
 * @param {string} linePrefix 当前行光标前的文本
 * @param {boolean} inRender 是否在 render 函数内（true 时只补全 UI 系列 API）
 * @returns {vscode.CompletionItem[]}
 */
function itemsForContext(linePrefix, inRender) {
  // KEY. / SC. / GK. （常量在任何上下文中都可用）
  let m;
  if ((m = /(?:^|[^\w.])KEY\.(\w*)$/.exec(linePrefix))) {
    return keyItems();
  }
  if ((m = /(?:^|[^\w.])SC\.(\w*)$/.exec(linePrefix))) {
    return tableConstantItems('SC', api.SC_CONSTANTS);
  }
  if ((m = /(?:^|[^\w.])GK\.(\w*)$/.exec(linePrefix))) {
    return tableConstantItems('GK', api.GK_CONSTANTS);
  }
  // minecraft.chat.（始终给出全部成员，保证补全完整）
  if (/(?:^|[^\w.])minecraft\.chat\.(\w*)$/.test(linePrefix)) {
    return api.GC_FUNCTIONS
      .filter(f => f.name.startsWith('minecraft.chat.'))
      .map(f => memberCompletion(f));
  }
  // minecraft.
  if (/(?:^|[^\w.])minecraft\.(\w*)$/.test(linePrefix)) {
    const item = new vscode.CompletionItem('chat', vscode.CompletionItemKind.Module);
    item.detail = 'minecraft.chat 命名空间';
    item.insertText = 'chat';
    item.sortText = '1bchat';
    item.label = { label: 'chat', description: '命名空间' };
    return [item];
  }
  // 标准库成员：string. / table. / math. / os. / coroutine.
  if ((m = /(?:^|[^\w.])(string|table|math|os|coroutine)\.(\w*)$/.exec(linePrefix))) {
    const lib = m[1];
    const members = api.LUA_STDLIB.libraries[lib] || [];
    return members.map(mem => {
      const item = new vscode.CompletionItem(mem, vscode.CompletionItemKind.Method);
      item.detail = `${lib}.${mem}`;
      item.insertText = `${mem}`;
      item.sortText = '1b' + mem;
      item.label = { label: mem, description: lib };
      return item;
    });
  }

  // 默认：始终给出全部 GC API（补全保持完整）
  // 按上下文可用性分组：可用项在前（sortText 1a），不可用项在后（sortText 2a）并加标注
  // 限制本身由诊断负责
  const items = [];
  const availableApis = [];
  const unavailableApis = [];
  for (const def of fmt.dedupe(api.GC_FUNCTIONS)) {
    const unavailable = inRender ? def.category !== 'gui' : def.category === 'gui';
    const tag = unavailable ? (inRender ? 'render 内不可用' : '仅 render 可用') : null;
    const item = apiCompletion(def, tag, unavailable ? '2a' : '1a');
    if (unavailable) unavailableApis.push(item);
    else availableApis.push(item);
  }
  items.push(...availableApis);
  items.push(...unavailableApis);

  items.push(...lifecycleItems());

  const minecraft = new vscode.CompletionItem('minecraft', vscode.CompletionItemKind.Module);
  minecraft.detail = 'minecraft.chat 命名空间';
  minecraft.sortText = '2bminecraft';
  minecraft.label = { label: 'minecraft', description: '命名空间' };
  items.push(minecraft);
  for (const t of ['KEY', 'SC', 'GK']) {
    const mod = new vscode.CompletionItem(t, vscode.CompletionItemKind.Enum);
    mod.detail = `${t} 常量表`;
    mod.sortText = '3a' + t;
    mod.label = { label: t, description: '常量表' };
    items.push(mod);
  }
  items.push(...stdlibItems());
  items.push(...keywordItems());
  return items;
}

class GCLuaCompletionProvider {
  provideCompletionItems(document, position) {
    const linePrefix = document.lineAt(position.line).text.slice(0, position.character);

    // 判断当前光标是否在 render 函数内，用于上下文敏感的补全过滤
    let inRender = false;
    try {
      const text = document.getText();
      const offset = document.offsetAt(position);
      const { tokens } = scanner.tokenize(text);
      const scopes = scanner.buildFunctionScopes(tokens);
      inRender = scopes.some(s => s.name === 'render' && offset >= s.start && offset <= s.end);
    } catch (e) {
      // 扫描失败时不过滤，返回全量补全
    }

    const items = itemsForContext(linePrefix, inRender);
    // 仅替换光标处的标识符本身；`KEY.` / `minecraft.chat.` 等前缀保留在文档中
    const wordRange = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);

    // 智能 preselect：根据已输入前缀选中最佳项
    // 1. 优先精确匹配（不区分大小写）
    // 2. 其次前缀匹配
    // 3. 跳过上下文不可用的项（带「不可用」/「仅 render」标注）
    const prefixMatch = /([A-Za-z_][A-Za-z0-9_]*)$/.exec(linePrefix);
    const prefix = prefixMatch ? prefixMatch[1].toLowerCase() : '';
    if (prefix) {
      const lower = prefix;
      let preselected = false;
      // 第一轮：精确匹配
      for (const item of items) {
        if (preselected) break;
        const label = typeof item.label === 'string' ? item.label : item.label.label;
        const desc = typeof item.label === 'string' ? '' : (item.label.description || '');
        const unavailable = desc.includes('不可用') || desc.includes('仅 render');
        if (!unavailable && label.toLowerCase() === lower) {
          item.preselect = true;
          preselected = true;
        }
      }
      // 第二轮：前缀匹配
      if (!preselected) {
        for (const item of items) {
          if (preselected) break;
          const label = typeof item.label === 'string' ? item.label : item.label.label;
          const desc = typeof item.label === 'string' ? '' : (item.label.description || '');
          const unavailable = desc.includes('不可用') || desc.includes('仅 render');
          if (!unavailable && label.toLowerCase().startsWith(lower)) {
            item.preselect = true;
            preselected = true;
          }
        }
      }
    }

    return items.map(item => {
      if (wordRange && item.range === undefined) item.range = wordRange;
      return item;
    });
  }
}

module.exports = { GCLuaCompletionProvider, itemsForContext, buildSnippet };