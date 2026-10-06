#pragma once
#include "Definitons.h"
#include "ScriptAPI.h"
#include <memory> // make_shared/shared_ptr��StartScript �߳̾���������ڣ�

lua_State* LuaInit();
HANDLE StartScript(const string& file);
void StartInstalledScriptsOnBoot();
DWORD WINAPI ScriptKeyControlThread(LPVOID lpParam);
static int panic_handler(lua_State* L);
bool RenderScriptGui(const string& file);
int SaveScriptConfig(const string& file);
int LoadScript(lua_State* L, const string& file);
void CloseScriptThread(const string& filename, ScriptInfo& info);
int CalledScriptFunction(lua_State* L, const string& func);
inline bool InitLuaState(const string& file, LuaState& lua);

class SpaceKeyCodeDefine
{
public:
    unordered_map<string, int> KeyMap;
    SpaceKeyCodeDefine()
    {
        // ���Ƽ�
        KeyMap["VK_RETURN"] = 0x0D;      // Enter
        KeyMap["VK_ESCAPE"] = 0x1B;      // Esc
        KeyMap["VK_SPACE"] = 0x20;       // �ո�
        KeyMap["VK_BACK"] = 0x08;        // Backspace
        KeyMap["VK_TAB"] = 0x09;         // Tab
        KeyMap["VK_CAPITAL"] = 0x14;     // Caps Lock

        // ���μ�
        KeyMap["VK_SHIFT"] = 0x10;
        KeyMap["VK_CONTROL"] = 0x11;     // Ctrl
        KeyMap["VK_MENU"] = 0x12;        // Alt
        KeyMap["VK_LSHIFT"] = 0xA0;      // ��Shift
        KeyMap["VK_RSHIFT"] = 0xA1;      // ��Shift
        KeyMap["VK_LCONTROL"] = 0xA2;    // ��Ctrl
        KeyMap["VK_RCONTROL"] = 0xA3;    // ��Ctrl
        KeyMap["VK_LMENU"] = 0xA4;       // ��Alt
        KeyMap["VK_RMENU"] = 0xA5;       // ��Alt
        KeyMap["VK_LWIN"] = 0x5B;        // ��Windows
        KeyMap["VK_RWIN"] = 0x5C;        // ��Windows

        // �����
        KeyMap["VK_LEFT"] = 0x25;        // ��
        KeyMap["VK_UP"] = 0x26;          // ��
        KeyMap["VK_RIGHT"] = 0x27;       // ��
        KeyMap["VK_DOWN"] = 0x28;        // ��

        // �༭��
        KeyMap["VK_INSERT"] = 0x2D;      // Insert
        KeyMap["VK_DELETE"] = 0x2E;      // Delete
        KeyMap["VK_HOME"] = 0x24;        // Home
        KeyMap["VK_END"] = 0x23;         // End
        KeyMap["VK_PRIOR"] = 0x21;       // Page Up
        KeyMap["VK_NEXT"] = 0x22;        // Page Down

        // ���ż�
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

lua_State* LuaInit()
{
    lua_State* L = luaL_newstate();

    if (L == NULL)
    {
        PrintLog("Lua��ʼ��ʧ��", PL_ERROR);
        IsGCExit = true;
        return L;
    }

    // ����Lua�Ĵ���������
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
        
        //=================================================GUI��API=================================================
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
        // ������ĸ
        for (char c = 'A'; c <= 'Z'; c++)
        {
            lua_pushinteger(L, c);
            char str[2] = { c, '\0' }; // ��ĸת��Ϊ�ַ���
            lua_setfield(L, -2, str);
        }

        // ��������
        for (int i = 0; i <= 9; i++)
        {
            lua_pushinteger(L, i);
            char str[2];
            str[0] = '0' + i;
            str[1] = '\0';
            lua_setfield(L, -2, str);
        }

        // �������ⰴ��
        SpaceKeyCodeDefine SpaceKeyMap;
        for (const auto& it : SpaceKeyMap.KeyMap)
        {
            lua_pushinteger(L, it.second);
            lua_setfield(L, -2, it.first.c_str());
        }
        lua_setglobal(L, "KEY"); // ���ӵ�KEY�У���������ͨ��KEY.Aʹ��

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
    }
    {
        // �������п�
        luaL_openlibs(L);

        // ��ȫ�Ƴ�io��
        lua_pushnil(L);
        lua_setglobal(L, "io");

        // ��ȫ�Ƴ�package��
        lua_pushnil(L);
        lua_setglobal(L, "package");

        // �Ƴ�os���Σ�պ���
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

        // �Ƴ�Σ�յ�ȫ�ֺ���
        lua_pushnil(L); lua_setglobal(L, "dofile");
        lua_pushnil(L); lua_setglobal(L, "loadfile");
        lua_pushnil(L); lua_setglobal(L, "loadstring");
        lua_pushnil(L); lua_setglobal(L, "load");
        lua_pushnil(L); lua_setglobal(L, "collectgarbage");
        lua_pushnil(L); lua_setglobal(L, "print");
        lua_pushnil(L); lua_setglobal(L, "rawhook");

        // �Ƴ�debug�⣨ȫ���Ƴ���
        lua_pushnil(L);
        lua_setglobal(L, "debug");
    }

    return L;
}

// Lua ������������������Ϊ lua_pcall �Ĵ�����������msgh�����룬
// �����ڴ���ջչ��ǰ���� luaL_traceback������ȡ�������ĵ��ö�ջ��
static int LuaErrorHandler(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    if (msg == NULL)
    {
        // ����������ַ��������� table��������ת��Ϊ�ɶ��ı�
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
            return 1;
        msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);
    return 1;
}

// ��¼һ�� Lua �ű�������д�뱨����ʷ����"������¼"ҳ����ʾ��
void ReportScriptError(const string& file, const string& message)
{
    lock_guard lock(ScriptData::ErrorRecordsMtx);
    ScriptErrorRecord record;
    record.file = file;
    record.message = message;
    record.time = FormatTime(time(0), true);
    ScriptData::ErrorRecords.push_back(record);
    if (ScriptData::ErrorRecords.size() > MAX_SCRIPT_ERROR_RECORDS)
        ScriptData::ErrorRecords.erase(ScriptData::ErrorRecords.begin());
}

// ���ش�����
static int panic_handler(lua_State* L)
{
    string msg = lua_tostring(L, -1);
    if (msg.empty())
        msg = "(no error message)";
    ::PrintLog("Lua�ű�����: " + msg, PL_ERROR);
    //IsEnableScript = false;
    msg = StringToUTF8(msg);
    RenderMessageBox(msg.c_str(), config.UseChinese ? u8"������������" : "Fatal error occurred", MB_OK);
    IsGCExit = true;
    return 0;
}

int LoadScript(lua_State* L, const string& file)
{
    bool IsError = false;
    string errmsg;
    try
    {
        PrintLog("����Lua�ű���" + file, PL_INFO);
        // �����������������֤�ڴ���ջչ��ǰ������ö�ջ
        lua_pushcfunction(L, LuaErrorHandler);
        const int errfunc = lua_gettop(L);
        int status = luaL_loadfile(L, file.c_str());
        if (status == LUA_OK)
            status = lua_pcall(L, 0, 0, errfunc);
        if (status != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            const string errText = err ? err : "(error object is not a string)";
            lua_settop(L, errfunc - 1);   // ͬʱ�Ƴ�������������������
            PrintLog("Lua�ű����ش���" + errText, PL_ERROR);
            RenderTip((config.UseChinese ? u8"�ű�ִ�д���"s : "Script execution error"s) + StringToUTF8(errText), 10.0f, PT_ERROR);
            MessageBeep(MB_ICONERROR);
            ReportScriptError(file, errText);
            return 3;
        }
        lua_settop(L, errfunc - 1);   // �ɹ�ʱ�Ƴ�����������
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
        PrintLog("����Lua�ű�ʧ�ܣ�" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"����Lua�ű�ʧ�ܣ�"s + errmsg : "Failed to load Lua script: "s + errmsg), 10.0f, PT_ERROR);
        MessageBeep(MB_ICONERROR);
        return 1;
    }
    return 0;
}

