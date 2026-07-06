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

typedef struct LuaState
{
    lua_State* lua;
    mutex Mtx;
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

bool SimulatedInputText(HWND hwnd, const string& text)
{
    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        // 为多字节文本分配内存
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
        if (hMem)
        {
            char* pMem = static_cast<char*>(GlobalLock(hMem));
            if (pMem)
            {
                // 使用 strcpy_s 复制多字节字符串
                strcpy_s(pMem, text.length() + 1, text.c_str());
                GlobalUnlock(pMem);

                // 设置为 CF_TEXT 格式
                SetClipboardData(CF_TEXT, hMem);
            }
        }
        CloseClipboard();
    }
    else
        return false;

    INPUT input[2]{};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_CONTROL;
    input[0].ki.dwFlags = 0;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 'V';
    input[1].ki.dwFlags = 0;
    SendInput(2, input, sizeof(INPUT));
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_CONTROL;
    input[0].ki.dwFlags = KEYEVENTF_KEYUP;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 'V';
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, input, sizeof(INPUT));

    return true;
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
        if (lua_gettop(L) != 2)
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
        WritePrivateProfileString(app, key, value.c_str(), IniPos.c_str()); 
        return 0;
    }

    int GetClickerSettings(lua_State* L)
    {
        if (lua_gettop(L) != 2)
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
        GetPrivateProfileString(app, key, value.c_str(), buffer, MAX_BUF, IniPos.c_str());
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
        int mode = (count >= 3) ? luaL_checkinteger(L, 3) : SC_SEND_INPUT;
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
        int mode = (count >= 3) ? luaL_checkinteger(L, 3) : SC_SEND_INPUT;
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
        int mode = (count >= 3) ? luaL_checkinteger(L, 3) : SC_SEND_INPUT;
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

    int GetMousePos(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetMousePos的参数太多");
        POINT pos;
        ::GetCursorPos(&pos);
        lua_pushinteger(L, pos.x);
        lua_pushinteger(L, pos.y);
        return 2;
    }

    int SetMousePos(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "SetMousePos的参数不足或太多");
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
        lua_pushinteger(L, CurrentCPS.right);
        return 1;
    }

    int GetCpsLeft(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCpsLeft的参数太多");
        lua_pushinteger(L, CurrentCPS.left);
        return 1;
    }

    int GetHumanCpsLeft(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetHumanCpsLeft的参数太多");
        lua_pushinteger(L, HumanCPS.left);
        return 1;
    }

    int GetHumanCpsRight(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetHumanCpsRight的参数太多");
        lua_pushinteger(L, HumanCPS.right);
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

        ::SetKeyClick(config.ItemEnableClickerKey[pos], sleep, mode);
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

    static map<int, HANDLE> PipeList;
    int ConnectNamedPipe(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "ConnectNamedPipe的参数不足或太多");
        const char* pipe_name = luaL_checkstring(L, 1);

        if (!WaitNamedPipe(pipe_name, NMPWAIT_WAIT_FOREVER))
        {
            ::PrintLog("[Script] 连接管道失败：管道不存在", PL_ERROR);
            lua_pushinteger(L, -1);
            return 1;
        }

        HANDLE hPipe;
        hPipe = CreateFile(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            ::PrintLog("[Script] 连接管道失败：无法打开管道", PL_ERROR);
            lua_pushinteger(L, -1);
            return 1;
        }
        unsigned int id = rand() + 1;
        PipeList[id] = hPipe;
        lua_pushinteger(L, id);
        return 1;
    }

    int ReadNamedPipe(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "ReadNamedPipe的参数不足或太多");
        int id = luaL_checkinteger(L, 1);
        if (PipeList.find(id) == PipeList.end())
        {
            ::PrintLog("[Script] 读取管道失败：管道不存在", PL_ERROR);
            lua_pushstring(L, "");
            return 1;
        }
        char buffer[MAX_BUF];
        DWORD dwRead;
        if (!ReadFile(PipeList[id], buffer, MAX_BUF, &dwRead, NULL))
        {
            ::PrintLog("[Script] 读取管道失败：读取失败", PL_ERROR);
            lua_pushstring(L, "");
            return 1;
        }
        lua_pushstring(L, buffer);
        return 1;
    }

    int WriteNamedPipe(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "WriteNamedPipe的参数不足或太多");
        int id = luaL_checkinteger(L, 1);
        if (PipeList.find(id) == PipeList.end())
        {
            ::PrintLog("[Script] 写入管道失败：管道不存在", PL_ERROR);
            lua_pushinteger(L, -1);
            return 1;
        }
        string str = luaL_checkstring(L, 2);
        try
        {
            str = StringToUTF8(str);
        }
        catch (...) {}
        if (!WriteFile(PipeList[id], str.c_str(), str.size(), NULL, NULL))
        {
            ::PrintLog("[Script] 写入管道失败：写入失败", PL_ERROR);
            lua_pushinteger(L, -1);
            return 1;
        }
        lua_pushinteger(L, 0);
        return 1;
    }

    int CloseNamedPipe(lua_State* L)
    {
        if (lua_gettop(L) != 1)
            luaL_error(L, "CloseNamedPipe的参数不足或太多");
        int id = luaL_checkinteger(L, 1);
        if (PipeList.find(id) == PipeList.end())
        {
            ::PrintLog("[Script] 关闭管道失败：管道不存在", PL_ERROR);
            lua_pushinteger(L, -1);
            return -1;
        }
        CloseHandle(PipeList[id]);
        PipeList.erase(id);
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
        
        for (const auto& it : LuaStates)
        {
            if (it.second.lua == L)
            {
                // 读取锁
                shared_lock<shared_mutex> lock(ScriptStatesMtx);
                lua_pushboolean(L, ScriptStates[it.first].IsPressedKey);
                return 1;
            }
        }
        lua_pushboolean(L, false);
        return 1;
    }

    int IsScriptKeyUp(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "IsScriptKeyUp的参数太多");

        for (const auto& it : LuaStates)
        {
            if (it.second.lua == L)
            {
                // 写入锁
                lock_guard<shared_mutex> lock(ScriptStatesMtx);
                auto& kc = ScriptStates[it.first];
                lua_pushboolean(L, kc.IsReleaseKey);
                kc.IsReleaseKey = false;
                return 1;
            }
        }
        lua_pushboolean(L, false);
        return 1;
    }

    int IsScriptKeyClick(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "IsScriptKeyClick的参数太多");

        for (const auto& it : LuaStates)
        {
            if (it.second.lua == L)
            {
                // 写入锁
                lock_guard<shared_mutex> lock(ScriptStatesMtx);
                auto& kc = ScriptStates[it.first];
                lua_pushboolean(L, kc.IsClickedKey);
                kc.IsClickedKey = false;
                return 1;
            }
        }
        lua_pushboolean(L, false);
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
        
        for (const auto& it : LuaStates)
        {
            if (it.second.lua == L)
            {
                // 写入锁
                lock_guard<shared_mutex> lock(ScriptStatesMtx);
                ScriptStates[it.first].LoopDelay = delay;
                return 0;
            }
        }
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
        return 1;
    }

    int PlayDisableSound(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "PlayDisableSound的参数太多");
        ::PlayDisableSound();
        return 1;
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
        bool checked = lua_isboolean(L, 2);
        ImGui::Checkbox(label.c_str(), &checked);
        if (!checked)
            cout << "asd" << endl;
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

    int InputText(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "InputText的参数不足或太多");
        string label = StringToUTF8(luaL_checkstring(L, 1));
        string temp = StringToUTF8(luaL_checkstring(L, 2));
        char* text = (char*)temp.c_str();
        ImGui::InputText(label.c_str(), text, 128);
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
        if (lua_gettop(L) != 0)
            luaL_error(L, "DrawSpiltLine的参数太多");
        ::DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
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

    int PopTip(lua_State* L)
    {
        if (lua_gettop(L) != 3)
            luaL_error(L, "PopTip的参数不足或太多");
        string msg = StringToUTF8(luaL_checkstring(L, 1));
        const int timeout = luaL_checkinteger(L, 2);
        const int type = luaL_checkinteger(L, 3);
        bool result = ::PopTip(msg, timeout, type);
        lua_pushboolean(L, result);
        return 1;
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

    int SetCursorPos(lua_State* L)
    {
        if (lua_gettop(L) != 2)
            luaL_error(L, "SetCursorPos的参数不足或太多");
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

    int GetCursorPos(lua_State* L)
    {
        if (lua_gettop(L) != 0)
            luaL_error(L, "GetCursorPos的参数太多");
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