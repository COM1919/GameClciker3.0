#pragma once

#define _WINSOCKAPI_

// 支持WIN7，防止使用WIN7不支持的API
#define _WIN32_WINNT   0x0601
#define WINVER         0x0601

#define byte win_byte_override
#include <windows.h>
#include <wincodec.h>
#undef byte
#include <iostream>
#include <mmsystem.h>
#include <math.h>
#include <io.h>
#include <shlobj.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <chrono>
#include <time.h>
#include <sstream>
#include <fstream>
#include <thread>
#include <array>
#include <codecvt>
#include <future>
#include <comdef.h>
#include <map>
#include <algorithm>
#include <deque>
#include <queue>
#include <shared_mutex>
#include <filesystem>

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "ImGui/imconfig.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imstb_rectpack.h"
#include "ImGui/imstb_textedit.h"
#include "ImGui/imstb_truetype.h"
#include "ImGui/implot.h"
#include "ImGui/implot_internal.h"
#include "ImGui/imgui_range_slider.h"
#include "ImGui/implot3d.h"
#include "ImGui/imgui_stdlib.h"
#include "ImGui/TextEditor/TextEditor.h"

#include "json/picojson.h"

//#include "ImGui/Icons/fa.h"
//#include "ImGui/Icons/IconsFontAwesome6.h"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <wininet.h>
#pragma comment(lib, "winmm.lib")

#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

#include <d2d1.h>
#include <cstdlib>
#include <cstdio>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")

using ::bind;
using namespace std;
namespace fs = std::filesystem;
