'use strict';
/**
 * GC Lua API 数据表
 * 数据来源：GC 连点器 3.x 脚本系统实际注册表（Script.h -> LuaInit）+ script-docs 文档
 * 说明：以运行时“实际注册”为准，文档中已弃用/未注册的 API 会被标记为 deprecated。
 *
 * 字段说明：
 *   name        函数名
 *   category    分类：functional（功能性）/ gui（界面）/ system（系统提示）
 *   signature   显示用签名
 *   description 中文说明（支持 markdown）
 *   params      [{ name, type, optional, desc }]
 *   returns     返回值说明
 *   example     示例代码
 *   deprecated  是否已弃用
 */

const GC_FUNCTIONS = [
  // ============================ 功能性 API ============================
  {
    name: 'SetScriptLoopDelay',
    category: 'functional',
    signature: 'SetScriptLoopDelay([delay])',
    description: '设置当前脚本 `main()` 主函数的轮询延迟。不传参数时默认每 `5` 毫秒调用一次。',
    params: [{ name: 'delay', type: '整数', optional: true, desc: '轮询延迟（毫秒），即每隔多少毫秒执行一次 main 函数' }],
    returns: '无',
    example: 'SetScriptLoopDelay(5) -- 每 5 毫秒执行一次 main\nSetScriptLoopDelay()  -- 使用默认 5 毫秒'
  },
  {
    name: 'RenderMessageBox',
    category: 'system',
    signature: 'RenderMessageBox(message, title, type)',
    description: '显示一个**阻塞式**消息框，直到用户点击按钮才继续运行。返回 `1` 表示确定，`2` 表示取消。',
    params: [
      { name: 'message', type: '字符串', desc: '消息框显示的内容' },
      { name: 'title', type: '字符串', desc: '消息框标题' },
      { name: 'type', type: '整数', desc: '类型：`0` 仅“确定”，`1` “确定”+“取消”' }
    ],
    returns: '`整数`：`1` 为确定，`2` 为取消',
    example: 'if RenderMessageBox("我是弹窗消息", "我是标题", 0) == 1 then\n    RenderPopTip("不可以点取消!", 1.5, 3)\nend'
  },
  {
    name: 'RenderPopTip',
    category: 'system',
    signature: 'RenderPopTip(text, duration, type)',
    description: '显示一个**非阻塞**的弹出提示。',
    params: [
      { name: 'text', type: '字符串', desc: '提示文本内容' },
      { name: 'duration', type: '小数', desc: '持续时间（秒）' },
      { name: 'type', type: '整数', desc: '类型：`1` 消息，`2` 警告，`3` 错误' }
    ],
    returns: '无',
    example: 'RenderPopTip("我弹出来啦!", 2.4, 1)'
  },
  {
    name: 'GetTickCount',
    category: 'functional',
    signature: 'GetTickCount()',
    description: '返回从开机到现在的 Tick 计数，单位毫秒（非真实时间）。',
    params: [],
    returns: '`整数`：Tick 计数',
    example: 'local tick = GetTickCount()\nPrintLog("[MyScript]当前tick:" .. tick, 1)'
  },
  {
    name: 'RandomInt',
    category: 'functional',
    signature: 'RandomInt(min, max)',
    description: '生成一个指定范围内的随机整数。',
    params: [
      { name: 'min', type: '整数', desc: '最小值' },
      { name: 'max', type: '整数', desc: '最大值' }
    ],
    returns: '`整数`：随机数字',
    example: 'local n = RandomInt(1, 100)'
  },
  {
    name: 'RandomBool',
    category: 'functional',
    signature: 'RandomBool(probability)',
    description: '按概率返回一个随机状态（`true` / `false`）。',
    params: [{ name: 'probability', type: '浮点数', desc: '概率，取值范围 `0`~`1`' }],
    returns: '`Bool`：随机状态',
    example: 'local b = RandomBool(0.5) -- 50% 概率返回 true'
  },
  {
    name: 'GetVersion',
    category: 'functional',
    signature: 'GetVersion()',
    description: '获取当前 GC 版本号。只返回 `x.y` 中的 `y`，例如 `3.52` 返回 `52`。',
    params: [],
    returns: '`整数`：版本号',
    example: 'local ver = GetVersion()'
  },
  {
    name: 'SetClickerSettings',
    category: 'functional',
    signature: 'SetClickerSettings(section, key, value)',
    description: '设置连点器配置项的值（等价于直接修改 GC 的配置文件 `settings.ini`）。',
    params: [
      { name: 'section', type: '字符串', desc: '分组名（app 名）' },
      { name: 'key', type: '字符串', desc: '配置项名' },
      { name: 'value', type: '任意类型', desc: '要设置的值' }
    ],
    returns: '无',
    example: 'SetClickerSettings("CPS", "LCPS", 14)'
  },
  {
    name: 'GetClickerSettings',
    category: 'functional',
    signature: 'GetClickerSettings(section, key, default)',
    description: '获取连点器配置项的值（等价于直接读取 GC 的配置文件）。',
    params: [
      { name: 'section', type: '字符串', desc: '分组名（app 名）' },
      { name: 'key', type: '字符串', desc: '配置项名' },
      { name: 'default', type: '任意类型', desc: '配置项不存在时返回的默认值' }
    ],
    returns: '`字符串`：设置项内容',
    example: 'local cps = GetClickerSettings("CPS", "LCPS", "12")'
  },
  {
    name: 'PrintLog',
    category: 'functional',
    signature: 'PrintLog(message, level)',
    description: '打印日志。标准格式为 `[脚本名]内容`。',
    params: [
      { name: 'message', type: '字符串', desc: '日志消息，建议格式 `[脚本名]内容`' },
      { name: 'level', type: '整数', desc: '级别：`1` 普通，`2` 警告，`3` 错误' }
    ],
    returns: '无',
    example: 'PrintLog("[AutoFishingRod]出现未知错误", 3)'
  },
  {
    name: 'GetKeyDown',
    category: 'functional',
    signature: 'GetKeyDown(key, [humanOnly], [mode])',
    description: '检查指定按键是否被按下。',
    params: [
      { name: 'key', type: '整数', desc: '按键代码，建议使用 `KEY.*` 常量' },
      { name: 'humanOnly', type: 'Bool', optional: true, desc: '仅获取人类点击，可过滤自身模拟点击' },
      { name: 'mode', type: '整数', optional: true, desc: '获取方式：`GK.HOOK` / `GK.BOTH`（仅建议）' }
    ],
    returns: '`Bool`：是否按下',
    example: 'if GetKeyDown(KEY.SPACE, true) then\n    SetKeyUp(KEY.SPACE)\nend'
  },
  {
    name: 'GetKeyClick',
    category: 'functional',
    signature: 'GetKeyClick(key, [humanOnly], [mode])',
    description: '检查指定按键是否被点击。参数含义同 `GetKeyDown`。',
    params: [
      { name: 'key', type: '整数', desc: '按键代码' },
      { name: 'humanOnly', type: 'Bool', optional: true, desc: '仅获取人类点击' },
      { name: 'mode', type: '整数', optional: true, desc: '获取方式：`GK.HOOK` / `GK.BOTH`' }
    ],
    returns: '`Bool`：是否点击',
    example: 'if GetKeyClick(KEY.SPACE) then ... end'
  },
  {
    name: 'GetKeyUp',
    category: 'functional',
    signature: 'GetKeyUp(key, [humanOnly], [mode])',
    description: '检查指定按键是否被释放。参数含义同 `GetKeyDown`。',
    params: [
      { name: 'key', type: '整数', desc: '按键代码' },
      { name: 'humanOnly', type: 'Bool', optional: true, desc: '仅获取人类点击' },
      { name: 'mode', type: '整数', optional: true, desc: '获取方式：`GK.HOOK` / `GK.BOTH`' }
    ],
    returns: '`Bool`：是否释放',
    example: 'if GetKeyUp(KEY.SPACE) then ... end'
  },
  {
    name: 'SetKeyDown',
    category: 'functional',
    signature: 'SetKeyDown(key, [mode])',
    description: '模拟按下指定按键。',
    params: [
      { name: 'key', type: '整数', desc: '按键代码' },
      { name: 'mode', type: '整数', optional: true, desc: '点击方式：`SC.MOUSE_EVENT` / `SC.SEND_INPUT` / `SC.POST_MESSAGE`' }
    ],
    returns: '无',
    example: 'SetKeyDown(KEY.SPACE)\nSetKeyDown(KEY.SPACE, SC.SEND_INPUT) -- 基岩版需使用 SEND_INPUT'
  },
  {
    name: 'SetKeyUp',
    category: 'functional',
    signature: 'SetKeyUp(key, [mode])',
    description: '模拟释放指定按键。',
    params: [
      { name: 'key', type: '整数', desc: '按键代码' },
      { name: 'mode', type: '整数', optional: true, desc: '点击方式（同 `SetKeyDown`）' }
    ],
    returns: '无',
    example: 'SetKeyUp(KEY.SPACE)'
  },
  {
    name: 'SetKeyClick',
    category: 'functional',
    signature: 'SetKeyClick(key, [mode], [interval])',
    description: '模拟点击指定按键（按下后自动释放）。',
    params: [
      { name: 'key', type: '整数', desc: '按键代码' },
      { name: 'mode', type: '整数', optional: true, desc: '点击方式' },
      { name: 'interval', type: '整数', optional: true, desc: '按下到释放的间隔（毫秒），默认 `60`' }
    ],
    returns: '无',
    example: 'SetKeyClick(KEY.SPACE)\nSetKeyClick(KEY.SPACE, SC.SEND_INPUT, 50)'
  },
  {
    name: 'GetCursorPos',
    category: 'functional',
    signature: 'GetCursorPos()',
    description: '获取鼠标当前坐标。',
    params: [],
    returns: '`整数, 整数`：鼠标的 X、Y 坐标（像素）',
    example: 'local x, y = GetCursorPos()'
  },
  {
    name: 'SetCursorPos',
    category: 'functional',
    signature: 'SetCursorPos(x, y)',
    description: '设置鼠标位置。某个坐标传 `-1` 表示不修改该轴。',
    params: [
      { name: 'x', type: '整数', desc: 'X 坐标，`-1` 表示不设置' },
      { name: 'y', type: '整数', desc: 'Y 坐标，`-1` 表示不设置' }
    ],
    returns: '无',
    example: 'SetCursorPos(11, -1) -- 仅设置 X\nSetCursorPos(60, 20) -- 设置 X 与 Y'
  },
  {
    name: 'GetCpsRight',
    category: 'functional',
    signature: 'GetCpsRight()',
    description: '获取右键点击 CPS。',
    params: [],
    returns: '`整数`：CPS 值',
    example: 'local cps = GetCpsRight()'
  },
  {
    name: 'GetCpsLeft',
    category: 'functional',
    signature: 'GetCpsLeft()',
    description: '获取左键点击 CPS。',
    params: [],
    returns: '`整数`：CPS 值',
    example: 'local cps = GetCpsLeft()'
  },
  {
    name: 'GetHumanCpsRight',
    category: 'functional',
    signature: 'GetHumanCpsRight()',
    description: '获取真实人类右键 CPS，连点器模拟点击不会影响该值。',
    params: [],
    returns: '`整数`：CPS 值',
    example: 'local cps = GetHumanCpsRight()'
  },
  {
    name: 'GetHumanCpsLeft',
    category: 'functional',
    signature: 'GetHumanCpsLeft()',
    description: '获取真实人类左键 CPS，连点器模拟点击不会影响该值。',
    params: [],
    returns: '`整数`：CPS 值',
    example: 'local cps = GetHumanCpsLeft()'
  },
  {
    name: 'Sleep',
    category: 'functional',
    signature: 'Sleep(milliseconds)',
    description: '暂停执行指定毫秒数。为实时判断脚本是否退出，误差约为 ±3 毫秒。',
    params: [{ name: 'milliseconds', type: '整数', desc: '暂停时间（毫秒）' }],
    returns: '无',
    example: 'Sleep(100)'
  },
  {
    name: 'GetItemBarPos',
    category: 'functional',
    signature: 'GetItemBarPos()',
    description: '获取物品栏当前位置。索引从 `0` 开始（第 1 个格子返回 `0`）。',
    params: [],
    returns: '`整数`：物品栏位置（`0`~`8`）'
  },
  {
    name: 'SetItemBarPos',
    category: 'functional',
    signature: 'SetItemBarPos(index, [mode], [interval])',
    description: '设置物品栏位置。',
    params: [
      { name: 'index', type: '整数', desc: '位置（`0`~`8`）' },
      { name: 'mode', type: '整数', optional: true, desc: '点击方式' },
      { name: 'interval', type: '整数', optional: true, desc: '点击间隔（毫秒）' }
    ],
    returns: '无',
    example: 'SetItemBarPos(0) -- 切换到第 1 个格子'
  },
  {
    name: 'SetIniFile',
    category: 'functional',
    signature: 'SetIniFile(section, key, value, file)',
    description: '设置脚本自己的 INI 文件中的值。',
    params: [
      { name: 'section', type: '字符串', desc: '节点 / 应用名' },
      { name: 'key', type: '字符串', desc: '键名' },
      { name: 'value', type: '字符串 | 整数 | Bool', desc: '要写入的值' },
      { name: 'file', type: '字符串', desc: '文件名（避免使用路径与 `../`）' }
    ],
    returns: '无',
    example: 'SetIniFile("配置", "cps", 14, "MyScript.ini")'
  },
  {
    name: 'GetIniFile',
    category: 'functional',
    signature: 'GetIniFile(section, key, default, file)',
    description: '读取脚本自己的 INI 文件中的值。',
    params: [
      { name: 'section', type: '字符串', desc: '节点名' },
      { name: 'key', type: '字符串', desc: '键名' },
      { name: 'default', type: '字符串 | 整数 | Bool', desc: '未找到时返回的默认值' },
      { name: 'file', type: '字符串', desc: '文件名' }
    ],
    returns: '`字符串 | 整数 | Bool`',
    example: 'local cps = GetIniFile("配置", "cps", 12, "MyScript.ini")'
  },
  {
    name: 'SimulatedInputText',
    category: 'functional',
    signature: 'SimulatedInputText(text)',
    description: '通过剪贴板向焦点窗口输入任意文本。**属于受限 API，需要审核授权。**',
    params: [{ name: 'text', type: '字符串', desc: '要输入的内容' }],
    returns: '`Bool`：成功返回 `true`，失败返回 `false`',
    restricted: true,
    example: 'local ok = SimulatedInputText("你好世界")'
  },
  {
    name: 'ConnectNetwork',
    category: 'functional',
    signature: 'ConnectNetwork(ip, port)',
    description: '连接一个 TCP 网络服务器。**需要用户授权**，首次调用会弹窗请求网络权限。',
    params: [
      { name: 'ip', type: '字符串', desc: '服务器 IP 地址' },
      { name: 'port', type: '整数', desc: '服务器端口号' }
    ],
    returns: '`整数`：连接 ID（供其他网络 API 使用），失败返回 `-1`',
    restricted: true,
    example: 'local id = ConnectNetwork("127.0.0.1", 8080)'
  },
  {
    name: 'SendNetwork',
    category: 'functional',
    signature: 'SendNetwork(id, data)',
    description: '向已连接的服务器发送数据。**需要网络授权。**',
    params: [
      { name: 'id', type: '整数', desc: '`ConnectNetwork` 返回的连接 ID' },
      { name: 'data', type: '字符串', desc: '要发送的内容' }
    ],
    returns: '`整数`：成功 `0`，失败 `-1`',
    restricted: true
  },
  {
    name: 'RecvNetwork',
    category: 'functional',
    signature: 'RecvNetwork(id)',
    description: '接收服务器发来的数据。**需要网络授权。**',
    params: [{ name: 'id', type: '整数', desc: '连接 ID' }],
    returns: '`字符串`：收到的内容，无数据或出错返回 `""`',
    restricted: true
  },
  {
    name: 'CloseNetwork',
    category: 'functional',
    signature: 'CloseNetwork(id)',
    description: '关闭一个网络连接。**需要网络授权。**',
    params: [{ name: 'id', type: '整数', desc: '连接 ID' }],
    returns: '`整数`：成功 `1`，失败 `-1`',
    restricted: true
  },
  {
    name: 'GetScreenSize',
    category: 'functional',
    signature: 'GetScreenSize()',
    description: '获取屏幕大小，单位为像素。',
    params: [],
    returns: '`整数, 整数`：屏幕宽、高',
    example: 'local x, y = GetScreenSize()'
  },
  {
    name: 'GetClipboardText',
    category: 'functional',
    signature: 'GetClipboardText()',
    description: '获取剪贴板中的文字。**属于受限 API，需要审核授权。**',
    params: [],
    returns: '`字符串`：剪贴板文字，非文字内容返回空字符串',
    restricted: true
  },
  {
    name: 'SetClipboardText',
    category: 'functional',
    signature: 'SetClipboardText(text)',
    description: '设置剪贴板文字。**属于受限 API，需要审核授权。**',
    params: [{ name: 'text', type: '字符串', desc: '要写入剪贴板的文字' }],
    returns: '`Bool`：成功 `true`，失败 `false`',
    restricted: true
  },
  {
    name: 'IsScriptKeyDown',
    category: 'functional',
    signature: 'IsScriptKeyDown()',
    description: '当前脚本的启动按键是否被按下（在 GC 已安装脚本界面设置）。',
    params: [],
    returns: '`Bool`：是否按下',
    example: 'if IsScriptKeyDown() then ... end'
  },
  {
    name: 'IsScriptKeyClick',
    category: 'functional',
    signature: 'IsScriptKeyClick()',
    description: '当前脚本的启动按键是否被点击。',
    params: [],
    returns: '`Bool`：是否点击',
    example: 'if IsScriptKeyClick() then enable = not enable end'
  },
  {
    name: 'IsScriptKeyUp',
    category: 'functional',
    signature: 'IsScriptKeyUp()',
    description: '当前脚本的启动按键是否被释放。读取后立即重置为 `false`，仅在释放后的一次轮询返回 `true`。',
    params: [],
    returns: '`Bool`：是否释放',
    example: 'if IsScriptKeyUp() then ... end'
  },
  {
    name: 'MessageBeep',
    category: 'system',
    signature: 'MessageBeep(type)',
    description: '播放系统提示音。',
    params: [{ name: 'type', type: '整数', desc: '`1` 提示音，`2` 警告音，`3` 错误音' }],
    returns: '无',
    example: 'MessageBeep(1) -- 提示音'
  },
  {
    name: 'PlayEnableSound',
    category: 'system',
    signature: 'PlayEnableSound()',
    description: '播放连点器开启时的提示音。',
    params: [],
    returns: '无',
    example: 'PlayEnableSound()'
  },
  {
    name: 'PlayDisableSound',
    category: 'system',
    signature: 'PlayDisableSound()',
    description: '播放连点器关闭时的提示音。',
    params: [],
    returns: '无',
    example: 'PlayDisableSound()'
  },
  {
    name: 'PlayToggleSound',
    category: 'system',
    signature: 'PlayToggleSound(enable)',
    description: '根据布尔值播放开启或关闭提示音（`PlayEnableSound` / `PlayDisableSound` 的封装）。',
    params: [{ name: 'enable', type: 'Bool', desc: '`true` 播放开启音，`false` 播放关闭音' }],
    returns: '无',
    example: 'PlayToggleSound(enable)'
  },
  {
    name: 'IsOpenGui',
    category: 'functional',
    signature: 'IsOpenGui()',
    description: '玩家是否打开了游戏 GUI（箱子、商店等界面）。',
    params: [],
    returns: '`Bool`：是否打开 GUI',
    example: 'if IsOpenGui() then\n    return\nend'
  },
  {
    name: 'IsPlayerMoving',
    category: 'functional',
    signature: 'IsPlayerMoving()',
    description: '玩家是否正在移动。跳跃也算移动，按 Shift 不算。',
    params: [],
    returns: '`Bool`：是否移动',
    example: 'if IsPlayerMoving() then\n    PrintLog("[MyScript]玩家正在移动", 1)\nend'
  },
  {
    name: 'GetCurrentWindowPos',
    category: 'functional',
    signature: 'GetCurrentWindowPos()',
    description: '获取当前焦点窗口左上角在屏幕坐标系中的位置。',
    params: [],
    returns: '`整数, 整数`：X、Y 坐标',
    example: 'local x, y = GetCurrentWindowPos()'
  },
  {
    name: 'GetCurrentWindowRect',
    category: 'functional',
    signature: 'GetCurrentWindowRect()',
    description: '获取当前焦点窗口的矩形区域。',
    params: [],
    returns: '`整数, 整数, 整数, 整数`：left、top、right、bottom',
    example: 'local l, t, r, b = GetCurrentWindowRect()'
  },
  {
    name: 'GetCurrentWindowSize',
    category: 'functional',
    signature: 'GetCurrentWindowSize()',
    description: '获取当前焦点窗口的大小（像素）。',
    params: [],
    returns: '`整数, 整数`：宽度、高度',
    example: 'local w, h = GetCurrentWindowSize()'
  },
  {
    name: 'GetWindowsCenterPos',
    category: 'functional',
    signature: 'GetWindowsCenterPos()',
    description: '获取当前焦点窗口中心点在屏幕坐标系中的位置。',
    params: [],
    returns: '`整数, 整数`：中心点 X、Y 坐标',
    example: 'local cx, cy = GetWindowsCenterPos()'
  },
  {
    name: 'MoveCursor',
    category: 'functional',
    signature: 'MoveCursor(dx, dy)',
    description: '按相对偏移移动鼠标。',
    params: [
      { name: 'dx', type: '整数', desc: 'X 方向偏移量' },
      { name: 'dy', type: '整数', desc: 'Y 方向偏移量' }
    ],
    returns: '无',
    example: 'MoveCursor(-1, -5) -- 向左上移动'
  },
  {
    name: 'MoveCursorX',
    category: 'functional',
    signature: 'MoveCursorX(dx)',
    description: '仅按相对偏移移动鼠标的 X 坐标。',
    params: [{ name: 'dx', type: '整数', desc: 'X 方向偏移量' }],
    returns: '无',
    example: 'MoveCursorX(10) -- 向右移动 10 像素'
  },
  {
    name: 'MoveCursorY',
    category: 'functional',
    signature: 'MoveCursorY(dy)',
    description: '仅按相对偏移移动鼠标的 Y 坐标。',
    params: [{ name: 'dy', type: '整数', desc: 'Y 方向偏移量' }],
    returns: '无',
    example: 'MoveCursorY(-5) -- 向上移动 5 像素'
  },
  {
    name: 'GetItemBarKey',
    category: 'functional',
    signature: 'GetItemBarKey(index)',
    description: '获取物品栏指定格子（`0`~`8`）的快捷键键值。',
    params: [{ name: 'index', type: '整数', desc: '物品栏格子索引（`0`~`8`）' }],
    returns: '`整数`：对应格子的快捷键键值',
    example: 'local key = GetItemBarKey(0) -- 获取第 1 个格子的快捷键'
  },

  // ============================ 界面（GUI）API ============================
  {
    name: 'IsCloseUI',
    category: 'gui',
    signature: 'IsCloseUI()',
    description: '**该接口已被禁用**：调用会直接触发 Lua 错误（`IsCloseUI 在新版本已被禁用`），不要使用。',
    params: [],
    returns: '无（调用即报错）'
  },
  {
    name: 'Button',
    category: 'gui',
    signature: 'Button(label, width, height)',
    description: '创建一个按钮，用户点击时返回 `true`。宽度和高度必须同时提供。',
    params: [
      { name: 'label', type: '字符串', desc: '按钮文本，`####` 之后的文字会隐藏（不可与其他控件重名）' },
      { name: 'width', type: '整数', desc: '按钮宽度' },
      { name: 'height', type: '整数', desc: '按钮高度' }
    ],
    returns: '`Bool`：是否被点击',
    example: 'if Button("点我####隐藏", 120, 30) then\n    PrintLog("[MyScript]按钮被点击", 1)\nend'
  },
  {
    name: 'Checkbox',
    category: 'gui',
    signature: 'Checkbox(label, value)',
    description: '创建一个复选框，返回更新后的状态。',
    params: [
      { name: 'label', type: '字符串', desc: '复选框文本（不可与其他控件重名）' },
      { name: 'value', type: 'Bool', desc: '初始状态（需为全局变量）' }
    ],
    returns: '`Bool`：更新后的状态',
    example: 'a = Checkbox("帮我打上勾####隐藏", a)'
  },
  {
    name: 'Toggle',
    category: 'gui',
    signature: 'Toggle(label, value)',
    description: '创建一个滑动开关，返回更新后的状态。',
    params: [
      { name: 'label', type: '字符串', desc: '开关文本（不可与其他控件重名）' },
      { name: 'value', type: 'Bool', desc: '初始状态（需为全局变量）' }
    ],
    returns: '`Bool`：更新后的状态'
  },
  {
    name: 'Text',
    category: 'gui',
    signature: 'Text(text)',
    description: '在界面中显示一行文本。',
    params: [{ name: 'text', type: '字符串', desc: '要显示的文本' }],
    returns: '无',
    example: 'Text("我是文字")'
  },
  {
    name: 'InputText',
    category: 'gui',
    signature: 'InputText(label, value)',
    description: '创建一个文本输入框，返回修改后的文本。',
    params: [
      { name: 'label', type: '字符串', desc: '标签（不可与其他控件重名）' },
      { name: 'value', type: '字符串', desc: '初始文本（必须为变量）' }
    ],
    returns: '`字符串`：修改后的值'
  },
  {
    name: 'SliderInt',
    category: 'gui',
    signature: 'SliderInt(label, value, min, max, format)',
    description: '创建一个整数滑块，返回修改后的值。',
    params: [
      { name: 'label', type: '字符串', desc: '标签（不可与其他控件重名）' },
      { name: 'value', type: '整数', desc: '当前值（必须为变量）' },
      { name: 'min', type: '整数', desc: '最小值' },
      { name: 'max', type: '整数', desc: '最大值' },
      { name: 'format', type: '字符串', desc: '格式字符串，例如 `"%d"`' }
    ],
    returns: '`整数`：修改后的值',
    example: 't = SliderInt("我是文字####隐藏", t, 10, 100, "%d")'
  },
  {
    name: 'SliderFloat',
    category: 'gui',
    signature: 'SliderFloat(label, value, min, max, format)',
    description: '创建一个浮点数滑块，返回修改后的值。',
    params: [
      { name: 'label', type: '字符串', desc: '标签（不可与其他控件重名）' },
      { name: 'value', type: '浮点数', desc: '当前值（必须为变量）' },
      { name: 'min', type: '浮点数', desc: '最小值' },
      { name: 'max', type: '浮点数', desc: '最大值' },
      { name: 'format', type: '字符串', desc: '格式字符串，例如 `"%.2f"`' }
    ],
    returns: '`浮点数`：修改后的值'
  },
  {
    name: 'RangeSliderInt',
    category: 'gui',
    signature: 'RangeSliderInt(label, min, max, sliderMin, sliderMax, format)',
    description: '创建一个整数范围滑块，返回修改后的最小值与最大值（共 2 个返回值）。',
    params: [
      { name: 'label', type: '字符串', desc: '标签（不可与其他控件重名）' },
      { name: 'min', type: '整数', desc: '当前范围最小值（必须为变量）' },
      { name: 'max', type: '整数', desc: '当前范围最大值（必须为变量）' },
      { name: 'sliderMin', type: '整数', desc: '滑块允许的最小值' },
      { name: 'sliderMax', type: '整数', desc: '滑块允许的最大值' },
      { name: 'format', type: '字符串', desc: '格式字符串，例如 `"%dCPS"`' }
    ],
    returns: '`整数, 整数`：范围最小值、最大值',
    example: 'minCps, maxCps = RangeSliderInt("####范围滑块", minCps, maxCps, 1, 40, "%dCPS")'
  },
  {
    name: 'RangeSliderFloat',
    category: 'gui',
    signature: 'RangeSliderFloat(label, min, max, sliderMin, sliderMax, format)',
    description: '创建一个浮点数范围滑块，返回修改后的最小值与最大值（共 2 个返回值）。',
    params: [
      { name: 'label', type: '字符串', desc: '标签（不可与其他控件重名）' },
      { name: 'min', type: '浮点数', desc: '当前范围最小值（必须为变量）' },
      { name: 'max', type: '浮点数', desc: '当前范围最大值（必须为变量）' },
      { name: 'sliderMin', type: '浮点数', desc: '滑块允许的最小值' },
      { name: 'sliderMax', type: '浮点数', desc: '滑块允许的最大值' },
      { name: 'format', type: '字符串', desc: '格式字符串，例如 `"%.1fCPS"`' }
    ],
    returns: '`浮点数, 浮点数`：范围最小值、最大值'
  },
  {
    name: 'Combo',
    category: 'gui',
    signature: 'Combo(label, selected, items)',
    description: '创建一个下拉框，返回当前选中项的索引（从 `0` 开始）。',
    params: [
      { name: 'label', type: '字符串', desc: '标签（不可与其他控件重名）' },
      { name: 'selected', type: '整数', desc: '当前选中索引（从 `0` 开始，必须为变量）' },
      { name: 'items', type: 'table', desc: '选项字符串数组，例如 `{"选项一", "选项二"}`' }
    ],
    returns: '`整数`：当前选中索引',
    example: 'select = Combo("####下拉框", select, {"选项一", "选项二", "选项三"})'
  },
  {
    name: 'SameLine',
    category: 'gui',
    signature: 'SameLine()',
    description: '让下一个元素与上一个元素显示在同一行。',
    params: [],
    returns: '无'
  },
  {
    name: 'InputKey',
    category: 'gui',
    signature: 'InputKey(label, key)',
    description: '创建一个按键输入框，返回修改后的按键键值。',
    params: [
      { name: 'label', type: '字符串', desc: '标签' },
      { name: 'key', type: '整数', desc: '初始按键值（按键代码，必须为变量）' }
    ],
    returns: '`整数`：修改后的键值'
  },
  {
    name: 'MessageBox',
    category: 'gui',
    signature: 'MessageBox(message, title, type)',
    description: '在界面中创建一个消息框。**非阻塞**，未点击时返回 `-1`。',
    params: [
      { name: 'message', type: '字符串', desc: '消息内容' },
      { name: 'title', type: '字符串', desc: '标题' },
      { name: 'type', type: '整数', desc: '`0` 仅确定，`1` 确定 + 取消' }
    ],
    returns: '`整数`：`1` 确定，`2` 取消，`-1` 尚未点击'
  },
  {
    name: 'AttachTooltip',
    category: 'gui',
    signature: 'AttachTooltip(text, delay)',
    description: '为上一个控件附加悬停帮助文本。需紧跟在目标控件之后调用。',
    params: [
      { name: 'text', type: '字符串', desc: '帮助内容' },
      { name: 'delay', type: '小数', desc: '悬停多少秒后显示，`0` 为立即显示' }
    ],
    returns: '无',
    example: 'Text("把鼠标放在我这里")\nAttachTooltip("我是帮助文字", 1)'
  },
  {
    name: 'RadioButton',
    category: 'gui',
    signature: 'RadioButton(label, id, selectedId)',
    description: '创建一个单选按钮，返回更新后的选中 ID。',
    params: [
      { name: 'label', type: '字符串', desc: '文本标签（不可与其他控件重名）' },
      { name: 'id', type: '整数', desc: '该按钮的 ID' },
      { name: 'selectedId', type: '整数', desc: '当前选中的 ID' }
    ],
    returns: '`整数`：更新后的选中 ID'
  },
  {
    name: 'SetNextItemPos',
    category: 'gui',
    signature: 'SetNextItemPos(x, y)',
    description: '设置下一个控件出现的位置（UI 光标）。某坐标传 `-1` 表示不修改该轴。',
    params: [
      { name: 'x', type: '整数', desc: 'X 坐标，`-1` 不设置' },
      { name: 'y', type: '整数', desc: 'Y 坐标，`-1` 不设置' }
    ],
    returns: '无'
  },
  {
    name: 'GetNextItemPos',
    category: 'gui',
    signature: 'GetNextItemPos()',
    description: '获取当前 UI 光标位置。',
    params: [],
    returns: '`整数, 整数`：X、Y 坐标'
  },
  {
    name: 'SetNextItemWidth',
    category: 'gui',
    signature: 'SetNextItemWidth(width)',
    description: '设置下一个控件的宽度。',
    params: [{ name: 'width', type: '整数', desc: '宽度（像素）' }],
    returns: '无'
  },
  {
    name: 'BeginChild',
    category: 'gui',
    signature: 'BeginChild(id, width, height, [border])',
    description: '开始一个子窗口（子区域），**必须与 `EndChild` 成对使用**。',
    params: [
      { name: 'id', type: '字符串', desc: '子窗口唯一标识' },
      { name: 'width', type: '整数', desc: '宽度，`0` 为自动填满' },
      { name: 'height', type: '整数', desc: '高度，`0` 为自动填满' },
      { name: 'border', type: 'Bool', optional: true, desc: '是否显示边框，默认 `false`' }
    ],
    returns: '`Bool`：子窗口是否打开',
    pair: 'EndChild',
    example: 'if BeginChild("####区域", 600, 200, true) then\n    Text("内容")\nend\nEndChild()'
  },
  {
    name: 'EndChild',
    category: 'gui',
    signature: 'EndChild()',
    description: '结束由 `BeginChild` 创建的子窗口。',
    params: [],
    returns: '无',
    pairStart: 'BeginChild'
  },
  {
    name: 'BeginGroup',
    category: 'gui',
    signature: 'BeginGroup()',
    description: '开始一个分组，**必须与 `EndGroup` 成对使用**。',
    params: [],
    returns: '无',
    pair: 'EndGroup'
  },
  {
    name: 'EndGroup',
    category: 'gui',
    signature: 'EndGroup()',
    description: '结束由 `BeginGroup` 创建的分组。',
    params: [],
    returns: '无',
    pairStart: 'BeginGroup'
  },
  {
    name: 'SetCurentColor',
    category: 'gui',
    signature: 'SetCurentColor(r, g, b, a)',
    description: '设置当前绘制颜色。',
    params: [
      { name: 'r', type: '整数', desc: '红色分量（`0`~`255`）' },
      { name: 'g', type: '整数', desc: '绿色分量（`0`~`255`）' },
      { name: 'b', type: '整数', desc: '蓝色分量（`0`~`255`）' },
      { name: 'a', type: '整数', desc: '透明度（`0`~`255`）' }
    ],
    returns: '无',
    example: 'SetCurentColor(255, 0, 0, 255) -- 红色不透明'
  },
  {
    name: 'SetDrawOverAll',
    category: 'gui',
    signature: 'SetDrawOverAll(overall)',
    description: '设置绘制内容是否绘制在最顶层。',
    params: [{ name: 'overall', type: 'Bool', desc: '`true` 绘制在最上层，`false` 自动调整层级' }],
    returns: '无'
  },
  {
    name: 'DrawSpiltLine',
    category: 'gui',
    signature: 'DrawSpiltLine([length])',
    description: '绘制一条水平分隔线。',
    params: [{ name: 'length', type: '整数', optional: true, desc: '分隔线长度（像素），默认为 `600`' }],
    returns: '无',
    example: 'DrawSpiltLine()      -- 默认长度 600\nDrawSpiltLine(300)   -- 指定长度 300'
  },
  {
    name: 'DrawLine',
    category: 'gui',
    signature: 'DrawLine(x1, y1, x2, y2, thickness)',
    description: '绘制一条线段。',
    params: [
      { name: 'x1', type: '整数', desc: '起点 X' },
      { name: 'y1', type: '整数', desc: '起点 Y' },
      { name: 'x2', type: '整数', desc: '终点 X' },
      { name: 'y2', type: '整数', desc: '终点 Y' },
      { name: 'thickness', type: '整数', desc: '线宽' }
    ],
    returns: '无'
  },
  {
    name: 'DrawRect',
    category: 'gui',
    signature: 'DrawRect(x1, y1, x2, y2, rounding)',
    description: '绘制一个矩形边框。',
    params: [
      { name: 'x1', type: '整数', desc: '左上角 X' },
      { name: 'y1', type: '整数', desc: '左上角 Y' },
      { name: 'x2', type: '整数', desc: '右下角 X' },
      { name: 'y2', type: '整数', desc: '右下角 Y' },
      { name: 'rounding', type: '小数', desc: '圆角半径' }
    ],
    returns: '无'
  },
  {
    name: 'DrawRectFilled',
    category: 'gui',
    signature: 'DrawRectFilled(x1, y1, x2, y2, rounding)',
    description: '绘制一个填充矩形。',
    params: [
      { name: 'x1', type: '整数', desc: '左上角 X' },
      { name: 'y1', type: '整数', desc: '左上角 Y' },
      { name: 'x2', type: '整数', desc: '右下角 X' },
      { name: 'y2', type: '整数', desc: '右下角 Y' },
      { name: 'rounding', type: '小数', desc: '圆角半径' }
    ],
    returns: '无'
  },
  {
    name: 'DrawCircle',
    category: 'gui',
    signature: 'DrawCircle(x, y, radius, thickness)',
    description: '绘制一个圆形边框。',
    params: [
      { name: 'x', type: '整数', desc: '圆心 X' },
      { name: 'y', type: '整数', desc: '圆心 Y' },
      { name: 'radius', type: '整数', desc: '半径' },
      { name: 'thickness', type: '小数', desc: '线宽' }
    ],
    returns: '无'
  },
  {
    name: 'DrawCircleFilled',
    category: 'gui',
    signature: 'DrawCircleFilled(x, y, radius, thickness)',
    description: '绘制一个填充圆。',
    params: [
      { name: 'x', type: '整数', desc: '圆心 X' },
      { name: 'y', type: '整数', desc: '圆心 Y' },
      { name: 'radius', type: '整数', desc: '半径' },
      { name: 'thickness', type: '小数', desc: '必须传入（当前版本未实际使用该参数）' }
    ],
    returns: '无',
    example: 'DrawCircleFilled(100, 100, 50, 0)'
  },
  {
    name: 'DrawText',
    category: 'gui',
    signature: 'DrawText(x, y, text)',
    description: '在指定位置绘制文本。',
    params: [
      { name: 'x', type: '整数', desc: '文本左上角 X' },
      { name: 'y', type: '整数', desc: '文本左上角 Y' },
      { name: 'text', type: '字符串', desc: '要绘制的文本' }
    ],
    returns: '无'
  },
  {
    name: 'DrawImage',
    category: 'gui',
    signature: 'DrawImage(x, y, width, height, path)',
    description: '在指定位置绘制图片。脚本只能读取自身文件夹（解压后的 `temp` 目录）内的图片。',
    params: [
      { name: 'x', type: '整数', desc: '图片左上角 X' },
      { name: 'y', type: '整数', desc: '图片左上角 Y' },
      { name: 'width', type: '整数', desc: '图片宽度' },
      { name: 'height', type: '整数', desc: '图片高度' },
      { name: 'path', type: '字符串', desc: '图片文件路径（相对脚本自身文件夹）' }
    ],
    returns: '无',
    example: 'DrawImage(10, 10, 100, 50, "icon.png")'
  },

  // ============================ minecraft.chat 命名空间 ============================
  {
    name: 'minecraft.chat.OpenReader',
    category: 'functional',
    signature: 'minecraft.chat.OpenReader()',
    description: '打开当前脚本的聊天读取器（通常在 `initScript` 中调用）。',
    params: [],
    returns: '无',
    example: 'function initScript()\n    minecraft.chat.OpenReader()\nend'
  },
  {
    name: 'minecraft.chat.GetNewMsg',
    category: 'functional',
    signature: 'minecraft.chat.GetNewMsg()',
    description: '获取一条新的聊天消息，没有新消息时返回 `nil`。',
    params: [],
    returns: '`字符串` 或 `nil`：新的聊天消息',
    example: 'local msg = minecraft.chat.GetNewMsg()\nwhile msg ~= nil do\n    PrintLog("[MyScript] " .. msg, 1)\n    msg = minecraft.chat.GetNewMsg()\nend'
  }
];

