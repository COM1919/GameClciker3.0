#pragma once
#include "Definitons.h"
#include "ShareCode.h"
#include <TlHelp32.h>

/*
* INITDriver();  加载驱动
* SelfProtect();  设置自身保护
* RemoveSelfProtect();  移除自身保护
* HiddenSelf();  隐藏自身进程
* SuspendNeteaseProcess();  挂起网易进程(可能会失败记得看一眼任务管理器是否挂起成功)
* ResumeNeteaseProcess();  恢复网易进程
* 重要的事情说三遍!
* 不要改DriverUnload！
* 不要改DriverUnload！
* 不要改DriverUnload！
* 否则蓝屏概不负责
*/

#define DRIVER_ERROR 1
#define DRIVER_SUCCESS 0
#define DRIVER_STOPPED 2
#define DRIVER_RUNNING 3

static int DriverStatus = DRIVER_SUCCESS;

#define IOCTL_SUSPEND_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RESUME_PROCESS  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_Set_PPL         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_Remove_PPL      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_Hidden_Process  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)


DWORD WINAPI StartDriver(LPVOID lpParam);
DWORD WINAPI UnLoadDriver(LPVOID lpParam);
int   LoadDriver(LPCSTR driverPath, LPCSTR serviceName);    //加载驱动
int   DriverUnload(const string& serviceName);              //卸载驱动
int   INITDriver();                                         //初始化驱动
int   UNINITDriver();                                       //关闭驱动
int   SuspendProcess(DWORD pid);                            //挂起进程
int   ResumeProcess(DWORD pid);                             //恢复进程
void  SuspendProcessByName(const char* processName);        //挂起进程
int   ResumeProcessByName(const char* processName);         //恢复进程
int   SelfProtect();                                        //设置自身保护
int   RemoveSelfProtect();                                  //移除自身保护
int   HiddenSelf();                                         //隐藏自身进程
void  SuspendNeteaseProcess();                              //挂起网易进程
void  ResumeNeteaseProcess();                               //恢复网易进程


DWORD WINAPI StartDriver(LPVOID lpParam)
{
    DriverStatus = DRIVER_RUNNING;
    SuspendNeteaseProcess();
    static bool isInit = false;
    if (!isInit)
    {
        if(INITDriver() != 0)
            DriverStatus = DRIVER_ERROR;
    }

    if (SelfProtect() != 0)
        DriverStatus = DRIVER_ERROR;

    if (config.UseHiddenProcess)
        HiddenSelf();

    if (config.UseSuspendNetease)
        ResumeNeteaseProcess();
    return 0;
}

DWORD WINAPI UnLoadDriver(LPVOID lpParam)
{
    DriverStatus = DRIVER_STOPPED;
    SuspendNeteaseProcess();

    RemoveSelfProtect();

    ResumeNeteaseProcess();
    return 0;
}

int LoadDriver(LPCSTR driverPath, LPCSTR serviceName)
{
    SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == nullptr)
    {
        PrintLog(("Failed to open service manager" + to_string(GetLastError())).c_str(), PL_ERROR);
        return 1;
    }

    SC_HANDLE hService = CreateService(hSCManager, serviceName, serviceName, 
        SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, 
        driverPath, NULL, NULL, NULL, NULL, NULL);

    if (hService == nullptr)
    {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS)
        {
            hService = OpenService(hSCManager, serviceName, SERVICE_START);
            if (hService == nullptr)
            {
                PrintLog(("Failed to open existing service" + to_string(error)).c_str(), PL_ERROR);
                CloseServiceHandle(hSCManager);
                return 2;
            }
        }
        else
        {
            PrintLog(("Failed to create service" + to_string(error)).c_str(), PL_ERROR);
            CloseServiceHandle(hSCManager);
            return 3;
        }
    }

    if (!StartService(hService, 0, nullptr))
    {
        PrintLog(("Failed to start service" + to_string(GetLastError())).c_str(), PL_ERROR);
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return 4;
    }

    PrintLog("Load Driver SuccessFul", PL_INFO);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 0;
}

int DriverUnload(const string& serviceName)
{
    SC_HANDLE serviceControlManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!serviceControlManager)
    {
        PrintLog(("Failed Open SCManager" + to_string(GetLastError())).c_str(), PL_ERROR);
        return 2;
    }

    SC_HANDLE serviceHandle = OpenService(serviceControlManager, serviceName.c_str(), SERVICE_STOP | DELETE);
    if (!serviceHandle)
    {
        PrintLog(("Failed Open Service" + to_string(GetLastError())).c_str(), PL_ERROR);
        CloseServiceHandle(serviceControlManager);
        return 1;
    }

    if (DeleteService(serviceHandle))
    {
        PrintLog("Driver Unload Success", PL_INFO);
    }
    else
    {
        PrintLog(("Failed Delete Service" + to_string(GetLastError())).c_str(), PL_ERROR);
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(serviceControlManager);
        return -1;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(serviceControlManager);
    return 0;
}

int INITDriver()
{
    LPCSTR DrvName = "GameClicker-Kernel-Service";
    //return LoadDriver(DriverFile.c_str(), DrvName);
    return 0;
}

