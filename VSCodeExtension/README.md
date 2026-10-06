# GC Lua

面向 **GC 连点器（GameClicker）** 脚本系统的 VSCode 语言扩展。GC Lua 是在 Lua 5.1（LuaJIT）基础上、由 GC 连点器注入大量专用 API 的“魔改 Lua”，本扩展为它提供与 Visual Studio 2026 同级的 IntelliSense 体验。

## 功能特性

| 能力 | 说明 |
| --- | --- |
| 语法高亮 | 完整 Lua 语法 + GC 专属 API、`KEY` / `SC` / `GK` 常量表、`minecraft.chat` 命名空间、已弃用 API 的失效标记 |
| 智能补全 | GC API（带参数占位与中文文档）、常量表成员、`minecraft.chat`、Lua 可用标准库、生命周期函数、关键字与代码片段 |
| 悬停文档 | 鼠标悬停显示签名、参数、返回值、示例、受限/弃用提示 |
| 参数提示 | 调用括号内实时显示参数列表并高亮当前参数 |
| 错误识别 | 未闭合字符串/注释、括号不匹配、块关键字（`function`/`if`/`do`/`repeat`）未闭合或多余、`repeat` 误用 `end` |
| 警告识别 | 未定义的 GC API、参数数量不符、已弃用/已移除的 API、沙箱中不可用的库（`io`/`package`/`debug`）、未知常量成员、`BeginChild`/`EndChild` 等不成对 |
| 语法纠正 | 一键快速修复：替换为推荐 API、按名称相似度修正拼写、删除多余的 `end`、补全未闭合块/括号、补全成对控件 |
| 括号补全 | 括号、引号、块注释自动配对，智能缩进与 `else`/`elseif` 智能回退 |

## 配色（VS2026）

扩展内置颜色主题 **GC Lua — VS2026 Dark**，色板对齐 VSCode “2026 Dark”（VS2026 风格）：

| 语义 | 颜色 |
| --- | --- |
| 关键字 | `#FF7B72` |
| 字符串 | `#A5D6FF` |
| 注释 | `#8B949E` |
| GC API / 常量表 / 标准库 | `#79C0FF` |
| 用户函数名 | `#D2A8FF` |
| 已弃用 / 非法 | `#FFA198`（斜体） |

**启用方式**：按 `Ctrl+K Ctrl+T` 选择颜色主题 → **GC Lua — VS2026 Dark**。
另外，扩展已为 GC 专属作用域提供 VS2026 色值默认值，因此**即使不切换主题**，GC API、常量表与弃用标记同样按 VS2026 配色显示。

## 安装

### 方式一：安装打包好的扩展（推荐）

```powershell
# Microsoft VS Code
code --install-extension gclua-1.0.2.vsix
```

若使用其它 VS Code 内核编辑器（如 Trae），请用其自带 CLI：

```powershell
& "D:\Program Files\TRAE SOLO CN\bin\trae-solo-cn.cmd" --install-extension gclua-1.0.2.vsix --force
```

> 安装后需**重启编辑器**（或执行 `Developer: Reload Window`）才会生效。
> 若 `.lua` 未识别为 GC Lua，可执行命令 **GC Lua: 将当前文件设为 GC Lua 语言**。

### 方式二：开发调试

1. 用 VSCode 打开本 `VSCodeExtension` 文件夹；
2. 按 `F5` 启动“运行 GC Lua 扩展”，会打开一个扩展开发宿主窗口；
3. 在宿主窗口中打开任意 `.lua` 脚本即可体验。

## 使用说明

本扩展注册了独立的语言 `gclua`，同时关联 `.lua` 与 `.gclua` 扩展名。

- 若同时安装了其它 Lua 扩展，可能出现语法/补全竞争。可点击 VSCode 右下角语言模式，手动选择 **GC Lua**；或对 GC 脚本目录禁用其它 Lua 扩展。
- 若 `render.md` 里出现但脚本系统并未注册的函数（如 `PopTip`、`Helpmarker`），扩展会标记为**已弃用**并给出替代方案，避免运行时报错。

## 配置项

| 配置 | 默认值 | 说明 |
| --- | --- | --- |
| `gclua.diagnostics.enable` | `true` | 总开关：错误与警告识别 |
| `gclua.diagnostics.undefinedFunction` | `warning` | 未定义函数的严重级别 |
| `gclua.diagnostics.argumentCount` | `warning` | 参数数量不符的严重级别 |
| `gclua.diagnostics.deprecated` | `true` | 是否提示弃用/移除 API |
| `gclua.diagnostics.contextScope` | `warning` | 上下文 API 限制：`render` 内只能用 UI 系列，`main` 等禁止 UI 系列（`off` 关闭） |
| `gclua.completion.enable` | `true` | 智能补全开关 |
| `gclua.hover.enable` | `true` | 悬停文档开关 |
| `gclua.signatureHelp.enable` | `true` | 参数提示开关 |

## 数据来源

扩展的 API 数据以 GC 连点器脚本系统的**实际注册表**为准（`Script.h` 中的 `LuaInit`），并结合 `script-docs/functions.md`、`script-docs/render.md` 补全中文说明。已弃用/未注册 API 单独标记，以保证“文档里写着、实际却调用会报错”的情况能被提前发现。

## 目录结构

```
VSCodeExtension/
├── package.json                 扩展清单
├── language-configuration.json  括号/注释/缩进配置
├── syntaxes/gclua.tmLanguage.json  语法高亮
├── snippets/gclua.json          代码片段
├── themes/vs2026-dark.json      VS2026 配色主题
├── themes/dark_modern.json      主题底座（VS2026 主题所引用）
├── data/api.js                  GC Lua API 数据表
├── src/
│   ├── extension.js             入口与 Provider 注册
│   ├── luaScanner.js            轻量词法/结构扫描
│   ├── diagnostics.js           诊断引擎
│   ├── completionProvider.js    智能补全
│   ├── hoverProvider.js         悬停文档
│   ├── signatureProvider.js     参数提示
│   ├── codeActions.js           快速修复
│   └── docFormatter.js          文档格式化
└── test/run-tests.js            核心逻辑自测
```

## 自测

```powershell
node test/run-tests.js
```

## 许可

MIT