#pragma once
#include <Windows.h>
#include "Definitons.h"

DWORD WINAPI CreateTrayIcon(LPVOID lpParam);
LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

DWORD WINAPI CreateTrayIcon(LPVOID lpParam)
{
    HWND hwnd{};
    WNDCLASS wndclass{};

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = TrayWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = GCInstance;
    wndclass.hIcon = LoadIcon(GCInstance, MAKEINTRESOURCE(GC_ICON));
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = "GameClicker TRAY";

    if (!RegisterClass(&wndclass))
        return -1;

    TrayHwnd = CreateWindowEx(WS_EX_TOOLWINDOW, wndclass.lpszClassName, wndclass.lpszClassName,
        WS_POPUP, 0, 0, 0, 0, NULL, NULL, GCInstance, NULL);

    ShowWindow(TrayHwnd, SW_HIDE);
    UpdateWindow(TrayHwnd);

    {
        lock_guard<shared_mutex> lock(InitSuccessTimeMutex);
        if (InitSuccessTime == 0 || InitSuccessTime < GetNowMs())
            InitSuccessTime = GetNowMs();
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (GCExit)
            break;
    }
    return 0;
}

#define MENU_EXIT 1
#define MENU_SHOW 2
#define MENU_SITE 3
#define MENU_SPONSOR 4

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
    switch (message)
    {
        case WM_CREATE:
        {
            TrayNid.cbSize = sizeof(TrayNid);
            TrayNid.hWnd = hwnd;
            TrayNid.uID = 0;
            TrayNid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            TrayNid.uCallbackMessage = WM_TRAYICON;
            TrayNid.hIcon = LoadIcon(GCInstance, MAKEINTRESOURCE(GC_ICON));

            lstrcpy(TrayNid.szTip, "GameClicker");
            Shell_NotifyIcon(NIM_ADD, &TrayNid);
            break;
        }
        case WM_TRAYICON:
        {
            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                {
                    static time_t LastTime = 0;
                    if (GetTickCount64() - LastTime > 100)
                    {
                        ShowGui = !ShowGui;
                        ShowWindow(GCHWND, ShowGui);
                        LastTime = GetTickCount64();
                    }
                    break;
                }
                case WM_RBUTTONDOWN:
                {
                    POINT pt;
                    GetCursorPos(&pt);

                    SetForegroundWindow(hwnd);

                    // 创建弹出菜单
                    HMENU hPopupMenu = CreatePopupMenu();

                    AppendMenu(hPopupMenu, MF_STRING, MENU_SHOW, "显示");
                    AppendMenu(hPopupMenu, MF_STRING, MENU_SHOW, "官网");
                    AppendMenu(hPopupMenu, MF_STRING, MENU_SPONSOR, "赞助");
                    AppendMenu(hPopupMenu, MF_STRING, MENU_EXIT, "退出");
                    
                    int cmd = TrackPopupMenu(hPopupMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hPopupMenu);

                    // 根据菜单项的ID处理菜单命令
                    switch (cmd)
                    {
                        case MENU_EXIT:
                        {
                            GCExit = true;
                            break;
                        }
                        case MENU_SHOW:
                        {
                            ShowGui = true;
                            ShowWindow(GCHWND, ShowGui);
                            break;
                        }
                        case MENU_SITE:
                        {
                            ShellExecute(NULL, NULL, "https://www.mcgc.top", NULL, NULL, SW_SHOWNORMAL);
                            break;
                        }
                        case MENU_SPONSOR:
                        {
                            ShellExecute(NULL, NULL, "https://afdian.com/a/gameclicker", NULL, NULL, SW_SHOWNORMAL);
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        default:
        {
           // 防止当Explorer.exe 崩溃以后，程序在系统托盘中的图标就消失
            if (message == WM_TASKBARCREATED)
                SendMessage(hwnd, WM_CREATE, wParam, lParam);
            break;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}