# GC Lua 基础教程

> 本文覆盖 GC Lua 语言基础、逻辑与符号、多文件导入、入口函数规则，以及每个可用标准库的使用方法。
>
> GC Lua 基于 **LuaJIT（Lua 5.1 语法）**，并做了沙箱裁剪：大部分标准库完整可用，但危险库被移除（详见最后一章）。

---

## 一、脚本结构与主文件规则

一个 GC 脚本是一个压缩包（`.zip`），解压后目录中至少包含一个 `.lua` 文件，可选包含 `info.ini`、`icon.png`、`readme.txt`。

### 1. 入口函数

GC 通过以下 **4 个全局函数** 驱动脚本，它们都必须定义在 **同一个文件** 里：

| 入口函数 | 调用时机 | 限制 |
|----------|----------|------|
| `initScript()` | 脚本加载完成后调用一次，适合做一次性初始化 | **不能使用 UI 系列 API** |
| `main()` | 主循环，按 `SetScriptLoopDelay` 设定的间隔反复调用 | **不能使用 UI 系列 API** |
| `render()` | 设置界面渲染时由 UI 线程调用 | **只能使用 UI 系列 API** |
| `saveConfig()` | 保存配置时调用，用于持久化 | **不能使用 UI 系列 API** |

```lua
function initScript()
    SetScriptLoopDelay(10)          -- 主循环间隔 10ms
end

function main()
    -- 主逻辑写在这里
end

function render()
    -- 只能调用 Button / Checkbox / Text / Draw* 等 UI API
end

function saveConfig()
end
```

> **重要：** 入口函数必须是**全局函数**（直接写 `function main()`），不要写 `local function main()`，否则 GC 找不到它。

### 2. 多文件脚本与主文件判定

- 压缩包内**只有一个** `.lua` 文件时：这个文件就是主文件（**文件名任意**，兼容旧版脚本）。
- 压缩包内有**多个** `.lua` 文件时：GC 会查找**定义了 `main` 函数**的那个文件作为**主文件**，其余文件都作为**依赖**，只有被 `require` 导入时才会执行。

因此约定如下：

> **`main`、`render`、`initScript`、`saveConfig` 必须全部写在主文件（含 `main` 的那个文件）中**，依赖文件里只放工具函数与数据。

```
MyScript.zip
├── info.ini
├── icon.png
├── main.lua        ← 主文件：包含 main / render / initScript / saveConfig
└── utils.lua       ← 依赖：工具函数，被 require 导入
```

### 3. 顶层不要写“会自动执行”的代码

脚本主文件在加载时会被执行一次，但 GC 约定**不允许编写自动执行的 chunk**（顶层直接调用 API）。请把逻辑放进入口函数：

```lua
-- ✗ 错误：顶层自动执行
PrintLog("[MyScript]我执行了", 1)

-- ✓ 正确：放进 initScript
function initScript()
    PrintLog("[MyScript]我执行了", 1)
end

-- ✓ 允许：顶层只做 local 声明与函数定义
local utils = require("utils")
function main()
end
```

---

## 二、基本语法

### 1. 注释

```lua
-- 单行注释

--[[
  多行注释
]]

--[[
  多行注释也可以这样嵌套
  --[[ 内层 ]]--
]]
```

### 2. 变量与 `local`

Lua 的变量默认是**全局**的，GC 约定**定义变量必须加 `local`**（全局变量污染是常见 BUG 来源）。

```lua
local count = 0          -- ✓ 局部变量
local name = "GC"        -- ✓

count = 1                -- ✗ 这是在创建/修改全局变量，会告警
```

> 只读地引用全局 API（如 `PrintLog(...)`）不受影响，这条规则针对的是**赋值定义变量**。

### 3. 数据类型

| 类型 | 说明 | 示例 |
|------|------|------|
| `nil` | 空值，表示“不存在” | `local a = nil` |
| `boolean` | 布尔 | `true` / `false` |
| `number` | 数字（整数与浮点统一为 double） | `100`、`3.14`、`0x1F` |
| `string` | 字符串 | `"abc"`、`'abc'`、`[[abc]]` |
| `table` | 表（数组 / 字典 / 对象都用它） | `{1, 2, 3}`、`{a = 1}` |
| `function` | 函数 | `function() end` |

用 `type(x)` 判断类型：

```lua
local t = type(123)        -- "number"
local s = type("abc")      -- "string"
local b = type(true)       -- "boolean"
local n = type(nil)        -- "nil"
local tb = type({})        -- "table"
local f = type(print)      -- "function"
```

### 4. 运算符与符号

