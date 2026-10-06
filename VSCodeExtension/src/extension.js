'use strict';
/**
 * GC Lua 扩展入口
 * 注册 IntelliSense 相关 Provider 与诊断。
 */

const vscode = require('vscode');
const { computeDiagnostics } = require('./diagnostics');
const { GCLuaCompletionProvider } = require('./completionProvider');
const { GCLuaHoverProvider } = require('./hoverProvider');
const { GCLuaSignatureProvider } = require('./signatureProvider');
const { GCLuaCodeActionProvider } = require('./codeActions');
const { GCLuaCodeLensProvider, ENTRY: ENTRY_FUNCTIONS } = require('./codeLensProvider');

const LANGUAGE_ID = 'gclua';
// 同时服务于 GC Lua 与普通 Lua 语言模式：
// 即使 .lua 文件被 VSCode 内置 lua 语言（或第三方 Lua 扩展）接管，IntelliSense 与诊断依然生效
const LANGUAGE_IDS = ['gclua', 'lua'];
const DIAGNOSTIC_SOURCE = 'GC Lua';

const TEMPLATE = `-- GC Lua 脚本模板
-- 生命周期：initScript 初始化 / main 主循环 / render 设置界面 / saveConfig 保存配置

local enable = false

function initScript()
    SetScriptLoopDelay(10)
end

function main()
    if IsScriptKeyClick() then
        enable = not enable
        PrintLog("[MyScript]" .. (enable and "已开启" or "已关闭"), 1)
    end
end

function render()
    Text("我的脚本设置")
end

function saveConfig()
end
`;

/** 严重级别字符串 -> vscode 枚举 */
function toSeverity(sev) {
  switch (sev) {
    case 'error': return vscode.DiagnosticSeverity.Error;
    case 'warning': return vscode.DiagnosticSeverity.Warning;
    case 'information': return vscode.DiagnosticSeverity.Information;
    case 'hint': return vscode.DiagnosticSeverity.Hint;
    default: return vscode.DiagnosticSeverity.Warning;
  }
}

/** 读取扩展配置 */
function readOptions() {
  const cfg = vscode.workspace.getConfiguration('gclua');
  return {
    enable: cfg.get('diagnostics.enable', true),
    undefinedFunction: cfg.get('diagnostics.undefinedFunction', 'warning'),
    argumentCount: cfg.get('diagnostics.argumentCount', 'warning'),
    deprecated: cfg.get('diagnostics.deprecated', true),
    contextScope: cfg.get('diagnostics.contextScope', 'warning'),
    unused: cfg.get('diagnostics.unused', 'hint'),
    redundant: cfg.get('diagnostics.redundant', 'hint'),
    suspicious: cfg.get('diagnostics.suspicious', 'warning'),
    typeCheck: cfg.get('diagnostics.typeCheck', 'warning'),
    undefinedVariable: cfg.get('diagnostics.undefinedVariable', 'hint'),
    entry: cfg.get('diagnostics.entry', 'warning'),
    missingLocal: cfg.get('diagnostics.missingLocal', 'warning'),
    topLevelCode: cfg.get('diagnostics.topLevelCode', 'error'),
    duplicateEntry: cfg.get('diagnostics.duplicateEntry', 'error'),
    completion: cfg.get('completion.enable', true),
    hover: cfg.get('hover.enable', true),
    signatureHelp: cfg.get('signatureHelp.enable', true),
    codeLensEntry: cfg.get('codeLens.entryFunctions', true)
  };
}

function activate(context) {
  const collection = vscode.languages.createDiagnosticCollection('gclua');
  context.subscriptions.push(collection);

  const timers = new Map();

  function refresh(document) {
    if (!LANGUAGE_IDS.includes(document.languageId)) return;
    const opts = readOptions();
    const raw = computeDiagnostics(document.getText(), opts);
    const diags = raw.map(d => {
      const diag = new vscode.Diagnostic(
        new vscode.Range(
          d.range.start.line, d.range.start.character,
          d.range.end.line, d.range.end.character
        ),
        d.message,
        toSeverity(d.severity)
      );
      diag.source = DIAGNOSTIC_SOURCE;
      diag.code = d.code;
      return diag;
    });
    collection.set(document.uri, diags);
  }

  function scheduleRefresh(document) {
    const key = document.uri.toString();
    if (timers.has(key)) clearTimeout(timers.get(key));
    timers.set(key, setTimeout(() => {
      timers.delete(key);
      refresh(document);
    }, 300));
  }

  // 打开/已存在的文档
  vscode.workspace.textDocuments.forEach(refresh);

  context.subscriptions.push(
    vscode.workspace.onDidOpenTextDocument(refresh),
    vscode.workspace.onDidChangeTextDocument(e => scheduleRefresh(e.document)),
    vscode.workspace.onDidCloseTextDocument(doc => {
      collection.delete(doc.uri);
      const key = doc.uri.toString();
      if (timers.has(key)) { clearTimeout(timers.get(key)); timers.delete(key); }
    }),
    vscode.workspace.onDidChangeConfiguration(e => {
      if (e.affectsConfiguration('gclua')) {
        vscode.workspace.textDocuments.forEach(refresh);
      }
    })
  );

  const opts = readOptions();

  if (opts.completion) {
    context.subscriptions.push(
      vscode.languages.registerCompletionItemProvider(
        LANGUAGE_IDS,
        new GCLuaCompletionProvider(),
        '.', ':'
      )
    );
  }
  if (opts.hover) {
    context.subscriptions.push(
      vscode.languages.registerHoverProvider(LANGUAGE_IDS, new GCLuaHoverProvider())
    );
  }
  if (opts.signatureHelp) {
    context.subscriptions.push(
      vscode.languages.registerSignatureHelpProvider(
        LANGUAGE_IDS,
        new GCLuaSignatureProvider(),
        '(',
        ','
      )
    );
  }
  context.subscriptions.push(
    vscode.languages.registerCodeActionsProvider(LANGUAGE_IDS, new GCLuaCodeActionProvider(), {
      providedCodeActionKinds: [vscode.CodeActionKind.QuickFix]
    })
  );

  // 入口函数标记（CodeLens）
  if (opts.codeLensEntry) {
    context.subscriptions.push(
      vscode.languages.registerCodeLensProvider(LANGUAGE_IDS, new GCLuaCodeLensProvider())
    );
  }
  context.subscriptions.push(
    vscode.commands.registerCommand('gclua.showEntryInfo', (name) => {
      const desc = ENTRY_FUNCTIONS[name];
      vscode.window.showInformationMessage(desc ? `${name}：${desc}` : `${name}: GC 事件函数`);
    })
  );

  // 插入模板命令
  context.subscriptions.push(
    vscode.commands.registerCommand('gclua.insertTemplate', async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor) return;
      await editor.edit(builder => {
        builder.insert(new vscode.Position(0, 0), TEMPLATE);
      });
    })
  );

  // 将当前文件语言设为 GC Lua（应对 .lua 被其它扩展/编辑器抢占的情况）
  context.subscriptions.push(
    vscode.commands.registerCommand('gclua.setLanguage', async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor) {
        vscode.window.showInformationMessage('没有打开的文件。');
        return;
      }
      if (editor.document.languageId === LANGUAGE_ID) {
        vscode.window.showInformationMessage('当前文件已经是 GC Lua 语言。');
        return;
      }
      await vscode.languages.setTextDocumentLanguage(editor.document, LANGUAGE_ID);
      vscode.window.showInformationMessage('已将当前文件语言设为 GC Lua。');
    })
  );
}

function deactivate() {}

module.exports = { activate, deactivate };