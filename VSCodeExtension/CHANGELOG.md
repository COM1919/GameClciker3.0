# 更新日志

## 1.0.12

- 发布者 ID 改为 `COM1919`
- 新增扩展图标（取自软件图标 `GCicon.ico`）
- 精简 README

## 1.0.11

- **主文件判定改为「包含 `main` 的文件」**（配套引擎改动）：多文件脚本以定义了 `main` 的文件为主文件，其余文件作为依赖；入口函数（`main` / `render` / `initScript` / `saveConfig`）必须写在同一文件；单文件脚本（文件名任意）保持兼容
- **新增 `regex` 正则库补全**：`regex.test` / `regex.find` / `regex.match` / `regex.replace` / `regex.split`，并加入语法高亮
- 文档：新增 `script-docs/lua-base.md`（GC Lua 基础教程：语法、逻辑、符号、文件导入、入口函数规则、各标准库用法、被移除内容与排错）

## 1.0.10

- **引擎支持多文件脚本**（配套改动）：一个脚本包（zip）现在可以包含多个 `.lua` 文件，入口固定为 `script.lua`（没有 `script.lua` 时仍兼容「仅一个 lua 文件」的旧包）；同名子目录中的 lua 文件不会被误当作入口
- **恢复受限的 `require`**：`package` 不再被整体移除，改为受限模式 —— 仅能在脚本自身目录内解析 `?.lua`，C 库加载器被移除、`package.loadlib` 被删除，无法加载本地 DLL
- 扩展同步：`require` 纳入标准库补全；`package` 视为受限可用库（`loadlib` 除外）；`package.xxx` 不再误报为「库已被整体移除」，仅 `package.loadlib` 报移除
- `string` / `table` / `math` / `coroutine` 标准库始终完整可用

## 1.0.9

- **所有符号改为蓝色**：运算符（`+ - * / % ^ # < > <= >= == ~= .. ...`）、标点（`() [] {} , ; .`）统一按关键字蓝 `#569CD6` 着色，不再显示为灰色；其中 `...`（变长参数）单独识别为 `keyword.operator.vararg`，`.`（访问符）单独识别为 `punctuation.accessor`，避免与 `..` 混淆
- 修复主题中「裸 `punctuation` 通配规则」把字符串引号也染灰的问题（收窄为具体的标点/访问符作用域）

## 1.0.8

- **修复表构造被误判「变量需要使用 local」**：`local keyData = { enabled = false, clickKey = 32, lastClickTime = 0, ... }` 里的字段名（`enabled`、`lastClickTime` 等）此前被当作变量定义并报缺少 `local`。现按表构造深度（`{ ... }`，含嵌套）识别表字段，不再误报；表外真正漏写 `local` 的赋值仍会提示
- **修复字符串内 `#` 被错误着色**：移除字符串内对 `#` 的专门着色规则，字符串内容颜色统一；字符串外的 `#`（长度运算符）仍按运算符着色

## 1.0.7

- **配色全面对齐 Visual Studio 2026（C++）**：内置主题改名 `GC Lua — VS2026 C++`，token 配色改用 VS IDE Dark 的 C++ 色板 —— 关键字 `#569CD6`、类型 `#4EC9B0`、函数 `#DCDCAA`、字符串 `#D69D85`、注释 `#57A64A`、数字 `#B5CEA8`、运算符与普通文本 `#D4D4D4`
- **修复 `not` 未高亮**：`and` / `or` / `not` 作为运算符关键字统一按关键字蓝着色（此前继承了基础主题的浅色，看起来像没高亮）
- **修复字符串配色**：字符串的引号定义 `punctuation.definition.string` 与字符串内容统一为 VS C++ 暗橙，避免字符串内符号显示不一致
- **`..` 连接符着色**：`..` 单独标记为 `keyword.operator.concat.gclua`（关键字蓝）；字符串外的 `#`（长度运算符）按运算符着色
- **入口函数特殊标记**：`initScript` / `main` / `render` / `saveConfig` 定义处使用专门的语法作用域（加粗）并在函数上方显示事件函数 CodeLens（如「事件函数 · 主循环」）
- **新增诊断**：
  - 提示：未定义函数、未定义变量、不可达代码、冗余条件 / 冗余分支 / 空代码块、未使用的变量 / 函数
  - 警告：缺少入口函数、变量定义缺少 `local`、死循环
  - 错误：顶层可执行代码（不允许自动执行的 chunk）、重复定义入口函数
- 新增配置项：`undefinedVariable` / `redundant` / `entry` / `missingLocal` / `topLevelCode` / `duplicateEntry` / `codeLens.entryFunctions`；`undefinedFunction` 默认级别调整为提示

## 1.0.6