| 分类 | 符号 | 说明 |
|------|------|------|
| 算术 | `+` `-` `*` `/` `%` `^` | 加减乘除、取模、幂 |
| 赋值 | `=` | 赋值 |
| 比较 | `==` `~=` `<` `>` `<=` `>=` | `~=` 是“不等于” |
| 逻辑 | `and` `or` `not` | 与、或、非 |
| 连接 | `..` | 字符串拼接 |
| 长度 | `#` | 取字符串长度或表长度 |
| 其他 | `(` `)` `[` `]` `{` `}` `,` `;` `.` `:` | 括号、下标、分隔符、成员访问、方法调用 |

```lua
local a = 10 + 3 * 2          -- 16
local b = 2 ^ 10              -- 1024
local c = 10 % 3              -- 1

local s = "GC" .. " Lua"      -- "GC Lua"
local len = #"hello"          -- 5
local n = #{1, 2, 3}          -- 3

local ok = (a > 1) and not false   -- true
```

> **注意：`..` 与 `...` 的区别**
> - `..` 是字符串连接符：`"a" .. "b"`
> - `...` 是函数可变参数：`function f(...)`
>
> 两者不要写混。

---

## 三、逻辑控制

### 1. 条件判断

```lua
local x = 10
if x > 20 then
    PrintLog("[Demo]大于20", 1)
elseif x > 5 then
    PrintLog("[Demo]大于5", 1)
else
    PrintLog("[Demo]小于等于5", 1)
end
```

在 Lua 中，**`nil` 与 `false` 为假，其余一切为真**（包括 `0` 和 `""`）。

```lua
if 0 then
    -- 这里会执行！因为 0 为真
end
```

### 2. 数值 for 循环

```lua
-- for 变量 = 起点, 终点 [, 步长] do ... end
for i = 1, 5 do
    PrintLog("[Demo]" .. i, 1)
end

for i = 10, 1, -2 do
    PrintLog("[Demo]" .. i, 1)   -- 10 8 6 4 2
end
```

> 步长不能为 `0`，否则会造成死循环。

### 3. 泛型 for 循环

```lua
local arr = {"a", "b", "c"}
for index, value in ipairs(arr) do
    PrintLog("[Demo]" .. index .. "=" .. value, 1)
end

local dict = {name = "GC", ver = 3}
for key, value in pairs(dict) do
    PrintLog("[Demo]" .. key .. "=" .. tostring(value), 1)
end
```

- `ipairs` 只遍历数组部分（从 1 开始连续整数下标），遇到 `nil` 停止。
- `pairs` 遍历所有键值对（顺序不保证）。

### 4. while 与 repeat

```lua
local i = 0
while i < 3 do
    i = i + 1
end

repeat
    i = i - 1
until i <= 0
```

### 5. break 与 return

```lua
for i = 1, 100 do
    if i == 5 then
        break          -- 跳出循环
    end
end

function check(x)
    if x < 0 then
        return false   -- 提前返回
    end
    return true
end
```

---

## 四、函数与表

### 1. 函数

```lua
-- 全局函数
function Add(a, b)
    return a + b
end

-- 局部函数
local function Sub(a, b)
    return a - b
end

-- 多返回值
local function DivMod(a, b)
    return math.floor(a / b), a % b
end
local q, r = DivMod(10, 3)      -- q=3, r=1

-- 可变参数
local function Sum(...)
    local total = 0
    for _, v in ipairs({...}) do
        total = total + v
    end
    return total
end
```

### 2. 表（table）

表是 Lua 唯一的数据结构，既能当数组也能当字典：

```lua
-- 数组（下标从 1 开始！）
local arr = {10, 20, 30}
PrintLog("[Demo]" .. arr[1], 1)      -- 10
arr[4] = 40

-- 字典
local cfg = {enabled = true, cps = 8}
PrintLog("[Demo]" .. tostring(cfg.cps), 1)

-- 嵌套
local data = {
    enabled = false,
    clickKey = 32,
    lastClickTime = 0,
    nested = {x = 1, y = 2}
}
```

> **注意：** 表构造 `{ ... }` 里的 `键 = 值` 是**表字段**，不是变量定义，不需要也不应该加 `local`。
>
> ```lua
> local keyData = {          -- local 加在这里
>     enabled = false,       -- 这是表字段
>     lastClickTime = 0      -- 这是表字段
> }
> ```

---

## 五、文件导入（require）

多文件脚本通过 `require` 导入同一脚本目录下的其他 `.lua` 文件。

```lua
-- 顶层做 local 声明是被允许的
local utils = require("utils")
```

