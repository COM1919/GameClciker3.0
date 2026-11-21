#pragma once
#include "Definitons.h"
#include "ScriptAPI.h"
#include <memory> // make_shared/shared_ptr（StartScript 线程句柄生命周期）

static int LuaErrorHandler(lua_State* L);
lua_State* LuaInit(const string& scriptFile);
HANDLE StartScript(const string& file);
DWORD WINAPI ScriptKeyControlThread(LPVOID lpParam);
static int panic_handler(lua_State* L);
bool RenderScriptGui(const string& file);
int SaveScriptConfig(const string& file);
int LoadScript(lua_State* L, const string& file);
void RestoreInstalledScripts();
void CloseScriptThread(const string& filename, ScriptInfo& info);
int CalledScriptFunction(lua_State* L, const string& func);
inline bool InitLuaState(const string& file, LuaState& lua);
DWORD WINAPI ScriptAutoStartThread(LPVOID lpParam);
void ReportScriptError(const string& file, const string& message);
void RequestScriptUninstall(const string& file, const string& reason);
bool UninstallScript(const string& file, const string& reason);
void ProcessScriptUninstallQueue();
void SaveInstalledScriptList();   // 定义在 ConfigParser.h：卸载脚本后需要立即把已安装列表落盘
void RefushScriptList();          // 定义在 HelperCode.h：重新扫描并解压脚本目录
void StoreScriptThreadHandle(const string& file, HANDLE hThread);   // 把脚本线程句柄记进"已安装"列表
bool UpdateScriptFromShop(const ScriptInfo& script);                // 定义在 HelperCode.h：下载/复制新版本脚本文件
bool InstallScriptByName(const string& name, int key = 0, bool IsSuspend = false);
bool UpdateScriptKeepInstallState(const ScriptInfo& script);

class SpaceKeyCodeDefine
{
public:
    unordered_map<string, int> KeyMap;
    SpaceKeyCodeDefine()
    {
        // 控制键
        KeyMap["VK_RETURN"] = 0x0D;      // Enter
        KeyMap["VK_ESCAPE"] = 0x1B;      // Esc
        KeyMap["VK_SPACE"] = 0x20;       // 空格
        KeyMap["VK_BACK"] = 0x08;        // Backspace
        KeyMap["VK_TAB"] = 0x09;         // Tab
        KeyMap["VK_CAPITAL"] = 0x14;     // Caps Lock

        // 修饰键
        KeyMap["VK_SHIFT"] = 0x10;
        KeyMap["VK_CONTROL"] = 0x11;     // Ctrl
        KeyMap["VK_MENU"] = 0x12;        // Alt
        KeyMap["VK_LSHIFT"] = 0xA0;      // 左Shift
        KeyMap["VK_RSHIFT"] = 0xA1;      // 右Shift
        KeyMap["VK_LCONTROL"] = 0xA2;    // 左Ctrl
        KeyMap["VK_RCONTROL"] = 0xA3;    // 右Ctrl
        KeyMap["VK_LMENU"] = 0xA4;       // 左Alt
        KeyMap["VK_RMENU"] = 0xA5;       // 右Alt
        KeyMap["VK_LWIN"] = 0x5B;        // 左Windows
        KeyMap["VK_RWIN"] = 0x5C;        // 右Windows

        // 方向键
        KeyMap["VK_LEFT"] = 0x25;        // ←
        KeyMap["VK_UP"] = 0x26;          // ↑
        KeyMap["VK_RIGHT"] = 0x27;       // →
        KeyMap["VK_DOWN"] = 0x28;        // ↓

        // 编辑键
        KeyMap["VK_INSERT"] = 0x2D;      // Insert
        KeyMap["VK_DELETE"] = 0x2E;      // Delete
        KeyMap["VK_HOME"] = 0x24;        // Home
        KeyMap["VK_END"] = 0x23;         // End
        KeyMap["VK_PRIOR"] = 0x21;       // Page Up
        KeyMap["VK_NEXT"] = 0x22;        // Page Down

        // 符号键
        KeyMap["VK_OEM_PLUS"] = 0xBB;    // =+
        KeyMap["VK_OEM_MINUS"] = 0xBD;   // -_
        KeyMap["VK_OEM_PERIOD"] = 0xBE;  // .>
        KeyMap["VK_OEM_COMMA"] = 0xBC;   // ,<
        KeyMap["VK_OEM_1"] = 0xBA;       // ;:
        KeyMap["VK_OEM_2"] = 0xBF;       // /?
        KeyMap["VK_OEM_3"] = 0xC0;       // `~
        KeyMap["VK_OEM_4"] = 0xDB;       // [{
        KeyMap["VK_OEM_5"] = 0xDC;       // \|
        KeyMap["VK_OEM_6"] = 0xDD;       // ]}
        KeyMap["VK_OEM_7"] = 0xDE;       // '"
    }
};