- **修复“补全只有几个 API”（关键）**：扩展原先只注册在 `gclua` 语言上。在 VS Code 中打开 `.lua` 文件时语言仍是内置的 `lua`，扩展根本没激活，看到的只是默认的单词补全。现在扩展**同时服务于 `lua` 与 `gclua` 两种语言**（补全、悬停、参数提示、诊断、语法高亮全部生效）
- 新增 **未使用变量 / 函数** 警告：从未被引用的 `local` 变量与从未被调用的函数（以 `_` 开头及生命周期函数豁免），可用 `gclua.diagnostics.unused` 配置
- 新增 **可疑逻辑** 检查（`gclua.diagnostics.suspicious`）：
  - `=` 与 `==` 混淆（`if x = 1 then`）
  - 死循环（`while true` 无 `break`/`return`、`for` 步长为 0）
  - 除零 / 对 0 取模
  - 自我赋值（`x = x`）
  - 恒真 / 恒假条件（`if true then`）
  - 空代码块（`then`/`do` 后直接 `end`）
  - 重复定义同名函数
- 新增 **实参类型校验**（`gclua.diagnostics.typeCheck`）：把明显的字面量类型不匹配（如 `Sleep("abc")`、`SetKeyDown("A")`）标注出来

## 1.0.5

- **字符串配色改为 VS2026 暗黄 `#CE9178`**（单引号 / 双引号 / 长字符串统一）
- **`end` 等关键词自动补全**：关键字补全扩展到 22 个，新增 `end`、`do`、`local`、`nil`、`true`、`false`、`and`、`or`、`not` 等；`if`/`for`/`while`/`do`/`function` 的代码片段自动补齐 `end`
- 新增代码片段：`gcifelse`、`gcifelseif`、`gcdo`、`gclocal`、`gcwhile`、`gcfor`、`gcforin`
- 补全**分类与默认选择**优化：每项带中文类别标签（功能 / UI / 系统 / Lua 等），按上下文可用性分组排序，并根据已输入前缀智能 `preselect`
- 修复 `GK` 未知成员提示中仍列出已移除的 `GK.API` 的 BUG

## 1.0.4

- **补全不再按上下文隐藏 API**：智能补全现在**始终给出全部 87 个 API**（此前在 `main` 中看不到 UI API、在 `render` 中看不到功能性 API，容易被误认为“补全缺 API”）
- 对当前函数上下文中**不可用**的 API 改为「标注 + 排序靠后」：非 render 上下文中的 UI API 标注 `仅 render 可用`，`render` 中的功能性 API 标注 `render 内不可用`
- `render` / `main` 的 UI 使用限制仍由**诊断**强制执行（错误/警告），与补全互不影响

## 1.0.3

- **修复语法高亮整体失效（关键）**：`keywords` 规则中存在一条 Oniguruma 非法正则 `\b(...)\b(?=\s*\()?`（把量词 `?` 施加在断言组上），导致 VSCode 的 TextMate 引擎**编译整份语法失败 → 文件全部无高亮**。已修正（这也是此前“打开 lua 全是白字”的根因之一）
- 修正语法规则顺序：`deprecated` 规则移到 `functions` 之前，使 `dofile` / `print` 等被正确标记为失效（此前被当作普通函数调用）
- 新增回归测试：检测「对断言组施加量词」这类 Oniguruma 非法正则、并校验规则顺序

## 1.0.2

- **VS2026 配色**：内置颜色主题 `GC Lua — VS2026 Dark`（对齐 VS Code “2026 Dark” 色板），并为 GC 专属作用域提供 VS2026 色值默认值（无需切换主题也能生效）
- 修复补全替换范围 BUG：`KEY.` / `SC.` / `GK.` / 标准库 / `minecraft.chat.` 的补全不再重复或丢失前缀
- 修复 `obj:Method()` 方法调用被误报为「未定义函数」
- 修正语法细节：`fileTypes` 去掉点号、`GK` 高亮移除未注册的 `GK.API`
- `GC_RESTRICTED` 改为由各 API 的 `restricted` 标记派生，保持一致
- 新增命令「GC Lua: 将当前文件设为 GC Lua 语言」
- 生命周期函数文档补充 UI 使用限制说明

## 1.0.1

- 新增上下文 API 限制：`render` 函数中只能使用 UI 系列 API，`main` 及其他函数中不允许使用 UI 系列 API（补全与诊断同时生效，可通过 `gclua.diagnostics.contextScope` 配置）
- 智能补全按函数上下文过滤：`render` 内只提示 UI 系列 API，其他位置只提示功能性 API
- 修正 API 签名：`DrawSpiltLine` 支持可选长度参数、`DrawCircleFilled` 需要 4 个参数
- 将已禁用的 `IsCloseUI` 与已移除的 `load` 纳入弃用识别（诊断、悬停、语法高亮）
- 修正 `GK` 常量表（移除未注册的 `GK.API`）
- 修复：`DrawSpiltLine(300)` 等合法调用不再被误报为参数过多

## 1.0.0

- 首个版本：GC Lua 语言支持
- 语法高亮、智能补全、悬停文档、参数提示
- 错误/警告识别：括号与块配对、未定义 API、参数数量、弃用与沙箱移除 API、常量表成员校验
- 语法纠正：基于快速修复的 API 替换与结构补全
- 括号、引号自动补全与智能缩进