### 1. 路径规则

- `require("utils")` → 加载**同目录**下的 `utils.lua`
- `require("lib.util")` → 加载同目录下 `lib/util.lua`（点号对应目录分隔）

### 2. 模块写法（推荐）

模块文件应返回一个表：

```lua
-- utils.lua
local utils = {}

function utils.clamp(v, min, max)
    if v < min then return min end
    if v > max then return max end
    return v
end

return utils
```

使用：

```lua
-- main.lua
local utils = require("utils")

function main()
    local v = utils.clamp(120, 0, 100)   -- 100
end
```

### 3. 注意事项

- `require` **只能加载脚本自身目录及其子目录**下的 `.lua` 文件，无法访问目录之外的文件（沙箱限制）。
- 同一个模块只会被执行一次（结果被缓存），重复 `require` 返回同一张表。
- 入口函数（`main` / `render` / `initScript` / `saveConfig`）**必须写在主文件**里，不要写进依赖模块。
- 依赖模块里**不要**写会自动执行的语句；把逻辑包成函数，让主文件调用。

---

## 六、标准库教程

### 1. string 库（字符串）

Lua 的字符串模式匹配（`find` / `match` / `gmatch` / `gsub`）使用的是 **Lua 自身的模式语法**，不是正则表达式；需要正则请使用 `regex` 库。

| 函数 | 说明 | 示例 |
|------|------|------|
| `string.len(s)` | 字符串长度 | `string.len("abc")` → `3` |
| `string.sub(s, i [, j])` | 截取子串（下标从 1 开始，支持负数） | `string.sub("hello", 2, 4)` → `"ell"` |
| `string.upper(s)` / `string.lower(s)` | 转大写 / 小写 | `string.upper("abc")` → `"ABC"` |
| `string.rep(s, n)` | 重复 n 次 | `string.rep("ab", 2)` → `"abab"` |
| `string.reverse(s)` | 反转 | `string.reverse("abc")` → `"cba"` |
| `string.byte(s [, i])` | 取字符编码 | `string.byte("A")` → `65` |
| `string.char(...)` | 编码转字符 | `string.char(65, 66)` → `"AB"` |
| `string.format(fmt, ...)` | 格式化 | `string.format("%d-%s", 3, "a")` → `"3-a"` |
| `string.find(s, pat [, init])` | 查找，返回起止下标 | `string.find("hello", "ll")` → `3 4` |
| `string.match(s, pat)` | 返回匹配到的内容 | `string.match("abc123", "%d+")` → `"123"` |
| `string.gmatch(s, pat)` | 迭代所有匹配 | 见下方示例 |
| `string.gsub(s, pat, repl)` | 替换，返回新串与替换次数 | `string.gsub("a-b", "-", "+")` → `"a+b", 1` |
| `string.dump(f)` | 导出函数字节码 | 一般用不到 |

```lua
local s = "CPS: 12, Time: 34"
local cps = string.match(s, "CPS: (%d+)")
PrintLog("[Demo]cps=" .. tostring(cps), 1)          -- 12

for word in string.gmatch("a,b,c", "[^,]+") do
    PrintLog("[Demo]" .. word, 1)                   -- a / b / c
end

local new, n = string.gsub("a1b2", "%d", "#")
PrintLog("[Demo]" .. new .. " 替换了" .. n .. "次", 1)   -- a#b# 替换了2次
```

Lua 模式常用字符：`%d` 数字、`%a` 字母、`%s` 空白、`%w` 字母数字、`%p` 标点、`.` 任意字符、`*` 0+、`+` 1+、`-` 0+（非贪婪）、`?` 0 或 1。

### 2. table 库

| 函数 | 说明 |
|------|------|
| `table.insert(t, [pos,] value)` | 插入元素（省略 pos 则追加到末尾） |
| `table.remove(t [, pos])` | 删除元素并返回它（省略 pos 则删末尾） |
| `table.concat(t [, sep [, i [, j]]])` | 连接成字符串 |
| `table.sort(t [, comp])` | 排序 |
| `table.maxn(t)` | 最大数字下标 |
| `table.pack(...)` | 把参数打包成表（含 `n` 字段） |
| `table.unpack(t [, i [, j]])` | 拆表为多返回值（等价于全局 `unpack`） |

```lua
local t = {3, 1, 2}
table.insert(t, 5)              -- {3,1,2,5}
table.insert(t, 1, 0)           -- {0,3,1,2,5}
local last = table.remove(t)    -- 5

table.sort(t)                   -- 升序 {0,1,2,3}
table.sort(t, function(a, b) return a > b end)   -- 降序

local s = table.concat({"a", "b", "c"}, "-")     -- "a-b-c"
```

