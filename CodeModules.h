#pragma once
#include "Definitons.h"
#include "Log.h"
#include <iomanip>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")

int GetCaptionHeight();
void SetupCrashHandler();
inline void Sleep(DWORD ms);
template <typename T> bool Clamp(T& value, T min, T max);
template<typename T> inline bool VarHasChanged(const T& var, const string& label, bool UseInit = false);
inline ULONGLONG GetNowMs();
string GetMachineCode();
string GetPhysicalMacAddress();
string GetDiskSerial();
bool IsProcessElevated();
string BytesToHex(const unsigned char* data, size_t len);
bool IsContainsString(const string& str, const vector<string>& vec);
void ExceptionHandling(DWORD code, string what);
LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo);
inline void BeginSleep(const char* flag);
inline void EndSleep(const char* flag, int time);

bool IsProcessElevated()
{
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return false;

    TOKEN_ELEVATION elevation = { 0 };
    DWORD dwSize = 0;
    BOOL bResult = FALSE;

    if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        bResult = elevation.TokenIsElevated;

    CloseHandle(hToken);
    return bResult != FALSE;
}

string BytesToHex(const unsigned char* data, size_t len)
{
    stringstream ss;
    ss << hex << uppercase << setfill('0');
    for (size_t i = 0; i < len; ++i)
        ss << setw(2) << (int)data[i];
    return ss.str();
}

// 获取物理网卡MAC地址
string GetPhysicalMacAddress()
{
    string result;
    ULONG bufLen = 15000;
    IP_ADAPTER_ADDRESSES* pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufLen);

    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &bufLen) == NO_ERROR)
    {
        for (IP_ADAPTER_ADDRESSES* p = pAddresses; p; p = p->Next)
        {
            // 过滤条件：必须是物理网卡，且不是环路，且不是VPN/虚拟网卡
            if (p->IfType == IF_TYPE_ETHERNET_CSMACD ||
                p->IfType == IF_TYPE_IEEE80211)
            {

                // 排除常见虚拟网卡关键词
                wstring desc = p->Description;
                bool isVirtual = (desc.find(L"Virtual") != wstring::npos ||
                    desc.find(L"VMware") != wstring::npos ||
                    desc.find(L"VirtualBox") != wstring::npos ||
                    desc.find(L"Hyper-V") != wstring::npos ||
                    desc.find(L"VPN") != wstring::npos ||
                    desc.find(L"TAP") != wstring::npos);

                if (!isVirtual && p->PhysicalAddressLength == 6)
                {
                    // 取第一个找到的物理网卡MAC
                    result = BytesToHex(p->PhysicalAddress, p->PhysicalAddressLength);
                    break;
                }
            }
        }
    }
    free(pAddresses);
    return result;
}

// 获取硬盘序列号
string GetDiskSerial()
{
    string result;
    HRESULT hres;

    // 初始化COM
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return "";

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
    if (SUCCEEDED(hres))
    {
        IWbemServices* pSvc = NULL;
        hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, 0, 0, 0, &pSvc);

        if (SUCCEEDED(hres))
        {
            // 设置安全级别
            hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL, EOAC_NONE);

            if (SUCCEEDED(hres))
            {
                IEnumWbemClassObject* pEnumerator = NULL;
                hres = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("SELECT SerialNumber FROM Win32_DiskDrive WHERE Index=0"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator);

                if (SUCCEEDED(hres))
                {
                    IWbemClassObject* pclsObj = NULL;
                    ULONG uReturn = 0;

                    if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK)
                    {
                        VARIANT vtProp;
                        if (SUCCEEDED(pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0)))
                        {
                            if (vtProp.vt == VT_BSTR)
                            {
                                char buffer[128];
                                WideCharToMultiByte(CP_ACP, 0, vtProp.bstrVal, -1,
                                    buffer, sizeof(buffer), NULL, NULL);
                                result = string(buffer);
                            }
                            VariantClear(&vtProp);
                        }
                        pclsObj->Release();
                    }
                    pEnumerator->Release();
                }
            }
            pSvc->Release();
        }
        pLoc->Release();
    }
    CoUninitialize();
    return result;
}

string SimpleHash(const string& input)
{
    string data = "GC" + input;
    unsigned long long hash = 14695981039346656037ULL;  // 64位FNV偏移基数
    for (char c : data)
    {
        hash ^= static_cast<unsigned long long>(c);
        hash *= 1099511628211ULL;  // 64位FNV素数
    }

    stringstream ss;
    ss << hex << uppercase << setfill('0');
    ss << setw(16) << hash;
    return ss.str();
}

string GetMachineCode()
{
    string mac = GetPhysicalMacAddress();
    if (mac.empty())
        mac = "NOMAC"; // 极端情况兜底
    string diskSN = GetDiskSerial();
    if (diskSN.empty())
        diskSN = "NODISK";
    string rawFingerprint = mac + "-" + diskSN;
    return SimpleHash(rawFingerprint);
}

