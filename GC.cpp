#include "GuideGui.h"
#include "Script.h"
#include "StartAnim.h"
#include "Tray.h"
#include "GetInfo.h"
#include "Driver.h"
#include "GameGui.h"
#include <ctime>

//#include "CodeEditor.h"

typedef HRESULT(WINAPI* PFNSETPROCESSDESCRIPTION)(HANDLE, PCSTR);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    StartTime = GetNowMs();
#ifdef GC_REMOTE_DEBUG
    MessageBox(NULL, "已开启远程调试，点击确定继续", "GC", MB_OK);
#endif //GC_REMOTE_DEBUG

    // 初始化符号处理（用于更好的堆栈跟踪）
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
    // 设置崩溃处理器
    SetupCrashHandler();

    if (!IsProcessElevated())
    {
        char szPath[MAX_PATH];
        GetModuleFileName(NULL, szPath, MAX_PATH);

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = szPath;
        sei.nShow = SW_NORMAL;

        if (!ShellExecuteEx(&sei))
        {
            // 提权失败或用户取消，显示提示
            MessageBox(NULL, "该程序需要管理员权限才能运行。", "错误", MB_OK | MB_ICONERROR);
        }
        return -1; // 退出当前进程
    }

    if (GetSystemLanguage() == LANG_CHINESE)
        config.UseChinese = true;
    else
        config.UseChinese = false;

    GCInstance = hInstance;
    //服了，远古代码来的，千万不能改，否则会出现双开
    AntiMultiOpenMutex = CreateMutex(NULL, FALSE, _T("GameClick_MC1919"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if(AntiMultiOpenMutex)
            CloseHandle(AntiMultiOpenMutex);
        MessageBox(NULL, config.UseChinese ? "你已经打开了一个GC连点器了!" : "You can only run one instance of GC at a time.", config.UseChinese ? "提示" : "Warning", MB_OK | MB_ICONQUESTION);
        return -1145;
    }

    InitRandom();

    // 不管怎么try都抓不住你的心
    try
    {
        //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        GCMainLoop();
    }
    catch (exception& e)
    {
        ExceptionHandling(GetLastError(), e.what() + " - [Main]"s);
        PrintLog("[Exception] 程序发生崩溃，原因："s + e.what(), PL_ERROR);
        return -1919;
    }
    // 关闭互斥量
    if (AntiMultiOpenMutex)
        CloseHandle(AntiMultiOpenMutex);
    // 清除符号处理
    SymCleanup(GetCurrentProcess());
    return 1919;
}

DWORD WINAPI SystemMsgHookLoop(LPVOID lpParam)
{
#ifndef GC_NO_HOOK

    HHOOK Hook_MouseSound = SetWindowsHookEx(WH_KEYBOARD_LL, KeybdSoundHook, GetModuleHandle(NULL), 0);
    HHOOK Hook_Clicker = SetWindowsHookEx(WH_MOUSE_LL, ClickerControlHook, GetModuleHandle(NULL), 0);
    HHOOK Hook_KeyState = SetWindowsHookEx(WH_KEYBOARD_LL, KeyStateHookProc, GetModuleHandle(NULL), 0);
    HHOOK Hook_MouseState = SetWindowsHookEx(WH_MOUSE_LL, MouseStateHookProc, GetModuleHandle(NULL), 0);

    bool IsError = false;
    if (Hook_Clicker == NULL)
    {
        PrintLog("[Hook] 无法设置鼠标钩子", PL_ERROR);
        IsError = true;
    }
    if (Hook_MouseSound == NULL)
    {
        PrintLog("[Hook] 无法设置鼠标声音钩子", PL_ERROR);
        IsError = true;
    }
    if (Hook_KeyState == NULL)
    {
        PrintLog("[Hook] 无法设置按键状态钩子", PL_ERROR);
        IsError = true;
    }
    if (Hook_MouseState == NULL)
    {
        PrintLog("[Hook] 无法设置鼠标状态钩子", PL_ERROR);
        IsError = true;
    }

    if (IsError)
    {
        RenderMessageBox(config.UseChinese ? "无法设置全局钩子，这会导致部分功能无法使用。" : "Cannot set mouse sound hook, \nsome functions may not work.", config.UseChinese ? "错误" : "Error", MB_OK);
        return 0;
    }
    PrintLog("[Hook] 鼠标钩子安装成功", PL_INFO);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (GCExit) break;
    }

    PrintLog("[Hook] 卸载鼠标HOOK", PL_INFO);
    UnhookWindowsHookEx(Hook_Clicker);
    UnhookWindowsHookEx(Hook_MouseSound);
    UnhookWindowsHookEx(Hook_KeyState);
    UnhookWindowsHookEx(Hook_MouseState);
#endif // !GC_NO_HOOK
    return 0;
}

