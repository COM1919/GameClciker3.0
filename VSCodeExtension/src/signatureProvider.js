'use strict';
/**
 * GC Lua 函数参数提示（Signature Help）提供者
 * 支持在调用括号内自动定位当前参数。
 */

const vscode = require('vscode');
const scanner = require('./luaScanner');
const fmt = require('./docFormatter');

/**
 * 根据光标偏移，找出当前所在的调用上下文
 * @param {string} text 全文
 * @param {number} offset 光标偏移
 * @returns {{name:string, activeParameter:number, openIndex:number}|null}
 */
function findCallContext(text, offset) {
  const { tokens } = scanner.tokenize(text);
  const mask = scanner.buildCodeMask(text, tokens);

  // 从光标向前寻找匹配的 '('
  let depth = 0;
  let openIndex = -1;
  for (let i = offset - 1; i >= 0; i--) {
    const c = mask[i];
    if (c === ')' || c === ']' || c === '}') { depth++; continue; }
    if (c === '(' || c === '[' || c === '{') {
      if (depth === 0) {
        if (c === '(') { openIndex = i; }
        break;
      }
      depth--;
      continue;
    }
    // 遇到语句边界则停止
    if (depth === 0 && (c === '\n' || c === ';')) break;
  }
  if (openIndex < 0) return null;

  // 取 '(' 之前的函数名（支持点号路径）
  let s = openIndex - 1;
  while (s >= 0 && /[A-Za-z0-9_.]/.test(mask[s])) s--;
  const name = mask.slice(s + 1, openIndex).trim();
  if (!name || !/^[A-Za-z_]/.test(name)) return null;

  // 统计从 '(' 到光标的顶层逗号数量
  let d = 0;
  let activeParameter = 0;
  for (let i = openIndex + 1; i < offset; i++) {
    const c = mask[i];
    if (c === '(' || c === '[' || c === '{') d++;
    else if (c === ')' || c === ']' || c === '}') d--;
    else if (c === ',' && d === 0) activeParameter++;
  }

  return { name, activeParameter, openIndex };
}

class GCLuaSignatureProvider {
  provideSignatureHelp(document, position) {
    const text = document.getText();
    const offset = document.offsetAt(position);
    const ctx = findCallContext(text, offset);
    if (!ctx) return null;

    const def = fmt.findByName(ctx.name);
    if (!def) return null;

    const info = fmt.signatureInfo(def);
    const sig = new vscode.SignatureInformation(info.label, new vscode.MarkdownString(info.documentation));
    sig.parameters = info.parameters.map(p => new vscode.ParameterInformation(p.label, new vscode.MarkdownString(p.documentation)));

    const help = new vscode.SignatureHelp();
    help.signatures = [sig];
    help.activeSignature = 0;
    help.activeParameter = Math.min(ctx.activeParameter, Math.max(0, sig.parameters.length - 1));
    return help;
  }
}

module.exports = { GCLuaSignatureProvider, findCallContext };