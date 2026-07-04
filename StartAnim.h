#pragma once
#include "resource.h"
#include "maths.h"
#include "ShareCode.h"

void WindowCloseAnimate(HWND Hwnd, POINT pos, float speed = 0.25f);
void WindowOpenAnimate(HWND Hwnd, POINT pos, float speed = 0.25f);

class WindowDraw
{
private:
    RECT wdRect;
    HWND wdHwnd;
    HDC wdDC;
    HDC wdMemDC;
    HBITMAP wdMemBmp;
    bool wdClose = false;
    int TextSize = 12;
    char* FontFile = NULL;
    int WindowsX = 0;
    int WindowsY = 0;
    int WindowsWidth = 0;
    int WindowsHeight = 0;
    static LRESULT CALLBACK DrawProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void ReloadTextFont();
public:
    void DrawString(string st, int tx, int ty, int size, COLORREF color);
    void SetTextSize(int size);
    BOOL DrawIcon(int ix1, int iy1, int Width, int Height, LPCSTR lpIconName);
    HDC GetDrawHDC() const;
    void DrawLine(int x1, int y1, int x2, int y2, COLORREF color, int width);
    void DrawRound(int x1, int y1, int x2, int y2, int Radius, COLORREF color, bool fill);
    void DrawMemory();
    void DeleateMemory();
    void CreateMemory();
    void SetWindow(int x, int y, int width, int height);
    void Init();
    HWND GetDrawWindowHwnd() const;
    ~WindowDraw();
};