### 3. math 库

| 函数 | 说明 |
|------|------|
| `math.abs(x)` | 绝对值 |
| `math.floor(x)` / `math.ceil(x)` | 向下 / 向上取整 |
| `math.max(...)` / `math.min(...)` | 最大 / 最小 |
| `math.sqrt(x)` | 平方根 |
| `math.pow(x, y)` / `x ^ y` | 幂 |
| `math.fmod(x, y)` / `x % y` | 取模 |
| `math.modf(x)` | 拆分整数与小数部分 |
| `math.random()` / `math.random(m)` / `math.random(m, n)` | 随机数 |
| `math.randomseed(x)` | 设置随机种子 |
| `math.sin/cos/tan/deg/rad/exp/log` | 三角函数与指数对数 |
| `math.huge` | 无穷大 |
| `math.pi` | 圆周率 |

```lua
local v = math.floor(3.7)        -- 3
local r = math.random(1, 100)    -- 1~100 的随机整数
```

> GC 另外提供了更高层的随机 API：`RandomInt(min, max)`、`RandomBool(rate)`，推荐优先使用。

### 4. os 库（已裁剪）

仅保留安全函数，**危险函数已被移除**（见第七章）。

| 函数 | 说明 |
|------|------|
| `os.time()` | 当前时间戳（秒） |
| `os.clock()` | CPU 时间（秒，常用于计时） |
| `os.date(fmt)` | 格式化时间 |
| `os.difftime(t2, t1)` | 时间差（秒） |

```lua
local now = os.time()
local text = os.date("%Y-%m-%d %H:%M:%S")
PrintLog("[Demo]" .. text, 1)
```

### 5. coroutine 库（协程）

| 函数 | 说明 |
|------|------|
| `coroutine.create(f)` | 创建协程 |
| `coroutine.resume(co, ...)` | 启动 / 继续执行 |
| `coroutine.yield(...)` | 挂起 |
| `coroutine.status(co)` | 状态：`suspended` / `running` / `dead` |
| `coroutine.wrap(f)` | 创建协程并返回可调用函数 |
| `coroutine.running()` | 返回当前协程 |

```lua
local co = coroutine.create(function(a)
    local b = coroutine.yield(a + 1)
    return b * 2
end)

local ok1, v1 = coroutine.resume(co, 10)   -- v1 = 11
local ok2, v2 = coroutine.resume(co, 5)    -- v2 = 10
```

### 6. package 库（受限）

`package` 被保留但**受限**，主要用于支持 `require`：

| 字段 | 说明 |
|------|------|
| `package.path` | 模块搜索路径，已被固定为脚本自身目录（`<脚本目录>\?.lua`） |
| `package.loaded` | 已加载模块的缓存表 |
| `package.preload` | 预加载函数表 |
| `package.loaders` | 模块加载器列表（仅保留 preload 与 Lua 文件加载器） |
| `package.cpath` | C 模块路径，已置空 |
| `package.config` | 平台配置字符串 |

> `package.loadlib` 已被移除，**无法加载本地 DLL**。

### 7. regex 库（正则表达式，GC 扩展）

`regex` 是 GC 提供的正则库，底层为 **ECMAScript 语法**（与 JavaScript 的正则一致）。

> **重要：Lua 字符串转义**
> 反斜杠在 Lua 字符串中需要写成 `\\`，或用长字符串 `[[...]]`：
> ```lua
> local p1 = "\\d+"      -- 表示正则 \d+
> local p2 = [[\d+]]     -- 同上，更直观
> ```

#### `regex.test(s, pattern [, flags])`

判断是否存在匹配，返回 `true` / `false`。

```lua
if regex.test("CPS:12", [[\d+]]) then
    PrintLog("[Demo]包含数字", 1)
end
```

#### `regex.find(s, pattern [, init [, flags]])`

查找匹配，返回 `起始下标, 结束下标 [, 捕获组...]`；未匹配返回 `nil`。下标从 **1** 开始。

```lua
local s, e = regex.find("abc123def", [[\d+]])     -- 4, 6
```

配合 `init` 可以手动迭代所有匹配：

```lua
local text = "a1 b2 c3"
local pos = 1
while true do
    local s, e, num = regex.find(text, [[(\d)]], pos)
    if not s then break end
    PrintLog("[Demo]找到 " .. num .. " 位于 " .. s, 1)
    pos = e + 1
end
```

#### `regex.match(s, pattern [, flags])`

返回匹配到的整段文本与捕获组；未匹配返回 `nil`。