int CalledScriptFunction(lua_State* L, const string& func)
{
    bool IsError = false;
    string errmsg;
    try
    {
        // ��ѹ���������������Ϊ lua_pcall �� msgh������λ�ڱ����ú���֮�£�
        lua_pushcfunction(L, LuaErrorHandler);
        const int errfunc = lua_gettop(L);

        lua_getglobal(L, func.c_str());
        if (!lua_isfunction(L, -1))
        {
            PrintLog("�޷��ҵ�Lua�ű�������" + func, PL_ERROR);
            MessageBeep(MB_ICONERROR);
            lua_settop(L, errfunc - 1);   // �Ƴ�����������
            return 1;
        }
        if (lua_pcall(L, 0, 0, errfunc) != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            const string errText = err ? err : "(error object is not a string)";
            lua_settop(L, errfunc - 1);   // ͬʱ�Ƴ�������������������
            PrintLog("Lua�ű�ִ�д���" + errText, PL_ERROR);

            const string errUtf8 = StringToUTF8(errText);   // ���е��������������
            RenderTip((config.UseChinese ? u8"�ű�ִ�д���"s : "Script execution error"s) + errUtf8, 10.0f, PT_ERROR);
            MessageBeep(MB_ICONERROR);
            ReportScriptError(CurrentScriptFile, errText);
            return 2;
        }
        lua_settop(L, errfunc - 1);   // �ɹ�ʱ�Ƴ�����������
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
        PrintLog("����Lua�ű�����ʧ�ܣ�" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"����Lua�ű�����ʧ�ܣ�"s + errmsg : "Failed to call Lua script function: "s + errmsg), 10.0f, PT_ERROR);
        MessageBeep(MB_ICONERROR);
        return 1;
    }
    return 0;
}

