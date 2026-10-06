'use strict';
/**
 * GC Lua 悬停文档提供者
 */

const vscode = require('vscode');
const api = require('../data/api');
const fmt = require('./docFormatter');

/** 提取光标处的完整名称（支持点号路径） */
function getFullName(document, position) {
  const line = document.lineAt(position.line).text;
  const isNameChar = (c) => /[A-Za-z0-9_.]/.test(c);
  let start = position.character;
  let end = position.character;
  while (start > 0 && isNameChar(line[start - 1])) start--;
  while (end < line.length && isNameChar(line[end])) end++;
  if (start === end) return null;
  return { name: line.slice(start, end), range: new vscode.Range(position.line, start, position.line, end) };
}

class GCLuaHoverProvider {
  provideHover(document, position) {
    const info = getFullName(document, position);
    if (!info) return null;
    const name = info.name;
    const parts = name.split('.');

    // KEY / SC / GK 常量
    if (parts[0] === 'KEY' && parts[1]) {
      const v = api.KEY_CONSTANTS[parts[1]];
      if (v !== undefined) {
        return new vscode.Hover(new vscode.MarkdownString(`**KEY.${parts[1]}**\n\n按键代码：\`${v}\`（0x${v.toString(16).toUpperCase()}）`), info.range);
      }
    }
    if (parts[0] === 'SC' && parts[1] && api.SC_CONSTANTS[parts[1]]) {
      const e = api.SC_CONSTANTS[parts[1]];
      return new vscode.Hover(new vscode.MarkdownString(`**SC.${parts[1]}** = \`${e.value}\`\n\n${e.desc}`), info.range);
    }
    if (parts[0] === 'GK' && parts[1] && api.GK_CONSTANTS[parts[1]]) {
      const e = api.GK_CONSTANTS[parts[1]];
      return new vscode.Hover(new vscode.MarkdownString(`**GK.${parts[1]}** = \`${e.value}\`\n\n${e.desc}`), info.range);
    }

    // 弃用 / 已移除
    if (api.GC_DEPRECATED[name]) {
      const d = api.GC_DEPRECATED[name];
      const fix = d.replacement ? `\n\n**替代方案：** \`${d.replacement}\`` : '';
      return new vscode.Hover(new vscode.MarkdownString(`**${name}**（已弃用）\n\n${d.reason}${fix}`), info.range);
    }

    // GC API
    const def = fmt.findByName(name);
    if (def) {
      const md = new vscode.MarkdownString(fmt.formatDoc(def));
      if (def.restricted) md.appendMarkdown('\n\n> ⚠️ **受限 API**');
      return new vscode.Hover(md, info.range);
    }

    // 生命周期
    if (api.LIFECYCLE[name]) {
      return new vscode.Hover(new vscode.MarkdownString(`**${name}**\n\n${api.LIFECYCLE[name]}`), info.range);
    }

    // 标准库成员
    if (parts.length === 2 && api.LUA_STDLIB.libraries[parts[0]] && api.LUA_STDLIB.libraries[parts[0]].includes(parts[1])) {
      return new vscode.Hover(new vscode.MarkdownString(`**${name}**\n\nLua 标准库函数 \`${parts[0]}\``), info.range);
    }
    if (parts.length === 1 && api.LUA_STDLIB.globals.includes(parts[0])) {
      return new vscode.Hover(new vscode.MarkdownString(`**${name}**\n\nLua 标准库全局函数`), info.range);
    }

    return null;
  }
}

module.exports = { GCLuaHoverProvider, getFullName };