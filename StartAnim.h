#pragma once
#include "resource.h"
#include "HelperCode.h"

// libwebp 头文件
#include <webp/demux.h>
#include <webp/decode.h>

#undef free

// 窗口尺寸
const int WINDOW_WIDTH = 434;
const int WINDOW_HEIGHT = 72;

// 全局资源
HWND AnimHwnd = nullptr;
ID2D1Factory* D2dFactory = nullptr;
ID2D1HwndRenderTarget* RenderTarget = nullptr;
ID2D1Bitmap* Bitmap = nullptr;           // 当前显示的位图

// 动画帧数据（预创建位图）
struct FrameData
{
    ID2D1Bitmap* bitmap;   // 预创建的位图
    int duration_ms;       // 该帧持续毫秒数
};
FrameData* AnimFrames = nullptr;
UINT TotalFrames = 0;
UINT CurrentFrame = 0;
UINT ImageWidth = 0;
UINT ImageHeight = 0;
DWORD AnimStartTime = 0;
MMRESULT FrameTimerId = 0;
bool AnimPlaying = false;
DWORD AccumulatedTime = 0;
DWORD LastFrameTime = 0;

#define WM_FRAME_UPDATE (WM_USER + 100)

// 窗口居中
static void CenterWindow(HWND hwnd)
{
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, nullptr,
        (sw - WINDOW_WIDTH) / 2,
        (sh - WINDOW_HEIGHT) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// 初始化 Direct2D
static bool InitD2D(HWND hwnd)
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2dFactory);
    if (FAILED(hr))
        return false;

    RECT rc;
    GetClientRect(hwnd, &rc);
    hr = D2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        ),
        D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)),
        &RenderTarget
    );
    return SUCCEEDED(hr);
}

// 从 BGRA 数据直接创建 D2D 位图（不依赖 WIC）
static bool CreateBitmapFromBGRA(const unsigned char* bgraData, UINT width, UINT height)
{
    if (!RenderTarget || !bgraData || width == 0 || height == 0)
        return false;

    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    HRESULT hr = RenderTarget->CreateBitmap(
        D2D1::SizeU(width, height),
        bgraData,
        width * 4,          // stride
        props,
        &Bitmap
    );
    return SUCCEEDED(hr);
}

// 释放所有帧资源
static void FreeFrames()
{
    if (AnimFrames)
    {
        for (UINT i = 0; i < TotalFrames; ++i)
        {
            if (AnimFrames[i].bitmap)
            {
                AnimFrames[i].bitmap->Release();
                AnimFrames[i].bitmap = nullptr;
            }
        }
        free(AnimFrames);
        AnimFrames = nullptr;
    }
    TotalFrames = 0;
    CurrentFrame = 0;
}