// ����Ƿ��ʼ����û�о��Զ���ʼ��
inline bool InitLuaState(const string& file, LuaState& LuaInfo)
{
    bool IsError = false;
    string errmsg;
    {
        // д����
        lock_guard lock(LuaInfo.Mtx);
        try
        {
            if (LuaInfo.lua == NULL)
                LuaInfo.lua = LuaInit();
            if (!LuaInfo.IsLoaded)
            {
                int ret = LoadScript(LuaInfo.lua, file);
                if (ret != 0) return false;
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
        PrintLog("��ʼ��Lua����ʧ�ܣ�" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"��ʼ��Lua����ʧ�ܣ�"s + errmsg : "Failed to initialize Lua environment: "s + errmsg), 10.0f, PT_ERROR);
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
        // �޸���ԭ�����ڶ����ڱ��� map �������� modifications�������ͷź�����д����������Щ��������
        // �ڼ������̣߳��� CloseScriptThread������ erase Ԫ�� �� ������ʧЧ �� 0xC0000005��
        // ��Ϊ���� key��string����д�������� find ���������ԡ�
        vector<pair<string, int>> modifications;
        {
            // ��ȡ��
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
        } // read_lock �������Զ��ͷ�

        if (!modifications.empty())
        {
            lock_guard write_lock(ScriptData::StatesMtx);
            for (const auto& mod : modifications)
            {
                auto it = ScriptData::States.find(mod.first);
                if (it == ScriptData::States.end())
                    continue; // �ű��ѱ��ر�/ɾ��������
                // mod.second ��Ҫ���õ�ֵ
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
    PrintLog("[Script] �����ű��̣߳�" + file, PL_INFO);
    
    auto thread_func = [](std::atomic<HANDLE>* hThread, const string& file)
    {
        // ��ǵ�ǰ�ű��̵߳� file���� ScriptAPI ��λ�Լ��� ScriptState��������� LuaStates��������������
        CurrentScriptFile = file;
        // �ű��߳����б�ǣ���"����"�����жϣ��������˳�·����ʧ��/����/��������Ҫ��λ
        auto MarkNotRunning = [&file]()
        {
            lock_guard lock(ScriptData::StatesMtx);
            auto it = ScriptData::States.find(file);
            if (it != ScriptData::States.end())
                it->second.IsRunning = false;
        };
        // ȷ�� ScriptData::States ��Ŀ���ڣ�ԭ������ѭ��ͷ ScriptData::States[file]��operator[]����ʽ������
        // �ĳ� find �������ʽ�����������°�װ�Ľű��״����л��� find ʧ�ܶ������˳���
        {
            lock_guard lock(ScriptData::StatesMtx);
            ScriptData::States[file].IsRunning = true;
        }
        bool IsError = false;
        string errmsg;
        LuaState* lua = nullptr;
        {
            // д�������ű��̵߳�"������"��ɫ������ operator[] �ɰ�ȫ���루CloseScriptThread ��ȱ��߳��˳��� erase��
            lock_guard lock(LuaStatesMtx);
            lua = &LuaStates[file];
            if (!lua->lua)
            {
                // �޸�����ʼ��Ҳ������ RunningMtx �ڡ���RenderScriptGui/SaveScriptConfig �����������߳�
                // �� RunningMtx ���� render/saveConfig�����˴����� RunningMtx�������̻߳��ͬһ lua_State
                // ͬʱ���� Lua API��Lua ���̰߳�ȫ���� ״̬��/������
                // ��˳��ȫ��ͳһ��LuaStatesMtx �� RunningMtx �� Mtx��
                lock_guard run_lock(lua->RunningMtx);
                if (!InitLuaState(file, *lua))
                {
                    hThread->store(nullptr);
                    MarkNotRunning();
                    return;
                }
            }
        }
        // �����Ƿ��һ�����У�����Ҫ�����߳̾��
        {
            HANDLE hRealHandle = NULL;
            HANDLE hPseudoHandle = GetCurrentThread();
            if (!DuplicateHandle(GetCurrentProcess(), hPseudoHandle, GetCurrentProcess(), &hRealHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
            {
                PrintLog("[Script] �����߳̾��ʧ��(Error ID:1)", PL_ERROR);
                hThread->store(nullptr);
                MarkNotRunning();
                return;
            }
            hThread->store(hRealHandle);
        }

        {
            lock_guard run_lock(lua->RunningMtx);
            CalledScriptFunction(lua->lua, "initScript");
        }
        // �����ű���������ʼ�ű�ѭ��
        bool IsSuspend = false;
        bool IsExit = false;
        int LoopDelay = 0;
        while (!IsGCExit)
        {
            {
                // ��ȡ��
                shared_lock lock(ScriptData::StatesMtx);
                // �޸���ԭ������ shared_lock���������µ��� ScriptData::States[file]��operator[]����
                // key ������ʱ�����Ԫ�أ�д����ȴ�ֶ������� ���ݾ�������Ϊ find��
                auto it = ScriptData::States.find(file);
                if (it == ScriptData::States.end())
                    break; // ״̬�ѱ�������ű����رգ����˳��ű�
                const ScriptState& state = it->second;
                if (state.IsExit)
                    break; // �˳��ű�
                IsSuspend = state.IsSuspend;
                LoopDelay = state.LoopDelay;
            }

            if (IsSuspend)
            {
                // ��ʱδ�����κ�������ȫ˯��
                Sleep(10);
                continue;
            }

            int ret = 0;
            {
                // д����
                lock_guard run_lock(lua->RunningMtx);
                ret = CalledScriptFunction(lua->lua, "main");
            }
            if (ret != 0)
            {
                if (file != ScriptTestFile) // ���Խű��Ͳ���
                {
                    // �ű������ˣ��� �Ѱ�װ �У�ɾ����ǰ�ű�
                    // д����
                    lock_guard lock(ScriptData::InstalledMtx);
                    // �޸���range-for �� erase ��������������Ϊ����ѭ�����ҵ���ɾ����������
                    for (int i = 0; i < (int)ScriptData::Installed.size(); i++)
                    {
                        if (ScriptData::Installed[i].file == file)
                        {
                            ScriptData::Installed.erase(ScriptData::Installed.begin() + i);
                            break;
                        }
                    }
                }
                break; // �˳���
            }
            Sleep(LoopDelay);
        }
        {
            // �޸���lua_close �� render/saveConfig/main �� Lua ���ò��� �� ͬһ lua_State ˫�̷߳��ʡ�
            // release() Ҳ������ RunningMtx �ڴ��л���
            lock_guard run_lock(lua->RunningMtx);
            lua->release();
        }
        Minecraft::chat::ChatManager::instance().CloseReader(file);
        MarkNotRunning();   // �����˳�����λ���б�ǣ���"����"�����´�������
        PrintLog("[Script] �ű��߳��˳���" + file, PL_INFO);
    };
    // �޸���ԭ����Ѿֲ����� hThread �ĵ�ַ���� detach �̣߳�thread_func ��д�� *hThread����
    // �� StartScript �ȴ���ʱ���غ��̲߳�д�� �� д�����ٵ�ջ�ڴ棨����д����
    // ��Ϊ shared_ptr �ѷ����������̳߳��и�����֤�������ڡ�
    auto hThread = make_shared<std::atomic<HANDLE>>(nullptr);
    thread t([hThread, file, thread_func]() { thread_func(hThread.get(), file); });
    t.detach();

    // �ȴ��߳��������һ�õ��߳̾��
    int try_count = 20; // 50 * 20 = 1000 = 1s
    while (hThread->load() == nullptr && try_count > 0)
    {
        Sleep(50);
        try_count--;
    }
    return hThread->load();
}

// ���������Ѱ�װ�Ľű������ڳ�������ʱ��"�ű��Զ�����"��
void StartInstalledScriptsOnBoot()
{
    vector<ScriptInfo> scripts;
    {
        shared_lock lock(ScriptData::InstalledMtx);
        scripts = ScriptData::Installed;
    }
    if (scripts.empty())
        return;
    PrintLog("[Script] �Զ������Ѱ�װ�ű����� " + to_string(scripts.size()) + " ��", PL_INFO);
    for (const ScriptInfo& script : scripts)
    {
        {
            lock_guard lock(ScriptData::StatesMtx);
            ScriptState& state = ScriptData::States[script.file];
            if (state.IsRunning)
                continue;   // �������еĽű����ظ�����
            state.IsRunning = true;
        }
        const HANDLE hThread = StartScript(script.file);
        if (hThread != NULL)
        {
            lock_guard lock(ScriptData::InstalledMtx);
            for (ScriptInfo& s : ScriptData::Installed)
            {
                if (s.file == script.file)
                {
                    s.hThread = hThread;
                    break;
                }
            }
        }
        else
        {
            // StartScript ʧ�ܣ����� NULL�����߳����ѵ��� MarkNotRunning ���� IsRunning��
            // ������ʱ���� NULL ���߳��Կ��ڳ�ʼ�����趵�����ã�����"��ʾ������ȴ�޾��"
            lock_guard lock(ScriptData::StatesMtx);
            auto it = ScriptData::States.find(script.file);
            if (it != ScriptData::States.end())
                it->second.IsRunning = false;
        }
    }
}

bool RenderScriptGui(const string& file)
{
    // �޸��������� LuaStatesMtx ������� RunningMtx ��ȡ��ȫ�� Lua ���á���
    // ����ȡָ�����ͷ�����CloseScriptThread ���ڴ����� release()+erase �� LuaState ���� UAF��
    // ��˳��ȫ��ͳһ��LuaStatesMtx �� RunningMtx �� Mtx���� thread_func ��ʼ��/CloseScriptThread һ�£���
    lock_guard lock(LuaStatesMtx);
    auto it = LuaStates.find(file);
    if (it == LuaStates.end())
    {
        // �޸���ɾ���Զ�������ű��߳̿���δ���У�LuaStates ����Ŀ����
        // �����ý���������Ⱦ������������ LuaState ��Ŀ����ʼ������ render ���ơ�
        // ע�⣺LuaState �� mutex ��Ա�������ƶ�/�������������� emplace ��ֵ���� operator[] �͵�Ĭ�Ϲ��졣
        LuaStates[file];
        it = LuaStates.find(file);
    }
    // �޸���RunningMtx �� try_lock�����ű� main ����ִ�У��� Sleep/��ʱ������ʱ��
    // ���߳���Ⱦ�������ȴ���������Ⱦѭ������ = ������ú� UI ��ס������֡���� render����һ֡���ԡ�
    unique_lock<mutex> run_lock(it->second.RunningMtx, try_to_lock);
    if (!run_lock.owns_lock())
        return true;   // �ű� main æ����֡���� render����������õ�����
    // ���� CurrentScriptFile���ýű� render �ڵ� IsScriptKeyDown/SetScriptLoopDelay ���ܶ�λ�Լ���״̬
    const string PrevScript = CurrentScriptFile;
    CurrentScriptFile = file;
    if (!InitLuaState(file, it->second))
    {
        CurrentScriptFile = PrevScript;
        return 0;
    }
    const int ret = CalledScriptFunction(it->second.lua, "render") == 0;
    CurrentScriptFile = PrevScript;
    return ret;
}

int SaveScriptConfig(const string& file)
{
    // �޸���SaveScriptConfig �� detach �߳����У�Ӧ���˳�ʱ��GCExit��CloseAllLua ����� LuaStates��
    // �����̷߳��ʻ� UAF �� ��������ڼ���˳���־��
    if (IsGCExit)
        return 0;
    // �޸���ԭ�������� LuaStates[file]��operator[]���� map ��������ȡָ����ͷ��� �� UAF��
    // �� RenderScriptGui ͬ���� LuaStatesMtx �������ȫ�����ʡ�
    lock_guard lock(LuaStatesMtx);
    auto it = LuaStates.find(file);
    if (it == LuaStates.end())
    {
        // �޸����ű��߳�δ����ʱͬ������ LuaState ��Ŀ����֤ saveConfig �ɱ����ã�����Ĭ�����ã���
        // LuaState �� mutex�������ƶ������� operator[] �͵�Ĭ�Ϲ��졣
        LuaStates[file];
        it = LuaStates.find(file);
    }
    // �޸����� RenderScriptGui һ�£�RunningMtx �� try_lock���ű�æʱ�������棨�����̨�̶߳ѻ���
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
    const int ret = CalledScriptFunction(it->second.lua, "saveConfig") == 0;
    CurrentScriptFile = PrevScript;
    return ret;
}

void CloseScriptThread(const string& filename, ScriptInfo& info)
{
    {
        // д����
        lock_guard lock(ScriptData::StatesMtx);
        ScriptData::States[filename].IsExit = true;
    }

    bool IsExit = false;
    DWORD ret = WaitForSingleObject(info.hThread, 2000);
    if (ret == WAIT_FAILED)
        PrintLog("[Script] �ű��߳̾����Ч��" + info.file, PL_ERROR);
    else if (ret == WAIT_TIMEOUT)
    {
        PrintLog("[Script] �ű��̳߳�ʱ�˳�����ǿ�ƽ�����" + info.file, PL_WARNING);
        if (info.hThread)
            TerminateThread(info.hThread, -1);
        else
            PrintLog("[Script] �߳̾����Ч��" + info.file, PL_ERROR);
    }

    if (info.hThread != NULL)
    {
        CloseHandle(info.hThread);
        info.hThread = NULL;
    }
    {
        // д����
        lock_guard lock(ScriptData::StatesMtx);
        ScriptData::States.erase(filename);
    }
    // ���� LuaStates���Ա��´�����ʱ���¼��ؽű�
    {
        // д����
        lock_guard lock(LuaStatesMtx);
        auto it = LuaStates.find(filename);
        if (it != LuaStates.end())
        {
            // �޸���lua_close ������ render/saveConfig/main �� Lua ���ô��л���Lua ���̰߳�ȫ����
            // �����߳������˳���WAIT_OBJECT_0��ʱ�ȴ� RunningMtx�������� TerminateThread ǿɱ��
            // �߳̿��ܳ��� RunningMtx����ʱ��ȡ���������ÿ������̡߳�
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
        {
            // д����
            lock_guard lock1(ScriptData::InstalledMtx);
            for (ScriptInfo& script : ScriptData::Installed)
                CloseScriptThread(script.file, script);
            ScriptData::Installed.clear();
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
            // �޸���ԭ���� LuaStates.clear() ������ִ�У������� find/operator[] ���� �� map ����
            LuaStates.clear();
        }
        {
            // д����
            lock_guard lock(ScriptData::AvaiableMtx);
            ScriptData::Avaiable.clear();
        }

        // ����ļ���
        DeleteDirectory(ScriptPath);
        CreateDirectory(ScriptPath.c_str(), NULL);
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
        PrintLog("����ű�����ʧ�ܣ�" + errmsg, PL_ERROR);
        RenderTip((config.UseChinese ? u8"����ű�����ʧ�ܣ�"s + errmsg : "Failed to clear script cache: "s + errmsg), 10.0f, PT_ERROR);
        MessageBeep(MB_ICONERROR);
    }
    else
        PrintLog("����ű�����ɹ�", PL_INFO);
}

void CloseAllLua()
{
    {
        // д����
        lock_guard lock1(ScriptData::InstalledMtx);
        for (ScriptInfo script : ScriptData::Installed)
            CloseScriptThread(script.file, script);
        ScriptData::Installed.clear();
        ScriptData::States.clear();
    }
    {
        // д����
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
        // �޸���ԭ���� LuaStates.clear() ������ִ�У������� find/operator[] ���� �� map ����
        LuaStates.clear();
    }
}