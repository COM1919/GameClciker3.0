'use strict';
/**
 * GC Lua 快速修复（语法纠正）提供者
 * 依据诊断结果给出可执行修正：
 *  - 弃用 API -> 替换为推荐 API
 *  - 未定义函数 / 常量表成员 -> 提供相近名称的替换
 *  - 多余的 end / until -> 删除
 *  - 未闭合块 / 括号 -> 在文件末尾补全
 *  - BeginChild/EndChild 不成对 -> 补全缺失项
 */

const vscode = require('vscode');
const api = require('../data/api');

/** 计算编辑距离 */
function levenshtein(a, b) {
  const m = a.length;
  const n = b.length;
  const dp = Array.from({ length: m + 1 }, () => new Array(n + 1).fill(0));
  for (let i = 0; i <= m; i++) dp[i][0] = i;
  for (let j = 0; j <= n; j++) dp[0][j] = j;
  for (let i = 1; i <= m; i++) {
    for (let j = 1; j <= n; j++) {
      const cost = a[i - 1] === b[j - 1] ? 0 : 1;
      dp[i][j] = Math.min(dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost);
    }
  }
  return dp[m][n];
}

/** 在候选集中找最相近的名称 */
function closest(name, candidates, maxDistance = 4) {
  let best = null;
  let bestDist = Infinity;
  const lower = name.toLowerCase();
  for (const c of candidates) {
    const d = levenshtein(lower, c.toLowerCase());
    if (d < bestDist) { bestDist = d; best = c; }
  }
  return bestDist <= maxDistance ? best : null;
}

const ALL_API = api.GC_API_NAMES;

class GCLuaCodeActionProvider {
  provideCodeActions(document, range, context) {
    const actions = [];
    for (const diag of context.diagnostics) {
      const action = this.actionFor(document, diag);
      if (action) actions.push(action);
    }
    return actions;
  }

  actionFor(document, diag) {
    const code = diag.code;
    const badText = document.getText(diag.range);

    // 1. 弃用 API
    if (code === 'deprecated-api') {
      const dep = api.GC_DEPRECATED[badText];
      if (dep && dep.replacement) {
        const action = new vscode.CodeAction(`替换为 ${dep.replacement}`, vscode.CodeActionKind.QuickFix);
        action.edit = new vscode.WorkspaceEdit();
        action.edit.replace(document.uri, diag.range, dep.replacement);
        action.diagnostics = [diag];
        action.isPreferred = true;
        return action;
      }
    }

    // 2. 未定义函数 -> 相近 API
    if (code === 'undefined-function') {
      const target = closest(badText, ALL_API.concat(Object.keys(api.LUA_STDLIB.libraries)));
      if (target) {
        const action = new vscode.CodeAction(`是否想使用 ${target}？`, vscode.CodeActionKind.QuickFix);
        action.edit = new vscode.WorkspaceEdit();
        action.edit.replace(document.uri, diag.range, target);
        action.diagnostics = [diag];
        action.isPreferred = true;
        return action;
      }
    }

    // 3. 常量表成员
    if (code === 'unknown-key') {
      const member = badText.replace(/^KEY\./, '');
      const target = closest(member, Object.keys(api.KEY_CONSTANTS));
      if (target) return replaceAction(document, diag, `KEY.${target}`);
    }
    if (code === 'unknown-sc') {
      const member = badText.replace(/^SC\./, '');
      const target = closest(member, Object.keys(api.SC_CONSTANTS));
      if (target) return replaceAction(document, diag, `SC.${target}`);
    }
    if (code === 'unknown-gk') {
      const member = badText.replace(/^GK\./, '');
      const target = closest(member, Object.keys(api.GK_CONSTANTS));
      if (target) return replaceAction(document, diag, `GK.${target}`);
    }
    if (code === 'unknown-member') {
      const member = badText.split('.').pop();
      const target = closest(member, ['OpenReader', 'GetNewMsg']);
      if (target) return replaceAction(document, diag, `minecraft.chat.${target}`);
    }

    // 4. 多余的 end / until -> 删除所在行
    if (code === 'unexpected-end' || code === 'unexpected-until') {
      const line = document.lineAt(diag.range.start.line);
      const delRange = new vscode.Range(line.range.start, line.rangeIncludingLineBreak.end);
      const action = new vscode.CodeAction(`删除多余的 \`${badText}\``, vscode.CodeActionKind.QuickFix);
      action.edit = new vscode.WorkspaceEdit();
      action.edit.delete(document.uri, delRange);
      action.diagnostics = [diag];
      return action;
    }

    // 5. 未闭合块 -> 在末尾补 end / until
    if (code === 'unclosed-block') {
      const closer = diag.message.includes('until') ? 'until false' : 'end';
      return appendAction(document, diag, `在文件末尾补全 \`${closer}\``, `\n${closer}\n`);
    }

    // 6. 未闭合括号 -> 补全
    if (code === 'unclosed-bracket') {
      const map = { '(': ')', '[': ']', '{': '}' };
      const closer = map[badText];
      if (closer) return appendAction(document, diag, `补全 \`${closer}\``, closer);
    }

    // 7. 成对 API 不成对 -> 补全
    if (code === 'unbalanced-pair') {
      const isBegin = badText === 'BeginChild' || badText === 'BeginGroup';
      const counterpart = isBegin
        ? (badText === 'BeginChild' ? '\nEndChild()\n' : '\nEndGroup()\n')
        : (badText === 'EndChild' ? 'BeginChild("####", 0, 0)\n' : 'BeginGroup()\n');
      return appendAction(document, diag, `补全成对的 \`${isBegin ? counterpart.trim() : counterpart.trim()}\``, counterpart);
    }

    return null;
  }
}

function replaceAction(document, diag, newText) {
  const action = new vscode.CodeAction(`替换为 ${newText}`, vscode.CodeActionKind.QuickFix);
  action.edit = new vscode.WorkspaceEdit();
  action.edit.replace(document.uri, diag.range, newText);
  action.diagnostics = [diag];
  action.isPreferred = true;
  return action;
}

function appendAction(document, diag, title, text) {
  const action = new vscode.CodeAction(title, vscode.CodeActionKind.QuickFix);
  action.edit = new vscode.WorkspaceEdit();
  const end = document.positionAt(document.getText().length);
  action.edit.insert(document.uri, end, text);
  action.diagnostics = [diag];
  return action;
}

module.exports = { GCLuaCodeActionProvider, levenshtein, closest };