// 使用 libwebp 解码动画并预创建所有帧的 D2D 位图
static bool DecodeWebPAnimation(const char* filename)
{
    // 读取文件
    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD fileSize = GetFileSize(hFile, nullptr);
    if (fileSize == 0)
    {
        CloseHandle(hFile);
        return false;
    }

    unsigned char* fileData = (unsigned char*)malloc(fileSize);
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, fileData, fileSize, &bytesRead, nullptr) || bytesRead != fileSize)
    {
        free(fileData);
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);

    // 准备 WebPData
    WebPData webpData;
    WebPDataInit(&webpData);
    webpData.bytes = fileData;
    webpData.size = fileSize;

    // 初始化解码器
    WebPAnimDecoderOptions decOptions;
    WebPAnimDecoderOptionsInit(&decOptions);
    decOptions.color_mode = MODE_BGRA;

    WebPAnimDecoder* decoder = WebPAnimDecoderNew(&webpData, &decOptions);
    if (!decoder)
    {
        free(fileData);
        return false;
    }

    WebPAnimInfo animInfo;
    if (!WebPAnimDecoderGetInfo(decoder, &animInfo))
    {
        WebPAnimDecoderDelete(decoder);
        free(fileData);
        return false;
    }

    ImageWidth = animInfo.canvas_width;
    ImageHeight = animInfo.canvas_height;
    UINT frameCount = animInfo.frame_count;
    if (frameCount == 0)
    {
        WebPAnimDecoderDelete(decoder);
        free(fileData);
        return false;
    }

    // 分配帧数组
    FrameData* frames = (FrameData*)malloc(sizeof(FrameData) * frameCount);
    if (!frames)
    {
        WebPAnimDecoderDelete(decoder);
        free(fileData);
        return false;
    }

    // 逐帧解码并创建位图
    UINT decodedFrames = 0;
    while (WebPAnimDecoderHasMoreFrames(decoder))
    {
        uint8_t* frameBgra = nullptr;
        int timestamp = 0;
        if (!WebPAnimDecoderGetNext(decoder, &frameBgra, &timestamp))
            break;

        // 手动进行 Alpha 预乘处理
        UINT pixelCount = animInfo.canvas_width * animInfo.canvas_height;
        for (UINT i = 0; i < pixelCount; ++i)
        {
            uint8_t a = frameBgra[i * 4 + 3];
            if (a < 255)
            {
                frameBgra[i * 4 + 0] = (uint8_t)((frameBgra[i * 4 + 0] * a) / 255); // B
                frameBgra[i * 4 + 1] = (uint8_t)((frameBgra[i * 4 + 1] * a) / 255); // G
                frameBgra[i * 4 + 2] = (uint8_t)((frameBgra[i * 4 + 2] * a) / 255); // R
                // A 保持不变
            }
        }
        // 创建 D2D 位图
        ID2D1Bitmap* bmp = nullptr;
        HRESULT hr = RenderTarget->CreateBitmap(
            D2D1::SizeU(animInfo.canvas_width, animInfo.canvas_height),
            frameBgra,
            animInfo.canvas_width * 4,
            // 关键：恢复为 PREMULTIPLIED，配合上面的手动预乘，消除黑边且不会报错
            D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
            &bmp
        );
        if (FAILED(hr))
        {
            // 清理已分配的帧
            for (UINT i = 0; i < decodedFrames; ++i)
                if (frames[i].bitmap) frames[i].bitmap->Release();
            free(frames);
            WebPAnimDecoderDelete(decoder);
            free(fileData);
            return false;
        }

        frames[decodedFrames].bitmap = bmp;
        frames[decodedFrames].duration_ms = timestamp;
        decodedFrames++;
    }

    // 计算每帧持续时间，并按你的要求将延迟向前移一位（当前帧用上一帧的延迟）
    if (decodedFrames > 0)
    {
        // 1. 先算出原始的相邻帧时间差（相对延迟）
        vector<int> originalDurations(decodedFrames, 16);
        for (UINT i = 0; i < decodedFrames - 1; ++i)
        {
            int diff = frames[i + 1].duration_ms - frames[i].duration_ms;
            if (diff <= 0)
                diff = 16;
            originalDurations[i] = diff;
        }

        // 2. 重新组合：当前帧的延迟使用上一帧的延迟
        vector<int> shiftedDurations(decodedFrames, 16);
        shiftedDurations[0] = originalDurations[0]; // 第一帧用自己默认的时间
        for (UINT i = 1; i < decodedFrames; ++i)
            shiftedDurations[i] = originalDurations[i - 1]; // 当前帧用上一帧的延迟

        // 3. 重新生成平滑后的绝对时间戳，供 WM_FRAME_UPDATE 使用
        int accumTimestamp = 0;
        for (UINT i = 0; i < decodedFrames; ++i)
        {
            int currentDuration = shiftedDurations[i];   // 获取错位后的相对延迟
            frames[i].duration_ms = accumTimestamp;      // 覆盖为新的绝对时间戳
            accumTimestamp += currentDuration;
        }
    }

    TotalFrames = decodedFrames;
    AnimFrames = frames;

    WebPAnimDecoderDelete(decoder);
    free(fileData);
    return true;
}

// 切换到指定帧（直接切换指针，无需重建）
static bool SwitchToFrame(UINT frameIndex)
{
    if (!AnimFrames || frameIndex >= TotalFrames)
        return false;

    if (Bitmap)
    {
        Bitmap->Release();
        Bitmap = nullptr;
    }
    Bitmap = AnimFrames[frameIndex].bitmap;
    if (Bitmap) Bitmap->AddRef(); // 增加引用，保证释放时正常
    return (Bitmap != nullptr);
}

// 渲染当前帧
static void RenderFrame()
{
    if (!RenderTarget || TotalFrames == 0) return;
    ID2D1Bitmap* currentBmp = AnimFrames[CurrentFrame].bitmap;

    RenderTarget->BeginDraw();
    RenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));

    if (currentBmp)
    {
        float x = (WINDOW_WIDTH - ImageWidth) / 2.0f;
        float y = (WINDOW_HEIGHT - ImageHeight) / 2.0f;
        RenderTarget->DrawBitmap(currentBmp,
            D2D1::RectF(x, y, x + ImageWidth, y + ImageHeight));
    }

    RenderTarget->EndDraw();
}

void CALLBACK FrameTimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    // 向窗口发送自定义消息，触发帧更新（避免在回调中直接操作UI）
    PostMessage(AnimHwnd, WM_FRAME_UPDATE, 0, 0);
}

