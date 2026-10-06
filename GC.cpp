#include "GuideGui.h"
#include "Script.h"
#include "StartAnim.h"
#include "Tray.h"
#include "GetInfo.h"
#include "ScriptEditor.h"
#include "HookCentral.h"   // 中央分发钩子（4 个低级钩子合并为键盘/鼠标各 1 个）

#include <cstring>   // strstr（命令行 debug 参数检测）

typedef HRESULT(WINAPI* PFNSETPROCESSDESCRIPTION)(HANDLE, PCSTR);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // 调试模式：命令行带 "debug" 参数（如 "GC.exe debug"）→ 同步 cout 日志。
    // GUI 程序默认无控制台：先从父进程（cmd）附加控制台，失败则创建独立控制台，
    // 并重定向 stdout，使 PrintLog 里的 cout 能显示在终端里（崩溃前可立即看到日志）。
    if (lpCmdLine && strstr(lpCmdLine, "debug"))
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
            AllocConsole();
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        LogDebugCout = true;

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        cout << ASCIITitle << endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        cout << "===========================Debug模式已开启===========================\n\n" << endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
    }
    int ret = StartUpGC(hInstance);
    cout << "程序退出，返回值：" << ret << endl;
}

int StartUpGC(HINSTANCE hInstance)
{
    StartTime = GetNowMs();

    // 获取系统默认语言信息
    if (PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) == LANG_CHINESE)
        config.UseChinese = true;
    else
        config.UseChinese = false;

    GCInstance = hInstance;
    //服了，远古代码来的，千万不能改，否则会出现双开
    AntiMultiOpenMutex = CreateMutex(NULL, FALSE, _T("GameClick_MC1919"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (AntiMultiOpenMutex)
            CloseHandle(AntiMultiOpenMutex);
        MessageBox(NULL, config.UseChinese ? "你已经打开了一个GC连点器了!" : "You can only run one instance of GC at a time.", config.UseChinese ? "提示" : "Warning", MB_OK | MB_ICONQUESTION);
        return 30;
    }

#ifdef GC_DEBUG_CMD
    if (!AllocConsole())
    {
        OutputDebugString("[Init] 无法分配控制台窗口");
        return 20;
    }
    // 设置标准输入输出错误句柄到新控制台
    FILE* stream;
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    HWND ConsoleHwnd = GetConsoleWindow();
    RECT rect;
    GetWindowRect(ConsoleHwnd, &rect);
    SetWindowPos(ConsoleHwnd, NULL, 0, 0, rect.right - rect.left + 100, rect.bottom - rect.top + 20, SWP_NOMOVE);
#endif // GC_DEBUG_CMD

#ifdef GC_REMOTE_DEBUG
    MessageBox(NULL, "已开启远程调试，点击确定继续", "GC", MB_OK);
#endif //GC_REMOTE_DEBUG

    if (WorkPath.empty())
    {
        MessageBox(NULL, ((config.UseChinese ? "无法获取工作目录：" : "Oops!\nFailed to obtain the working directory:") + to_string(GetLastError())).c_str(), config.UseChinese ? "错误" : "Error", MB_OK | MB_ICONERROR);
        return 50;
    }
    {
        char TempPath[MAX_PATH] = { 0 };
        GetTempPath(MAX_PATH, TempPath);
        string exe(WorkPath);
        string temp(TempPath);
        for (char& c : exe) c = tolower(c);
        for (char& c : temp) c = tolower(c);
        if (exe.find(temp) == 0)
        {
            MessageBox(NULL, config.UseChinese ? "请解压文件后使用！" : "Please unzip the program file,\nand then you can run it.", "错误", MB_OK | MB_ICONERROR);
            return 60;
        }
    }

    if (_access(LogFile.c_str(), 0) != 0)
        CreateFile(LogFile);
    else
    {
        PackageLogFile();
        ClearFile(LogFile);
    }
    InitLogSystem(LogFile); // 初始化日志系统，启动日志线程

    if (!IsRunningAsElevatedAdmin())
    {
        const int ret = MessageBox(NULL, config.UseChinese ? "该程序需要管理员权限才能运行。\n但拒绝后你仍然可以使用，但可能不会正常工作。\n点击确定自动请求权限" : "This program requires admin privileges to run. \nBut even if you refuse, you can still use it,\nbut it may not work properly.\nClick OK to auto request permissions.", config.UseChinese ? "GameClicker警告" : "GameClicker Warning", MB_OKCANCEL);
        if (ret == IDOK)
        {
            CloseHandle(AntiMultiOpenMutex);
            const DWORD err = RequestAdminAndRestart();
            if (err != 0)
            {
                PrintLog("请求管理员权限失败，错误代码：" + to_string(err), PL_ERROR);
                MessageBox(NULL, config.UseChinese ? "请求管理员权限失败，\n请尝试手动选择管理员身份运行。" : "Failed to request administrator privileges.\nPlease try manually selecting the administrator identity to run.", config.UseChinese ? "GameClicker错误" : "GameClicker Error", MB_OK);
                return 80;
            }
		    return 81; // 成功请求权限并重启，退出当前进程
        }
		MessageBox(NULL, config.UseChinese ? "你拒绝了管理员权限请求，\n程序将继续运行，但可能不会正常工作。" : "You refused the administrator privilege request,\nthe program will continue to run, but it may not work properly.", config.UseChinese ? "GameClicker提示" : "GameClicker Warning", MB_OK);
    }

    PrintLog("[Init] 开始加载动画", PL_INFO);
    CreateThreadSafe(LoadAnimationThread, NULL, 0, "LoadAnimationThread");

    // 初始化符号处理（用于更好的堆栈跟踪）
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
    // 设置崩溃处理器
    SetupCrashHandler();

    // 不管怎么try都抓不住你的心
    try
    {
        //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        cout << "进入GCMainLoop" << endl;
        int ret = GCMainLoop();
        cout << "GCMainLoop退出，返回值：" << ret << endl;
    }
    catch (exception& e)
    {
        ExceptionHandling(GetLastError(), e.what() + " - [Main]"s);
        PrintLog("[Exception] 程序发生崩溃，原因："s + e.what(), PL_ERROR);
        NetworkClient.Send("exit");
        return 70;
    }
    catch (...)
    {
        ExceptionHandling(GetLastError(), "未知原因 - [Main]"s);
        PrintLog("[Exception] 程序发生崩溃，原因很神秘", PL_ERROR);
        NetworkClient.Send("exit");
        return 71;
    }

    //===============================结束===============================
    NetworkClient.Send("exit");
    PrintLog("[Render] 释放GUI", PL_INFO);
    // 先关闭脚本线程, 避免其仍在用 D3D 加载纹理(LoadTextureFromFile)时 ReleasGui 释放设备
    CloseAllLua();
    // 停止并等待所有后台线程结束，再释放 GUI/全局对象，避免退出时访问已析构内存
    StopAndWaitBackgroundThreads();
    ReleasGui(wcex, &MainGuiData);

    PrintLog("[Render] 释放Dx11", PL_INFO);
    DragAcceptFiles(GCHWND, FALSE);
    ShowWindow(GCHWND, SW_HIDE);
    if (IsFirstRun)
        DelFile(ConfigFile.c_str());

    Shell_NotifyIcon(NIM_DELETE, &TrayNid);

    // 关闭互斥量
    if (AntiMultiOpenMutex)
        CloseHandle(AntiMultiOpenMutex);
    // 清除符号处理
    SymCleanup(GetCurrentProcess());

    SaveConfig(GetPresetFile(PresetConfig::CurrentName));

    PrintLog("[Render] GC连点器退出成功 - 开发者：COM1919", PL_INFO);
    return 1919;
}

DWORD WINAPI SystemMsgHookLoop(LPVOID lpParam)
{
#ifndef GC_NO_HOOK
    bool IsRetry = false;
retry:
    // 钩子合并：原 4 个低级钩子（键盘 x2 + 鼠标 x2）-> 2 个（键盘 x1 + 鼠标 x1），
    // 同类型事件在回调内靠中央分发（见 HookCentral.h）。低级钩子每次事件都会打断进程，
    // 合并后同一次事件只打断一次，重复 CallNextHookEx 链遍历的开销随之消失。
    HHOOK Hook_Mouse = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
    HHOOK Hook_Keybd = SetWindowsHookEx(WH_KEYBOARD_LL, KeybdHookProc, GetModuleHandle(NULL), 0);

    bool IsError = false;
    if (Hook_Mouse == NULL)
    {
        PrintLog("[Hook] 无法设置鼠标钩子", PL_ERROR);
        IsError = true;
    }
    if (Hook_Keybd == NULL)
    {
        PrintLog("[Hook] 无法设置键盘钩子", PL_ERROR);
        IsError = true;
    }

    if (IsError)
    {
        map<string, UINT> button;
        map<string, ImVec4> colors;
        button[u8"好的"] = IDOK;
        button[u8"重试"] = IDRETRY;
        colors[u8"好的"] = GuiColor[ImGuiCol_ButtonHovered];
        colors[u8"重试"] = GuiColor[ImGuiCol_Button];
        if (RenderMessageBoxEx(config.UseChinese ? "无法设置全局钩子（HOOK），这会导致部分功能无法使用。" : 
            "Cannot set global hook.\nSome functions may not work!", config.UseChinese ? "错误" : "Error", 
            button, colors, true) == IDRETRY)
        {
            IsRetry = true;
            goto retry;
        }
        return 0;
    }
    else if (IsRetry)
        RenderTip(config.UseChinese ? "全局钩子设置成功！" : "Global hook set successfully", 4.0f, PT_INFO);
    PrintLog("[Hook] 全局钩子安装成功（键盘/鼠标各 1 个，事件中央分发）", PL_INFO);

    // 启动钩子事件分发线程：钩子回调只入队（低频、不可阻塞），所有 Handle
    // （状态/声音/连点器）在 HookDispatchThread 单一工作线程中异步执行（见 HookCentral.h）
    CreateThreadSafe(HookDispatchThread, NULL, 0, "HookDispatchThread");
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (IsGCExit)
            break;
    }

    PrintLog("[Hook] 卸载全局HOOK", PL_INFO);
    UnhookWindowsHookEx(Hook_Mouse);
    UnhookWindowsHookEx(Hook_Keybd);
#endif // !GC_NO_HOOK
    return 0;
}

void ScrollAnim::HandleScrolling()
{
    ImGuiIO& io = ImGui::GetIO();
    // 处理垂直滚动
    float scroll_now_y = 0.0f;
    if (fabs(ScrollAnim::EnergyY.load()) > 0.001f)
    {
        scroll_now_y = ScrollAnim::EnergyY.load() * io.DeltaTime * ScrollAnim::SMOOTHING;
        ScrollAnim::AddEnergy(ScrollAnim::EnergyY, -scroll_now_y);
    }
    else
        ScrollAnim::EnergyY.store(0.0f);

    // 处理水平滚动（如果有）
    float scroll_now_x = 0.0f;
    if (fabs(ScrollAnim::EnergyX.load()) > 0.001f)
    {
        scroll_now_x = ScrollAnim::EnergyX.load() * io.DeltaTime * ScrollAnim::SMOOTHING;
        ScrollAnim::AddEnergy(ScrollAnim::EnergyX, -scroll_now_x);
    }
    else
        ScrollAnim::EnergyX.store(0.0f);

    // 将平滑值赋给 IO（注意符号：ImGui 的正方向与 Win32 可能相反）
    io.MouseWheel = scroll_now_y / 2.0f;   // 垂直（除以2来减少滑动距离）
    io.MouseWheelH = -scroll_now_x;        // 水平（可能需要负号）
}

