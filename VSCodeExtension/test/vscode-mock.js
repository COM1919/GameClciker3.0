'use strict';
/**
 * 极简 vscode 模块替身，用于在 Node 中测试依赖 vscode API 的纯逻辑部分。
 * 仅实现测试所必需的最小接口。
 */

class Position {
  constructor(line, character) { this.line = line; this.character = character; }
}
class Range {
  constructor(a, b, c, d) {
    if (typeof a === 'number') {
      this.start = new Position(a, b);
      this.end = new Position(c, d);
    } else {
      this.start = a;
      this.end = b;
    }
  }
}
class MarkdownString {
  constructor(value) { this.value = value || ''; }
  appendMarkdown(v) { this.value += v; return this; }
}
class SnippetString {
  constructor(value) { this.value = value || ''; }
}
class CompletionItem {
  constructor(label, kind) { this.label = label; this.kind = kind; }
}
class Hover {
  constructor(contents, range) { this.contents = contents; this.range = range; }
}
class ParameterInformation {
  constructor(label, documentation) { this.label = label; this.documentation = documentation; }
}
class SignatureInformation {
  constructor(label, documentation) { this.label = label; this.documentation = documentation; this.parameters = []; }
}
class SignatureHelp {
  constructor() { this.signatures = []; this.activeSignature = 0; this.activeParameter = 0; }
}
class CodeAction {
  constructor(title, kind) { this.title = title; this.kind = kind; }
}
class Diagnostic {
  constructor(range, message, severity) { this.range = range; this.message = message; this.severity = severity; }
}
class CodeLens {
  constructor(range, command) { this.range = range; this.command = command; }
}

const CompletionItemKind = {
  Text: 0, Method: 1, Function: 2, Constructor: 3, Field: 4, Variable: 5,
  Class: 6, Struct: 7, Interface: 8, Module: 9, Property: 10, Unit: 11,
  Value: 12, Enum: 13, Keyword: 14, Snippet: 15, Color: 16, File: 17,
  Reference: 18, Folder: 19, EnumMember: 20, Constant: 21
};
const CompletionItemTag = { Deprecated: 1 };
const DiagnosticSeverity = { Error: 0, Warning: 1, Information: 2, Hint: 3 };
const CodeActionKind = { QuickFix: 'quickfix', Refactor: 'refactor' };

module.exports = {
  Position,
  Range,
  MarkdownString,
  SnippetString,
  CompletionItem,
  CompletionItemKind,
  CompletionItemTag,
  Hover,
  ParameterInformation,
  SignatureInformation,
  SignatureHelp,
  CodeAction,
  CodeActionKind,
  Diagnostic,
  DiagnosticSeverity,
  CodeLens,
  workspace: {
    getConfiguration: () => ({ get: (k, d) => d })
  },
  window: {},
  languages: {},
  commands: {},
  Uri: { file: (p) => ({ fsPath: p, toString: () => 'file://' + p }) }
};