/** 已弃用 / 已移除的 API：名称 -> { reason, replacement } */
const GC_DEPRECATED = {
  Helpmarker: { reason: '自 3.27 起弃用，并从后续版本代码中移除，调用会报错 `attempt to call a nil value`。', replacement: 'AttachTooltip' },
  PopTip: { reason: 'GUI 版 `PopTip` 未在脚本系统注册，调用会报错。请使用 `RenderPopTip` 或 `MessageBox`。', replacement: 'RenderPopTip' },
  IsCloseUI: { reason: '`IsCloseUI` 在新版本已被禁用，调用会直接触发 Lua 错误。', replacement: null },
  print: { reason: '`print` 已被脚本沙箱移除，调用会报错。', replacement: 'PrintLog' },
  dofile: { reason: '`dofile` 已被脚本沙箱移除，调用会报错。', replacement: null },
  loadfile: { reason: '`loadfile` 已被脚本沙箱移除，调用会报错。', replacement: null },
  loadstring: { reason: '`loadstring` 已被脚本沙箱移除，调用会报错。', replacement: null },
  load: { reason: '`load` 已被脚本沙箱移除，调用会报错。', replacement: null },
  collectgarbage: { reason: '`collectgarbage` 已被脚本沙箱移除，调用会报错。', replacement: null },
  rawhook: { reason: '`rawhook` 已被脚本沙箱移除，调用会报错。', replacement: null },
  io: { reason: '`io` 库已被脚本沙箱整体移除，无法使用。', replacement: null },
  package: { reason: '`package` 库已被脚本沙箱整体移除，`require` 不可用。', replacement: null },
  debug: { reason: '`debug` 库已被脚本沙箱整体移除，无法使用。', replacement: null },
  'os.execute': { reason: '`os.execute` 已被沙箱移除，无法使用。', replacement: null },
  'os.remove': { reason: '`os.remove` 已被沙箱移除，无法使用。', replacement: null },
  'os.rename': { reason: '`os.rename` 已被沙箱移除，无法使用。', replacement: null },
  'os.exit': { reason: '`os.exit` 已被沙箱移除，无法使用。', replacement: null },
  'os.tmpname': { reason: '`os.tmpname` 已被沙箱移除，无法使用。', replacement: null },
  'os.getenv': { reason: '`os.getenv` 已被沙箱移除，无法使用。', replacement: null }
};