int GCMainLoop()
{
    thread th2(DeleteOldLog); // 删除和清理old日志
    th2.detach();

    // 一定要在下面文件初始化前面
#ifdef GC_DEBUG_GUIDE_UI
    IsFirstRun = true;
#else
    IsFirstRun = (_access(ConfigFile.c_str(), 0) != 0);
#endif // GC_DEBUG_GUIDE_UI

    char path[MAX_PATH]{};
    if (GetModuleFileName(NULL, path, MAX_PATH) == 0)
        PrintLog("[Init] 当前程序的绝对位置："s + path, PL_INFO);

    PrintLog("[Init] 检查文件完整性 - MainThread", PL_INFO);

    // 在RefushScriptList之前删除
    if (DeleteDirectory(ScriptShopDir))
        PrintLog("[File] 删除脚本缓存成功！", PL_INFO);
    else
        PrintLog("[File] 删除脚本缓存失败！", PL_ERROR);
    if (DeleteDirectory(ScriptTemp))
        PrintLog("[File] 删除脚本商店缓存成功！", PL_INFO);
    else
        PrintLog("[File] 删除脚本商店缓存失败！", PL_ERROR);
    if (_access(ConfigPath.c_str(), 0) != 0)
        CreateDirectory(ConfigPath.c_str(), NULL);

    LoadConfig();
    app_status.StartCount++;   // 启动次数 +1（本次启动）
    PresetConfig::Presets = ScanPresets();

    if (_access(LogPath.c_str(), 0) != 0)
        CreateDirectory(LogPath.c_str(), NULL);

    //===============================创建窗口===============================
    {
        HICON hIcon = LoadIcon(GCInstance, MAKEINTRESOURCE(GC_ICON));
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = GCInstance;
        wcex.hIcon = hIcon;
        wcex.hCursor = nullptr;
        wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = "  ";
        wcex.hIconSm = hIcon;

        WindowWidth = IsFirstRun ? 600 : 800;
        WindowHeight = IsFirstRun ? 400 : 500;
        {
            RegisterClassEx(&wcex);
            GCHWND = CreateWindow(wcex.lpszClassName,
                "   ", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_CAPTION & WS_POPUP /* & WS_EX_LAYERED*/ & WS_EX_ACCEPTFILES,
                (ScreenWidth - WindowWidth) / 2, ScreenHeigth + 40, WindowWidth, WindowHeight,
                NULL, NULL, wcex.hInstance, NULL);

            LONG_PTR Style = GetWindowLongPtr(GCHWND, GWL_STYLE);
            Style = Style & ~WS_CAPTION & ~WS_SYSMENU & ~WS_SIZEBOX;
            SetWindowLongPtr(GCHWND, GWL_STYLE, Style);

            //SetRoundRectRgn(GCHWND, SizeX, SizeY, 24);
            SetWindowDisplayAffinity(GCHWND, config.UseBroadcasterMode ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
            //SetLayeredWindowAttributes(GCHWND, 0, round(255 * WindowAlpha), LWA_ALPHA);

            // 使用DWM扩展帧实现玻璃效果
            /*
            DwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
            MARGINS margins = { -1 };
            DwmExtendFrameIntoClientArea(GCHWND, &margins);
            UINT attribute = DWMSBT_TRANSIENTWINDOW;
            DwmSetWindowAttribute(GCHWND, DWMWA_SYSTEMBACKDROP_TYPE, &attribute, sizeof(attribute));
            DwmSetWindowAttribute(GCHWND, DWMWA_WINDOW_CORNER_PREFERENCE, &attribute, sizeof(attribute));
            */

            ShowWindow(GCHWND, SW_HIDE);
            UpdateWindow(GCHWND);
            PrintLog("[Render] 创建窗口成功！", PL_INFO);
        }
    }

    if (!CreateDeviceD3D(GCHWND))
    {
        CleanupDeviceD3D();
        DestroyWindow(GCHWND);
        UnregisterClass(wcex.lpszClassName, wcex.hInstance);
        PrintLog("[Render] Dx11初始化失败", PL_ERROR);
        ExceptionHandling(GetLastError(), "Dx11初始化失败 - 你的电脑视乎很古董");
        return 1;
    }
    PrintLog("[Render] Dx11初始化成功！", PL_INFO);

    IMGUI_CHECKVERSION();
    GCCtx = CreateContext();
    ImPlot::CreateContext();
    ImPlot3D::CreateContext();

    ImGuiIO& io = GetIO();
    (void)io;

    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;
    UpdateWindow(GCHWND);
    InitializationGui(io);

    CreateThreadSafe(InitGC, NULL, 0, "InitGC");
    
    bool ShouldFirstClearMemory = true;

    long LastTime = 0;
    long DeltaTime = 0;
    int RenderSleepCount = 0;
    const int SleepFps = round(1000 / RenderFps);
    PrintLog("[Render] 开始渲染UI", PL_INFO);

    IsWindowInitSuccessful = true;

    while (!IsGCExit)
    {
        LastTime = clock();
        //===============================消息处理===============================
        {
            constexpr int MaxMsgsPerFrame = 128;   // 每帧处理的消息数量，限制每帧的处理数据量
            int processed = 0;
            MSG msg;
            while (processed < MaxMsgsPerFrame && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                bool IsClicker = IsClickering.right || IsClickering.left;
                switch (msg.message)
                {
                case WM_LBUTTONUP:
                case WM_LBUTTONDOWN:
                {
                    if (IsClicker && config.LClickerMode & C_MOUSESMODE && config.LClickerMode & C_ASSISTMODE)
                        continue;
                    break;
                }
                case WM_RBUTTONUP:
                case WM_RBUTTONDOWN:
                {
                    if (IsClicker && config.RClickerMode & C_MOUSESMODE && config.RClickerMode & C_ASSISTMODE)
                        continue;
                    break;
                }
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);

                ++processed;
            }
        }

        //===============================处理暂停渲染逻辑===============================
        {
            bool IsWindowTopmost = (GetWindowLongPtr(GCHWND, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
            // 当程序最小化时，不渲染UI
            static bool RenderStoped = false;
            const bool IsMinWindows = (MainGuiData.pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) && MainGuiData.SwapChainOccluded;
            static bool IsCoveredWindows = false; // 每秒更新一次
            {
                static ULONGLONG LastTime = GetTickCount64();
                if (LastTime + 200 <= GetTickCount64())
                {
                    IsCoveredWindows = IsCoveredWindow(GCHWND, 0.8f);
                    LastTime = GetTickCount64();
                }
            }
            if ((!IsShowGui || IsMinWindows || IsCoveredWindows) && !IsWindowTopmost)
            {
                if (!RenderStoped)
                    RenderStoped = true;
                if (RenderSleepCount >= 0)
                {
                    RenderSleepCount++;
                    if (RenderSleepCount > (1000 * 30) / 20) // 超过30秒不使用UI，就优化内存使用
                    {
                        ClearProcessMemory();
                        RenderSleepCount = -1;
                    }
                }
                Sleep(20);
                continue;
            }
            else if (RenderSleepCount != 0)
                RenderSleepCount = 0;
            else if (RenderStoped)
                RenderStoped = false;
            MainGuiData.SwapChainOccluded = false;
        }

        //===============================处理其他渲染逻辑===============================
        // 处理窗口大小调整（我们不直接在WM_SIZE处理程序中调整大小）
        if (MainGuiData.ResizeWidth != 0 && MainGuiData.ResizeHeight != 0)
        {
            CleanupRenderTarget();
            MainGuiData.pSwapChain->ResizeBuffers(0, MainGuiData.ResizeWidth, MainGuiData.ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            MainGuiData.ResizeWidth = MainGuiData.ResizeHeight = 0;
            CreateRenderTarget();
        }
        
        // 平滑滚动条
        ScrollAnim::HandleScrolling();

        //===============================处理渲染===============================
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (IsFirstRun)
        {
            const int state = RenderGuide(io);
            if (state == 1)
            {
                IsFirstRun = false;
                IsReloadClicker = true;
                IsStopClicker = false;

                SetWindowText(GCHWND, "  ");
                // 设置窗口大小
                int x = (ScreenWidth - WindowWidth) / 2;
                SetWindowPos(GCHWND, NULL, x, ScreenHeigth + 30, 800, 500, SWP_NOZORDER | SWP_SHOWWINDOW);
                ShowWindow(GCHWND, SW_SHOW);

                WindowWidth = io.DisplaySize.x = 800;
                WindowHeight = io.DisplaySize.y = 500;

                WindowOpenAnimate(GCHWND, { x, (ScreenHeigth - WindowHeight) / 2 }, 0.3f);
                UpdateWindow(GCHWND);

                IsNeedReshowAgreement = false; // 不再显示Agreement
            }
            else if (state == -1)
                IsGCExit = true;
        }
        else
            RenderGui(io);

        /*if (IsAnyItemHovered() || IsAnyItemActive())
            cout << "a\n";*/

        //=======================渲染===========================
        ImGui::Render();
        const float ClearColor[4] = { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
        MainGuiData.pd3dDeviceContext->OMSetRenderTargets(1, &MainGuiData.mainRenderTargetView, nullptr);
        MainGuiData.pd3dDeviceContext->ClearRenderTargetView(MainGuiData.mainRenderTargetView, ClearColor);
        ImGui_ImplDX11_RenderDrawData(GetDrawData());

        // Present
        const HRESULT hr = MainGuiData.pSwapChain->Present(1, 0);// 使用垂直同步
        //const HRESULT hr = g_pSwapChain->Present(0, 0); // 不使用垂直同步
        MainGuiData.SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        DeltaTime = clock() - LastTime;
        if (DeltaTime < SleepFps)
            Sleep(SleepFps - DeltaTime - 1);
    }
    return 0;
}

DWORD WINAPI InitGC(LPVOID lpParam)
{
    PrintLog("[Init] 创建线程", PL_INFO);
    CreateThreadSafe(HandleCPSRecord, NULL, 0, "HandleCPSRecord");
    SetThreadPriority(ClickRunThread, THREAD_PRIORITY_HIGHEST);
    ClickSoundThread = CreateThreadSafe(ClickSound, NULL, 0, "ClickSound");
    CreateThreadSafe(GetInfo, NULL, 0, "GetInfo");
    CreateThreadSafe(Updata, NULL, 0, "Updata");
    CreateThreadSafe(RecordsPCRate, NULL, 0, "RecordsPCRate");
    CreateThreadSafe(CreateTrayIcon, NULL, 0, "CreateTrayIcon");
    CreateThreadSafe(ClickerControl, NULL, 0, "ClickerControl");
    CreateThreadSafe(MouseShakeMove, NULL, 0, "MouseShakeMove");
    CreateThreadSafe(SystemMsgHookLoop, NULL, 0, "SystemMsgHookLoop");
    CreateThreadSafe(NetworkClientThread, NULL, 0, "NetworkClientThread");
    CreateThreadSafe(ScriptKeyControlThread, NULL, 0, "ScriptKeyControlThread");

    //CreateThreadSafe(GameGuiThread, NULL, 0, "GameGuiThread");

    bool IsDownload = false;

    PrintLog("[Init] 检查文件完整性 - InitThread", PL_INFO);
    // 安装的脚本会在配置文件读取的函数中，获取到
    //RefreshScriptList(InstallScriptList, InstallTextureList, InstallScriptInfo, InstallScriptName);

    if (_access(ScriptPath.c_str(), 0) != 0)
    {
        CreateDirectory(ScriptPath.c_str(), NULL);
        ScriptData::Installed.clear();
        LuaStates.clear();
    }
    if (_access(ScriptPath.c_str(), 0) != 0)
        CreateDirectory(ScriptPath.c_str(), NULL);
    if (_access(ScriptTemp.c_str(), 0) != 0)
        CreateDirectory(ScriptTemp.c_str(), NULL);
    if (_access(Sound_CfgPath.c_str(), 0) != 0)
        CreateDirectory(Sound_CfgPath.c_str(), NULL);
    if (_access(Sound_KeybdPath.c_str(), 0) != 0)
        CreateDirectory(Sound_KeybdPath.c_str(), NULL);
    if (_access("D:\\GameClicker\\sound", 0) != 0)
        DeleteDirectory("D:\\GameClicker\\sound");

    string ver;
    char* buffer = new char[MAX_BUF];
    GetPrivateProfileString(u8"Version", u8"version", "", buffer, 100, IniFileVeision.c_str());
    ver = buffer;
    delete[] buffer;

    int curHver = 0;
    int curLver = 0;
    int newHver = 0;
    int newLver = 0;

    if(!ver.empty())
    {
        size_t dotPos = version.find('.');

        string majorVersion = version.substr(0, dotPos);
        string minorVersion = version.substr(dotPos + 1);
        try
        {
            curHver = stoi(majorVersion);
            curLver = stoi(minorVersion);
        }
        catch (const exception&)
        {
            PrintLog("[Init] 版本号格式错误", PL_ERROR);
        }
    }
    if (!ver.empty())
    {
        size_t dotPos = ver.find('.');

        string majorVersion = ver.substr(0, dotPos);
        string minorVersion = ver.substr(dotPos + 1);
        try
        {
            newHver = stoi(majorVersion);
            newLver = stoi(minorVersion);
        }
        catch (const exception&)
        {
            PrintLog("[Init] 版本号格式错误", PL_ERROR);
        }
    }

    // 允许拖拽文件到窗口
    DragAcceptFiles(GCHWND, true);

    CreateDirectory(ScriptShopDir.c_str(), NULL); // 注意顺序
    CreateDirectory(ScriptShopTemp.c_str(), NULL);
    CreateDirectory(ScriptTemp.c_str(), NULL);
    RefushScriptList();
    // 已安装列表恢复必须在 ScriptData::Avaiable 填充之后（原逻辑在 LoadConfig 中因列表为空被跳过）
    RestoreInstalledScripts();

    // 为已恢复的已安装脚本创建状态条目（供"启用"开关与启动按键显示）
    {
        shared_lock lock1(ScriptData::InstalledMtx);
        lock_guard lock2(ScriptData::StatesMtx);
        for (const ScriptInfo& script : ScriptData::Installed)
            ScriptData::States[script.file];
    }

    {
        while (!IsWindowInitSuccessful)
            Sleep(50);
        lock_guard lock(InitSuccessTimeMutex);
        if (InitSuccessTime == 0 || InitSuccessTime < GetNowMs())
            InitSuccessTime = GetNowMs();
    }

    // 未禁用脚本自动启动时，启动所有已安装脚本
    if (!DisableScriptAutoStart)
        StartInstalledScriptsOnBoot();
    PrintLog("[Init] 初始化已完成，耗时: " + to_string(GetNowMs() - StartTime), PL_INFO); // 原代码误打印了启动时间戳，而非耗时
    return 0;
}

DWORD WINAPI RecordsPCRate(LPVOID lpParam)
{
    while (!IsGCExit)
    {
        // 修复：原代码无锁 push_back/erase，与主渲染线程的 back()/copy 并发时，
        // vector 扩容/缩容导致迭代与访问已释放内存 -> 0xC0000005。写入统一加写锁。
        // 采样函数（内部会 Sleep）放在锁外，避免长时间占用锁阻塞渲染线程。
        const float cpuRate = GetCpuUsageRate(200) / 100.0f;
        const float gpuRate = GetGpuUsageRate(200) / 100.0f;
        const float memRate = GetMemUsageRate(200) / 100.0f;
        {
            lock_guard lock(PerformanceMutex);
            CpuRates.push_back(cpuRate);
            GpuRates.push_back(gpuRate);
            MemRates.push_back(memRate);

            if (CpuRates.size() > MAX_PERF_RECORDS)
                CpuRates.erase(CpuRates.begin());

            if (GpuRates.size() > MAX_PERF_RECORDS)
                GpuRates.erase(GpuRates.begin());

            if (MemRates.size() > MAX_PERF_RECORDS)
                MemRates.erase(MemRates.begin());
        }
        //GetCpuUsageRate等函数会进行Sleep
    }
    return 0;
}

void RenderGui(ImGuiIO& io)
{
    if (IsNeedResetConfig.exchange(false))
    {
        IsStopClicker = true;
        IsReloadClicker = true; // 连点线程重载时会重新读取新配置
        config.~Config();       // 析构
        new (&config) Config(); // 原地构造
        RenderTip(config.UseChinese ? u8"重置成功" : "Reset success", 1.5f, PT_INFO);
    }

    bool HasBgImage = _access(Pics_Background.c_str(), 0) == 0;

    SetCurrentContext(GCCtx);
    static ImGuiStyle& Style = GetStyle();
    static bool IsInit = false;
    GuiColor = Style.Colors;

    if (!IsInit || VarHasChanged(HasBgImage, "HasBgImage") || VarHasChanged(GuiItemAlpha, "GuiItemAlpha"))
    {
        Style = GetGuiStyle(Style);
        if (GuiItemAlpha > 100) GuiItemAlpha = 100;
        GuiColor = GetGuiColor(Style, HasBgImage ? GuiItemAlpha / 100.0f : 1.0f);
        IsInit = true;
    }

    //if (pb_message.open)
    //{
    //    RenderControl();
    //    return;
    //}

    if (IsKeyPressed(ImGuiKey_LeftCtrl) && IsKeyPressed(ImGuiKey_S))
        IsReloadClicker = true;

    PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.WindowPadding.x, 8));
    static const string label = StringToUTF8("       " + title).c_str();

    bool IsShowGuiCopy = IsShowGui.load(memory_order_relaxed);

    Begin(label.c_str(), &IsShowGuiCopy,
        ImGuiWindowFlags_NoResize |  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);

    // 把变化写回 atomic
    if (IsShowGuiCopy != IsShowGui.load(memory_order_relaxed))
        IsShowGui.store(IsShowGuiCopy, memory_order_relaxed);

    SetWindowSize({ 800, 500 });
    SetWindowPos({ 0, 0 });
    PopStyleVar();

    //=======================标题右键菜单===========================
    constexpr ImRect title_rect(0, 0, 800, 32);
    static bool IsShowMenu = false;
    if (IsMouseDown(ImGuiMouseButton_Right) && title_rect.Contains(io.MousePos))
        IsShowMenu = true;
    else if (IsMouseDown(ImGuiMouseButton_Left) && !title_rect.Contains(io.MousePos))
        IsShowMenu = false;

    if (IsShowMenu)
    {
        if (BeginPopupContextItem())
        {
            static bool IsTopMost = false;
            if (MenuItem(config.UseChinese ? (IsTopMost ? u8"取消置顶" : u8"置顶") : (IsTopMost ? "Cancel Topmost" : "Topmost")))
            {
                IsTopMost = !IsTopMost;
                SetWindowPos(GCHWND, (IsTopMost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
            }
            Separator();
            EndPopup();
        }
    }

    static atomic<bool> IsBgImageRefresh = false;
    if (HasBgImage)
    {
        int w = 0;
        int h = 0;
        static Texture2D texture = nullptr;
        if (IsBgImageRefresh || texture == nullptr)
        {
            texture = LoadTextureFromFile(Pics_Background, true, &w, &h); // 禁用缓存，手动管理
            IsBgImageRefresh = false;
        }
        ImDrawList* draw_list = GetWindowDrawList();
        if (!BgAutoResize)
        {
            // 计算图片缩放比例，让图片在缩放的同时不被拉伸，超过窗口的就不管了
            const float WindowRate = (float)WindowWidth / (float)WindowHeight;
            h = WindowHeight * (BgScale / 100.0f); // 对齐高度，让高度和窗口相同
            w = (int)(h * WindowRate); // 根据高度的缩放，来计算宽度
            draw_list->AddImage((ImTextureID)texture, ImVec2(BgOffsetX, BgOffsetY), ImVec2(w + BgOffsetX, h + BgOffsetY));
        }
        else
            draw_list->AddImage((ImTextureID)texture, ImVec2(0, 0), ImVec2(WindowWidth, WindowHeight));
        
        float BgImageAlpha = 1.0f - (BgLightness / 100.0f);
        if (BgImageAlpha > 1.0f) BgImageAlpha = 1.0f;
        if (BgImageAlpha < 0.0f) BgImageAlpha = 0.0f;
        draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(WindowWidth, WindowHeight), GetColorU32(ImVec4(0, 0, 0, BgImageAlpha)));
    }
    
    static bool IsShowGui_Backup = IsShowGui;
    if (IsShowGui_Backup != IsShowGui)
    {
        ShowWindow(GCHWND, IsShowGui);
        if (IsShowGui)
            SetForegroundWindow(GCHWND);
    }

    //-----------------------动画处理---------------------------
    static bool IsAnime = false;
    {
        static const ImVec2 ButtonSize = { 80, 40 };
        static const float ButtonPosX = ButtonSize.x + 10;
       
        float speed = 0.15f * (1.0f - GetIO().DeltaTime);
        static float AnimPos = 0;

        const float PosY = 84 + 32 + (50 * (NextTab - 1));
        
        AnimPos = ImLerp<float>(AnimPos, PosY, speed);
        
        GetWindowDrawList()->
            AddRectFilled(ImVec2(10, AnimPos), ImVec2(ButtonPosX, AnimPos + ButtonSize.y), GetColorU32(GuiColor[ImGuiCol_Button]), 6);
    }

    //=======================标签页===========================

    PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 0, 0, 0));
    PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(74, 69, 78, 255));
    PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0, 0, 0, 0));
    SetCursorPos({ 10, 84 + 32 });
    static const Texture2D image_texture1 = LoadTextureFromFile(Pics_Home);
    if (ButtonImage((ImTextureID)image_texture1, config.UseChinese ? u8"主页" : "Home", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Hub;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 134 + 32 });
    static const Texture2D image_texture2 = LoadTextureFromFile(Pics_Info);
    if (ButtonImage((ImTextureID)image_texture2, config.UseChinese ? u8"信息" : "Info", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Info;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 184 + 32 });
    static const Texture2D image_texture3 = LoadTextureFromFile(Pics_Setting);
    if (ButtonImage((ImTextureID)image_texture3, config.UseChinese ? u8"设置" : "Setting", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Setting;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 234 + 32 });
    static const Texture2D image_texture4 = LoadTextureFromFile(Pics_Script);
    if (ButtonImage((ImTextureID)image_texture4, config.UseChinese ? u8"脚本" : "Script", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Script;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 284 + 32 });
    static const Texture2D image_texture5 = LoadTextureFromFile(Pics_Other);
    if (ButtonImage((ImTextureID)image_texture5, config.UseChinese ? u8"其他" : "Other", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Other;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 334 + 32 });
    static const Texture2D image_texture6 = LoadTextureFromFile(Pics_About);
    if (ButtonImage((ImTextureID)image_texture6, config.UseChinese ? u8"关于" : "About", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::About;
        IsAnime = true;
        ClickTabCount++;
    }
    PopStyleColor(3);
    PopStyleVar();

    // 应对外部更改NextTab
    if (VarHasChanged(NextTab, "NextTab"))
        IsAnime = true;

    //=======================动画管理===========================
    static int B_ClickTabCount = ClickTabCount;
    if (B_ClickTabCount != ClickTabCount)
    {
        ResetAnimationPos();
        B_ClickTabCount = ClickTabCount;
    }

    //=======================信息显示===========================
    SetCursorPosX(14);
    {
        shared_lock lock(CurrentCPSMutex);
        Text(u8"CPS:%d | %d", CurrentCPS.left, CurrentCPS.right);
    }
    
    SetCursorPosX(12);
    ImDrawList* DrawList = GetWindowDrawList();
    ImVec2 RectPos = GetCursorPos();
    const ImVec2 RectSize(24, 24);
    ImRect rect(RectPos, ImVec2(RectPos.x + RectSize.x, RectPos.y + RectSize.y));

    for (int i = 0; i < 2; i++)
    {
        const bool IsClickerDown = GetKeyDown((i == 0) ? Click::Left : Click::Right);
        DrawList->AddRectFilled(rect.Min, rect.Max, IsClickerDown ? GetColorU32(GuiColor[ImGuiCol_Button]) : GetColorU32(GuiColor[ImGuiCol_FrameBg]), 6.0f);
        DrawList->AddRect(rect.Min, rect.Max, GetColorU32(GuiColor[ImGuiCol_Border]), 5.0f); // 圆角不使用 6 的原因是，在相同圆角值时AddRectFilled的圆角小一点
        DrawList->AddText(ImVec2(rect.Min.x + 7, rect.Min.y + 2), IsClickerDown ? GetColorU32(GuiColor[ImGuiCol_TextDisabled]) : GetColorU32(GuiColor[ImGuiCol_Text]), (i == 0) ? "L" : "R");
        
        if ((IsClickering.left && i == 0) || (IsClickering.right && i == 1))
            DrawList->AddRect(ImVec2(rect.Min.x - 1, rect.Min.y - 1), ImVec2(rect.Max.x + 1, rect.Max.y + 1), GetColorU32((ImVec4)ImColor(176, 51, 205, 255)), 5.5f, 0, 3.0f);

        if ((!config.EnableLeftClicker && i == 0) || (!config.EnableRightClicker && i == 1))
            DrawList->AddRectFilled(rect.Min, rect.Max, GetColorU32((ImVec4)ImColor(56, 56, 56, 120)), 5.5f);

        //移动矩形
        rect.Translate(ImVec2(RectSize.x + 20, 0));
    }

    {
        static bool IsShowDebugInfo = false;
        if (IsKeyPressed(ImGuiKey_F3))
            IsShowDebugInfo = !IsShowDebugInfo;

        if (IsShowDebugInfo)
        {
            SetCursorPos({ RectPos.x, RectPos.y + 16 });
            Text("%.1f FPS", io.Framerate);
            {
                shared_lock lock(HumanCPSMutex);
                Text("Human:%d|%d", HumanCPS.left, HumanCPS.right);
            }
        }
    }

    //=======================菜单显示===========================
    SetCursorPos(ImVec2(100, 6 + 32));
    PushStyleColor(ImGuiCol_ChildBg, GuiColor[ImGuiCol_WindowBg]);
    BeginChild(u8"##PaperTab", ImVec2(696, 456), 1);
    PopStyleColor();

    if (ClickTabCount >= 50)
    {
        if (MessageBox(config.UseChinese ? u8"不要在玩了~~都快被玩坏掉了~~\n你这么闲为什么非得玩我呢~" : "I hate it, don't play anymore. The GC-Clicker is not a toy", u8"彩蛋", MB_OK) != -1)
            ClickTabCount = 0;
    }

    //=======================标签页内容===========================
    const static ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    EndAnimation();
    switch (CurrentTab)
    {
        case Tab::Hub:
        {
            TabsText((ICON_FA_HOUSE + (config.UseChinese ? u8"主页"s : "Home"s)).c_str());

            // 修复：原代码为 static const int，会把 ULONGLONG 差值截断为 32 位 int（>24.8天即溢出），
            // 且 InitSuccessTime 可能尚未被 InitGC 写入（==0），0-StartTime 无符号下溢成巨大值。
            // 改为加锁读取 + ULONGLONG 类型 + 未初始化时实时计算。
            ULONGLONG StartupTime = 0;
            {
                shared_lock lock(InitSuccessTimeMutex);
                if (InitSuccessTime != 0 && InitSuccessTime >= StartTime)
                    StartupTime = InitSuccessTime - StartTime;
            }

            static string DailyMotto = DailyMottos[RandGentor.Random(0, 47)];
            Text(config.UseChinese ? u8"&c本次启动耗时" : "&cStartup time");
            SameLine();
            Text(config.UseChinese ? u8"&a%llu 毫秒" : "&a%llu ms", StartupTime);
            SameLine();
            Text(DailyMotto.c_str());

            /*
            BeginChild("##MarkdownView", ImVec2(677, 230), true);
            {
                MarkdownRender.print(NoticeStr.c_str(), NoticeStr.c_str() + NoticeStr.size());
            }
            EndChild();
            */
            {
                // 修复：NoticeStr 由 GetInfo 线程写入，主线程每帧读取 c_str() 会悬垂 → 快照拷贝
                shared_lock lock(NoticeMutex);
                InputTextMultiline(u8"##notice", (char*)NoticeStr.c_str(), NoticeStr.size(), ImVec2(677, -1), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_NoHorizontalScroll);
            }
            break;
        }
        case Tab::Info:
        {
            TabsText((ICON_FA_CHART_LINE + (config.UseChinese ? u8"信息"s : "Infomation"s)).c_str());

            BeginTabBar("##Script");

            //-----------------------电脑性能---------------------------
            if (BeginTabItem(config.UseChinese ? u8"电脑性能" : "PC Performance"))
            {
                //-----------------------CPS监控信息---------------------------

                const static ImGuiWindowFlags flags =
                    ImGuiWindowFlags_HorizontalScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoBackground;

                const float PosY = GetCursorPosY();

                PushStyleVar(ImGuiStyleVar_ScrollbarSize, 2);

                vector<float> temp;
                temp.resize(MAX_RECORDS, 0);
                vector<float> tempInstant;
                tempInstant.resize(MAX_RECORDS, 0);

                RecordsCPSMutex.lock();
                copy(RRecordsCPS.begin(), RRecordsCPS.end(), temp.begin());
                rotate(temp.begin(), temp.begin() + RRecordsCPS.size(), temp.end());
                copy(RRecordsInstantCPS.begin(), RRecordsInstantCPS.end(), tempInstant.begin());
                rotate(tempInstant.begin(), tempInstant.begin() + RRecordsInstantCPS.size(), tempInstant.end());
                RecordsCPSMutex.unlock();

                Text(config.UseChinese ? u8"右键CPS监控  &b普通CPS &7/ &6瞬时CPS" : "RCPS monitoring  Normal / Instant");
                const float PosY2 = GetCursorPosY();
                BeginChild("##RRecordsBeginChild", { 250, 120 }, false, flags);
                {
                    // Y 轴上限："正常CPS"与"瞬时CPS"两条曲线共同参与取最大值（谁大用谁），
                    // 最低 12（默认刻度）；下限固定 0，使矩形底部即 X 轴（CPS 不可能为负）。
                    // 每帧重算，随最大值自动变化。
                    float RMaxY = 12.0f;
                    for (const float v : temp)        if (v > RMaxY) RMaxY = v;
                    for (const float v : tempInstant) if (v > RMaxY) RMaxY = v;
                    if (RMaxY > 12.0f) RMaxY *= 1.1f;   // 有数据时顶部留 10% 余量，避免峰值贴顶

                    // 双曲线：蓝色=正常CPS（1秒窗口统计）；橙色=瞬时CPS（最近两次点击间隔推算）
                    // NoInputs：禁用 ImPlot 自身的缩放/平移，把鼠标事件让给外层 Child 的水平滚动条
                    if (ImPlot::BeginPlot("##RRecordsPlot", ImVec2((float)MAX_RECORDS, 110), ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs))
                    {
                        // Y 轴加 LockMin/LockMax，确保范围严格由上面计算值决定，不被自动适配或交互覆盖
                        ImPlot::SetupAxes(nullptr, nullptr,
                            ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks,
                            ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
                        ImPlot::SetupAxesLimits(0, MAX_RECORDS, 0, RMaxY, ImPlotCond_Always);
                        ImPlot::SetNextLineStyle(ImVec4(0.07f, 0.49f, 0.73f, 1.0f));
                        ImPlot::PlotLine(config.UseChinese ? u8"正常CPS" : "Normal CPS", temp.data(), MAX_RECORDS);
                        ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                        ImPlot::PlotLine(config.UseChinese ? u8"瞬时CPS" : "Instant CPS", tempInstant.data(), MAX_RECORDS);
                        ImPlot::EndPlot();
                    }
                    // 自动跟随最新：仅当滚动条本就贴在最右时才跟随；
                    // 用户向左拖动查看历史后暂停跟随，拖回最右则恢复。
                    const float scrollMaxX = GetScrollMaxX();
                    const float scrollX = GetScrollX();
                    if (scrollMaxX <= 0.0f || scrollX >= scrollMaxX - 0.5f)
                        SetScrollHereX(1.0f);
                }
                EndChild();

                SameLine();
                SetCursorPosY(PosY);

                const float PosX = GetCursorPosX();

                temp.clear();
                temp.resize(MAX_RECORDS, 0);
                tempInstant.clear();
                tempInstant.resize(MAX_RECORDS, 0);

                {
					lock_guard lock(RecordsCPSMutex);
                    copy(LRecordsCPS.begin(), LRecordsCPS.end(), temp.begin());
                    rotate(temp.begin(), temp.begin() + LRecordsCPS.size(), temp.end());
                    copy(LRecordsInstantCPS.begin(), LRecordsInstantCPS.end(), tempInstant.begin());
                    rotate(tempInstant.begin(), tempInstant.begin() + LRecordsInstantCPS.size(), tempInstant.end());
                }

                TextColored(ImVec4(0.07f, 0.49f, 0.73f, 1.0f), u8"CPU");
                SameLine();
                Text(u8"/");
                SameLine();
                TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), u8"GPU");
                SameLine();
                Text(config.UseChinese ? u8"左键CPS监控" : "LCPS Monitoring");

                SetCursorPos({ PosX, PosY2 });
                BeginChild("##LRecordsBeginChild", { 250, 120 }, false, flags);
                {
                    // Y 轴上限："正常CPS"与"瞬时CPS"两条曲线共同参与取最大值（谁大用谁），
                    // 最低 12（默认刻度）；下限固定 0，使矩形底部即 X 轴（CPS 不可能为负）。
                    // 每帧重算，随最大值自动变化。
                    float LMaxY = 12.0f;
                    for (const float v : temp)        if (v > LMaxY) LMaxY = v;
                    for (const float v : tempInstant) if (v > LMaxY) LMaxY = v;
                    if (LMaxY > 12.0f) LMaxY *= 1.1f;   // 有数据时顶部留 10% 余量，避免峰值贴顶

                    // 双曲线：蓝色=正常CPS（1秒窗口统计）；橙色=瞬时CPS（最近两次点击间隔推算）
                    // NoInputs：禁用 ImPlot 自身的缩放/平移，把鼠标事件让给外层 Child 的水平滚动条
                    if (ImPlot::BeginPlot("##LRecordsPlot", ImVec2((float)MAX_RECORDS, 110), ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs))
                    {
                        // Y 轴加 LockMin/LockMax，确保范围严格由上面计算值决定，不被自动适配或交互覆盖
                        ImPlot::SetupAxes(nullptr, nullptr,
                            ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks,
                            ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
                        ImPlot::SetupAxesLimits(0, MAX_RECORDS, 0, LMaxY, ImPlotCond_Always);
                        ImPlot::SetNextLineStyle(ImVec4(0.07f, 0.49f, 0.73f, 1.0f));
                        ImPlot::PlotLine(config.UseChinese ? u8"正常CPS" : "Normal CPS", temp.data(), MAX_RECORDS);
                        ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                        ImPlot::PlotLine(config.UseChinese ? u8"瞬时CPS" : "Instant CPS", tempInstant.data(), MAX_RECORDS);
                        ImPlot::EndPlot();
                    }
                    // 自动跟随最新：仅当滚动条本就贴在最右时才跟随；
                    // 用户向左拖动查看历史后暂停跟随，拖回最右则恢复。
                    const float scrollMaxX = GetScrollMaxX();
                    const float scrollX = GetScrollX();
                    if (scrollMaxX <= 0.0f || scrollX >= scrollMaxX - 0.5f)
                        SetScrollHereX(1.0f);
                }
                EndChild();

                PopStyleVar(); //ImGuiStyleVar_ScrollbarSize

                SameLine();

                RecordsCPSMutex.lock();
                int Rmax = 0;
                int Lmax = 0;
                if (!RRecordsCPS.empty())
                {
                    auto rtemp = max_element(RRecordsCPS.begin(), RRecordsCPS.end());
                    Rmax = static_cast<int>(*rtemp);
                }
                if (!LRecordsCPS.empty())
                {
                    auto ltemp = max_element(LRecordsCPS.begin(), LRecordsCPS.end());
                    Lmax = static_cast<int>(*ltemp);
                }
                RecordsCPSMutex.unlock();

                const float PosX2 = GetCursorPosX();

                Text(config.UseChinese ? u8"右键最高CPS：%d" : "Highest RCPS: %d", Rmax);
                SameLine();
                SetCursorPos({ PosX2, GetCursorPosY() + 19 });
                Text(config.UseChinese ? u8"左键最高CPS：%d" : "Highest LCPS: %d", Lmax);

                //-----------------------CPU/GPU/内存监控信息---------------------------
                {
                    shared_lock lock(PerformanceMutex);

                    DrawRoundProgressBar(config.UseChinese ? u8"CPU" : "CPU", (CpuRates.empty() ? 0.0f : CpuRates.back()), ImVec2(150, 160), ImVec4(0.22f, 0.50f, 0.88f, 1.0f));
                    SameLine();
                    DrawRoundProgressBar(config.UseChinese ? u8"GPU" : "GPU", (GpuRates.empty() ? 0.0f : GpuRates.back()), ImVec2(150, 160), ImVec4(0.20f, 0.76f, 0.58f, 1.0f));
                    SameLine();
                    DrawRoundProgressBar(config.UseChinese ? u8"内存" : "Memory", (MemRates.empty() ? 0.0f : MemRates.back()), ImVec2(150, 160), ImVec4(0.98f, 0.62f, 0.15f, 1.0f));

                    //-----------------------CPU/GPU---------------------------

                    const int PosY3 = GetCursorPosY();
                    TextColored((ImVec4)ImColor(17, 125, 187, 255), u8"CPU");
                    SameLine();
                    Text(u8"/");
                    SameLine();
                    TextColored((ImVec4)ImColor(222, 18, 20, 255), u8"GPU");
                    SameLine();
                    Text(config.UseChinese ? u8"占用率监控" : "CPS Monitoring");

                    //不用const staitc的，是因为这玩意任意错位
                    float PosY4 = GetCursorPosY();
                    BeginChild("##CPURecordsBeginChild", ImVec2(250, 120), false, flags);
                    {
                        temp.clear();
                        temp.resize(MAX_PERF_RECORDS, 0);
                        copy(CpuRates.begin(), CpuRates.end(), temp.begin());
                        rotate(temp.begin(), temp.begin() + CpuRates.size(), temp.end());

                        const static ImVec2 Pos = GetCursorPos();
                        PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor(17, 125, 187, 255));
                        PlotLines(u8"##CPURecordsPL", temp.data(), MAX_PERF_RECORDS, 0, u8"", 0, 1.0f, ImVec2(MAX_PERF_RECORDS, 110));
                        PopStyleColor();

                        temp.clear();
                        temp.resize(MAX_PERF_RECORDS, 0);
                        copy(GpuRates.begin(), GpuRates.end(), temp.begin());
                        rotate(temp.begin(), temp.begin() + GpuRates.size(), temp.end());

                        SetCursorPos(Pos);
                        PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor(222, 18, 20, 255));
                        PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0, 0, 0, 0));
                        PlotLines(u8"##GPURecordsPL", temp.data(), MAX_PERF_RECORDS, 0, u8"", 0.0f, 1.0f, ImVec2(MAX_PERF_RECORDS, 110));
                        PopStyleColor(2);

                        SetScrollHereX(1);
                    }
                    EndChild();

                    SameLine();
                    float PosX4 = GetCursorPosX();

                    //-----------------------Mem---------------------------

                    temp.clear();
                    temp.resize(MAX_RECORDS, 0);

                    copy(MemRates.begin(), MemRates.end(), temp.begin());
                    rotate(temp.begin(), temp.begin() + MemRates.size(), temp.end());

                    SetCursorPosY(PosY3);

                    Text(config.UseChinese ? u8"内存占用率监控" : "Memory Usage Monitoring");
                    SetCursorPos(ImVec2(PosX4, PosY4));
                    BeginChild("##MemRecordsBeginChild", ImVec2(250, 120), false, flags);
                    {
                        PlotLines(u8"##MemRecordsCPS", temp.data(), MAX_RECORDS, 0, u8"", 0, FLT_MAX, { MAX_RECORDS, 110 });
                        SetScrollHereX(1);
                    }
                    EndChild();
                }

                EndTabItem();
            }

            if (BeginTabItem(config.UseChinese ? u8"统计信息" : "Statistics"))
            {
                //=======================统计信息===========================

                const ULONGLONG ThisUseTime = (GetNowMs() - StartTime) / 60000; //60000MS为1分钟
                //-----------------------点击次数-----------------------
                BeginCard(config.UseChinese ? u8"总揽" : "Overall", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    Text(config.UseChinese ? u8"手动左键点击次数：" : "Manual left click count");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%llu", app_status.MouseClickCount.left);

                    Text(config.UseChinese ? u8"手动右键点击次数：" : "Manual right click count");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%llu", app_status.MouseClickCount.right);

                    Text(config.UseChinese ? u8"连点左键的次数：" : "Clicker left count:");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%llu", app_status.ClickerClickCount.left);

                    Text(config.UseChinese ? u8"连点右键的次数：" : "Clicker right count:");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%llu", app_status.ClickerClickCount.right);

                    Text(config.UseChinese ? u8"使用GC的时长：" : "Duration of using GC:");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%Lf", app_status.UsageDuration);

                    Text(config.UseChinese ? u8"本次使用GC的时长：" : "The duration of using GC this time:");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%Lf", ThisUseTime);

                    //-----------------------点击次数的比值-----------------------
                    Text(config.UseChinese ? u8"GC与手动点击次数的比值" : "The ratio of GC to manual clicks");
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    Text(config.UseChinese ? u8"左键" : "Left button");
					SameLine();
					SetCursorPosX(300);
                    ProgressBar((double)app_status.ClickerClickCount.left / max(app_status.ClickerClickCount.left + app_status.MouseClickCount.left, 1), ImVec2(300, 20));

                    Text(config.UseChinese ? u8"右键" : "Right button");
                    SameLine();
                    SetCursorPosX(300);
                    ProgressBar((double)app_status.ClickerClickCount.right / max(app_status.ClickerClickCount.right + app_status.MouseClickCount.right, 1), ImVec2(300, 20));

                    Text(config.UseChinese ? u8"左右键点击次数比值" : "Ratio of LB and RB clicks");
                    SameLine();
                    SetCursorPosX(300);
                    ProgressBar((double)app_status.ClickerClickCount.left / max(app_status.ClickerClickCount.left + app_status.ClickerClickCount.right, 1), ImVec2(300, 20));

                    //-----------------------鼠标平均点击次数-----------------------
                    Text(config.UseChinese ? u8"每分钟手动平均点击次数：" : "Manual average clicks per minute:");
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    Text(config.UseChinese ? u8"左键" : "Left button");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%.1f", (double)app_status.MouseClickCount.left / max(ThisUseTime, 1));

                    Text(config.UseChinese ? u8"右键" : "Right button");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%.1f", (double)app_status.MouseClickCount.right / max(ThisUseTime, 1));

                    //-----------------------GC平均点击次数-----------------------
                    Text(config.UseChinese ? u8"每分钟连点器平均点击次数：" : "Average clicker clicks per minute");
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    Text(config.UseChinese ? u8"左键" : "Left button");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%.1f", (double)app_status.ClickerClickCount.left / max(ThisUseTime, 1));

                    Text(config.UseChinese ? u8"右键" : "Right button");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%.1f", (double)app_status.ClickerClickCount.right / max(ThisUseTime, 1));
                }
                EndCard();

                //-----------------------GC连点与鼠标点击次数的平均比值-----------------------
                BeginCard(config.UseChinese ? u8"手动点击的比值" : "Ratio of manual clicks to GC", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    const float PlotSize = 200.0f;
                    static const ImPlotFlags PieFlag = ImPlotFlags_Equal | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs;
                    const float PosY = GetCursorPosY();

                    {
                        float StartX = GetCursorPosX();

                        // 计算标题宽度并设置居中
                        const char* LeftTitle = config.UseChinese ? u8"左键" : "Left Button";
                        SetCursorPosX(StartX + (PlotSize - CalcTextSize(LeftTitle).x) / 2);
                        Text(LeftTitle);

                        SetCursorPos(ImVec2(StartX, PosY + 20.0f));

                        // 绘制饼图
                        const unsigned long long values[] = { app_status.ClickerClickCount.left, app_status.MouseClickCount.left };
                        const char* labels[] = {
                            (char*)(config.UseChinese ? u8"连点器" : "Clicker"),
                            (char*)(config.UseChinese ? u8"手动" : "Manual")
                        };

                        if (ImPlot::BeginPlot("##stat_gml", ImVec2(PlotSize, PlotSize), PieFlag))
                        {
                            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                            ImPlot::SetupAxesLimits(0, 1, 0, 1);
                            ImPlot::PlotPieChart(labels, values, 2, 0.5, 0.5, 0.4, "%.0f", 180, ImPlotPieChartFlags_Normalize);
                            ImPlot::EndPlot();
                        }
                    }

                    SameLine();

                    {
                        float StartX = GetCursorPosX();

                        // 计算标题宽度并设置居中
                        const char* RightTitle = config.UseChinese ? u8"右键" : "Right Button";
                        SetCursorPos(ImVec2(StartX + (PlotSize - CalcTextSize(RightTitle).x) / 2, PosY));
                        Text(RightTitle);

                        SetCursorPos(ImVec2(StartX, PosY + 20));

                        // 绘制饼图
                        const unsigned long long values[] = { app_status.ClickerClickCount.right, app_status.MouseClickCount.right };
                        const char* labels[] = {
                            (char*)(config.UseChinese ? u8"连点器" : "Clicker"),
                            (char*)(config.UseChinese ? u8"手动" : "Manual")
                        };

                        if (ImPlot::BeginPlot("##stat_gmr", ImVec2(PlotSize, PlotSize), PieFlag))
                        {
                            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                            ImPlot::SetupAxesLimits(0, 1, 0, 1);
                            ImPlot::PlotPieChart(labels, values, 2, 0.5, 0.5, 0.4, "%.0f", 180, ImPlotPieChartFlags_Normalize);
                            ImPlot::EndPlot();
                        }
                    }
                }
                EndCard();

                EndTabItem();
            }
            EndTabBar();
            
            break;
        }
        case Tab::Setting: //-----------------------设置---------------------------
        {
            static const Texture2D ImageItembar = LoadTextureFromFile(Pics_Itembar, false);
            static const Texture2D ImageSelected = LoadTextureFromFile(Pics_Selected, false);

            const ImVec2 Pos = GetCursorPos();
            TabsText((ICON_FA_GEAR + (config.UseChinese ? u8"设置"s : "Setting"s)).c_str());

            const ImVec2 Pos2 = GetCursorPos();
            // 配置预设下拉列表（位于“保存”按钮的左侧）
            const float presetW = 110.0f;
            SetCursorPos(ImVec2(Pos.x + 540 - presetW - 8, Pos.y + 10));
            {
                string presetName;
                const int presetHit = PresetDropdownMenu("PresetMenu", presetW, ImVec2(presetW, 40), presetName);
                if (presetHit == PRESET_ACTION_SWITCH)
                {
                    if (presetName != PresetConfig::CurrentName)
                    {
                        // 切换丢到后台线程执行，UI 线程不再长时间持有 ConfigAccessMutex。
                        string FromName;
                        string ToName = presetName;
                        {
                            FromName = PresetConfig::CurrentName;
                            PresetConfig::CurrentName = ToName;
                        }
                        thread th([FromName, ToName]()
                        {
                            SwitchPreset(FromName, ToName);
                            IsReloadClicker = true;   // 载入完成后才通知点击线程保存
                        });
                        th.detach();
                    }
                }
                else if (presetHit == PRESET_ACTION_DELETE)
                {
                    const bool wasCurrent = (presetName == PresetConfig::CurrentName);
                    string delName = presetName;
                    if (wasCurrent)
                        PresetConfig::CurrentName = PresetDefaultName;
                    thread th([delName, wasCurrent]()
                    {
                        LoadConfig(ConfigFile);
                        IsReloadClicker = true;
                        DeletePreset(delName);
                    });
                    th.detach();
                    RenderTip(config.UseChinese ? u8"已删除配置预设" : "Preset deleted", 1.5f, PT_INFO);
                }
                else if (presetHit == PRESET_ACTION_ADD)
                {
                    PresetConfig::ShowAdd = true;
                    PresetConfig::NameBuf[0] = '\0';
                    PresetConfig::ErrorTime = 0.0f;
                }
            }

            SetCursorPos(ImVec2(Pos.x + 540, Pos.y + 10));
            if (Button((ICON_FA_FLOPPY_DISK + (config.UseChinese ? u8"保存设置"s : "Save"s)).c_str(), ImVec2(80, 40)))
                IsReloadClicker = true;
            SameLine();

            static const ButtonPopupMenuItem menu_items[] = {
                { ICON_FA_ARROW_ROTATE_LEFT + (config.UseChinese ? u8" 重置配置"s : " Reset"s), true },
                { ICON_FA_FOLDER_OPEN + (config.UseChinese ? u8" 打开配置目录"s : " Explore Config"s), true },
                { ICON_FA_FILE_EXPORT + (config.UseChinese ? u8" 导出当前配置"s : " Export Current Config"s), true },
                { ICON_FA_FILE_IMPORT + (config.UseChinese ? u8" 导入配置"s : " Import Config"s), true }
            };
            const int hit = ButtonPopupMenu(ICON_FA_BARS, "OtherMenuAction", menu_items, IM_ARRAYSIZE(menu_items), 150.0f, ImVec2(40, 40));
            switch (hit)
            {
                case 0:
                {
                    // 修复：原 detach 线程在确认后直接执行 config.~Config()+placement new，
                    // 与 ClickerControl/MouseSoundThread/GetInfo 等线程并发读写 config -> 0xC0000005。
                    // 线程只负责弹窗确认，真正的重建由主线程在下一帧 RenderGui 开头执行。
                    thread FunThread([]()
                    {
                        if (RenderMessageBox(config.UseChinese ? u8"真的要重置当前配置吗？" : "Are you sure\nto reset the current config?", config.UseChinese ? u8"提示" : "TIPS", MB_OKCANCEL, true) == IDOK)
                            IsNeedResetConfig = true;
                    });
                    FunThread.detach();
                    break;
                }
                case 1:
                {
                    thread t([](){ ShellExecute(NULL, "open", ConfigPath.c_str(), NULL, NULL, SW_SHOW); });
                    t.detach();
                    break;
                }
                case 2:
                {
                    // 打开文件保存对话框，保存当前预设配置的文件
                    // 不使用异步，因为保持逻辑统一，下面都没使用异步，那在这里有什么理由使用呢？
                    const string TargetFile = GetPresetFile(PresetConfig::CurrentName);

                    OPENFILENAME ofn;
                    char szFileName[MAX_PATH]{};
                    strncpy_s(szFileName, sizeof(szFileName), (UTF8ToString(PresetConfig::CurrentName) + ".ini").c_str(), _TRUNCATE);

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = NULL; // 可以设置为你的窗口句柄
                    ofn.lpstrFilter = "GC Config Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.hwndOwner = GCHWND;
                    // 设置对话框标志，例如提示覆盖等
                    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
                    ofn.lpstrDefExt = "ini"; // 默认扩展名

                    if (GetSaveFileName(&ofn))
                    {
                        string destinationPath = ofn.lpstrFile; // 用户选择的完整目标路径
                        if (CopyFile(TargetFile.c_str(), destinationPath.c_str(), FALSE) == FALSE)
                        {
                            DWORD error = GetLastError();
                            PrintLog("[Preset] 导出配置失败，错误码: " + to_string(error), PL_ERROR);
                        }
                    }
                    break;
                }
                case 3:
                {
                    // 打开文件选择对话框，选择一个配置文件导入
					// 但不使用异步，而是同时设置hwndOwner = GCHWND，选择文件时不能操作GC窗口，这样可以避免线程问题
                    OPENFILENAME ofn;
                    char szFileName[MAX_PATH]{};
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = NULL; // 可以设置为你的窗口句柄
                    ofn.lpstrFilter = "GC Config Files (*.ini)\0*.ini\0";
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.hwndOwner = GCHWND;
                    // 设置对话框标志，例如提示覆盖等
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                    ofn.lpstrDefExt = "ini"; // 默认扩展名
                    if (GetOpenFileName(&ofn))
                    {
                        string SourcePath = ofn.lpstrFile; // 用户选择的完整源路径
                        string PresetName = [&SourcePath]() -> string {
                            size_t pos = SourcePath.find_last_of("\\");
                            size_t dotPos = SourcePath.find_last_of(".");
                            if (pos != string::npos)
                                return SourcePath.substr(pos + 1, dotPos - pos - 1); // 获取文件名部分
                            else
                                return SourcePath.substr(0, 12); // 随便使用名字，但要限制长度为12，超过部分直接截断
                        }();
                        if (CopyFile(SourcePath.c_str(), GetPresetFile(PresetName).c_str(), TRUE)) // 设置TRUE表示不覆盖已经存在的目标文件
                        {
                            PresetConfig::Presets.push_back(PresetName);
                            SwitchPreset(PresetConfig::CurrentName, PresetName);
                            RenderTip(config.UseChinese ? u8"导入配置成功" : "Preset imported successfully", 2.0f, PT_INFO);
                        }
                        else
                        {
                            DWORD error = GetLastError();
                            if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) // 这样就不用遍历寻找列表有没有存在的了，提高性能
                            {
                                RenderTip((config.UseChinese ? u8"导入配置失败，名称已存在" : "Failed to import preset, name already exists"), 3.0f, PT_ERROR);
                                PrintLog("[Preset] 导入配置失败，名称已存在: " + PresetName, PL_ERROR);
                            }
                            else
                            {
                                RenderTip((config.UseChinese ? u8"导入配置失败" : "Failed to import preset") + " ("s + to_string(error) + ")", 3.0f, PT_ERROR);
                                PrintLog("[Preset] 导入配置失败，错误码: " + to_string(error), PL_ERROR);
                            }
                        }
                    }
                    break;
                }
            }

            //=======================新建配置预设弹窗===========================
            if (BeginPopupWindow(config.UseChinese ? u8"新建配置预设" : "New Config Preset", &PresetConfig::ShowAdd, ImVec2(400, 200), POPUP_ANIM_FADE, POPUP_OPT_HIDE_CLOSE | POPUP_OPT_SHADOW))
            {
                Text(config.UseChinese ? u8"请输入配置预设名称：" : "Enter a preset name:");

                SetNextItemWidth(-1);
                InputText("##preset_name", PresetConfig::NameBuf, IM_ARRAYSIZE(PresetConfig::NameBuf));

                // 右下角操作按钮（网页风格布局）
                const ImGuiStyle& pstyle = GetStyle();
                const float btnW = 92.0f, btnH = 34.0f;
                const float totalW = btnW * 2.0f + pstyle.ItemSpacing.x;
                const float btnY = ImGui::GetWindowHeight() - btnH - 16.0f;

                // 内联校验提示，显示在输入框下方（3 秒后自动淡出）
                if (PresetConfig::ErrorTime > 0.0f)
                {
                    PresetConfig::ErrorTime -= ImGui::GetIO().DeltaTime;
                    const float errAlpha = ImClamp(PresetConfig::ErrorTime / 0.5f, 0.0f, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.93f, 0.42f, 0.42f, errAlpha));
                    ImGui::SetCursorPosY(btnY - ImGui::GetTextLineHeight() - 6.0f);
                    ImGui::TextUnformatted(PresetConfig::ErrorMsg.c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::SetCursorPosY(btnY);
                ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - totalW);

                if (Button(config.UseChinese ? u8"确定" : "OK", ImVec2(btnW, btnH)))
                {
                    string NameStr = PresetConfig::NameBuf;
                    const size_t s = NameStr.find_first_not_of(" \t");
                    const size_t e = NameStr.find_last_not_of(" \t");
                    NameStr = (s == string::npos) ? string() : NameStr.substr(s, e - s + 1);

                    if (NameStr.empty())
                    {
                        PresetConfig::ErrorMsg = config.UseChinese ? u8"名称不能为空" : "Name cannot be empty";
                        PresetConfig::ErrorTime = 3.0f;
                    }
                    if (NameStr.length() > 13)
                    {
                        PresetConfig::ErrorMsg = config.UseChinese ? u8"名称太长" : "Name is too long";
                        PresetConfig::ErrorTime = 3.0f;
                    }
                    else
                    {
                        if (AddPreset(NameStr))
                        {
                            // 新建后的切换同样放后台线程。
                            string FromName, ToName = NameStr;
                            {
                                FromName = PresetConfig::CurrentName;
                                PresetConfig::CurrentName = ToName;
                            }
                            thread th([FromName, ToName]()
                            {
                                SwitchPreset(UTF8ToString(FromName), UTF8ToString(ToName));
                                IsReloadClicker = true;
                            });
                            th.detach();
                            RenderTip(config.UseChinese ? u8"创建预设配置成功" : "Preset created successfully", 2.0f, PT_INFO);
                            PresetConfig::ShowAdd = false;
                        }
                        else
                        {
                            PresetConfig::ErrorMsg = config.UseChinese ? u8"名称已存在或无效" : "Name already exists or is invalid";
                            PresetConfig::ErrorTime = 3.0f;
                        }
                    }
                }
                SameLine();
                if (Button(config.UseChinese ? u8"取消" : "Cancel", ImVec2(btnW, btnH)))
                    PresetConfig::ShowAdd = false;

                EndPopupWindow();
            }


            SetCursorPos(Pos2);

            BeginTabBar("##Script");

            //-----------------------基本设置---------------------------
            if (BeginTabItem((ICON_FA_SLIDERS + (config.UseChinese ? u8"基本设置"s : "Basic Setting"s)).c_str()))
            {
                BeginCard(config.UseChinese ? u8"基本" : "Basic", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    //-------------------------CPS设置/快捷键设置----------------------------
                    if (VarHasChanged(config.UseBrutalMode, "UseBrutalMode"))
                    {
                        if (config.UseBrutalMode)
                        {
                            MessageBeep(MB_ICONWARNING);
                            RenderTip(config.UseChinese ? u8"暴力模式下，可能会加大CPU的开销！" : "Brutal mode may cause a lot of computer CPU performance!", 2.0f, PT_WARNING);
                            config.RCPS += 60;
                            config.LCPS += 60;
                            config.UseRFluctuation = false;
                            config.UseLFluctuation = false;
                            config.UseLEmptyRate = false;
                            config.UseREmptyRate = false;
                        }
                        else
                        {
                            config.RCPS = DEFAULT_CPS;
                            config.LCPS = DEFAULT_CPS;
                        }
                    }

                    Text(config.UseChinese ? u8"右键连点按键" : "RClicker Key");
                    SameLine();
                    SetNextItemWidth(100);
                    {
                        int LastKey = config.RKEY;
                        if (InputKey("##RClickKey", &config.RKEY, config.UseChinese ? u8"请按下按键" : "Press a key"))
                        {
                            if (LastKey == 2)
                            {
                                config.RClickerMode |= C_BUTTONMODE;
                                config.RClickerMode &= ~C_MOUSESMODE;
                                RenderTip(config.UseChinese ? u8"已自动切换为 &6默认模式" : "Automatically switched to &6Default Mode", 3.0f, PT_INFO);
                            }
                            if (config.RKEY == 2)
                            {
                                config.RClickerMode |= C_MOUSESMODE;
                                config.RClickerMode &= ~C_BUTTONMODE;
                                RenderTip(config.UseChinese ? u8"已自动切换为 &6鼠标模式" : "Automatically switched to &6Mouse Mode", 3.0f, PT_INFO);
                            }
                        }
                    }
                    if (!(config.RClickerMode & C_BUTTONMODE || config.RClickerMode & C_SWITCHMODE) && 
                        (config.RClickerMode & C_MOUSESMODE || config.RClickerMode & C_ASSISTMODE))
                        AttachTooltip(config.UseChinese ? u8"&b辅助模式 &f或 &b鼠标模式&f不需要设置快捷键" : "AssistMode or MouseMode does not need to set a shortcut key");
                    else
                        AttachTooltip(config.UseChinese ? u8"如果需要实现 按住鼠标 来连点\n可以使用鼠标模式。" : "If you need to implement holding the mouse to click\nYou can use the mouse mode.");
                   
                    SameLine();

                    Text(config.UseChinese ? u8"右键CPS" : "RClicker CPS");
                    AttachTooltip(config.UseChinese ? u8"CPS指每秒连点次数，越高越快，\n一般右键设置 20CPS 就可以。" : "CPS means the number of clicks per second, the higher the faster, \ngenerally the left key sets 14CPS can be used.");
                    SameLine();
                    SetNextItemWidth(350);
                    if (config.UseRFluctuation && !config.UseBrutalMode)
                    {
                        static float MinCps = config.RCPS - config.RFluctuationCPS;
                        static float MaxCps = config.RCPS + config.RFluctuationCPS;
                        if (RangeSliderFloat(u8"##rcps", MIN_CPS, MAX_CPS, MinCps, MaxCps, "%.1fCPS"))
                        {
                            float mid = MinCps + (MaxCps - MinCps) / 2;
                            config.RCPS = round(mid);
                            config.RFluctuationCPS = round(mid - MinCps);
                            ClickerDynamicDelay.right = 1000.0f / static_cast<int>(max(config.RCPS, 0));
                        }
                    }
                    else
                    {
                        if (SliderInt(u8"##rcps", &config.RCPS, MIN_CPS, config.UseBrutalMode ? 1000 : MAX_CPS, (config.RCPS >= 1000) ? (config.UseChinese ? u8"极限" : "Max") : "%dCPS", ImGuiSliderFlags_AlwaysClamp))
                        {
                            if (config.RCPS >= 1000)
                            {
                                if (config.RCPS < 99999)
                                    config.RCPS = 999;
                                else
                                    config.RCPS = 99999;
                            }
                        }
                    }

                    Text(config.UseChinese ? u8"左键连点按键" : "LClicker Key");
                    SameLine();
                    SetNextItemWidth(100);
                    {
                        int LastKey = config.RKEY;
                        if (InputKey("##LClickKey", &config.LKEY, config.UseChinese ? u8"请按下按键" : "Press a key"))
                        {
                            if (LastKey == 2)
                            {
                                config.LClickerMode |= C_BUTTONMODE;
                                config.LClickerMode &= ~C_MOUSESMODE;
                                RenderTip(config.UseChinese ? u8"已自动切换为 &6默认模式" : "Automatically switched to &6Default Mode", 3.0f, PT_INFO);
                            }
                            if (config.LKEY == 1)
                            {
                                config.LClickerMode |= C_MOUSESMODE;
                                config.LClickerMode &= ~C_BUTTONMODE;
                                RenderTip(config.UseChinese ? u8"已自动切换为 &6鼠标模式" : "Automatically switched to &6Mouse Mode", 3.0f, PT_INFO);
                            }
                        }
                    }
                    if (!(config.LClickerMode & C_BUTTONMODE || config.LClickerMode & C_SWITCHMODE) &&
                        (config.LClickerMode & C_MOUSESMODE || config.LClickerMode & C_ASSISTMODE))
                        AttachTooltip(config.UseChinese ? u8"&b辅助模式 &f或 &b鼠标模式&f不需要设置快捷键" : "AssistMode or MouseMode does not need to set a shortcut key");
                    else
                        AttachTooltip(config.UseChinese ? u8"如果需要实现 按住鼠标 来连点\n可以使用鼠标模式。" : "If you need to implement holding the mouse to click\nYou can use the mouse mode.");

                    SameLine();

                    Text(config.UseChinese ? u8"左键CPS" : "LClicker CPS");
                    AttachTooltip(config.UseChinese ? u8"CPS指每秒连点次数，越高越快，\n一般左键设置 14CPS 就可以。" : "CPS means the number of clicks per second, the higher the faster, \ngenerally the right key sets 20CPS can be used.");
                    SameLine();
                    SetNextItemWidth(350);
                    if (config.UseLFluctuation && !config.UseBrutalMode)
                    {
                        static float MinCps = config.LCPS - config.LFluctuationCPS;
                        static float MaxCps = config.LCPS + config.LFluctuationCPS;
                        if (RangeSliderFloat(u8"##lcps", MIN_CPS, MAX_CPS, MinCps, MaxCps, "%.1fCPS"))
                        {
                            float mid = MinCps + (MaxCps - MinCps) / 2;
                            config.LCPS = round(mid);
                            config.LFluctuationCPS = round(mid - MinCps);
                            ClickerDynamicDelay.left = 1000.0f / static_cast<int>(max(config.LCPS, 0));
                        }
                    }
                    else
                    {
                        if (SliderInt(u8"##lcps", &config.LCPS, MIN_CPS, config.UseBrutalMode ? 1000 : MAX_CPS, (config.LCPS >= 1000) ? (config.UseChinese ? u8"极限" : "Max") : "%dCPS", ImGuiSliderFlags_AlwaysClamp))
                        {
                            if (config.LCPS >= 1000)
                            {
                                if (config.LCPS < 99999)
                                    config.LCPS = 999;
                                else
                                    config.LCPS = 99999;
                            }
                        }
                    }

                    Toggle(config.UseChinese ? u8"右键CPS波动" : "RCPS Fluctuation", &config.UseRFluctuation, ToggleFlags);
                    SameLine();
                    Toggle(config.UseChinese ? u8"左键CPS波动" : "LCPS Fluctuation", &config.UseLFluctuation, ToggleFlags);

                    BeginDisabled(!config.UseLFluctuation && !config.UseRFluctuation);
                    {
                        SameLine();
                        static bool IsOpen = false;
                        if (Button(config.UseChinese ? u8"随机曲线分布" : "Random Curve"))
                            IsOpen = !IsOpen;

                        static LRPair<vector<WaveParam>> WaveList;
                        static bool LastOpenState = false;
                        // 打开弹窗时加载已保存的数据
                        if (IsOpen && !LastOpenState)
                        {
                            if (!config.CPSWaves.left.empty())
                                WaveList.left = config.CPSWaves.left;
                            if (!config.CPSWaves.right.empty())
                                WaveList.right = config.CPSWaves.right;
                        }
                        LastOpenState = IsOpen;

                        if (BeginPopupWindow(config.UseChinese ? u8"随机波形设置" : "Rand Wave Settings", &IsOpen))
                        {
                            // 左右独立状态
                            static int CurrentTab = 0;                     // 0=左键, 1=右键
                            static int SelectedWaveIndexLeft = -1;
                            static int SelectedWaveIndexRight = -1;
                            static bool ShowWaveSettingsLeft = false;
                            static bool ShowWaveSettingsRight = false;
                            static bool ShowAppliedCPS = false;

                            if (Button(config.UseChinese ? u8"左键" : "Left", ImVec2(80, 0)))
                                CurrentTab = 0;
                            SameLine();
                            if (Button(config.UseChinese ? u8"右键" : "Right", ImVec2(80, 0)))
                                CurrentTab = 1;
                            SameLine();
                            if (CurrentTab == 0)
                                Text(config.UseChinese ? u8"左键波形设置" : "LB Wave");
                            else
                                Text(config.UseChinese ? u8"右键波形设置" : "RB Wave");

                            {
                                SameLine();
                                const char* BtnText = config.UseChinese ? u8"保存" : "Save";
                                const ImGuiStyle style = GetStyle();
                                const float BtnWidth = 50;
                                SetCursorPosX(GetWindowContentRegionMax().x - BtnWidth - style.FramePadding.x);
                                if (Button(BtnText, ImVec2(BtnWidth, 0)))
                                {
                                    config.CPSWaves.left = WaveList.left;
                                    config.CPSWaves.right = WaveList.right;
                                    IsCPSWaveSettingChanged = true;
                                    IsReloadClicker = true; // 重载配置 / 保存配置
                                }
                            }

                            Separator();

                            // 根据当前标签获取对应数据引用
                            vector<WaveParam>& waveList = (CurrentTab == 0) ? WaveList.left : WaveList.right;
                            float BaseCPS = (CurrentTab == 0) ? config.LCPS : config.RCPS;
                            float FluctRange = (CurrentTab == 0) ? config.LFluctuationCPS : config.RFluctuationCPS;
                            int& SelectedIdx = (CurrentTab == 0) ? SelectedWaveIndexLeft : SelectedWaveIndexRight;
                            bool& ShowSettings = (CurrentTab == 0) ? ShowWaveSettingsLeft : ShowWaveSettingsRight;

                            BeginChild("TopArea", ImVec2(0, GetContentRegionAvail().y * 0.6f), true);
                            {
                                // 左侧：预览图（占60%宽度）
                                BeginChild("LeftPreview", ImVec2(GetContentRegionAvail().x * 0.6f, 0), true);
                                {
                                    if (SelectedIdx >= 0 && SelectedIdx < (int)waveList.size())
                                    {
                                        if (ImPlot::BeginPlot("##WavePreview", ImVec2(-1, 0)))
                                        {
                                            const float rangeMax = 200.0f;
                                            float amp = waveList[SelectedIdx].amplitude;
                                            if (amp <= 0.0f) amp = 1.0f; // 防止除零
                                            ImPlot::SetupAxesLimits(0, rangeMax, -amp * 1.2f, amp * 1.2f);
                                            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_AutoFit);
                                            
                                            const int points = 200;
                                            static vector<float> xs(points), ys(points);
                                            for (int i = 0; i < points; ++i)
                                            {
                                                float t = (float)i / points * rangeMax;
                                                xs[i] = t;
                                                // 直接绘制实际振幅，不再归一化
                                                ys[i] = EvaluateWave(waveList[SelectedIdx], t);
                                            }
                                            ImPlot::PlotLine(config.UseChinese ? u8"波形" : "Waves", xs.data(), ys.data(), points);
                                            ImPlot::EndPlot();
                                        }
                                    }
                                    else
                                        Text(config.UseChinese ? u8"未选中波形" : "No wave selected");
                                }
                                EndChild();
                                SameLine();

                                // 右侧：列表 或 编辑面板
                                BeginChild("RightPanel", ImVec2(0, 0), true);
                                {
                                    if (!ShowSettings)
                                    {
                                        if (Button(config.UseChinese ? u8"添加波形" : "Add Waves"))
                                        {
                                            if (waveList.size() < 12)
                                            {
                                                WaveParam NewWave;
                                                if (CurrentTab == 0)
                                                {
                                                    NewWave.amplitude = RandGentor.Random(90000, 115000) / 100000.0f; // 0.9 - 1.15
                                                    NewWave.frequency = RandGentor.Random(20000, 600000) / 100000.0f; // 0.2 - 6
                                                    NewWave.phase = RandGentor.Random(-314159265, 314159265) / 100000000.f; // -pi ~ pi
                                                    NewWave.enabled = true;
                                                }
                                                else
                                                {
                                                    NewWave.amplitude = RandGentor.Random(60000, 200000) / 100000.0f; // 0.6 - 2
                                                    NewWave.frequency = RandGentor.Random(80000, 400000) / 100000.0f; // 0.8 - 4.0
                                                    NewWave.phase = RandGentor.Random(-314159265, 314159265) / 100000000.f; // -pi ~ pi
                                                    NewWave.enabled = true;
                                                }
                                                waveList.push_back(NewWave);
                                                SelectedIdx = (int)waveList.size() - 1;
                                            }
                                        }
                                        SameLine();
                                        if (SmallButton(config.UseChinese ? u8"编辑" : "Edit"))
                                            ShowSettings = true;
                                        BeginChild("WaveListChild", ImVec2(0, 0), true);
                                        for (int i = 0; i < (int)waveList.size(); ++i)
                                        {
                                            PushID(i);
                                            bool isSelected = (i == SelectedIdx);
                                            float avail = GetContentRegionAvail().x;

                                            float DelBtnWidth = 16 + GetStyle().FramePadding.x * 2 + 6;
                                            float TotalBtnWidth = DelBtnWidth + GetStyle().ItemSpacing.x * 2;

                                            char label[128];
                                            const WaveParam& w = waveList[i];
                                            sprintf_s(label, "A:%.2f F:%.2f P:%.2f", w.amplitude, w.frequency, w.phase);

                                            // 可选项（留出按钮空间）
                                            if (Selectable(label, isSelected, 0, ImVec2(avail - TotalBtnWidth, 0)))
                                                SelectedIdx = i;

                                            if (isSelected)
                                            {
                                                ImVec2 min = GetItemRectMin();
                                                ImVec2 max = GetItemRectMax();
                                                GetWindowDrawList()->AddRect(min, max, IM_COL32(255, 215, 0, 255), 0.0f, 0, 2.0f);
                                            }

                                            SameLine();
                                            static const Texture2D button_texture1 = LoadTextureFromFile(Pics_Delete);
                                            if (ImageButton("##delwave", button_texture1, ImVec2(16, 16)))
                                            {
                                                if (SelectedIdx == i)
                                                    SelectedIdx = -1;
                                                waveList.erase(waveList.begin() + i);
                                                PopID();
                                                break;
                                            }
                                            PopID();
                                        }
                                        EndChild();
                                    }
                                    else
                                    {
                                        if (Button(config.UseChinese ? u8"返回" : "Back"))
                                            ShowSettings = false;

                                        if (SelectedIdx >= 0 && SelectedIdx < (int)waveList.size())
                                        {
                                            WaveParam& w = waveList[SelectedIdx];
                                            // 振幅（步长0.05）
                                            SliderFloat(config.UseChinese ? u8"振幅&7(CPS)" : "Amplitude&7(CPS)", &w.amplitude, 0.0f, 10.0f);
                                            Text(config.UseChinese ? u8"微调" : "Tweak");
                                            SameLine();
                                            if (Button("-##amp")) { w.amplitude = max(0.0f, w.amplitude - 0.05f); }
                                            SameLine();
                                            if (Button("+##amp")) { w.amplitude = min(10.0f, w.amplitude + 0.05f); }

                                            // 频率（步长0.005）
                                            SliderFloat(config.UseChinese ? u8"频率&7(Hz)" : "Freq&7(Hz)", &w.frequency, 0.01f, 2.0f);
                                            Text(config.UseChinese ? u8"微调" : "Tweak");
                                            SameLine();
                                            if (Button("-##freq")) { w.frequency = max(0.01f, w.frequency - 0.005f); }
                                            SameLine();
                                            if (Button("+##freq")) { w.frequency = min(2.0f, w.frequency + 0.005f); }

                                            // 相位（步长0.05）
                                            SliderFloat(config.UseChinese ? u8"相位&7(弧度)" : "Phase&7(Rad)", &w.phase, -PI, PI);
                                            Text(config.UseChinese ? u8"微调" : "Tweak");
                                            SameLine();
                                            if (Button("-##phase")) { w.phase = max(-PI, w.phase - 0.05f); }
                                            SameLine();
                                            if (Button("+##phase")) { w.phase = min(PI, w.phase + 0.05f); }

                                            Toggle(config.UseChinese ? u8"启用" : "Enabled", &w.enabled, ToggleFlags);
                                        }
                                        else
                                            Text(config.UseChinese ? u8"未选中波形" : "No wave selected");
                                    }
                                }
                                EndChild();
                            }
                            EndChild();

                            Separator();

                            // ---------- 下半部分：总 CPS 叠加图 ----------
                            BeginChild("BottomArea", ImVec2(0, 216), true);
                            {
                                Toggle(config.UseChinese ? u8"显示应用CPS后的曲线" : "Show applied CPS curve", &ShowAppliedCPS, ToggleFlags);

                                if (ImPlot::BeginPlot("##OverallCPS", ImVec2(-1, 170)))
                                {
                                    const float rangeMax = 700.0f;
                                    // 计算总振幅
                                    float totalAmp = 0.0f;
                                    for (const auto& w : waveList)
                                        if (w.enabled)
                                            totalAmp += w.amplitude;
                                    float yMin = BaseCPS - totalAmp - 1;
                                    float yMax = BaseCPS + totalAmp + 1;
                                    if (yMin < 0) yMin = 0;
                                    ImPlot::SetupAxesLimits(0, rangeMax, yMin, yMax);

                                    const int points = 300;
                                    static vector<float> xs(points), ys_raw(points), ys_clamped(points);
                                    for (int i = 0; i < points; ++i)
                                    {
                                        float t = (float)i / points * rangeMax;
                                        xs[i] = t;
                                        float raw = EvaluateTotalCPS(t, BaseCPS, waveList);
                                        ys_raw[i] = raw;
                                        float clamped = clamp(raw, BaseCPS - FluctRange, BaseCPS + FluctRange);
                                        ys_clamped[i] = clamped;
                                    }

                                    ImPlot::PlotLine(config.UseChinese ? u8"原始总CPS" : "Raw total CPS", xs.data(), ys_raw.data(), points);
                                    if (ShowAppliedCPS)
                                    {
                                        ImPlot::SetNextLineStyle(ImVec4(1, 0.5f, 0, 1), 2);
                                        ImPlot::PlotLine(config.UseChinese ? u8"应用后CPS" : "Applied CPS", xs.data(), ys_clamped.data(), points);
                                    }
                                    ImPlot::EndPlot();
                                }
                            }
                            EndChild();

                            EndPopupWindow();
                        }

                        // 关闭弹窗时保存左右列表
                        if (!IsOpen && LastOpenState)
                        {
                            config.CPSWaves.left = WaveList.left;
                            config.CPSWaves.right = WaveList.right;
                            IsCPSWaveSettingChanged = true;
                            LastOpenState = false; // 防止重复保存
                        }
                    }
                    EndDisabled();

                    Text(config.UseChinese ? u8"&7如果要设置为鼠标左右键，请使用鼠标模式。" : "&7If you want to set it to the mouse buttons, please use the Mouse Mode.");
                }
                EndCard();
                
                SetCursorPosY(GetCursorPosY() + 10.0f);

                BeginCard("##Settings-1", ImVec2(-1, 193), ImGuiChildFlags_Borders);
                {
                    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

                    BeginChild("##Settings-1-1", ImVec2(156, -1), false);
                    {
                        //-------------------------暴力模式--------------------------
                        Toggle(config.UseChinese ? u8"暴力模式" : "Brutal Mode", &config.UseBrutalMode, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"让你的CPS轻松破百\n但可能会导致大量占用电脑CPU性能！\n并且可能会导致CPS记录器数据不准确！\n&d开启该功能会导致 物品栏开关 功能失效。" : "Let your CPS easily break 100\nBut it may cause a lot of computer CPU performance!\nAnd it may cause the CPS recorder data to be inaccurate!\n&dEnabling this feature will cause the ItemBarSwitch function to malfunction.");

                        //-------------------------启用连点--------------------------
                        Toggle(config.UseChinese ? u8"启用右键连点" : "Enable RClicker", &config.EnableRightClicker, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"该选项可能会被其他功能设置\n&7如果出现无法 &a打开&7/&c关闭 &f该选项\n可以尝试关闭一下功能：\n&7物品栏开关\n&7仅移动连点\n&7GUI检测" : "This option may be controlled by other functions.\n&fIf you encounter issues &aEnable/&cDisabled &fthis option,\nyou can try disabling the following features:\n&7Inventory Toggle\n&7Only Move Clicking\n&7GUI Detection");

                        Toggle(config.UseChinese ? u8"启用左键连点" : "Enable LClicker", &config.EnableLeftClicker, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"该选项可能会被其他功能设置\n&7如果出现无法 &a打开&7/&c关闭 &f该选项\n可以尝试关闭一下功能：\n&7物品栏开关\n&7仅移动连点\n&7GUI检测" : "This option may be controlled by other functions.\n&fIf you encounter issues &aEnable/&cDisabled &fthis option,\nyou can try disabling the following features:\n&7Inventory Toggle\n&7Only Move Clicking\n&7GUI Detection");

                        if (Toggle(config.UseChinese ? u8"开机自动启动" : "Start With System", &config.UseStartWithWindows, ToggleFlags))
                        {
                            static int FailedCount = 0;
                            thread FunThread([]()
                                {
                                    if (FailedCount >= 3)
                                        RenderTip(config.UseChinese ? u8"无法设置开机自动启动\n疑似杀毒软件自动拦截，\n请关闭杀毒软件后再试。" : "Failed to set start with system\nAnti-virus software interferes,\nplease close it and try again.", 4.0f, PT_WARNING);
                                    if (config.UseStartWithWindows)
                                    {
                                        if (!AddToStartup())
                                        {
                                            config.UseStartWithWindows = false;
                                            RenderTip(config.UseChinese ? u8"添加到开机启动失败" : "Failed to add to startup", 1.5f, PT_ERROR);
                                            PrintLog("[System] 添加到开机启动失败", PL_ERROR);
                                            FailedCount++;
                                        }
                                        else
                                        {
                                            RenderTip(config.UseChinese ? u8"添加到开机启动成功" : "Added to startup successfully", 1.5f, PT_INFO);
                                            PrintLog("[System] 添加到开机启动成功", PL_INFO);
                                            FailedCount = 0;
                                        }
                                    }
                                    else
                                    {
                                        if (!RemoveStartup())
                                        {
                                            config.UseStartWithWindows = true;
                                            RenderTip(config.UseChinese ? u8"删除开机自动启动失败" : "Failed to remove from startup", 1.5f, PT_ERROR);
                                            PrintLog("[System] 从开机启动中删除失败", PL_ERROR);
                                            FailedCount++;
                                        }
                                        else
                                        {
                                            RenderTip(config.UseChinese ? u8"从开机启动中删除成功" : "Removed from startup successfully", 1.5f, PT_INFO);
                                            PrintLog("[System] 从开机启动中删除成功", PL_INFO);
                                            FailedCount = 0;
                                        }
                                    }
                                });
                            FunThread.detach();
                        }

                        //-------------------------适配新版基岩版--------------------------
                        static int backup_mode = config.SimulateClickMode;
                        if (Toggle(config.UseChinese ? u8"适配&c新版&f基岩版" : "Adapt BE", &config.UseBEAdaptation, ToggleFlags))
                        {
                            if (config.UseBEAdaptation)
                            {
                                backup_mode = config.SimulateClickMode;
                                config.SimulateClickMode = SC_SEND_INPUT;
                                RenderTip(config.UseChinese ? u8"已调整 &d点击方式 &f切换为 &6Send Input" : "The &dSimulate Click mode set to &6Send Input.", 2.2f, PT_INFO);
                            }
                            else
                                config.SimulateClickMode = backup_mode;
                        }

                        //-------------------------主播模式--------------------------
                        if (Toggle(config.UseChinese ? u8"主播模式" : "Broadcaster Mode", &config.UseBroadcasterMode, ToggleFlags))
                        {
                            thread FunThread(SetWindowDisplayAffinity, GCHWND, config.UseBroadcasterMode ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
                            FunThread.detach();
                        }
                    }
                    EndChild();

                    SameLine();

                    BeginChild("##Settings-1-2", ImVec2(248, -1), false);
                    {
                        //-------------------------左右优先级--------------------------
                        static const array<const char*, 3> ClickerPriorityListText = {
                            (config.UseChinese ? u8"无" : "None"),
                            (config.UseChinese ? u8"右键连点优先" : "RC Highter Priority"),
                            (config.UseChinese ? u8"左键连点优先" : "LC Highter Priority")
                        };

                        Text(config.UseChinese ? u8"左右优先级" : "AC Priority");
                        SetNextItemWidth(146);
                        SameLine();
                        Combo("##ClickerPriorityMode", &config.ClickerPriorityMode, ClickerPriorityListText.data(), 3);

                        //-------------------------强制开关左右键连点--------------------------
                        SetNextItemWidth(80);
                        InputKey("##EnableRightAssisKey", &config.EnableRightClickerKey, config.UseChinese ? u8"请按下按键" : "Press a key");
                        AttachTooltip(config.UseChinese ? u8"这个不是控制连点开关的，是防止突发情况准备的强制性开关。" : "This is not a switch for the clicker,\nit is a forced switch to prevent an emergency situation.");
                        SameLine();
                        Text(config.UseChinese ? u8"强制开关右键连点" : "Force Key RClicker");

                        SetNextItemWidth(80);
                        InputKey("##EnableLeftAssisKey", &config.EnableLeftClickerKey, config.UseChinese ? u8"请按下按键" : "Press a key");
                        AttachTooltip(config.UseChinese ? u8"这个不是控制连点开关的，是防止突发情况准备的强制性开关。" : "This is not a switch for the clicker,\nit is a forced switch to prevent an emergency situation.");
                        SameLine();
                        Text(config.UseChinese ? u8"强制开关左键连点" : "Force Key LClicker");

                        //-----------------------自毁快捷键----------------------------
                        SetNextItemWidth(80);
                        InputKey("##sdkey", &config.SelfDestructionKey, config.UseChinese ? u8"请按下按键" : "Press a key");
                        AttachTooltip(config.UseChinese ? u8"该功能在GC窗口中将不起作用，\n请在其他地方按下按键方可生效。" : "This function will not work in the GC window, \nplease press the key in another place to take effect.");
                        SameLine();
                        Text(config.UseChinese ? u8"自毁快捷键" : "Self-Destruct Key");

                        //-------------------------允许控制连点开关--------------------------
                        Toggle(config.UseChinese ? u8"允许控制右键连点开关" : "Allow Ctrl RC Enables", &config.AllowSetRClicker, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"默认开启，关闭后，连点器将无法控制右键的开关状态。" : "The default is to turn on, and if you turn off,\nthe clicker will not be able to control the right-click switch status. ");
                        Toggle(config.UseChinese ? u8"允许控制左键连点开关" : "Allow Ctrl LC Enables", &config.AllowSetLClicker, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"默认开启，关闭后，连点器将无法控制左键的开关状态。" : "The default is to turn on, and if you turn off,\nthe clicker will not be able to control the left-click switch status. ");
                    }
                    EndChild();

                    SameLine();

                    BeginChild("##Settings-1-3", ImVec2(229, -1), false);
                    {
                        Toggle(config.UseChinese ? u8"过热保护" : "Feature Timeout", &config.UseOverheatProtec, ToggleFlags);

                        BeginDisabled(!config.UseOverheatProtec);
                        {
                            Text(config.UseChinese ? u8"过热时长" : "Timeout");
                            SameLine();
                            SliderFloat(u8"##OverheatTime", &config.OverheatTime, 1.0f, 20.0f, config.UseChinese ? u8"%.2f秒" : "%.2fs", ImGuiSliderFlags_AlwaysClamp);

                            Text(config.UseChinese ? u8"冷却时长" : "Cooldown");
                            SameLine();
                            SetNextItemWidth(120);
                            SliderFloat(u8"##OverheatCooldownTime", &config.OverheatCooldownTime, 0.0f, 6.0f, (config.OverheatCooldownTime <= 0) ? (config.UseChinese ? u8"禁用" : "Disabled") : "%.2f", ImGuiSliderFlags_AlwaysClamp);

                            static const array<const char*, 3> OverheatSoundTypeListText = {
                                (config.UseChinese ? u8"禁用" : "Disabled"),
                                (config.UseChinese ? u8"系统默认" : "System Default"),
                                (config.UseChinese ? u8"特殊的彩蛋" : "Special Easter Egg")
                            };

                            Text(config.UseChinese ? u8"提示音" : "Alert");
                            SetNextItemWidth(146);
                            SameLine();
                            Combo("##OverheatSoundType", &config.OverheatSoundType, OverheatSoundTypeListText.data(), 3);

                            Text(config.UseChinese ? u8"过热计时器值：%.2f" : "Overheat Timer: %.2f", OverheatTimer::Heat);
                            Text(config.UseChinese ? u8"冷却计时器值：%.2f" : "Cooldown Timer: %.2f", OverheatTimer::Cooldown);
                        }
                        EndDisabled();
                    }
                    EndChild();

                    PopStyleColor();
                }
                EndCard();

                //-------------------------连点模式----------------------------
                //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard("##ClickerModeCard", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    array<int*, 3> vlist = { &config.RClickerMode, &config.LClickerMode };
                    for (int i = 0; i < 2; i++)
                    {
                        PushID(i);
                        if (i == 2)
                            SetCursorPosY(GetCursorPosY() + 20);

                        Text(config.UseChinese ? ((i == 0) ? u8"右键连点模式" : u8"左键连点模式") : ((i == 0) ? "RClicker Mode" : "LClicker Mode"));
                        SameLine();

                        bool is_selected = *vlist[i] & C_BUTTONMODE;
                        if (Checkbox(config.UseChinese ? u8"默认模式" : "Default Mode", &is_selected))
                        {
                            *vlist[i] &= ~C_SWITCHMODE;
                            *vlist[i] = is_selected ? *vlist[i] | C_BUTTONMODE : *vlist[i] & ~C_BUTTONMODE;
                        }
                        AttachTooltip(config.UseChinese ? u8"按住按键连点，松开停止\n连点器默认模式。" : "Hold down the key to click\nRelease to stop\nThe default mode of the clicker.");

                        SameLine();

                        is_selected = *vlist[i] & C_SWITCHMODE;
                        if (Checkbox(config.UseChinese ? u8"开关模式##" : "Switch Mode", &is_selected))
                        {
                            *vlist[i] &= ~C_BUTTONMODE;
                            *vlist[i] = is_selected ? *vlist[i] | C_SWITCHMODE : *vlist[i] & ~C_SWITCHMODE;
                        }
                        AttachTooltip(config.UseChinese ? u8"按下按键连点，再按下按键停止，最普通的模式。" : "Press the key to click and release the key to stop\nThe most common mode.");

                        SameLine();

                        is_selected = *vlist[i] & C_ASSISTMODE;
                        if (Checkbox(config.UseChinese ? u8"辅助模式" : "Assist Mode", &is_selected))
                        {
                            *vlist[i] &= ~C_MOUSESMODE;
                            *vlist[i] = is_selected ? *vlist[i] | C_ASSISTMODE : *vlist[i] & ~C_ASSISTMODE;
                        }
                        AttachTooltip(config.UseChinese ? u8"当点击鼠标时，自动点击你设置的次数，以补全CPS\n类似鼠标宏的双倍点击。\n&d使用这个模式后，无法在连点器中连点，\n&d防止误点到连点器的设置。" : "When you click the mouse button automatically click the number you set\nSimilar to the double-click of the mouse macro.\n&dAfter using this mode, you cannot click in the clicker, \n&dto prevent misclicking the settings of the clicker.");

                        SameLine();

                        is_selected = *vlist[i] & C_MOUSESMODE;

                        if (Checkbox(config.UseChinese ? u8"鼠标模式" : "Mouse Mode", &is_selected))
                        {
                            *vlist[i] &= ~C_ASSISTMODE;
                            *vlist[i] = is_selected ? *vlist[i] | C_MOUSESMODE : *vlist[i] & ~C_MOUSESMODE;
                        }
                        AttachTooltip(config.UseChinese ? u8"按住鼠标键连点，松开停止\n会导致挖方块的速度减慢。\n&d使用这个模式后，无法在连点器中连点，防止误点到连点器的设置。" : "Hold down the mouse button to click\nRelease to stop\nIt will slow down the speed of mining blocks.\n&dAfter using this mode, you cannot click in the clicker, \n&dto prevent misclicking the settings of the clicker.");

                        // 不允许全关
                        if (*vlist[i] == 0)
                            *vlist[i] |= C_BUTTONMODE;
                        PopID();
                    }

                    //-------------------------破坏方块/使用物品--------------------------
                    if (config.RClickerMode & C_ASSISTMODE || config.LClickerMode & C_ASSISTMODE || config.RClickerMode & C_MOUSESMODE || config.LClickerMode & C_MOUSESMODE)
                    {
                        Toggle(config.UseChinese ? u8"破坏方块" : "Break Blocks", &config.UseLClickerTrigInter, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"开启后，你可以破坏方块。\n&d该功能与 按下间隔 冲突\n&d启用后将会禁用冲突功能。" : "If you turn on this option, you can use items.\n&dThis feature conflicts with the interval between clicks, \n&dand will disable the conflicting function.");

                        SameLine();

                        Toggle(config.UseChinese ? u8"使用物品" : "Use Items", &config.UseRClickerTrigInter, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"开启后，你可以破坏方块。\n&d该功能与 按下间隔 冲突\n&d启用后将会禁用冲突功能。" : "If you turn on this option, you can use items.\n&dThis feature conflicts with the interval between clicks, \n&dand will disable the conflicting function.");

                        SameLine();

                        Toggle(config.UseChinese ? u8"使用旧方法" : "Use Old Method", &config.UseOldMethodToTrigInter, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"本功能针对 &d使用物品 &7/ &d破坏方块&f\n如果出现上述功能无法使用\n则可以尝试开启本功能" : "This function is designed for&d UseItem s&7/&d BreakBlocks&f.\nAnd it can cause the mouse sound to be a bit strange\nIf the above functions cannot be used,\nyou can try to turn on this function");

                        if (config.UseLClickerTrigInter)
                            Text(config.UseChinese ? u8"&e注意：开启 破坏方块 可能出现卡刀的情况\n并且会导致鼠标声音有些奇怪\n请自行衡量！" : "&eNote: Enabling \"Break blocks\" may cause attack lag.\nUse at your own risk!");
                    }

                    //-------------------------过滤单点--------------------------
                    if (config.RClickerMode & C_MOUSESMODE)
                    {
                        Toggle(config.UseChinese ? u8"过滤右键单点" : "Filter RMB Single Clicks", &config.UseFilterRSingleClick, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"开启后，你普通的点击将&c不会触发连点" : "If you turn on this option, normal clicks will &cnot trigger the clicker.");
                    }

                    if (config.LClickerMode & C_MOUSESMODE)
                    {
                        if (config.RClickerMode & C_MOUSESMODE)
                            SameLine();
                        Toggle(config.UseChinese ? u8"过滤左键单点" : "Filter LMB Single Clicks", &config.UseFilterLSingleClick, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"开启后，你普通的点击将&c不会触发连点" : "If you turn on this option, normal clicks will &cnot trigger the clicker.");
                    }

                    //-------------------------辅助补全数--------------------------

                    if (config.RClickerMode & C_ASSISTMODE || config.LClickerMode & C_ASSISTMODE)
                    {
                        DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                        if (config.RClickerMode & C_ASSISTMODE)
                        {
                            Text(config.UseChinese ? u8"右键辅助补全数" : "RClicker Assist Completion");
                            SameLine();
                            SetNextItemWidth(300);
                            SetCursorPosX(300);
                            SliderInt(u8"##RMultipleClickValue", &config.RMultipleClickValue, 1, 6, config.UseChinese ? u8"%d次" : "%d times", ImGuiSliderFlags_AlwaysClamp);
                            AttachTooltip(config.UseChinese ? u8"当你点击鼠标右键时，\n会连续点击你设置的次数。" : "When you click the right mouse button, \nIt will continuously click the number you set.");

                            static int PredictingCPS = 0;
                            static int b_MCV = -1;
                            if (b_MCV != config.RMultipleClickValue)
                            {
                                PredictingCPS = 6 * (config.RMultipleClickValue + 1);
                                b_MCV = config.RMultipleClickValue;
                            }
                            Text(config.UseChinese ? u8"&7右键连点CPS预计为：%d - %dCPS" : "&7The connected CPS is approximately:%d - %dCPS", PredictingCPS - 1, PredictingCPS + 1);
                            AttachTooltip(config.UseChinese ? u8"本数据仅为预测数据，仅供参考\n实际CPS受用户点击的CPS影响。" : "This data is only for predictive purposes and is for reference only. \nActual CPS is affected by the CPS clicked by the user.");
                        }

                        if (config.LClickerMode & C_ASSISTMODE)
                        {
                            Text(config.UseChinese ? u8"左键辅助补全数" : "LClicker Assist Completion");
                            SameLine();
                            SetNextItemWidth(300);
                            SetCursorPosX(300);
                            SliderInt(u8"##LMultipleClickValue", &config.LMultipleClickValue, 1, 6, config.UseChinese ? u8"%d次" : "%d times", ImGuiSliderFlags_AlwaysClamp);
                            AttachTooltip(config.UseChinese ? u8"当你点击鼠标左键时，\n会连续点击你设置的次数。" : "When you click the left mouse button, \nIt will continuously click the number you set.");

                            static int PredictingCPS = 0;
                            static int b_MCV = -1;
                            if (b_MCV != config.LMultipleClickValue)
                            {
                                PredictingCPS = 6 * (config.LMultipleClickValue + 1);
                                b_MCV = config.LMultipleClickValue;
                            }
                            Text(config.UseChinese ? u8"&7左键连点CPS预计为：%d - %dCPS" : "&7The connected CPS is approximately:%d - %dCPS", PredictingCPS - 1, PredictingCPS + 1);
                            AttachTooltip(config.UseChinese ? u8"本数据仅为预测数据，仅供参考\n实际CPS受用户点击的CPS影响。" : "This data is only for predictive purposes and is for reference only. \nActual CPS is affected by the CPS clicked by the user.");
                        }

                        Text(config.UseChinese ? u8"触发阀门值" : "Trigger Threshold");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##AssistModeCPSThreshold", &config.AssistModeCPSThreshold, 1, 16, u8"%dCPS", ImGuiSliderFlags_AlwaysClamp);
                        AttachTooltip(config.UseChinese ? u8"当你点击的CPS达到预定值时，才会触发辅助点击\n&6该功能可有效降低误判。" : "When the CPS reaches the prescribed value, The assist click will be triggered, \n&6which can effectively reduce the misjudgment.");
                    }
                }
                EndCard();

                //-------------------------物品栏开关----------------------------
                //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"物品栏开关" : "Itembar Switch", ImVec2(0, 280), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    if (Toggle(config.UseChinese ? u8"物品栏开关" : "Itembar Switch", &config.UseItembarSwitch, ToggleFlags))
                    {
                        config.EnableLeftClicker = true;
                        config.EnableRightClicker = true;
                    }
                    AttachTooltip(config.UseChinese ? u8"将对应物品格子的按钮关闭，连点器就会在你手拿对应物品时关闭连点。\n其中的快捷键必须设置，主要用于判断物品栏位置！" : "Turn off the button corresponding to the corresponding item slot\nin the item bar when you hold the corresponding item. \nThe clicker will close the click when you hold the corresponding item.\nThe shortcut key must be set, mainly used to \ndetermine the position of the item bar!");

					BeginDisabled(!config.UseItembarSwitch);
                    {
                        if (Button(config.UseChinese ? u8"全部启用" : "Enable All"))
                        {
                            for (unsigned int i = 0; i < 9; i++)
                            {
                                config.ItemEnableClicker.left[i] = true;
                                config.ItemEnableClicker.right[i] = true;
                            }
                        }
                        SameLine();
                        if (Button(config.UseChinese ? u8"重置全部CPS" : "Reset All CPS"))
                        {
                            for (unsigned int i = 0; i < 9; i++)
                            {
                                config.ItemBarPresetCPS.left[i] = config.LCPS;
                                config.ItemBarPresetCPS.right[i] = config.RCPS;
                            }
                        }

                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
                        BeginChild("##ItembarChild", ImVec2(680, 250), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            ImDrawList* DrawList = GetWindowDrawList();
                            for (unsigned int j = 0; j < 2; j++)
                            {
                                PushID(j);
                                const char* text = config.UseChinese ? (j == 0 ? u8"左键设置" : u8"右键设置") : (j == 0 ? "LMB Settings" : "RMB Settings");
                                Text(text);
                                SameLine();
                                DrawSpiltLine(528 - CalcTextSize(text).x, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                                const ImVec2 pos1 = GetCursorPos();
                                {
                                    DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pPointSampler);
                                    Image((ImTextureID)ImageItembar, ImVec2(178 * 3, 19 * 3));
                                    DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pLinearSampler);
                                }
                                const ImVec2 pos2 = GetCursorPos();

                                SetCursorPos(ImVec2(GetCursorPosX() + ((60 - 40) / 2.0f) + 3, GetCursorPosY() - 55));

                                auto& IsEnableClicker = (j == 0) ? config.ItemEnableClicker.left : config.ItemEnableClicker.right;
                                auto& IsEnablePresetCPS = (j == 0) ? config.ItemBarEnablePresetCPS.left : config.ItemBarEnablePresetCPS.right;
                                auto& PresetCPS = (j == 0) ? config.ItemBarPresetCPS.left : config.ItemBarPresetCPS.right;

                                for (unsigned int i = 0; i < 9; i++)
                                {
                                    PushID(i);
                                    // 每个格子的左侧X坐标
                                    const float PosX = pos1.x + (i * 60.0f);
                                    if (ItemBarPos == i)
                                    {
                                        const ImVec2 pos2 = GetCursorPos();
                                        SetCursorPos(ImVec2(PosX - 1, pos1.y));
                                        {
                                            DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pPointSampler);
                                            Image((ImTextureID)ImageSelected, ImVec2(18 * 3, 19 * 3));
                                            DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pLinearSampler);
                                        }
                                        SetCursorPos(pos2);
                                    }

                                    // pos.y的值是格子的顶部Y坐标
                                    SetCursorPos(ImVec2(PosX + 7, pos1.y + 28));
                                    Toggle("##ItembarToggle", &IsEnableClicker[i], ToggleFlags);
                                    SameLine(0, 20);
                                    PopID();
                                }

                                SetCursorPos(ImVec2(pos1.x + 540 + 2, pos1.y));
                                PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
                                BeginChild("##ItembarChildIn", ImVec2(130, 19 * 3), false);
                                {
                                    BeginDisabled(!config.UseItembarSwitch);
                                    {
                                        Toggle(config.UseChinese ? u8"CPS切换" : "CPS Switch", &IsEnablePresetCPS[ItemBarPos], ToggleFlags);
                                        AttachTooltip(config.UseChinese ? u8"可以让每一个格子对应不同的CPS\n如果使用了CPS波动，&d会自动计算波动范围&f，你只需要调整CPS就可以" : "Each grid can correspond to a different CPS.\nIf CPS fluctuation is used, &dthe fluctuation range will be auto calculated,\n&fand you only need to adjust the CPS");

                                        BeginDisabled(!IsEnablePresetCPS[ItemBarPos] || config.UseBrutalMode);
                                        {
                                            SetNextItemWidth(100);
                                            SliderInt("##itembar_cpsset", &PresetCPS[ItemBarPos], MIN_CPS, MAX_CPS, "%dCPS", ImGuiSliderFlags_AlwaysClamp);
                                        }
                                        EndDisabled();
                                    }
                                    EndDisabled();
                                }
                                EndChild();
                                PopStyleColor();

                                PopID();
                            }
                        }
                        EndChild();
                        PopStyleVar(); //ImGuiStyleVar_WindowPadding
                    }
					EndDisabled();
                }
                EndCard();

				BeginCard(config.UseChinese ? u8"物品栏按键设置" : "Itembar Key Settings", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
				{
                    AttachTooltip(config.UseChinese ? u8"&c请确保快捷键和游戏中的一致！\n&6否则将无法判断物品栏切换" : "&cPlease make sure the shortcut key is consistent with the game!\n&6Otherwise, it cannot determine the position of the item bar switch!");

                    // 参考 ##ItembarChild 样式：用物品栏图片包装 9 个快捷键，InputKey 精确对齐每个格子
                    ImDrawList* DrawList = GetWindowDrawList();
                    const ImVec2 pos1 = GetCursorPos();          // 物品栏图片左上角（相对本 card）
                    const ImVec2 ItembarSize(178 * 3, 19 * 3);   // (534, 57)
                    const float SlotWidth = 60.0f;               // 每个格子的定位宽度（与 ##ItembarChild 一致）

                    // 画物品栏底图
                    DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pPointSampler);
                    Image((ImTextureID)ImageItembar, ItembarSize);
                    DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pLinearSampler);

                    // 9 个快捷键：InputKey 中心对齐格子中心。
                    // 水平：格子宽 60、InputKey 宽 40 -> 左偏移 (60-40)/2=10，中心=PosX+30；
                    // 垂直：图片高 57、InputKey 高 24 -> 偏移 (57-24)/2=16，中心与图片中心对齐。
                    for (unsigned int i = 0; i < 9; i++)
                    {
                        PushID(i);
                        const float PosX = pos1.x + (i * SlotWidth);

                        // 高亮当前选中的物品栏格子（与 ##ItembarChild 一致）
                        if (ItemBarPos == i)
                        {
                            const ImVec2 cur = GetCursorPos();
                            SetCursorPos(ImVec2(PosX - 1, pos1.y));
                            {
                                DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pPointSampler);
                                Image((ImTextureID)ImageSelected, ImVec2(18 * 3, 19 * 3));
                                DrawList->AddCallback(SetSamplerCallback, (void*)MainGuiData.pLinearSampler);
                            }
                            SetCursorPos(cur);
                        }

                        SetCursorPos(ImVec2(PosX + 10, pos1.y + 24));
                        SetNextItemWidth(40);
                        InputKey(u8"##item", &config.ItemBarKey.at(i), config.UseChinese ? u8"请按下按键" : "Press a key");
                        PopID();
                    }
				}
                EndCard();

                //-------------------------获取游戏窗口----------------------------
                //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                //SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"游戏窗口" : "Game Window", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    static string InfoText = config.UseChinese ? u8"未获取" : "Not Obtained";
                    static bool IsStartObtaining = false;
                    static string GameWindowTitle = config.UseChinese ? u8"未知" : "Unknow";

                    BeginDisabled(IsStartObtaining);
                    {
                        if (Button(config.UseChinese ? u8"获取" : "Choice", ImVec2(100, 40)))
                        {
                            GettingGameState = -1;
                            IsStartObtaining = true;

                            auto th = []()
                                {
                                    PrintLog("[Render] 等待用户选择游戏窗口...", PL_INFO);
                                    const HWND DesktopHwnd = GetDesktopWindow();
                                    while (!IsGCExit)
                                    {
                                        HWND hwnd = GetForegroundWindow();
                                        if (hwnd != GCHWND && hwnd != DesktopHwnd)
                                        {
                                            GameHwnd = hwnd;
                                            GettingGameState = 0;
                                            return 0;
                                        }
                                        if (hwnd == 0)
                                        {
                                            GettingGameState = 1;
                                            return 0;
                                        }
                                        Sleep(200);
                                    }
                                    return 0;
                                };
                            thread t(th);
                            t.detach();
                        }
                    }
                    EndDisabled();

                    if (GettingGameState == 0)
                    {
                        InfoText = config.UseChinese ? u8"&a获取成功" : "&aSuccessfully Obtained";
                        IsStartObtaining = false;

                        char buf[MAX_BUF];
                        if (GetWindowText(GameHwnd, buf, MAX_BUF))
                            GameWindowTitle = StringToUTF8(buf);
                        else
                            GameWindowTitle = u8"";

                        GettingGameState = -1;
                    }
                    else if (GettingGameState == 1)
                    {
                        InfoText = config.UseChinese ? u8"&c获取失败" : "&cFailed to obtain";
                        IsStartObtaining = false;
                        GettingGameState = -1;
                        GameHwnd = 0;
                    }
                    else if (IsStartObtaining)
                        InfoText = config.UseChinese ? u8"正在获取中，请点击游戏窗口。" : "Getting, please click the game window.";
                    // 防止GameWindowTitle中有格式化符号导致 崩溃 或者 漏洞
                    Text(((config.UseChinese ? u8"窗口名称：" : "Window Name: ") + GameWindowTitle).c_str());
                }
                EndCard();

                EndTabItem();
            }

            //-------------------------高级设置--------------------------
            if (BeginTabItem((ICON_FA_GEAR + (config.UseChinese ? u8"高级设置"s : "Advanced Setting"s)).c_str()))
            {
                BeginCard("##Settings-2", ImVec2(-1, 222), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

                    //-------------------------连点间隙----------------------------
                    BeginChild("##Settings-2-1", ImVec2(326, -1), false);
                    {
                        Toggle(config.UseChinese ? u8"右键连点间隙" : "RClicker Interval", &config.UseRClickTime, ToggleFlags);
                        BeginDisabled(!config.UseRClickTime);
                        {
                            Text(config.UseChinese ? u8"点击时长" : "Clicker Time");
                            AttachTooltip(config.UseChinese ? u8"连点的时长，单位毫秒" : "The duration of the click in milliseconds");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##RCTime", &config.RCTime, 10, 10000, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);

                            Text(config.UseChinese ? u8"停止时长" : "Stop Time");
                            AttachTooltip(config.UseChinese ? u8"连点停止的时长，单位毫秒" : "The duration of the stop in milliseconds");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##RCSleep", &config.RCSleep, 10, 10000, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();

                        Toggle(config.UseChinese ? u8"左键连点间隙" : "RClicker Interval", &config.UseLClickTime, ToggleFlags);
                        BeginDisabled(!config.UseLClickTime);
                        {
                            Text(config.UseChinese ? u8"连点时长" : "Clicker Time");
                            AttachTooltip(config.UseChinese ? u8"连点的时长，单位毫秒" : "The duration of the click in milliseconds");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##LCTime", &config.LCTime, 10, 10000, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);

                            Text(config.UseChinese ? u8"停止时长" : "Stop Time");
                            AttachTooltip(config.UseChinese ? u8"连点停止的时长，单位毫秒" : "The duration of the stop in milliseconds");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##LCSleep", &config.LCSleep, 10, 10000, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();

                        if (config.LClickerMode & C_ASSISTMODE || config.RClickerMode & C_ASSISTMODE)
                            Text(config.UseChinese ? u8"&c辅助模式不支持该功能" : "&cThe AssistMode does not support this function.");
                    }
                    EndChild();

                    SameLine();

                    //-------------------------空刀概率----------------------------
                    BeginChild("##Settings-2-2", ImVec2(326, -1), false);
                    {
                        Toggle(config.UseChinese ? u8"右键空刀概率" : "RClicker Empty Rate", &config.UseREmptyRate, ToggleFlags);
                        BeginDisabled(!config.UseREmptyRate);
                        {
                            Text(config.UseChinese ? u8"右键空刀概率" : "RClicker Empty Rate");
                            SameLine();
                            SetNextItemWidth(200);
                            SliderInt(u8"##UseREmptyRate", &config.REmptyRateValue, 1, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();

                        Toggle(config.UseChinese ? u8"左键空刀概率" : "LClicker Empty Rate", &config.UseLEmptyRate, ToggleFlags);
                        BeginDisabled(!config.UseLEmptyRate);
                        {
                            Text(config.UseChinese ? u8"左键空刀概率" : "LClicker Empty Rate");
                            SameLine();
                            SetNextItemWidth(200);
                            SliderInt(u8"##UseLEmptyRate", &config.LEmptyRateValue, 1, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();

                        BeginDisabled(!config.UseLEmptyRate && !config.UseREmptyRate);
                        {
                            Text(config.UseChinese ? u8"随机生成分布" : "Random Distribution");
                            SetNextItemWidth(200);
                            SameLine();
                            if (BeginCombo("##erandomdist", RandomDistList[config.EmptyRate_RandomDist]))
                            {
                                for (int n = 0; n < 3; n++)
                                {
                                    PushID(n);
                                    const bool is_selected = (config.EmptyRate_RandomDist == n);
                                    if (Selectable(BoolRandomDistList[n], is_selected))
                                        config.EmptyRate_RandomDist = n;
                                    if (is_selected)
                                        SetItemDefaultFocus();
                                    PopID();
                                }
                                EndCombo();
                            }

                            Text(config.UseChinese ? u8"随机模式" : "Random Mode");
                            AttachTooltip(config.UseChinese ? u8"&7普通随机：按设定概率均匀随机空刀\n&7疲劳效应：随时间推移空刀率上升（模拟疲劳）\n&7仿生幽灵：疲劳 + 手抖 + 马尔可夫纠错（模拟真人）\n\n&6注意：疲劳/仿生模式下，实际空刀率会随时间在 1%~40% 之间浮动，\n可能偏离你设置的概率。" : "Normal: uniform random by the set probability\nFatigue: error rate increases over time\nGhost: fatigue + hand shaking + Markov error correction (human-like)\n\nNote: in Fatigue/Ghost modes the actual empty rate floats between 1%~40% over time.");
                            SetNextItemWidth(200);
                            SameLine();
                            if (BeginCombo("##erandommode", RandomBoolModeList[config.EmptyRate_RandomMode]))
                            {
                                for (int n = 0; n < 3; n++)   // 修复：移除 STIFF 后仅 3 种模式
                                {
                                    PushID(n + 100); // 与上方分布 combo 的 PushID 错开，避免 ID 冲突
                                    const bool is_selected = (config.EmptyRate_RandomMode == n);
                                    if (Selectable(RandomBoolModeList[n], is_selected))
                                        config.EmptyRate_RandomMode = n;
                                    if (is_selected)
                                        SetItemDefaultFocus();
                                    PopID();
                                }
                                EndCombo();
                            }


                            static bool IsOpen = false;
                            if (Button(config.UseChinese ? u8"随机曲线分布" : "Random Curve"))
                                IsOpen = !IsOpen;

                            if (BeginPopupWindow(config.UseChinese ? u8"随机曲线分布" : "Random Curve", &IsOpen))
                            {
                                static int PointsSize = 200;
                                static vector<short> ClickDatas;
                                static vector<short> EmpytDatas;
                                static float GenDurationSum = 0;
                                static bool IsInit = false;
                                static bool IsGenerating = false;
                                static float auv = 0;
                                static float ActualRate = 0;
                                // 修复：ClickDatas/EmpytDatas 等由 detach 线程重填、主线程每帧绘制，
                                // 原代码无锁，线程 push_back 扩容时主线程 data()/size() 访问已释放内存 → 0xC0000005。
                                static mutex CurveMutex;

                                Text(config.UseChinese ? u8"数据点数量" : "Number of data points");
                                SameLine();
                                SetNextItemWidth(300);
                                SetCursorPosX(300);
                                SliderInt(u8"##rgc_points", &PointsSize, 10, 2000, "%d", ImGuiSliderFlags_AlwaysClamp);

                                if (VarHasChanged(config.UseLEmptyRate, "UseLEmptyRate") ||
                                    VarHasChanged(config.UseREmptyRate, "UseREmptyRate") ||
                                    VarHasChanged(config.EmptyRate_RandomDist, "EmptyRate_RandomDist") ||
                                    VarHasChanged(config.EmptyRate_RandomMode, "EmptyRate_RandomMode") ||
                                    VarHasChanged(config.REmptyRateValue, "EmptyRate_REmptyRateValue") ||
                                    VarHasChanged(PointsSize, "EmptyRate_PointsSize"))
                                {
                                    IsInit = false;
                                }

                                Text(config.UseChinese ? u8"&7*注：橙色柱代表实际有点击，灰色代表空刀，默认使用的是右键空刀概率。" : "&7*Note: The orange bar represents the actual click, \nthe gray bar represents the empty click, \nthe default is to use the left key empty rate.");
                                AttachTooltip(config.UseChinese ? u8"&6橙色柱：这个位置在实际情况中&a已点击&f\n&6灰色柱：这个位置在实际情况中&c没有点击&f\n你可以理解为，每个数据点代表一个点击\n颜色代表是否空刀，灰色为空刀。" : "Orange bar: This position has been clicked in the actual situation\nGray bar: This position has not been clicked in the actual situation\nYou can understand it as each data point represents a click\nColor represents whether it is an empty click, gray means empty.");

                                if ((Button(config.UseChinese ? u8"刷新" : "Refresh", ImVec2(70, 30)) || !IsInit) && !IsGenerating)
                                {
                                    IsGenerating = true;
                                    thread FunThread([]()
                                    {
                                        // 先在局部临时容器中生成，避免长时间持有锁
                                        vector<short> tmpClick;
                                        vector<short> tmpEmpty;
                                        tmpClick.reserve(PointsSize);
                                        tmpEmpty.reserve(PointsSize);
                                        float tmpDur = 0;
                                        int ClickCount = 0;

                                        // 补全：预览需与连点实际行为一致（随机模式：普通/极速/疲劳/仿生），
                                        // 改用 RandomBoolManager；rand() 返回 true=点击（与 MainClick.h 的 rbm 用法一致）。
                                        RandomBoolManager rbm;
                                        rbm.SetErrorRate(config.REmptyRateValue / 100.0f);
                                        rbm.SetMode((RandomBoolMode)config.EmptyRate_RandomMode);
                                        rbm.SetDistribution(config.EmptyRate_RandomDist);

                                        // 修复：预览是毫秒级快速生成，真实时间几乎不流逝 → FATIGUE/BIONIC 的
                                        // 疲劳因子恒≈1.0，各模式曲线看起来一样（实际连点中按真实时间累积才有效）。
                                        // 用右键 CPS 模拟真实点击节奏（每次点击推进一个点击间隔），
                                        // 让"随时间失误率上升"在曲线中可见（如 10CPS × 2000 点 ≈ 3 分钟，疲劳因子升至 ~1.5）。
                                        const int SimCPS = max(config.RCPS, 1);
                                        const chrono::steady_clock::duration SimStep = chrono::milliseconds(1000 / SimCPS);

                                        for (int i = 0; i < PointsSize; i++)
                                        {
                                            auto start = chrono::high_resolution_clock::now();

                                            rbm.AdvanceFatigueTime(SimStep);   // 模拟一次点击间隔的流逝
                                            const bool data = rbm.rand();   // true=有点击（橙色），false=空刀（灰色）
                                            tmpClick.push_back(data);
                                            tmpEmpty.push_back(!data);

                                            auto end = chrono::high_resolution_clock::now();
                                            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                                            tmpDur += duration.count() / 1000.0;
                                            if (data)
                                                ClickCount++;
                                        }
                                        {
                                            // 锁内一次性交换数据，主线程绘制期间能看到一致的数据
                                            lock_guard lock(CurveMutex);
                                            ClickDatas.swap(tmpClick);
                                            EmpytDatas.swap(tmpEmpty);
                                            GenDurationSum = tmpDur;
                                            auv = GenDurationSum / PointsSize;
                                            ActualRate = (double)ClickCount / PointsSize * 100;
                                            IsGenerating = false;
                                        }
                                    });
                                    FunThread.detach();
                                    IsInit = true;
                                }

                                if (ImPlot::BeginPlot("##Random_curves1", ImVec2(-1, -1), ImPlotFlags_CanvasOnly))
                                {
                                    ImPlot::SetupAxesLimits(0, PointsSize, -2, 2, ImGuiCond_Always);

                                    ImPlot::SetNextMarkerStyle(-1, 1.5f, (ImVec4)ImColor(0, 0, 0, 0));
                                    ImPlot::SetNextLineStyle((ImVec4)ImColor(254, 189, 57, 200), 0);
                                    {
                                        // 绘制期间加锁，防止后台线程 swap 导致 data() 悬垂
                                        lock_guard lock(CurveMutex);
                                        ImPlot::PlotBars(config.UseChinese ? u8"已点击" : "Clicked", ClickDatas.data(), ClickDatas.size(), 0.8);
                                    }

                                    ImPlot::SetNextMarkerStyle(-1, 1.5f, (ImVec4)ImColor(0, 0, 0, 0));
                                    ImPlot::SetNextLineStyle((ImVec4)ImColor(0, 106, 201, 200), 0);
                                    {
                                        lock_guard lock(CurveMutex);
                                        ImPlot::PlotBars(config.UseChinese ? u8"空点击" : "Empty Click", EmpytDatas.data(), EmpytDatas.size(), 0.8);
                                    }
                                    ImPlot::EndPlot();
                                }
                                Text(config.UseChinese ? u8"耗时：%.6f毫秒" : "Time: %.6f ms", GenDurationSum);
                                SameLine();
                                Text(config.UseChinese ? u8"共 %d 个数据点" : "A total of %d data points", PointsSize);
                                SameLine();
                                Text(config.UseChinese ? u8"平均用时：%.6f毫秒/数据点" : "Average time: %.6f ms/data point", auv);
                                Text(config.UseChinese ? u8"实际空刀概率：%.3f%" : "Actual empty click: %.3f%", ActualRate);
                                EndPopupWindow();
                            }
                        }
                        EndDisabled();
                    }
                    EndChild();

                    PopStyleColor();
                }
                EndCard();

                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard("##Settings-3", ImVec2(-1, -1), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    //-------------------------主循环延迟----------------------------
                    Text(config.UseChinese ? u8"主循环延迟" : "Loop Delay");
                    AttachTooltip(config.UseChinese ? u8"每次轮询间隔，单位毫秒，\n太高会导致有延迟(设置为1ms就几乎没有延迟)\n通常不需要设置，在GC占用率大时，调高该数值。\n不会就保持默认。" : "The interval between each polling, in milliseconds, \nToo high will cause delay (set to 1ms almost no delay)\nUsually do not need to set, increase this value when the GC occupancy is high. \nIt will not stay at the default value.");
                    SameLine();
                    SetNextItemWidth(300);
                    SetCursorPosX(300);
                    SliderInt(u8"##MainClickSleep", &config.MainClickSleep, 0, 50, config.UseChinese ? u8"%d毫秒" : "%d ms", ImGuiSliderFlags_AlwaysClamp);

                    BeginDisabled(config.UseBEAdaptation);
                    {
                        //-------------------------点击方式----------------------------
                        Text(config.UseChinese ? u8"点击方式" : "Click Method");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        unordered_map<int, const char*> ClickMethodList = {
                            { SC_MOUSE_EVENT,  config.UseChinese ? u8"Mouse Event - 模拟鼠标消息点击" : "Mouse Event - Simulate mouse message click" },
                            { SC_POST_MESSAGE, config.UseChinese ? u8"Post Message - 使用消息队列点击" : "Post Message - Use message queue click"     },
                            { SC_SEND_INPUT,   config.UseChinese ? u8"Send Input - 使用模拟鼠标输入" : "Send Input - Use simulated mouse input"     }
                        };
                        if (BeginCombo("##ClickMethod", ClickMethodList[config.SimulateClickMode]))
                        {
                            for (auto& it : ClickMethodList)
                            {
                                const bool is_selected = (config.SimulateClickMode == it.first);
                                if (Selectable(it.second, is_selected))
                                    config.SimulateClickMode = it.first;
                                if (is_selected)
                                    SetItemDefaultFocus();
                            }
                            EndCombo();
                        }
                    }
                    EndDisabled();
                    if (config.UseBEAdaptation)
                        Text(config.UseChinese ? u8"&c启用适配基岩版后，无法更改此选项。" : "&cAfter enabling Bedrock Edition adaptation, this option cannot be changed.");

                    Text(config.UseChinese ? u8"读取按键状态方式" : "Read Key Status Method");
                    SameLine();
                    SetNextItemWidth(300);
                    SetCursorPosX(300);
                    unordered_map<int, const char*> ReadKeyStateModeList = {
                        { GK_BOTH, config.UseChinese ? u8"自动选择"     : "Auto select"     },
                        { GK_API , config.UseChinese ? u8"使用系统API"  : "Use system API"  },
                        { GK_HOOK, config.UseChinese ? u8"使用系统钩子" : "Use system hook" }
                    };
                    if (BeginCombo("##ReadKeyStateMode", ReadKeyStateModeList[config.ReadKeyStateMode]))
                    {
                        for (auto& it : ReadKeyStateModeList)
                        {
                            const bool is_selected = (config.ReadKeyStateMode == it.first);
                            if (Selectable(it.second, is_selected))
                                config.ReadKeyStateMode = it.first;
                            if (is_selected)
                                SetItemDefaultFocus();
                        }
                        EndCombo();
                    }

                    Toggle(config.UseChinese ? u8"仅人类操作触发" : "Human Only Trigger", &config.UseHumanOnlyTrigger, ToggleFlags);
                }
                EndCard();

                //-------------------------过热降频--------------------------
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"过热降频" : "Overheat Throttle", ImVec2(-1, 167), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    Toggle(config.UseChinese ? u8"启用" : "Enable", &config.UseOverheatThrottle, ToggleFlags);
                    AttachTooltip(config.UseChinese ? u8"&6过热降频：连点时间过长会导致\"过热\"，CPS 自动下降，模拟真实手部/硬件疲劳。\n&c停止连点后自动恢复，下次连点重新计时" : "&6Overheat Throttle: Long continuous clicking causes \"overheating\" and the CPS automatically drops, simulating real hand/hardware fatigue.\n&cStops clicking and it recovers automatically; the next session restarts the timer");
                    BeginDisabled(!config.UseOverheatThrottle);
                    {
                        Text(config.UseChinese ? u8"CPS最低限制" : "Min CPS");
                        AttachTooltip(config.UseChinese ? u8"降频后 CPS 不会低于该值。\n例如设置为 10，则无论过热多久，CPS 最低保持 10。" : "CPS never drops below this value.\nE.g. 10 means CPS stays at least 10 even after long overheating.");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##OverheatMinCPS", &config.Overheat_MinCPS, 1, 100, config.UseChinese ? u8"%d CPS" : "%d CPS", ImGuiSliderFlags_AlwaysClamp);

                        Text(config.UseChinese ? u8"过热速度" : "Overheat Time");
                        AttachTooltip(config.UseChinese ? u8"连点持续多少秒后达到过热点。\n&a到达过热点之后才开始降频，之前保持设定的 CPS。" : "How many seconds of continuous clicking to reach the overheat point.\n&aThrottling starts only AFTER reaching it; CPS stays normal before that.");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderFloat(u8"##OverheatTime", &config.Overheat_Time, 1.0f, 60.0f, config.UseChinese ? u8"%.1f秒" : "%.1fs", ImGuiSliderFlags_AlwaysClamp);

                        Text(config.UseChinese ? u8"降频速度" : "Throttle Time");
                        AttachTooltip(config.UseChinese ? u8"到达过热点后，多少秒内从当前 CPS 线性降到最低限制。\n数值越小降频越快，越大越平缓。" : "After reaching the overheat point, how many seconds to linearly drop from current CPS to the minimum limit.\nSmaller = faster throttle, larger = smoother.");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderFloat(u8"##OverheatThrottleTime", &config.Overheat_ThrottleTime, 1.0f, 60.0f, config.UseChinese ? u8"%.1f秒" : "%.1fs", ImGuiSliderFlags_AlwaysClamp);
                    }
                    EndDisabled();
                }
                EndCard();

                //-------------------------延迟触发--------------------------
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"连点延迟触发" : "Clicker Delay Trigger", ImVec2(-1, 109), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

                    BeginChild("##Settings-4-1", ImVec2(323, -1), false);
                    {
                        Toggle(config.UseChinese ? u8"启用" : "Enable", &config.UseRStartClickerDelay, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"让你触发连点时有延迟\n&d通常为鼠标模式使用" : "Delay the trigger of the clicker\n&dUsually used in mouse mode");

                        BeginDisabled(!config.UseRStartClickerDelay);
                        {
                            Text(config.UseChinese ? u8"延迟时长" : "Delay Time");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##RStartClickerDelay", &config.RStartClickerDelay, config.UseFilterRSingleClick ? 140 : 5, 3000, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();
                    }
                    EndChild();

                    SameLine();

                    BeginChild("##Settings-4-2", ImVec2(328, -1), false);
                    {
                        Toggle(config.UseChinese ? u8"启用" : "Enable", &config.UseLStartClickerDelay, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"让你触发连点时有延迟\n&d通常为鼠标模式使用" : "Delay the trigger of the clicker\n&dUsually used in mouse mode");

                        BeginDisabled(!config.UseLStartClickerDelay);
                        {
                            Text(config.UseChinese ? u8"延迟时长" : "Delay Time");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##LStartClickerDelay", &config.LStartClickerDelay, config.UseFilterLSingleClick ? 140 : 5, 3000, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();
                    }
                    EndChild();

                    if (config.RClickerMode & C_ASSISTMODE || config.LClickerMode & C_ASSISTMODE)
                        Text(config.UseChinese ? u8"&c辅助模式不支持该功能" : "&cThe AssistMode does not support this function.");

                    PopStyleColor();
                }
                EndCard();

                //-------------------------按下间隔--------------------------
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"点击按下间隔" : "Click Press Interval", ImVec2(-1, 131), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

                    BeginChild("##Settings-5-1", ImVec2(328, -1), false);
                    {
                        Toggle(config.UseChinese ? u8"启用" : "Enable", &config.UseRClickPressTime, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"在每次点击之中，按下按键的持续时长。" : "The duration of the button press during each click.");

                        BeginDisabled(!config.UseRClickPressTime);
                        {
                            Toggle(config.UseChinese ? u8"动态设置按下时长&7（建议）" : "Dynamically set press duration &7(recommended)", &config.UseRAutoPressDuration, ToggleFlags);
                            AttachTooltip(config.UseChinese ? u8"动态的设置按下时长\n来模拟人类点击的的按下间隔\n&a除非特殊用途，否则建议开启。" : "Dynamically set the press duration to \nsimulate the press interval of a human click.\n&aIt is recommended to turn on unless there is a special purpose.");
                        }
                        EndDisabled();

                        BeginDisabled(!config.UseRClickPressTime || config.UseRAutoPressDuration);
                        {
                            Text(config.UseChinese ? u8"按住持续时长" : "Press Duration");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##PressDuration", &config.RPressDuration, 0, 500, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();
                    }
                    EndChild();

                    SameLine();

                    BeginChild("##Settings-5-2", ImVec2(328, -1), false);
                    {
                        Toggle(config.UseChinese ? u8"启用" : "Enable", &config.UseLClickPressTime, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"在每次点击之中，按下按键的持续时长。" : "The duration of the button press during each click.");

                        BeginDisabled(!config.UseLClickPressTime);
                        {
                            Toggle(config.UseChinese ? u8"动态设置按下时长&7（建议）" : "Dynamically set press duration &7(recommended)", &config.UseLAutoPressDuration, ToggleFlags);
                            AttachTooltip(config.UseChinese ? u8"动态的设置按下时长\n来模拟人类点击的的按下间隔\n&a除非特殊用途，否则建议开启。" : "Dynamically set the press duration to \nsimulate the press interval of a human click.\n&aIt is recommended to turn on unless there is a special purpose.");
                        }
                        EndDisabled();

                        BeginDisabled(!config.UseLClickPressTime || config.UseLAutoPressDuration);
                        {
                            Text(config.UseChinese ? u8"按住持续时长" : "Press Duration");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##PressDuration", &config.LPressDuration, 0, 500, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();
                    }
                    EndChild();

                    PopStyleColor();
                }
                EndCard();

                //-------------------------鼠标抖动--------------------------

                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"鼠标抖动" : "Mouse Shake", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    Toggle(config.UseChinese ? u8"启用" : "Enable", &config.UseMouseShake, ToggleFlags);
                    AttachTooltip(config.UseChinese ? u8"开启后，在连点时会随机抖动鼠标位置。" : "Turn on this function to randomly shake the mouse position during clicking.");

                    BeginDisabled(!config.UseMouseShake);
                    {
                        Text(config.UseChinese ? u8"抖动范围" : "Shake Range");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##MouseShakeRect", &config.MouseShakeRect, 1, 20, config.UseChinese ? u8"%d像素" : "%d px", ImGuiSliderFlags_AlwaysClamp);
                        AttachTooltip(config.UseChinese ? u8"抖动范围的正方形边长大小，建议不要太大。" : "The size of the shake rectangle, it is recommended not to be too large.");

                        Text(config.UseChinese ? u8"鼠标移动速度" : "Mouse Move Speed");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##MouseShakeSpeed", &config.MouseShakeSpeed, 100, 1000, config.UseChinese ? u8"%d像素/每分钟" : "%d px/minute", ImGuiSliderFlags_AlwaysClamp);
                        AttachTooltip(config.UseChinese ? u8"在两点之间运动的速度，也就是抖动时的速度。" : "The speed at which the mouse moves between two points, which is the speed of the shake.");

                        Toggle(config.UseChinese ? u8"非线性移动" : "Non-linear Move", &config.UseMSSmoothSpeed, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"使用非线性函数，来实现缓冲移动的效果。" : "Use a non-linear function to \nachieve the effect of buffer movement.");
                        SameLine();

                        Toggle(config.UseChinese ? u8"原始输入" : "Raw Input", &config.UseMouseShakeRawInput, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"模拟鼠标原始输入，防止系统对鼠标加速度\n导致移动距离不精确。" : "Simulate the raw mouse input to prevent the system from \naccurately measuring the mouse acceleration.");

                        Toggle(config.UseChinese ? u8"防止视角偏离" : "Prevent View Sliding", &config.UseMSPreventViewSliding, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"使用算法实现在鼠标抖动时不会出现视角偏离原先位置。" : "Use an algorithm to prevent the view from sliding during mouse shake.");

                        Text(config.UseChinese ? u8"随机生成分布" : "Random Distribution");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        if (BeginCombo("##mrandomdist", RandomDistList[config.MouseShake_RandomDist]))
                        {
                            for (int n = 0; n < 2; n++)
                            {
                                const bool is_selected = (config.MouseShake_RandomDist == n);
                                if (Selectable((RandomDistList[n] + "##"s + to_string(n)).c_str(), is_selected))
                                    config.MouseShake_RandomDist = n;
                                if (is_selected)
                                    SetItemDefaultFocus();
                            }
                            EndCombo();
                        }

                        TextColored((ImVec4)ImColor(255, 214, 143, 255), config.UseChinese ? u8"为了防止误触，该功能无法在GC窗口中使用！\n请切换到其他窗口该功能方可生效。" : "To prevent accidental triggering, this function cannot be used in the GC window!\nPlease switch to another window for this feature to take effect.");

                        static bool IsOpen = false;
                        if (Button(config.UseChinese ? u8"随机曲线分布" : "Random Curve"))
                            IsOpen = !IsOpen;

                        if (BeginPopupWindow(config.UseChinese ? u8"随机曲线分布" : "Random Curve", &IsOpen))
                        {
                            static int PointsSize = 200;
                            static vector<int> Points_x;
                            static vector<int> Points_y;
                            static vector<int> Points_z;
                            static float GenDurationSum = 0;
                            static bool IsInit = false;
                            static bool IsGenerating = false;
                            static float auv = 0;
                            // 修复：Points_x/y/z 由 detach 线程重填、主线程每帧绘制，
                            // 原代码无锁，线程 push_back 扩容时主线程 data() 访问已释放内存 → 0xC0000005。
                            static mutex CurveMutex2;

                            Text(config.UseChinese ? u8"数据点数量" : "Number of data points");
                            SameLine();
                            SetNextItemWidth(300);
                            SetCursorPosX(300);
                            SliderInt(u8"##ms_points2", &PointsSize, 10, 2000, "%d", ImGuiSliderFlags_AlwaysClamp);

                            if (VarHasChanged(config.MouseShake_RandomDist, "MouseShake_RandomDist") ||
                                VarHasChanged(PointsSize, "PointsSize_ms") ||
                                VarHasChanged(config.MouseShakeRect, "MouseShakeRect") ||
                                VarHasChanged(config.UseMSPreventViewSliding, "UseMSPreventViewSliding"))
                            {
                                IsInit = false;
                            }

                            if ((Button(config.UseChinese ? u8"刷新" : "Refresh", ImVec2(70, 30)) || !IsInit) && !IsGenerating)
                            {
                                IsGenerating = true;
                                thread FunThread([]()
                                    {
                                        // 先在局部临时容器中生成，避免长时间持有锁
                                        vector<int> tmp_x;
                                        vector<int> tmp_y;
                                        vector<int> tmp_z;
                                        tmp_x.reserve(PointsSize);
                                        tmp_y.reserve(PointsSize);
                                        tmp_z.reserve(PointsSize);
                                        float tmpDur = 0;
                                        POINT view{};

                                        for (int i = 0; i < PointsSize; i++)
                                        {
                                            auto start = chrono::high_resolution_clock::now();

                                            POINT pt = MouseShakeAlgorithm(view, { 0, 0 });
                                            tmp_x.push_back(pt.x);
                                            tmp_y.push_back(pt.y);
                                            tmp_z.push_back(i);

                                            auto end = chrono::high_resolution_clock::now();
                                            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                                            tmpDur += duration.count() / 1000.0;
                                        }
                                        {
                                            // 锁内一次性交换数据
                                            lock_guard lock(CurveMutex2);
                                            Points_x.swap(tmp_x);
                                            Points_y.swap(tmp_y);
                                            Points_z.swap(tmp_z);
                                            GenDurationSum = tmpDur;
                                            auv = GenDurationSum / PointsSize;
                                            IsGenerating = false;
                                        }
                                    });
                                FunThread.detach();
                                IsInit = true;
                            }

                            static int PointsTime = -1;

                            if (ImPlot3D::BeginPlot("##RandomPlot3D", ImVec2(420, 250) /*ImPlot3DFlags_NoMenus | ImPlot3DFlags_CanvasOnly*/))
                            {
                                ImPlot3D::SetupBoxRotation(ImPlot3DQuat::FromElAz(44.5f, 21.4f), false);
                                //ImPlot3D::SetupBoxScale(1.5f, 1.0f, 1.0f);
                                ImPlot3D::SetupAxes("X", config.UseChinese ? u8"时间" : "Time", "Y");
                                ImPlot3D::SetupAxesLimits(-20, 20, 0, PointsSize, -20, 20, ImGuiCond_Always);
                                {
                                    // 绘制期间加锁，防止后台线程 swap 导致 data() 悬垂
                                    lock_guard lock(CurveMutex2);
                                    ImPlot3D::PlotLine(config.UseChinese ? u8"鼠标位置" : "Mouse position", Points_x.data(), Points_z.data(), Points_y.data(), Points_z.size());
                                }

                                ImPlot3D::EndPlot();
                            }

                            SameLine();

                            if (ImPlot::BeginPlot("##RandomPlot2D", ImVec2(250, 250), ImPlotFlags_CanvasOnly))
                            {
                                ImPlot::SetNextMarkerStyle(-1, 1.5f, (ImVec4)ImColor(184, 30, 1, 255));
                                ImPlot::SetNextLineStyle((ImVec4)ImColor(30, 156, 214, 255), 2);
                                ImPlot::SetupAxesLimits(-20, 20, -20, 20, ImGuiCond_Always);

                                {
                                    lock_guard lock(CurveMutex2);
                                    ImPlot::PlotLine(config.UseChinese ? u8"鼠标位置" : "Mouse position", Points_x.data(), Points_y.data(), Points_z.size());
                                    // 修复：原代码 &Points_y[PointsSize] 在 PointsSize == size() 时越界读（0xC0000005 隐患），
                                    // 且滑块语义应为"当前时间点"，改用 PointsTime 并收紧边界检查
                                    if (PointsTime >= 0 && PointsTime < static_cast<int>(Points_z.size()))
                                        ImPlot::PlotScatter("##PlotPoints", &Points_y[PointsTime], &Points_x[PointsTime], 1);
                                }

                                ImPlot::EndPlot();
                            }

                            Text(config.UseChinese ? u8"时间点" : "point of time");
                            SameLine();
                            SetNextItemWidth(300);
                            SetCursorPosX(300);
                            SliderInt(u8"##randomplotmove", &PointsTime, -1, PointsSize, "%d", ImGuiSliderFlags_AlwaysClamp);

                            Text(config.UseChinese ? u8"耗时：%.6f毫秒" : "Time: %.6f millisecond", GenDurationSum);
                            SameLine();
                            Text(config.UseChinese ? u8"共 %d 个数据点" : "A total of %d data points", PointsSize);
                            Text(config.UseChinese ? u8"平均用时：%.3f毫秒/数据点" : "Average time: %.3f ms/data point", auv);
                            EndPopupWindow();
                        }
                    }
                    EndDisabled();
                }
                EndCard();

                EndTabItem();
            }
            /*
            if (BeginTabItem(config.UseChinese ? u8"GUI(新)" : "GUI(new)"))
            {
                if (GameHwnd == NULL)
                    Text(config.UseChinese ? u8"&c请先选择游戏窗口" : "&cPlease select the game window first");
                BeginDisabled(GameHwnd == NULL);
                {
                    if (Button(config.UseChinese ? u8"启动" : "Start"))
                    {

                    }
                }
                EndDisabled();

                EndTabItem();
            }*/

            //-------------------------鼠标声音--------------------------
            if (BeginTabItem((ICON_FA_VOLUME_HIGH + (config.UseChinese ? u8"点击声音"s : "Click Sound"s)).c_str()))
            {
                auto ReloadSoundFile = []() {
                    RenderTip(config.UseChinese ? u8"正在重新加载声音..." : "Reloading sounds...", 4.0f, PT_INFO);
                    LoadSounds();
                    RenderTip(config.UseChinese ? u8"重新加载成功！" : "Reloading success!", 3.0f, PT_INFO);
                };
                //-------------------------点击声音--------------------------
                BeginCard(config.UseChinese ? u8"鼠标声音" : "Mouse Sound", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    static int MouseType = []()->int {
                        if (MouseSoundList.empty())
                            return -1;
                        auto it = find(MouseSoundList.begin(), MouseSoundList.end(), config.MouseSoundType);
                        if (it == MouseSoundList.end())
                            return 0;  // 或者 return -1，看你默认想显示第几个
                        return static_cast<int>(distance(MouseSoundList.begin(), it));
                    }();
                    Toggle(config.UseChinese ? u8"启用鼠标声音" : "Enable Mouse Sound", &config.UseMouseSound, ToggleFlags);
                    BeginDisabled(!config.UseMouseSound);
                    {
                        Text(config.UseChinese ? u8"鼠标声音类型" : "Mouse Sound Type");
                        if (BeginCombo("##MouseSoundType", config.MouseSoundType.c_str()))
                        {
                            int size = MouseSoundList.size();
                            for (int n = 0; n < size; n++)
                            {
                                PushID(n); // 防止鼠标声音的name重复
                                const bool isSelected = (MouseType == n);
                                if (Selectable(MouseSoundList.at(n).c_str(), isSelected))
                                {
                                    MouseType = n;
                                    config.MouseSoundType = MouseSoundList.at(n);
                                }
                                if (isSelected)
                                    SetItemDefaultFocus();
                                PopID();
                            }
                            EndCombo();
                        }

                        Text(config.UseChinese ? u8"鼠标点击音量" : "Mouse Click Volume");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##MouseSoundVolume", &config.MouseSoundVolume, 0, 200, "%d%", ImGuiSliderFlags_AlwaysClamp);
                    }
                    EndDisabled();
                }
                EndCard();

                SetCursorPosY(GetCursorPosY() + 10);

                BeginCard(config.UseChinese ? u8"键盘声音" : "Keybd Sound", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    static int KeybdType = []()->int {
                        if (KeybdSoundList.empty())
                            return -1;
                        auto it = find(KeybdSoundList.begin(), KeybdSoundList.end(), config.KeybdSoundType);
                        if (it == KeybdSoundList.end())
                            return 0;
                        return static_cast<int>(distance(KeybdSoundList.begin(), it));
                        }();
                    Toggle(config.UseChinese ? u8"启用键盘声音" : "Enable Keybd Sound", &config.UseKeybdSound, ToggleFlags);
                    BeginDisabled(!config.UseKeybdSound);
                    {
                        Text(config.UseChinese ? u8"键盘声音类型" : "Keybd Sound Type");
                        if (BeginCombo("##KeybdSoundType", config.KeybdSoundType.c_str()))
                        {
                            int size = KeybdSoundList.size();
                            for (int n = 0; n < size; n++)
                            {
                                PushID(n); // 防止鼠标声音的name重复
                                const bool isSelected = (KeybdType == n);
                                if (Selectable(KeybdSoundList.at(n).c_str(), isSelected))
                                {
                                    KeybdType = n;
                                    config.KeybdSoundType = KeybdSoundList.at(n);
                                }
                                if (isSelected)
                                    SetItemDefaultFocus();
                                PopID();
                            }
                            EndCombo();
                        }

                        static bool AlreadDownload = _access(Tmp_KeySounds.c_str(), 0) == 0;

                        // 下载按钮冷却
                        static ULONGLONG LastDownloadClick = 0;
                        BeginDisabled(GetTickCount64() - LastDownloadClick <= 3000);
                        {
                            if (Button(config.UseChinese ?
                                (AlreadDownload ? u8"重新下载资源(30MB)" : u8"下载官方资源(30MB)") :
                                (AlreadDownload ? "Redownload Sounds (30MB)" : "Download Sounds (30MB)"))
                                )
                            {
                                LastDownloadClick = GetTickCount64();
                                auto dlth = []() {
                                    DownloadKeybdSounds(); // 下载

                                    RenderTip(config.UseChinese ? u8"正在重新加载声音..." : "Reloading sounds...", 4.0f, PT_INFO);
                                    LoadSounds();
                                    RenderTip(config.UseChinese ? u8"重新加载成功！" : "Reloading success!", 3.0f, PT_INFO);
                                    };
                                thread th(dlth);
                                th.detach();
                            }
                        }
                        EndDisabled();

                        SameLine();

                        // mechvibes 按钮冷却
                        static ULONGLONG LastMechvibesClick = 0;
                        BeginDisabled(GetTickCount64() - LastMechvibesClick <= 2000);
                        {
                            if (Button(config.UseChinese ? u8"前往mechvibes下载更多声音" : "Download more from mechvibes"))
                            {
                                LastMechvibesClick = GetTickCount64();
                                ShellExecute(NULL, NULL, "https://mechvibes.com/sound-packs/", NULL, NULL, SW_SHOWNORMAL);
                                RenderMessageBox(u8"在打开的网页列表中，找到想要的音色\n点击左边的“Learn More”，然后点击“Download”就可以下载了。\n&6如何导入？\n点击旁边的“导入文件”按钮然后选择你刚刚下载的压缩包。", config.UseChinese ? u8"导入音色教程" : "How to import sounds", MB_OK, false);
                            }
                        }
                        EndDisabled();

                        SameLine();

                        if (Button(config.UseChinese ? u8"导入文件" : "Import File"))
                        {
                            static bool IsOpen = false;
                            if (!IsOpen)
                            {
                                thread th([&]()
                                    {
                                        IsOpen = true;

                                        OPENFILENAME ofn;
                                        char szFileName[MAX_PATH]{};
                                        ZeroMemory(&ofn, sizeof(ofn));
                                        ofn.lStructSize = sizeof(ofn);
                                        ofn.hwndOwner = NULL; // 可以设置为你的窗口句柄
                                        ofn.lpstrFilter = "Zip压缩包\0*.zip\0\0";
                                        ofn.lpstrFile = szFileName;
                                        ofn.nMaxFile = MAX_PATH;
                                        ofn.hwndOwner = GCHWND;
                                        // 设置对话框标志，例如提示覆盖等
                                        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                                        ofn.lpstrDefExt = "ini"; // 默认扩展名
                                        if (GetOpenFileName(&ofn))
                                        {
                                            const string OriginFile = szFileName;
                                            size_t LastSlash = OriginFile.find_last_of("\\/");
                                            string FileName = (LastSlash == string::npos) ? OriginFile : OriginFile.substr(LastSlash + 1);

                                            // 去除 ".zip" 后缀（不区分大小写，先统一转小写比较）
                                            string BaseName = FileName;
                                            string ext = ".zip";
                                            if (BaseName.size() >= ext.size() &&
                                                _stricmp(BaseName.c_str() + BaseName.size() - ext.size(), ext.c_str()) == 0)
                                            {
                                                BaseName = BaseName.substr(0, BaseName.size() - ext.size());
                                            }
                                            string TargetDir = Sound_KeybdPath + "\\" + BaseName;
                                            // 若该目录已存在，则添加 (1), (2) ...
                                            string OriginalTarget = TargetDir;
                                            int counter = 1;
                                            while (GetFileAttributes(TargetDir.c_str()) != INVALID_FILE_ATTRIBUTES)
                                            {
                                                TargetDir = OriginalTarget + "(" + to_string(counter) + ")";
                                                counter++;
                                            }
                                            if (CreateDirectory(TargetDir.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
                                                UnZipFile(OriginFile.c_str(), TargetDir.c_str());
                                            else
                                            {
                                                PrintLog("[File] 无法创建目录 " + TargetDir, PL_ERROR);
                                                RenderTip(config.UseChinese ? u8"无法创建目录 " : "Failed to create directory " + TargetDir, 3.0f, PT_ERROR);
                                            }
                                            RenderTip(config.UseChinese ? u8"导入成功！" : "Import success!", 3.0f, PT_INFO);

                                            ReloadSoundFile();

                                            IsBgImageRefresh = true;
                                        }

                                        IsOpen = false;
                                    });
                                th.detach();
                            }
                        }

                        Text(config.UseChinese ? u8"键盘点击音量" : "Keybd Click Volume");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##KeybdSoundVolume", &config.KeybdSoundVolume, 0, 200, "%d%", ImGuiSliderFlags_AlwaysClamp);
                    }
                    EndDisabled();
                }
                EndCard();

                SetCursorPosY(GetCursorPosY() + 10);

                BeginCard(config.UseChinese ? u8"杂项" : "Othre", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    BeginDisabled(!(config.UseMouseSound || config.UseKeybdSound));
                    {
                        DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                        Text(config.UseChinese ? u8"循环检测延迟" : "Loop Detection Delay");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##MouseSoundSleep", &config.MouseSoundSleep, 0, 50, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);

                        Text(config.UseChinese ? u8"杂声音量" : "Noise Volume");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##NoiseVolume", &config.NoiseVolume, 0, 100, "%d%", ImGuiSliderFlags_AlwaysClamp);
                    }
                    EndDisabled();

                    // 重载声音文件 按钮冷却
                    static ULONGLONG LastReloadClick = 0;
                    BeginDisabled(GetTickCount64() - LastReloadClick <= 3000);
                    {
                        if (Button(config.UseChinese ? u8"重载声音文件" : "Reload Sound Files"))
                        {
                            LastReloadClick = GetTickCount64();
                            thread th(ReloadSoundFile);
                            th.detach();
                        }
                    }
                    EndDisabled();
                }
                EndCard();

                EndTabItem();
            }

            //-------------------------连点控制--------------------------
            if (BeginTabItem((ICON_FA_CIRCLE_PLAY + (config.UseChinese ? u8"连点控制"s : "Click Control"s)).c_str()))
            {
                //-------------------------GUI检测&其他--------------------------
                SetCursorPosY(GetCursorPosY() + 10.0f);

                BeginCard("##CCtrlSettings-1", ImVec2(235, 174), ImGuiChildFlags_Borders);
                {
                    Toggle(config.UseChinese ? u8"GUI检测" : "GUI Detect", & config.UseGUIDetection, ToggleFlags);
                    AttachTooltip(config.UseChinese ? u8"&a开启该功能后\n&f打开GUI时，会自动关闭连点\n&d前提条件：&6开启允许控制连点开关" : "&aTurn on this function\n&fWhen the GUI is opened, the clicker will be automatically closed\n&dPrerequisite: &6Turn on the function of allowing control of clicker switch");
                }
                EndCard();

                SameLine();

                BeginCard("##CCtrlSettings-2", ImVec2(215, 174), ImGuiChildFlags_Borders);
                {
                    //-------------------------仅移动触发连点--------------------------

                    Toggle(config.UseChinese ? u8"仅移动触发右键连点" : " Only Moving Trig RC", &config.UseROnlyMoveClicker, ToggleFlags);
                    AttachTooltip(config.UseChinese ? u8"开启后，连点器只会在你移动鼠标时触发连点。" : "Turn on this function to trigger the clicker only when you move the mouse.");

                    Toggle(config.UseChinese ? u8"仅移动触发左键连点" : " Only Moving Trig LC", &config.UseLOnlyMoveClicker, ToggleFlags);
                    AttachTooltip(config.UseChinese ? u8"开启后，连点器只会在你移动鼠标时触发连点。" : "Turn on this function to trigger the clicker only when you move the mouse.");

                    //-------------------------仅跳跃触发连点--------------------------
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    BeginDisabled(config.UseROnlyMoveClicker);
                    {
                        Toggle(config.UseChinese ? u8"仅跳跃触发右键连点" : "Only Jump On RC", &config.UseROnlyJumpClicker, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"开启后，只有你跳跃时才能连点。\n如果你已经开启了 &6仅移动触发右键连点 &f功能，则该功能无效。\n因为已经包含此功能。" : "After activation, you can only connect when jumping. \nIf you have turned on the function of &6[Only move to trigger right clicker]&f, this function is invalid. \nBecause it already contains this function.");
                    }
                    EndDisabled();

                    BeginDisabled(config.UseLOnlyMoveClicker);
                    {
                        Toggle(config.UseChinese ? u8"仅跳跃触发左键连点" : " Only Jump On LC", &config.UseLOnlyJumpClicker, ToggleFlags);
                        AttachTooltip(config.UseChinese ? u8"开启后，只有你跳跃时才能连点。\n如果你已经开启了 &6仅移动触发右键连点 &f功能，则该功能无效。\n因为已经包含此功能。" : "After activation, you can only connect when jumping. \nIf you have turned on the function of &6[Only move to trigger right clicker]&f, this function is invalid. \nBecause it already contains this function.");
                    }
                    EndDisabled();
                }
                EndCard();

                SameLine();

                BeginCard("##CCtrlSettings-3", ImVec2(215, 174), ImGuiChildFlags_Borders);
                {
                    Toggle(config.UseChinese ? u8"仅限游戏内连点" : "In-game clicker only", & config.UseInGameOnlyClicker, ToggleFlags);
                    if (GameHwnd == NULL)
                        Text(config.UseChinese ? u8"&e请先获取游戏窗口" : "&ePlease choose game window first");
                    else if (!config.AllowSetLClicker || config.AllowSetRClicker)
                        Text(config.UseChinese ? u8"&e请打开 允许控制连点开关" : "&ePlease enable \"Allow Control Clicker\"");

                    Toggle(config.UseChinese ? u8"停止移动时关闭连点" : "Stop Click On Stop Move", &config.UseDisableMoveOnStopMove, ToggleFlags);
                }
                EndCard();

                //-------------------------快速切换CPS--------------------------
                BeginCard(config.UseChinese ? u8"快速切换CPS" : "Quick Switch CPS", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    bool IsSelectable = false;
                    static vector<int> ListContainer;
                    static bool IsInit = false;
                    if (!IsInit)
                    {
                        IsInit = true;
                        for (int i = 0; i < config.QuickSwitchTotal; i++)
                            ListContainer.push_back(i);
                    }

                    if (BeginListBox("##QuickSwitchListBox", ImVec2(600, 200)))
                    {
                        static int QuickSwitchIndex = 0;
                        int count = 0;
                        for (int i : ListContainer)
                        {
                            PushID(i);

                            IsSelectable = Selectable(to_string(ListContainer[i]).c_str(), QuickSwitchIndex == i, 0, { 30, 30 });
                            QuickSwitchIndex = (IsSelectable) ? i : QuickSwitchIndex;

                            SameLine(0, 10);
                            Text(config.UseChinese ? u8"右键CPS：%d" : "RCPS: %d", config.RQuickSwitch[i]);
                            SameLine(0, 10);
                            Text(config.UseChinese ? u8"左键CPS：%d" : "LCPS: %d", config.LQuickSwitch[i]);
                            SameLine(0, 10);
                            Text(config.UseChinese ? u8"快捷键" : "Shortcut");
                            SameLine();
                            SetNextItemWidth(150);
                            InputKey(("##QuickSwitchKey" + to_string(i)).c_str(), &config.QuickSwitchKey[i], config.UseChinese ? u8"请按下按键" : "Press a key");

                            SameLine(0, 10);

                            if (Button(config.UseChinese ? u8"设置" : "Settings"))
                                OpenPopup("ToolbarPopup");

                            // 定义弹出窗口的位置 - 让它出现在按钮旁边
                            SetNextWindowPos(GetItemRectMin(), ImGuiCond_Appearing, ImVec2(0, 1)); // 对齐到按钮左上角
                            if (BeginPopup("ToolbarPopup"))
                            {
                                Text(config.UseChinese ? u8"右键CPS" : "RCPS");
                                SameLine();
                                SliderInt(u8"##QuickSwitchRCPS", &config.RQuickSwitch[i], MIN_CPS, MAX_CPS, "%d%", ImGuiSliderFlags_AlwaysClamp);

                                Text(config.UseChinese ? u8"左键CPS" : "LCPS");
                                SameLine();
                                SliderInt(u8"##QuickSwitchLCPS", &config.LQuickSwitch[i], MIN_CPS, MAX_CPS, "%d%", ImGuiSliderFlags_AlwaysClamp);

                                EndPopup();
                            }

                            SameLine();

                            static const Texture2D button_texture1 = LoadTextureFromFile(Pics_Delete);
                            if (ImageButton("##qsc_delete", (ImTextureID)button_texture1, ImVec2(16, 16)))
                            {
                                ListContainer.erase(ListContainer.begin() + count);
                                config.RQuickSwitch[i] = 0;
                                config.LQuickSwitch[i] = 0;
                                config.QuickSwitchKey[i] = 0;
                                config.QuickSwitchTotal--;
                                QuickSwitchIndex = -1;
                            }

                            count++;
                            PopID();
                        }
                        EndListBox();

                        SameLine();

                        static const Texture2D image_texture2 = LoadTextureFromFile(Pics_Add);
                        if (ImageButton("##qsc_add", (ImTextureID)image_texture2, ImVec2(32, 32)))
                        {
                            ListContainer.reserve(MAX_QUICK_SWITCH + 1);
                            if (config.QuickSwitchTotal < MAX_QUICK_SWITCH)
                            {
                                config.QuickSwitchTotal++;
                                int num = FindFirstMissingNumber(ListContainer, MAX_QUICK_SWITCH);
                                ListContainer.push_back(num);
                                config.RQuickSwitch[num] = config.RCPS;
                                config.LQuickSwitch[num] = config.LCPS;
                            }
                        }
                    }
                }
                EndCard();

                EndTabItem();
            }
            
            if (BeginTabItem((ICON_FA_PALETTE + (config.UseChinese ? u8"个性化"s : "Custom"s)).c_str()))
            {
                BeginCard(config.UseChinese ? u8"背景图片" : "Background Image", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    Text(config.UseChinese ? u8"背景偏移" : "Background Offset");
                    {
                        Text(config.UseChinese ? u8"X坐标" : "X Coordinate");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##BgOffsetX", &BgOffsetX, -100, 100, config.UseChinese ? u8"%d像素" : "%dpx", ImGuiSliderFlags_AlwaysClamp);

                        Text(config.UseChinese ? u8"Y坐标" : "Y Coordinate");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##BgOffsetY", &BgOffsetY, -100, 100, config.UseChinese ? u8"%d像素" : "%dpx", ImGuiSliderFlags_AlwaysClamp);
                    }

                    Text(config.UseChinese ? u8"背景亮度" : "Y Coordinate");
                    SameLine();
                    SetNextItemWidth(300);
                    SetCursorPosX(300);
                    SliderInt(u8"##BgLightness", &BgLightness, 0, 100, u8"%d %", ImGuiSliderFlags_AlwaysClamp);

                    Text(config.UseChinese ? u8"背景缩放" : "Background Scale");
                    SameLine();
                    SetNextItemWidth(300);
                    SetCursorPosX(300);
                    SliderInt(u8"##BgScale", &BgScale, 0, 300, u8"%d %", ImGuiSliderFlags_AlwaysClamp);

                    static float bk_alph = GuiItemAlpha / 100.0f;
                    PushStyleColor(ImGuiCol_WindowBg, GetColorU32(ImGuiCol_WindowBg, bk_alph));
                    PushStyleColor(ImGuiCol_FrameBg, GetColorU32(ImGuiCol_FrameBg, bk_alph));
                    PushStyleColor(ImGuiCol_SliderGrab, GetColorU32(ImGuiCol_SliderGrab, bk_alph));
                    PushStyleColor(ImGuiCol_SliderGrabActive, GetColorU32(ImGuiCol_SliderGrabActive, bk_alph));
                    {
                        Text(config.UseChinese ? u8"GUI透明度" : "GUI Transparency");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##GuiItemAlpha", &GuiItemAlpha, 16, 100, "%d%", ImGuiSliderFlags_AlwaysClamp);
                        if (!IsItemHovered())
                            bk_alph = GuiItemAlpha / 100.0f;
                    }
                    PopStyleColor(4);

                    Toggle(config.UseChinese ? u8"自动调整图片大小" : "Auto Image Size", &BgAutoResize, ToggleFlags);

                    Text(config.UseChinese ? u8"背景图片：" : "Background Image: ");
                    SameLine();
                    if (Button(config.UseChinese ? u8"选择文件" : "Select File"))
                    {
                        static bool IsOpen = false;
                        if (!IsOpen)
                        {
                            thread th([]()
                            {
                                IsOpen = true;
                                OPENFILENAME ofn;
                                char szFileName[MAX_PATH]{};
                                ZeroMemory(&ofn, sizeof(ofn));
                                ofn.lStructSize = sizeof(ofn);
                                ofn.hwndOwner = NULL; // 可以设置为你的窗口句柄
                                ofn.lpstrFilter = "图片文件\0*.png;*.bmp;*.jpg\0\0";
                                ofn.lpstrFile = szFileName;
                                ofn.nMaxFile = MAX_PATH;
                                ofn.hwndOwner = GCHWND;
                                // 设置对话框标志，例如提示覆盖等
                                ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                                ofn.lpstrDefExt = "ini"; // 默认扩展名
                                if (GetOpenFileName(&ofn))
                                CopyFile(szFileName, Pics_Background.c_str(), false);
                                PrintLog("[File] 复制文件 "s + szFileName + " 到 " + Pics_Background, PL_INFO);
                                IsBgImageRefresh = true;
                                IsOpen = false;
                            });
                            th.detach();
                        }
                    }
                }
                EndCard();

                EndTabItem();
            }

            EndTabBar();
            break;
        }
        case Tab::Script:
        {
            TabsText((ICON_FA_CODE + (config.UseChinese ? u8"脚本"s : "Scripts"s)).c_str());
            
            {
                //-------------------------获取脚本--------------------------
                static int ScriptSettingsSelected = -1; // -1为没有打开任何设置
                static bool IsWillGotoShopTab = false; // 用于调整到其他tab

                static auto NothingsHereFunc = []()
                {
                    const ImVec2 WindowSize = GetWindowSize();

                    const ImVec2 BoxSize(380, 160);
                    const ImVec2 BoxPos((WindowSize.x - BoxSize.x) / 2, (WindowSize.y - BoxSize.y) / 2);

                    static Texture2D iamge_texture = LoadTextureFromFile(Pics_Empyt);
                    const string Title = config.UseChinese ? u8"&b这里什么都没有" : "&bThere's nothing here";
                    const ImVec2 TitleSize = CalcTextSize(Title.c_str());
                    const ImVec2 ImageSize(32, 32);
                    const float TitleYOffset = 12;
                    const ImVec2 ImagePos((BoxSize.x - TitleSize.x - ImageSize.x) / 2, TitleYOffset);
                    const ImVec2 TitlePos(ImagePos.x + ImageSize.x + 10, TitleYOffset + ((ImageSize.y - TitleSize.y) / 2)); // 10是padding

                    const ImVec2 LinePos(20, TitlePos.y + TitleSize.y + 16);
                    const float LineLength = BoxSize.x - LinePos.x - 20;
                    const string Info = config.UseChinese ? u8"你可以将脚本文件拖入进来。" : "You can drop the script file into here.";
                    const ImVec2 InfoSize = CalcTextSize(Info.c_str());
                    const ImVec2 InfoPos((BoxSize.x - InfoSize.x) / 2, LinePos.y + 16);

                    const ImVec2 ButtonSize(140, 36);
                    const ImVec2 ButtonPos((BoxSize.x - (ButtonSize.x * 2 + 20)) / 2, BoxSize.y - ButtonSize.y - 16);
                    const ImVec2 OriginCursorPos = GetCursorPos();

                    SetCursorPos(BoxPos);
                    //SetNextWindowBgAlpha(0.7f);
                    PushStyleColor(ImGuiCol_ChildBg, GetColorU32(GuiColor[ImGuiCol_FrameBg]));
                    BeginChild("##AvailableScripts_box", BoxSize, true, flags);
                    {
                        SetCursorPos(ImagePos);
                        ImageRounded((ImTextureID)iamge_texture, ImageSize, 4.0f);
                        SetCursorPos(TitlePos);
                        Text(Title.c_str());

                        const ImVec2 WindowPos = GetWindowPos();
                        const ImVec2 LinePos2(LinePos.x + WindowPos.x, LinePos.y + WindowPos.y);
                        GetForegroundDrawList()->AddLine(LinePos2, ImVec2(LinePos2.x + LineLength, LinePos2.y), GetColorU32(GuiColor[ImGuiCol_CheckMark]), 2.0f);

                        SetCursorPos(InfoPos);
                        Text(Info.c_str());

                        SetCursorPos(ButtonPos);
                        if (Button(config.UseChinese ? u8"论坛" : "Forum", ButtonSize))
                            ShellExecute(NULL, NULL, "https://bbs.mcgc.top/t/script", NULL, NULL, SW_SHOWNORMAL);
                        SameLine(0, 20);
                        if (Button(config.UseChinese ? u8"前往下载" : "Download", ButtonSize))
                            IsWillGotoShopTab = true;
                    }
                    EndChild();
                    PopStyleColor(); // ImGuiCol_ChildBg

                    SetCursorPos(OriginCursorPos);
                };

                static bool IsNeedRefushShopList = false;
                static bool IsShopTab = false;
                static bool IsShopListLoaded = false;
                bool IsScriptCreatorTab = false; // 切换到对应tab时，才设置为true，所以不用static
                const ImVec2 pos = GetCursorPos();
                if (BeginTabBar("##Script"))
                {
                    // shop放前面相当于先刷新shop，防止可用列表先加载导致检查更新失败
                    if (BeginTabItem((ICON_FA_STORE + (config.UseChinese ? u8"脚本商场"s : "Shop"s)).c_str(), NULL, IsWillGotoShopTab ? ImGuiTabItemFlags_SetSelected : 0))
                    {
                        IsShopTab = true;
                        static bool IsInit = false;
                        if (!IsInit)
                        {
                            ResetAnimationPos();
                            BeginAnimation();
                            IsInit = true;
                        }
                        IsWillGotoShopTab = false;
                        //-------------------------脚本图片和信息--------------------------
                        const ImVec2 pos = GetCursorPos();
                        static int CurrentPaper = 1;
                        static int TotalPaper = 1;
                        int count = 0;

                        static bool IsRefushSuccess = false; // 代替繁杂的mutex
                        static bool IsRefushList = true;
                        if (IsRefushList || IsNeedRefushShopList)
                        {
                            thread FuncThread([&]()
                            {
                                // 修复：原代码在 detach 线程中无锁 clear/重填 ScriptData::ShopList，
                                // 与主线程遍历、CheckUpdateScriptFromShop（另一 detach 线程）并发，
                                // vector 扩容 -> 迭代器失效 -> 0xC0000005。
                                // 改为先在临时列表填充，锁内一次性 swap。
                                vector<ScriptInfo> tmpList;
                                const int papers = RefushScriptShopList(tmpList, CurrentPaper, false, IsScriptUseGitee);
                                {
                                    lock_guard lock(ScriptData::ShopListMtx);
                                    ScriptData::ShopList.swap(tmpList);
                                    TotalPaper = papers;
                                    IsRefushSuccess = true;
                                    IsShopListLoaded = true;
                                }
                            });
                            FuncThread.detach();
                            IsRefushList = false;
                            IsNeedRefushShopList = false;
                        }

                        static const char* SourceNames[] = { config.UseChinese ? u8"稳定仓库源" : "Stable Repo Source", config.UseChinese ? u8"官方下载源" : "Official Source" };
                        static int SourceIndex = 1;
                        Text(config.UseChinese ? u8"下载源" : "Source");
                        SameLine();
                        SetNextItemWidth(110.0f);
                        if (Combo("##ScriptMarketSourceCombo", &SourceIndex, SourceNames, IM_ARRAYSIZE(SourceNames)))
                        {
                            IsRefushList = true;
                            IsNeedRefushShopList = true;
                            IsRefushSuccess = false; // 触发刷新动画
                        }
                        IsScriptUseGitee = (SourceIndex == 0);

                        if (IsRefushSuccess)
                        {
                            // 修复：遍历期间加读锁，防止刷新线程 swap 导致迭代器/元素引用失效
                            shared_lock shopLock(ScriptData::ShopListMtx);
                            for (const ScriptInfo& script : ScriptData::ShopList)
                            {
                                PushStyleColor(ImGuiCol_ChildBg, GetColorU32(GuiColor[ImGuiCol_FrameBg]));

                                PushID(count);
                                ImVec2 AnimPos = CalcWidgetsAnimHoriz(GetID(label.c_str()));
                                SetNextWindowPos(AnimPos);
                                BeginChild("##ScriptData::ShopList", ImVec2(673, 72), true, flags);
                                {
                                    DisableAnimation(true);
                                    PopStyleColor(); // ImGuiCol_ChildBg

                                    // 脚本图片
                                    {
                                        Texture2D iamge_texture = LoadTextureFromFile(script.texture, false);
                                        SetCursorPos(ImVec2(4.0f, 4.0f));
                                        ImageRounded((ImTextureID)iamge_texture, ImVec2(64, 64), 4.0f);
                                    }

                                    // 标题和信息文字
                                    {
                                        const char* TitleText = script.title.c_str();
                                        const float TitleHeight = CalcTextSize(TitleText).y;
                                        const float PosX = 4.0f + 64.0f + 6.0f;
                                        SetCursorPos(ImVec2(PosX, 4.0f));
                                        Text(TitleText);
                                        SetCursorPos(ImVec2(PosX, 4.0f + TitleHeight));
                                        TextColored((ImVec4)ImColor(165, 165, 165, 255), script.info.c_str());
                                    }

                                    ImVec2 regionMin = GetWindowContentRegionMin();
                                    ImVec2 regionMax = GetWindowContentRegionMax();
                                    const float WindowWidth = regionMax.x - regionMin.x;
                                    const float WindowHeight = regionMax.y - regionMin.y;

                                    // 版本号文本
                                    {
                                        const string text = ((config.UseChinese ? u8"版本:" : "Version:") + to_string(script.ver));
                                        float TextWidth = CalcTextSize(text.c_str()).x;
                                        SetCursorPos(ImVec2(WindowWidth - TextWidth - 6.0f, 4));
                                        TextColored((ImVec4)ImColor(150, 150, 150, 255), text.c_str());
                                    }
                                    // 按钮和其他的
                                    {
                                        const bool HasLinkButton = !script.link.empty();
                                        const string LinkButtonText = (ICON_FA_EARTH_AFRICA + (config.UseChinese ? u8"教程网站"s : "Web Guide"s));
                                        const float LinkButtonWidth = CalcTextSize(LinkButtonText.c_str()).x + 16;
                                        const float ImageButtonWidth = 32.0f;
                                        const float spacing = 6.0f;

                                        float BeginPosX = WindowWidth - ImageButtonWidth - spacing;
                                        if (HasLinkButton)
                                            BeginPosX -= LinkButtonWidth + spacing;
                                        SetCursorPosX(BeginPosX);

                                        // 教程链接按钮
                                        if (HasLinkButton)
                                        {
                                            SetCursorPosY(WindowHeight - 32);
                                            if (Button(LinkButtonText.c_str(), ImVec2(LinkButtonWidth, 32)))
                                            {
                                                // 值捕获：script 是 for 循环元素引用，线程执行时元素可能已失效
                                                thread th([scriptCopy = script]()
                                                {
                                                    ShellExecute(NULL, NULL, scriptCopy.link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                                });
                                                th.detach();
                                            }
                                            SameLine(0, spacing);
                                        }

                                        // 下载按钮 - ImageButtonWidth也是hight，因为是正方形，所以 w = h
                                        SetCursorPosY(WindowHeight - ImageButtonWidth);
                                        static const Texture2D image_texture1 = LoadTextureFromFile(Pics_Download);
                                        if (ImageButton("##script_download", (ImTextureID)image_texture1, ImVec2(ImageButtonWidth, ImageButtonWidth)))
                                        {
                                            thread FuncThread([scriptCopy = script]()
                                                {
                                                    bool IsDownloadSuccess = false;
                                                    if (IsScriptUseGitee)
                                                    {
                                                        const string filename = ScriptShopDir + "\\" + scriptCopy.name + ".gcdat";
                                                        const string target = ScriptPath + "\\" + scriptCopy.name + ".zip";
                                                        if (!CopyFile(filename.c_str(), target.c_str(), false))
                                                        {
                                                            // 修复：打印具体错误码（如 ERROR_FILE_NOT_FOUND/ERROR_PATH_NOT_FOUND）便于定位编码/路径问题
                                                            PrintLog("[ScriptMarket] 脚本下载失败：" + filename + "，错误：" + to_string(GetLastError()), PL_ERROR);
                                                            RenderTip(config.UseChinese ? (scriptCopy.name + u8"下载失败") : (scriptCopy.title + " Download Failed"), 2.0f, PT_INFO);
                                                        }
                                                        else
                                                            IsDownloadSuccess = true;
                                                    }
                                                    else
                                                    {
                                                        // 修复：市场源脚本名为 UTF-8（可能含中文，如"我的刀附"），
                                                        // 直接拼进窄字符路径会被 Windows 按 GBK 解释 → 乱码文件 + 错误 1113。
                                                        // 改用下载链接末尾的 ID 作为 ASCII 文件名（显示名仍由解压后 info.ini 的 title 提供）。
                                                        string id = scriptCopy.file.substr(scriptCopy.file.find_last_of('/') + 1);
                                                        if (id.empty())
                                                            id = "script";
                                                        for (char& c : id)
                                                            if (!isalnum((unsigned char)c)) c = '_';
                                                        const string target = ScriptPath + "\\download_" + id + ".zip";
                                                        if (!DownloadURLFilesRetry(scriptCopy.file, target))
                                                        {
                                                            PrintLog("[ScriptMarket] 脚本下载失败，URL：" + scriptCopy.file, PL_ERROR);
                                                            RenderTip(config.UseChinese ? (scriptCopy.name + u8"下载失败") : (scriptCopy.title + " Download Failed"), 2.0f, PT_INFO);
                                                        }
                                                        else
                                                            IsDownloadSuccess = true;
                                                    }
                                                    // 修复：原代码此处无条件弹"下载成功"，即使上面下载失败也会同时弹"失败+成功"两个提示。
                                                    // 改为仅在成功时提示。
                                                    if (IsDownloadSuccess)
                                                        RenderTip(config.UseChinese ? (scriptCopy.name + u8"下载成功") : (scriptCopy.title + " Download Success"), 2.0f, PT_INFO);
                                                    RefushScriptList(); // 刷新
                                                });
                                            FuncThread.detach();
                                        }
                                    }

                                    DisableAnimation(false);
                                }
                                EndChild();
                                PopID();

                                count++;
                            }

                            BeginDisabled(CurrentPaper <= 1);
                            {
                                if (Button(config.UseChinese ? u8"上一页" : "Back"))
                                {
                                    CurrentPaper--;
                                    IsRefushList = true;
                                    IsRefushSuccess = false;
                                }
                            }
                            EndDisabled();

                            SameLine();
                            Text(config.UseChinese ? u8"第 %d/%d 页" : "Page %d/%d", CurrentPaper, TotalPaper);
                            SameLine();

                            BeginDisabled(CurrentPaper >= TotalPaper);
                            {
                                if (Button(config.UseChinese ? u8"下一页" : "Next"))
                                {
                                    CurrentPaper++;
                                    IsRefushList = true;
                                    IsRefushSuccess = false;
                                }
                            }
                            EndDisabled();
                        }
                        else
                            RenderSkeletonList(10);
                        EndTabItem();
                    }

                    if (BeginTabItem((ICON_FA_CIRCLE_CHECK + (config.UseChinese ? u8"可用"s : "Available"s)).c_str()))
                    {
                        IsShopTab = false;
                        static bool IsCheckUpdate = false;
                        if (!IsCheckUpdate && IsShopListLoaded)
                        {
                            // 修复：lambda 无需捕获任何局部变量（只访问全局 ScriptData::Avaiable 与全局函数），
                            // 空捕获列表避免未来误用悬垂引用
                            thread FuncThread([]() 
                            {
                                // 写入锁
                                {
                                    lock_guard write_lock(ScriptData::AvaiableMtx);
                                    for (ScriptInfo& script : ScriptData::Avaiable)
                                        CheckUpdateScriptFromShop(script);
                                }
                                // 同步检测已安装列表，使已安装页也能显示更新按钮
                                {
                                    lock_guard write_lock2(ScriptData::InstalledMtx);
                                    for (ScriptInfo& script : ScriptData::Installed)
                                        CheckUpdateScriptFromShop(script);
                                }
                            });
                            FuncThread.detach();
                            IsCheckUpdate = true;
                        }

                        // 读取锁
                        shared_lock read_lock(ScriptData::AvaiableMtx);

                        const int TotalScript = ScriptData::Avaiable.size();
                        if (TotalScript == 0)
                            NothingsHereFunc();
                        //-------------------------脚本图片和信息--------------------------
                        const ImVec2 pos = GetCursorPos();
                        const int PaperSize = 10;
                        const int TotalPaper = (TotalScript + PaperSize - 1) / PaperSize;
                        static int CurrentPaper = 1;

                        // 显示内置文档的脚本ID，负数为不显示（或者是无）
                        int ShowDocsScriptID = -1;
                        // 文档内容
                        string DocsText;

                        const int MaxCount = min(PaperSize * CurrentPaper, TotalScript);
                        for (int count = (CurrentPaper - 1) * PaperSize; count < MaxCount; count++) // count不能等于MaxCount，不然会出现翻页后重复出现
                        {
                            ScriptInfo& script = ScriptData::Avaiable[count];
                            PushStyleColor(ImGuiCol_ChildBg, GetColorU32(GuiColor[ImGuiCol_FrameBg]));

                            PushID(count);
                            ImVec2 AnimPos = CalcWidgetsAnimHoriz(GetID(label.c_str()));
                            SetNextWindowPos(AnimPos);
                            BeginChild("##AvailableScripts", ImVec2(680, 72), true, flags);
                            {
                                DisableAnimation(true);
                                PopStyleColor(); // ImGuiCol_ChildBg

                                // 脚本图片
                                {
                                    Texture2D iamge_texture = LoadTextureFromFile(script.texture, false);
                                    SetCursorPos(ImVec2(4.0f, 4.0f));
                                    ImageRounded((ImTextureID)iamge_texture, ImVec2(64, 64), 4.0f);
                                }

                                // 标题、信息文字、权限显示、更新按钮
                                {
                                    const char* TitleText = script.title.c_str();
                                    const float TitleHeight = CalcTextSize(TitleText).y;
                                    const float PosX = 4.0f + 64.0f + 6.0f;
                                    SetCursorPos(ImVec2(PosX, 4.0f));
                                    Text(TitleText);
                                    SameLine();
                                    Text(ICON_FA_CIRCLE_INFO);
                                    AttachTooltip((script.RestrictedFuncsStr.length() > 0) ? 
                                        (((config.UseChinese ? u8"脚本请求的权限：\n" : "Permissions requested by the script:\n") + script.RestrictedFuncsStr).c_str()) : 
                                        (config.UseChinese ? u8"&7无风险" : "&7No Risks"));

                                    SetCursorPos(ImVec2(PosX, 4.0f + TitleHeight));
                                    TextColored((ImVec4)ImColor(165, 165, 165, 255), script.info.c_str());

                                    if (script.IsHasUpdate)
                                    {
                                        SameLine();
                                        if (Button((ICON_FA_DOWNLOAD + (config.UseChinese ? u8"更新"s : "Update"s)).c_str(), ImVec2(60, 32)))
                                        {
                                            thread FuncThread([ScriptCopy = script]() // 值捕获，避免 script 引用随 for 循环/并发删除失效
                                                {
                                                    if (UpdateScriptFromShop(ScriptCopy))
                                                        RenderTip(config.UseChinese ? (ScriptCopy.name + u8"更新成功") : (ScriptCopy.title + " Update Success"), 2.0f, PT_INFO);
                                                    else
                                                        RenderTip(config.UseChinese ? (ScriptCopy.name + u8"更新失败") : (ScriptCopy.title + " Update Failed"), 2.0f, PT_ERROR);
                                                    RefushScriptList(); // 刷新
                                                });
                                            FuncThread.detach();
                                        }
                                    }
                                }

                                ImVec2 regionMin = GetWindowContentRegionMin();
                                ImVec2 regionMax = GetWindowContentRegionMax();
                                const float WindowWidth = regionMax.x - regionMin.x;
                                const float WindowHeight = regionMax.y - regionMin.y;

                                // 版本号文本
                                {
                                    const string text = ((config.UseChinese ? u8"版本:" : "Version:") + to_string(script.ver));
                                    const float TextWidth = CalcTextSize(text.c_str()).x;
                                    const float PosX = WindowWidth - TextWidth;
                                    SetCursorPos(ImVec2(PosX, 4));
                                    TextColored((ImVec4)ImColor(150, 150, 150, 255), text.c_str());
                                }

                                // 按钮和其他的
                                {
                                    const bool HasLinkButton = !script.link.empty();
                                    const bool HasDocsButton = !script.docs.empty();
                                    const string LinkButtonText = (ICON_FA_EARTH_AFRICA + (config.UseChinese ? u8"教程网站"s : "Web Guide"s));
                                    const string DocsButtonText = (ICON_FA_FILE_WORD + (config.UseChinese ? u8"文档"s : "Docs"s));
                                    const float LinkButtonWidth = CalcTextSize(LinkButtonText.c_str()).x + 16;
                                    const float DocsButtonWidth = CalcTextSize(DocsButtonText.c_str()).x + 16;
                                    const float ImageButtonWidth = 32.0f;
                                    const float spacing = 6.0f;

                                    float BeginPosX = WindowWidth - ((ImageButtonWidth + spacing) * 2);
                                    if (HasLinkButton)
                                        BeginPosX -= LinkButtonWidth + spacing;
                                    if (DocsButtonWidth)
                                        BeginPosX -= DocsButtonWidth + spacing;
                                    SetCursorPosX(BeginPosX);

                                    if (!script.IsInstalled)
                                    {
                                        // 教程网站按钮
                                        if (HasLinkButton)
                                        {
                                            SetCursorPosY(WindowHeight - ImageButtonWidth);
                                            if (Button(LinkButtonText.c_str(), ImVec2(LinkButtonWidth, 32)))
                                            {
                                                // 值捕获：script 是 for 循环元素引用，线程执行时元素可能已失效
                                                thread th([scriptCopy = script]()
                                                {
                                                    ShellExecute(NULL, NULL, scriptCopy.link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                                });
                                                th.detach();
                                            }
                                            SameLine(0, spacing);
                                        }

                                        // 文档显示按钮
                                        if (HasDocsButton)
                                        {
                                            SetCursorPosY(WindowHeight - 32);
                                            if (Button(DocsButtonText.c_str(), ImVec2(DocsButtonWidth, 32)))
                                            {
                                                ShowDocsScriptID = count;
                                                auto func = [&DocsText](const string& path)
                                                {
                                                    ifstream file(path);
                                                    if (!file.is_open())
                                                    {
                                                        PrintLog("[Script] 无法打开文档文件：" + path, PL_ERROR);
                                                        return;
                                                    }
                                                    ostringstream ss;
                                                    ss << file.rdbuf();
                                                    DocsText = ss.str();
                                                };
                                                thread th(func, script.docs);
                                                th.detach();
                                            }
                                            SameLine(0, spacing);
                                        }

                                        // 安装 - ImageButtonWidth也是hight，因为是正方形，所以 w = h
                                        SetCursorPosY(WindowHeight - ImageButtonWidth);
                                        static const Texture2D image_texture1 = LoadTextureFromFile(Pics_Add);
                                        if (ImageButton("##script_install", (ImTextureID)image_texture1, ImVec2(ImageButtonWidth, ImageButtonWidth)))
                                        {
                                            ScriptInfo ScriptCopy = script; // 拷贝时仍持有 read_lock，引用有效
                                            thread FuncThread([ScriptCopy]() mutable
                                                {
                                                    const HANDLE hThread = nullptr;
                                                    ScriptCopy.hThread = hThread;
                                                    // 创建 ScriptState 条目（供"启用"开关/启动按键使用，IsRunning=false）
                                                    {
                                                        lock_guard lock(ScriptData::StatesMtx);
                                                        ScriptData::States[ScriptCopy.file];
                                                    }
                                                    {
                                                        lock_guard lock(ScriptData::InstalledMtx);
                                                        ScriptData::Installed.push_back(ScriptCopy);
                                                    }
                                                    {
                                                        lock_guard lock(ScriptData::AvaiableMtx);
                                                        for (ScriptInfo& s : ScriptData::Avaiable)
                                                        {
                                                            if (s.file == ScriptCopy.file)
                                                            {
                                                                s.hThread = hThread;
                                                                s.IsInstalled = true;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    {
                                                        lock_guard lock(ScriptData::StatesMtx);
                                                        ScriptData::States[ScriptCopy.file].IsRunning = true;
                                                    }
                                                    {
                                                        // 保存线程句柄，删除脚本时才能正确终止线程
                                                        const HANDLE hThread = StartScript(ScriptCopy.file);
                                                        if (hThread != NULL)
                                                        {
                                                            lock_guard lock(ScriptData::AvaiableMtx);
                                                            for (ScriptInfo& s : ScriptData::Avaiable)
                                                            {
                                                                if (s.file == ScriptCopy.file)
                                                                {
                                                                    s.hThread = hThread;
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                });
                                            FuncThread.detach();
                                        }
                                        SameLine(0, spacing);

                                        // 删除文件 - ImageButtonWidth也是hight，因为是正方形，所以 w = h
                                        SetCursorPosY(WindowHeight - ImageButtonWidth);
                                        static const Texture2D image_texture2 = LoadTextureFromFile(Pics_Delete);
                                        if (ImageButton("script_delete", (ImTextureID)image_texture2, ImVec2(ImageButtonWidth, ImageButtonWidth)))
                                        {
                                            thread FunThread([scriptCopy = script]()
                                            {
                                                DeleteDirectoryRecursively(ScriptTemp + "\\" + scriptCopy.name);
                                                DelFile(ScriptPath + "\\" + scriptCopy.name + ".zip");
                                                // 修复：原代码无锁 erase，与主线程（持读锁遍历 ScriptData::Avaiable）并发 → 崩溃。
                                                // 写入必须加写锁（主线程释放读锁后本线程才能执行）；
                                                // 同时不用外部捕获的旧索引 count，改为按名称查找后删除。
                                                {
                                                    lock_guard lock(ScriptData::AvaiableMtx);
                                                    for (auto it = ScriptData::Avaiable.begin(); it != ScriptData::Avaiable.end(); ++it)
                                                    {
                                                        if (it->name == scriptCopy.name)
                                                        {
                                                            ScriptData::Avaiable.erase(it);
                                                            break;
                                                        }
                                                    }
                                                }
                                                //RefushScriptList();
                                            });
                                            FunThread.detach();
                                        }
                                    }
                                    else
                                    {
                                        // 已安装状态文字
                                        const char* InstalledText = config.UseChinese ? u8"已安装" : "Installed";
                                        const ImVec2 TextSize = CalcTextSize(InstalledText);
                                        SetCursorPos(ImVec2(GetWindowWidth() - TextSize.x - 12.0f * 3, (GetWindowHeight() - TextSize.y) / 2));
                                        Text(InstalledText);
                                    }
                                }
                                DisableAnimation(false);
                            }
                            EndChild();
                            PopID();
                        }

                        if (ShowDocsScriptID >= 0)
                        {
                            ScriptInfo& script = ScriptData::Avaiable[ShowDocsScriptID];

                            if (BeginPopupWindow(config.UseChinese ? u8"脚本文档" : "Script Document"))
                            {
                                TextUnformatted(DocsText.c_str());
                                EndPopupWindow();
                            }
                        }

                        BeginDisabled(CurrentPaper <= 1);
                        {
                            if (Button(config.UseChinese ? u8"上一页" : "Back"))
                                CurrentPaper--;
                        }
                        EndDisabled();

                        SameLine();
                        Text(config.UseChinese ? u8"第 %d/%d 页" : "Page %d/%d", CurrentPaper, TotalPaper);
                        SameLine();

                        BeginDisabled(CurrentPaper >= TotalPaper);
                        {
                            if (Button(config.UseChinese ? u8"下一页" : "Next"))
                                CurrentPaper++;
                        }
                        EndDisabled();

                        EndTabItem();
                    }

                    if (BeginTabItem((ICON_FA_BOLT + (config.UseChinese ? u8"已安装"s : "Installed"s)).c_str()))
                    {
                        IsShopTab = false;
                        // 读取锁（提前到 size() 检查之前，原代码在锁外读 size() 与安装线程 push_back 并发 → 数据竞争）
                        shared_lock read_lock(ScriptData::InstalledMtx);
                        if (ScriptData::Installed.size() == 0)
                            NothingsHereFunc();
                        int count = 0;
                        for (const ScriptInfo& script : ScriptData::Installed)
                        {
                            PushStyleColor(ImGuiCol_ChildBg, GetColorU32(GuiColor[ImGuiCol_FrameBg]));

                            PushID(count);
                            ImVec2 AnimPos = CalcWidgetsAnimHoriz(GetID(label.c_str()));
                            SetNextWindowPos(AnimPos);
                            BeginChild("##ScriptData::Installed", ImVec2(680, 72), true, flags);
                            {
                                DisableAnimation(true);
                                PopStyleColor(); // ImGuiCol_ChildBg

                                // 脚本图片
                                {
                                    Texture2D iamge_texture = LoadTextureFromFile(script.texture, false);
                                    SetCursorPos(ImVec2(4.0f, 4.0f));
                                    ImageRounded((ImTextureID)iamge_texture, ImVec2(64, 64), 4.0f);
                                }

                                ImVec2 regionMin = GetWindowContentRegionMin();
                                ImVec2 regionMax = GetWindowContentRegionMax();
                                const float WindowWidth = regionMax.x - regionMin.x;
                                const float WindowHeight = regionMax.y - regionMin.y;

                                // 标题、信息文字、权限显示、更新按钮
                                {
                                    const char* TitleText = script.title.c_str();
                                    const float TitleHeight = CalcTextSize(TitleText).y;
                                    const float PosX = 4.0f + 64.0f + 6.0f;
                                    SetCursorPos(ImVec2(PosX, 4.0f));
                                    Text(TitleText);
                                    SameLine();
                                    Text(ICON_FA_CIRCLE_INFO);
                                    AttachTooltip((script.RestrictedFuncsStr.length() > 0) ?
                                        (((config.UseChinese ? u8"脚本请求的权限：\n" : "Permissions requested by the script:\n") + script.RestrictedFuncsStr).c_str()) :
                                        (config.UseChinese ? u8"&7无风险" : "&7No Risks"));

                                    SetCursorPos(ImVec2(PosX, 4.0f + TitleHeight));
                                    TextColored((ImVec4)ImColor(165, 165, 165, 255), script.info.c_str());

                                    if (script.IsHasUpdate)
                                    {
                                        SameLine();
                                        if (Button((ICON_FA_DOWNLOAD + (config.UseChinese ? u8"更新"s : "Update"s)).c_str(), ImVec2(60, 32)))
                                        {
                                            thread FuncThread([ScriptCopy = script]() // 值捕获，避免 script 引用随 for 循环/并发删除失效
                                                {
                                                    if (UpdateScriptFromShop(ScriptCopy))
                                                        RenderTip(config.UseChinese ? (ScriptCopy.name + u8"更新成功") : (ScriptCopy.title + " Update Success"), 2.0f, PT_INFO);
                                                    else
                                                        RenderTip(config.UseChinese ? (ScriptCopy.name + u8"更新失败") : (ScriptCopy.title + " Update Failed"), 2.0f, PT_ERROR);
                                                    RefushScriptList(); // 刷新
                                                });
                                            FuncThread.detach();
                                        }
                                    }
                                }

                                {
                                    const float ImageButtonWidth = 32.0f;
                                    const float InputKeyWidth = 80.0f;
                                    const float spacing = 6.0f;

                                    float BeginPosX = WindowWidth - ((ImageButtonWidth + spacing) * 2) - (InputKeyWidth + spacing) - (40.0f + spacing); // 40.0f是toggle
                                    SetCursorPosX(BeginPosX);

                                    // 启动按键
                                    SetNextItemWidth(InputKeyWidth);
                                    {
                                        // 写入锁
                                        lock_guard lock(ScriptData::StatesMtx);
                                        // find 替代 operator[]，避免脚本已删除后仍被本循环复活僵尸 ScriptState 条目
                                        auto st = ScriptData::States.find(script.file);
                                        if (st != ScriptData::States.end())
                                        {
                                            SetCursorPosY(WindowHeight - 26.0f);
                                            InputKey("##script_key", &st->second.key, config.UseChinese ? u8"请按下按键" : "Press a key");
                                        }
                                    }
                                    SameLine(0, spacing);

                                    // 启用开关：关闭 -> IsSuspend = true -> 脚本线程安全睡眠（停止运行）
                                    // 修复：删除自动启动后，开关从"关闭"拨到"开启"且脚本线程未运行时，手动启动脚本线程
                                    SetCursorPosY(WindowHeight - 24.0f);
                                    {
                                        // 写入锁
                                        lock_guard lock(ScriptData::StatesMtx);
                                        // find 替代 operator[]，避免脚本已删除后仍被本循环复活僵尸 ScriptState 条目
                                        auto st = ScriptData::States.find(script.file);
                                        if (st != ScriptData::States.end())
                                        {
                                            bool IsEnable = !st->second.IsSuspend;
                                            if (Toggle("##script_enable", &IsEnable, ToggleFlags))
                                            {
                                                st->second.IsSuspend = !IsEnable;
                                                // 开启时若脚本线程尚未运行（含正在启动中），启动它，算是增强健壮了吧
                                                if (IsEnable && !st->second.IsRunning)
                                                {
                                                    st->second.IsRunning = true; // 乐观置位，防止重复启动
                                                    thread th([file = script.file]()
                                                    {
                                                        // 保存线程句柄，删除脚本时才能正确终止线程
                                                        const HANDLE hThread = StartScript(file);
                                                        if (hThread != NULL)
                                                        {
                                                            lock_guard lock(ScriptData::AvaiableMtx);
                                                            for (ScriptInfo& s : ScriptData::Avaiable)
                                                            {
                                                                if (s.file == file)
                                                                {
                                                                    s.hThread = hThread;
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                        {
                                                            lock_guard lock2(ScriptData::InstalledMtx);
                                                            for (ScriptInfo& s : ScriptData::Installed)
                                                            {
                                                                if (s.file == file)
                                                                {
                                                                    s.hThread = hThread;
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                    });
                                                    th.detach();
                                                }
                                            }
                                        }
                                    }
                                    AttachTooltip(config.UseChinese ? u8"关闭后：脚本停止运行\n开启时：若脚本未运行则启动脚本" : "Disabled: the script pauses\nEnabled: starts the script if it is not running");
                                    SameLine(0, spacing);

                                    // 脚本设置按键 - ImageButtonWidth也是hight，因为是正方形，所以 w = h
                                    SetCursorPosY(WindowHeight - ImageButtonWidth);
                                    static const Texture2D image_texture1 = LoadTextureFromFile(Pics_FunSetting);
                                    if (ImageButton("##script_settings", (ImTextureID)image_texture1, ImVec2(ImageButtonWidth, ImageButtonWidth)))
                                    {
                                        ScriptSettingsSelected = count;
                                        // 打开设置时暂停脚本，避免脚本与设置界面并发操作
                                        lock_guard lock(ScriptData::StatesMtx);
                                        auto st = ScriptData::States.find(script.file);
                                        if (st != ScriptData::States.end())
                                            st->second.IsSuspend = true;
                                    }
                                    SameLine(0, spacing);

                                    // 删除脚本 - ImageButtonWidth也是hight，因为是正方形，所以 w = h
                                    SetCursorPosY(WindowHeight - ImageButtonWidth);
                                    static const Texture2D image_texture2 = LoadTextureFromFile(Pics_Delete);
                                    if (ImageButton("##script_delete", (ImTextureID)image_texture2, ImVec2(ImageButtonWidth, ImageButtonWidth)))
                                    {
                                        thread FuncThread([](const ScriptInfo& script)
                                        {
                                            bool IsError = false;
                                            string errormsg;
                                            ScriptSettingsSelected = -1;
                                            {
                                                // 修复：原代码用 shared_lock（读锁）却修改 s.IsInstalled 并调用
                                                // CloseScriptThread（会写 s.hThread）→ 读锁内写的数据竞争。改为写锁。
                                                lock_guard lock(ScriptData::AvaiableMtx);
                                                // 将可用脚本列表的IsInstalled改为false
                                                for (ScriptInfo& s : ScriptData::Avaiable)
                                                {
                                                    if (s.file == script.file)
                                                    {
                                                        s.IsInstalled = false;
                                                        CloseScriptThread(s.file, s);
                                                        break;
                                                    }
                                                }
                                            }
                                            const string file = script.file;
                                            {
                                                // 修复：原代码 LuaStates.find 在锁外执行，与下方锁内 erase 并发 → 竞争。移入锁内。
                                                lock_guard lock(LuaStatesMtx);
                                                auto it = LuaStates.find(file);
                                                if (it != LuaStates.end())
                                                {
                                                    it->second.release();
                                                    LuaStates.erase(it);
                                                }
                                            }
                                            PrintLog("[Script] 已删除脚本：" + script.file, PL_INFO);
                                            {
                                                // 修复：原代码在主线程 read_lock.unlock() 后 erase ScriptData::Installed 再重锁，
                                                // 重锁后 range-for 迭代器失效 -> 崩溃。改为在此 detach 线程内按 file 查找删除
                                                // （独立加锁、不嵌套，避免与 ClearScriptCache 的锁序形成环）。
                                                lock_guard lock(ScriptData::InstalledMtx);
                                                for (auto it = ScriptData::Installed.begin(); it != ScriptData::Installed.end(); ++it)
                                                {
                                                    if (it->file == script.file)
                                                    {
                                                        ScriptData::Installed.erase(it);
                                                        break;
                                                    }
                                                }
                                            }
                                        }, script);
                                        FuncThread.detach();
                                    }
                                }

                                DisableAnimation(false);
                            }
                            EndChild();
                            PopID();

                            count++;
                        }
                        EndTabItem();
                    }

                    if (BeginTabItem((ICON_FA_CODE + (config.UseChinese ? u8"脚本开发"s : "Script Creator"s)).c_str(), NULL))
                    {
                        IsScriptCreatorTab = true;

                        static bool IsInit1 = false;
                        static bool IsInit2 = false;
                        if (!IsInit1 || !IsInit2)
                        {
                            if (!IsInit1)
                            {
                                ScriptEditor::InitEditor();
                                ScriptEditor::editor.SetText(config.UseChinese ? ScriptEditor::DefaultScriptCodeChinese : ScriptEditor::DefaultScriptCodeEnglish);
                                IsInit1 = true;
                            }
                            // 修复：CodeContext 由 detach 线程写入、主线程读取/clear，原代码无锁 → 竞争。
                            static string CodeContext = "null";
                            static mutex CodeContextMutex;
                            auto func = [&]()
                                {
                                    if (_access(ScriptTestFile.c_str(), 0) == 0)
                                    {
                                        ifstream file;
                                        file.open(ScriptTestFile);
                                        if (file.is_open())
                                        {
                                            stringstream buf;
                                            buf << file.rdbuf();
                                            {
                                                lock_guard lock(CodeContextMutex);
                                                CodeContext = buf.str();
                                            }
                                            return;
                                        }
                                    }
                                };
                            thread th(func);
                            th.detach();

                            {
                                lock_guard lock(CodeContextMutex);
                                if (CodeContext != "null")
                                {
                                    const string code = CodeContext;
                                    CodeContext.clear(); // 释放内存
                                    if (!code.empty())
                                        ScriptEditor::editor.SetText(StringToUTF8(code));
                                    IsInit2 = true;
                                }
                            }
                        }

                        DisableColorText(true);
                        ScriptEditor::Render();
                        DisableColorText(false);

                        EndTabItem();
                    }

                    if (BeginTabItem((ICON_FA_TRIANGLE_EXCLAMATION + (config.UseChinese ? u8"报错记录"s : "Errors"s)).c_str(), NULL))
                    {
                        IsScriptCreatorTab = true; // 也算
                        // 拷贝一份记录，避免持锁期间进行渲染
                        vector<ScriptErrorRecord> errorRecords;
                        {
                            shared_lock lock(ScriptData::ErrorRecordsMtx);
                            errorRecords = ScriptData::ErrorRecords;
                        }
                        if (errorRecords.empty())
                        {
                            // 空状态：居中提示，避免使用"无脚本"的引导弹窗
                            const ImVec2 WindowSize = GetWindowSize();
                            const string emptyText = config.UseChinese ? u8"暂无报错记录" : "No error records";
                            const ImVec2 TextSize = CalcTextSize(emptyText.c_str());
                            const ImVec2 TextPos((WindowSize.x - TextSize.x) / 2, (WindowSize.y - TextSize.y) / 2);
                            SetCursorPos(TextPos);
                            PushStyleColor(ImGuiCol_Text, GuiColor[ImGuiCol_TextDisabled]);
                            Text(emptyText.c_str());
                            PopStyleColor();
                        }
                        else
                        {
                            BeginChild("##script_error_list", ImVec2(0, 0), true);
                            {
                                // 最新的报错排在最前面
                                for (int i = (int)errorRecords.size() - 1; i >= 0; i--)
                                {
                                    const ScriptErrorRecord& record = errorRecords[i];
                                    PushID(i);
                                    // 从完整路径中取出文件名作为标题
                                    string shortName = record.file;
                                    const size_t splitPos = shortName.find_last_of("\\/");
                                    if (splitPos != string::npos)
                                        shortName = shortName.substr(splitPos + 1);
                                    const string header = shortName + "##script_error_" + to_string(i);
                                    if (CollapsingHeader(header.c_str()))
                                    {
                                        AttachTooltip(record.file.c_str());
                                        Text(((config.UseChinese ? u8"脚本路径："s : "Path: "s) + record.file).c_str());
                                        Text(((config.UseChinese ? u8"报错时间："s : "Time: "s) + record.time).c_str());
                                        string detail = record.message;
                                        InputTextMultiline(("##script_error_detail_" + to_string(i)).c_str(), &detail, ImVec2(-1, 160), ImGuiInputTextFlags_ReadOnly);
                                        DrawSpiltLine(GetContentRegionAvail().x, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                                    }
                                    PopID();
                                }
                            }
                            EndChild();
                        }
                        EndTabItem();
                    }

                    EndTabBar();
                }
                
                SetCursorPos(ImVec2(pos.x + (IsScriptCreatorTab ? 384 : 528), pos.y - 18));
                if (IsScriptCreatorTab)
                {
                    static bool IsRunning = false;
                    static HANDLE ThreadHandle = nullptr;
                    static int StartKey = VK_END; // 用于持久化按键信息，防止每次启动都要设置
                    
                    const float PosX = GetCursorPosX();
                    SetCursorPosY(pos.y - 22);
                    Text(config.UseChinese ? u8"启动按键" : "Start Key");

                    SetCursorPos(ImVec2(PosX, pos.y - 2));
                    SetNextItemWidth(100);
                    {
                        // 修复：原代码 ScriptData::States.count(ScriptTestFile) 在锁外执行（与 ScriptKeyControlThread
                        // 的遍历、StartScript 线程的插入并发 → map 结构竞争）。移入写锁内。
                        // 写入锁
                        lock_guard lock(ScriptData::StatesMtx);
                        const bool IsNoState = !IsRunning || (ScriptData::States.count(ScriptTestFile) == 0);
                        InputKey("##script_tset_key", &StartKey, config.UseChinese ? u8"请按下按键" : "Press a key");
                    }

                    SameLine();
                    SetCursorPosY(pos.y - 18);
                   
                    if (!IsRunning)
                    {
                        static const Texture2D image_texture = LoadTextureFromFile(Pics_Run);
                        if (ButtonImage((ImTextureID)image_texture, config.UseChinese ? u8"运行" : "Run", ImVec2(80, 40)))
                        {
                            auto func = [&](const string& code)
                                {
                                    ofstream file(ScriptTestFile, ios::out | ios::trunc);
                                    if (!file.is_open())
                                    {
                                        PrintLog("[Script] 无法打开或者创建文件：" + ScriptTestFile, PL_ERROR);
                                        RenderTip(config.UseChinese ? u8"无法打开或者创建文件\n日志已记录详细信息" : "Unable to open or create file.\nThe log has recorded detailed info.", 5.0f, PT_ERROR);
                                        return;
                                    }
                                    file.clear();
                                    file << UTF8ToString(code, true);
                                    file.close(); // 不要忘记

                                    ThreadHandle = StartScript(ScriptTestFile); // 自动创建state在ScriptStates中
                                    {
                                        // 修复：原代码在 detach 线程中无锁 operator[] 插入/修改 ScriptData::States，
                                        // 与 ScriptKeyControlThread 的遍历并发 → map 结构竞争。加写锁。
                                        lock_guard lock(ScriptData::StatesMtx);
                                        ScriptData::States[ScriptTestFile].key = StartKey;  // 设置默认启动按键为End，方便一点，一定要在StartScript之后赋值（因为如果在StartScript之前，会导致ScriptStates[file]被创建，导致ScriptStates不初始化）
                                    }
                                    RenderTip(config.UseChinese ? u8"脚本开始运行" : "Script starts running", 3.0f, PT_INFO);
                                    IsRunning = true;
                                };
                            thread th(func, ScriptEditor::editor.GetText());
                            th.detach();
                        }
                    }
                    else
                    {
                        static const Texture2D image_texture = LoadTextureFromFile(Pics_Stop);
                        if (ButtonImage((ImTextureID)image_texture, config.UseChinese ? u8"停止" : "Stop", ImVec2(80, 40)))
                        {
                            auto func = [=]()
                                {
                                    ScriptInfo temp;
                                    temp.hThread = ThreadHandle;
                                    CloseScriptThread(ScriptTestFile, temp);
                                    RenderTip(config.UseChinese ? u8"脚本已停止运行" : "The script has stopped.", 3.0f, PT_INFO);
                                    IsRunning = false;
                                };
                            thread th(func);
                            th.detach();
                        }
                    }

                    SameLine();
                    SetCursorPosY(pos.y - 18);

                    BeginDisabled(!IsRunning);
                    {
                        static const Texture2D image_texture = LoadTextureFromFile(Pics_Setting);
                        if (ButtonImage((ImTextureID)image_texture, config.UseChinese ? u8"脚本配置" : "Script Cfg", ImVec2(100, 40)))
                        {
                            ScriptSettingsSelected = 999999999;
                            // 写入锁
                            lock_guard lock(ScriptData::StatesMtx);
                            ScriptData::States[ScriptTestFile].IsSuspend = true;
                        }
                    }
                    EndDisabled();
                }
                else
                {
                    static const Texture2D image_texture1 = LoadTextureFromFile(Pics_Refresh);
                    if (ImageButton(config.UseChinese ? u8"刷新" : "Refresh", (ImTextureID)image_texture1, ImVec2(32, 32)))
                    {
                        if (IsShopTab)
                            IsNeedRefushShopList = true;
                        else
                        {
                            thread FunThread(RefushScriptList);
                            FunThread.detach();
                        }
                    }

                    SameLine();

                    static const Texture2D image_texture2 = LoadTextureFromFile(Pics_Book);
                    if (ImageButton(config.UseChinese ? u8"脚本文档" : "Script Doc", (ImTextureID)image_texture2, ImVec2(32, 32)))
                    {
#ifdef USE_IN_MICROSOFT
                        ShellExecute(NULL, NULL, "https://bbs.mcgc.top/d/214", NULL, NULL, SW_SHOWNORMAL);
#else
                        ShellExecute(NULL, NULL, "https://wiki.mcgc.top/zh/script/start", NULL, NULL, SW_SHOWNORMAL);
#endif // USE_IN_MICROSOFT
                    }

                    SameLine();

                    static bool IsClearing = false;
                    static const Texture2D image_texture3 = LoadTextureFromFile(Pics_ClearCache);
                    if (ImageButton(config.UseChinese ? u8"清除缓存" : "Clear Cache", (ImTextureID)image_texture3, ImVec2(32, 32)))
                    {
                        if (!IsClearing)
                        {
                            thread FunThread([]()
                                {
                                    ClearScriptCache();
                                    IsClearing = false;
                                });
                            FunThread.detach();
                            IsClearing = true;
                        }
                    }
                    AttachTooltip(config.UseChinese ? u8"清除脚本缓存，重新加载所有脚本\n如果你是脚本开发者，每次修改完脚本都要清除缓存\n否则会导致修改不起作用。\n&c目前因为BUG，可能会导致GC崩溃" : "Clear script cache and reload all scripts. \nIf you are a script developer, \nyou need to clear the cache every time you modify the script. \nOtherwise, the modification will not take effect.\n&cCurrently, due to a BUG, the GC may crash.");
                }
                //-----------------------脚本设置GUI---------------------------

                if (ScriptSettingsSelected > -1)
                {
                    const bool IsTestScript = (ScriptSettingsSelected == 999999999);
                    ScriptInfo script;
                    if (!IsTestScript)
                    {
                        // 读取锁
                        shared_lock lock(ScriptData::InstalledMtx);
                        // 修复：ScriptSelected 是打开设置时的列表索引，若期间脚本被删除（列表变短）
                        // 会越界访问 → 崩溃。加边界检查，越界则关闭设置。
                        if (ScriptSettingsSelected >= (int)ScriptData::Installed.size())
                            ScriptSettingsSelected = -114514;
                        else
                            script = ScriptData::Installed[ScriptSettingsSelected];
                    }
                    if (IsTestScript || ScriptSettingsSelected > -1)
                    {
                        const string title = IsTestScript ? (config.UseChinese ? u8"测试脚本设置" : "Test Script Settings") : (script.title + (config.UseChinese ? u8"设置" : "Settings"));
                        const string& filename = IsTestScript ? ScriptTestFile : script.file;

                        static bool IsOpened = true;
                        bool RunState = true;
                       
                        const bool ret = BeginPopupWindow(title.c_str(), &IsOpened);
                        if (ret)
                        {
                            RunState = RenderScriptGui(filename);
                            EndPopupWindow();
                        }

                        // 是否关闭或者崩溃
                        if (!ret || !RunState)
                        {
                            IsOpened = true; // 重置打开状态
                            ScriptSettingsSelected = -114514;
                            thread FuncThread(SaveScriptConfig, filename);
                            FuncThread.detach();
                            {
                                // 写入锁
                                lock_guard lock2(ScriptData::StatesMtx);
                                // find 替代 operator[] -- 脚本可能在弹窗期间被删除，
                                // operator[] 会插入僵尸 ScriptState 条目
                                auto st = ScriptData::States.find(filename);
                                if (st != ScriptData::States.end())
                                    st->second.IsSuspend = false;
                            }
                            PrintLog("[Script] 已保存脚本配置：" + filename, PL_INFO);
                        }
                    }
                }
            }

            break;
        }
        case Tab::Other://-----------------------其他---------------------------
        {
            TabsText((ICON_FA_BARS_PROGRESS + (config.UseChinese ? u8"其他"s : "Other"s)).c_str());

            BeginTabBar("##Other");
            
            static int IsOpenLnagPaper = -1;
            if (IsOpenLnagPaper == 1)// 这个函数告诉ImGui，下一个要绘制的标签项应该处于打开/激活状态
                ImGui::SetNextItemOpen(true);
            if (BeginTabItem((ICON_FA_SLIDERS + (config.UseChinese ? u8"软件设置"s : "Software Setings"s)).c_str(), NULL))
            {
                //-----------------------语言设置---------------------------
                {
                    const static array<const char*, 2> LanguageList = { u8"中文", "English" };
                    static int LanguageSelected = 0;

                    if (IsOpenLnagPaper != 0) // 重置激活状态
                        IsOpenLnagPaper = -1;

                    Text(config.UseChinese ? u8"语言" : "Language");
                    SameLine();
                    SetNextItemWidth(150);
                    if (BeginCombo("##Language", LanguageList[LanguageSelected]))
                    {
                        for (int n = 0; n < 2; n++)
                        {
                            const bool is_selected = (LanguageSelected == n);
                            if (Selectable(LanguageList[n], is_selected))
                            {
                                LanguageSelected = n;
                                RenderTip(config.UseChinese ? u8"部分文字需要重启应用才能生效" : "Some text may need to restart\nthe app to take effect.", 3.0f, PT_INFO);
                            }
                            if (is_selected)
                                SetItemDefaultFocus();
                        }
                        EndCombo();
                    }

                    if (LanguageSelected == 0)
                        config.UseChinese = true;
                    else
                        config.UseChinese = false;

                    static bool b_Lang = config.UseChinese;
                    if (b_Lang != config.UseChinese)
                    {
                        b_Lang = config.UseChinese;
                        IsOpenLnagPaper = 1;
                    }

                    if (!config.UseChinese)
                        Text("&bThe text content is translated by AI and may contain errors.\nYou can report this issue at https://bbs.mcgc.top.");
                }

                //-----------------------脚本自动启动---------------------------
                {
                    Text(config.UseChinese ? u8"脚本自动启动" : "Script Auto-Start");
                    SameLine();
                    bool DisableAutoStart = DisableScriptAutoStart;
                    if (Toggle("##disable_script_autostart", &DisableAutoStart, ToggleFlags))
                        DisableScriptAutoStart = DisableAutoStart;
                    AttachTooltip(config.UseChinese ?
                        u8"关闭：启动软件时自动运行所有已安装脚本\n开启：不自动运行，需在\"已安装\"列表中手动启动" :
                        "Off: auto-run all installed scripts on launch\nOn: start scripts manually in the \"Installed\" list");
                }
                EndTabItem();
            }

            if (BeginTabItem((ICON_FA_LIST_UL + (config.UseChinese ? u8"日志"s : "Logs"s)).c_str(), NULL))
            {
                // LogContext 由日志线程（Log.h 写端加 LogContextMtx）持续追加，主线程需加锁读取
                lock_guard lock(LogContextMtx);
                string log = StringToUTF8(LogContext, true);
                InputTextMultiline("##log_show", &log, ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
                EndTabItem();
            }

            EndTabBar();

            break;
        }
        case Tab::About:
        {
            TabsText((ICON_FA_CIRCLE_INFO + (config.UseChinese ? u8"关于"s : "About"s)).c_str());

            BeginTabBar("##About");

            if (BeginTabItem((ICON_FA_ADDRESS_CARD + (config.UseChinese ? u8"关于我们"s : "About"s)).c_str()))
            {
                const static ImGuiWindowFlags flags =
                    ImGuiWindowFlags_HorizontalScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoScrollbar;

                BeginCard(config.UseChinese ? u8"关于" : "About", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    Text(u8"GameClicker开发者：COM1919");
                    Text(u8"服务商：y_bowen");
                    Text(u8"YoClicker开发者：挽芙大魔王");
                }
                EndCard();

                TabsText((ICON_FA_LINK + (config.UseChinese ? u8"相关链接"s : "Related Links"s)).c_str());
                BeginCard(config.UseChinese ? u8"网站链接" : "Website Links", ImVec2(180, 190), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    if (Button(config.UseChinese ? u8"论坛" : "Forum"))
                        ShellExecute(NULL, NULL, "https://bbs.mcgc.top", NULL, NULL, SW_SHOWNORMAL);
                    if (Button(config.UseChinese ? u8"文档 && 维基百科" : "Document && Wikipedia"))
                    {
#ifdef USE_IN_MICROSOFT
                        ShellExecute(NULL, NULL, "https://bbs.mcgc.top/d/215", NULL, NULL, SW_SHOWNORMAL);
#else
                        ShellExecute(NULL, NULL, "https://wiki.mcgc.top", NULL, NULL, SW_SHOWNORMAL);
#endif // USE_IN_MICROSOFT
                    }
                }
                EndCard();
                SameLine();
                BeginCard(config.UseChinese ? u8"沙雕群" : "QQ Group", ImVec2(180, 190), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    if (Button(config.UseChinese ? u8"QQ群1" : "QQ Group 1"))
                        ShellExecute(NULL, NULL, "https://qm.qq.com/q/GFLkSmxR2o", NULL, NULL, SW_SHOWNORMAL);
                    if (Button(config.UseChinese ? u8"QQ群2" : "QQ Group 2"))
                        ShellExecute(NULL, NULL, "https://qm.qq.com/q/kuesbZeYIa", NULL, NULL, SW_SHOWNORMAL);
                    if (Button(config.UseChinese ? u8"QQ群3" : "QQ Group 3"))
                        ShellExecute(NULL, NULL, "https://qm.qq.com/q/lzWlCOTMeQ", NULL, NULL, SW_SHOWNORMAL);
                    if (Button(config.UseChinese ? u8"QQ群4" : "QQ Group 4"))
                        ShellExecute(NULL, NULL, "https://qm.qq.com/q/5oDpFEGnE4", NULL, NULL, SW_SHOWNORMAL);
                    if (Button(config.UseChinese ? u8"QQ群-YC/GC" : "QQ Group - YC/GC"))
                        ShellExecute(NULL, NULL, "https://qm.qq.com/q/daIs0oTYv6", NULL, NULL, SW_SHOWNORMAL);
                }
                EndCard();

                BeginCard(config.UseChinese ? u8"赞助列表(不分排名)" : "Sponsors (not ranked)", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    if (Sponsor::Mtx.try_lock_shared())
                    {
                        int count = -1;
                        for (string name : Sponsor::List)
                        {
                            const float TextWidth = CalcTextSize(name.c_str()).x;
                            count += TextWidth + 10;
                            if (count >= 680 - 50) // 680是BeginChild的宽度，减去50是为了容错
                                count = 0;
                            else if (count != -1)
                                SameLine(0, 10);
                            Text(name.c_str());
                        }
                        Sponsor::Mtx.unlock_shared();
                    }
                }
                EndCard();

                //-----------------------依赖的库---------------------------

                BeginCard(config.UseChinese ? u8"依赖的库" : "Dependencies", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    // 1. Dear ImGui
                    Text(u8"1. Dear ImGui");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2014-2024 Omar Cornut.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 2. ImPlot
                    Text(u8"2. ImPlot");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2023 Evan Pezent.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 3. Stb
                    Text(u8"3. Stb");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2017 Sean Barrett.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 4. OpenAL
                    Text(u8"4. OpenAL");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright (c) 2006, Creative Labs Inc.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 5. SDL2
                    Text(u8"5. SDL2");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1997-2024 Sam Lantinga <slouken@libsdl.org>.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 6. Zlib
                    Text(u8"6. Zlib");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1995-2024 Jean-loup Gailly and Mark Adler.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 7. Minizip
                    Text(u8"7. Minizip");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1998-2010 Gilles Vollant.\nCopyright(C) 2007-2008 Even Rouault.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 8. Picojson
                    Text(u8"8. Picojson");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2009-2010 Cybozu Labs, Inc.\nCopyright(C) 2011-2014 Kazuho Oku.");

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 9. Lua
                    Text(u8"9. Lua");
                    SameLine();
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1994-2025 Lua.org, PUC-Rio.");
                }
                EndCard();

                //-----------------------版权与隐私--------------------------
                BeginCard(config.UseChinese ? u8"版权与隐私声明" : "Copyright && Privacy Statement", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    Text(config.UseChinese ? u8"软件版权 Copyright 2026 By COM1919"                   : "Software Copyright 2026 By COM1919");
                    Text(config.UseChinese ? u8"本软件仅供个人使用，不得用于任何商业用途。"           : "This software is for personal use only and cannot be used for any commercial purpose.");
                    Text(config.UseChinese ? u8"本软件尊重您的隐私，仅上传加密保护的设备ID和IP地址。" : "This software respects your privacy and only uploads encrypted device IDs and IP addresses.");
                    Text(config.UseChinese ? u8"本软件不提供任何形式的担保，不承担任何法律责任。"     : "This software does not provide any form of guarantee and is not responsible for any legal liability.");
                    Text(config.UseChinese ? u8"本软件使用了第三方库，其版权归属于其各自拥有者。"     : "This software uses third-party libraries, whose copyright belongs to them.");
                    Text(config.UseChinese ? u8"其他版权声明和协议见下方。"                           : "Other copyright statements and licenses are listed below.");

                    SetCursorPosY(GetCursorPosY() + 12);

                    Text(config.UseChinese ? u8"用户协议" : "Usage Help");
                    if (config.UseChinese)
                        TextUnformatted(AgreementCNString.c_str(), AgreementCNString.c_str() + AgreementCNString.size());
					else
                        TextUnformatted(AgreementENString.c_str(), AgreementENString.c_str() + AgreementENString.size());
                }
                EndCard();

                EndTabItem();
            }

            EndTabBar();
            break;
        }
    }
    EndChild();

    //=======================动画处理===========================

    if (IsAnime)
    {
        float speed = 0.15f * (1.0f - GetIO().DeltaTime);
        const int TagerAlpha = 2;
        static float AnimAlpha = 0;
        AnimAlpha = ImLerp<float>(AnimAlpha, TagerAlpha, speed);

        GetForegroundDrawList()->
            AddRectFilled(ImVec2(105, 8 + 32), ImVec2(790, 461 + 32), GetColorU32(ImVec4(0, 0, 0, (AnimAlpha >= 1.0f) ? 2.0f - AnimAlpha : AnimAlpha)), 1);

        if(AnimAlpha >= 0.85f)
            CurrentTab = NextTab;
        if (AnimAlpha >= 1.85f)
        {
            IsAnime = false;
            AnimAlpha = 0;
        }
    }

    //=======================用户协议重新显示===========================
    bool ForceOpen = IsNeedReshowAgreement; // 保证BeginPopupWindow一直开着，只能同意才能关闭
    if (BeginPopupWindow(config.UseChinese ? u8"用户协议更新" : "Agreement Update", &ForceOpen))
    {
        BeginChild(u8"##ua_child", ImVec2(711, 260), true, ImGuiWindowFlags_HorizontalScrollbar);
        SetWindowFontScale(0.8f);
        if (config.UseChinese)
            TextUnformatted(AgreementCNString.c_str(), AgreementCNString.c_str() + AgreementCNString.size());
        else
            TextUnformatted(AgreementENString.c_str(), AgreementENString.c_str() + AgreementENString.size());
        SetWindowFontScale(1.0f);
        EndChild();

        Text(config.UseChinese ? u8"&c本软件完全免费！！！\n&c如果你是购买的本软件，可以举报。" : "This software is completely free! \nIf you have purchased this software, you can report it.");

        static bool IsCheck = false;
        Toggle(config.UseChinese ? u8"我已阅读并同意用户协议" : "I have read and agree to the user agreement", &IsCheck, ToggleFlags);

        if (Button(config.UseChinese ? u8"不同意" : "Decline", ImVec2(70, 40)))
            IsGCExit = true;
        SameLine();
        BeginDisabled(!IsCheck);
        {
            if (Button(config.UseChinese ? u8"同意" : " Next", ImVec2(70, 40)) && IsCheck)
            {
                IsNeedReshowAgreement = false;
                thread th([](){
                    SaveConfig(GetPresetFile(PresetConfig::CurrentName));
                });
                th.detach();
            }
        }
        EndDisabled();
        EndPopupWindow();
    }

    //-----------------------绘制标题栏图标---------------------------
    static const Texture2D image_texture = LoadTextureFromFile(Pics_GCIcon);
    GetForegroundDrawList()->
        AddImage((ImTextureID)image_texture, ImVec2(4, 4), ImVec2(32, 32));

    End();

    RenderControl();

    // 修复：client_id 由网络线程写，此处每帧读取——锁内拷贝后使用，避免 string 并发读写的数据竞争
    string CurClientID;
    {
        lock_guard lock(ClientIDMtx);
        CurClientID = ClientID;
    }
    ImVec2 TextSize = CalcTextSize(CurClientID.c_str());
    GetForegroundDrawList()->AddText(ImVec2((WindowWidth - TextSize.x) / 2, 6), GetColorU32((ImVec4)ImColor(128, 255, 0, 5)), CurClientID.c_str());
}
// 哎~ 这代码写的真的好烂啊...
