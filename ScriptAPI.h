#pragma once
#include "Definitons.h"
#include "Log.h"
#include "ShareCode.h"
#include "CodeModules.h"
#include "GC.h"

extern "C"
{
#include <LuaJIT/lua.h>
#include <LuaJIT/lauxlib.h>
#include <LuaJIT/lualib.h>
#include <LuaJIT/luajit.h>
}

#undef DrawText

// 当前脚本线程正在执行的脚本 file（thread_local：每个脚本线程/主线程各自独立）。
// 用途：让 ScriptAPI（IsScriptKeyDown/Up/Click、SetScriptLoopDelay）无需遍历 LuaStates 即可
// 定位自己的 ScriptState，避免"在 RunningMtx 内获取 LuaStatesMtx"与 RenderScriptGui 的
// "LuaStatesMtx→RunningMtx"形成死锁环（LuaJIT 同一 lua_State 不可跨线程并发调用）。
thread_local string CurrentScriptFile;

typedef struct LuaState
{
    lua_State* lua;
    mutex Mtx;
    mutex RunningMtx;
    bool IsLoaded;

    LuaState() : lua(nullptr), IsLoaded(false) {}
    void release()
    {
        if (lua)
        {
            luaL_dostring(lua, "if jit then jit.off() jit.flush() end collectgarbage('collect') collectgarbage('collect')");
            lua_close(lua);
        }
        lua = nullptr;
    }
    ~LuaState() { release(); }
};
map<string, LuaState> LuaStates;
shared_mutex LuaStatesMtx;

//=========================================前置函数=========================================
// 检查节是否存在
static bool SectionExists(LPCTSTR lpAppName, LPCTSTR lpFileName)
{
    TCHAR szBuffer[256] = { 0 };
    return GetPrivateProfileString(lpAppName, NULL, _T(""),
        szBuffer, sizeof(szBuffer), lpFileName) > 0;
}

// 检查键是否存在
static bool KeyExists(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpFileName)
{
    TCHAR szBuffer[1] = { 0 };
    return GetPrivateProfileString(lpAppName, lpKeyName, _T(""),
        szBuffer, sizeof(szBuffer), lpFileName) > 0;
}

// 写入或更新INI文件中的字符串
static bool SetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName)
{
    if (SectionExists(lpAppName, lpFileName))
    {
        if (KeyExists(lpAppName, lpKeyName, lpFileName))
        {
            // 如果节和键都存在，先删除键
            if (!WritePrivateProfileString(lpAppName, lpKeyName, NULL, lpFileName))
                return false;
        }
    }
    // 写入新的键值对
    if (!WritePrivateProfileString(lpAppName, lpKeyName, lpString, lpFileName))
        return false;
    return true;
}

struct ClipboardData
{
    UINT format;
    HANDLE hData;
};

// 将 GBK (ANSI) 字符串转换为 UTF-16 宽字符串
wstring GBKToWide(const string& gbk)
{
    if (gbk.empty())
        return L"";
    int wlen = MultiByteToWideChar(CP_ACP, 0, gbk.c_str(), -1, nullptr, 0);
    if (wlen <= 0)
    {
        // 若 CP_ACP 失败，尝试显式 GBK 代码页 936
        wlen = MultiByteToWideChar(936, 0, gbk.c_str(), -1, nullptr, 0);
        if (wlen <= 0)
            return L"";
    }
    wstring wstr(wlen, L'\0');
    if (MultiByteToWideChar(CP_ACP, 0, gbk.c_str(), -1, &wstr[0], wlen) == 0)
    {
        // 如果 CP_ACP 转换失败，再用 936
        MultiByteToWideChar(936, 0, gbk.c_str(), -1, &wstr[0], wlen);
    }
    wstr.pop_back(); // 去掉末尾 L'\0'
    return wstr;
}

// hwnd是以前代码废弃的，保留就行，以后改回PostMessage就方便一点
bool SimulatedInputText(HWND hwnd, const string& text)
{
    if (text.empty())
        return true;

    // 1. 将 GBK 转为 UTF-16
    wstring wtext = GBKToWide(text);
    if (wtext.empty())
        return false;

    // 2. 构造 INPUT 数组（每个字符按下+释放）
    vector<INPUT> inputs;
    inputs.reserve(wtext.size() * 2);

    for (wchar_t ch : wtext)
    {
        // 按下
        INPUT in_down = {};
        in_down.type = INPUT_KEYBOARD;
        in_down.ki.wVk = 0;
        in_down.ki.wScan = ch;
        in_down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(in_down);

        // 释放
        INPUT in_up = {};
        in_up.type = INPUT_KEYBOARD;
        in_up.ki.wVk = 0;
        in_up.ki.wScan = ch;
        in_up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(in_up);
    }

    UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    return (sent == inputs.size());
}

//=========================================功能API=========================================
namespace ScriptAPI
{
    int GetTickCount(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetTickCount的参数太多");
        lua_pushinteger(L, GetTickCount64());
        return 1;
    }