/** 受限 API：需要审核 / 用户授权后才能使用（由各 API 的 `restricted` 标记派生，保证一致） */
const GC_RESTRICTED = GC_FUNCTIONS.filter(f => f.restricted).map(f => f.name);

/** KEY 常量表：名称 -> 键值 */
const KEY_CONSTANTS = {
  A: 65, B: 66, C: 67, D: 68, E: 69, F: 70, G: 71, H: 72, I: 73, J: 74, K: 75, L: 76, M: 77,
  N: 78, O: 79, P: 80, Q: 81, R: 82, S: 83, T: 84, U: 85, V: 86, W: 87, X: 88, Y: 89, Z: 90,
  '0': 48, '1': 49, '2': 50, '3': 51, '4': 52, '5': 53, '6': 54, '7': 55, '8': 56, '9': 57,
  VK_RETURN: 0x0d, VK_ESCAPE: 0x1b, VK_SPACE: 0x20, VK_BACK: 0x08, VK_TAB: 0x09, VK_CAPITAL: 0x14,
  VK_SHIFT: 0x10, VK_CONTROL: 0x11, VK_MENU: 0x12, VK_LSHIFT: 0xa0, VK_RSHIFT: 0xa1,
  VK_LCONTROL: 0xa2, VK_RCONTROL: 0xa3, VK_LMENU: 0xa4, VK_RMENU: 0xa5, VK_LWIN: 0x5b, VK_RWIN: 0x5c,
  VK_LEFT: 0x25, VK_UP: 0x26, VK_RIGHT: 0x27, VK_DOWN: 0x28,
  VK_INSERT: 0x2d, VK_DELETE: 0x2e, VK_HOME: 0x24, VK_END: 0x23, VK_PRIOR: 0x21, VK_NEXT: 0x22,
  VK_OEM_PLUS: 0xbb, VK_OEM_MINUS: 0xbd, VK_OEM_PERIOD: 0xbe, VK_OEM_COMMA: 0xbc,
  VK_OEM_1: 0xba, VK_OEM_2: 0xbf, VK_OEM_3: 0xc0, VK_OEM_4: 0xdb, VK_OEM_5: 0xdc, VK_OEM_6: 0xdd, VK_OEM_7: 0xde
};