```lua
local y, m, d = regex.match("2026-10-04", [[(\d{4})-(\d{2})-(\d{2})]])
-- y="2026", m="10", d="04"
```

#### `regex.replace(s, pattern, repl [, flags])`

替换**所有**匹配，返回新字符串。`repl` 中可用 `$&` 表示整个匹配、`$1`、`$2`… 表示捕获组。

```lua
local r = regex.replace("a1b2", [[\d]], "#")        -- "a#b#"
local r2 = regex.replace("john smith", [[(\w+)\s(\w+)]], "$2 $1")   -- "smith john"
```

#### `regex.split(s, pattern [, flags])`

按正则切分，返回字符串数组（表的 `#` 即分片数）。

```lua
local parts = regex.split("a,b;c", [[[,;]]])
-- parts = {"a", "b", "c"}
```

#### flags 参数

第三个（或第四个）参数是标志字符串：

| 标志 | 含义 |
|------|------|
| `i` | 忽略大小写 |
| `m` | 多行模式（`^` / `$` 匹配每行行首行尾） |

```lua
local ok = regex.test("ABC", "abc", "i")     -- true
```

#### 常用正则符号

| 符号 | 含义 |
|------|------|
| `.` | 任意字符（默认不含换行） |
| `\d` / `\w` / `\s` | 数字 / 字母数字下划线 / 空白 |
| `*` `+` `?` | 0+ / 1+ / 0 或 1 次 |
| `{n}` `{n,m}` | 恰好 n 次 / n~m 次 |
| `[abc]` / `[^abc]` | 字符集合 / 取反 |
| `(...)` | 捕获组 |
| `\|` | 或 |
| `^` / `$` | 行首 / 行尾 |

> 注意：`.` 默认不匹配换行符；需要跨行时请使用 `[\s\S]`。

---

## 七、被移除或受限的内容

为保证脚本安全，以下内容在 GC Lua 沙箱中**已被移除**，调用会直接报错：

| 类别 | 被移除项 |
|------|----------|
| 库 | `io`（整体）、`debug`（整体） |
| package | `package.loadlib` |
| os 危险函数 | `os.execute`、`os.remove`、`os.rename`、`os.exit`、`os.tmpname`、`os.getenv` |
| 全局函数 | `dofile`、`loadfile`、`loadstring`、`load`、`collectgarbage`、`print`、`rawhook` |

**替代方案：**

- `print(...)` → 使用 `PrintLog(message, level)`（`level`：1 普通、2 警告、3 错误）
- `dofile` / `loadfile` → 使用 `require("模块名")` 导入同目录模块
- `os.time()` 等只读时间函数仍可用

```lua
-- ✗ 报错
print("hello")
io.open("a.txt")
os.execute("cmd")

-- ✓ 正确
PrintLog("[MyScript]hello", 1)
```

---

## 八、常见错误与排查

| 现象 | 原因 | 解决 |
|------|------|------|
| `attempt to call a nil value (global 'xxx')` | 调用了不存在或被移除的函数 | 检查拼写；`print` 改为 `PrintLog` |
| `attempt to index a nil value (global 'package')` | 访问了被移除的库成员 | 该库已移除（如 `io.open`） |
| 脚本没有运行 | 入口函数写成了 `local function main()` | 改为全局 `function main()` |
| 找不到入口函数 | 入口函数分散在多个文件 | 把 4 个入口函数都放进主文件 |
| `main` 未执行 / 脚本被跳过 | 多文件脚本中没有任何文件定义 `main` | 确保主文件里定义了全局 `main` |
| 变量值被重置 | 变量写在了 `render()` 里 | 变量定义移到函数外，用 `local` 声明 |
| `invalid escape sequence` | Lua 字符串里写了 `"\d"` | 改成 `"\\d"` 或 `[[\d]]` |
| 正则不生效 | 用 Lua 模式语法写成了正则 | 用 `string.*` 走 Lua 模式，用 `regex.*` 走正则 |
| 顶层代码报错“不允许自动执行” | 顶层直接调用了 API | 把逻辑放进 `initScript()` / `main()` |

---

## 九、最小可用模板

```lua
-- 依赖模块（可选）
local utils = require("utils")

local enabled = false

function initScript()
    SetScriptLoopDelay(10)
end

function main()
    if IsScriptKeyClick() then
        enabled = not enabled
    end

    if enabled then
        -- 主逻辑
    end
end

function render()
    enabled = Checkbox("启用####enable", enabled)
    Text("当前状态：" .. (enabled and "已启用" or "已停止"))
end

function saveConfig()
end
```