// 判断字符串是否在字符串数组中
bool IsContainsString(const string& str, const vector<string>& vec)
{
    for (const string& st : vec)
    {
        if (st.find(str) != string::npos)
            return true;
    }
    return false;
}

int GetCaptionHeight()
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
        return ncm.iCaptionHeight;
    return 0; // 获取失败
}

// 设置错误处理器
void SetupCrashHandler()
{
    // 设置未处理异常过滤器
    SetUnhandledExceptionFilter(TopLevelExceptionFilter);
    // 终止处理
    SetErrorMode(SEM_FAILCRITICALERRORS);
    // 控制台异常处理
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
}

LONG WINAPI TopLevelExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
    stringstream str;
    str << "异常代码: 0x" << hex << pExceptionInfo->ExceptionRecord->ExceptionCode
        << "异常地址: " << pExceptionInfo->ExceptionRecord->ExceptionAddress;
    cerr << "GC发生严重崩溃 - " << str.str() << endl;
    PrintLog("[Exception] GC发生严重崩溃 - " + str.str(), PL_ERROR);
    // 处理特定异常
    string err;
    switch (pExceptionInfo->ExceptionRecord->ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
            err = "内存访问违规";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            err = "整数除零错误";
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            err = "浮点数除零错误";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            err = "栈溢出";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            err = "数组越界";
            break;
        default:
            err = "未知异常";
    }
    ExceptionHandling(pExceptionInfo->ExceptionRecord->ExceptionCode, err);
    return EXCEPTION_EXECUTE_HANDLER;
}

// 每当调用这个函数就代表致命错误，会主动退出GC
void ExceptionHandling(DWORD code, string what)
{
    string msg;
    if (config.UseChinese)
    {
        msg += "GC发生了致命错误！\n";
        msg += "原因：" + what + "\n";
        msg += "系统错误代码：" + to_string(code) + "\n\n";
        msg += "你可以前往 GC连点器论坛 反馈此问题，我们会尽快修复。\n";
        msg += "GC论坛：bbs.mcgc.top\n";
        msg += "或者必应搜索“GC连点器论坛”\n";
    }
    else
    {
        msg += "GC has encountered a fatal error!\n";
        msg += "Reason: " + what + "\n";
        msg += "System error code: " + to_string(code) + "\n\n";
        msg += "You can report this issue to the GC Clicker Forum, \nwe will fix it as soon as possible.\n";
        msg += "GC Forum: bbs.mcgc.top\n";
        msg += "Or Bing search \"GameClicker\"";
    }

    // 强制用记事本打开
    ShellExecute(NULL, "open", "notepad.exe", LogFile.c_str(), NULL, SW_SHOWNORMAL);

    MessageBox(NULL, msg.c_str(), config.UseChinese ? "GC连点器错误" : "GC Error", MB_OK | MB_ICONERROR);
    PrintLog("[Exception] GC发生了致命错误 - 错误代码" + to_string(code) + "原因：" + what, PL_ERROR);
    // 关闭互斥量，关闭防止双开
    if (AntiMultiOpenMutex)
        CloseHandle(AntiMultiOpenMutex);
    GCExit = true;
}

inline void Sleep(DWORD ms)
{
    this_thread::sleep_for(chrono::milliseconds(ms));
}

template <typename T>
bool Clamp(T& value, T min, T max)
{
    if (value < min)
    {
        value = min;
        return false;
    }
    else if (value > max)
    {
        value = max;
        return false;
    }
    return true;
}

template<typename T>
inline bool VarHasChanged(const T& var, const string& label, bool UseInit)
{
    static unordered_map<string, T> lastValues;
    static mutex mtx;

    lock_guard<mutex> lock(mtx);

    // 尝试插入，如果已存在则获取迭代器
    auto [it, inserted] = lastValues.try_emplace(label, var);
    if (inserted) [[unlikely]]
        return UseInit;

    // 比较并更新
    if (var != it->second) [[likely]]
    {
        it->second = var;  // 这里可以根据需要改用 std::move
        return true;
    }

    return false;
}

inline ULONGLONG GetNowMs()
{
    return static_cast<int>
        (
            chrono::time_point_cast<chrono::milliseconds>
            (chrono::high_resolution_clock::now())
            .time_since_epoch().count()
            );
}

static map<string, time_t> LastTime;
// 不一定需要End
inline void BeginSleep(const char* flag)
{
    LastTime[flag] = GetTickCount64();
}

inline void EndSleep(const char* flag, int time)
{
    const ULONGLONG deltaTime = GetTickCount64() - LastTime[flag];
    if (deltaTime < time)
        Sleep(time - deltaTime);
}