/** SC / GK 常量表：名称 -> { value, desc } */
const SC_CONSTANTS = {
  MOUSE_EVENT: { value: 0, desc: '使用 `mouse_event` / `keybd_event` 模拟系统点击消息' },
  SEND_INPUT: { value: 1, desc: '使用 `SendInput` 模拟底层点击（支持最新基岩版）' },
  POST_MESSAGE: { value: 2, desc: '使用 `PostMessage` 向窗口发送点击消息' }
};

const GK_CONSTANTS = {
  HOOK: { value: 1, desc: '使用全局系统钩子获取按键状态' },
  BOTH: { value: 3, desc: '钩子与系统 API 都用，由 GC 自动选择' }
};

/**
 * Lua 标准库中在 GC 脚本沙箱内“仍然可用”的部分。
 * 注意：io / package / debug 被整体移除；print / dofile / loadfile / loadstring / load /
 * collectgarbage / require 等被移除；os 仅保留部分函数。
 */
const LUA_STDLIB = {
  globals: ['assert', 'error', 'ipairs', 'pairs', 'next', 'pcall', 'xpcall', 'select', 'tonumber', 'tostring', 'type', 'unpack', 'setmetatable', 'getmetatable', 'rawget', 'rawset', 'rawequal', 'rawlen', '_G', '_VERSION'],
  libraries: {
    string: ['byte', 'char', 'dump', 'find', 'format', 'gmatch', 'gsub', 'len', 'lower', 'match', 'rep', 'reverse', 'sub', 'upper'],
    table: ['concat', 'insert', 'maxn', 'remove', 'sort', 'unpack', 'pack'],
    math: ['abs', 'ceil', 'cos', 'deg', 'exp', 'floor', 'fmod', 'huge', 'log', 'max', 'min', 'modf', 'pi', 'pow', 'rad', 'random', 'randomseed', 'sin', 'sqrt', 'tan'],
    os: ['clock', 'date', 'difftime', 'time'],
    coroutine: ['create', 'resume', 'running', 'status', 'wrap', 'yield']
  }
};

/** 脚本生命周期函数说明 */
const LIFECYCLE = {
  initScript: '脚本初始化函数，在脚本加载后调用一次。适合做一次性设置（如 `SetScriptLoopDelay`、`minecraft.chat.OpenReader`）。',
  main: '脚本主循环函数，按 `SetScriptLoopDelay` 设定的间隔反复调用。**注意：不能使用 UI 系列 API。**',
  render: '脚本设置界面的渲染函数，由 GC 的 UI 线程调用。**注意：只能使用 UI 系列 API（Button / Checkbox / Slider / Draw* 等），不能调用功能性 API。**',
  saveConfig: '保存配置时调用，用于持久化脚本设置。**不能使用 UI 系列 API。**'
};

const GC_API_NAMES = GC_FUNCTIONS.map(f => f.name);

module.exports = {
  GC_FUNCTIONS,
  GC_API_NAMES,
  GC_DEPRECATED,
  GC_RESTRICTED,
  KEY_CONSTANTS,
  SC_CONSTANTS,
  GK_CONSTANTS,
  LUA_STDLIB,
  LIFECYCLE
};