lua_State* LuaInit(const string& scriptFile)
{
    lua_State* L = luaL_newstate();

    if (L == NULL)
    {
        PrintLog("Lua初始化失败", PL_ERROR);
        IsGCExit = true;
        return L;
    }

    // 设置Lua的错误处理函数
    lua_atpanic(L, panic_handler);

    {
        lua_newtable(L);
        lua_pushcfunction(L, ScriptAPI::PrintLog);           lua_setglobal(L, "PrintLog");
        lua_pushcfunction(L, ScriptAPI::GetVersion);         lua_setglobal(L, "GetVersion");
        lua_pushcfunction(L, ScriptAPI::GetKeyDown);         lua_setglobal(L, "GetKeyDown");
        lua_pushcfunction(L, ScriptAPI::GetKeyUp);           lua_setglobal(L, "GetKeyUp");
        lua_pushcfunction(L, ScriptAPI::GetKeyClick);        lua_setglobal(L, "GetKeyClick");
        lua_pushcfunction(L, ScriptAPI::SetKeyDown);         lua_setglobal(L, "SetKeyDown");
        lua_pushcfunction(L, ScriptAPI::SetKeyUp);           lua_setglobal(L, "SetKeyUp");
        lua_pushcfunction(L, ScriptAPI::SetKeyClick);        lua_setglobal(L, "SetKeyClick");
        lua_pushcfunction(L, ScriptAPI::GetCursorPos);      lua_setglobal(L, "GetCursorPos");
        lua_pushcfunction(L, ScriptAPI::SetCursorPos);      lua_setglobal(L, "SetCursorPos");
        lua_pushcfunction(L, ScriptAPI::GetCpsRight);        lua_setglobal(L, "GetCpsRight");
        lua_pushcfunction(L, ScriptAPI::GetCpsLeft);         lua_setglobal(L, "GetCpsLeft");
        lua_pushcfunction(L, ScriptAPI::GetHumanCpsLeft);    lua_setglobal(L, "GetHumanCpsLeft");
        lua_pushcfunction(L, ScriptAPI::GetHumanCpsRight);   lua_setglobal(L, "GetHumanCpsRight");
        lua_pushcfunction(L, ScriptAPI::Sleep);              lua_setglobal(L, "Sleep");
        lua_pushcfunction(L, ScriptAPI::GetItemBarPos);      lua_setglobal(L, "GetItemBarPos");
        lua_pushcfunction(L, ScriptAPI::SetIniFile);         lua_setglobal(L, "SetIniFile");
        lua_pushcfunction(L, ScriptAPI::GetIniFile);         lua_setglobal(L, "GetIniFile");
        lua_pushcfunction(L, ScriptAPI::SetItemBarPos);      lua_setglobal(L, "SetItemBarPos");
        lua_pushcfunction(L, ScriptAPI::SetClickerSettings); lua_setglobal(L, "SetClickerSettings");
        lua_pushcfunction(L, ScriptAPI::GetClickerSettings); lua_setglobal(L, "GetClickerSettings");
        lua_pushcfunction(L, ScriptAPI::RandomBool);         lua_setglobal(L, "RandomBool");
        lua_pushcfunction(L, ScriptAPI::RandomInt);          lua_setglobal(L, "RandomInt");
        lua_pushcfunction(L, ScriptAPI::GetTickCount);       lua_setglobal(L, "GetTickCount");
        lua_pushcfunction(L, ScriptAPI::SimulatedInputText); lua_setglobal(L, "SimulatedInputText");
        lua_pushcfunction(L, ScriptAPI::GetScreenSize);      lua_setglobal(L, "GetScreenSize");
        lua_pushcfunction(L, ScriptAPI::ConnectNetwork);    lua_setglobal(L, "ConnectNetwork");
        lua_pushcfunction(L, ScriptAPI::RecvNetwork);       lua_setglobal(L, "RecvNetwork");
        lua_pushcfunction(L, ScriptAPI::SendNetwork);       lua_setglobal(L, "SendNetwork");
        lua_pushcfunction(L, ScriptAPI::CloseNetwork);      lua_setglobal(L, "CloseNetwork");
        lua_pushcfunction(L, ScriptAPI::GetClipboardText);   lua_setglobal(L, "GetClipboardText");
        lua_pushcfunction(L, ScriptAPI::SetClipboardText);   lua_setglobal(L, "SetClipboardText");
        lua_pushcfunction(L, ScriptAPI::RenderMessageBox);   lua_setglobal(L, "RenderMessageBox");
        lua_pushcfunction(L, ScriptAPI::RenderPopTip);       lua_setglobal(L, "RenderPopTip");
        lua_pushcfunction(L, ScriptAPI::IsScriptKeyDown);    lua_setglobal(L, "IsScriptKeyDown");
        lua_pushcfunction(L, ScriptAPI::IsScriptKeyClick);   lua_setglobal(L, "IsScriptKeyClick");
        lua_pushcfunction(L, ScriptAPI::IsScriptKeyUp);      lua_setglobal(L, "IsScriptKeyUp");
        lua_pushcfunction(L, ScriptAPI::MessageBeep);        lua_setglobal(L, "MessageBeep");
        lua_pushcfunction(L, ScriptAPI::SetScriptLoopDelay); lua_setglobal(L, "SetScriptLoopDelay");
        lua_pushcfunction(L, ScriptAPI::IsOpenGui);          lua_setglobal(L, "IsOpenGui");
        lua_pushcfunction(L, ScriptAPI::IsPlayerMoving);     lua_setglobal(L, "IsPlayerMoving");
        lua_pushcfunction(L, ScriptAPI::PlayEnableSound);    lua_setglobal(L, "PlayEnableSound");
        lua_pushcfunction(L, ScriptAPI::PlayDisableSound);   lua_setglobal(L, "PlayDisableSound");
        lua_pushcfunction(L, ScriptAPI::PlayToggleSound);    lua_setglobal(L, "PlayToggleSound");
        lua_pushcfunction(L, ScriptAPI::GetCurrentWindowPos);lua_setglobal(L, "GetCurrentWindowPos");
        lua_pushcfunction(L, ScriptAPI::GetCurrentWindowRect);lua_setglobal(L, "GetCurrentWindowRect");
        lua_pushcfunction(L, ScriptAPI::GetCurrentWindowSize);lua_setglobal(L, "GetCurrentWindowSize");
        lua_pushcfunction(L, ScriptAPI::GetWindowsCenterPos);lua_setglobal(L, "GetWindowsCenterPos");
        lua_pushcfunction(L, ScriptAPI::MoveCursor);         lua_setglobal(L, "MoveCursor");
        lua_pushcfunction(L, ScriptAPI::MoveCursorX);        lua_setglobal(L, "MoveCursorX");
        lua_pushcfunction(L, ScriptAPI::MoveCursorY);        lua_setglobal(L, "MoveCursorY");
        lua_pushcfunction(L, ScriptAPI::GetItemBarKey);      lua_setglobal(L, "GetItemBarKey");
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, ScriptAPI::MinecraftChatOpenReader);
        lua_setfield(L, -2, "OpenReader");
        lua_pushcfunction(L, ScriptAPI::MinecraftChatGetNewMsg);
        lua_setfield(L, -2, "GetNewMsg");
        lua_setfield(L, -2, "chat");
        lua_setglobal(L, "minecraft");
        
        //=================================================GUI的API=================================================
        lua_pushcfunction(L, ScriptAPI::IsCloseUI);          lua_setglobal(L, "IsCloseUI");
        lua_pushcfunction(L, ScriptAPI::Button);             lua_setglobal(L, "Button");
        lua_pushcfunction(L, ScriptAPI::Checkbox);           lua_setglobal(L, "Checkbox");
        lua_pushcfunction(L, ScriptAPI::Toggle);             lua_setglobal(L, "Toggle");
        lua_pushcfunction(L, ScriptAPI::Text);               lua_setglobal(L, "Text");
        lua_pushcfunction(L, ScriptAPI::InputText);          lua_setglobal(L, "InputText");
        lua_pushcfunction(L, ScriptAPI::SliderInt);          lua_setglobal(L, "SliderInt");
        lua_pushcfunction(L, ScriptAPI::SliderFloat);        lua_setglobal(L, "SliderFloat");
        lua_pushcfunction(L, ScriptAPI::RangeSliderInt);     lua_setglobal(L, "RangeSliderInt");
        lua_pushcfunction(L, ScriptAPI::RangeSliderFloat);   lua_setglobal(L, "RangeSliderFloat");
        lua_pushcfunction(L, ScriptAPI::Combo);              lua_setglobal(L, "Combo");
        lua_pushcfunction(L, ScriptAPI::SameLine);           lua_setglobal(L, "SameLine");
        lua_pushcfunction(L, ScriptAPI::InputKey);           lua_setglobal(L, "InputKey");
        lua_pushcfunction(L, ScriptAPI::MessageBox);         lua_setglobal(L, "MessageBox");
        lua_pushcfunction(L, ScriptAPI::AttachTooltip);      lua_setglobal(L, "AttachTooltip");
        lua_pushcfunction(L, ScriptAPI::RadioButton);        lua_setglobal(L, "RadioButton");
        lua_pushcfunction(L, ScriptAPI::GetNextItemPos);     lua_setglobal(L, "GetNextItemPos");
        lua_pushcfunction(L, ScriptAPI::SetNextItemPos);     lua_setglobal(L, "SetNextItemPos");
        lua_pushcfunction(L, ScriptAPI::GetScreenSize);      lua_setglobal(L, "GetScreenSize");
        lua_pushcfunction(L, ScriptAPI::DrawSpiltLine);      lua_setglobal(L, "DrawSpiltLine");
        lua_pushcfunction(L, ScriptAPI::SetNextItemWidth);   lua_setglobal(L, "SetNextItemWidth");
        lua_pushcfunction(L, ScriptAPI::BeginChild);         lua_setglobal(L, "BeginChild");
        lua_pushcfunction(L, ScriptAPI::EndChild);           lua_setglobal(L, "EndChild");
        lua_pushcfunction(L, ScriptAPI::BeginGroup);         lua_setglobal(L, "BeginGroup");
        lua_pushcfunction(L, ScriptAPI::EndGroup);           lua_setglobal(L, "EndGroup");
        lua_pushcfunction(L, ScriptAPI::SetCurentColor);     lua_setglobal(L, "SetCurentColor");
        lua_pushcfunction(L, ScriptAPI::SetDrawOverAll);     lua_setglobal(L, "SetDrawOverAll");
        lua_pushcfunction(L, ScriptAPI::DrawLine);           lua_setglobal(L, "DrawLine");
        lua_pushcfunction(L, ScriptAPI::DrawRect);           lua_setglobal(L, "DrawRect");
        lua_pushcfunction(L, ScriptAPI::DrawRectFilled);     lua_setglobal(L, "DrawRectFilled");
        lua_pushcfunction(L, ScriptAPI::DrawCircle);         lua_setglobal(L, "DrawCircle");
        lua_pushcfunction(L, ScriptAPI::DrawCircleFilled);   lua_setglobal(L, "DrawCircleFilled");
        lua_pushcfunction(L, ScriptAPI::DrawImage);          lua_setglobal(L, "DrawImage");
        lua_pushcfunction(L, ScriptAPI::DrawText);           lua_setglobal(L, "DrawText");
    }
    {
        lua_newtable(L);
        // 添加字母
        for (char c = 'A'; c <= 'Z'; c++)
        {
            lua_pushinteger(L, c);
            char str[2] = { c, '\0' }; // 字母转化为字符串
            lua_setfield(L, -2, str);
        }

        // 添加数字
        for (int i = 0; i <= 9; i++)
        {
            lua_pushinteger(L, i);
            char str[2];
            str[0] = '0' + i;
            str[1] = '\0';
            lua_setfield(L, -2, str);
        }

        // 添加特殊按键
        SpaceKeyCodeDefine SpaceKeyMap;
        for (const auto& it : SpaceKeyMap.KeyMap)
        {
            lua_pushinteger(L, it.second);
            lua_setfield(L, -2, it.first.c_str());
        }
        lua_setglobal(L, "KEY"); // 添加到KEY中，这样可以通过KEY.A使用

        lua_newtable(L);
        lua_pushinteger(L, SC_MOUSE_EVENT);
        lua_setfield(L, -2, "MOUSE_EVENT");
        lua_pushinteger(L, SC_SEND_INPUT);
        lua_setfield(L, -2, "SEND_INPUT");
        lua_pushinteger(L, SC_POST_MESSAGE);
        lua_setfield(L, -2, "POST_MESSAGE");
        lua_setglobal(L, "SC");

        lua_newtable(L);
        lua_pushinteger(L, GK_HOOK);
        lua_setfield(L, -2, "HOOK");
        lua_pushinteger(L, GK_BOTH);
        lua_setfield(L, -2, "BOTH");
        lua_setglobal(L, "GK");

        lua_newtable(L);
        lua_pushcfunction(L, ScriptAPI::RegexTest);    lua_setfield(L, -2, "test");
        lua_pushcfunction(L, ScriptAPI::RegexFind);    lua_setfield(L, -2, "find");
        lua_pushcfunction(L, ScriptAPI::RegexMatch);   lua_setfield(L, -2, "match");
        lua_pushcfunction(L, ScriptAPI::RegexReplace); lua_setfield(L, -2, "replace");
        lua_pushcfunction(L, ScriptAPI::RegexSplit);   lua_setfield(L, -2, "split");
        lua_setglobal(L, "regex");
    }
    {
        // 加载所有库
        luaL_openlibs(L);

        // 完全移除io库
        lua_pushnil(L);
        lua_setglobal(L, "io");

        // Restricted package: keep require() but confine loading to the script folder.
        {
            string scriptDir = ".";
            size_t slashPos = scriptFile.find_last_of("\\/");
            if (slashPos != string::npos)
                scriptDir = scriptFile.substr(0, slashPos);

            lua_getglobal(L, "package");
            if (!lua_isnil(L, -1))
            {
                string luaPath = scriptDir + "\\?.lua";
                lua_pushstring(L, luaPath.c_str());
                lua_setfield(L, -2, "path");     // search only the script folder
                lua_pushstring(L, "");
                lua_setfield(L, -2, "cpath");    // no C modules
                lua_pushnil(L);
                lua_setfield(L, -2, "loadlib");  // no native library loading

                // Keep only the preload loader and the Lua file loader;
                // drop the C loaders so cpath cannot be re-enabled later.
                const char* loaderFields[2] = { "loaders", "searchers" };
                for (int i = 0; i < 2; i++)
                {
                    lua_getfield(L, -1, loaderFields[i]);
                    if (lua_istable(L, -1))
                    {
                        lua_newtable(L);
                        lua_rawgeti(L, -2, 1);
                        lua_rawseti(L, -2, 1);
                        lua_rawgeti(L, -2, 2);
                        lua_rawseti(L, -2, 2);
                        lua_setfield(L, -3, loaderFields[i]);
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }


        // 移除os库的危险函数
        lua_getglobal(L, "os");
        if (!lua_isnil(L, -1))
        {
            lua_pushnil(L); lua_setfield(L, -2, "execute");
            lua_pushnil(L); lua_setfield(L, -2, "remove");
            lua_pushnil(L); lua_setfield(L, -2, "rename");
            lua_pushnil(L); lua_setfield(L, -2, "exit");
            lua_pushnil(L); lua_setfield(L, -2, "tmpname");
            lua_pushnil(L); lua_setfield(L, -2, "getenv");
        }
        lua_pop(L, 1);

        // 移除危险的全局函数
        lua_pushnil(L); lua_setglobal(L, "dofile");
        lua_pushnil(L); lua_setglobal(L, "loadfile");
        lua_pushnil(L); lua_setglobal(L, "loadstring");
        lua_pushnil(L); lua_setglobal(L, "load");
        lua_pushnil(L); lua_setglobal(L, "collectgarbage");
        lua_pushnil(L); lua_setglobal(L, "print");
        lua_pushnil(L); lua_setglobal(L, "rawhook");

        // 移除debug库（全部移除）
        lua_pushnil(L);
        lua_setglobal(L, "debug");
    }

    return L;
}

// Lua 错误处理函数：必须作为 lua_pcall 的错误处理函数（msgh）传入，
// 这样在错误栈展开前调用 luaL_traceback，才能取到完整的调用堆栈。
static int LuaErrorHandler(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    if (msg == NULL)
    {
        // 错误对象不是字符串（例如 table），尝试转换为可读文本
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
            return 1;
        msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);
    return 1;
}

// 记录一次 Lua 脚本报错（写入报错历史，供"报错记录"页面显示）
void ReportScriptError(const string& file, const string& message)
{
    lock_guard lock(ScriptData::ErrorRecordsMtx);
    ScriptErrorRecord record;
    record.file = StringToUTF8(file);
    record.message = StringToUTF8(message);
    record.time = StringToUTF8(FormatTime(time(0), true));
    ScriptData::ErrorRecords.push_back(record);
    if (ScriptData::ErrorRecords.size() > MAX_SCRIPT_ERROR_RECORDS)
        ScriptData::ErrorRecords.erase(ScriptData::ErrorRecords.begin());
}

void RestoreInstalledScripts()
{
    // 恢复前 Installed 必为空，这里打印数量会误导；改由调用方在恢复完成后打印真实数量
    if (ScriptData::Avaiable.empty())
        return;

    ifstream ScriptFile(ScriptInstallList.c_str());
    if (!ScriptFile.is_open())
        return;

    string line;
    while (getline(ScriptFile, line))
    {
        if (line.empty())
            continue;
        // 如果脚本名没有 ":" 时，触发键按 0（未设置）处理
        string name = line;
        int key = 0;
        const size_t tab = line.find(':');
        if (tab != string::npos)
        {
            name = line.substr(0, tab);
            try
            {
                key = stoi(line.substr(tab + 1));
            }
            catch (...)
            {
                PrintLog("[Plugins] 恢复已安装插件时，解析触发键失败，已忽略该行：" + line, PL_WARNING);
                continue;
            }
        }

        {
            // 写入锁
            lock_guard lock1(ScriptData::AvaiableMtx);
            lock_guard lock2(ScriptData::InstalledMtx);
            for (ScriptInfo& asc : ScriptData::Avaiable)
            {
                if (asc.name == name)
                {
                    // 防重：安装列表里同名脚本只恢复一份
                    //（历史遗留的重复安装会在 cfg 里留下多行，恢复两次会让同一脚本跑两份线程）
                    bool IsAlreadyInstalled = false;
                    for (const ScriptInfo& inst : ScriptData::Installed)
                    {
                        if (inst.name == name)
                        {
                            IsAlreadyInstalled = true;
                            break;
                        }
                    }
                    if (IsAlreadyInstalled)
                        break;
                    asc.IsInstalled = true;
                    ScriptData::Installed.push_back(asc);
                    {
                        lock_guard lock3(ScriptData::StatesMtx);
                        ScriptData::States[asc.file].key = key;
                    }
                    // 这里只恢复"已安装"列表与触发键，不启动脚本：
                    // 脚本现在默认开机自动启动，统一由 ScriptAutoStartThread 启动，
                    // 若此处也 StartScript，同一脚本会被启动两份（main 被执行两次）。
                    break;
                }
            }
        }
    }
}

// 严重错误处理
static int panic_handler(lua_State* L)
{
    string msg = lua_tostring(L, -1);
    if (msg.empty())
        msg = "(no error message)";
    ::PrintLog("Lua错误: " + msg, PL_ERROR);
    //IsEnableScript = false;
    RenderMessageBox(StringToUTF8(msg).c_str(), config.UseChinese ? u8"Lua解析器出现致命错误" : "Lua parser fatal error occurred", MB_OK);
    IsGCExit = true;
    return 0;
}

// 脚本自动启动线程：等窗口初始化完成后，按 InstallList.txt 恢复出的已安装列表逐个启动。
DWORD WINAPI ScriptAutoStartThread(LPVOID lpParam)
{
    while (!IsGCExit && !IsWindowInitSuccessful)
        Sleep(50);
    if (IsGCExit)
        return 0;

    vector<ScriptInfo> scripts;
    {
        shared_lock lock(ScriptData::InstalledMtx);
        scripts = ScriptData::Installed;
    }
    if (scripts.empty())
        return 0;

    vector<string> failed;
    for (const ScriptInfo& script : scripts)
    {
        if (IsGCExit)
            break;
        {
            // 兜底防护：已在运行的脚本不重复启动（重复启动会让 main 被两个线程同时执行）
            shared_lock lock(ScriptData::StatesMtx);
            auto st = ScriptData::States.find(script.file);
            if (st != ScriptData::States.end() && st->second.IsRunning)
                continue;
        }
        const HANDLE hThread = StartScript(script.file);
        if (hThread != NULL)
            StoreScriptThreadHandle(script.file, hThread);
        else
        {
            failed.push_back(script.name);
            lock_guard lock(ScriptData::StatesMtx);
            auto it = ScriptData::States.find(script.file);
            if (it != ScriptData::States.end())
                it->second.IsRunning = false;
        }
    }

    if (!failed.empty())
    {
        string names;
        for (const string& n : failed)
            names += (names.empty() ? "" : ", ") + n;
        PrintLog("[Plugins] 插件启动失败：" + names, PL_ERROR);
        RenderTip((config.UseChinese ? u8"插件启动失败："s : "Failed to start: "s) + names, 6.0f, PT_ERROR);
    }
    return 0;
}


int LoadScript(lua_State* L, const string& file)
{
    bool IsError = false;
    string errmsg;
    if (L == NULL)   // LuaInit 失败（如内存不足）时返回 NULL，直接交给 Lua API 会空指针崩溃
    {
        PrintLog("[Plugins] 加载插件失败：lua_State 未初始化：" + file, PL_ERROR);
        return 1;
    }
    try
    {
        PrintLog("[Plugins] 加载插件：" + file, PL_INFO);
        // 传入错误处理函数，保证在错误栈展开前捕获调用堆栈
        lua_pushcfunction(L, LuaErrorHandler);
        const int errfunc = lua_gettop(L);
        int status = luaL_loadfile(L, file.c_str());
        if (status == LUA_OK)
            status = lua_pcall(L, 0, 0, errfunc);
        if (status != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            const string errText = err ? err : "(error object is not a string)";
            lua_settop(L, errfunc - 1);   // 同时移除错误对象与错误处理函数
            PrintLog("[Plugins] 插件加载错误：" + errText, PL_ERROR);
            RenderTip((config.UseChinese ? u8"插件执行错误"s : "Plugin execution error"s) + StringToUTF8(errText), 10.0f, PT_ERROR);
            MessageBeep(MB_ICONERROR);
            ReportScriptError(file, errText);
            return 3;
        }
        lua_settop(L, errfunc - 1);   // 成功时移除错误处理函数
    }
    catch (const exception& e)
    {
        IsError = true;
        errmsg = e.what();
    }
    catch (...)
    {
        IsError = true;
        errmsg = "Unknown error";
    }

    if (IsError)
    {
        PrintLog("[Plugins] 加载插件失败：" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"加载插件失败："s + errmsg : "Failed to load plugin: "s + errmsg), 10.0f, PT_ERROR);
        MessageBeep(MB_ICONERROR);
        return 1;
    }
    return 0;
}

int CalledScriptFunction(lua_State* L, const string& func)
{
    bool IsError = false;
    string errmsg;
    if (L == NULL)   // lua_State 未初始化：不要调用任何 Lua API
    {
        PrintLog("[Plugins] 调用插件函数失败：lua_State 未初始化：" + func, PL_ERROR);
        return 1;
    }
    try
    {
        // 先压入错误处理函数，作为 lua_pcall 的 msgh（必须位于被调用函数之下）
        lua_pushcfunction(L, LuaErrorHandler);
        const int errfunc = lua_gettop(L);

        lua_getglobal(L, func.c_str());
        if (!lua_isfunction(L, -1))
        {
            PrintLog("[Plugins] 无法找到插件函数：" + func, PL_ERROR);
            MessageBeep(MB_ICONERROR);
            lua_settop(L, errfunc - 1);   // 移除错误处理函数
            return 1;
        }
        if (lua_pcall(L, 0, 0, errfunc) != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            const string errText = err ? err : "(error object is not a string)";
            lua_settop(L, errfunc - 1);   // 同时移除错误对象与错误处理函数
            PrintLog("[Plugins] 插件执行错误：" + errText, PL_ERROR);

            const string errUtf8 = StringToUTF8(errText);   // 持有到函数作用域结束
            RenderTip((config.UseChinese ? u8"插件执行错误"s : "Plugin execution error"s) + errUtf8, 10.0f, PT_ERROR);
            MessageBeep(MB_ICONERROR);
            ReportScriptError(CurrentScriptFile, errText);
            return 2;
        }
        lua_settop(L, errfunc - 1);   // 成功时移除错误处理函数
    }
    catch (const exception& e)
    {
        IsError = true;
        errmsg = e.what();
    }
    catch (...)
    {
        IsError = true;
        errmsg = "Unknown error";
    }

    if (IsError)
    {
        PrintLog("[Plugins] 调用插件函数失败：" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"调用插件函数失败："s + errmsg : "Failed to call plugin function: "s + errmsg), 10.0f, PT_ERROR);
        MessageBeep(MB_ICONERROR);
        return 1;
    }
    return 0;
}