LRESULT CALLBACK WindowDraw::DrawProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void WindowDraw::Init()
{
    HINSTANCE Hinstance = (HINSTANCE)GetModuleHandle(NULL);
    // 注册窗口类
    WNDCLASSEX wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowDraw::DrawProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = Hinstance;
    wcex.hIcon = LoadIcon(Hinstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GC Starting";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassEx(&wcex))
        return;
    // 创建窗口
    wdHwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, "GC Starting", "", WS_POPUP, WindowsX, WindowsY, WindowsWidth, WindowsHeight, NULL, NULL, Hinstance, NULL);
    if (!wdHwnd)
        return;
    // 设置窗口透明度
    SetLayeredWindowAttributes(wdHwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(wdHwnd, true);
    //UpdateWindow(wdHwnd);

    wdDC = GetDC(wdHwnd);

    wdMemDC = wdDC;
    // 初始化成员变量
    wdRect = { 0, 0, 0, 0 };
    wdMemBmp = NULL;
}

void WindowDraw::SetWindow(int x, int y, int width, int height)
{
    if (wdHwnd != NULL)
        SetWindowPos(wdHwnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);
    WindowsX = x;
    WindowsY = y;
    WindowsWidth = width;
    WindowsHeight = height;
}

void WindowDraw::CreateMemory()
{
    CreateCompatibleDC(wdMemDC);
    wdMemBmp = CreateCompatibleBitmap(wdDC, WindowsWidth, WindowsHeight);
    SelectObject(wdMemDC, wdMemBmp);
}

void WindowDraw::DrawMemory()
{
    BitBlt(wdDC, WindowsX, WindowsY, WindowsWidth, WindowsHeight, wdMemDC, 0, 0, SRCCOPY);
}

void WindowDraw::DeleateMemory()
{
    DeleteObject(wdMemBmp);
}

void WindowDraw::ReloadTextFont()
{
    HFONT hFont = CreateFont(TextSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "方正粗黑宋简体");
    SelectObject(wdMemDC, hFont);
}

void WindowDraw::SetTextSize(int size)
{
    TextSize = size;
    ReloadTextFont();
}

void WindowDraw::DrawString(string st, int tx, int ty, int size, COLORREF color)
{
    SetTextSize(size);
    
    SelectObject(wdMemDC, CreateSolidBrush(color));
    SetTextColor(wdMemDC, color);
    SetBkMode(wdMemDC, TRANSPARENT);

    SIZE textSize;
    GetTextExtentPoint32(wdMemDC, st.c_str(), (int)st.size(), &textSize);

    RECT TextRect{};
    TextRect.left = tx;
    TextRect.top = ty;
    TextRect.right = tx + textSize.cx;
    TextRect.bottom = ty + textSize.cy;

    DrawTextA(wdMemDC, st.c_str(), (int)st.size(), &TextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

BOOL WindowDraw::DrawIcon(int ix1, int iy1, int Width, int Height, LPCSTR lpIconName)
{
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), lpIconName);
    if (!hIcon) return FALSE;

    DestroyIcon(hIcon);

    return DrawIconEx(wdMemDC, ix1, iy1, hIcon, Width, Height, 0, NULL, DI_NORMAL);
}

void WindowDraw::DrawLine(int x1, int y1, int x2, int y2, COLORREF color, int width)
{
    SelectObject(wdMemDC, CreateSolidBrush(color));
    MoveToEx(wdMemDC, x1, y1, NULL);
    LineTo(wdMemDC, x2, y2);
}

void WindowDraw::DrawRound(int x1, int y1, int x2, int y2, int Radius, COLORREF color, bool fill = false)
{
    SelectObject(wdMemDC, CreateSolidBrush(color));
    RoundRect(wdMemDC, x1, y1, x2, y2, Radius, Radius);
}

HDC WindowDraw::GetDrawHDC() const
{
    return wdMemDC;
}

HWND WindowDraw::GetDrawWindowHwnd() const
{
    return wdHwnd;
}

WindowDraw::~WindowDraw()
{
    ReleaseDC(wdHwnd, wdMemDC);
    UpdateWindow(wdHwnd);
    SetWindowPos(wdHwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_SHOWWINDOW);
    DestroyWindow(wdHwnd);
    UnregisterClass("GCDrawWindow", GetModuleHandle(NULL));
}

void LoadAnimation()
{
    // 窗口大小
    const int WindowWidth = 530;
    const int WindowHeight = 82;
    int smx = (GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2;
    int smy = (GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2;
    WindowDraw Draw;
    Draw.SetWindow(smx, smy, WindowWidth, WindowHeight);
    Draw.Init();

    Draw.DrawIcon((WindowWidth - 64) / 2, (WindowHeight - 64) / 2, 64, 64, MAKEINTRESOURCE(GC_ICON));
    Sleep(800);

    //动画的相对坐标位置
    int posy1 = 0;
    int posy2 = posy1 + 82;

    const string Title = "GAMECLICKER";
    int TtiLelen = Title.size();

    COLORREF tColor = RGB(178, 24, 208);
    LPCSTR IDI = MAKEINTRESOURCE(GC_ICON);
    const int FPS = 60;
    float AnimSpeed = 0.2f * (1.0f - (1.0f / (1000.0f / FPS)));

    int tick = 0;
    const int MaxTick = WindowWidth / 2;
    while (tick < MaxTick - 2)
    {
        BeginSleep("DrawAnimation");
        Draw.DeleateMemory();
        Draw.CreateMemory();

        tick = Lerp(tick, MaxTick + 4, AnimSpeed);
        int x = (WindowWidth / 2) - tick;
        string str = Title;

        Draw.DrawRound(x, posy1, (WindowWidth / 2) + tick, posy2, 20, RGB(60, 60, 60), true);
        Draw.DrawIcon(((tick * 2) > 73) ? x + 9 : (WindowWidth / 2) - 32, posy1 + 9, 64, 64, IDI);

        int dlen = ((tick * 2) - 82 - 12) / 62 * 2;
        Draw.DrawString((dlen >= (TtiLelen - 1)) ? str : str.erase((dlen < 0) ? 0 : dlen, TtiLelen), x + 82, WindowHeight / 2 - 32, 64, tColor);
        Draw.DrawMemory();
        EndSleep("DrawAnimation", 1000 / FPS);
    }
    ULONGLONG tickCount = GetTickCount64();
    // 转换为秒
    long seconds = tickCount / 1000;

    long end_time = seconds + 3;
    int x = (WindowWidth / 2) - MaxTick;
    Sleep(1500);
}

void WindowOpenAnimate(HWND Hwnd, POINT pos, float speed)
{
    static auto AnimateThread = [](HWND hwnd, POINT pos, float speed)
        {
            int ScreenSize = GetSystemMetrics(SM_CYSCREEN);
            int PosY = ScreenSize;

            const int FPS = 60;
            float AnimSpeed = speed * (1.0f - (1.0f / (1000.0f / FPS)));
            while (abs(PosY - pos.y) > 2)
            {
                BeginSleep("WindowOpenAnimate");

                PosY = Lerp(PosY, (int)pos.y, AnimSpeed);
                SetWindowPos(hwnd, NULL, pos.x, PosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

                EndSleep("WindowOpenAnimate", 1000 / FPS);
            }
            SetWindowPos(hwnd, NULL, pos.x, pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        };
    thread FunThread(AnimateThread, Hwnd, pos, speed);
    FunThread.detach();
}

void WindowCloseAnimate(HWND Hwnd, POINT pos, float speed)
{
    static auto AnimateThread = [](HWND hwnd, POINT pos, float speed)
        {
            RECT rect{};
            GetWindowRect(hwnd, &rect);
            int TagerPosY = GetSystemMetrics(SM_CYSCREEN);
            
            const int FPS = 60;
            float AnimSpeed = speed * (1.0f - (1.0f / (1000.0f / FPS)));
            while (abs(pos.y - TagerPosY) > 2)
            {
                BeginSleep("WindowCloseAnimate");

                pos.y = Lerp((int)pos.y, TagerPosY, AnimSpeed);
                SetWindowPos(hwnd, NULL, pos.x, pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

                EndSleep("WindowCloseAnimate", 1000 / FPS);
            }
            SetWindowPos(hwnd, NULL, pos.x, TagerPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        };
    thread FunThread(AnimateThread, Hwnd, pos, speed);
    FunThread.detach();
}

DWORD WINAPI LoadAnimationThread(LPVOID lpParam)
{
    while (!IsCreateWindowSussce)
        Sleep(1);
#ifdef  GC_NO_ANIMATION
    ShowWindow(GCHWND, SW_SHOWDEFAULT);
    return 0;
#endif // GC_NO_ANIMATION
    Starting = true;
    LoadAnimation();

    ShowWindow(GCHWND, SW_SHOWDEFAULT);

    RECT rect{};
    GetWindowRect(GCHWND, &rect);

    PrintLog("[StartAnim] 窗口开始移动", PL_INFO);
    const int x = (ScreenWidth - WindowWidth) / 2;
    SetWindowPos(GCHWND, HWND_TOP, x, ScreenHeigth + 30, WindowWidth, WindowHeight, SWP_NOMOVE);
    WindowOpenAnimate(GCHWND, { x, (ScreenHeigth - WindowHeight) / 2 }, 0.3f);

    SetForegroundWindow(GCHWND);
    PrintLog("[StartAnim] 动画播放完成！", PL_INFO);
    Starting = false;
    return 0;
}