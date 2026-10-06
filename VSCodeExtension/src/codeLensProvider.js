'use strict';
/**
 * GC Lua 事件函数标记（CodeLens）
 * 在四个入口函数（initScript / main / render / saveConfig）上方显示一行灰色小字，
 * 标明它是 GC 自动调用的事件函数，风格与编辑器原生 CodeLens 一致，不显突兀。
 */

const vscode = require('vscode');
const scanner = require('./luaScanner');

/** 入口函数 -> 说明 */
const ENTRY = {
  initScript: '脚本初始化，加载时执行一次',
  main: '主循环，按 SetScriptLoopDelay 的间隔反复执行',
  render: '设置界面渲染，只能使用 UI 系列 API',
  saveConfig: '保存配置时执行'
};

class GCLuaCodeLensProvider {
  provideCodeLenses(document) {
    const lenses = [];
    let tokens;
    try {
      tokens = scanner.tokenize(document.getText()).tokens;
    } catch (e) {
      return lenses;
    }
    for (let i = 0; i < tokens.length; i++) {
      const t = tokens[i];
      if (!(t.type === 'keyword' && t.value === 'function')) continue;
      const nt = tokens[i + 1];
      if (!(nt && nt.type === 'name' && ENTRY[nt.value])) continue;
      const pos = new vscode.Position(t.line, t.col);
      const range = new vscode.Range(pos, pos);
      lenses.push(new vscode.CodeLens(range, {
        title: `事件函数 · ${ENTRY[nt.value]}`,
        command: 'gclua.showEntryInfo',
        arguments: [nt.value]
      }));
    }
    return lenses;
  }
}

module.exports = { GCLuaCodeLensProvider, ENTRY };
