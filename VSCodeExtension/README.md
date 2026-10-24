# GC Lua

GC 连点器（GameClicker）脚本的 VS Code 语言扩展。

提供语法高亮、补全、悬停文档、参数提示、诊断与快速修复，配色对齐 Visual Studio 2026 C++。

## 功能

- 语法高亮：Lua 语法、GC API、`KEY` / `SC` / `GK`、`minecraft.chat`、弃用标记
- 补全：GC API、常量表、标准库（含 `regex`）、入口函数、关键字与代码片段
- 悬停文档与参数提示
- 诊断：括号/块配对、未定义函数与变量、参数数量、弃用 API、`render` / `main` 上下文限制、未使用变量、缺少 `local`、顶层执行、死循环等
- 快速修复：拼写纠正、API 替换、补全 `end` / 括号 / 成对控件
- 自动配对括号、引号与块注释

## 安装

```powershell
code --install-extension gclua-1.0.12.vsix --force
```

安装后执行 `Developer: Reload Window`。

如果 `.lua` 没有自动识别为 GC Lua，执行命令 **GC Lua: 将当前文件设为 GC Lua 语言**。

## 配置

| 配置 | 默认 | 说明 |
| --- | --- | --- |
| `gclua.diagnostics.enable` | `true` | 诊断总开关 |
| `gclua.diagnostics.contextScope` | `warning` | `render` 仅限 UI API，`main` 等禁用 UI API |
| `gclua.diagnostics.missingLocal` | `warning` | 变量缺少 `local` |
| `gclua.diagnostics.topLevelCode` | `error` | 顶层自动执行代码 |
| `gclua.completion.enable` | `true` | 补全 |
| `gclua.hover.enable` | `true` | 悬停文档 |
| `gclua.signatureHelp.enable` | `true` | 参数提示 |

其余诊断级别可在扩展设置中调整。

## 开发

```powershell
node test/run-tests.js
node node_modules\@vscode\vsce\vsce package --no-dependencies
```

## 许可

MIT