// 检测是否初始化，没有就自动初始化
inline bool InitLuaState(const string& file, LuaState& LuaInfo)
{
    bool IsError = false;
    string errmsg;
    {
        // 写入锁
        lock_guard lock(LuaInfo.Mtx);
        try
        {
            if (LuaInfo.lua == NULL)
            {
                LuaInfo.lua = LuaInit(file);
                if (LuaInfo.lua == NULL)
                {
                    // LuaInit 失败时已置 IsGCExit 并打过日志；这里直接返回，
                    // 不能把 NULL 继续传给 LoadScript（会空指针崩溃）
                    PrintLog("初始化Lua环境失败：lua_State 创建失败", PL_ERROR);
                    return false;
                }
            }
            if (!LuaInfo.IsLoaded)
            {
                int ret = LoadScript(LuaInfo.lua, file);
                if (ret != 0)
                {
                    // 脚本自身有问题（语法错误 / 加载期报错）：请求自动卸载，
                    // 保证"已安装"列表里只留下能正常运行的脚本。
                    RequestScriptUninstall(file, config.UseChinese ? u8"插件加载失败" : "plugin failed to load");
                    return false;
                }
                LuaInfo.IsLoaded = true;
            }
        }
        catch (const exception& e)
        {
            IsError = true;
            errmsg = e.what();
        }
        catch (...)
        {
            IsError = true;
            errmsg = "Unknown error";
        }
    }

    if (IsError)
    {
        PrintLog("[Plugins] 初始化插件环境失败：" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"初始化插件环境失败："s + errmsg : "Failed to initialize plugin environment: "s + errmsg), 10.0f, PT_ERROR);
        MessageBeep(MB_ICONERROR);
        return false;
    }
    return true;
}