static LRESULT CALLBACK StartupAnimWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            RenderFrame();
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_FRAME_UPDATE:
        {
            if (TotalFrames == 0)
                break;

            DWORD now = timeGetTime();
            DWORD elapsed = now - AnimStartTime; // 当前距离动画开始的绝对时间差

            // 查找当前时间应该显示哪一帧
            UINT targetFrame = 0;
            for (UINT i = 0; i < TotalFrames; ++i)
            {
                // AnimFrames[i].duration_ms 存储的是该帧开始的绝对时间戳
                if (elapsed >= (DWORD)AnimFrames[i].duration_ms)
                    targetFrame = i;
                else
                    break; // 时间戳是递增的，一旦当前时间小于该帧时间戳，说明还没到该帧
            }

            // 如果目标帧和当前帧不同，则切换并渲染
            if (targetFrame != CurrentFrame)
            {
                CurrentFrame = targetFrame;
                RenderFrame();
            }

            // 判断动画是否结束
            DWORD lastFrameStart = AnimFrames[TotalFrames - 1].duration_ms;
            DWORD lastDuration = 16; // 默认最后一帧16ms
            if (TotalFrames >= 2)
            {
                // 估算最后一帧的时长：用最后一帧和倒数第二帧的时间差
                lastDuration = lastFrameStart - AnimFrames[TotalFrames - 2].duration_ms;
                if (lastDuration <= 0)
                    lastDuration = 16;
            }

            // 如果当前时间已经超过了最后一帧的开始时间 + 最后一帧的预估时长，则退出
            if (elapsed >= lastFrameStart + lastDuration)
                PostQuitMessage(0);
            return 0;
        }
        case WM_TIMER:
        {
            if (wParam == 1) // 备用强制退出
            {
                KillTimer(hwnd, 1);
                PostQuitMessage(0);
            }
            else if (wParam == 2) // 最后一帧播完退出
            {
                KillTimer(hwnd, 2);
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void Cleanup()
{
    if (Bitmap) { Bitmap->Release();       Bitmap = nullptr; }
    if (RenderTarget) { RenderTarget->Release(); RenderTarget = nullptr; }
    if (D2dFactory) { D2dFactory->Release();   D2dFactory = nullptr; }
    FreeFrames();
}

static int RenerStartupAnimation(HINSTANCE hInstance)
{
    CoInitialize(nullptr);

    // 注册窗口类
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StartupAnimWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "GC Startup Class";
    wc.hbrBackground = nullptr; // 必须为 nullptr
    RegisterClassEx(&wc);

    AnimHwnd = CreateWindowEx(
        WS_EX_TRANSPARENT  | WS_EX_TOPMOST,
        "GC Startup Class", "",
        WS_POPUP, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(AnimHwnd, &margins);

    if (!AnimHwnd)
    {
        CoUninitialize();
        return 1;
    }

    CenterWindow(AnimHwnd);

    if (!InitD2D(AnimHwnd))
    {
        DestroyWindow(AnimHwnd);
        CoUninitialize();
        return 1;
    }

    // 解码动画
    if (!DecodeWebPAnimation(StartupMov.c_str()))
    {
        MessageBox(nullptr, "解码 WebP 动画失败", "错误", MB_OK);
        Cleanup();
        DestroyWindow(AnimHwnd);
        CoUninitialize();
        return 1;
    }
    if (TotalFrames == 0)
    {
        MessageBox(nullptr, "动画无帧数据", "错误", MB_OK);
        Cleanup();
        DestroyWindow(AnimHwnd);
        CoUninitialize();
        return 1;
    }

    // 显示第一帧
    CurrentFrame = 0;
    if (!SwitchToFrame(0))
    {
        MessageBox(nullptr, "无法创建第一帧位图", "错误", MB_OK);
        Cleanup();
        DestroyWindow(AnimHwnd);
        CoUninitialize();
        return 1;
    }

    ShowWindow(AnimHwnd, SW_SHOW);
    UpdateWindow(AnimHwnd);
    RenderFrame();

    // 记录开始时间
    AnimStartTime = timeGetTime();

    FrameTimerId = timeSetEvent(15, 1, FrameTimerProc, 0, TIME_PERIODIC);
    if (!FrameTimerId)
    {
        MessageBox(nullptr, "定时器创建失败", "错误", MB_OK);
        Cleanup();
        DestroyWindow(AnimHwnd);
        CoUninitialize();
        return 1;
    }

    // 保留备用超时退出（防卡死）
    SetTimer(AnimHwnd, 1, 4000, nullptr); // 4秒后强制退出

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (FrameTimerId)
    {
        timeKillEvent(FrameTimerId);
        FrameTimerId = 0;
    }

    Cleanup();
    CoUninitialize();
    return 0;
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
#ifdef  GC_NO_ANIMATION
    ShowWindow(GCHWND, SW_SHOWDEFAULT);
    return 0;
#endif // GC_NO_ANIMATION
    RenerStartupAnimation(GCInstance);

	while (!IsWindowInitSuccessful)
		Sleep(10);

    ShowWindow(GCHWND, SW_SHOWDEFAULT);

    RECT rect{};
    GetWindowRect(GCHWND, &rect);

    PrintLog("[StartAnim] 窗口开始移动", PL_INFO);
    const int x = (ScreenWidth - WindowWidth) / 2;
    SetWindowPos(GCHWND, HWND_TOP, x, ScreenHeigth + 30, WindowWidth, WindowHeight, SWP_NOMOVE);
    WindowOpenAnimate(GCHWND, { x, (ScreenHeigth - WindowHeight) / 2 }, 0.3f);

    SetForegroundWindow(GCHWND);
    PrintLog("[StartAnim] 动画播放完成！", PL_INFO);
    return 0;
}