int GCMainLoop()
{
#ifdef GC_DEBUG_CMD
    if (!AllocConsole())
    {
        OutputDebugString("[Init] 无法分配控制台窗口");
        return -2;
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

    InitLogSystem(LogFile); // 初始化日志系统，启动日志线程

    cout << "[彩蛋] 你是怎么看到我的呢，好难猜啊\n这么闲的吗？" << endl;

    // 一定要在下面文件初始化前面
#ifdef GC_DEBUG_GUIDE_UI
    IsFirstRun = true;
#else
    IsFirstRun = (_access(IniPos.c_str(), 0) != 0);
#endif // GC_DEBUG_GUIDE_UI

    if (_access(LogFile.c_str(), 0) != 0)
        CreateFile(LogFile);
    else
    {
        PackageLogFile();
        ClearFile(LogFile);
    }

    PrintLog("[Init] 开始加载动画", PL_INFO);
    CreateThreadSafe(LoadAnimationThread, NULL, 0, "LoadAnimationThread");
    CreateThreadSafe(DeleteOldLog, NULL, 0, "DeleteOldLog");
    
    char path[MAX_PATH]{};
    if (GetModuleFileName(NULL, path, MAX_PATH) == 0)
        PrintLog("[Init] 当前程序的绝对位置："s + path, PL_INFO);

    PrintLog("[Init] 检查文件完整性 - MainThread", PL_INFO);
    // 如果该文件存在就代表上一次程序没有正常退出
    if (_access(ExitFlagFile.c_str(), 0) == 0)
        AbnormalExit = true;
    else // 创建识别正常退出标识
        CreateFile(ExitFlagFile);

    // 在RefushScriptList之前删除
    if (DeleteDirectory(ScriptShopDir))
        PrintLog("[File] 删除脚本缓存成功！", PL_INFO);
    else
        PrintLog("[File] 删除脚本缓存失败！", PL_ERROR);
    if (DeleteDirectory(ScriptTemp))
        PrintLog("[File] 删除脚本商店缓存成功！", PL_INFO);
    else
        PrintLog("[File] 删除脚本商店缓存失败！", PL_ERROR);
    if (_access(IniPath.c_str(), 0) != 0)
        CreateDirectory(IniPath.c_str(), NULL);
    GetIni(IniPos);

    if (_access(LogPath.c_str(), 0) != 0)
        CreateDirectory(LogPath.c_str(), NULL);
    if (_access(ShaderLogFile.c_str(), 0) != 0)
        CreateFile(ShaderLogFile);
    if (_access(Tmp_TempPath.c_str(), 0) != 0)
        CreateDirectory(Tmp_TempPath.c_str(), NULL);

    CreateThreadSafe(WindowProcessMessagesThread, NULL, 0, "WindowProcessMessagesThread");

    while (!IsCreateWindowSussce)
        Sleep(1);

    if (!CreateDeviceD3D(GCHWND))
    {
        CleanupDeviceD3D();
        DestroyWindow(GCHWND);
        UnregisterClass(wcex.lpszClassName, wcex.hInstance);
        PrintLog("[Render] Dx11初始化失败", PL_ERROR);
        ExceptionHandling(GetLastError(), "Dx11初始化失败");
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
    while (!GCExit)
    {
        LastTime = clock();
        //===============================处理暂停渲染逻辑===============================
        bool IsWindowTopmost = (GetWindowLongPtr(GCHWND, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        // 当程序最小化时，不渲染UI
        static bool RenderStoped = false;
        const bool IsMinWindows = (MainGuiData.pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) && MainGuiData.SwapChainOccluded;
        const bool IsCoveredWindows = IsCoveredWindow(GCHWND);
        if ((!(ShowGui || Starting) || IsMinWindows || IsCoveredWindows) && !IsWindowTopmost)
        {
            if (!RenderStoped)
                RenderStoped = true;
            if (RenderSleepCount >= 0)
            {
                RenderSleepCount++;
                if (RenderSleepCount > (1000 * 6) / 20) // 超过6秒不使用UI，就优化内存使用
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
        //===============================处理其他渲染逻辑===============================
        // 处理窗口大小调整（我们不直接在WM_SIZE处理程序中调整大小）
        if (MainGuiData.ResizeWidth != 0 && MainGuiData.ResizeHeight != 0)
        {
            CleanupRenderTarget();
            MainGuiData.pSwapChain->ResizeBuffers(0, MainGuiData.ResizeWidth, MainGuiData.ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            MainGuiData.ResizeWidth = MainGuiData.ResizeHeight = 0;
            CreateRenderTarget();
        }
        
        //===============================处理渲染===============================
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        NewFrame();

        if (IsFirstRun)
        {
            const int state = RenderGuide(io);
            if (state == 1)
            {
                IsFirstRun = false;
                ReloadClicker = true;
                StopClicker = false;

                string name = RandomString(Random(6, 16));
                SetWindowText(GCHWND, name.c_str());
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
                GCExit = true;
        }
        else
            RenderGui(io);

        /*if (IsAnyItemHovered() || IsAnyItemActive())
            cout << "a\n";*/

        //=======================渲染===========================
        Render();
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
    //===============================结束===============================
    client.Send("exit");
    PrintLog("[Render] 释放GUI", PL_INFO);
    ReleasGui(wcex, &MainGuiData);

    PrintLog("[Render] 释放Dx11", PL_INFO);
    DragAcceptFiles(GCHWND, FALSE);
    ShowWindow(GCHWND, SW_HIDE);
    if (IsFirstRun)
        DelFile(IniPos.c_str());

    Shell_NotifyIcon(NIM_DELETE, &TrayNid);
    DeleteFile(ExitFlagFile.c_str());
    CloseAllLua();

    PrintLog("[Render] 等待其他线程退出 - 你是不是闲着来看日志了awa", PL_INFO);
    Sleep(450);
    PrintLog("[Render] GC连点器退出成功 - 开发者：COM1919", PL_INFO);
    return 0;
}

DWORD WINAPI InitGC(LPVOID lpParam)
{
    PrintLog("[Init] 创建线程", PL_INFO);
    CreateThreadSafe(RecordsCPS, NULL, 0, "RecordsCPS");
    SetThreadPriority(ClickRunThread, THREAD_PRIORITY_HIGHEST);
    ClickSoundThread = CreateThreadSafe(ClickSound, NULL, 0, "ClickSound");
    CreateThreadSafe(GetInfo, NULL, 0, "GetInfo");
    CreateThreadSafe(Updata, NULL, 0, "Updata");
    CreateThreadSafe(RecordsPCRate, NULL, 0, "RecordsPCRate");
    CreateThreadSafe(CreateTrayIcon, NULL, 0, "CreateTrayIcon");
    //CreateThreadSafe(ItemBarSwitch, NULL, 0, "ItemBarSwitch"); // 内存搜索的物品栏
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
    if (_access(DownloadPos.c_str(), 0) != 0)
        CreateDirectory(DownloadPos.c_str(), NULL);
    if (_access(ScriptPath.c_str(), 0) != 0)
    {
        CreateDirectory(ScriptPath.c_str(), NULL);
        InstalledScripts.clear();
        LuaStates.clear();
    }
    if (_access(ScriptPath.c_str(), 0) != 0)
        CreateDirectory(ScriptPath.c_str(), NULL);
    if (_access(ScriptTemp.c_str(), 0) != 0)
        CreateDirectory(ScriptTemp.c_str(), NULL);
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

    {
        // 写入锁
        lock_guard<shared_mutex> lock1(InstalledScriptsMtx);
        for (auto& script : InstalledScripts)
            script.hThread = StartScript(script.file);
        // 不用设置IsInstall，因为config里面已经设置了
    }

    // 允许拖拽文件到窗口
    DragAcceptFiles(GCHWND, true);

    CreateDirectory(ScriptShopDir.c_str(), NULL); // 注意顺序
    CreateDirectory(ScriptShopTemp.c_str(), NULL);
    CreateDirectory(ScriptTemp.c_str(), NULL);
    RefushScriptList();

    while (!IsCreateWindowSussce)
        Sleep(2);
    
    {
        lock_guard<shared_mutex> lock(InitSuccessTimeMutex);
        if (InitSuccessTime == 0 || InitSuccessTime < GetNowMs())
            InitSuccessTime = GetNowMs();
    }
    PrintLog("[Init] 初始化已完成，耗时: " + to_string(StartTime), PL_INFO);
    return 0;
}

DWORD WINAPI RecordsPCRate(LPVOID lpParam)
{
    while (!GCExit)
    {
        CpuRates.push_back(GetCpuUsageRate(200) / 100.0f);
        GpuRates.push_back(GetGpuUsageRate(200) / 100.0f);
        MemRates.push_back(GetMemUsageRate(200) / 100.0f);

        if(CpuRates.size() > MAX_PERF_RECORDS)
            CpuRates.erase(CpuRates.begin());

        if (GpuRates.size() > MAX_PERF_RECORDS)
            GpuRates.erase(GpuRates.begin());

        if(MemRates.size() > MAX_PERF_RECORDS)
            MemRates.erase(MemRates.begin());
        //GetCpuUsageRate等函数会进行Sleep

        /*
        size_t size = ScriptThreads.size();
        if(size <= 0)
            Sleep(200);
        else
        {
            for (int i = 0; i < size; i++)
            {
                vector<float> rates;
                rates.push_back(GetThreadCpuUsage(ClickSoundThread, 200) / 100.0f);
                if (rates.size() > MAX_PERF_RECORDS)
                    rates.erase(rates.begin());
                ScriptCpuRates.push_back(rates);
            }
        }*/
    }
    return 0;
}

void RenderGui(ImGuiIO& io)
{
    bool HasBgImage = _access(Bmp_Background.c_str(), 0) == 0;

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

    if (IsKeyPressed(ImGuiKey_F3))
        DebugInfo = !DebugInfo;

    //if (pb_message.open)
    //{
    //    RenderControl();
    //    return;
    //}

    if (IsKeyPressed(ImGuiKey_LeftCtrl) && IsKeyPressed(ImGuiKey_S))
    {
        ReloadClicker = true;
        RenderTip(config.UseChinese ? u8"已保存设置" : "Saved settings", 1.5f, PT_INFO);
    }

    PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.WindowPadding.x, 8));
    static const string label = StringToUTF8("       " + title).c_str();
    Begin(label.c_str(), &ShowGui,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoMove);
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
            texture = LoadTextureFromFile(Bmp_Background, true, D3D11_FILTER_MIN_MAG_MIP_LINEAR, &w, &h); // 禁用缓存，手动管理
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
    
    static bool ShowGui_Backup = ShowGui;
    if (ShowGui_Backup != ShowGui)
    {
        ShowWindow(GCHWND, ShowGui);
        if (ShowGui)
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
    static const Texture2D image_texture1 = LoadTextureFromFile(Bmp_Home);
    if (ButtonImage((ImTextureID)image_texture1, config.UseChinese ? u8"主页" : "Home", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Hub;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 134 + 32 });
    static const Texture2D image_texture2 = LoadTextureFromFile(Bmp_Info);
    if (ButtonImage((ImTextureID)image_texture2, config.UseChinese ? u8"信息" : "Info", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Info;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 184 + 32 });
    static const Texture2D image_texture3 = LoadTextureFromFile(Bmp_Setting);
    if (ButtonImage((ImTextureID)image_texture3, config.UseChinese ? u8"设置" : "Setting", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Setting;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 234 + 32 });
    static const Texture2D image_texture4 = LoadTextureFromFile(Bmp_Script);
    if (ButtonImage((ImTextureID)image_texture4, config.UseChinese ? u8"脚本" : "Script", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Script;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 284 + 32 });
    static const Texture2D image_texture5 = LoadTextureFromFile(Bmp_Other);
    if (ButtonImage((ImTextureID)image_texture5, config.UseChinese ? u8"其他" : "Other", { 80, 40 }))
    {
        LastTab = CurrentTab;
        NextTab = Tab::Other;
        IsAnime = true;
        ClickTabCount++;
    }

    SetCursorPos({ 10, 334 + 32 });
    static const Texture2D image_texture6 = LoadTextureFromFile(Bmp_About);
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
    Text(u8"CPS:%d | %d", CurrentCPS.left, CurrentCPS.right);

    ImDrawList* DrawList = GetWindowDrawList();
    ImVec2 RectPos = GetCursorPos();
    ImVec2 RectSize = { 20, 20 };
    ImRect rect(RectPos, ImVec2(RectPos.x + RectSize.x, RectPos.y + RectSize.y));

    for (int i = 0; i < 2; i++)
    {
        DrawList->AddRectFilled(rect.Min, rect.Max, ((i != 0) ? IsClickerDown.right : IsClickerDown.left) ? GetColorU32(GuiColor[ImGuiCol_Button]) : GetColorU32(GuiColor[ImGuiCol_FrameBg]), 6.0f);
        DrawList->AddRect(rect.Min, rect.Max, GetColorU32(GuiColor[ImGuiCol_Border]), 5.0f); // 圆角不使用 6 的原因是，在相同圆角值时AddRectFilled的圆角小一点
        DrawList->AddText(ImVec2(rect.Min.x + 5, rect.Min.y), ((i != 0) ? IsClickerDown.right : IsClickerDown.left) ? GetColorU32(GuiColor[ImGuiCol_TextDisabled]) : GetColorU32(GuiColor[ImGuiCol_Text]), (i == 0) ? "L" : "R");
        
        if ((IsClickering.left && i == 0) || (IsClickering.right && i == 1))
            DrawList->AddRect(ImVec2(rect.Min.x - 1, rect.Min.y - 1), ImVec2(rect.Max.x + 1, rect.Max.y + 1), GetColorU32((ImVec4)ImColor(176, 51, 205, 255)), 5.5f, 0, 3.0f);

        if ((!config.EnableLeftClicker && i == 0) || (!config.EnableRightClicker && i == 1))
            DrawList->AddRectFilled(rect.Min, rect.Max, GetColorU32((ImVec4)ImColor(56, 56, 56, 120)), 5.5f);

        //移动矩形
        rect.Translate(ImVec2(RectSize.x + 20, 0));
    }

    if (DebugInfo)
    {
        SetCursorPos({ RectPos.x, RectPos.y + 16 });
        Text(u8"%.1f FPS", io.Framerate);
        Text(u8"Human:%d|%d", HumanCPS.left, HumanCPS.right);
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
    const static ImGuiWindowFlags flags =
        ImGuiWindowFlags_HorizontalScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoScrollbar;
    EndAnimation();
    switch (CurrentTab)
    {
        case Tab::Hub:
        {
            TabsText(config.UseChinese ? u8"主页" : "Home");

            InputTextMultiline(u8"##notice", (char*)NoticeStr.c_str(), NoticeStr.size(), ImVec2(677, 230),
                ImGuiInputTextFlags_ReadOnly |
                ImGuiInputTextFlags_AllowTabInput |
                ImGuiInputTextFlags_NoHorizontalScroll);

            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoTitleBar;

            static const int StartupTime = InitSuccessTime - StartTime;
            static string DailyMotto = DailyMottos[Random(0, 29)];
            Text(config.UseChinese ? u8"&c本次启动耗时" : "&cStartup time");
            SameLine();
            Text(config.UseChinese ? u8"&a%d 毫秒" : "&a%d ms", StartupTime);
            SameLine();
            PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 0, 0, 0));
            PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0, 0, 0, 0));
            PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0, 0, 0, 0));
            {
                if (Button(DailyMotto.c_str()))
                    DailyMotto = DailyMottos.at(Random(0, 29));
            }
            PopStyleColor(3);

            unique_lock<shared_mutex> lock(AdListMutex, try_to_lock);
            if (lock.owns_lock() && AdList.size() >= 4)
            {
                const int count = AdList.size();
                for (int i = 0; i < count && i < 4; i++)
                {
                    PushID(i);
                    PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    BeginChild(u8"##AD", ImVec2(335, 70), true, flags);
                    {
                        SetCursorPosY(3);
                        ImDrawList* list = GetWindowDrawList();
                        if (AdList.at(i).texture != NULL)
                            ImageRounded((ImTextureID)AdList.at(i).texture, ImVec2(64, 64), 4.0f);
                        else
                            InvisibleButton("##I from China", ImVec2(64, 64));
                        SameLine();
                        SetCursorPosY(4);
                        Text(AdList.at(i).text.c_str());
                    }
                    EndChild();
                    PopStyleVar();

                    if (i != 3 && (i % 2) == 0)
                        SameLine();
                    PopID();
                }
            }

            break;
        }
        case Tab::Info:
        {
            TabsText(config.UseChinese ? u8"信息" : "Infomation");

            BeginTabBar("##Script");

            //-----------------------电脑性能---------------------------
            if (BeginTabItem(config.UseChinese ? u8"电脑性能" : "PC Performance"))
            {
                //-----------------------CPS监控信息---------------------------

                const static ImGuiWindowFlags flags =
                    ImGuiWindowFlags_HorizontalScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse;

                const float PosY = GetCursorPosY();

                PushStyleVar(ImGuiStyleVar_ScrollbarSize, 2);

                vector<float> temp;
                temp.resize(MAX_RECORDS, 0);

                RecordsCPSMutex.lock();
                copy(RRecordsCPS.begin(), RRecordsCPS.end(), temp.begin());
                rotate(temp.begin(), temp.begin() + RRecordsCPS.size(), temp.end());
                RecordsCPSMutex.unlock();

                Text(config.UseChinese ? u8"右键CPS监控" : "RCPS monitoring");
                const float PosY2 = GetCursorPosY();
                BeginChild("##RRecordsBeginChild", { 250, 120 }, true, flags);
                {
                    PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor(220, 220, 170, 255));
                    PlotLines(u8"##RRecordsCPS", temp.data(), MAX_RECORDS, 0, u8"", 0, FLT_MAX, { MAX_RECORDS, 110 });
                    PopStyleColor();
                    SetScrollHereX(1);
                }
                EndChild();

                SameLine();
                SetCursorPosY(PosY);

                const float PosX = GetCursorPosX();

                temp.clear();
                temp.resize(MAX_RECORDS, 0);

                RecordsCPSMutex.lock();
                copy(LRecordsCPS.begin(), LRecordsCPS.end(), temp.begin());
                rotate(temp.begin(), temp.begin() + LRecordsCPS.size(), temp.end());
                RecordsCPSMutex.unlock();

                Text(config.UseChinese ? u8"左键CPS监控" : "LCPS monitoring");
                SetCursorPos({ PosX, PosY2 });
                BeginChild("##LRecordsBeginChild", { 250, 120 }, true, flags);
                {
                    PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor(220, 220, 170, 255));
                    PlotLines(u8"##LRecordsCPS", temp.data(), MAX_RECORDS, 0, u8"", 0, FLT_MAX, { MAX_RECORDS, 110 });
                    PopStyleColor();
                    SetScrollHereX(1);
                }
                EndChild();

                PopStyleVar(); //ImGuiStyleVar_ScrollbarSize

                SameLine();

                RecordsCPSMutex.lock();
                auto rtemp = max_element(RRecordsCPS.begin(), RRecordsCPS.end());
                int Rmax = static_cast<int>(*rtemp);
                auto ltemp = max_element(LRecordsCPS.begin(), LRecordsCPS.end());
                int Lmax = static_cast<int>(*ltemp);
                RecordsCPSMutex.unlock();

                const float PosX2 = GetCursorPosX();

                Text(config.UseChinese ? u8"右键最高CPS：%d" : "highest RCPS: %d", Rmax);
                SameLine();
                SetCursorPos({ PosX2, GetCursorPosY() + 19 });
                Text(config.UseChinese ? u8"左键最高CPS：%d" : "highest LCPS: %d", Lmax);

                //-----------------------CPU/GPU/内存监控信息---------------------------

                DrawProgressBar(config.UseChinese ? u8"CPU占用率" : "CPU usage", CpuRates.back(), { 120, 120 }, GuiColor[ImGuiCol_CheckMark], GuiColor[ImGuiCol_FrameBg]);
                SameLine();
                DrawProgressBar(config.UseChinese ? u8"显卡占用率" : "GPU usage", GpuRates.back(), { 120, 120 }, GuiColor[ImGuiCol_CheckMark], GuiColor[ImGuiCol_FrameBg]);
                SameLine();
                DrawProgressBar(config.UseChinese ? u8"内存占用率" : "Memory usage", MemRates.back(), { 120, 120 }, GuiColor[ImGuiCol_CheckMark], GuiColor[ImGuiCol_FrameBg]);

                //-----------------------CPU/GPU---------------------------

                const int PosY3 = GetCursorPosY();
                TextColored((ImVec4)ImColor(17, 125, 187, 255), u8"CPU");
                SameLine();
                Text(u8"/");
                SameLine();
                TextColored((ImVec4)ImColor(222, 18, 20, 255), u8"GPU");
                SameLine();
                Text(config.UseChinese ? u8"占用率监控" : "CPS monitoring");

                //不用const staitc的，是因为这玩意任意错位
                float PosY4 = GetCursorPosY();
                BeginChild("##CPURecordsBeginChild", { 250, 120 }, true, flags);
                {
                    temp.clear();
                    temp.resize(MAX_RECORDS, 0);
                    copy(CpuRates.begin(), CpuRates.end(), temp.begin());
                    rotate(temp.begin(), temp.begin() + CpuRates.size(), temp.end());

                    const static ImVec2 Pos = GetCursorPos();
                    PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor(17, 125, 187, 255));
                    PlotLines(u8"##CPURecordsCPS", temp.data(), MAX_RECORDS, 0, u8"", 0, FLT_MAX, { MAX_RECORDS, 110 });
                    PopStyleColor();

                    temp.clear();
                    temp.resize(MAX_RECORDS, 0);
                    copy(GpuRates.begin(), GpuRates.end(), temp.begin());
                    rotate(temp.begin(), temp.begin() + GpuRates.size(), temp.end());

                    SetCursorPos(Pos);
                    PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor(222, 18, 20, 255));
                    PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0, 0, 0, 0));
                    PlotLines(u8"##GPURecordsCPS", temp.data(), MAX_RECORDS, 0, u8"", 0, FLT_MAX, { MAX_RECORDS, 110 });
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

                Text(config.UseChinese ? u8"内存占用率监控" : "Memory usage monitoring");
                SetCursorPos({ PosX4, PosY4 });
                BeginChild("##MemRecordsBeginChild", { 250, 120 }, true, flags);
                {
                    PlotLines(u8"##MemRecordsCPS", temp.data(), MAX_RECORDS, 0, u8"", 0, FLT_MAX, { MAX_RECORDS, 110 });
                    SetScrollHereX(1);
                }
                EndChild();


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
                    Text("%Lf", app_status.UseTime);

                    Text(config.UseChinese ? u8"本次使用GC的时长：" : "The duration of using GC this time:");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%Lf", ThisUseTime);

                    //-----------------------点击次数的比值-----------------------
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    Text(config.UseChinese ? u8"GC与手动点击次数的比值" : "The ratio of GC to manual clicks");

                    Text(config.UseChinese ? u8"左键" : "Left button");
                    ProgressBar((double)app_status.ClickerClickCount.left / max(app_status.ClickerClickCount.left + app_status.MouseClickCount.left, 1), ImVec2(300, 20));

                    Text(config.UseChinese ? u8"右键" : "Right button");
                    ProgressBar((double)app_status.ClickerClickCount.right / max(app_status.ClickerClickCount.right + app_status.MouseClickCount.right, 1), ImVec2(300, 20));

                    Text(config.UseChinese ? u8"左右键点击次数比值" : "Ratio of left and right button clicks");
                    ProgressBar((double)app_status.ClickerClickCount.left / max(app_status.ClickerClickCount.left + app_status.ClickerClickCount.right, 1), ImVec2(300, 20));

                    //-----------------------鼠标平均点击次数-----------------------
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    Text(config.UseChinese ? u8"每分钟手动平均点击次数：" : "Manual average clicks per minute:");

                    Text(config.UseChinese ? u8"左键" : "Left button");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%.1f", (double)app_status.MouseClickCount.left / max(ThisUseTime * 60, 1));

                    Text(config.UseChinese ? u8"右键" : "Right button");
                    SameLine();
                    SetCursorPosX(300);
                    Text("%.1f", (double)app_status.MouseClickCount.right / max(ThisUseTime * 60, 1));

                    //-----------------------GC平均点击次数-----------------------
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    Text(config.UseChinese ? u8"每分钟连点器平均点击次数：" : "Average clicker clicks per minute");

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
                        const char* LeftTitle = config.UseChinese ? u8"左键" : "Left button";
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
                        const char* RightTitle = config.UseChinese ? u8"右键" : "Right button";
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
            /*
            //-----------------------脚本性能---------------------------
            if (BeginTabItem(config.UseChinese ? u8"脚本性能" : "Script Setting"))
            {
                ImDrawList* DrawList = GetWindowDrawList();

                const static ImGuiWindowFlags flags =
                    ImGuiWindowFlags_HorizontalScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoScrollbar;

                TextColored((ImVec4)ImColor(132, 53, 79, 255), config.UseChinese ? u8"当脚本优化足够好时，CPU占用会出现几乎0的情况。" : "When the script is optimized well, the CPU usage will be almost 0.");
                
                size_t size = ScriptThreads.size();
                for (int i = 0; i < size; i++)
                {
                    BeginChild(("##ScriptChild" + to_string(i)).c_str(), ImVec2(660, 150), true, flags);
                    {
                        TextColored((ImVec4)ImColor(245, 87, 98, 255), InstallScriptName[i].c_str());

                        const static ImGuiWindowFlags flags =
                            ImGuiWindowFlags_HorizontalScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse;

                        const float PosY = GetCursorPosY();

                        PushStyleVar(ImGuiStyleVar_ScrollbarSize, 2);

                        vector<float> temp;
                        temp.resize(MAX_RECORDS, 0);

                        const vector<float>& rates = ScriptCpuRates[i];

                        copy(rates.begin(), rates.end(), temp.begin());
                        rotate(temp.begin(), temp.begin() + rates.size(), temp.end());

                        SameLine(0, 80);

                        Text(config.UseChinese ? u8"CPU占用率监控" : "CPU usage monitoring");
                        const float PosY2 = GetCursorPosY();
                        BeginChild("##ScriptRecordsBeginChild", { 500, 100 }, true, flags);
                        {
                            PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor(220, 220, 170, 255));
                            PlotLines(u8"##ScriptRecordsCPU", temp.data(), MAX_RECORDS, 0, u8"", 0, FLT_MAX, { MAX_RECORDS, 110 });
                            PopStyleColor();
                            SetScrollHereX(1);
                        }
                        EndChild();

                        SameLine();

                        DrawProgressBar(config.UseChinese ? u8"脚本CPU占用率" : "Script cpu usage", rates.back(), { 100, 100 }, GuiColor[ImGuiCol_ButtonActive], GuiColor[ImGuiCol_FrameBg]);

                        PopStyleVar(); //ImGuiStyleVar_ScrollbarSize
                    }
                }

                EndTabItem();
            }*/
            EndTabBar();
            
            break;
        }
        case Tab::Setting: //-----------------------设置---------------------------
        {
            static const Texture2D image_itembar = LoadTextureFromFile(Bmp_Itembar, false, D3D11_FILTER_MIN_MAG_MIP_POINT);
            static const Texture2D image_hotbar = LoadTextureFromFile(Bmp_HotBar, false, D3D11_FILTER_MIN_MAG_MIP_POINT);

            const ImVec2 Pos = GetCursorPos();
            TabsText(config.UseChinese ? u8"设置" : "Setting");

            const ImVec2 Pos2 = GetCursorPos();
            SetCursorPos(ImVec2(Pos.x + 500, Pos.y + 10));
            if (Button(config.UseChinese ? u8"保存设置" : "Save", ImVec2(80, 40)))
            {
                ReloadClicker = true;
                RenderTip(config.UseChinese ? u8"已保存设置" : "Saved Settings", 1.5f, PT_INFO);
            }
            SameLine();
            if (Button(config.UseChinese ? u8"重置配置" : "Reset Settings", ImVec2(80, 40)))
            {
                thread FunThread([]()
                    {
                        if (RenderMessageBox(config.UseChinese ? u8"真的要重置连点器的设置吗？" : "Are you sure you want to\nreset the clicker settings?", config.UseChinese ? u8"提示" : "TIPS", MB_OKCANCEL, true) == IDOK)
                        {
                            config = Config();
                            ReloadClicker = true; // 这个相当于保存了配置
                            RenderTip(config.UseChinese ? u8"重置成功" : "Reset success", 1.5f, PT_INFO);
                        }
                    });
                FunThread.detach();
            }
            SetCursorPos(Pos2);

            BeginTabBar("##Script");

            //-----------------------基本设置---------------------------
            if (BeginTabItem(config.UseChinese ? u8"基本设置" : "Basic Setting"))
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
                    InputKey("##RClickKey", &config.RKEY, config.UseChinese ? u8"请按下按键" : "Press a key");
                    if (config.RClickerMode & C_MOUSESMODE || config.RClickerMode & C_ASSISTMODE)
                        AttachTooltip(config.UseChinese ? u8"&b辅助模式 &f或 &b鼠标模式&f不需要设置快捷键" : "AssistMode or MouseMode does not need to set a shortcut key");

                    SameLine();

                    Text(config.UseChinese ? u8"右键CPS" : "RClicker CPS");
                    AttachTooltip(config.UseChinese ? u8"CPS指每秒连点次数，越高越快，\n一般右键设置 20CPS 就可以。" : "CPS means the number of clicks per second, the higher the faster, \ngenerally the left key sets 14CPS can be used.");
                    SameLine();
                    SetNextItemWidth(300);
                    if (config.UseRFluctuation && !config.UseBrutalMode)
                    {
                        static float MinCps = config.RCPS - config.RFluctuationCPS;
                        static float MaxCps = config.RCPS + config.RFluctuationCPS;
                        if (RangeSliderFloat(u8"##rcps", MIN_CPS, MAX_CPS, MinCps, MaxCps, "%.1fCPS"))
                        {
                            float mid = MinCps + (MaxCps - MinCps) / 2;
                            config.RCPS = round(mid);
                            config.RFluctuationCPS = round(mid - MinCps);
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
                    InputKey("##LClickKey", &config.LKEY, config.UseChinese ? u8"请按下按键" : "Press a key");
                    if (config.LClickerMode & C_MOUSESMODE || config.LClickerMode & C_ASSISTMODE)
                        AttachTooltip(config.UseChinese ? u8"&b辅助模式 &f或 &b鼠标模式&f不需要设置快捷键" : "AssistMode or MouseMode does not need to set a shortcut key");
                    else
                        AttachTooltip(config.UseChinese ? u8"如果需要实现 按住鼠标 来连点\n可以使用鼠标模式。" : "If you need to implement holding the mouse to click\nYou can use the mouse mode.");

                    SameLine();

                    Text(config.UseChinese ? u8"左键CPS" : "LClicker CPS");
                    AttachTooltip(config.UseChinese ? u8"CPS指每秒连点次数，越高越快，\n一般左键设置 14CPS 就可以。" : "CPS means the number of clicks per second, the higher the faster, \ngenerally the right key sets 20CPS can be used.");
                    SameLine();
                    SetNextItemWidth(300);
                    if (config.UseLFluctuation && !config.UseBrutalMode)
                    {
                        static float MinCps = config.LCPS - config.LFluctuationCPS;
                        static float MaxCps = config.LCPS + config.LFluctuationCPS;
                        if (RangeSliderFloat(u8"##lcps", MIN_CPS, MAX_CPS, MinCps, MaxCps, "%.1fCPS"))
                        {
                            float mid = MinCps + (MaxCps - MinCps) / 2;
                            config.LCPS = round(mid);
                            config.LFluctuationCPS = round(mid - MinCps);
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

                    Checkbox(config.UseChinese ? u8"右键CPS波动" : "RCPS Fluctuation", &config.UseRFluctuation);
                    SameLine();
                    Checkbox(config.UseChinese ? u8"左键CPS波动" : "LCPS Fluctuation", &config.UseLFluctuation);

                    BeginDisabled(!config.UseLFluctuation && !config.UseRFluctuation);
                    {
                        SameLine();
                        static bool IsOpen = false;
                        if (Button(config.UseChinese ? u8"随机曲线分布" : "Random Curve"))
                            IsOpen = !IsOpen;

                        Text(config.UseChinese ? u8"随机生成算法" : "Rand Algorithm");
                        SetNextItemWidth(200);
                        SameLine();
                        if (BeginCombo("##crandomgen", RandomGenList[config.Fluctuation_RandomGen]))
                        {
                            for (int n = 0; n < 8; n++)
                            {
                                const bool is_selected = (config.Fluctuation_RandomGen == n);
                                if (Selectable((RandomGenList[n] + "##"s + to_string(n)).c_str(), is_selected))
                                    config.Fluctuation_RandomGen = n;
                                if (is_selected)
                                    SetItemDefaultFocus();
                            }
                            EndCombo();
                        }

                        SameLine();

                        Text(config.UseChinese ? u8"随机生成分布" : "Rand Distribution");
                        SetNextItemWidth(200);
                        SameLine();
                        if (BeginCombo("##crandomdist", RandomDistList[config.Fluctuation_RandomDist]))
                        {
                            for (int n = 0; n < 2; n++)
                            {
                                const bool is_selected = (config.Fluctuation_RandomDist == n);
                                if (Selectable((RandomDistList[n] + "##"s + to_string(n)).c_str(), is_selected))
                                    config.Fluctuation_RandomDist = n;
                                if (is_selected)
                                    SetItemDefaultFocus();
                            }
                            EndCombo();
                        }

                        static LRPair<vector<WaveParam>> WaveList;
                        static bool lastOpenState = false;
                        // 打开弹窗时加载已保存的数据
                        if (IsOpen && !lastOpenState)
                        {
                            if (!config.CPSWaves.left.empty())
                                WaveList.left = config.CPSWaves.left;
                            if (!config.CPSWaves.right.empty())
                                WaveList.right = config.CPSWaves.right;
                        }
                        lastOpenState = IsOpen;

                        if (BeginPopupWindow(config.UseChinese ? u8"随机曲线分布" : "Random Curve", &IsOpen))
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
                                Text(config.UseChinese ? u8"左键波形设置" : "LB Wave Settings");
                            else
                                Text(config.UseChinese ? u8"右键波形设置" : "RB Wave Settings");

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
                                                random_device rd;
                                                mt19937 gen(rd());
                                                uniform_real_distribution<> ampDist(0.5f, 3.0f);
                                                uniform_real_distribution<> freqDist(0.05f, 2.0f);
                                                uniform_real_distribution<> phaseDist(-3.1415926535f, 3.1415926535f);

                                                WaveParam NewWave;
                                                NewWave.amplitude = (float)ampDist(gen);
                                                NewWave.frequency = (float)freqDist(gen);
                                                NewWave.phase = (float)phaseDist(gen);
                                                NewWave.enabled = true;
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
                                            static const Texture2D button_texture1 = LoadTextureFromFile(Bmp_Delete);
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

                                            Checkbox("Enabled", &w.enabled);
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
                                Checkbox(config.UseChinese ? u8"显示应用CPS后的曲线" : "Show applied CPS curve", &ShowAppliedCPS);

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
                        if (!IsOpen && lastOpenState)
                        {
                            config.CPSWaves.left = WaveList.left;
                            config.CPSWaves.right = WaveList.right;
                            IsCPSWaveSettingChanged = true;
                            lastOpenState = false; // 防止重复保存
                        }
                    }
                    EndDisabled();

                    Text(config.UseChinese ? u8"&7如果要设置为鼠标左右键，请使用鼠标模式。" : "&7If you want to set it to the mouse buttons, please use the Mouse Mode.");
                }
                EndCard();
                
                SetCursorPosY(GetCursorPosY() + 10.0f);
                
                BeginCard("##CPSSettings-1", ImVec2(180, 190), ImGuiChildFlags_Borders);
                {
                    //-------------------------暴力模式--------------------------
                    Checkbox(config.UseChinese ? u8"暴力模式" : "Brutal Mode", &config.UseBrutalMode);
                    AttachTooltip(config.UseChinese ? u8"让你的CPS轻松破百\n但可能会导致大量占用电脑CPU性能！\n并且可能会导致CPS记录器数据不准确！" : "Let your CPS easily break 100\nBut it may cause a lot of computer CPU performance!\nAnd it may cause the CPS recorder data to be inaccurate!");
                    
                    //-------------------------启用连点--------------------------
                    Checkbox(config.UseChinese ? u8"启用右键连点" : "Enable RClicker", &config.EnableRightClicker);
                    Checkbox(config.UseChinese ? u8"启用左键连点" : "Enable LClicker", &config.EnableLeftClicker);
                    if (Checkbox(config.UseChinese ? u8"开机自动启动" : "Start With System", &config.UseStartWithWindows))
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
                    if (Checkbox(config.UseChinese ? u8"适配&c新版&f基岩版" : "Adapt BE", &config.UseBEAdaptation))
                    {
                        if (config.UseBEAdaptation)
                        {
                            backup_mode = config.SimulateClickMode;
                            config.SimulateClickMode = SC_SEND_INPUT;
                            RenderTip(config.UseChinese ? u8"已开启适配基岩版\n已调整 &d模拟点击 &f模式为 &6Sned Input" : "Adaptation to BE has been turned on\nThe &dSimulate Click mode set to &6Send Input.", 2.2f, PT_INFO);
                        }
                        else
                            config.SimulateClickMode = backup_mode;
                    }

                    //-------------------------主播模式--------------------------
                    if (Checkbox(config.UseChinese ? u8"主播模式" : "Broadcaster Mode", &config.UseBroadcasterMode))
                    {
                        thread FunThread(SetWindowDisplayAffinity, GCHWND, config.UseBroadcasterMode ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
                        FunThread.detach();
                    }
                }
                EndCard();

                SameLine();

                BeginCard("##CPSSettings-2", ImVec2(250, 190), ImGuiChildFlags_Borders);
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
                    if (BeginCombo("##ClickerPriorityMode", ClickerPriorityListText[config.ClickerPriorityMode]))
                    {
                        for (int n = 0; n < 3; n++)
                        {
                            PushID(n);
                            const bool is_selected = (config.ClickerPriorityMode == n);
                            if (Selectable(ClickerPriorityListText[n], is_selected))
                                config.ClickerPriorityMode = n;
                            if (is_selected)
                                SetItemDefaultFocus();
                            PopID();
                        }
                        EndCombo();
                    }

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
                    Checkbox(config.UseChinese ? u8"允许控制右键连点开关" : "Allow Ctrl RC Enables", & config.AllowSetRClicker);
                    AttachTooltip(config.UseChinese ? u8"默认开启，关闭后，连点器将无法控制右键的开关状态。" : "The default is to turn on, and if you turn off,\nthe clicker will not be able to control the right-click switch status. ");
                    Checkbox(config.UseChinese ? u8"允许控制左键连点开关" : "Allow Ctrl LC Enables", & config.AllowSetLClicker);
                    AttachTooltip(config.UseChinese ? u8"默认开启，关闭后，连点器将无法控制左键的开关状态。" : "The default is to turn on, and if you turn off,\nthe clicker will not be able to control the left-click switch status. ");
                }
                EndCard();

                SameLine();

                BeginCard("##CPSSettings-3", ImVec2(235, 190), ImGuiChildFlags_Borders);
                {
                    Checkbox(config.UseChinese ? u8"过热保护" : "Feature Timeout", &config.UseOverheatProtec);

                    BeginDisabled(!config.UseOverheatProtec);
                    {
                        Text(config.UseChinese ? u8"过热时长" : "Timeout");
                        SameLine();
                        SliderFloat(u8"##OverheatTime", &config.OverheatTime, 0.1f, 30, config.UseChinese ? u8"%.2f秒" : "%.2fs", ImGuiSliderFlags_AlwaysClamp);

                        Checkbox(config.UseChinese ? u8"播放提示音" : "Play Sound", &config.UseOHPPlaySound);
                        Checkbox(config.UseChinese ? u8"立即重置计时器" : "Immediately reset the timer", &config.UseOHPResetTimerOnStop);
                        AttachTooltip(config.UseChinese ? u8"停止连点后立即重置计时器，\n而不是等等计时器的时间慢慢流失直到0。" : "Immediately reset the timer when stopping the clicker, \ninstead of waiting for the timer to slowly lose time until 0.");

                        Text(config.UseChinese ? u8"时间流失速度" : "Time Loss Speed");
                        AttachTooltip(config.UseChinese ? u8"数值越大，计时器时间流失越快。" : "The higher the value, the faster the timer loses time.");
                        SameLine();
                        SetNextItemWidth(120);
                        SliderFloat(u8"##OHPTimerTimeLossSpeed", &config.OHPTimerTimeLossSpeed, 0.05f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

                        Text(config.UseChinese ? u8"计时器值：%.3f" : "Timer Value: %.3f", OverheatProtecTimer);
                    }
                    EndDisabled();
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
                        Checkbox(config.UseChinese ? u8"破坏方块" : "Break Blocks", &config.UseBreakBlocksWithClicker);
                        AttachTooltip(config.UseChinese ? u8"开启后，你可以破坏方块。\n&d该功能与 按下间隔 冲突\n&d启用后将会禁用冲突功能。" : "If you turn on this option, you can use items.\n&dThis feature conflicts with the interval between clicks, \n&dand will disable the conflicting function.");

                        SameLine();

                        Checkbox(config.UseChinese ? u8"使用物品" : "Use Items", &config.UseItemWithClicker);
                        AttachTooltip(config.UseChinese ? u8"开启后，你可以破坏方块。\n&d该功能与 按下间隔 冲突\n&d启用后将会禁用冲突功能。" : "If you turn on this option, you can use items.\n&dThis feature conflicts with the interval between clicks, \n&dand will disable the conflicting function.");

                        if (config.UseBreakBlocksWithClicker)
                            Text(config.UseChinese ? u8"&e注意：开启 破坏方块 可能出现卡刀的情况，请自行衡量！" : "&eNote: Enabling \"Break blocks\" may cause attack lag. Use at your own risk!");
                    }

                    //-------------------------过滤单点--------------------------
                    if (config.RClickerMode & C_MOUSESMODE)
                    {
                        Checkbox(config.UseChinese ? u8"过滤右键单点" : "Filter RMB Single Clicks", &config.UseFilterRSingleClick);
                        AttachTooltip(config.UseChinese ? u8"开启后，你普通的点击将&c不会触发连点" : "If you turn on this option, normal clicks will &cnot trigger the clicker.");
                    }

                    if (config.LClickerMode & C_MOUSESMODE)
                    {
                        if (config.RClickerMode & C_MOUSESMODE)
                            SameLine();
                        Checkbox(config.UseChinese ? u8"过滤左键单点" : "Filter LMB Single Clicks", &config.UseFilterLSingleClick);
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
                            TextColored((ImVec4)ImColor(80, 80, 80, 255), config.UseChinese ? u8"右键连点CPS预计为：%dCPS" : "The connected CPS is approximately:% dCPS", PredictingCPS);
                            AttachTooltip(config.UseChinese ? u8"&c本数据仅为预测数据，仅供参考\n&c实际CPS受用户点击的CPS影响。" : "&cThis data is only for predictive purposes and is for reference only. \n&cActual CPS is affected by the CPS clicked by the user.");
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
                            TextColored((ImVec4)ImColor(80, 80, 80, 255), config.UseChinese ? u8"左键连点CPS预计为：%dCPS" : "The connected CPS is approximately:% dCPS", PredictingCPS);
                            AttachTooltip(config.UseChinese ? u8"&c本数据仅为预测数据，仅供参考\n&c实际CPS受用户点击的CPS影响。" : "&cThis data is only for predictive purposes and is for reference only. \n&cActual CPS is affected by the CPS clicked by the user.");
                        }

                        Text(config.UseChinese ? u8"触发阀门值" : "Trigger Threshold");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##AssistModeCPSThreshold", &config.AssistModeCPSThreshold, 1, 16, u8"%dCPS", ImGuiSliderFlags_AlwaysClamp);
                        AttachTooltip(config.UseChinese ? u8"当你点击的CPS达到预定值时，才会触发辅助点击\n&6该功能可有效降低误判。" : "When the CPS reaches the prescribed value, The assist click will be triggered, \n&6which can effectively reduce the misjudgment.");
                    }
                    if (!config.UseConciseMode && (config.RClickerMode & C_MOUSESMODE || config.LClickerMode & C_MOUSESMODE))
                    {
                        SetCursorPosY(GetCursorPosY() + 10);
                        Checkbox(config.UseChinese ? u8"自动重置鼠标状态" : "Auto Reset Mouse Status", &config.UseAutoResetOnDisable);
                        AttachTooltip(config.UseChinese ? u8"当你设置为鼠标模式时，并且在使用过程中被连点控制禁用了连点\n此时可能会发现你明明没有按下鼠标，\n但是游戏却表现出按下鼠标的状态。\n&6此功能可以消除这种问题，\n但是可能对于手动禁用然后挖方块的人造成挖掘状态断开。" : "When you set the clicker to mouse mode \nand disable the clicker control during use\nYou may find that you are not pressing the mouse, \nbut the game is showing the state of pressing the mouse. \n&6This feature can eliminate this problem,\nBut it may cause mining status disconnection for \nthose who manually disable and mine blocks.");
                    }
                }
                EndCard();

                //-------------------------物品栏开关----------------------------
                //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"物品栏开关" : "Itembar Switch", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    Checkbox(config.UseChinese ? u8"物品栏开关" : "Itembar Switch", &config.UseItembarSwitch);
                    AttachTooltip(config.UseChinese ? u8"将对应物品格子的按钮关闭，连点器就会在你手拿对应物品时关闭连点。\n其中的快捷键必须设置，主要用于判断物品栏位置！" : "Turn off the button corresponding to the corresponding item slot\nin the item bar when you hold the corresponding item. \nThe clicker will close the click when you hold the corresponding item.\nThe shortcut key must be set, mainly used to \ndetermine the position of the item bar!");

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
                        for (unsigned int j = 0; j < 2; j++)
                        {
                            PushID(j);
                            const char* text = config.UseChinese ? (j == 0 ? u8"左键设置" : u8"右键设置") : (j == 0 ? "LMB Settings" : "RMB Settings");
                            Text(text);
                            SameLine();
                            DrawSpiltLine(528 - CalcTextSize(text).x, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                            const ImVec2 pos1 = GetCursorPos();
                            Image((ImTextureID)image_itembar, ImVec2(540, 66));
                            const ImVec2 pos2 = GetCursorPos();

                            ImVec2 MainPos(GetCursorPosX() + ((60 - 40) / 2.0f) + 3, GetCursorPosY() - 55);
                            SetCursorPos(MainPos);

                            ImDrawList* draw_list = GetWindowDrawList();
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
                                    SetCursorPos({ PosX - 3, pos1.y });
                                    ImageRounded((ImTextureID)image_hotbar, ImVec2(66, 66), 6.0f);
                                    SetCursorPos(pos2);
                                }

                                // pos.y的值是格子的顶部Y坐标
                                const ImVec2 BoxPos(PosX + 10, pos1.y + 42);
                                const ImVec2 BoxSize(46, 4);
                                ImRect BoxRect(BoxPos, ImVec2(BoxPos.x + 40, BoxPos.y + 16));
                                BoxRect.Translate(GetWindowPos());

                                const ImU32 color = GetColorU32(IsEnableClicker[i] ? GuiColor[ImGuiCol_CheckMark] : GuiColor[ImGuiCol_ScrollbarGrabActive]);
                                draw_list->AddRectFilled(BoxRect.Min, BoxRect.Max, color, 2.0f);
                                SameLine(0, 20);
                                PopID();
                            }
                            // 和下面注释同理,不过要微调
                            SetCursorPos(ImVec2(pos1.x + 540 + 2, (j == 1) ? pos1.y - 25 + 4 : pos1.y));

                            // +25是因为中间有50px的高度空间，50/2=25
                            BeginChild("##ItembarChildIn", ImVec2(130, 66 + 35), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                            {
                                Checkbox(config.UseChinese ? u8"启用连点" : "Enable Clicker", &IsEnableClicker[ItemBarPos]);
                                Checkbox(config.UseChinese ? u8"CPS切换" : "CPS Switch", &IsEnablePresetCPS[ItemBarPos]);

                                BeginDisabled(!IsEnablePresetCPS[ItemBarPos] || config.UseBrutalMode);
                                {
                                    Text("CPS");
                                    SameLine();
                                    SetNextItemWidth(80);
                                    SliderInt("##ib_cpsset", &PresetCPS[ItemBarPos], MIN_CPS, MAX_CPS, "%dCPS", ImGuiSliderFlags_AlwaysClamp);
                                }
                                EndDisabled();
                            }
                            EndChild();

                            if (j == 0)
                            {
                                SetCursorPos(pos2);
                                const char* text = config.UseChinese ? u8"物品栏快捷键设置" : "Itembar Shortcut Key Settings";
                                Text(text);
                                AttachTooltip(config.UseChinese ? u8"&c请确保快捷键和游戏中的一致！\n&6否则将无法判断物品栏切换" : "&cPlease make sure the shortcut key is consistent with the game!\n&6Otherwise, it cannot determine the position of the item bar switch!");
                                SameLine();
                                DrawSpiltLine(528 + CalcTextSize(text).x, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                                SetCursorPos(ImVec2(pos2.x, pos2.y + (50 / 2.0f) - 3));
                                for (unsigned int i = 0; i < 9; i++)
                                {
                                    PushID(i);
                                    SetNextItemWidth(40);
                                    InputKey(u8"##item", &config.ItemEnableClickerKey.at(i), config.UseChinese ? u8"请按下按键" : "Press a key");
                                    if (i != 8)
                                        SameLine(0, 21);
                                    PopID();
                                }
                                // 中间露出50px高度的空间
                                SetCursorPos(ImVec2(pos2.x, pos2.y + 50));
                            }
                            PopID();
                        }
                    }
                    EndChild();
                    PopStyleVar(); //ImGuiStyleVar_WindowPadding
                }
                EndCard();

                //-------------------------获取游戏窗口----------------------------
                //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                //SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"游戏窗口" : "Game window", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    static string InfoText = config.UseChinese ? u8"未读取" : "Unread";
                    static ImVec4 color;
                    static bool Starting = false;
                    static string GameWindowTitle = config.UseChinese ? u8"未知" : "Unknow";
                    BeginDisabled(Starting);
                    {
                        if (Button(config.UseChinese ? u8"获取" : "Choice", ImVec2(100, 40)))
                        {
                            GettingGameState = -1;
                            Starting = true;

                            thread GetHwndThread([]()
                                {
                                    PrintLog("[Render]等待用户选择游戏窗口...", PL_INFO);
                                    while (!GCExit)
                                    {
                                        HWND FoundHwnd = GetForegroundWindow();
                                        HWND DesktopHwnd = GetDesktopWindow();
                                        if (FoundHwnd != GCHWND && FoundHwnd != DesktopHwnd)
                                        {
                                            GameHwnd = GetForegroundWindow();
                                            GettingGameState = 0;
                                            return 0;
                                        }
                                        if (FoundHwnd == 0)
                                        {
                                            GettingGameState = 1;
                                            return 0;
                                        }
                                        Sleep(200);
                                    }
                                    return 0;
                                });
                            GetHwndThread.detach();
                        }
                    }
                    EndDisabled();

                    if (GettingGameState == 0)
                    {
                        InfoText = config.UseChinese ? u8"获取成功" : "Get success";
                        color = (ImVec4)ImColor(149, 219, 125, 255);
                        Starting = false;

                        char buf[MAX_BUF];
                        if (GetWindowText(GameHwnd, buf, MAX_BUF))
                            GameWindowTitle = StringToUTF8(buf);
                        else
                            GameWindowTitle = u8"获取失败";

                        GettingGameState = -1;
                    }
                    else if (GettingGameState == 1)
                    {
                        InfoText = config.UseChinese ? u8"获取失败" : "Get failed";
                        color = (ImVec4)ImColor(252, 62, 54, 255);
                        Starting = false;
                        GettingGameState = -1;
                        GameHwnd = 0;
                    }
                    else if (Starting)
                    {
                        InfoText = config.UseChinese ? u8"正在获取中，请点击游戏窗口。" : "Getting, please click the game window.";
                        color = (ImVec4)ImColor(255, 214, 143, 255);
                    }

                    if (GameGuiError)
                    {
                        InfoText = config.UseChinese ? u8"无法在该窗口创建GUI" : "Cannot create GUI in this window";
                        color = (ImVec4)ImColor(252, 62, 54, 255);
                    }

                    TextColored(color, InfoText.c_str());
                    Text(config.UseChinese ? u8"窗口名称：%s" : "Window Name: %s", GameWindowTitle.c_str());
                }
                EndCard();

                EndTabItem();
            }

            //-------------------------高级设置--------------------------
            if (BeginTabItem(config.UseChinese ? u8"高级设置" : "Advanced Setting"))
            {
                //-------------------------CPS波动----------------------------
                BeginDisabled(config.UseBrutalMode);
                BeginCard("##CPSChangeTrendCard", ImVec2(334, 237));
                {
                    Text(config.UseChinese ? u8"这里什么都没有\n正在制作新功能" : "Nothing here\nNew function is being developed");
                }
                EndCard();
                EndDisabled();

                SameLine();

                //-------------------------空刀概率----------------------------
                BeginDisabled(config.UseBrutalMode);
                BeginCard("##EmptyRateCard", ImVec2(334, 237));
                {
                    Checkbox(config.UseChinese ? u8"右键空刀概率" : "RClicker Empty Rate", &config.UseREmptyRate);

                    BeginDisabled(!config.UseREmptyRate);
                    {
                        Text(config.UseChinese ? u8"右键空刀概率" : "RClicker Empty Rate");
                        SameLine();
                        SetNextItemWidth(200);
                        SliderInt(u8"##UseREmptyRate", &config.REmptyRateValue, 1, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp);
                    }
                    EndDisabled();

                    Checkbox(config.UseChinese ? u8"左键空刀概率" : "LClicker Empty Rate", &config.UseLEmptyRate);
                    
                    BeginDisabled(!config.UseLEmptyRate);
                    {
                        Text(config.UseChinese ? u8"左键空刀概率" : "LClicker Empty Rate");
                        SameLine();
                        SetNextItemWidth(200);
                        SliderInt(u8"##UseLEmptyRate", &config.LEmptyRateValue, 1, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp);
                    }
                    EndDisabled();

                    BeginDisabled(!config.UseLEmptyRate && !config.UseREmptyRate);
                    if (!config.UseConciseMode)
                    {
                        Text(config.UseChinese ? u8"随机生成算法" : "Random Algorithm");
                        SetNextItemWidth(200);
                        SameLine();
                        if (BeginCombo("##erandomgen", RandomGenList[config.EmptyRate_RandomGen]))
                        {
                            for (int n = 0; n < 8; n++)
                            {
                                PushID(n);
                                const bool is_selected = (config.EmptyRate_RandomGen == n);
                                if (Selectable(RandomGenList[n], is_selected))
                                    config.EmptyRate_RandomGen = n;
                                if (is_selected)
                                    SetItemDefaultFocus();
                                PopID();
                            }
                            EndCombo();
                        }

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

                            Text(config.UseChinese ? u8"数据点数量" : "Number of data points");
                            SameLine();
                            SetNextItemWidth(300);
                            SetCursorPosX(300);
                            SliderInt(u8"##rgc_points", &PointsSize, 10, 2000, "%d", ImGuiSliderFlags_AlwaysClamp);

                            if (VarHasChanged(config.UseLEmptyRate, "UseLEmptyRate") ||
                                VarHasChanged(config.UseREmptyRate, "UseREmptyRate") ||
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
                                        ClickDatas.clear();
                                        EmpytDatas.clear();
                                        GenDurationSum = 0;
                                        int ClickCount = 0;
                                        const float Proportion = config.REmptyRateValue / 100.0f;

                                        for (int i = 0; i < PointsSize; i++)
                                        {
                                            auto start = chrono::high_resolution_clock::now();

                                            const bool data = RandomBool(Proportion, config.EmptyRate_RandomGen, config.EmptyRate_RandomDist);
                                            ClickDatas.push_back(data);
                                            EmpytDatas.push_back(!data);

                                            auto end = chrono::high_resolution_clock::now();
                                            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                                            GenDurationSum += duration.count() / 1000.0;
                                            if (ClickDatas.back())
                                                ClickCount++;
                                        }
                                        auv = GenDurationSum / PointsSize;
                                        ActualRate = (double)ClickCount / PointsSize * 100;
                                        IsGenerating = false;
                                    });
                                FunThread.detach();
                                IsInit = true;
                            }

                            if (ImPlot::BeginPlot("##Random_curves1", ImVec2(-1, -1), ImPlotFlags_CanvasOnly))
                            {
                                ImPlot::SetupAxesLimits(0, PointsSize, -2, 2, ImGuiCond_Always);

                                ImPlot::SetNextMarkerStyle(-1, 1.5f, (ImVec4)ImColor(0, 0, 0, 0));
                                ImPlot::SetNextLineStyle((ImVec4)ImColor(254, 189, 57, 200), 0);
                                ImPlot::PlotBars(config.UseChinese ? u8"已点击" : "Clicked", ClickDatas.data(), ClickDatas.size(), 0.8);

                                ImPlot::SetNextMarkerStyle(-1, 1.5f, (ImVec4)ImColor(0, 0, 0, 0));
                                ImPlot::SetNextLineStyle((ImVec4)ImColor(0, 106, 201, 200), 0);
                                ImPlot::PlotBars(config.UseChinese ? u8"空点击" : "Empty Click", EmpytDatas.data(), EmpytDatas.size(), 0.8);
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
                EndCard();
                EndDisabled();

                SetCursorPosY(GetCursorPosY() + 10.0f);
                //-------------------------主循环延迟----------------------------
                if (!config.UseConciseMode)
                {
                    BeginCard("##OtherNBCard", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    {
                        Text(config.UseChinese ? u8"主循环延迟" : "Loop Delay");
                        AttachTooltip(config.UseChinese ? u8"每次轮询间隔，单位毫秒，\n太高会导致有延迟(设置为1ms就几乎没有延迟)\n通常不需要设置，在GC占用率大时，调高该数值。\n不会就保持默认。" : "The interval between each polling, in milliseconds, \nToo high will cause delay (set to 1ms almost no delay)\nUsually do not need to set, increase this value when the GC occupancy is high. \nIt will not stay at the default value.");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        SliderInt(u8"##MainClickSleep", &config.MainClickSleep, 0, 50, config.UseChinese ? u8"%d毫秒" : "%d ms", ImGuiSliderFlags_AlwaysClamp);
                    }
                    EndCard();
                }

                //-------------------------点击方式----------------------------
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard("##ClickMethodAndOther", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    BeginDisabled(config.UseBEAdaptation);
                    {
                        Text(config.UseChinese ? u8"点击方式" : "Click Method");
                        SameLine();
                        SetNextItemWidth(300);
                        SetCursorPosX(300);
                        unordered_map<int, const char*> ClickMethodList = {
                            { SC_MOUSE_EVENT,  config.UseChinese ? u8"Mouse Event - 模拟鼠标消息点击"  : "Mouse Event - Simulate mouse message click" },
                            { SC_POST_MESSAGE, config.UseChinese ? u8"Post Message - 使用消息队列点击" : "Post Message - Use message queue click"     },
                            { SC_SEND_INPUT,   config.UseChinese ? u8"Send Input - 使用模拟鼠标输入"   : "Send Input - Use simulated mouse input"     }
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

                    Checkbox(config.UseChinese ? u8"仅人类操作触发" : "Human Only Trigger", &config.UseHumanOnlyTrigger);
                }
                EndCard();

                //-------------------------连点间隙----------------------------
                if (!config.UseMinimalistMode)
                {
                    SetCursorPosY(GetCursorPosY() + 10.0f);
                    BeginCard("##ClickerIntrval", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    {
                        //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                        BeginDisabled(config.RClickerMode & C_ASSISTMODE);
                        {
                            Checkbox(config.UseChinese ? u8"右键连点间隙" : "RClicker Interval", &config.UseRClickTime);

                            BeginDisabled(!config.UseRClickTime);
                            {
                                Text(config.UseChinese ? u8"右键点击时长" : "RClicker Time");
                                AttachTooltip(config.UseChinese ? u8"连点的时长，单位毫秒" : "The duration of the click in milliseconds");
                                SameLine();
                                SetNextItemWidth(200);
                                InputInt("##RCTime", &config.RCTime, 10, 10000);

                                SameLine();

                                Text(config.UseChinese ? u8"右键停止时长" : "RClicker Stop Time");
                                AttachTooltip(config.UseChinese ? u8"连点停止的时长，单位毫秒" : "The duration of the stop in milliseconds");
                                SameLine();
                                SetNextItemWidth(200);
                                InputInt("##RCSleep", &config.RCSleep, 10, 10000);
                            }
                            EndDisabled();
                        }
                        EndDisabled();

                        BeginDisabled(config.LClickerMode & C_ASSISTMODE);
                        {
                            Checkbox(config.UseChinese ? u8"左键连点间隙" : "UseLClickTime", &config.UseLClickTime);

                            BeginDisabled(!config.UseLClickTime);
                            {
                                Text(config.UseChinese ? u8"左键点击时长" : "LClick Time");
                                AttachTooltip(config.UseChinese ? u8"连点的时长，单位毫秒" : "The duration of the click in milliseconds");
                                SameLine();
                                SetNextItemWidth(200);
                                InputInt("##LCTime", &config.LCTime, 10, 10000);

                                SameLine();

                                Text(config.UseChinese ? u8"左键停止时长" : "LClick Stop Time");
                                AttachTooltip(config.UseChinese ? u8"连点停止的时长，单位毫秒" : "The duration of the stop in milliseconds");
                                SameLine();
                                SetNextItemWidth(200);
                                InputInt("##LCSleep", &config.LCSleep, 10, 10000);
                            }
                            EndDisabled();
                        }
                        EndDisabled();

                        if (config.LClickerMode & C_ASSISTMODE || config.RClickerMode & C_ASSISTMODE)
                            Text(config.UseChinese ? u8"&c辅助模式不支持该功能" : "&cThe AssistMode does not support this function.");
                    }
                    EndCard();
                }

                //-------------------------延迟启动--------------------------
                //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                SetCursorPosY(GetCursorPosY() + 10.0f);
                BeginCard(config.UseChinese ? u8"右键延迟触发" : "RClicker Delay Trigger", ImVec2(335, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    BeginDisabled(config.RClickerMode & C_ASSISTMODE);
                    {
                        Checkbox(config.UseChinese ? u8"启用" : "Enable", &config.UseRStartClickerDelay);
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
                    EndDisabled();
                }
                EndCard();

                SameLine();

                BeginCard(config.UseChinese ? u8"左键延迟触发" : "LClicker Delay Trigger", ImVec2(335, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    BeginDisabled(config.LClickerMode & C_ASSISTMODE);
                    {
                        Checkbox(config.UseChinese ? u8"启用" : "Enable", &config.UseLStartClickerDelay);
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
                    EndDisabled();
                    if (config.RClickerMode & C_ASSISTMODE || config.LClickerMode & C_ASSISTMODE)
                        Text(config.UseChinese ? u8"&c辅助模式不支持该功能" : "&cThe AssistMode does not support this function.");
                }
                EndCard();

                //-------------------------按下间隔--------------------------
                if (!config.UseConciseMode)
                {
                    //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                    SetCursorPosY(GetCursorPosY() + 10.0f);
                    BeginCard(config.UseChinese ? u8"左键按下间隔" : "LB Press Interval", ImVec2(335, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    {
                        Checkbox(config.UseChinese ? u8"启用" : "Enable", &config.UseRPressInterval);
                        AttachTooltip(config.UseChinese ? u8"在每次点击之中，按下按键的间隔，单位毫秒" : "The interval between each press of the right button, in milliseconds");

                        BeginDisabled(!config.UseRPressInterval);
                        {
                            Checkbox(config.UseChinese ? u8"动态设置按下时长&7(建议)" : "Dynamically set press duration &7(recommended)", &config.UseRAutoPressDuration);
                            AttachTooltip(config.UseChinese ? u8"动态的设置按下时长\n来模拟人类点击的的按下间隔\n&a除非特殊用途，否则建议开启。" : "Dynamically set the press duration to \nsimulate the press interval of a human click.\n&aIt is recommended to turn on unless there is a special purpose.");
                        }
                        EndDisabled();

                        BeginDisabled(!config.UseRPressInterval || config.UseRAutoPressDuration);
                        {
                            Text(config.UseChinese ? u8"按下间隔时长" : "Press Interval Time");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##PressDuration", &config.RPressDuration, 1, 500, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();
                    }
                    EndCard();
                    
                    SameLine();

                    BeginCard(config.UseChinese ? u8"右键按下间隔" : "RB Press Interval", ImVec2(335, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    {
                        Checkbox(config.UseChinese ? u8"启用" : "Enable", &config.UseLPressInterval);
                        AttachTooltip(config.UseChinese ? u8"在每次点击之中，按下按键的间隔，单位毫秒" : "The interval between each press of the right button, in milliseconds");

                        BeginDisabled(!config.UseLPressInterval);
                        {
                            Checkbox(config.UseChinese ? u8"动态设置按下时长&7(建议)" : "Dynamically set press duration &7(recommended)", &config.UseLAutoPressDuration);
                            AttachTooltip(config.UseChinese ? u8"动态的设置按下时长\n来模拟人类点击的的按下间隔\n&a除非特殊用途，否则建议开启。" : "Dynamically set the press duration to \nsimulate the press interval of a human click.\n&aIt is recommended to turn on unless there is a special purpose.");
                        }
                        EndDisabled();

                        BeginDisabled(!config.UseLPressInterval || config.UseLAutoPressDuration);
                        {
                            Text(config.UseChinese ? u8"按下间隔时长" : "Press Interval Time");
                            SameLine();
                            SetNextItemWidth(220);
                            SliderInt(u8"##PressDuration", &config.LPressDuration, 1, 500, u8"%d毫秒", ImGuiSliderFlags_AlwaysClamp);
                        }
                        EndDisabled();
                    }
                    EndCard();
                }

                //-------------------------鼠标抖动--------------------------

                if (!config.UseMinimalistMode)
                {
                    //DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                    SetCursorPosY(GetCursorPosY() + 10.0f);
                    BeginCard(config.UseChinese ? u8"鼠标抖动" : "Mouse Shake", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    {
                        Checkbox(config.UseChinese ? u8"启用" : "Enable", &config.UseMouseShake);
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

                            Checkbox(config.UseChinese ? u8"非线性移动" : "Non-linear Move", &config.UseMSSmoothSpeed);
                            AttachTooltip(config.UseChinese ? u8"使用非线性函数，来实现缓冲移动的效果。" : "Use a non-linear function to \nachieve the effect of buffer movement.");
                            SameLine();

                            Checkbox(config.UseChinese ? u8"原始输入" : "Raw Input", &config.UseMouseShakeRawInput);
                            AttachTooltip(config.UseChinese ? u8"模拟鼠标原始输入，防止系统对鼠标加速度\n导致移动距离不精确。" : "Simulate the raw mouse input to prevent the system from \naccurately measuring the mouse acceleration.");

                            Checkbox(config.UseChinese ? u8"防止视角偏离" : "Prevent View Sliding", &config.UseMSPreventViewSliding);
                            AttachTooltip(config.UseChinese ? u8"使用算法实现在鼠标抖动时不会出现视角偏离原先位置。" : "Use an algorithm to prevent the view from sliding during mouse shake.");

                            if (!config.UseConciseMode)
                            {
                                Text(config.UseChinese ? u8"随机生成算法" : "Random Algorithm");
                                SameLine();
                                SetNextItemWidth(300);
                                SetCursorPosX(300);
                                if (BeginCombo("##mrandomgen", RandomGenList[config.MouseShake_RandomGen]))
                                {
                                    for (int n = 0; n < 8; n++)
                                    {
                                        const bool is_selected = (config.MouseShake_RandomGen == n);
                                        if (Selectable((RandomGenList[n] + "##"s + to_string(n)).c_str(), is_selected))
                                            config.MouseShake_RandomGen = n;
                                        if (is_selected)
                                            SetItemDefaultFocus();
                                    }
                                    EndCombo();
                                }

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

                                    Text(config.UseChinese ? u8"数据点数量" : "Number of data points");
                                    SameLine();
                                    SetNextItemWidth(300);
                                    SetCursorPosX(300);
                                    SliderInt(u8"##ms_points2", &PointsSize, 10, 2000, "%d", ImGuiSliderFlags_AlwaysClamp);

                                    if (VarHasChanged(config.MouseShake_RandomDist, "MouseShake_RandomDist") ||
                                        VarHasChanged(config.MouseShake_RandomGen, "MouseShake_RandomGen") ||
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
                                                Points_x.clear();
                                                Points_y.clear();
                                                Points_z.clear();
                                                GenDurationSum = 0;
                                                POINT view{};

                                                for (int i = 0; i < PointsSize; i++)
                                                {
                                                    auto start = chrono::high_resolution_clock::now();

                                                    POINT pt = UseMouseShakeAlgorithm(view, { 0, 0 });
                                                    Points_x.push_back(pt.x);
                                                    Points_y.push_back(pt.y);
                                                    Points_z.push_back(i);

                                                    auto end = chrono::high_resolution_clock::now();
                                                    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
                                                    GenDurationSum += duration.count() / 1000.0;
                                                }
                                                auv = GenDurationSum / PointsSize;
                                                IsGenerating = false;
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
                                        ImPlot3D::PlotLine(config.UseChinese ? u8"鼠标位置" : "Mouse position", Points_x.data(), Points_z.data(), Points_y.data(), Points_z.size());

                                        ImPlot3D::EndPlot();
                                    }

                                    SameLine();

                                    if (ImPlot::BeginPlot("##RandomPlot2D", ImVec2(250, 250), ImPlotFlags_CanvasOnly))
                                    {
                                        ImPlot::SetNextMarkerStyle(-1, 1.5f, (ImVec4)ImColor(184, 30, 1, 255));
                                        ImPlot::SetNextLineStyle((ImVec4)ImColor(30, 156, 214, 255), 2);
                                        ImPlot::SetupAxesLimits(-20, 20, -20, 20, ImGuiCond_Always);

                                        ImPlot::PlotLine(config.UseChinese ? u8"鼠标位置" : "Mouse position", Points_x.data(), Points_y.data(), Points_z.size());
                                        if (PointsTime >= 0 && PointsTime <= Points_z.size())
                                            ImPlot::PlotScatter("##PlotPoints", &Points_y[PointsSize], &Points_x[PointsSize], 1);

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
                        }
                        EndDisabled();
                    }
                    EndCard();
                }

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
            if (BeginTabItem(config.UseChinese ? u8"点击声音" : "Click Sound"))
            {
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
                    Checkbox(config.UseChinese ? u8"启用鼠标声音" : "Enable Mouse Sound", &config.UseMouseSound);
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
                    Checkbox(config.UseChinese ? u8"启用键盘声音" : "Enable Keybd Sound", &config.UseKeybdSound);
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
                                RenderMessageBox(u8"在打开的网页列表中，找到想要的音色\n点击左边的“Learn More”，然后点击“Download”就可以下载了。\n&6如何导入？\n&f1.打开文件夹&5D:\\GameClicker\\sound\\keys&f\n2.新建一个文件夹（任意名字）\n3.打开新建的文件夹，将压缩包里面的文件\n解压到当前文件夹。\n4.重启GC连点器。", u8"教程", MB_OK, false);
                            }
                        }
                        EndDisabled();

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
                            auto dlth = []() {
                                RenderTip(config.UseChinese ? u8"正在重新加载声音..." : "Reloading sounds...", 4.0f, PT_INFO);
                                LoadSounds();
                                RenderTip(config.UseChinese ? u8"重新加载成功！" : "Reloading success!", 3.0f, PT_INFO);
                                };
                            thread th(dlth);
                            th.detach();
                        }
                    }
                    EndDisabled();
                }
                EndCard();

                EndTabItem();
            }

            //-------------------------连点控制--------------------------
            if (BeginTabItem(config.UseChinese ? u8"连点控制" : "Click Control"))
            {
                //-----------------------自动开关连点---------------------------
                /*
                DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);
                TabsText(config.UseChinese ? u8"自动开关连点" : "Auto click switch");

                TextColored((ImVec4)ImColor(255, 214, 143, 255), config.UseChinese ? u8"提示：使用内存搜索可以实现0误\n但是重新进入 存档/服务器 需要重新搜索，有点麻烦。\n如果不开启内存搜索将使用按键判断方法来判断，误差可能会比较大。" : "Note: Using memory search can achieve 0 error\nBut you need to search again when you enter the save/server, which is a bit troublesome.\nIf you do not turn on memory search, the key judgment method will be used to determine, \nand the error may be a bit larger.");

                Checkbox(config.UseChinese ? u8"使用内存搜索" : "Use memory search", &UseItembarUseMemSearch);
                AttachTooltip(config.UseChinese ? u8"在游戏窗口的内存中搜索物品栏的位置信息，\n实现0误差判断物品栏位置。" : "Search the position information of the item bar in the game window memory\nto achieve 0 error judgment of the item bar position.");

                BeginDisabled(!UseItembarUseMemSearch);
                {
                    if (GameHwnd == NULL)
                        TextColored((ImVec4)ImColor(255, 214, 143, 255), config.UseChinese ? u8"请先去 基本设置 里面获取游戏窗口！" : "Please go to the BasicSetting to get the game window!");

                    static bool WillStart = false;
                    BeginDisabled(IsMemSearch || GameHwnd == NULL);
                    {
                        if (Button(config.UseChinese ? u8"开始内存搜索" : "Start mem search", ImVec2(100, 40)))
                            WillStart = true;
                    }
                    EndDisabled();

                    if (WillStart)
                    {
                        int ret = MessageBox(config.UseChinese ? u8"请注意！如果要开始搜索，请注意以下几点：\n    1.在搜索过程中请勿乱动，保持在游戏里面就行，也不要打开背包等UI。\n    2.确保物品栏开关的快捷键都设置正确，否则GC无法切换物品栏。\n是否继续？" : "Please note that if you want to start the search, please pay attention to the following points: \n    1. Do not move during the search, just stay in the game. \n    2. Make sure that the shortcut keys for the item bar switch are set correctly, otherwise GC cannot switch the item bar. \nDo you want to continue?",
                            config.UseChinese ? u8"提示" : "Reminder", MB_OKCANCEL);
                        if (ret == IDOK)
                        {
                            IsMemSearch = true;
                            WillStart = false;
                        }
                    }

                    ImVec4 color;
                    string str;

                    switch (IsSearchState)
                    {
                        case MS_ERROR:
                            color = (ImVec4)ImColor(252, 62, 54, 255);
                            str = config.UseChinese ? u8"内存搜索失败" : "Memory search failed";
                            break;
                        case MS_SEARCHING:
                            color = (ImVec4)ImColor(255, 214, 143, 255);
                            str = config.UseChinese ?
                                (u8"已开始内存搜索的第 " + to_string(MemSearchCount) + u8" 次"s) :
                                ("Memory search at ") + to_string(MemSearchCount) + " count";
                            break;
                        case MS_SUCCESS:
                            color = (ImVec4)ImColor(149, 219, 125, 255);
                            str = config.UseChinese ? u8"内存搜索成功！" : "Memory search success!";
                            break;
                    }

                    TextColored(color, (u8"内存搜索状态：" + str).c_str());

                    Text(config.UseChinese ? u8"内存搜索的延迟" : "Memory search delay");
                    SameLine();
                    SetNextItemWidth(300);
                    SetCursorPosX(300);
                    SliderInt(u8"##MemSearchSleep", &MemSearchSleep, 1, 500, "%d", ImGuiSliderFlags_AlwaysClamp);
                    AttachTooltip(config.UseChinese ? u8"延迟越低，性能开销越大，判断的速度越快。" : "The lower the delay the higher the performance \noverhead and the faster the judgment speed.");
                }
                EndDisabled();

                InvisibleButton("##PaddingButton1", ImVec2(1, 10));
                */

                //-------------------------GUI检测&其他--------------------------
                SetCursorPosY(GetCursorPosY() + 10.0f);

                BeginCard("##CCtrlSettings-1", ImVec2(235, 174), ImGuiChildFlags_Borders);
                {
                    Checkbox(config.UseChinese ? u8"GUI检测" : "GUI Detect", & config.UseGUIDetection);
                    AttachTooltip(config.UseChinese ? u8"&a开启该功能后\n&f打开GUI时，会自动关闭连点\n&d前提条件：&6开启允许控制连点开关" : "&aTurn on this function\n&fWhen the GUI is opened, the clicker will be automatically closed\n&dPrerequisite: &6Turn on the function of allowing control of clicker switch");
                }
                EndCard();

                SameLine();

                BeginCard("##CCtrlSettings-2", ImVec2(215, 174), ImGuiChildFlags_Borders);
                {
                    //-------------------------仅移动触发连点--------------------------

                    Checkbox(config.UseChinese ? u8"仅移动触发右键连点" : " Only Moving Trig RC", &config.UseROnlyMoveClicker);
                    AttachTooltip(config.UseChinese ? u8"开启后，连点器只会在你移动鼠标时触发连点。" : "Turn on this function to trigger the clicker only when you move the mouse.");

                    Checkbox(config.UseChinese ? u8"仅移动触发左键连点" : " Only Moving Trig LC", &config.UseLOnlyMoveClicker);
                    AttachTooltip(config.UseChinese ? u8"开启后，连点器只会在你移动鼠标时触发连点。" : "Turn on this function to trigger the clicker only when you move the mouse.");

                    //-------------------------仅跳跃触发连点--------------------------
                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    BeginDisabled(config.UseROnlyMoveClicker);
                    {
                        Checkbox(config.UseChinese ? u8"仅跳跃触发右键连点" : "Only Jump On RC", &config.UseROnlyJumpClicker);
                        AttachTooltip(config.UseChinese ? u8"开启后，只有你跳跃时才能连点。\n如果你已经开启了 &6仅移动触发右键连点 &f功能，则该功能无效。\n因为已经包含此功能。" : "After activation, you can only connect when jumping. \nIf you have turned on the function of &6[Only move to trigger right clicker]&f, this function is invalid. \nBecause it already contains this function.");
                    }
                    EndDisabled();

                    BeginDisabled(config.UseLOnlyMoveClicker);
                    {
                        Checkbox(config.UseChinese ? u8"仅跳跃触发左键连点" : " Only Jump On LC", &config.UseLOnlyJumpClicker);
                        AttachTooltip(config.UseChinese ? u8"开启后，只有你跳跃时才能连点。\n如果你已经开启了 &6仅移动触发右键连点 &f功能，则该功能无效。\n因为已经包含此功能。" : "After activation, you can only connect when jumping. \nIf you have turned on the function of &6[Only move to trigger right clicker]&f, this function is invalid. \nBecause it already contains this function.");
                    }
                    EndDisabled();
                }
                EndCard();

                SameLine();

                BeginCard("##CCtrlSettings-3", ImVec2(215, 174), ImGuiChildFlags_Borders);
                {
                    Checkbox(config.UseChinese ? u8"仅限游戏内连点" : "In-game clicker only", & config.UseInGameOnlyClicker);
                    if (GameHwnd == 0)
                        AttachTooltip(config.UseChinese ? u8"&e请先获取游戏窗口" : "&ePlease choose game window first");
                    else if (!config.AllowSetLClicker || config.AllowSetRClicker)
                        AttachTooltip(config.UseChinese ? u8"&e请打开 允许控制连点开关" : "&ePlease enable \"Allow Control Clicker\"");

                    Checkbox(config.UseChinese ? u8"停止移动时关闭连点" : "Stop Click On Stop Move", &config.UseDisableMoveOnStopMove);
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

                            static const Texture2D button_texture1 = LoadTextureFromFile(Bmp_Delete);
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

                        static const Texture2D image_texture2 = LoadTextureFromFile(Bmp_Add);
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
            
            if (BeginTabItem(config.UseChinese ? u8"个性化" : "Custom"))
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

                    Checkbox(config.UseChinese ? u8"自动调整图片大小" : "Auto Image Size", &BgAutoResize);

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
                                    const string file = SelectFile("图片文件\0*.png;*.bmp;*.jpg\0\0");
                                    CopyFile(file.c_str(), Bmp_Background.c_str(), false);
                                    PrintLog("[File] 复制文件 " + file + " 到 " + Bmp_Background, PL_INFO);
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

            //-------------------------信息显示--------------------------
            /*if (BeginTabItem(config.UseChinese ? u8"视觉效果" : "Visual effect"))
            {
                Text(u8"还没做好呢");
                //BeginDisabled(GameHwnd == NULL);
                BeginDisabled(true);
                Checkbox(config.UseChinese ? u8"启用信息显示" : "Enable visual effect", &UseEnableGameGui);

                EndDisabled();

                string str;
                ImVec4 color;
                switch (GameGuiState)
                {
                    case GAMEGUI_ERROR:
                        color = (ImVec4)ImColor(252, 62, 54, 255);
                        str = config.UseChinese ? u8"启动失败": "Fail to start";
                        break;
                    case GAMEGUI_ALREADY:
                        color = (ImVec4)ImColor(255, 214, 143, 255);
                        str = config.UseChinese ? u8"准备就绪" : "ready to go";
                        break;
                    case GAMEGUI_SUCCES:
                        color = (ImVec4)ImColor(149, 219, 125, 255);
                        str = config.UseChinese ? u8"启动成功" : "Successfully started";
                        break;
                }
                if (GameGuiState == GAMEGUI_ERROR)
                {
                    switch (GameGui_ErrorCode)
                    {
                        case 0:
                            str += config.UseChinese ? u8" - 无法创建进程" : " - Unable to create process";
                            break;
                        case -1:
                            str += config.UseChinese ? u8" - 已经启动了一个进程" : " - A process has already been started";
                            break;
                        case -2:
                            str += config.UseChinese ? u8" - 无法识别GC连点器进程" : " - Unable to recognize GaemClicker process";
                            break;
                        case -3:
                            str += config.UseChinese ? u8" - 启动参数错误" : " - Startup parameter error";
                            break;
                        case -4:
                            str += config.UseChinese ? u8" - 无法识别游戏窗口" : " - Unable to recognize game window";
                            break;
                    }
                }
                Text(config.UseChinese ? u8"状态：" : "state:");
                SameLine();
                TextColored(color, str.c_str());

                BeginDisabled(GameHwnd == NULL);
                {
                    static string GameTitle;
                    char buf[MAX_BUF];

                    if (GameTitle.empty())
                    {
                        try{
                            GameTitle = StringToUTF8(GameWindowTitle);
                        }catch (...){
                            GameTitle = GameWindowTitle;
                        }
                        try {
                            *buf = *StringToUTF8(buf).c_str();
                        }
                        catch (...)
                        {
                            *buf = config.UseChinese ? (char)u8"错误" : (char)"ERROR";
                        }
                    }

                    Text(config.UseChinese ? u8"修改游戏标题" : "Edit game title");
                    SameLine();
                    
                    SetNextItemWidth(800);
                    InputText("##ChangeTitle", buf, MAX_BUF);
                    try{
                        GameTitle = StringToUTF8(buf);
                    }catch (...){
                        GameTitle = buf;
                    }
                    try {
                        *buf = *StringToUTF8(buf).c_str();
                    }
                    catch (...)
                    {
                        *buf = config.UseChinese ? (char)u8"错误" : (char)"ERROR";
                    }
                    SameLine();
                    if(Button(config.UseChinese ? u8"确定" : "OK", ImVec2(50, 28)))
                    {
                        SetWindowText(GameHwnd, GameTitle.c_str());
                    }
                }
                EndDisabled();

                if (GameHwnd == NULL)
                    TextColored((ImVec4)ImColor(255, 214, 143, 255), config.UseChinese ? u8"请先去 基本设置 里面获取游戏窗口！" : "Please go to the Basic Settings to get the game window!");

                EndTabItem();
            }*/
            /*
            //-----------------------驱动保护---------------------------
            if (BeginTabItem(config.UseChinese ? u8"绕过检测" : "Driver protection"))
            {
                if (!config.UseChinese)
                    Text("This feature can only be used to bypass the detection of the Chinese version of Minecraft (NetEase)");

                static bool IsStartingDriver = false;
                static bool IsLoadDriver = false;
                static bool FirstStart = true;
                const static ImVec2 pos = GetCursorPos();

                const static bool RunAdmin = UseRunAdmin();

                if (!RunAdmin || FirstStart)
                {
                    if (!RunAdmin)
                        FirstStart = false;
                    const static ImVec2 size = GetWindowSize();
                    const static ImVec2 pos = { 102, 100 };
                    const static ImVec4 color(
                        GuiColor[ImGuiCol_WindowBg].x,
                        GuiColor[ImGuiCol_WindowBg].y,
                        GuiColor[ImGuiCol_WindowBg].z,
                        GuiColor[ImGuiCol_WindowBg].w * 0.7f
                    );
                    ImDrawList* draw_list = GetForegroundDrawList();

                    string str = FirstStart ?
                        (config.UseChinese ? u8"免责声明：对于部分电脑可能出现蓝屏的情况，但不会损坏您的电脑\n但如果第一次使用后蓝屏，后续请谨慎开启。\n因为本功能导致的损失开发者不负责。" : "Disclaimer: Some computers may cause a blue screen, but they will not damage your computer.\nHowever, if the first time you use it, please be cautious to turn it on. \nThe loss caused by this function is not the responsibility of the developer.") :
                        (config.UseChinese ? u8"请以管理员身份运行" : "Please run as administrator");
                    
                    ImVec2 textSize = CalcTextSize(str.c_str());
                    ImVec2 textPos = { (size.x - textSize.x) / 2, (size.y - textSize.y) / 2 - (FirstStart ? 15 : 0) };
                    draw_list->AddText(textPos, GetColorU32(GuiColor[ImGuiCol_Text]), str.c_str());

                    if (FirstStart)
                    {
                        SetCursorPos({ textPos.x + ((textSize.x - textPos.x) / 2) - 80, textPos.y + 40 });
                        if (Button(config.UseChinese ? u8"同意" : "Agree", ImVec2(80, 30)))
                            FirstStart = false;
                    }
                }

                SetCursorPos(pos);
                BeginDisabled(FirstStart);
                {
                    TextColored((ImVec4)ImColor(255, 214, 143, 255), config.UseChinese ? u8"注意：启动后，很可能无法关闭，不影响正常使用，但会被腾讯等大型游戏检测到作弊\n所以如果你不用GC了，请重启电脑来关闭绕过" : "Note: After starting, it is possible to not be able to close, but it will be detected by Tencent and other large games.\nSo if you do not use GC anymore, please restart your computer to close the bypass.");
                    Checkbox(config.UseChinese ? u8"隐藏GC进程(谨慎开启)" : "Hide GC process (caution)", &UseHiddenProcess);
                    AttachTooltip(config.UseChinese ? u8"开启后，GC进程将会隐藏，但是如果关闭GC就会蓝屏！" : "Turn on this function to hide the GC process and prevent it from crashing. \nHowever if you turn off the GC it will cause a blue screen!");

                    SameLine();

                    Checkbox(config.UseChinese ? u8"挂起网易进程" : "Suspend Netease process", &UseSuspendNetease);
                    AttachTooltip(config.UseChinese ? u8"开启后，网易进程会被挂起，此时你无法使用网易，等于将网易停止运行！" : "Turn on this function to suspend the Netease process and prevent you from using Netease. \nIt is equivalent to stopping Netease.");

                    BeginDisabled(IsStartingDriver);
                    {
                        if (Button(config.UseChinese ? u8"启动" : "Start", { 70, 40 }))
                        {
                            CreateThreadSafe(StartDriver, NULL, 0);
                            IsStartingDriver = true;
                        }
                    }
                    EndDisabled();

                    SameLine();

                    BeginDisabled(!IsStartingDriver);
                    {
                        if (Button(config.UseChinese ? u8"停止" : "Stop", { 70, 40 }))
                        {
                            CreateThreadSafe(UnLoadDriver, NULL, 0);
                            IsStartingDriver = false;
                        }
                    }
                    EndDisabled();

                    Text(u8"驱动状态:%s",
                        (DriverStatus == DRIVER_ERROR) ? (config.UseChinese ? u8"错误" : "Error") :
                        (DriverStatus == DRIVER_SUCCESS) ? (config.UseChinese ? u8"准备就绪" : "Ready") :
                        (DriverStatus == DRIVER_STOPPED) ? (config.UseChinese ? u8"已停止" : "Stopped") :
                        (config.UseChinese ? u8"正在运行" : "Running")
                    );
                    AttachTooltip(u8"开启驱动保护，可以保护GC进程不会被网易检测到。\n简称绕过网易，防止封号。\n使用该功能的后果由使用者承担。开发者不承担任何责任。");
                }
                EndDisabled();

                EndTabItem();
            }
            */
            
            EndTabBar();
            break;
        }
        case Tab::Script:
        {
            TabsText(config.UseChinese ? u8"脚本" : "Scripts");
            if (IsEnableScript);
            {
                const static ImGuiWindowFlags flags =
                    ImGuiWindowFlags_HorizontalScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoScrollbar;

                //-------------------------获取脚本--------------------------
                static int ScriptSize = 0;
                static int ScriptSelected = -1; // -1为没有打开任何设置
                static int ScriptTab = 0;
                static bool IsWillGotoShopTab = false; // 用于调整到其他tab

                static auto NothingsHereFunc = []()
                    {
                        const ImVec2 WindowSize = GetWindowSize();

                        const ImVec2 BoxSize(380, 160);
                        const ImVec2 BoxPos((WindowSize.x - BoxSize.x) / 2, (WindowSize.y - BoxSize.y) / 2);

                        static Texture2D iamge_texture = LoadTextureFromFile(Bmp_Empyt);
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
                const ImVec2 pos = GetCursorPos();
                if (BeginTabBar("##Script"))
                {
                    // shop放前面相当于先刷新shop，防止可用列表先加载导致检查更新失败
                    if (BeginTabItem(config.UseChinese ? u8"脚本商场" : "Shop", NULL, IsWillGotoShopTab ? ImGuiTabItemFlags_SetSelected : 0))
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
                        static bool IsRefushList = false;
                        if (!IsRefushList || IsNeedRefushShopList)
                        {
                            thread FuncThread([&]() {
                                ScriptShopList.clear();
                                TotalPaper = RefushScriptShopList(ScriptShopList, CurrentPaper, false);
                                IsRefushSuccess = true;
                                });
                            FuncThread.detach();
                            IsRefushList = true;
                            IsNeedRefushShopList = false;
                        }
                        if (IsRefushSuccess)
                        {
                            for (const ScriptInfo& script : ScriptShopList)
                            {
                                PushStyleColor(ImGuiCol_ChildBg, GetColorU32(GuiColor[ImGuiCol_FrameBg]));

                                PushID(count);
                                ImVec2 AnimPos = CalcWidgetsAnimHoriz(GetID(label.c_str()));
                                SetNextWindowPos(AnimPos);
                                BeginChild("##ScriptShopList", ImVec2(680, 72), true, flags);
                                {
                                    DisableAnimation(true);
                                    PopStyleColor(); // ImGuiCol_ChildBg
                                    // 绘画脚本图片
                                    Texture2D iamge_texture = LoadTextureFromFile(script.texture, false, D3D11_FILTER_MIN_MAG_MIP_POINT);
                                    SetCursorPosY(GetCursorPosY() - 6 + 2);
                                    ImageRounded((ImTextureID)iamge_texture, ImVec2(64, 64), 4.0f);

                                    SameLine();
                                    static ImVec2 pos2 = GetCursorPos();

                                    Text(script.title.c_str());
                                    SameLine();
                                    {
                                        const string VersionStr = (config.UseChinese ? u8"版本:" : "Version:") + to_string(script.ver);
                                        const ImVec2 SteSize = CalcTextSize(VersionStr.c_str());
                                        SetCursorPosX(680 - 6 - SteSize.x);
                                        Text(VersionStr.c_str());
                                    }
                                    SameLine();

                                    const bool HasLinkButtonShow = !script.link.empty();
                                    const char* LinkText = config.UseChinese ? u8"教程" : "Guide";
                                    const float LinkButtonWidth = CalcTextSize(LinkText).x + 12;
                                    {
                                        float TagetX = pos2.x + 480;
                                        if (script.IsHasUpdate)
                                            TagetX -= 48 + 6;
                                        if (HasLinkButtonShow)
                                            TagetX -= LinkButtonWidth + 6;
                                        SetCursorPosX(TagetX);
                                    }

                                    if (HasLinkButtonShow)
                                    {
                                        SetCursorPosY(12);
                                        if (Button(LinkText, ImVec2(LinkButtonWidth, 32)))
                                        {
                                            thread th([=]() {
                                                ShellExecute(NULL, NULL, script.link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                            });
                                            th.detach();
                                        }
                                        SameLine();
                                    }

                                    SetCursorPosY(12);
                                    static const Texture2D image_texture1 = LoadTextureFromFile(Bmp_Download);
                                    if (ImageButton("##script_download", (ImTextureID)image_texture1, ImVec2(32, 32)))
                                    {
                                        thread FuncThread([&]() {
                                            const string filename = ScriptShopDir + "\\" + script.name + ".gcdat";
                                            const string target = ScriptPath + "\\" + script.name + ".zip";
                                            if (CopyFile(filename.c_str(), target.c_str(), false))
                                                RenderTip(config.UseChinese ? (script.name + u8"下载成功") : (script.title + " Download Success"), 2.0f, PT_INFO);
                                            else
                                                RenderTip(config.UseChinese ? (script.name + u8"下载失败") : (script.title + " Download Failed"), 2.0f, PT_INFO);
                                            RefushScriptList(); // 刷新
                                            });
                                        FuncThread.detach();
                                    }

                                    SetCursorPos(ImVec2(pos2.x, pos2.y + 16));
                                    TextColored((ImVec4)ImColor(165, 165, 165, 255), script.info.c_str());

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
                                    IsRefushList = false;
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
                                    IsRefushList = false;
                                    IsRefushSuccess = false;
                                }
                            }
                            EndDisabled();

                        }
                        else
                            RenderLoadingAnimation();
                        EndTabItem();
                    }

                    if (BeginTabItem(config.UseChinese ? u8"可用" : "Available"))
                    {
                        IsShopTab = false;
                        static bool IsCheckUpdate = false;
                        if (!IsCheckUpdate)
                        {
                            thread FuncThread([&]() {
                                // 写入锁
                                lock_guard<shared_mutex> write_lock(AvaiableScriptsMtx);
                                for (ScriptInfo& script : AvaiableScripts)
                                    CheckUpdateScriptFromShop(script);
                                });
                            FuncThread.detach();
                            IsCheckUpdate = true;
                        }

                        // 读取锁
                        shared_lock<shared_mutex> read_lock(AvaiableScriptsMtx);
                        if (AvaiableScripts.size() == 0)
                            NothingsHereFunc();
                        //-------------------------脚本图片和信息--------------------------
                        const ImVec2 pos = GetCursorPos();
                        int count = 0;
                        for (ScriptInfo& script : AvaiableScripts)
                        {
                            PushStyleColor(ImGuiCol_ChildBg, GetColorU32(GuiColor[ImGuiCol_FrameBg]));

                            PushID(count);
                            ImVec2 AnimPos = CalcWidgetsAnimHoriz(GetID(label.c_str()));
                            SetNextWindowPos(AnimPos);
                            BeginChild("##AvailableScripts", ImVec2(680, 72), true, flags);
                            {
                                DisableAnimation(true);
                                PopStyleColor(); // ImGuiCol_ChildBg
                                // 绘画脚本图片
                                Texture2D iamge_texture = LoadTextureFromFile(script.texture, false, D3D11_FILTER_MIN_MAG_MIP_POINT);
                                SetCursorPosY(GetCursorPosY() - 6 + 2);
                                ImageRounded((ImTextureID)iamge_texture, ImVec2(64, 64), 4.0f);

                                SameLine();
                                static ImVec2 pos2 = GetCursorPos();

                                Text(script.title.c_str());
                                SameLine();
                                {
                                    const string VersionStr = (config.UseChinese ? u8"版本:" : "Version:") + to_string(script.ver);
                                    const ImVec2 SteSize = CalcTextSize(VersionStr.c_str());
                                    SetCursorPosX(680 - 6 - SteSize.x);
                                    Text(VersionStr.c_str());
                                }
                                SameLine();
                                const bool IsLinkButtonShow = !script.link.empty();
                                const char* LinkText = config.UseChinese ? u8"教程" : "Guide";
                                const float LinkButtonWidth = CalcTextSize(LinkText).x + 12;
                                {
                                    float TagetX = pos2.x + 460;
                                    if (script.IsHasUpdate)
                                        TagetX -= 48 + 6;
                                    if (IsLinkButtonShow)
                                        TagetX -= LinkButtonWidth + 6;
                                    SetCursorPosX(TagetX);
                                }

                                if (!script.IsInstalled)
                                {
                                    if (IsLinkButtonShow)
                                    {
                                        SetCursorPosY(16);
                                        if (Button(config.UseChinese ? u8"教程" : "Guide", ImVec2(LinkButtonWidth, 32)))
                                        {
                                            thread th([=]() {
                                                ShellExecute(NULL, NULL, script.link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                                });
                                            th.detach();
                                        }
                                        SameLine();
                                    }

                                    if (script.IsHasUpdate)
                                    {
                                        SetCursorPosY(16);
                                        if (Button(config.UseChinese ? u8"更新" : "Update", ImVec2(48, 32)))
                                        {
                                            thread FuncThread([=]() { // 不要使用&，因为script作为for的变量，会改变
                                                const string filename = ScriptShopDir + "\\" + script.name + ".gcdat";
                                                const string target = ScriptPath + "\\" + script.name + ".zip";
                                                if (CopyFile(filename.c_str(), target.c_str(), false))
                                                    RenderTip(config.UseChinese ? (script.name + u8"更新成功") : (script.title + " Update Success"), 2.0f, PT_INFO);
                                                else
                                                    RenderTip(config.UseChinese ? (script.name + u8"更新失败") : (script.title + " Update Failed"), 2.0f, PT_ERROR);
                                                RefushScriptList(); // 刷新
                                                });
                                            FuncThread.detach();
                                        }
                                        SameLine(0, 10);
                                    }

                                    SetCursorPosY(16);
                                    static const Texture2D image_texture1 = LoadTextureFromFile(Bmp_Add);
                                    if (ImageButton("##script_install", (ImTextureID)image_texture1, ImVec2(32, 32)))// 安装
                                    {
                                        read_lock.unlock();
                                        thread FuncThread([&]()
                                            {
                                            script.hThread = StartScript(script.file);
                                            script.IsInstalled = true;
                                            });
                                        FuncThread.detach();

                                        // 写入锁
                                        lock_guard<shared_mutex> write_lock(InstalledScriptsMtx);
                                        InstalledScripts.push_back(script);
                                        read_lock.lock();
                                    }
                                    SameLine(0, 10);
                                    SetCursorPosY(16);
                                    static const Texture2D image_texture2 = LoadTextureFromFile(Bmp_Delete);
                                    if (ImageButton("script_delete", (ImTextureID)image_texture2, ImVec2(32, 32))) // 删除文件
                                    {
                                        // 不要使用&，因为引用的count会因为for导致变化，count会++，所以count可能为末尾值
                                        // 所以erase越界，修复办法：用=直接复制
                                        thread FunThread([=]() {
                                            DeleteDirectoryRecursively(ScriptTemp + "\\" + script.name);
                                            DelFile(ScriptPath + "\\" + script.name + ".zip");
                                            AvaiableScripts.erase(AvaiableScripts.begin() + count);
                                            //RefushScriptList();
                                            });
                                        FunThread.detach();
                                    }
                                }
                                else
                                    TextColored((ImVec4)ImColor(192, 51, 74, 255), config.UseChinese ? u8"已安装" : "Installed");

                                SetCursorPos({ pos2.x, pos2.y + 16 });
                                TextColored((ImVec4)ImColor(165, 165, 165, 255), script.info.c_str());

                                const bool HasUsedFunc = script.RestrictedFuncsStr.length() > 0;
                                string str;
                                if (HasUsedFunc)
                                    str = (config.UseChinese ? u8"该脚本使用了：" : "The script used: ") + script.RestrictedFuncsStr;
                                SetCursorPos(ImVec2(680 - 16 - 4, 72 - 16 - 4));
                                static const Texture2D image_texture3 = LoadTextureFromFile(Bmp_Info);
                                Image((ImTextureID)image_texture3, ImVec2(16, 16));
                                AttachTooltip(HasUsedFunc ? str.c_str() : (config.UseChinese ? u8"&7未使用需要认证权限功能" : "&7No use of the function that \n&7requires authentication permission"));
                                DisableAnimation(false);
                            }
                            EndChild();
                            PopID();

                            count++;
                        }
                        EndTabItem();
                    }

                    if (BeginTabItem(config.UseChinese ? u8"已安装" : "Installed"))
                    {
                        IsShopTab = false;
                        if (InstalledScripts.size() == 0)
                            NothingsHereFunc();
                        int count = 0;
                        // 读取锁
                        shared_lock<shared_mutex> read_lock(InstalledScriptsMtx);
                        for (const ScriptInfo script : InstalledScripts)
                        {
                            PushStyleColor(ImGuiCol_ChildBg, GetColorU32(GuiColor[ImGuiCol_FrameBg]));

                            PushID(count);
                            ImVec2 AnimPos = CalcWidgetsAnimHoriz(GetID(label.c_str()));
                            SetNextWindowPos(AnimPos);
                            BeginChild("##InstalledScripts", ImVec2(680, 72), true, flags);
                            {
                                // 边框颜色显示-是否启用
                                //{
                                    // 写入锁
                                    //lock_guard<shared_mutex> lock(ScriptStatesMtx);
                                    //const bool IsEnable = ScriptStates[script.file].e
                                //}

                                DisableAnimation(true);
                                PopStyleColor(); // ImGuiCol_ChildBg
                                // 绘画脚本图片
                                Texture2D iamge_texture = LoadTextureFromFile(script.texture, false, D3D11_FILTER_MIN_MAG_MIP_POINT);
                                SetCursorPosY(GetCursorPosY() - 6 + 2);
                                ImageRounded((ImTextureID)iamge_texture, ImVec2(64, 64), 4.0f);

                                SameLine();
                                ImVec2 pos = GetCursorPos();

                                Text(script.title.c_str());
                                SameLine();
                                {
                                    const string VersionStr = to_string(script.ver);
                                    const ImVec2 SteSize = CalcTextSize(VersionStr.c_str());
                                    SetCursorPosX(680 - 6 - SteSize.x);
                                    Text(VersionStr.c_str());
                                }
                                SetCursorPos({ pos.x, pos.y + 16 });
                                TextColored((ImVec4)ImColor(165, 165, 165, 255), script.info.c_str());

                                const string KeyText = config.UseChinese ? u8"启动按键" : "Start key";
                                const float TextWidth = CalcTextSize(KeyText.c_str()).x;
                                SetCursorPos(ImVec2(pos.x + 380 - TextWidth - 6, 26));
                                Text(KeyText.c_str());
                                SameLine();
                                SetCursorPosY(26);
                                SetNextItemWidth(100);
                                {
                                    // 写入锁
                                    lock_guard<shared_mutex> lock(ScriptStatesMtx);
                                    InputKey("##script_key", &ScriptStates[script.file].key, config.UseChinese ? u8"请按下按键" : "Press a key");
                                }

                                SameLine(0, 10);
                                SetCursorPosY(16);

                                static const Texture2D image_texture1 = LoadTextureFromFile(Bmp_FunSetting);
                                // 脚本设置按键
                                if (ImageButton("##script_settings", (ImTextureID)image_texture1, ImVec2(32, 32)))
                                {
                                    ScriptSelected = count;
                                    // 写入锁
                                    lock_guard<shared_mutex> lock(ScriptStatesMtx);
                                    ScriptStates[script.file].IsSuspend = true;
                                }
                                SameLine(0, 10);
                                SetCursorPosY(16);

                                static const Texture2D image_texture2 = LoadTextureFromFile(Bmp_Delete);
                                // 删除脚本
                                if (ImageButton("##script_delete", (ImTextureID)image_texture2, ImVec2(32, 32)))
                                {
                                    thread FuncThread([](const ScriptInfo& script)
                                        {
                                            bool IsError = false;
                                            string errormsg;
                                            ScriptSelected = -1;
                                            {
                                                // 读取锁
                                                shared_lock<shared_mutex> lock666(AvaiableScriptsMtx);
                                                // 将可用脚本列表的IsInstalled改为false
                                                for (ScriptInfo& s : AvaiableScripts)
                                                {
                                                    if (s.file == script.file)
                                                    {
                                                        s.IsInstalled = false;
                                                        // 读取锁
                                                        shared_lock<shared_mutex> lock(ScriptStatesMtx);
                                                        CloseScriptThread(ScriptStates[s.name], s);
                                                        break;
                                                    }
                                                }
                                            }
                                            const string file = ScriptPath + "\\" + script.file;
                                            if (LuaStates.find(file) != LuaStates.end())
                                            {
                                                // 写入锁
                                                lock_guard<shared_mutex> lock(LuaStatesMtx);
                                                LuaStates[file].release();
                                                LuaStates.erase(file);
                                            }
                                            {
                                                // 写入锁
                                                lock_guard<shared_mutex> lock999(ScriptStatesMtx);
                                                ScriptStates.erase(file);
                                            }
                                            PrintLog("[Script] 已删除脚本：" + script.file, PL_INFO);
                                        }, script);
                                    FuncThread.detach();

                                    read_lock.unlock();
                                    {
                                        // 写入锁
                                        lock_guard<shared_mutex> write_lock(InstalledScriptsMtx);
                                        InstalledScripts.erase(InstalledScripts.begin() + count);
                                    }// 一定要等待write_lock生命周期结束，否则会死锁
                                    read_lock.lock();
                                }
                                DisableAnimation(false);
                            }
                            EndChild();
                            PopID();

                            count++;
                        }
                        EndTabItem();
                    }

                    EndTabBar();
                }
                
                SetCursorPos({ pos.x + 528, pos.y - 20 });
                
                static const Texture2D image_texture1 = LoadTextureFromFile(Bmp_Refresh);
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

                static const Texture2D image_texture2 = LoadTextureFromFile(Bmp_Book);
                if (ImageButton(config.UseChinese ? u8"脚本文档" : "Script Doc", (ImTextureID)image_texture2, ImVec2(32, 32)))
                    ShellExecute(NULL, NULL, "https://wiki.mcgc.top/zh/script", NULL, NULL, SW_SHOWNORMAL);

                SameLine();

                static bool IsClearing = false;
                static const Texture2D image_texture3 = LoadTextureFromFile(Bmp_ClearCache);
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

                //-----------------------脚本设置GUI---------------------------

                if (ScriptSelected > -1)
                {
                    // 读取锁
                    shared_lock<shared_mutex> lock(InstalledScriptsMtx);
                    const ScriptInfo script = InstalledScripts[ScriptSelected];
                    bool IsOpened = true;
                    if (BeginPopupWindow((script.title + u8"设置").c_str(), &IsOpened))
                    {
                        const string& filename = script.file;
                        unsigned int result = 0;
                        if (!RenderScriptGui(filename))
                            result = 1;

                        // 是否关闭或者崩溃
                        if (!IsOpened || result == 1)
                        {
                            ScriptSelected = -114514;
                            thread FuncThread(SaveScriptConfig, filename);
                            FuncThread.detach();
                            {
                                // 写入锁
                                lock_guard<shared_mutex> lock2(ScriptStatesMtx);
                                ScriptStates[filename].IsSuspend = false;
                            }
                            PrintLog("[Script] 已保存脚本配置：" + filename, PL_INFO);
                        }

                        EndPopupWindow();
                    }
                }
            }

            if (!IsEnableScript)
            {
                const static ImVec2 size = GetWindowSize();
                const static ImVec2 pos = { 102, 100 };
                const static ImVec4 color(
                    GuiColor[ImGuiCol_WindowBg].x,
                    GuiColor[ImGuiCol_WindowBg].y,
                    GuiColor[ImGuiCol_WindowBg].z,
                    GuiColor[ImGuiCol_WindowBg].w * 0.7f
                );
                ImDrawList* draw_list = GetForegroundDrawList();

                string str = config.UseChinese ? u8"&cLua脚本加载系统发生致命错误\n并且GC在尝试重启Lua脚本系统后无果" : "&cLua script loading system has a fatal error and \nthe GC has no choice but to restart the Lua script system";
                ImVec2 textSize = CalcTextSize(str.c_str());
                ImVec2 textPos = { (size.x - textSize.x) / 2, (size.y - textSize.y) / 2 - 15 };
                draw_list->AddText(textPos, GetColorU32(GuiColor[ImGuiCol_Text]), str.c_str());
            }

            break;
        }
        case Tab::Other://-----------------------其他---------------------------
        {
            TabsText(config.UseChinese ? u8"其他" : "Other");

            BeginTabBar("##Other");
            
            if (BeginTabItem(config.UseChinese ? u8"其他设置" : "OtherSettings"))
            {
                BeginDisabled(config.UseMinimalistMode);
                {
                    if (Checkbox(config.UseChinese ? u8"简洁模式" : "Concise Mode", &config.UseConciseMode))
                        ReloadClicker = true;
                }
                EndDisabled();

                if (Checkbox(config.UseChinese ? u8"极简模式" : "Minimal Mode", &config.UseMinimalistMode) && config.UseMinimalistMode)
                {
                    ReloadClicker = true; // 保存当前极简模式的开关状态
                    RenderMessageBox(config.UseChinese ? u8"打开极简功能后，大部分功能将隐藏。\n并且会重置连点设置。" : "After turning on the minimalist feature, most of the functions will be hidden. \nAnd it will reset the connection point settings.", config.UseChinese ? u8"警告" : "Warning", MB_OK, false);

                    config = Config();
                    // 为了保证极简为开启状态并且不会被重置
                    config.UseMinimalistMode = true;
                    config.UseConciseMode = true;

                    ReloadClicker = true; // 这个相当于保存了配置

                    RenderTip(config.UseChinese ? u8"重置成功" : "Reset Success", 1.5f, PT_INFO);
                }

                EndTabItem();
            }
            
            static int IsOpenLnagPaper = -1;
            if (IsOpenLnagPaper == 1)// 这个函数告诉ImGui，下一个要绘制的标签项应该处于打开/激活状态
                ImGui::SetNextItemOpen(true);
            if (BeginTabItem(config.UseChinese ? u8"语言设置" : "Language", NULL))
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
                            RenderTip(config.UseChinese ? u8"部分文字需要重启应用才能生效" : "Some text may need to restart the application to take effect.", 3.0f, PT_INFO);
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

                EndTabItem();
            }

            EndTabBar();

            break;
        }
        case Tab::About:
        {
            TabsText(config.UseChinese ? u8"其他" : "Other");

            BeginTabBar("##About");

            if (BeginTabItem(config.UseChinese ? u8"关于我们" : "About"))
            {
                const static ImGuiWindowFlags flags =
                    ImGuiWindowFlags_HorizontalScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoScrollbar;

                BeginCard(config.UseChinese ? u8"关于" : "About", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    BeginChild("##COM1919", ImVec2(0, 100), true, flags);
                    {
                        const ImVec2 pos = GetCursorPos();
                        Text(u8"COM1919");
                        SetCursorPos(ImVec2(pos.x, pos.y + 15));
                        TextColored((ImVec4)ImColor(149, 157, 164, 255), config.UseChinese ? u8"GC连点器开发者，连点器是我一个人开发的" : "Creator of GameClicker, I developed it alone.");
                    }
                    EndChild();

                    BeginChild("##挽芙大魔王", ImVec2(0, 100), true, flags);
                    {
                        const ImVec2 pos = GetCursorPos();
                        Text(u8"挽芙大魔王");
                        SetCursorPos(ImVec2(pos.x, pos.y + 15));
                        TextColored((ImVec4)ImColor(149, 157, 164, 255), config.UseChinese ? u8"YoClicker连点器开发者" : "Creator of YoClicker, he gave me many ideas, thank you very much.");
                    }
                    EndChild();

                    BeginChild("##BOWEN", ImVec2(0, 100), true, flags);
                    {
                        const ImVec2 pos = GetCursorPos();
                        Text(u8"y_bowen");
                        SetCursorPos(ImVec2(pos.x, pos.y + 15));
                        TextColored((ImVec4)ImColor(149, 157, 164, 255), config.UseChinese ? u8"提供网站和服务器的费用" : "Paid for website and server");
                        TextColored((ImVec4)ImColor(12, 12, 12, 255), config.UseChinese ? u8"从GC一开始的开发都有他的身影。" : "He is the face of GC from the beginning.");
                    }
                    EndChild();
                }
                EndCard();

                TabsText(config.UseChinese ? u8"相关链接" : "Related Links");
                BeginCard(config.UseChinese ? u8"网站链接" : "Website Links", ImVec2(180, 190), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    if (Button(config.UseChinese ? u8"论坛" : "Forum"))
                        ShellExecute(NULL, NULL, "https://bbs.mcgc.top", NULL, NULL, SW_SHOWNORMAL);
                    if (Button(config.UseChinese ? u8"文档 && 维基百科" : "Document && Wikipedia"))
                        ShellExecute(NULL, NULL, "https://wiki.mcgc.top", NULL, NULL, SW_SHOWNORMAL);
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
                    if (AdListMutex.try_lock_shared())
                    {
                        int count = -1;
                        for (string name : SponsorList)
                        {
                            const float TextWidth = CalcTextSize(name.c_str()).x;
                            count += TextWidth + 10;
                            if (count >= 680 - 50) // 680是BeginChild的宽度，减去50是为了容错
                                count = 0;
                            else if (count != -1)
                                SameLine(0, 10);
                            Text(name.c_str());
                        }
                        AdListMutex.unlock_shared();
                    }
                }
                EndCard();

                //-----------------------依赖的库---------------------------

                BeginCard(config.UseChinese ? u8"依赖的库" : "Dependencies", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                {
                    // 1. Dear ImGui
                    Text(u8"1. Dear ImGui");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##ImGui" : "License##ImGui"))
                        OpenPopup("LicensePopup_DearImGui");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2014-2024 Omar Cornut.");

                    // 1. Dear ImGui Popup
                    if (BeginPopup("LicensePopup_DearImGui"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "Dear ImGui - MIT License");
                        Separator();
                        TextWrapped(
                            "The MIT License (MIT)\n\n"
                            "Copyright (c) 2014-2024 Omar Cornut\n\n"
                            "Permission is hereby granted, free of charge, to any person obtaining a copy "
                            "of this software and associated documentation files (the \"Software\"), to deal "
                            "in the Software without restriction, including without limitation the rights "
                            "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
                            "copies of the Software, and to permit persons to whom the Software is "
                            "furnished to do so, subject to the following conditions:\n\n"
                            "The above copyright notice and this permission notice shall be included in all "
                            "copies or substantial portions of the Software.\n\n"
                            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
                            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
                            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
                            "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
                            "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
                            "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
                            "SOFTWARE."
                        );

                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 2. ImPlot
                    Text(u8"2. ImPlot");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##ImPlot" : "License##ImPlot"))
                        OpenPopup("LicensePopup_ImPlot");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2023 Evan Pezent.");

                    // ImPlot Popup
                    if (BeginPopup("LicensePopup_ImPlot"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "ImPlot - MIT License");
                        Separator();
                        TextWrapped(
                            "MIT License\n\n"
                            "Copyright (c) 2023 Evan Pezent\n\n"
                            "Permission is hereby granted, free of charge, to any person obtaining a copy "
                            "of this software and associated documentation files (the \"Software\"), to deal "
                            "in the Software without restriction, including without limitation the rights "
                            "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
                            "copies of the Software, and to permit persons to whom the Software is "
                            "furnished to do so, subject to the following conditions:\n\n"
                            "The above copyright notice and this permission notice shall be included in all "
                            "copies or substantial portions of the Software.\n\n"
                            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
                            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
                            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
                            "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
                            "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
                            "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
                            "SOFTWARE."
                        );
                       
                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 3. Stb
                    Text(u8"3. Stb");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##Stb" : "License##Stb"))
                        OpenPopup("LicensePopup_Stb");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2017 Sean Barrett.");

                    // Stb Popup
                    if (BeginPopup("LicensePopup_Stb"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "Stb - Public Domain / MIT");
                        Separator();
                        TextWrapped(
                            "This software is available under 2 licenses -- choose whichever you prefer.\n\n"
                            "ALTERNATIVE A - MIT License\n"
                            "Copyright (c) 2017 Sean Barrett\n\n"
                            "Permission is hereby granted, free of charge, to any person obtaining a copy of "
                            "this software and associated documentation files (the \"Software\"), to deal in "
                            "the Software without restriction, including without limitation the rights to "
                            "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies "
                            "of the Software, and to permit persons to whom the Software is furnished to do "
                            "so, subject to the following conditions:\n\n"
                            "The above copyright notice and this permission notice shall be included in all "
                            "copies or substantial portions of the Software.\n\n"
                            "ALTERNATIVE B - Public Domain (www.unlicense.org)\n"
                            "This is free and unencumbered software released into the public domain.\n\n"
                            "Anyone is free to copy, modify, publish, use, compile, sell, or distribute this "
                            "software, either in source code form or as a compiled binary, for any purpose, "
                            "commercial or non-commercial, and by any means."
                        );

                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 4. Mechvibes
                    Text(u8"4. Mechvibes");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##Mechvibes" : "License##Mechvibes"))
                        OpenPopup("LicensePopup_Mechvibes");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2023 Pyro Calzone.");

                    // Mechvibes Popup
                    if (BeginPopup("LicensePopup_Mechvibes"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "Mechvibes - MIT License");
                        Separator();
                        TextWrapped(
                            "MIT License\n\n"
                            "Copyright (c) 2023 Pyro Calzone\n\n"
                            "Permission is hereby granted, free of charge, to any person obtaining a copy "
                            "of this software and associated documentation files (the \"Software\"), to deal "
                            "in the Software without restriction, including without limitation the rights "
                            "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
                            "copies of the Software, and to permit persons to whom the Software is "
                            "furnished to do so, subject to the following conditions:\n\n"
                            "The above copyright notice and this permission notice shall be included in all "
                            "copies or substantial portions of the Software.\n\n"
                            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
                            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
                            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
                            "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
                            "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
                            "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
                            "SOFTWARE."
                        );
                       
                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 5. SDL2
                    Text(u8"5. SDL2");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##SDL2" : "License##SDL2"))
                        OpenPopup("LicensePopup_SDL2");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1997-2024 Sam Lantinga <slouken@libsdl.org>.");

                    // SDL2 Popup
                    if (BeginPopup("LicensePopup_SDL2"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "SDL2 - zlib License");
                        Separator();
                        TextWrapped(
                            "zlib License\n\n"
                            "Copyright (c) 1997-2024 Sam Lantinga <slouken@libsdl.org>\n\n"
                            "This software is provided 'as-is', without any express or implied warranty. "
                            "In no event will the authors be held liable for any damages arising from the "
                            "use of this software.\n\n"
                            "Permission is granted to anyone to use this software for any purpose, "
                            "including commercial applications, and to alter it and redistribute it freely, "
                            "subject to the following restrictions:\n\n"
                            "1. The origin of this software must not be misrepresented; you must not "
                            "claim that you wrote the original software. If you use this software in a "
                            "product, an acknowledgment in the product documentation would be appreciated "
                            "but is not required.\n\n"
                            "2. Altered source versions must be plainly marked as such, and must not be "
                            "misrepresented as being the original software.\n\n"
                            "3. This notice may not be removed or altered from any source distribution."
                        );

                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 6. Zlib
                    Text(u8"6. Zlib");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##Zlib" : "License##Zlib"))
                        OpenPopup("LicensePopup_Zlib");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1995-2024 Jean-loup Gailly and Mark Adler.");

                    // Zlib Popup (与 SDL2 类似，也是 zlib License)
                    if (BeginPopup("LicensePopup_Zlib"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "Zlib - zlib License");
                        Separator();
                        TextWrapped(
                            "zlib License\n\n"
                            "Copyright (c) 1995-2024 Jean-loup Gailly and Mark Adler\n\n"
                            "This software is provided 'as-is', without any express or implied warranty. "
                            "In no event will the authors be held liable for any damages arising from the "
                            "use of this software.\n\n"
                            "Permission is granted to anyone to use this software for any purpose, "
                            "including commercial applications, and to alter it and redistribute it freely, "
                            "subject to the following restrictions:\n\n"
                            "1. The origin of this software must not be misrepresented; you must not "
                            "claim that you wrote the original software. If you use this software in a "
                            "product, an acknowledgment in the product documentation would be appreciated "
                            "but is not required.\n\n"
                            "2. Altered source versions must be plainly marked as such, and must not be "
                            "misrepresented as being the original software.\n\n"
                            "3. This notice may not be removed or altered from any source distribution."
                        );

                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 7. Minizip
                    Text(u8"7. Minizip");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##Minizip" : "License##Minizip"))
                        OpenPopup("LicensePopup_Minizip");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1998-2010 Gilles Vollant.\nCopyright(C) 2007-2008 Even Rouault.");

                    // Minizip Popup
                    if (BeginPopup("LicensePopup_Minizip"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "Minizip - zlib License");
                        Separator();
                        TextWrapped(
                            "zlib License\n\n"
                            "Copyright (c) 1998-2010 Gilles Vollant\n"
                            "Copyright (c) 2007-2008 Even Rouault\n\n"
                            "This software is provided 'as-is', without any express or implied warranty. "
                            "In no event will the authors be held liable for any damages arising from the "
                            "use of this software.\n\n"
                            "Permission is granted to anyone to use this software for any purpose, "
                            "including commercial applications, and to alter it and redistribute it freely, "
                            "subject to the following restrictions:\n\n"
                            "1. The origin of this software must not be misrepresented; you must not "
                            "claim that you wrote the original software. If you use this software in a "
                            "product, an acknowledgment in the product documentation would be appreciated "
                            "but is not required.\n\n"
                            "2. Altered source versions must be plainly marked as such, and must not be "
                            "misrepresented as being the original software.\n\n"
                            "3. This notice may not be removed or altered from any source distribution."
                        );

                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 8. Picojson
                    Text(u8"8. Picojson");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##Picojson" : "License##Picojson"))
                        OpenPopup("LicensePopup_Picojson");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 2009-2010 Cybozu Labs, Inc.\nCopyright(C) 2011-2014 Kazuho Oku.");

                    // Picojson Popup
                    if (BeginPopup("LicensePopup_Picojson"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "Picojson - BSD 2-Clause License");
                        Separator();
                        TextWrapped(
                            "BSD 2-Clause License\n\n"
                            "Copyright (c) 2009-2010 Cybozu Labs, Inc.\n"
                            "Copyright (c) 2011-2014 Kazuho Oku\n\n"
                            "Redistribution and use in source and binary forms, with or without "
                            "modification, are permitted provided that the following conditions are met:\n\n"
                            "1. Redistributions of source code must retain the above copyright notice, "
                            "this list of conditions and the following disclaimer.\n\n"
                            "2. Redistributions in binary form must reproduce the above copyright notice, "
                            "this list of conditions and the following disclaimer in the documentation "
                            "and/or other materials provided with the distribution.\n\n"
                            "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" "
                            "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE "
                            "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE "
                            "DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE "
                            "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL "
                            "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR "
                            "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER "
                            "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, "
                            "OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE "
                            "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
                        );

                        PopStyleVar(2);
                        EndPopup();
                    }

                    DrawSpiltLine(800, GuiColor[ImGuiCol_FrameBgHovered], 4.0f, 20);

                    // 9. Lua
                    Text(u8"9. Lua");
                    SameLine();
                    if (Button(config.UseChinese ? "许可证信息##Lua" : "License##Lua"))
                        OpenPopup("LicensePopup_Lua");
                    TextColored((ImVec4)ImColor(149, 157, 164, 255), u8"Copyright(C) 1994-2025 Lua.org, PUC-Rio.");

                    // Lua Popup
                    if (BeginPopup("LicensePopup_Lua"))
                    {
                        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

                        TextColored(ImVec4(1, 1, 0, 1), "Lua - MIT License");
                        Separator();
                        TextWrapped(
                            "MIT License\n\n"
                            "Copyright (c) 1994-2025 Lua.org, PUC-Rio.\n\n"
                            "Permission is hereby granted, free of charge, to any person obtaining a copy "
                            "of this software and associated documentation files (the \"Software\"), to deal "
                            "in the Software without restriction, including without limitation the rights "
                            "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
                            "copies of the Software, and to permit persons to whom the Software is "
                            "furnished to do so, subject to the following conditions:\n\n"
                            "The above copyright notice and this permission notice shall be included in all "
                            "copies or substantial portions of the Software.\n\n"
                            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
                            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
                            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
                            "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
                            "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
                            "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
                            "SOFTWARE."
                        );

                        PopStyleVar(2);
                        EndPopup();
                    }
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
                    TextUnformatted(UserAgreement.c_str(), UserAgreement.c_str() + UserAgreement.size());
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
        Text(UserAgreement.c_str());
        SetWindowFontScale(1.0f);
        EndChild();

        Text(config.UseChinese ? u8"&c本软件完全免费！！！\n&c如果你是购买的本软件，可以举报。" : "This software is completely free! \nIf you have purchased this software, you can report it.");

        static bool IsCheck = false;
        Checkbox(config.UseChinese ? u8"我已阅读并同意用户协议" : "I have read and agree to the user agreement", & IsCheck);

        if (Button(config.UseChinese ? u8"不同意" : "Decline", ImVec2(70, 40)))
            GCExit = true;
        SameLine();
        BeginDisabled(!IsCheck);
        {
            if (Button(config.UseChinese ? u8"同意" : " Next", ImVec2(70, 40)) && IsCheck)
            {
                IsNeedReshowAgreement = false;
                thread th(SaveIni, IniPos);
                th.detach();
            }
        }
        EndDisabled();
        EndPopupWindow();
    }

    //-----------------------绘制标题栏图标---------------------------
    static const Texture2D image_texture = LoadTextureFromFile(Bmp_GCIcon);

    GetForegroundDrawList()->
        AddImage((ImTextureID)image_texture, ImVec2(6, 6), ImVec2(26, 26));

    End();

    RenderControl();

    ImVec2 TextSize = CalcTextSize(client_id.c_str());
    GetForegroundDrawList()->AddText(ImVec2((WindowWidth - TextSize.x) / 2, 6), GetColorU32((ImVec4)ImColor(128, 255, 0, 5)), client_id.c_str());
}
// 哎~ 这代码写的真的好烂啊...