DWORD WINAPI ScriptKeyControlThread(LPVOID lpParam)
{
#define SKC_KEY_UP 1
#define SKC_KEY_DOWN 2
#define SKC_KEY_CLICK 3
    while (!IsGCExit)
    {
        // 修复：原代码在读锁内保存 map 迭代器到 modifications，读锁释放后再用写锁解引用这些迭代器。
        // 期间其他线程（如 CloseScriptThread）可能 erase 元素 → 迭代器失效 → 0xC0000005。
        // 改为保存 key（string），写锁内重新 find 并检查存在性。
        vector<pair<string, int>> modifications;
        {
            // 读取锁
            shared_lock lock(ScriptData::StatesMtx);
            for (const auto& entry : ScriptData::States)
            {
                const string& file = entry.first;
                const int key = entry.second.key;
                if (GetKeyClick(key))
                    modifications.emplace_back(file, SKC_KEY_CLICK);
                if (GetKeyDown(key))
                    modifications.emplace_back(file, SKC_KEY_DOWN);
                if (GetKeyUp(key))
                    modifications.emplace_back(file, SKC_KEY_UP);
            }
        } // read_lock 在这里自动释放

        if (!modifications.empty())
        {
            lock_guard write_lock(ScriptData::StatesMtx);
            for (const auto& mod : modifications)
            {
                auto it = ScriptData::States.find(mod.first);
                if (it == ScriptData::States.end())
                    continue; // 脚本已被关闭/删除，跳过
                // mod.second 是要设置的值
                switch (mod.second)
                {
                case SKC_KEY_CLICK:
                    it->second.IsClickedKey = true;
                    break;
                case SKC_KEY_DOWN:
                    it->second.IsPressedKey = true;
                    break;
                case SKC_KEY_UP:
                    it->second.IsPressedKey = false;
                    it->second.IsReleaseKey = true;
                    break;
                }
            }
        }

        Sleep(8);
    }
#undef SKC_KEY_UP
#undef SKC_KEY_DOWN
#undef SKC_KEY_CLICK
    return 0;
}