    int RandomInt(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "RandomInt的参数不足或太多");
        int min = luaL_checkinteger(L, 1);
        int max = luaL_checkinteger(L, 2);
        lua_pushinteger(L, Random(min, max));
        return 1;
    }

    int RandomBool(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "RandomBool的参数不足或太多");
        double rate = luaL_checknumber(L, 1);
        lua_pushinteger(L, ::RandomBool(rate));
        return 1;
    }

    int GetVersion(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetVersion的参数太多");
        string temp = version;
        temp.erase(0, 2);
        int ver = stoi(temp);
        lua_pushinteger(L, ver);
        return 1;
    }

    int SetClickerSettings(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "SetClickerSettings的参数不足或太多");
        const char* app = luaL_checkstring(L, 1);
        const char* key = luaL_checkstring(L, 2);
        string value;
        switch (lua_type(L, 3))
        {
            case LUA_TSTRING:   value = luaL_checkstring(L, 3);                                break;
            case LUA_TNUMBER:   value = to_string(lua_tointeger(L, 3));                        break;
            case LUA_TBOOLEAN:  value = lua_toboolean(L, 3) ? (char*)"TRUE" : (char*)"FALSE";  break;
            default: value = "0"; break;
        }
        WritePrivateProfileString(app, key, value.c_str(), ConfigFile.c_str()); 
        return 0;
    }

    int GetClickerSettings(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "GetClickerSettings的参数不足或太多");
        const char* app = luaL_checkstring(L, 1);
        const char* key = luaL_checkstring(L, 2);
        string value;
        switch (lua_type(L, 3))
        {
            case LUA_TSTRING:   value = luaL_checkstring(L, 3);                                break;
            case LUA_TNUMBER:   value = to_string(lua_tointeger(L, 3));                        break;
            case LUA_TBOOLEAN:  value = lua_toboolean(L, 3) ? (char*)"TRUE" : (char*)"FALSE";  break;
            default: value = "0"; break;
        }
        char* buffer = new char[MAX_BUF];
        string st;
        GetPrivateProfileString(app, key, value.c_str(), buffer, MAX_BUF, ConfigFile.c_str());
        st = buffer;
        lua_pushstring(L, st.c_str());
        return 1;
    }

    int PrintLog(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "PrintLog的参数不足或太多");
        const char* msg = luaL_checkstring(L, 1);
        int pl = luaL_checkinteger(L, 2);
        ::PrintLog(msg, pl);
        return 0;
    }

    int GetKeyDown(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count <= 0 || count > 3)
            luaL_error(L, "GetKeyDown的参数不足或太多");
        int key = luaL_checkinteger(L, 1);
        bool human = (count >= 2) ? lua_toboolean(L, 2) : false;
        int mode = (count >= 3) ? luaL_checkinteger(L, 3) : GK_API;
        lua_pushboolean(L, ::GetKeyDown(key, human, mode));
        return 1;
    }

    int GetKeyClick(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count <= 0 || count > 3)
            luaL_error(L, "GetKeyClick的参数不足或太多");
        int key = luaL_checkinteger(L, 1);
        bool human = (count >= 2) ? lua_toboolean(L, 2) : false;
        int mode = (count >= 3) ? luaL_checkinteger(L, 3) : GK_API;
        lua_pushboolean(L, ::GetKeyClick(key, human, mode));
        return 1;
    }

    int GetKeyUp(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count <= 0 || count > 3)
            luaL_error(L, "GetKeyUp的参数不足或太多");
        int key = luaL_checkinteger(L, 1);
        bool human = (count >= 2) ? lua_toboolean(L, 2) : false;
        int mode = (count >= 3) ? luaL_checkinteger(L, 3) : GK_API;
        lua_pushboolean(L, ::GetKeyUp(key, human, mode));
        return 1;
    }

    int SetKeyDown(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count != 1 && count != 2)
            luaL_error(L, "SetKeyDown的参数不足或太多");
        
        int key = luaL_checkinteger(L, 1);
        int mode = luaL_optinteger(L, 2, config.SimulateClickMode);
        
        ::SetKeyDown(key, mode);
        return 0;
    }

    int SetKeyUp(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count != 1 && count != 2)
            luaL_error(L, "SetKeyUp的参数不足或太多");

        int key = luaL_checkinteger(L, 1);
        int mode = luaL_optinteger(L, 2, config.SimulateClickMode);

        ::SetKeyUp(key, mode);
        return 0;
    }

    int SetKeyClick(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count <= 0 || count > 3)
            luaL_error(L, "SetKeyClick的参数不足或太多");

        int key = luaL_checkinteger(L, 1);
        int mode = luaL_optinteger(L, 2, config.SimulateClickMode);
        int sleep = luaL_optinteger(L, 3, 60);

        ::SetKeyClick(key, sleep, mode);
        return 0;
    }

    int GetCursorPos(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCursorPos的参数太多");
        POINT pos;
        ::GetCursorPos(&pos);
        lua_pushinteger(L, pos.x);
        lua_pushinteger(L, pos.y);
        return 2;
    }

    int SetCursorPos(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "SetCursorPos的参数不足或太多");
        int x = luaL_checkinteger(L, 1);
        int y = luaL_checkinteger(L, 2);
        POINT pos;
        ::GetCursorPos(&pos);
        if (x < 0 && y >= 0)
            ::SetCursorPos(pos.x, y);
        else if (y < 0 && x >= 0)
            ::SetCursorPos(x, pos.y);
        else if (y >= 0 && x >= 0)
            ::SetCursorPos(x, y);
        return 0;
    }

    int GetCpsRight(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCpsRight的参数太多");
        {
            shared_lock<shared_mutex> lock(CurrentCPSMutex);
            lua_pushinteger(L, CurrentCPS.right);
        }
        return 1;
    }

    int GetCpsLeft(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCpsLeft的参数太多");
        {
            shared_lock<shared_mutex> lock(CurrentCPSMutex);
            lua_pushinteger(L, CurrentCPS.left);
        }
        return 1;
    }

    int GetHumanCpsLeft(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetHumanCpsLeft的参数太多");
        {
            shared_lock<shared_mutex> lock(HumanCPSMutex);
            lua_pushinteger(L, HumanCPS.left);
        }
        return 1;
    }

    int GetHumanCpsRight(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetHumanCpsRight的参数太多");
        {
            shared_lock<shared_mutex> lock(HumanCPSMutex);
            lua_pushinteger(L, HumanCPS.right);
        }
        return 1;
    }

    int Sleep(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "Sleep的参数不足或太多");
        int sleep = lua_tonumber(L, 1);
        if (sleep > 0)
            ::Sleep(sleep);
        return 0;
    }
    
    int GetItemBarPos(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetItemBarPos的参数太多");
        lua_pushinteger(L, ItemBarPos);
        return 1;
    }

    int SetItemBarPos(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count <= 0 || count > 3)
            luaL_error(L, "SetItemBarPos的参数不足或太多");

        int pos = luaL_checkinteger(L, 1);
        if (pos < 0 || pos > 8)
            luaL_error(L, "SetItemBarPos的pos参数超过 0 到 8 的范围");
        int mode = luaL_optinteger(L, 2, config.SimulateClickMode);
        int sleep = luaL_optinteger(L, 3, 60);

        ItemBarPos = pos;
        ::SetKeyClick(config.ItemBarKey[pos], sleep, mode);
        return 0;
    }

    int SetIniFile(lua_State* L)
    {
        if (lua_gettop(L) != 4)
            luaL_error(L, "SetIniFile的参数不足或太多");
        string appname = luaL_checkstring(L, 1);
        string key = luaL_checkstring(L, 2);
        string value;
        switch (lua_type(L, 3))
        {
        case LUA_TSTRING:   value = luaL_checkstring(L, 3);                                break;
        case LUA_TNUMBER:   value = to_string(lua_tointeger(L, 3));                        break;
        case LUA_TBOOLEAN:  value = lua_toboolean(L, 3) ? (char*)"TRUE" : (char*)"FALSE";  break;
        default: value = "0"; break;
        }
        string file = ScriptPath + "\\" + luaL_checkstring(L, 4);
        if (file.find("..") != string::npos)
        {
            ::PrintLog("[Script] 该脚本不安全：文件路径不合法", PL_WARNING);
            luaL_error(L, "该脚本不安全 - 文件路径不合法");
            return 0;
        }
        if (_access(file.c_str(), 0) != 0)
            CreateFile(file);
        SetPrivateProfileString(appname.c_str(), key.c_str(), value.c_str(), file.c_str());
        return 0;
    }

    int GetIniFile(lua_State* L)
    {
        if (lua_gettop(L) != 4)
            luaL_error(L, "GetIniFile的参数不足或太多");
        string appname = luaL_checkstring(L, 1);
        string key = luaL_checkstring(L, 2);
        string value;
        int type = lua_type(L, 3);
        switch (type)
        {
        case LUA_TSTRING:   value = luaL_checkstring(L, 3);                                  break;
        case LUA_TNUMBER:   value = to_string(lua_tointeger(L, 3));                          break;
        case LUA_TBOOLEAN:  value = (lua_toboolean(L, 3) ? (char*)"TRUE" : (char*)"FALSE");  break;
        default: value = "0"; break;
        }
        string file = ScriptPath + "\\" + luaL_checkstring(L, 4);
        if (file.find("..") != string::npos)
        {
            ::PrintLog("[Script] 该脚本不安全，访问的文件路径不合法", PL_WARNING);
            luaL_error(L, "该脚本不安全，访问的文件路径不合法");
            return 0;
        }
        char* buffer = new char[MAX_BUF];
        if (_access(file.c_str(), 0) != 0)
            CreateFile(file);
        GetPrivateProfileString(appname.c_str(), key.c_str(), value.c_str(), buffer, MAX_BUF, file.c_str());
        string str;
        if (buffer)
            str = buffer;
        else
            str = "";
        delete[] buffer;
        switch (type)
        {
        case LUA_TSTRING:  lua_pushstring(L, str.c_str());  break;
        case LUA_TNUMBER:  lua_pushinteger(L, stoi(str));   break;
        case LUA_TBOOLEAN: lua_pushboolean(L, str.find("TRUE") != string::npos);   break;
        default: luaL_error(L, "value参数类型错误"); break;
        }
        return 1;
    }

    int SimulatedInputText(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "SimulatedInputText的参数不足或太多");
        string text = luaL_checkstring(L, 1);
        lua_pushboolean(L, ::SimulatedInputText(GetForegroundWindow(), text));
        return 1;
    }

    // 网络权限控制（每个脚本独立）
    static bool CheckNetworkPermission()
    {
        if (CurrentScriptFile.empty())
            return false;
        int perm = 0;
        {
            shared_lock<shared_mutex> lock(ScriptData::StatesMtx);
            auto it = ScriptData::States.find(CurrentScriptFile);
            if (it == ScriptData::States.end())
                return false;
            perm = it->second.NetworkPermission;
        }
        if (perm == 1)
            return true;
        if (perm == 2)
            return false;
        // 未请求过，弹窗询问用户
        string title = config.UseChinese ? "网络权限请求" : "Network Permission Request";
        string msg = config.UseChinese ?
            ("脚本 [" + CurrentScriptFile + "] 请求使用网络功能，是否允许？") :
            ("Script [" + CurrentScriptFile + "] requests network access. Allow?");
        int ret = ::RenderMessageBox(msg, title.c_str(), MB_OKCANCEL);
        const bool allowed = (ret == IDOK);
        {
            lock_guard<shared_mutex> lock(ScriptData::StatesMtx);
            auto it = ScriptData::States.find(CurrentScriptFile);
            if (it != ScriptData::States.end())
                it->second.NetworkPermission = allowed ? 1 : 2;
        }
        return allowed;
    }

    // 每个脚本独立的套接字表，避免不同脚本的 socket 混淆
    static map<string, map<int, SOCKET>> ScriptSockets;
    static mutex ScriptSocketsMtx;
    static atomic<int> NextSocketId(1);

    static bool EnsureWsaInit()
    {
        static bool WsaStarted = false;
        static mutex WsaMtx;
        if (WsaStarted)
            return true;
        lock_guard<mutex> lock(WsaMtx);
        if (WsaStarted)
            return true;
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
        {
            WsaStarted = true;
            return true;
        }
        return false;
    }

    static SOCKET FindScriptSocket(int id)
    {
        lock_guard<mutex> lock(ScriptSocketsMtx);
        auto outer = ScriptSockets.find(CurrentScriptFile);
        if (outer == ScriptSockets.end())
            return INVALID_SOCKET;
        auto it = outer->second.find(id);
        if (it == outer->second.end())
            return INVALID_SOCKET;
        return it->second;
    }

    int ConnectNetwork(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "ConnectNetwork的参数不足或太多");
        string ip = luaL_checkstring(L, 1);
        int port = luaL_checkinteger(L, 2);
        if (!CheckNetworkPermission())
        {
            ::PrintLog("[Script] 网络权限被拒绝，连接失败", PL_WARNING);
            lua_pushinteger(L, -1);
            return 1;
        }
        if (!EnsureWsaInit())
        {
            ::PrintLog("[Script] WSA初始化失败", PL_ERROR);
            lua_pushinteger(L, -1);
            return 1;
        }
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET)
        {
            ::PrintLog("[Script] 创建socket失败", PL_ERROR);
            lua_pushinteger(L, -1);
            return 1;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
        {
            ::PrintLog("[Script] IP地址无效：" + ip, PL_ERROR);
            closesocket(s);
            lua_pushinteger(L, -1);
            return 1;
        }
        if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            ::PrintLog("[Script] 连接失败：" + ip, PL_ERROR);
            closesocket(s);
            lua_pushinteger(L, -1);
            return 1;
        }
        const int id = NextSocketId++;
        {
            lock_guard<mutex> lock(ScriptSocketsMtx);
            ScriptSockets[CurrentScriptFile][id] = s;
        }
        lua_pushinteger(L, id);
        return 1;
    }

    int RecvNetwork(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "RecvNetwork的参数不足或太多");
        int id = luaL_checkinteger(L, 1);
        if (!CheckNetworkPermission())
        {
            lua_pushstring(L, "");
            return 1;
        }
        SOCKET s = FindScriptSocket(id);
        if (s == INVALID_SOCKET)
        {
            lua_pushstring(L, "");
            return 1;
        }
        char buffer[MAX_BUF];
        int len = recv(s, buffer, MAX_BUF - 1, 0);
        if (len <= 0)
        {
            lua_pushstring(L, "");
            return 1;
        }
        buffer[len] = '\0';
        lua_pushstring(L, buffer);
        return 1;
    }

    int SendNetwork(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "SendNetwork的参数不足或太多");
        int id = luaL_checkinteger(L, 1);
        string data = luaL_checkstring(L, 2);
        if (!CheckNetworkPermission())
        {
            lua_pushinteger(L, -1);
            return 1;
        }
        SOCKET s = FindScriptSocket(id);
        if (s == INVALID_SOCKET)
        {
            lua_pushinteger(L, -1);
            return 1;
        }
        int ret = send(s, data.c_str(), (int)data.size(), 0);
        lua_pushinteger(L, (ret == SOCKET_ERROR) ? -1 : 0);
        return 1;
    }

    int CloseNetwork(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "CloseNetwork的参数不足或太多");
        int id = luaL_checkinteger(L, 1);
        if (!CheckNetworkPermission())
        {
            lua_pushinteger(L, -1);
            return 1;
        }
        {
            lock_guard<mutex> lock(ScriptSocketsMtx);
            auto outer = ScriptSockets.find(CurrentScriptFile);
            if (outer == ScriptSockets.end())
            {
                lua_pushinteger(L, -1);
                return 1;
            }
            auto it = outer->second.find(id);
            if (it == outer->second.end())
            {
                lua_pushinteger(L, -1);
                return 1;
            }
            closesocket(it->second);
            outer->second.erase(it);
        }
        lua_pushinteger(L, 1);
        return 1;
    }

    int GetScreenSize(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetScreenWidthSize的参数太多");
        lua_pushinteger(L, GetSystemMetrics(SM_CXSCREEN));
        lua_pushinteger(L, GetSystemMetrics(SM_CYSCREEN));
        return 2;
    }

    int RenderMessageBox(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "RenderMessageBox的参数不足或太多");
        string title = StringToUTF8(luaL_checkstring(L, 1));
        string msg = StringToUTF8(luaL_checkstring(L, 2));
        int type = luaL_checkinteger(L, 3);
        int result = ::RenderMessageBox(title.c_str(), msg.c_str(), type);
        lua_pushinteger(L, result);
        return 1;
    }

    int RenderPopTip(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "RenderPopTip的参数不足或太多");
        string msg = StringToUTF8(luaL_checkstring(L, 1));
        int timeout = luaL_checkinteger(L, 2);
        int type = luaL_checkinteger(L, 3);
        ::RenderTip(msg.c_str(), timeout, type);
        return 0;
    }

    int GetClipboardText(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetClipboardText的参数太多");
        char* buffer = new char[MAX_BUF];
        
        if (OpenClipboard(NULL))
        {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData)
                strcpy_s(buffer, MAX_BUF, (char*)GlobalLock(hData));
            CloseClipboard();
        }
        else
        {
            lua_pushstring(L, "");
            return 1;
        }

        if (strlen(buffer) != 0)
        {
            string str = UTF8ToString(buffer);
            delete[] buffer;
            lua_pushstring(L, str.c_str());
        }
        else
            lua_pushstring(L, "");
        return 1;
    }

    int SetClipboardText(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "SetClipboardText的参数不足或太多");

        string text = StringToUTF8(luaL_checkstring(L, 1));

        if (OpenClipboard(NULL))
        {
            EmptyClipboard();
            size_t len = text.size();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
            if (!hMem)
            {
                CloseClipboard();
                lua_pushboolean(L, false);
                return 1;
            }
            // 锁定并复制文本，使用utf写法
            wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
            memcpy(pMem, text.c_str(), len);
            GlobalUnlock(hMem);
            // 放入剪贴板
            SetClipboardData(CF_UNICODETEXT, hMem);
            CloseClipboard();
        }
        else
        {
            lua_pushboolean(L, false);
            return 1;
        }

        lua_pushboolean(L, true);
        return 1;
    }

    int IsScriptKeyDown(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "IsScriptKeyDown的参数太多");

        // 修复：不再遍历 LuaStates（避免 RunningMtx 内获取 LuaStatesMtx 形成死锁环），
        // 用 thread_local 的 CurrentScriptFile 直接定位；find 替代 operator[]（避免读锁下插入/复活僵尸条目）
        if (CurrentScriptFile.empty())
        {
            lua_pushboolean(L, false);
            return 1;
        }
        // 读取锁
        shared_lock<shared_mutex> lock(ScriptData::StatesMtx);
        auto st = ScriptData::States.find(CurrentScriptFile);
        lua_pushboolean(L, st != ScriptData::States.end() ? st->second.IsPressedKey : false);
        return 1;
    }

    int IsScriptKeyUp(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "IsScriptKeyUp的参数太多");

        if (CurrentScriptFile.empty())
        {
            lua_pushboolean(L, false);
            return 1;
        }
        // 写入锁
        lock_guard<shared_mutex> lock(ScriptData::StatesMtx);
        auto st = ScriptData::States.find(CurrentScriptFile);
        if (st == ScriptData::States.end())
        {
            lua_pushboolean(L, false);
            return 1;
        }
        lua_pushboolean(L, st->second.IsReleaseKey);
        st->second.IsReleaseKey = false;
        return 1;
    }

    int IsScriptKeyClick(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "IsScriptKeyClick的参数太多");

        if (CurrentScriptFile.empty())
        {
            lua_pushboolean(L, false);
            return 1;
        }
        // 写入锁
        lock_guard<shared_mutex> lock(ScriptData::StatesMtx);
        auto st = ScriptData::States.find(CurrentScriptFile);
        if (st == ScriptData::States.end())
        {
            lua_pushboolean(L, false);
            return 1;
        }
        lua_pushboolean(L, st->second.IsClickedKey);
        st->second.IsClickedKey = false;
        return 1;
    }

    int MessageBeep(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "MessageBeep的参数不足或太多");
        int type = luaL_checkinteger(L, 1);
        int param = MB_ICONINFORMATION;
        if (type == 3)
            param = MB_ICONERROR;
        else if (type == 2)
            param = MB_ICONWARNING;

        ::MessageBeep(param);
        return 0;
    }

    int SetScriptLoopDelay(lua_State* L)
    {
        int ParamNumber = lua_gettop(L);
        if (ParamNumber != 1 && ParamNumber != 0)
            luaL_error(L, "SetScriptLoopDelay的参数太多");
        int delay = 5;
        if (ParamNumber == 1)
            delay = luaL_checkinteger(L, 1);

        // 修复：同 IsScriptKeyDown，用 thread_local CurrentScriptFile 定位，避免死锁环与 LuaStates 遍历竞争
        if (CurrentScriptFile.empty())
            return 0;
        // 写入锁
        lock_guard<shared_mutex> lock(ScriptData::StatesMtx);
        auto st = ScriptData::States.find(CurrentScriptFile);
        if (st != ScriptData::States.end())
            st->second.LoopDelay = delay;
        return 0;
    }

    int IsOpenGui(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "IsOpenGui的参数太多");
        lua_pushboolean(L, ::IsOpenGui);
        return 1;
    }

    int IsPlayerMoving(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "IsPlayerMoving的参数太多");
        lua_pushboolean(L, ::IsMoving);
        return 1;
    }

    int PlayEnableSound(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "PlayEnableSound的参数太多");
        ::PlayEnableSound();
        return 0;
    }

    int PlayDisableSound(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "PlayDisableSound的参数太多");
        ::PlayDisableSound();
        return 0;
    }

    int PlayToggleSound(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "PlayToggleSound的参数不足或太多");
        bool enable = lua_toboolean(L, 1);
        if (enable)
            ::PlayEnableSound();
        else
            ::PlayDisableSound();
        return 0;
    }

    int GetCurrentWindowPos(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCurrentWindowPos的太多");
        HWND hwnd = GetForegroundWindow();
        RECT rect;
        GetWindowRect(hwnd , &rect);
        lua_pushnumber(L, rect.left);
        lua_pushnumber(L, rect.top);
        return 2;
    }

    int GetCurrentWindowRect(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCurrentWindowPos的太多");
        HWND hwnd = GetForegroundWindow();
        RECT rect;
        GetWindowRect(hwnd, &rect);
        lua_pushnumber(L, rect.left);
        lua_pushnumber(L, rect.top);
        lua_pushnumber(L, rect.right);
        lua_pushnumber(L, rect.bottom);
        return 4;
    }

    int GetCurrentWindowSize(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCurrentWindowSize的参数太多");
        HWND hwnd = GetForegroundWindow();
        RECT rect;
        GetWindowRect(hwnd, &rect);
        lua_pushinteger(L, rect.right - rect.left);
        lua_pushinteger(L, rect.bottom - rect.top);
        return 2;
    }

    int MoveCursor(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "MoveCursor的参数不足或太多");
        int dx = luaL_checkinteger(L, 1);
        int dy = luaL_checkinteger(L, 2);
        POINT pos;
        ::GetCursorPos(&pos);
        ::SetCursorPos(pos.x + dx, pos.y + dy);
        return 0;
    }

    int MoveCursorX(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "MoveCursorX的参数不足或太多");
        int dx = luaL_checkinteger(L, 1);
        POINT pos;
        ::GetCursorPos(&pos);
        ::SetCursorPos(pos.x + dx, pos.y);
        return 0;
    }

    int MoveCursorY(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "MoveCursorY的参数不足或太多");
        int dy = luaL_checkinteger(L, 1);
        POINT pos;
        ::GetCursorPos(&pos);
        ::SetCursorPos(pos.x, pos.y + dy);
        return 0;
    }

    int GetItemBarKey(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "GetItemBarKey的参数不足或太多");
        int index = luaL_checkinteger(L, 1);
        if (index < 0 || index > 8)
            luaL_error(L, "GetItemBarKey的index必须在0到8的范围");
        lua_pushinteger(L, config.ItemBarKey[index]);
        return 1;
    }

    int GetWindowsCenterPos(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetWindowsCenterPos的参数太多");
        POINT center = GetGameWindowCenter(GetForegroundWindow());
        lua_pushinteger(L, center.x);
        lua_pushinteger(L, center.y);
        return 2;
    }

    //--------------------GUI的API------------------------------

    int IsCloseUI(lua_State* L)
    {
        luaL_error(L, "IsCloseUI函数在新版本已被弃用！");
        return 1;
    }

    int Button(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "Button的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        int x = luaL_checkinteger(L, 2);
        int y = luaL_checkinteger(L, 3);
        bool result = ImGui::Button(label.c_str(), { float(x), float(y) });
        lua_pushboolean(L, result);
        return 1;
    }

    int Checkbox(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "Checkbox的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        bool checked = lua_toboolean(L, 2);
        ImGui::Checkbox(label.c_str(), &checked);
        lua_pushboolean(L, checked);
        return 1;
    }

    int Toggle(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "Checkbox的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        bool checked = lua_toboolean(L, 2);
        ImGui::Toggle(label.c_str(), &checked, ToggleFlags);
        lua_pushboolean(L, checked);
        return 1;
    }

    int Text(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "Text的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        ImGui::Text(label.c_str());
        return 0;
    }

    // InputText的静态回调，用于调整 string 的大小
    static int InputTextCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            // UserData 存储了 string* 指针
            string* str = static_cast<string*>(data->UserData);
            // 调整 string 大小（data->BufSize 包含末尾的 '\0'）
            str->resize(data->BufSize);
            // 更新缓冲区指针，指向 string 内部数据区
            data->Buf = &(*str)[0];
        }
        return 0;
    }
    int InputText(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "InputText 需要两个参数");

        // 将 Lua 字符串转为 UTF8（假设 Lua 内部已为 UTF8，此处仅为保险）
        string label = StringToUTF8(luaL_checkstring(L, 1));
        string text = StringToUTF8(luaL_checkstring(L, 2));

        // 确保 text 至少有一个字符（存放 '\0'），以便 &text[0] 合法
        if (text.empty())
            text.resize(1);

        // 预分配足够的初始容量（可根据需求调整，例如 256 或 1024）
        text.reserve(256);

        // 调用 ImGui::InputText，传入可写指针、当前容量+1（作为缓冲区大小）
        // 并绑定回调，UserData 传入 text 的地址
        ImGui::InputText(
            label.c_str(),
            &text[0],                               // 可写指针
            static_cast<int>(text.capacity()) + 1,  // 缓冲区大小（包括 '\0'）
            ImGuiInputTextFlags_CallbackResize,     // 启用调整回调
            InputTextCallback,
            &text                                   // 传递给回调的 UserData
        );

        // 将编辑后的 UTF8
        lua_pushstring(L, UTF8ToString(text).c_str());
        return 1;
    }

    int SliderInt(lua_State* L)
    {
        if (lua_gettop(L) != 5)
            luaL_error(L, "SliderInt的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        int value = luaL_checkinteger(L, 2);
        int min = luaL_checkinteger(L, 3);
        int max = luaL_checkinteger(L, 4);
        string format = StringToUTF8(luaL_checkstring(L, 5));
        ImGui::SliderInt(label.c_str(), &value, min, max, format.c_str());
        lua_pushinteger(L, value);
        return 1;
    }

    int SliderFloat(lua_State* L)
    {
        if (lua_gettop(L) != 5)
            luaL_error(L, "SliderFloat的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        float value = luaL_checkinteger(L, 2);
        float min = luaL_checknumber(L, 3);
        float max = luaL_checknumber(L, 4);
        string format = StringToUTF8(luaL_checkstring(L, 5));
        ImGui::SliderFloat(label.c_str(), &value, min, max, format.c_str());
        lua_pushnumber(L, (double)value);
        return 1;
    }

    int RangeSliderInt(lua_State* L)
    {
        if (lua_gettop(L) != 6)
            luaL_error(L, "RangeSliderInt的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        int low = luaL_checkinteger(L, 2);
        int high = luaL_checkinteger(L, 3);
        int min = luaL_checkinteger(L, 4);
        int max = luaL_checkinteger(L, 5);
        string format = StringToUTF8(luaL_checkstring(L, 6));
        ImGui::RangeSliderInt(label.c_str(), min, max, low, high, format.c_str());
        lua_pushinteger(L, low);
        lua_pushinteger(L, high);
        return 2;
    }

    int RangeSliderFloat(lua_State* L)
    {
        if (lua_gettop(L) != 6)
            luaL_error(L, "RangeSliderFloat的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        float low = luaL_checknumber(L, 2);
        float high = luaL_checknumber(L, 3);
        float min = luaL_checknumber(L, 4);
        float max = luaL_checknumber(L, 5);
        string format = StringToUTF8(luaL_checkstring(L, 6));
        ImGui::RangeSliderFloat(label.c_str(), min, max, low, high, format.c_str());
        lua_pushnumber(L, (double)low);
        lua_pushnumber(L, (double)high);
        return 2;
    }

    int Combo(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "Combo的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        int current = luaL_checkinteger(L, 2);
        if (lua_type(L, 3) != LUA_TTABLE)
            luaL_error(L, "Combo的第3个参数必须是一个table数组");
        // 依次读取table数组中的每个字符串选项
        vector<string> items;
        const int count = (int)lua_objlen(L, 3);
        if (count <= 0)
            luaL_error(L, "Combo的选项列表不能为空");
        for (int i = 1; i <= count; i++)
        {
            lua_rawgeti(L, 3, i);
            if (lua_type(L, -1) != LUA_TSTRING)
                luaL_error(L, "Combo的选项必须是字符串");
            items.push_back(StringToUTF8(lua_tostring(L, -1)));
            lua_pop(L, 1);
        }
        // 构造 ImGui 需要的 const char* 指针数组
        vector<const char*> item_ptrs;
        item_ptrs.reserve(items.size());
        for (const string& item : items)
            item_ptrs.push_back(item.c_str());
        // 修正当前选中项越界
        if (current < 0)
            current = 0;
        if (current >= (int)items.size())
            current = (int)items.size() - 1;
        ImGui::Combo(label.c_str(), &current, item_ptrs.data(), (int)item_ptrs.size());
        lua_pushinteger(L, current);
        return 1;
    }

    int SameLine(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "SameLine的参数太多");
        ImGui::SameLine();
        return 0;
    }

    int InputKey(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "InputKey的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        int key = luaL_checkinteger(L, 2);
        ::InputKey(label.c_str(), &key, config.UseChinese ? u8"请按下按键" : "Press a key");
        lua_pushinteger(L, key);
        return 1;
    }
    
    int DrawSpiltLine(lua_State* L)
    {
        int gt = lua_gettop(L);
        if (gt != 1 && gt != 0)
            luaL_error(L, "DrawSpiltLine的参数太多");
        const int len = (gt == 1) ? luaL_checkinteger(L, 1) : 600;
        ::DrawSpiltLine(len, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
        return 0;
    }

    int AttachTooltip(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "AttachTooltip的参数不足或太多");
        string msg = StringToUTF8(luaL_checkstring(L, 1));
        const int delay = luaL_checkinteger(L, 2);
        ::AttachTooltip(msg.c_str(), delay);
        return 0;
    }

    int MessageBox(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "MessageBox的参数不足或太多（只能3或4个）");
        string msg = StringToUTF8(luaL_checkstring(L, 1));
        string title = StringToUTF8(luaL_checkstring(L, 2));
        int type = luaL_checkinteger(L, 3);

        if (type != MB_OK && type != MB_OKCANCEL)
            luaL_error(L, "MessageBox的第3个参数不正确，只能为0(OK)和1(OKCANCEL)");

        int result = ::MessageBox(msg.c_str(), title.c_str(), type);
        lua_pushinteger(L, result);
        return 1;
    }

    int RadioButton(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "RadioButton的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        const int button = luaL_checkinteger(L, 2);
        int v = luaL_checkinteger(L, 3);
        ImGui::RadioButton(label.c_str(), &v, button);
        lua_pushinteger(L, v);
        return 1;
    }

    int SetNextItemPos(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "SetNextItemPos的参数不足或太多");
        int x = luaL_checkinteger(L, 1);
        int y = luaL_checkinteger(L, 2);
        if (x < 0 && y >= 0)
            ImGui::SetCursorPosY(y);
        else if (y < 0 && x >= 0)
            ImGui::SetCursorPosX(x);
        else if (y >= 0 && x >= 0)
            ImGui::SetCursorPos(ImVec2(x, y));
        return 0;
    }

    int GetNextItemPos(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetNextItemPos的参数太多");
        ImVec2 pos = ImGui::GetCursorPos();
        lua_pushinteger(L, pos.x);
        lua_pushinteger(L, pos.y);
        return 2;
    }

    int SetNextItemWidth(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "SetNextWindowSize的参数不足或太多");
        int w = luaL_checkinteger(L, 1);
        ImGui::SetNextItemWidth(w);
        return 0;
    }

    int BeginChild(lua_State* L)
    {
        int count = lua_gettop(L);
        if (count != 3 && count != 4)
            luaL_error(L, "BeginChild的参数不足或太多");
        string name = StringToUTF8(luaL_checkstring(L, 1));
        const float w = (float)luaL_checknumber(L, 2);
        const float h = (float)luaL_checknumber(L, 3);
        const bool border = (count == 4) ? lua_toboolean(L, 4) : false;
        const bool result = ImGui::BeginChild(name.c_str(), ImVec2(w, h), border);
        lua_pushboolean(L, result);
        return 1;
    }

    int EndChild(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "EndChild的参数太多");
        ImGui::EndChild();
        return 0;
    }

    int BeginGroup(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "BeginGroup的参数太多");
        ImGui::BeginGroup();
        return 0;
    }

    int EndGroup(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "EndGroup的参数太多");
        ImGui::EndGroup();
        return 0;
    }

    static bool IsDrawOverAll = false; // Draw的绘画是否覆盖所有控件
    ImVec4 CurentColor = ImVec4(255, 255, 255, 255); // 当前颜色
    int SetCurentColor(lua_State* L)
    {
        if (lua_gettop(L) != 4)
            luaL_error(L, "SetCurentColor的参数不足或太多");
        int r = luaL_checkinteger(L, 1);
        int g = luaL_checkinteger(L, 2);
        int b = luaL_checkinteger(L, 3);
        int a = luaL_checkinteger(L, 4);
        CurentColor = ImVec4(r, g, b, a);
        return 0;
    }

    int SetDrawOverAll(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "SetDrawOverAll的参数不足或太多");
        IsDrawOverAll = lua_toboolean(L, 1);
        return 0;
    }

    int DrawLine(lua_State* L)
    {
        if (lua_gettop(L) != 5)
            luaL_error(L, "DrawLine的参数不足或太多");
        int x1 = luaL_checkinteger(L, 1);
        int y1 = luaL_checkinteger(L, 2);
        int x2 = luaL_checkinteger(L, 3);
        int y2 = luaL_checkinteger(L, 4);
        float thickness = luaL_checknumber(L, 5);

        ImDrawList* draw_list = IsDrawOverAll ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();
        draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImColor(CurentColor), thickness);
        return 0;
    }

    int DrawRect(lua_State* L)
    {
        if (lua_gettop(L) != 5)
            luaL_error(L, "DrawRect的参数不足或太多");
        int x1 = luaL_checkinteger(L, 1);
        int y1 = luaL_checkinteger(L, 2);
        int x2 = luaL_checkinteger(L, 3);
        int y2 = luaL_checkinteger(L, 4);
        float rounding = luaL_checknumber(L, 5);

        ImDrawList* draw_list = IsDrawOverAll ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();
        draw_list->AddRect(ImVec2(x1, y1), ImVec2(x2, y2), ImColor(CurentColor), rounding);
        return 0;
    }

    int DrawRectFilled(lua_State* L)
    {
        if (lua_gettop(L) != 5)
            luaL_error(L, "DrawRectFilled的参数不足或太多");
        int x1 = luaL_checkinteger(L, 1);
        int y1 = luaL_checkinteger(L, 2);
        int x2 = luaL_checkinteger(L, 3);
        int y2 = luaL_checkinteger(L, 4);
        float rounding = luaL_checknumber(L, 5);

        ImDrawList* draw_list = IsDrawOverAll ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImColor(CurentColor), rounding);
        return 0;
    }

    int DrawCircle(lua_State* L)
    {
        if (lua_gettop(L) != 4)
            luaL_error(L, "DrawCircle的参数不足或太多");
        int x = luaL_checkinteger(L, 1);
        int y = luaL_checkinteger(L, 2);
        int radius = luaL_checkinteger(L, 3);
        float thickness = luaL_checknumber(L, 4);

        ImDrawList* draw_list = IsDrawOverAll ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();
        draw_list->AddCircle(ImVec2(x, y), radius, ImColor(CurentColor), thickness);
        return 0;
    }

    int DrawCircleFilled(lua_State* L)
    {
        if (lua_gettop(L) != 4)
            luaL_error(L, "DrawCircleFilled的参数不足或太多");
        int x = luaL_checkinteger(L, 1);
        int y = luaL_checkinteger(L, 2);
        int radius = luaL_checkinteger(L, 3);

        ImDrawList* draw_list = IsDrawOverAll ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();
        draw_list->AddCircleFilled(ImVec2(x, y), radius, ImColor(CurentColor));
        return 0;
    }

    int DrawText(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "DrawText的参数不足或太多");
        int x = luaL_checkinteger(L, 1);
        int y = luaL_checkinteger(L, 2);
        string text = StringToUTF8(luaL_checkstring(L, 3));

        ImDrawList* draw_list = IsDrawOverAll ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();
        draw_list->AddText(ImVec2(x, y), ImColor(CurentColor), text.c_str());
        return 0;
    }

    int DrawImage(lua_State* L)
    {
        if (lua_gettop(L) != 5)
            luaL_error(L, "DrawImage的参数不足或太多");
        int x = luaL_checkinteger(L, 1);
        int y = luaL_checkinteger(L, 2);
        int w = luaL_checkinteger(L, 3);
        int h = luaL_checkinteger(L, 4);
        string file = luaL_checkstring(L, 5);
       
        ImDrawList* draw_list = IsDrawOverAll ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();
        Texture2D texture_id = LoadTextureFromFile(file);
        draw_list->AddImage(texture_id, ImVec2(x, y), ImVec2(x + w, y + h));
        return 0;
    }
}