int UNINITDriver()
{
    string DserviceName = "GameClicker-Kernel-Service";
    return DriverUnload(DserviceName);
}

int SuspendProcess(DWORD pid)
{
    HANDLE hDevice;
    TCHAR szDeviceName[] = TEXT("\\\\.\\Patch");
    DWORD bytesReturned;

    hDevice = CreateFile(szDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        PrintLog(("Failed to open device. Error " + to_string(GetLastError())).c_str(), PL_ERROR);
        return 1;
    }

    DeviceIoControl(hDevice, IOCTL_SUSPEND_PROCESS, &pid, sizeof(DWORD), NULL, 0, &bytesReturned, NULL);

    CloseHandle(hDevice);
    return 0;
}

int ResumeProcess(DWORD pid)
{
    HANDLE hDevice;
    TCHAR szDeviceName[] = TEXT("\\\\.\\Patch");
    DWORD bytesReturned;

    hDevice = CreateFile(szDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        PrintLog(("Failed to open device. Error " + to_string(GetLastError())).c_str(), PL_ERROR);
        return 1;
    }

    DeviceIoControl(hDevice, IOCTL_RESUME_PROCESS, &pid, sizeof(DWORD), NULL, 0, &bytesReturned, NULL);

    CloseHandle(hDevice);
    return 0;
}

static map<DWORD, int> SuspendCount;

void SuspendProcessByName(const char* processName)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32{};
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        PrintLog("CreateToolhelp32Snapshot failed", PL_ERROR);
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        PrintLog("Process32First failed", PL_ERROR);
        CloseHandle(hProcessSnap);
        return;
    }

    do
    {
        if (strcmp((char*)pe32.szExeFile, processName) == 0)
        {
            PrintLog((" PID: " + to_string(pe32.th32ProcessID)).c_str(), PL_INFO);
            PrintLog((" Process " + string(processName)).c_str(), PL_INFO);
            SuspendProcess(pe32.th32ProcessID);
            SuspendCount[pe32.th32ProcessID]++;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}

int ResumeProcessByName(const char* processName)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32{};
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        PrintLog("CreateToolhelp32Snapshot failed", PL_ERROR);
        return 1;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        PrintLog("Process32First failed", PL_ERROR);
        CloseHandle(hProcessSnap);
        return 2;
    }

    do
    {
        if (strcmp((char*)pe32.szExeFile, processName) == 0)
        {
            PrintLog((" PID: " + to_string(pe32.th32ProcessID)).c_str(), PL_INFO);

            const int count = SuspendCount[pe32.th32ProcessID];

            for (int i = 0; i < count; i++)
            {
                ResumeProcess(pe32.th32ProcessID);
                SuspendCount[pe32.th32ProcessID]--;
            }
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return 0;
}

int SelfProtect()  //保护自身进程
{
    DWORD pid = GetCurrentProcessId();
    HANDLE hDevice;
    TCHAR szDeviceName[] = TEXT("\\\\.\\Patch");
    DWORD bytesReturned;

    hDevice = CreateFile(szDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        PrintLog(("Failed to open device. Error " + to_string(GetLastError())).c_str(), PL_ERROR);
        return 1;
    }

    DeviceIoControl(hDevice, IOCTL_Set_PPL, &pid, sizeof(DWORD), NULL, 0, &bytesReturned, NULL);

    CloseHandle(hDevice);
    return 0;
}

int RemoveSelfProtect() //移除自身保护
{
    DWORD pid = GetCurrentProcessId();
    HANDLE hDevice;
    TCHAR szDeviceName[] = TEXT("\\\\.\\Patch");
    DWORD bytesReturned;

    hDevice = CreateFile(szDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        PrintLog(("Failed to open device. Error " + to_string(GetLastError())).c_str(), PL_ERROR);
        return 1;
    }

    DeviceIoControl(hDevice, IOCTL_Remove_PPL, &pid, sizeof(DWORD), NULL, 0, &bytesReturned, NULL);

    CloseHandle(hDevice);
    return 0;
}

int HiddenSelf()  //隐藏自身进程(关闭程序会蓝屏)
{
    DWORD pid = GetCurrentProcessId();
    HANDLE hDevice;
    TCHAR szDeviceName[] = TEXT("\\\\.\\Patch");
    DWORD bytesReturned;

    hDevice = CreateFile(szDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        PrintLog(("Failed to open device. Error " + to_string(GetLastError())).c_str(), PL_ERROR);
        return 1;
    }

    DeviceIoControl(hDevice, IOCTL_Hidden_Process, &pid, sizeof(DWORD), NULL, 0, &bytesReturned, NULL);

    CloseHandle(hDevice);
    return 0;
}

void SuspendNeteaseProcess()
{
    SuspendProcessByName("WPFLauncher.exe");
    SuspendProcessByName("CefSharp.BrowserSubprocess.exe");
    SuspendProcessByName("FeverGamesWeb.exe");
}

void ResumeNeteaseProcess()
{
    ResumeProcessByName("WPFLauncher.exe");
    ResumeProcessByName("CefSharp.BrowserSubprocess.exe");
    ResumeProcessByName("FeverGamesWeb.exe");
}