HANDLE StartScript(const string& file)
{
    PrintLog("[Plugins] 启动插件线程：" + file, PL_INFO);
    
    auto thread_func = [](std::atomic<HANDLE>* hThread, const string& file)
    {
        // 标记当前脚本线程的 file，供 ScriptAPI 定位自己的 ScriptState（无需遍历 LuaStates，避免死锁环）
        CurrentScriptFile = file;
        // 脚本线程运行标记（供"启用"开关判断）：所有退出路径（失败/崩溃/正常）都要复位
        auto MarkNotRunning = [&file]()
        {
            lock_guard lock(ScriptData::StatesMtx);
            auto it = ScriptData::States.find(file);
            if (it != ScriptData::States.end())
                it->second.IsRunning = false;
        };
        // 确保 ScriptData::States 条目存在：原代码在循环头 ScriptData::States[file]（operator[]）隐式创建，
        // 改成 find 后必须显式创建，否则新安装的脚本首次运行会因 find 失败而立即退出。
        {
            lock_guard lock(ScriptData::StatesMtx);
            ScriptData::States[file].IsRunning = true;
        }
        bool IsError = false;
        string errmsg;
        LuaState* lua = nullptr;
        {
            // 写入锁：脚本线程的"所有者"角色，锁内 operator[] 可安全插入（CloseScriptThread 会等本线程退出才 erase）
            lock_guard lock(LuaStatesMtx);
            lua = &LuaStates[file];
            if (!lua->lua)
            {
                // 修复：初始化也必须在 RunningMtx 内——RenderScriptGui/SaveScriptConfig 可能在其他线程
                // 持 RunningMtx 调用 render/saveConfig，若此处不加 RunningMtx，两个线程会对同一 lua_State
                // 同时调用 Lua API（Lua 非线程安全）→ 状态损坏/崩溃。
                // 锁顺序全局统一：LuaStatesMtx → RunningMtx → Mtx。
                lock_guard run_lock(lua->RunningMtx);
                if (!InitLuaState(file, *lua))
                {
                    hThread->store(nullptr);
                    MarkNotRunning();
                    return;
                }
            }
        }
        // 无论是否第一次运行，都需要复制线程句柄
        {
            HANDLE hRealHandle = NULL;
            HANDLE hPseudoHandle = GetCurrentThread();
            if (!DuplicateHandle(GetCurrentProcess(), hPseudoHandle, GetCurrentProcess(), &hRealHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
            {
                PrintLog("[Plugins] 复制线程句柄失败(Error ID:1)", PL_ERROR);
                hThread->store(nullptr);
                MarkNotRunning();
                return;
            }
            hThread->store(hRealHandle);
            // 句柄复制成功后立即回填"已安装"列表：StartScript 的调用方最多只等 1s，
            // 超时会漏记句柄，之后卸载/退出就停不掉这个脚本线程（CloseScriptThread 只能干等/报"句柄无效"）。
            // 用 try_lock：ClearScriptCache/CloseAllLua 会持 InstalledMtx 写锁等待本线程退出，
            // 这里绝不能阻塞等待该锁（否则互相等待）。
            {
                unique_lock<shared_mutex> installedLock(ScriptData::InstalledMtx, try_to_lock);
                if (installedLock.owns_lock())
                {
                    for (ScriptInfo& s : ScriptData::Installed)
                    {
                        if (s.file == file)
                        {
                            s.hThread = hRealHandle;
                            break;
                        }
                    }
                }
            }
        }

        {
            lock_guard run_lock(lua->RunningMtx);
            // initScript 是可选入口：返回 1 = 脚本没定义它（正常）；返回 2 = 执行报错，脚本自身有问题
            if (CalledScriptFunction(lua->lua, "initScript") == 2)
                RequestScriptUninstall(file, config.UseChinese ? u8"initScript 执行报错" : "initScript raised an error");
        }
        // 启动脚本！！！开始脚本循环
        bool IsSuspend = false;
        bool IsExit = false;
        int LoopDelay = 0;
        while (!IsGCExit)
        {
            {
                // 读取锁
                shared_lock lock(ScriptData::StatesMtx);
                // 修复：原代码在 shared_lock（读锁）下调用 ScriptData::States[file]（operator[]），
                // key 不存在时会插入元素（写操作却持读锁）→ 数据竞争。改为 find。
                auto it = ScriptData::States.find(file);
                if (it == ScriptData::States.end())
                    break; // 状态已被清除（脚本被关闭），退出脚本
                const ScriptState& state = it->second;
                if (state.IsExit)
                    break; // 退出脚本
                IsSuspend = state.IsSuspend;
                LoopDelay = state.LoopDelay;
            }

            if (IsSuspend)
            {
                // 此时未持有任何锁，安全睡眠
                Sleep(10);
                continue;
            }

            int ret = 0;
            {
                // 写入锁
                lock_guard run_lock(lua->RunningMtx);
                ret = CalledScriptFunction(lua->lua, "main");
            }
            if (ret != 0)
            {
                // 脚本运行期报错（ret=2）或没有 main 入口（ret=1）：请求自动卸载，
                // 由后台线程完成"停线程 + 清状态 + 移出已安装列表 + 写盘"。
                // 测试脚本（脚本开发页的 test.lua）不参与安装，这里由 RequestScriptUninstall 自动跳过。
                RequestScriptUninstall(file, (ret == 2)
                    ? (config.UseChinese ? u8"main 执行报错" : "main raised an error")
                    : (config.UseChinese ? u8"缺少 main 入口函数" : "main entry function is missing"));
                break; // 退出！
            }
            Sleep(LoopDelay);
        }
        {
            // 修复：lua_close 与 render/saveConfig/main 的 Lua 调用并发 → 同一 lua_State 双线程访问。
            // release() 也必须在 RunningMtx 内串行化。
            lock_guard run_lock(lua->RunningMtx);
            lua->release();
        }
        Minecraft::chat::ChatManager::instance().CloseReader(file);
        MarkNotRunning();   // 正常退出：复位运行标记（供"启用"开关下次启动）
        PrintLog("[Plugins] 插件线程退出：" + file, PL_INFO);
    };
    // 修复：原代码把局部变量 hThread 的地址传给 detach 线程（thread_func 会写入 *hThread），
    // 若 StartScript 等待超时返回后线程才写入 → 写已销毁的栈内存（悬垂写）。
    // 改为 shared_ptr 堆分配句柄，由线程持有副本保证生命周期。
    auto hThread = make_shared<std::atomic<HANDLE>>(nullptr);
    thread t([hThread, file, thread_func]() { thread_func(hThread.get(), file); });
    t.detach();

    // 等待线程启动并且获得到线程句柄
    int try_count = 20; // 50 * 20 = 1000 = 1s
    while (hThread->load() == nullptr && try_count > 0)
    {
        Sleep(50);
        try_count--;
    }
    return hThread->load();
}

bool RenderScriptGui(const string& file)
{
    // 修复：必须在 LuaStatesMtx 锁内完成 RunningMtx 获取与全部 Lua 调用——
    // 若先取指针再释放锁，CloseScriptThread 可在窗口期 release()+erase → LuaState 悬垂 UAF。
    // 锁顺序全局统一：LuaStatesMtx → RunningMtx → Mtx（与 thread_func 初始化/CloseScriptThread 一致）。
    lock_guard lock(LuaStatesMtx);
    auto it = LuaStates.find(file);
    if (it == LuaStates.end())
    {
        // 修复：删除自动启动后脚本线程可能未运行（LuaStates 无条目），
        // 但设置界面仍需渲染——主动创建 LuaState 条目并初始化，供 render 绘制。
        // 注意：LuaState 含 mutex 成员（不可移动/拷贝），不能用 emplace 传值，需 operator[] 就地默认构造。
        LuaStates[file];
        it = LuaStates.find(file);
    }
    // 修复：RunningMtx 用 try_lock——脚本 main 正在执行（长 Sleep/耗时迭代）时，
    // 主线程渲染不阻塞等待（否则渲染循环卡死 = 点击设置后 UI 卡住），本帧跳过 render，下一帧再试。
    unique_lock<mutex> run_lock(it->second.RunningMtx, try_to_lock);
    if (!run_lock.owns_lock())
        return true;   // 脚本 main 忙：本帧跳过 render（不误关设置弹窗）
    // 设置 CurrentScriptFile，让脚本 render 内的 IsScriptKeyDown/SetScriptLoopDelay 等能定位自己的状态
    const string PrevScript = CurrentScriptFile;
    CurrentScriptFile = file;
    if (!InitLuaState(file, it->second))
    {
        CurrentScriptFile = PrevScript;
        return 0;
    }
    const int funcRet = CalledScriptFunction(it->second.lua, "render");
    CurrentScriptFile = PrevScript;
    // render 执行报错（2）：脚本自身有问题 → 请求自动卸载；找不到 render（1）属正常，只是没有设置界面。
    if (funcRet == 2)
        RequestScriptUninstall(file, config.UseChinese ? u8"设置界面 render 执行报错" : "render raised an error");
    return funcRet == 0;
}

int SaveScriptConfig(const string& file)
{
    // 修复：SaveScriptConfig 在 detach 线程运行，应用退出时（GCExit）CloseAllLua 已清空 LuaStates，
    // 残留线程访问会 UAF → 崩溃。入口检查退出标志。
    if (IsGCExit)
        return 0;
    // 修复：原代码无锁 LuaStates[file]（operator[]）→ map 竞争；且取指针后释放锁 → UAF。
    // 与 RenderScriptGui 同样在 LuaStatesMtx 锁内完成全部访问。
    lock_guard lock(LuaStatesMtx);
    auto it = LuaStates.find(file);
    if (it == LuaStates.end())
    {
        // 修复：脚本线程未运行时同样创建 LuaState 条目，保证 saveConfig 可被调用（保存默认配置）。
        // LuaState 含 mutex（不可移动），用 operator[] 就地默认构造。
        LuaStates[file];
        it = LuaStates.find(file);
    }
    // 修复：与 RenderScriptGui 一致，RunningMtx 用 try_lock，脚本忙时跳过保存（避免后台线程堆积）
    unique_lock<mutex> run_lock(it->second.RunningMtx, try_to_lock);
    if (!run_lock.owns_lock())
        return 0;
    const string PrevScript = CurrentScriptFile;
    CurrentScriptFile = file;
    if (!InitLuaState(file, it->second))
    {
        CurrentScriptFile = PrevScript;
        return 0;
    }
    const int funcRet = CalledScriptFunction(it->second.lua, "saveConfig");
    CurrentScriptFile = PrevScript;
    // saveConfig 执行报错（2）：脚本自身有问题 → 请求自动卸载；没有 saveConfig（1）属正常。
    if (funcRet == 2)
        RequestScriptUninstall(file, config.UseChinese ? u8"保存配置 saveConfig 执行报错" : "saveConfig raised an error");
    return funcRet == 0;
}

void CloseScriptThread(const string& filename, ScriptInfo& info)
{
    {
        // 写入锁
        lock_guard lock(ScriptData::StatesMtx);
        ScriptData::States[filename].IsExit = true;
    }

    bool IsExit = false;
    DWORD ret = WaitForSingleObject(info.hThread, 2000);
    if (ret == WAIT_FAILED)
        PrintLog("[Plugins] 插件线程句柄无效：" + info.file, PL_ERROR);
    else if (ret == WAIT_TIMEOUT)
    {
        PrintLog("[Plugins] 插件线程超时退出，将强制结束：" + info.file, PL_WARNING);
        if (info.hThread)
            TerminateThread(info.hThread, -1);
        else
            PrintLog("[Plugins] 线程句柄无效：" + info.file, PL_ERROR);
    }

    if (info.hThread != NULL)
    {
        CloseHandle(info.hThread);
        info.hThread = NULL;
    }
    {
        // 写入锁
        lock_guard lock(ScriptData::StatesMtx);
        ScriptData::States.erase(filename);
    }
    // 清理 LuaStates，以便下次运行时重新加载脚本
    {
        // 写入锁
        lock_guard lock(LuaStatesMtx);
        auto it = LuaStates.find(filename);
        if (it != LuaStates.end())
        {
            // 修复：lua_close 必须与 render/saveConfig/main 的 Lua 调用串行化（Lua 非线程安全）。
            // 仅在线程正常退出（WAIT_OBJECT_0）时等待 RunningMtx——若走 TerminateThread 强杀，
            // 线程可能持有 RunningMtx，此时获取该锁会永久卡死本线程。
            if (ret == WAIT_OBJECT_0)
            {
                lock_guard run_lock(it->second.RunningMtx);
                it->second.release();
            }
            else
            {
                it->second.lua = nullptr;
                it->second.IsLoaded = false;
            }
            LuaStates.erase(it);
        }
    }
}

void ClearScriptCache()
{
    bool IsError = false;
    string errmsg;
    try
    {
        // 关闭线程会清掉该脚本的 ScriptState（含触发键与"暂停"标记），
        // 但清缓存不应该把用户设置的启动按键/暂停状态重置，先取快照。
        // 另外不清空 ScriptData::Installed：脚本是插件，"清缓存"不等于"卸载"，
        // 已安装列表必须与 InstalledScripts.cfg 保持一致（否则界面上列表空了、
        // 配置文件里还有条目，之后任何一次安装写盘都会把其它脚本条目覆盖掉）。
        vector<pair<string, int>> savedKeys;
        vector<string> suspendedFiles;
        {
            shared_lock stateLock(ScriptData::StatesMtx);
            for (const ScriptInfo& script : ScriptData::Installed)
            {
                auto it = ScriptData::States.find(script.file);
                if (it == ScriptData::States.end())
                    continue;
                savedKeys.push_back({ script.file, it->second.key });
                if (it->second.IsSuspend)
                    suspendedFiles.push_back(script.file);
            }
        }
        {
            // 写入锁
            lock_guard lock1(ScriptData::InstalledMtx);
            for (ScriptInfo& script : ScriptData::Installed)
                CloseScriptThread(script.file, script);
        }
        {
            lock_guard lock2(ScriptData::StatesMtx);
            ScriptData::States.clear();
        }
        {
            lock_guard lock(LuaStatesMtx);
            for (auto& lua : LuaStates)
            {
                lua.second.IsLoaded = false;
                unique_lock<mutex> run_lock(lua.second.RunningMtx, try_to_lock);
                if (run_lock.owns_lock())
                    lua.second.release();
                else
                {
                    lua.second.lua = nullptr;
                    lua.second.IsLoaded = false;
                }
            }
            // 修复：原代码 LuaStates.clear() 在锁外执行，与锁内 find/operator[] 并发 → map 竞争
            LuaStates.clear();
        }
        // 只清解压缓存目录（ScriptPath\temp）。
        // 原代码 DeleteDirectory(ScriptPath) 会把整个脚本目录删掉 —— 那里放着用户所有
        // 脚本 zip 和 InstalledScripts.cfg，删掉后脚本无法恢复（只能重新下载）。
        DeleteDirectory(ScriptTemp);
        CreateDirectory(ScriptTemp.c_str(), NULL);

        {
            // 重新扫描脚本目录（重新解压）并刷新可用列表
            RefushScriptList();
        }

        // 重新启动已安装脚本：脚本是插件，"已安装即运行"，
        // 清缓存的意义就是让改过的脚本重新加载，所以必须重新拉起脚本线程。
        vector<ScriptInfo> scripts;
        {
            shared_lock lock(ScriptData::InstalledMtx);
            scripts = ScriptData::Installed;
        }
        for (const ScriptInfo& script : scripts)
        {
            // 恢复清缓存前的触发键与暂停状态
            int key = 0;
            for (const auto& item : savedKeys)
            {
                if (item.first == script.file)
                {
                    key = item.second;
                    break;
                }
            }
            bool IsSuspend = false;
            for (const string& file : suspendedFiles)
            {
                if (file == script.file)
                {
                    IsSuspend = true;
                    break;
                }
            }
            {
                lock_guard lock(ScriptData::StatesMtx);
                ScriptState& state = ScriptData::States[script.file];
                state.key = key;
                state.IsSuspend = IsSuspend;
                state.IsRunning = !IsSuspend;   // 被用户暂停的脚本清缓存后仍然保持暂停
            }
            if (IsSuspend)
                continue;
            StoreScriptThreadHandle(script.file, StartScript(script.file));
        }
    }
    catch (const exception& e)
    {
        IsError = true;
        errmsg = e.what();
    }
    catch (...)
    {
        IsError = true;
        errmsg = "Unknown error";
    }

    if (IsError)
    {
        PrintLog("清除插件缓存失败：" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"清除插件缓存失败："s + errmsg : "Failed to clear plugin cache: "s + errmsg), 10.0f, PT_ERROR);
        MessageBeep(MB_ICONERROR);
    }
}

void CloseAllLua()
{
    {
        // 写入锁
        lock_guard lock1(ScriptData::InstalledMtx);
        for (ScriptInfo script : ScriptData::Installed)
            CloseScriptThread(script.file, script);
        // 已安装列表的持久化由 SaveInstalledScriptList 负责（安装/卸载时即时写盘），
        // 退出时这里只清理运行期状态。原先的 InstalledScriptsOnExit 快照写完后从未被读取，已删除。
        {
            lock_guard stateLock(ScriptData::StatesMtx);
            ScriptData::States.clear();
        }
        ScriptData::Installed.clear();
    }
    {
        // 写入锁
        lock_guard lock(LuaStatesMtx);
        for (auto& lua : LuaStates)
        {
            lua.second.IsLoaded = false;
            unique_lock<mutex> run_lock(lua.second.RunningMtx, try_to_lock);
            if (run_lock.owns_lock())
                lua.second.release();
            else
            {
                lua.second.lua = nullptr;
                lua.second.IsLoaded = false;
            }
        }
        // 修复：原代码 LuaStates.clear() 在锁外执行，与锁内 find/operator[] 并发 → map 竞争
        LuaStates.clear();
    }
}

//=======================脚本卸载（插件模型）=======================
// 脚本升级为"插件"后只有两种状态：已安装 / 未安装。已安装即运行，
// 因此脚本一旦因自身报错（Lua error）无法正常运行，就必须回到"未安装"状态。
//
// 三步流程，避免在 Lua 调用栈里"自杀"：
//   1) RequestScriptUninstall：任意线程（含脚本线程、Lua 回调内）只入队；
//   2) ProcessScriptUninstallQueue：主渲染循环每帧派发到一个独立线程；
//   3) UninstallScript：停线程 → 清 Lua/状态 → 移出已安装列表 → 立即写盘。

// 请求卸载（只入队，不加任何可能阻塞的锁，可在脚本线程/Lua 回调内安全调用）
void RequestScriptUninstall(const string& file, const string& reason)
{
    if (file.empty() || file == ScriptTestFile) // 脚本开发页的 test.lua 只是测试用，不参与安装/卸载
        return;

    lock_guard lock(ScriptData::UninstallQueueMtx);
    for (const auto& item : ScriptData::UninstallQueue)
    {
        if (item.first == file)
            return;   // 同一脚本已排队，保留第一次的卸载原因
    }
    ScriptData::UninstallQueue.push_back({ file, reason });
    PrintLog("[Plugins] 插件报错，已提交自动卸载请求：" + file + "（" + reason + "）", PL_WARNING);
}

// 真正的卸载。调用前不要持有 ScriptData 的锁（内部会依次加锁），由卸载线程调用。
bool UninstallScript(const string& file, const string& reason)
{
    if (file.empty() || file == ScriptTestFile)
        return false;

    ScriptInfo target;
    bool IsFound = false;
    {
        lock_guard lock(ScriptData::InstalledMtx);
        for (int i = 0; i < (int)ScriptData::Installed.size(); i++)
        {
            if (ScriptData::Installed[i].file == file)
            {
                target = ScriptData::Installed[i];
                ScriptData::Installed.erase(ScriptData::Installed.begin() + i);
                IsFound = true;
                break;
            }
        }
    }
    if (!IsFound)
        return false;   // 已经不在"已安装"列表（例如被手动卸载过），无需重复处理

    {
        // 可用列表同步回"未安装"，否则脚本市场/可用列表会一直显示"已安装"而无法重新安装
        lock_guard lock(ScriptData::AvaiableMtx);
        for (ScriptInfo& s : ScriptData::Avaiable)
        {
            if (s.file == file)
            {
                s.IsInstalled = false;
                s.hThread = NULL;
                break;
            }
        }
    }

    // 停止脚本线程并释放该脚本的 Lua 状态（CloseScriptThread 内部自己加锁）
    CloseScriptThread(file, target);

    // 已安装列表已变化，立即落盘：否则重启后会把出错脚本又恢复成"已安装"
    SaveInstalledScriptList();

    // 日志用文件路径（日志文件是 ANSI/GBK，ScriptInfo::name 是 UTF-8，混写会乱码）；
    // 界面提示用 name（ImGui 需要 UTF-8）。
    if (reason.empty())
        PrintLog("[Plugins] 已卸载插件：" + file, PL_INFO);
    else
    {
        PrintLog("[Plugins] 插件已自动卸载：" + file + "（" + reason + "）", PL_WARNING);
        RenderTip((config.UseChinese ? u8"插件已自动卸载："s : "Plugin auto-uninstalled: "s) + target.name +
            (config.UseChinese ? u8"\n原因："s : "\nReason: "s) + reason, 8.0f, PT_ERROR);
    }
    return true;
}

// 主渲染循环每帧调用：把卸载请求交给独立线程执行，
// 避免 CloseScriptThread 的等待/强杀阻塞 UI 线程（脚本 main 可能正卡在长 Sleep 里）。
void ProcessScriptUninstallQueue()
{
    if (IsGCExit || ScriptData::IsUninstallWorkerBusy.load())
        return;

    vector<pair<string, string>> tasks;
    {
        lock_guard lock(ScriptData::UninstallQueueMtx);
        if (ScriptData::UninstallQueue.empty())
            return;
        tasks.swap(ScriptData::UninstallQueue);
    }

    ScriptData::IsUninstallWorkerBusy.store(true);
    thread th([tasks]()
        {
            for (const auto& task : tasks)
                UninstallScript(task.first, task.second);
            ScriptData::IsUninstallWorkerBusy.store(false);
        });
    th.detach();
}

// 把脚本线程句柄记进"已安装"列表：卸载/退出时 CloseScriptThread 要靠它正常停止脚本线程。
// 调用方：启动脚本的各处（自动启动/安装/启用开关/清缓存）。StartScript 超时（返回 NULL）时
// 由脚本线程自己在内部用 try_lock 回填，两处互补。
void StoreScriptThreadHandle(const string& file, HANDLE hThread)
{
    if (hThread == NULL)
        return;
    lock_guard lock(ScriptData::InstalledMtx);
    for (ScriptInfo& s : ScriptData::Installed)
    {
        if (s.file == file)
        {
            s.hThread = hThread;
            break;
        }
    }
}

// 安装一个脚本（按脚本名从"可用脚本"列表取元数据）。"安装"按钮与"更新后重新安装"共用这一处实现，
// 避免加入已安装列表 / 建状态 / 启动 / 写盘 这几步在多处各写一遍。
// key、IsSuspend 用于"更新后重新安装"时恢复用户原来的触发键与暂停状态。
bool InstallScriptByName(const string& name, int key, bool IsSuspend)
{
    ScriptInfo script;
    bool IsFound = false;
    {
        shared_lock lock(ScriptData::AvaiableMtx);
        for (const ScriptInfo& s : ScriptData::Avaiable)
        {
            if (s.name == name)
            {
                script = s;
                IsFound = true;
                break;
            }
        }
    }
    if (!IsFound)
    {
        PrintLog("[Plugins] 安装失败：可用插件列表中没有该插件：" + name, PL_ERROR);
        return false;
    }

    script.hThread = nullptr;
    {
        // 防重复安装：刷新/竞态下同一脚本可能被点两次"安装"，
        // 已安装列表就会出现两条同名脚本、两份线程同时跑
        lock_guard lock(ScriptData::InstalledMtx);
        for (const ScriptInfo& s : ScriptData::Installed)
        {
            if (s.name == name)
            {
                PrintLog("[Plugins] 插件已安装，忽略重复安装：" + script.file, PL_WARNING);
                return true;
            }
        }
        ScriptData::Installed.push_back(script);
    }
    {
        // 建 ScriptState：触发键、暂停状态（供"启用"开关与启动按键使用）
        lock_guard lock(ScriptData::StatesMtx);
        ScriptState& state = ScriptData::States[script.file];
        state.key = key;
        state.IsSuspend = IsSuspend;
        state.IsRunning = !IsSuspend;
    }
    {
        lock_guard lock(ScriptData::AvaiableMtx);
        for (ScriptInfo& s : ScriptData::Avaiable)
        {
            if (s.file == script.file)
            {
                s.IsInstalled = true;
                break;
            }
        }
    }
    if (!IsSuspend)
    {
        // 启动脚本并记录线程句柄（卸载/退出时靠它停线程）
        // StartScript 最多只等 1s，超时返回 NULL 时由脚本线程自己补记。
        StoreScriptThreadHandle(script.file, StartScript(script.file));
    }
    SaveInstalledScriptList();
    PrintLog("[Plugins] 已安装插件：" + script.file, PL_INFO);
    return true;
}

// 脚本市场"更新"按钮的完整流程：
//   未安装 -> 只覆盖旧版本脚本文件；
//   已安装 -> 卸载脚本（停线程/释放 Lua/移出已安装列表） -> 覆盖旧版本文件 -> 重新安装并运行。
// 必须走"卸载 -> 覆盖 -> 重装"：文件被覆盖后，正在运行的脚本线程仍在跑旧代码，
// 而且它的 Lua 状态是旧文件加载出来的（原先只能靠用户手动点"清除缓存"才生效）。
// 更新后按原来的触发键/暂停状态恢复；若新版本自身有 Lua 错误，脚本会被自动卸载。
bool UpdateScriptKeepInstallState(const ScriptInfo& script)
{
    // 取已安装副本，以及它当前的触发键与暂停状态
    ScriptInfo installed;
    bool IsInstalled = false;
    {
        shared_lock lock(ScriptData::InstalledMtx);
        for (const ScriptInfo& s : ScriptData::Installed)
        {
            if (s.name == script.name)
            {
                installed = s;
                IsInstalled = true;
                break;
            }
        }
    }
    int key = 0;
    bool IsSuspend = false;
    if (IsInstalled)
    {
        shared_lock lock(ScriptData::StatesMtx);
        auto it = ScriptData::States.find(installed.file);
        if (it != ScriptData::States.end())
        {
            key = it->second.key;
            IsSuspend = it->second.IsSuspend;
        }
    }

    // 已安装：先卸载（停线程、释放 Lua、移出已安装列表并写盘）
    if (IsInstalled)
    {
        PrintLog("[Plugins] 更新已安装插件，先卸载：" + installed.file, PL_INFO);
        UninstallScript(installed.file, "");
    }

    // 覆盖旧版本脚本文件
    if (!UpdateScriptFromShop(script))
    {
        PrintLog("[Plugins] 更新插件失败（无法获取新文件）：" + script.name, PL_ERROR);
        if (IsInstalled)
        {
            // 覆盖失败：把原来的脚本按原状态装回去，别让用户平白丢一个脚本
            InstallScriptByName(script.name, key, IsSuspend);
        }
        return false;
    }

    // 重新扫描脚本目录（重新解压新文件）
    RefushScriptList();

    // 已安装过：重新安装并运行
    if (IsInstalled)
        return InstallScriptByName(script.name, key, IsSuspend);
    return true;
}