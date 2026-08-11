#pragma once
#include "MainClick.h"
#include "ShareCode.h"
#include "maths.h"
#include "resource.h"
#include "MouseSound.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

using namespace ImGui;

struct GuiData
{
    ID3D11Device*            pd3dDevice = nullptr;
    ID3D11DeviceContext*     pd3dDeviceContext = nullptr;
    IDXGISwapChain*          pSwapChain = nullptr;
    bool                     SwapChainOccluded = false;
    UINT                     ResizeWidth = 0;
    UINT                     ResizeHeight = 0;
    ID3D11RenderTargetView*  mainRenderTargetView = nullptr;
    ID3D11SamplerState*      pPointSampler = nullptr;
    ID3D11SamplerState*      pLinearSampler = nullptr;

}MainGuiData;

// LoatTexture函数需要的，主线程渲染使用
ID3D11SamplerState* LT_SamplerState = nullptr;

ImFont* Fonts;
ImGuiToggleFlags ToggleFlags = ImGuiToggleFlags_Animated | ImGuiToggleFlags_Bordered | ImGuiToggleFlags_Shadowed | ImGuiToggleFlags_Green;
ImVec4 clear_color = (ImVec4)ImColor(14, 14, 14, 255);

int CurrentTab = 1;
int LastTab = 1;
int NextTab = 1;

ImVec4* GuiColor;
ImGuiContext* GCCtx;

WNDCLASSEX wcex{};

enum Tab
{
    Hub = 1,
    Info = 2,
    Setting = 3,
    Script = 4,
    Other = 5,
    About = 6
};

bool CreateDeviceD3D(HWND hWnd);
void CreateSamplers();
static void SetSamplerCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

// GC.cpp定义
void RenderGui(ImGuiIO& io);
int GCMainLoop();
DWORD WINAPI SystemMsgHookLoop(LPVOID lpParam);
DWORD WINAPI RecordsPCRate(LPVOID lpParam);
DWORD WINAPI InitGC(LPVOID lpParam);

// 本文件定义
DWORD WINAPI WindowProcessMessagesThread(LPVOID lpParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ReleasGui(WNDCLASSEX wcex, GuiData* gui);
void TabsText(const char* Text);
auto GetGuiColor(ImGuiStyle& Style, float alpha = 1.0f);
auto GetGuiStyle(ImGuiStyle& Style);
bool PopTip(string msg, float timeout, UINT type);
static int GetKeyPressed();
void InitializationGui(ImGuiIO& imio);
void RenderControl();
bool BeginPopupWindow(const char* name, bool* p_open = NULL, ImVec2 size = ImVec2(720, 420));
void EndPopupWindow();
void NoticeFlag(const char* msg, UINT type = 0);
bool InputKey(const char* label, int* key, const char* press_tip);
void RenderLoadingAnimation(float speed = 6.5f, float radius = 25.0f, float thickness = 4.0f, ImU32 color = IM_COL32(240, 240, 240, 200));
void AttachTooltip(const char* desc, float delay = 0.1f, float maxwidth = 0.5f);
int MessageBoxEx(const char* message, const char* title, map<string, UINT> button, map<string, ImVec4> colors);
int MessageBox(const char* message, const char* title, UINT type);
void BeginCard(const char* label, ImVec2 size = ImVec2(0, 0), ImGuiChildFlags ChildFlags = ImGuiChildFlags_Borders, ImGuiWindowFlags flag = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
void DrawGlowBorder(ImVec2 min, ImVec2 max, ImU32 color, float thickness = 3.0f);
void DrawSpiltLine(float lenght, ImVec4 color, float thickness = 1.0f, float SpitlSize = 200);
void DrawProgressBar(const char* label, float progress, ImVec2 size, ImVec4 Color, ImVec4 bgColor, float thickness = 5.0f);
//bool ClipCoveredRenderRect(HWND hwnd);
static Texture2D LoadTexture(unsigned char* data, int& image_width, int& image_height, int* w, int* h, const char* id);
Texture2D LoadTextureFromFile(const string& filename, bool CacheDisabled = false, int* w = NULL, int* h = NULL);
Texture2D LoadTextureFromMemory(const unsigned char* buffer, int len, const char* id, bool CacheDisabled = false, int* w = NULL, int* h = NULL);
void ClearTextureCache();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct MarkdownRenderClass : public imgui_md
{
    ImFont* get_font() const override
    {
        if (m_is_table_header)
        {
            SetWindowFontScale(1.4f);
            return GetIO().Fonts->Fonts[0];
        }

        switch (m_hlevel)
        {
        case 0:
        {
            if (m_is_strong)
                SetWindowFontScale(1.2f);
            else
                SetWindowFontScale(1.1f);
            return GetIO().Fonts->Fonts[0];
        }
        case 1:
        {
            SetWindowFontScale(1.1f);
            return GetIO().Fonts->Fonts[0];
        }
        default:
        {
            return GetIO().Fonts->Fonts[0];
        }
        }
    };

    void open_url() const override
    {
        ShellExecute(NULL, "open", m_href.c_str(), NULL, NULL, SW_SHOW);
    }
}MarkdownRender;

struct BeginCard_element
{
    ImVec4 t;
    ImVec4 b;
};
static ImVec4 EndCardColor;
void BeginCard(const char* label, ImVec2 size, ImGuiChildFlags ChildFlags, ImGuiWindowFlags WindowFlags)
{
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
    BeginChild(label, size, ChildFlags, WindowFlags);
    PopStyleVar();
    ImGuiID id = GetID(label);
    static map <ImGuiID, BeginCard_element> anim;
    auto it_anim = anim.find(id);
    if (it_anim == anim.end())
    {
        anim.insert({ id, { ColorConvertU32ToFloat4(GetColorU32(ImGuiCol_Text))} });
        it_anim = anim.find(id);
    }
    float speed = 0.1f * (1.0f - GetIO().DeltaTime);
    
    bool IsHovered = IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    it_anim->second.t = ImLerp(it_anim->second.t, IsHovered ? GuiColor[ImGuiCol_CheckMark] : GuiColor[ImGuiCol_Text], speed);
    it_anim->second.b = ImLerp(it_anim->second.b, IsHovered ? GuiColor[ImGuiCol_CheckMark] : ImVec4(0, 0, 0, 200), speed);
    EndCardColor = it_anim->second.b;
    if (IsHovered)
        PushStyleColor(ImGuiCol_Text, it_anim->second.t);
    if (strlen(label) < 2 || (label[0] != '#' && label[1] != '#'))
        Text(label);
    if (IsHovered)
        PopStyleColor();
}

void EndCard()
{
    EndChild();
    ImU32 col = IM_COL32(0, 0, 0, 90);
    DrawGlowBorder(GetItemRectMin(), GetItemRectMax(), ColorConvertFloat4ToU32(EndCardColor), 1.4f);
}

void DrawGlowBorder(ImVec2 min, ImVec2 max, ImU32 color, float thickness)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    ImDrawList* draw_list = GetWindowDrawList();

    // 绘制多层半透明边框，从内到外逐渐变淡
    const int count = thickness * 10;
    for (int i = 0; i < count; i++)
    {
        float alpha = 1.0f - (static_cast<float>(i + 1) / count); // 透明度递减
        float offset = i * 0.1f; // 向外偏移

        ImU32 GlowColor = GetColorU32(
            IM_COL32(
                (color >> IM_COL32_R_SHIFT) & 0xFF,
                (color >> IM_COL32_G_SHIFT) & 0xFF,
                (color >> IM_COL32_B_SHIFT) & 0xFF,
                int(alpha * 200) // 适当加大透明度（255 to 200）
            )
        );

        draw_list->AddRect(
            ImVec2(min.x - offset, min.y - offset),
            ImVec2(max.x + offset, max.y + offset),
            GlowColor,
            style.ChildRounding, // 圆角
            0.0f, 0.1f
        );
    }
}

void RenderLoadingAnimation(float speed, float radius, float thickness, ImU32 color)
{
    // 获取当前窗口的位置和大小（上一层 Begin 的窗口）
    ImVec2 ParentPos = GetWindowPos();
    ImVec2 parentSize = GetWindowSize();
    ImVec2 ChildSize = ImVec2(radius * 2.0f + thickness * 2.0f, radius * 2.0f + thickness * 2.0f);
    // 使它在父窗口中心
    ImVec2 ChildPos = ImVec2(
        ParentPos.x + (parentSize.x - ChildSize.x) * 0.5f,
        ParentPos.y + (parentSize.y - ChildSize.y) * 0.5f
    );
    ImVec2 RelativePos = ImVec2(
        ChildPos.x - ParentPos.x,
        ChildPos.y - ParentPos.y
    );
    SetCursorPos(RelativePos);
    SetCursorPos(ImVec2((parentSize.x - ChildSize.x) * 0.5f, (parentSize.y - ChildSize.y) * 0.5f));

    BeginChild("##LoadingAnimationChild", ChildSize, false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    {
        // 获取子窗口的绘制列表和几何信息
        ImDrawList* drawList = GetWindowDrawList();
        ImVec2 windowPos = GetWindowPos();
        ImVec2 windowSize = GetWindowSize();
        ImVec2 center = ImVec2(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);

        // 基于时间计算旋转角度和弧长
        float time = GetTime();
        float angle = time * speed; // 旋转角度（弧度）
        // 弧长在 [1.0, 5.0] 弧度之间正弦变化，产生伸缩效果
        float span = 2.0f + sinf(time * 3.0f) * 1.5f; // 范围约 0.5 ~ 3.5 弧度

        // 计算圆弧的起始和结束角度（逆时针方向）
        float startAngle = angle;
        float endAngle = angle + span;

        // 绘制圆弧（分段数 40 保证平滑）
        drawList->PathArcTo(center, radius, startAngle, endAngle, 40);
        drawList->PathStroke(color, ImDrawFlags_RoundCornersAll, thickness);
    }
    EndChild();
}

int ImGuiKeyToVirtualKey(ImGuiKey key)
{
    // 字母键 A-Z
    if (key >= ImGuiKey_A && key <= ImGuiKey_Z)
        return key - ImGuiKey_A + 'A';
    // 数字键 0-9
    else if (key >= ImGuiKey_0 && key <= ImGuiKey_1)
        return key - ImGuiKey_0 + '0';
    // 功能键 F1-F24
    else if (key >= ImGuiKey_F1 && key <= ImGuiKey_F24)
        return key - ImGuiKey_F1 + VK_F1;
    // 小键盘数字
    else if (key >= ImGuiKey_Keypad0 && key <= ImGuiKey_Keypad9)
        return key - ImGuiKey_Keypad0 + VK_NUMPAD0;
    // 鼠标键
    else if (key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseWheelY)
        return key - ImGuiKey_MouseLeft + 1;
    switch (key)
    {
        // 方向键
    case ImGuiKey_UpArrow:    return VK_UP;
    case ImGuiKey_DownArrow:  return VK_DOWN;
    case ImGuiKey_LeftArrow:  return VK_LEFT;
    case ImGuiKey_RightArrow: return VK_RIGHT;

        // 特殊键
    case ImGuiKey_Escape:       return VK_ESCAPE;
    case ImGuiKey_Enter:        return VK_RETURN;
    case ImGuiKey_Tab:          return VK_TAB;
    case ImGuiKey_Backspace:    return VK_BACK;
    case ImGuiKey_Insert:       return VK_INSERT;
    case ImGuiKey_Delete:       return VK_DELETE;
    case ImGuiKey_Home:         return VK_HOME;
    case ImGuiKey_End:          return VK_END;
    case ImGuiKey_PageUp:       return VK_PRIOR;
    case ImGuiKey_PageDown:     return VK_NEXT;
    case ImGuiKey_Space:        return VK_SPACE;
    case ImGuiKey_CapsLock:     return VK_CAPITAL;
    case ImGuiKey_ScrollLock:   return VK_SCROLL;
    case ImGuiKey_NumLock:      return VK_NUMLOCK;
    case ImGuiKey_PrintScreen:  return VK_SNAPSHOT;
    case ImGuiKey_Pause:        return VK_PAUSE;
    case ImGuiKey_KeypadEnter:  return VK_RETURN; // 小键盘 Enter 通常映射到 VK_RETURN

        // 修饰键
    case ImGuiKey_LeftCtrl:   return VK_LCONTROL;
    case ImGuiKey_LeftShift:  return VK_LSHIFT;
    case ImGuiKey_LeftAlt:    return VK_LMENU;    // Alt 键在 Windows VK 中对应 MENU
    case ImGuiKey_LeftSuper:  return VK_LWIN;
    case ImGuiKey_RightCtrl:  return VK_RCONTROL;
    case ImGuiKey_RightShift: return VK_RSHIFT;
    case ImGuiKey_RightAlt:   return VK_RMENU;
    case ImGuiKey_RightSuper: return VK_RWIN;
    case ImGuiKey_Menu:       return VK_APPS;     // Application Key

        // 符号
    case ImGuiKey_KeypadDecimal: return VK_DECIMAL;
    case ImGuiKey_KeypadDivide:  return VK_DIVIDE;
    case ImGuiKey_KeypadMultiply: return VK_MULTIPLY;
    case ImGuiKey_KeypadSubtract: return VK_SUBTRACT;
    case ImGuiKey_KeypadAdd:      return VK_ADD;

        // 符号键 (注意：这通常依赖于键盘布局，以下为美式键盘映射)
    case ImGuiKey_Apostrophe: return VK_OEM_7;   // ' "
    case ImGuiKey_Comma:      return VK_OEM_COMMA; // , <
    case ImGuiKey_Minus:      return VK_OEM_MINUS; // - _
    case ImGuiKey_Period:     return VK_OEM_PERIOD; // . >
    case ImGuiKey_Slash:      return VK_OEM_2;    // / ?
    case ImGuiKey_Semicolon:  return VK_OEM_1;    // ; :
    case ImGuiKey_Equal:      return VK_OEM_PLUS; // = +
    case ImGuiKey_LeftBracket:  return VK_OEM_4;  // [ {
    case ImGuiKey_Backslash:    return VK_OEM_5;  // \ |
    case ImGuiKey_RightBracket: return VK_OEM_6;  // ] }
    case ImGuiKey_GraveAccent:  return VK_OEM_3;  // ` ~

    default: return 0; 
    }
}

string GetVkeyName(int vk)
{
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9'))
        return string(1, (char)vk);

    switch (vk)
    {
        // 鼠标按键
    case VK_LBUTTON: return "Mouse Left";
    case VK_RBUTTON: return "Mouse Right";
    case VK_MBUTTON: return "Mouse Middle";
    case VK_XBUTTON1: return "Mouse Forward";
    case VK_XBUTTON2: return "Mouse Backward";

        // 修饰键
    case VK_SHIFT:   return "Shift";      // 通用的 Shift
    case VK_LSHIFT:  return "Left Shift";
    case VK_RSHIFT:  return "Right Shift";
    case VK_CONTROL: return "Ctrl";    // 通用的 Ctrl
    case VK_LCONTROL:return "Left Ctrl";
    case VK_RCONTROL:return "Right Ctrl";
    case VK_MENU:    return "Alt";        // 通用的 Alt
    case VK_LMENU:   return "Left Alt";
    case VK_RMENU:   return "Right Alt";
    case VK_LWIN:    return "Left Win";
    case VK_RWIN:    return "Right Win";
    case VK_APPS:    return "Apps Key";

        // 功能键
    case VK_RETURN:  return "Enter";
    case VK_SPACE:   return "Space";
    case VK_BACK:    return "Backspace";
    case VK_TAB:     return "Tab";
    case VK_ESCAPE:  return "Escape";
    case VK_CAPITAL: return "Caps Lock";
    case VK_NUMLOCK: return "Num Lock";
    case VK_SCROLL:  return "Scroll Lock";
    case VK_PAUSE:   return "Pause";
    case VK_INSERT:  return "Insert";
    case VK_DELETE:  return "Delete";
    case VK_HOME:    return "Home";
    case VK_END:     return "End";
    case VK_PRIOR:   return "Page Up";
    case VK_NEXT:    return "Page Down";

        // 方向键
    case VK_UP:      return "Up";
    case VK_DOWN:    return "Down";
    case VK_LEFT:    return "Left";
    case VK_RIGHT:   return "Right";

        // 小键盘
    case VK_ADD:      return "Numpad +";
    case VK_SUBTRACT: return "Numpad -";
    case VK_MULTIPLY: return "Numpad *";
    case VK_DIVIDE:   return "Numpad /";
    case VK_DECIMAL:  return "Numpad .";
        // 小键盘数字通常与主键盘数字冲突，这里区分显示
    case VK_NUMPAD0:  return "Numpad 0";
    case VK_NUMPAD1:  return "Numpad 1";
    case VK_NUMPAD2:  return "Numpad 2";
    case VK_NUMPAD3:  return "Numpad 3";
    case VK_NUMPAD4:  return "Numpad 4";
    case VK_NUMPAD5:  return "Numpad 5";
    case VK_NUMPAD6:  return "Numpad 6";
    case VK_NUMPAD7:  return "Numpad 7";
    case VK_NUMPAD8:  return "Numpad 8";
    case VK_NUMPAD9:  return "Numpad 9";

        // OEM 键 (符号键位置)
    case VK_OEM_1:    return "Semicolon";
    case VK_OEM_2:    return "Slash";
    case VK_OEM_3:    return "Tilde";
    case VK_OEM_4:    return "L Bracket";
    case VK_OEM_5:    return "Backslash";
    case VK_OEM_6:    return "R Bracket";
    case VK_OEM_7:    return "Apostrophe";
    case VK_OEM_PLUS: return "Plus";
    case VK_OEM_MINUS:return "Minus";
    case VK_OEM_COMMA:return "Comma";
    case VK_OEM_PERIOD:return "Period";
    }

    // 3. 功能键 F1-F24
    if (vk >= VK_F1 && vk <= VK_F24)
    {
        char buf[8];
        sprintf_s(buf, "F%d", vk - VK_F1 + 1);
        return std::string(buf);
    }
    return "None";
}

struct InputKey_element
{
    ImVec4 B;
};
bool InputKey(const char* label, int* key, const char* press_tip)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 AnimPos = CalcWidgetsAnimVert(id);

    const float width = g.NextItemData.Width;
    const ImVec2 size((width <= 0) ? 48 : width, 24);
    const ImRect rect(AnimPos, ImVec2(AnimPos.x + size.x, AnimPos.y + size.y));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool value_changed = false;
    int k = *key;

    string buf_display;
    string active_key = config.UseChinese ? u8"无" : "None";
    if (*key == ImGuiKey_Escape)
        *key = 0;
  
    if (!config.UseChinese)
        goto defa;
    switch (*key)
        {
        case 5: active_key = u8"鼠标后侧键"; break;
        case 6: active_key = u8"鼠标前侧键"; break;
        case 1: active_key = u8"鼠标左键";   break;
        case 2: active_key = u8"鼠标右键";   break;
        case 3: active_key = u8"鼠标中键";   break;
        case 4: active_key = u8"滚轮键";     break;
        case 0: active_key = u8"无";         break;
        default:
        defa:
            string vkey_name = GetVkeyName(*key);
            active_key = (char*)vkey_name.c_str();
            break;
        }

    if (g.ActiveId != id)
        buf_display = active_key;
    else if (g.ActiveId == id)
       buf_display = press_tip;

    const bool hovered_key = ItemHoverable(rect, id, g.LastItemData.ItemFlags);
    if (hovered_key && io.MouseClicked[0])
    {
        if (g.ActiveId != id)
        {
            // Start edition 使用自己的GetKeyUp后，就不需要这个了
            //memset(io.MouseDown, 0, sizeof(io.MouseDown));
            //memset(io.KeysDown, 0, sizeof(io.KeysDown));
            *key = 0;
        }
        SetActiveID(id, window);
        FocusWindow(window);
    }
    else if (io.MouseClicked[0])
    {
        // Release focus when we click outside
        if (g.ActiveId == id)
            ClearActiveID();
    }

    if (g.ActiveId == id)
    {
        for (unsigned int i = 3; i < 255; i++)
        {
            if (GetKeyUp(i))
            {
                *key = i;
                ClearActiveID();
                break;
            }
        }
    }

    static std::map <ImGuiID, InputKey_element> anim;
    auto it_anim = anim.find(id);
    if (it_anim == anim.end())
    {
        anim.insert({ id, { ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_Button))} });
        it_anim = anim.find(id);
    }
    float speed = 0.1f * (1.0f - ImGui::GetIO().DeltaTime);

    ImU32 tB;
    tB = (g.ActiveId == id) ? ImGui::GetColorU32(ImGuiCol_FrameBgActive) : hovered_key ? ImGui::GetColorU32(ImGuiCol_FrameBgHovered) : ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImVec4 tB_vec = ImGui::ColorConvertU32ToFloat4(tB);
    it_anim->second.B = ImLerp(it_anim->second.B, tB_vec, speed);

    // Render
    window->DrawList->AddRectFilled(rect.Min, ImVec2(rect.Max.x - 4, rect.Max.y), ImGui::ColorConvertFloat4ToU32(it_anim->second.B), 3.0f);
    ImGui::RenderTextClipped(ImVec2(rect.Min.x, rect.Min.y - 1), ImVec2(rect.Max.x - 4, rect.Max.y - 1), buf_display.c_str(), NULL, NULL, ImVec2(0.5f, 0.5f));

    return value_changed;
}

struct PopupWindow_element
{
    float pos;
    float alpha;
};
bool BeginPopupWindow(const char* name, bool* p_open, ImVec2 size)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiIO& io = GetIO();
    const ImGuiID id = GetID(name);

    const bool IsClose = p_open && !*p_open;
    static map <ImGuiID, PopupWindow_element> anim;
    static map <ImGuiID, bool> end_anim;
    if (IsClose && !end_anim[id])
        return false;
    
    const ImVec2 WindowPos((io.DisplaySize.x - size.x) / 2, (io.DisplaySize.y - size.y) / 2);
    const int TagerPos = IsClose ? (io.DisplaySize.y - (size.y * 0.2)) : WindowPos.y;
    const int TagerAlpha = IsClose ? 0 : 255;

    auto it_anim = anim.find(id);
    if (it_anim == anim.end())
    {
        anim.insert({ id, { io.DisplaySize.y, 0.0f } });
        it_anim = anim.find(id);
    }
    float speed = 0.25f * (1.0f - GetIO().DeltaTime);

    it_anim->second.pos = ImLerp(it_anim->second.pos, (float)TagerPos, speed);
    it_anim->second.alpha = ImLerp(it_anim->second.alpha, (float)TagerAlpha, speed);

    const ImVec2 RenderPos(WindowPos.x, it_anim->second.pos);

    // 背景阴影渲染
    {
        ImDrawList* draw_list = GetForegroundDrawList();
        const int alpha = (it_anim->second.alpha / 255.0f) * 100; // 将0-255映射到0-100范围
        const ImRect WindowRect = ImRect(RenderPos, ImVec2(RenderPos.x + size.x, RenderPos.y + size.y));
        const ImU32 BgColor = GetColorU32((ImVec4)ImColor(5, 5, 5, alpha));
        // 弹窗上方
        draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(io.DisplaySize.x, WindowRect.Min.y), BgColor);
        // 弹窗下方
        draw_list->AddRectFilled(ImVec2(0, WindowRect.Max.y), io.DisplaySize, BgColor);
        // 弹窗左边
        draw_list->AddRectFilled(ImVec2(0, WindowRect.Min.y), ImVec2(WindowRect.Min.x, WindowRect.Max.y), BgColor);
        // 弹窗右边
        draw_list->AddRectFilled(ImVec2(WindowRect.Max.x, WindowRect.Min.y), ImVec2(io.DisplaySize.x, WindowRect.Max.y), BgColor);
    }

    ImVec4 WindowColor = style.Colors[ImGuiCol_WindowBg];
    WindowColor.w = it_anim->second.alpha / 255.0f;
    PushStyleColor(ImGuiCol_WindowBg, WindowColor);
    ImVec4 BorderColor = style.Colors[ImGuiCol_Border];
    BorderColor.w = it_anim->second.alpha / 255.0f;
    PushStyleColor(ImGuiCol_Border, BorderColor);
    PushStyleVar(ImGuiStyleVar_WindowRounding, 6);

    static const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    Begin(name, p_open, flags);
    if (p_open && !*p_open)
        end_anim[id] = true;

    PopStyleColor(2);
    PopStyleVar();//ImGuiStyleVar_WindowRounding

    SetWindowPos(RenderPos);
    SetWindowSize(size);

    if (IsClose && abs(it_anim->second.pos - TagerPos) < 35.0f)
    {
        anim.erase(it_anim);
        end_anim[id] = false;
    }
    return true;
}

void EndPopupWindow()
{
    End();
}

#define NF_WARNING 0
#define NF_INFO    1
#define NF_ERROR   2
void NoticeFlag(const char* msg, UINT type)
{
    BeginTooltip();
    {
        const Texture2D texture = LoadTextureFromFile((type == NF_WARNING) ? Pics_TipWarning : ((type == NF_INFO) ? Pics_TipInfo : Pics_TipError));
        Image((ImTextureID)texture, ImVec2(24, 24));
        SameLine();
        Text(msg);
    }
    EndTooltip();
}

/*==========来自其他库移植代码==========*/
static ImGuiID tooltip_shownid = 0;
static ImGuiID tooltip_hoveredid = 0;
static int tooltip_thisframe = -1;
static bool tooltip_washovered = false;
static float tooltip_time = 0.f;
static float tooltip_lastactive = 0.f;
void AttachTooltip(const char* desc, float delay, float maxwidth)
{
    ImGuiContext* g = GetCurrentContext();
    ImGuiID id = g->CurrentWindow->ID;
    float time = GetTime();
    int thisframe = GetFrameCount();

    if (thisframe != tooltip_thisframe)
    {
        // run once every frame, in the first call
        if (!tooltip_washovered)
        {
            tooltip_time = time;
            tooltip_shownid = 0;
            tooltip_hoveredid = 0;
        }
        tooltip_thisframe = thisframe;
        tooltip_washovered = false;
    }

    if (IsItemHovered())
    {
        // If no tooltip is being shown and the mouse moves from one tooltip element
        // to another, this is the same as if it moved from a zone without any
        // tooltip elements.
        if (id != tooltip_hoveredid && tooltip_shownid == 0)
            tooltip_time = time;

        if (tooltip_lastactive != 0.f)
            delay = fmin(delay, fmax(tooltip_time - tooltip_lastactive, 0.f));

        tooltip_washovered = true;
        tooltip_hoveredid = id;

        if (time - tooltip_time > delay)
        {
            tooltip_shownid = id;
            tooltip_lastactive = time;
            BeginTooltip();
            static const Texture2D texture = LoadTextureFromFile(Pics_TipInfo);
            Image((ImTextureID)texture, ImVec2(24, 24));
            SameLine();
;           Text(desc);
            EndTooltip();
        }
    }
}
/*=================================*/
void DrawSpiltLine(float lenght, ImVec4 color, float thickness, float SpitlSize)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImDrawList* draw_list = window->DrawList;
    ImVec2 pos(window->DC.CursorPos);

    Dummy(ImVec2(1, SpitlSize));
    pos.y += SpitlSize / 2;
    draw_list->AddLine(pos, ImVec2(pos.x + lenght, pos.y), GetColorU32(color), thickness);
}

void RenderControl()
{
    //=======================进度条处理===========================

    if (pb_message.open)
    {
        ImDrawList* draw_list = GetWindowDrawList();
        draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(800, 468), GetColorU32((ImVec4)ImColor(5, 5, 5, 100)));
        const ImGuiIO& io = GetIO();
        const ImVec2 size = ImVec2(450, 120);

        SetNextWindowFocus();
        Begin("##progressbar", NULL,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings);
        SetWindowPos(ImVec2((io.DisplaySize.x - size.x) / 2, (io.DisplaySize.y - size.y) / 2));
        SetWindowSize(size);

        static double last_byte = 0;
        static double speed = 0;

        SetWindowFontScale(1.5f);
        Text(config.UseChinese ? u8"下载任务" : "Download Task");
        SetWindowFontScale(1.0f);

        Text(pb_message.title.c_str());
        SetNextItemWidth(420);

        const double RemainingTime = (speed == 0.0) ? 0.0 : (pb_message.maxmb - pb_message.mb) / max(speed, 1.0);

        ProgressBar(float(pb_message.mb / max(pb_message.maxmb, 1.0)), ImVec2(430, 20));
        Text(config.UseChinese ? u8"已下载:%.2f MB / %.2f MB - %.2f MB/s  剩余时间:%s 秒" : "Downloaded:%.2f MB / %.2f MB - %.2f MB/s  Time:%s seconds",
            pb_message.mb, pb_message.maxmb, speed, (speed <= 0.0) ? u8"无" : to_string((int)RemainingTime).c_str());

        static ULONGLONG last_time = GetTickCount64() + 1000;

        if (last_time < GetTickCount64())
        {
            speed = (last_byte == 0.0) ? 0.0 : max(pb_message.mb - last_byte, 0.0);
            last_byte = pb_message.mb;
            last_time = GetTickCount64() + 1000;
        }

        SetCursorPos(ImVec2(size.x - 12, size.y - 12));
        if (Button(config.UseChinese ? u8"取消" : "Cancel", ImVec2(80, 30)))
        {
            pb_message.ret_code = 1;
            pb_message.open = false;
        }

        End();
        // 如果在下载文件那么就不不渲染其他的了。
        return;
    }
#ifndef GC_NO_MSGBOX
    //=======================MessageBox处理===========================
    {
        // 读取锁
        shared_lock<shared_mutex> lock(MBChangeMutex);
        if (mb_message.open)
        {
            if (mb_message.type == -1)
            {
                int ret = MessageBoxEx(mb_message.message.c_str(), mb_message.title.c_str(), mb_message.button, mb_message.colors);
                if (ret != -1)
                {
                    lock.unlock();
                    // 写入锁
                    lock_guard<shared_mutex> lock2(MBChangeMutex);
                    mb_message.ret_code = ret;
                    mb_message.open = false;
                }
            }
            else
            {
                int ret = MessageBox(mb_message.message.c_str(), mb_message.title.c_str(), mb_message.type);
                if (ret != -1)
                {
                    lock.unlock();
                    // 写入锁
                    lock_guard<shared_mutex> lock2(MBChangeMutex);
                    mb_message.ret_code = ret;
                    mb_message.open = false;
                }
            }
        }
    }

#else
    if (mb_message.size() > 0)
        mb_message.clear();
#endif // GC_NO_MSGBOX
    //=======================提示框===========================
    {
        // 修复：原代码 for 遍历在锁外、erase 在锁内，且写端 RenderTip 完全无锁。
        // 任意线程 push_back 触发 vector 扩容 → 正在遍历的迭代器失效 → 0xC0000005。
        // 现在整个遍历+删除在写锁内完成，并用迭代器 erase 代替索引 erase。
        lock_guard<shared_mutex> lock(PTChangeMutex);
        for (auto it = pt_message.begin(); it != pt_message.end();)
        {
            if (!PopTip(it->message, it->timeout, it->type))
                it = pt_message.erase(it);
            else
                ++it;
        }
    }
}

struct PopTipState
{
    int      seq;        // 创建顺序编号，用于保证高度累加顺序正确
    int      height;     // 窗口高度
    bool     alive;      // 是否存活（处于移出态时为 false）
    int      endtime;    // 计时器
    ImVec2   pos;        // 当前实时位置
    float    alpha;      // 当前透明度
    float    targetY;    // 锁定的目标 Y 坐标（防止移出时被其它提示顶下去）
};

bool PopTip(string msg, float timeout, UINT type)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(msg.c_str()); // 保持原 ID 生成方式，兼容每帧调用
    const ImGuiIO& io = GetIO();
    const int count_end = RenderFps * timeout;
    constexpr unsigned int ImageSize = 32;      // 图标X大小
    constexpr unsigned int MaxTextLength = 48 * 3;  // 最大文本长度

    // 统一状态管理容器
    static std::unordered_map<ImGuiID, PopTipState> g_popTipStates;
    static int s_nextSeq = 0;        // 全局递增序号，用于保证层叠顺序

    // 如果文本过长，就开始分割为多行
    msg = UTF8ToString(msg);
    size_t text_length = msg.size();
    if (text_length > MaxTextLength)
    {
        // 处理极端情况，文本太长
        if (text_length > 256)
        {
            msg = msg.substr(0, 256);
            msg.append("...");
            text_length = msg.size(); // 更新长度
        }
        // 分割为多行
        for (unsigned int index = 0; index < text_length; index += MaxTextLength)
        {
            size_t pos = msg.find("\n", index);
            size_t next_break = index + MaxTextLength;
            // 没找到\n 或者 \n在当前切割点之后时，插入换行
            if (pos == string::npos || pos >= next_break)
            {
                msg.insert(next_break, "\n");
                text_length++; // 插入了换行符，长度要 +1，防止越界
            }
        }
    }
    msg = StringToUTF8(msg);

    ImVec2 MsgSize = CalcTextSize(msg.c_str());
    MsgSize.x += ImageSize;
    ImVec2 WindowSize(MsgSize.x + style.FramePadding.x * 2, style.FramePadding.y * 4 + MsgSize.y);

    // 获取或创建状态
    auto it_state = g_popTipStates.find(id);
    if (it_state == g_popTipStates.end())
    {
        // 新建提示状态
        PopTipState newState;
        newState.seq = s_nextSeq++; // 分配单调递增的序号
        newState.height = (int)WindowSize.y;
        newState.alive = true;
        newState.endtime = 0;
        newState.pos = ImVec2(io.DisplaySize.x + 32.0f, io.DisplaySize.y - WindowSize.y - 10.0f);
        newState.alpha = 0.0f;
        newState.targetY = io.DisplaySize.y - WindowSize.y - 10.0f;

        g_popTipStates[id] = newState;
        it_state = g_popTipStates.find(id);
    }

    PopTipState& state = it_state->second;
    state.height = (int)WindowSize.y; // 每帧更新高度，防止窗口尺寸变化

    // 计时逻辑
    if (state.alive)
    {
        if (state.endtime < count_end)
            state.endtime++;
        else
            state.alive = false; // 超时，进入移出态
    }

    // 核心治本逻辑：收集所有存活的项，并按创建序号(seq)排序，保证累加顺序绝对正确
    std::vector<std::pair<int, int>> activeTips; // {seq, height}
    for (const auto& kv : g_popTipStates)
    {
        if (kv.second.alive)
            activeTips.push_back({ kv.second.seq, kv.second.height });
    }
    std::sort(activeTips.begin(), activeTips.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
        });

    unsigned int LayersHightSum = 0;
    for (const auto& p : activeTips)
    {
        if (p.first == state.seq)
            break; // 碰到当前项自己，停止累加
        LayersHightSum += p.second;
    }

    // 计算目标位置
    float TargetX = state.alive ? (io.DisplaySize.x - WindowSize.x - 10.0f) : (io.DisplaySize.x + 20.0f);

    // 只有存活时才更新目标 Y。一旦进入移出态，Y 坐标锁死，让它笔直向右水平飞出
    if (state.alive)
    {
        state.targetY = io.DisplaySize.y - WindowSize.y - 10.0f - LayersHightSum;
    }

    // 插值速度计算，用 ImClamp 防止 DeltaTime 异常导致插值越界
    float MoveSpeed = ImClamp(0.20f * (1.0f - io.DeltaTime), 0.0f, 1.0f);
    float ColorSpeed = ImClamp(0.15f * (1.0f - io.DeltaTime), 0.0f, 1.0f);

    // X 和 Y 分别插值
    state.pos.x = ImLerp(state.pos.x, TargetX, MoveSpeed);
    state.pos.y = ImLerp(state.pos.y, state.targetY, MoveSpeed);
    state.alpha = ImLerp(state.alpha, state.alive ? 1.0f : 0.0f, ColorSpeed);

    // 绘制部分
    ImVec4 color = ColorConvertU32ToFloat4(GetColorU32((ImVec4)ImColor(30, 30, 30)));
    string title;
    Texture2D image = NULL;
    string TextureFileName;

    switch (type)
    {
    default:
    case PT_INFO:
        title = u8"信息";
        TextureFileName = Pics_TipInfo;
        break;
    case PT_WARNING:
        title = u8"警告";
        TextureFileName = Pics_TipWarning;
        break;
    case PT_ERROR:
        title = u8"错误";
        TextureFileName = Pics_TipError;
        break;
    }

    // 修复：根据 type 类型加载对应的图标，而不是写死 Pics_TipInfo
    image = LoadTextureFromFile(TextureFileName);

    ImDrawList* draw_list = GetForegroundDrawList();

    ImVec4 BgColor = ColorConvertU32ToFloat4(GetColorU32(color));
    BgColor.w = state.alpha;
    // 窗口背景
    draw_list->AddRectFilled(state.pos, ImVec2(
        state.pos.x + WindowSize.x,
        state.pos.y + WindowSize.y),
        GetColorU32(BgColor), 6);

    ImVec4 TextColor = ColorConvertU32ToFloat4(GetColorU32(ImGuiCol_Text));
    TextColor.w = state.alpha;
    // 文字消息
    draw_list->AddText(Fonts, 20, ImVec2(
        state.pos.x + style.FramePadding.x * 2 + ImageSize,
        state.pos.y + (WindowSize.y - MsgSize.y) / 2),
        GetColorU32(TextColor), msg.c_str());

    ImVec2 ImagePos(state.pos.x + style.FramePadding.x,
        state.pos.y + ((WindowSize.y / 2.0f) - (ImageSize / 2.0f)));
    // 图标
    draw_list->AddImage((ImTextureID)image, ImagePos, ImVec2(ImagePos.x + ImageSize, ImagePos.y + ImageSize));

    // 彻底退出后的清理工作
    if (!state.alive && state.pos.x > io.DisplaySize.x - 5.0f)
    {
        // 提示已完全飞出屏幕外且透明度为0，清理状态
        g_popTipStates.erase(id);
        return false;
    }

    return true;
}

void InitializationGui(ImGuiIO& imio)
{
    imio = GetIO();
    // 显式关闭键盘导航（这会禁用 Tab 切换按钮、方向键移动焦点等）
    imio.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    // 显式关闭手柄导航
    imio.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    //imio.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts; //实验性功能
    ImGuiStyle& style = GetStyle(); // 开启抗锯齿
    style.AntiAliasedLines = true; // 注意：在 ImGuiStyle 里，不是 ImGuiIO
    style.AntiAliasedFill = true;
    style.AntiAliasedLinesUseTex = true;

    typedef UINT(WINAPI* GetDpiForSystemFunc)();

    HMODULE hModule = LoadLibrary(TEXT("User32.dll"));
    if (hModule != NULL)
    {
        GetDpiForSystemFunc pGetDpiForSystem = (GetDpiForSystemFunc)GetProcAddress(hModule, "GetDpiForSystem");

        if (pGetDpiForSystem != NULL)
        {
            SystemScale = pGetDpiForSystem() / 96.0f;
            imio.FontGlobalScale = SystemScale;
        }

        FreeLibrary(hModule);
    }

    StyleColorsClassic();

    PrintLog("[Render] 初始化ImplWin32", PL_INFO);
    if (!ImGui_ImplWin32_Init(GCHWND))
    {
        ExceptionHandling(19190, config.UseChinese ? "无法初始化ImplWin32" : "Unable to initialize ImplWin32");
        return;
    }
    PrintLog("[Render] 初始化ImplDX11", PL_INFO);
    if (!ImGui_ImplDX11_Init(MainGuiData.pd3dDevice, MainGuiData.pd3dDeviceContext))
    {
        ExceptionHandling(19191, config.UseChinese ? "无法初始化DX11" : "Unable to initialize DX11");
        return;
    }
    imio.IniFilename = nullptr;

    PrintLog("[Render] 创建字体", PL_INFO);

    ImFontConfig FontConfig;
    FontConfig.SizePixels = 18.0f * SystemScale;
    FontConfig.PixelSnapH = true;
    FontConfig.OversampleH = 1;
    FontConfig.OversampleV = 1;
    Fonts = imio.Fonts->AddFontFromFileTTF(
        FontFile.c_str(),
        18.0f * SystemScale,
        &FontConfig,
        imio.Fonts->GetGlyphRangesChineseFull()  // 中文范围
    );

    // 合并 Font Awesome 图标字体
    ImFontConfig IconConfig;
    IconConfig.MergeMode = true;
    IconConfig.PixelSnapH = true;
    IconConfig.OversampleH = 1;
    IconConfig.OversampleV = 1;
    IconConfig.GlyphOffset.y = 1.0f; // 正数向下移动
    IconConfig.GlyphOffset.x = -1.0f; // 正数向右偏移

    static const ImWchar IconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFont* IconFont = imio.Fonts->AddFontFromMemoryCompressedTTF(
        (void*)FA_compressed_data,
        FA_compressed_size,
        16.0f * SystemScale,
        &IconConfig,
        IconsRanges
    );

    if (!IconFont)
		PrintLog("[Render] 无法加载图标字体", PL_ERROR);

    imio.Fonts->Build();
}

// 返回-1是没有点击按钮
int MessageBox(const char* message, const char* title, UINT type)
{
    map<string, UINT> button;
    map<string, ImVec4> colors;

    switch (type)
    {
        default:
        case MB_OK:
        {
            button[u8"确定"] = IDOK;
            break;
        }
        case MB_OKCANCEL:
        {
            button[u8"确定"] = IDOK;
            button[u8"取消"] = IDCANCEL;
            colors[u8"确定"] = GuiColor[ImGuiCol_ButtonHovered];
            colors[u8"取消"] = GuiColor[ImGuiCol_Button];
            break;
        }
        case MB_YESNO:
        {
            button[u8"是"] = IDYES;
            button[u8"否"] = IDNO;
            colors[u8"是"] = GuiColor[ImGuiCol_ButtonHovered];
            colors[u8"否"] = GuiColor[ImGuiCol_Button];
            break;
        }
    }

    return MessageBoxEx(message, title, button, colors);
}

struct MessageBox_element
{
    ImVec2 size;
    int alpha;
};

// 返回-1是没有点击按钮
int MessageBoxEx(const char* message, const char* title, map<string, UINT> button, map<string, ImVec4> colors)
{
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiIO& io = GetIO();
    const ImGuiID id = window->GetID(message);

    ImVec2 TextSize = CalcTextSize(message);
    ImVec2 TitleSize = CalcTextSize(title);
    ImVec2 ButtonSize(125, 30);

    float PosX = TextSize.x + 26 * 2;
    int VectorSize = button.size();
    int ButtonPosX  = (26 + 125) * VectorSize;
    // 计算窗口大小
    ImVec2 WindowSize((PosX < ButtonPosX) ? ButtonPosX + 26 : PosX, TextSize.y + TitleSize.y + 30 * 3 + 70);

    static map<ImGuiID, int> RetValueList;
    static map<ImGuiID, MessageBox_element> anim;

    if (RetValueList.find(id) == RetValueList.end())
        RetValueList[id] = -1;
    auto it_anim = anim.find(id);
    if (it_anim == anim.end())
    {
        anim.insert({ id, { ImVec2(WindowSize.x - 80.0f, WindowSize.y - 80.0f), 0}});
        it_anim = anim.find(id);
    }
    float AnimSizeSpeed = 0.4f * (1.0f - GetIO().DeltaTime);
    float AnimAlphaSpeed = 0.5f * (1.0f - GetIO().DeltaTime);

    const bool IsExit = RetValueList[id] != -1;
    it_anim->second.size = ImLerp(it_anim->second.size, IsExit ? ImVec2(WindowSize.x - 80.0f, WindowSize.y - 80.0f) : WindowSize, AnimSizeSpeed);
    it_anim->second.alpha = ImLerp(it_anim->second.alpha, IsExit ? 0 : 255, AnimAlphaSpeed);
   
    // 使用动画后的WindowSize，而不是WindowSize变量
    const ImVec2 RenderPos((io.DisplaySize.x - it_anim->second.size.x) / 2, (io.DisplaySize.y - it_anim->second.size.y) / 2);
    const ImVec2 RenderSize = it_anim->second.size;

    /*
    // 背景阴影渲染
    {
        ImDrawList* draw_list = GetForegroundDrawList();
        const int alpha = (it_anim->second.alpha / 255.0f) * 100; // 将0-255映射到0-100范围
        const ImRect WindowRect = ImRect(RenderPos, ImVec2(RenderPos.x + RenderSize.x, RenderPos.y + RenderSize.y));
        const ImU32 BgColor = GetColorU32((ImVec4)ImColor(5, 5, 5, alpha));
        // 弹窗上方
        draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(io.DisplaySize.x, WindowRect.Min.y), BgColor);
        // 弹窗下方
        draw_list->AddRectFilled(ImVec2(0, WindowRect.Max.y), io.DisplaySize, BgColor);
        // 弹窗左边
        draw_list->AddRectFilled(ImVec2(0, WindowRect.Min.y), ImVec2(WindowRect.Min.x, WindowRect.Max.y), BgColor);
        // 弹窗右边
        draw_list->AddRectFilled(ImVec2(WindowRect.Max.x, WindowRect.Min.y), ImVec2(io.DisplaySize.x, WindowRect.Max.y), BgColor);
    }*/

    const float alpha = it_anim->second.alpha / 255.0f;
    PushStyleColor(ImGuiCol_WindowBg, GetColorU32(ImGuiCol_WindowBg, 0)); // 这里不需要，使用下面的弹窗的背景
    PushStyleColor(ImGuiCol_Border, GetColorU32(ImGuiCol_Border, 0));
    PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, alpha * 0.5f));
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 6);

    static bool IsOpen = true;
    if (IsOpen)
    {
        OpenPopup("##messagebox");
        IsOpen = false;
    }
    if (BeginPopupModal("##messagebox", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse))
    {
        SetWindowPos(RenderPos);
        SetWindowSize(RenderSize);

        PopStyleColor(3);
        PopStyleVar(2);

        // 弹窗的背景对比渲染，上下一个黑一个亮，弹窗的阴影
        ImDrawList* draw_list = GetWindowDrawList();
        ImVec2 ScreenPos = GetCursorScreenPos();
        const float TempY = ScreenPos.y + it_anim->second.size.y - 70;
        draw_list->AddRectFilled(ImVec2(ScreenPos.x, TempY), ImVec2(ScreenPos.x + it_anim->second.size.x, ScreenPos.y + it_anim->second.size.y), GetColorU32((ImVec4)ImColor(20, 20, 20, it_anim->second.alpha))); // 下面
        draw_list->AddRectFilled(ScreenPos, ImVec2(ScreenPos.x + it_anim->second.size.x, TempY), GetColorU32((ImVec4)ImColor(28, 28, 28, it_anim->second.alpha)));// 上面

        const float TextAlpha = it_anim->second.alpha / 255.0f;
        
        SetWindowFontScale(1.5f);
        SetCursorPos(ImVec2(26, 30));
        Text(title, TextAlpha);
        SetWindowFontScale(1.0f);

        SetCursorPos(ImVec2(26, 72));
        Text(message, TextAlpha);

        if (RetValueList[id] == -1)
        {
            bool HasButtons = VectorSize > 1; //是否有多个按钮
            bool IsRet = false;

            int ButtonIndex = 0; // 引入按钮索引

            // --- 新增：计算靠右对齐的参数 ---
            const float RightMargin = 20.0f;     // 距离右边框的边距
            const float ButtonSpacing = 6.0f;    // 按钮之间的间距
            // 计算所有按钮的总宽度
            const float TotalButtonsWidth = VectorSize * ButtonSize.x + (VectorSize - 1) * ButtonSpacing;
            // 计算第一个按钮的起始 X 坐标
            const float StartX = it_anim->second.size.x - RightMargin - TotalButtonsWidth;

            for (const auto& pair : button)
            {
                string name = pair.first;
                bool UseColor = colors.find(name) != colors.end();

                if (UseColor)
                    PushStyleColor(ImGuiCol_Button, colors[name]);

                // 根据索引和间距，依次向右排列
                const float CurrentX = StartX + ButtonIndex * (ButtonSize.x + ButtonSpacing);
                SetCursorPos(ImVec2(CurrentX, it_anim->second.size.y - (ButtonSize.y * 1.5)));

                if (Button(name.c_str(), ButtonSize))
                {
                    IsRet = true;
                    RetValueList[id] = pair.second;
                }

                if (UseColor)
                    PopStyleColor();

                ButtonIndex++; // 索引递增
            }
        }

        if (RetValueList[id] != -1 && it_anim->second.alpha < 5)
        {
            const int ret = RetValueList[id];
            RetValueList.erase(id);
            anim.erase(id);
            IsOpen = true; // 保证下次弹窗可以打开
            CloseCurrentPopup();
            EndPopup();
            return ret;
        }
        EndPopup();
    }

    return -1;
}

void ReleasGui(WNDCLASSEX wcex, GuiData* gui)
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    DestroyContext();
    ImPlot::DestroyContext();
    ImPlot3D::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(GCHWND);
    UnregisterClass(wcex.lpszClassName, wcex.hInstance);
}

static Texture2D LoadTexture(unsigned char* data, int& image_width, int& image_height, int* w, int* h, const char* id)
{
    if (image_width <= 0 || image_height <= 0)
    {
        image_width = 32;
        image_height = 32;
    }
    if (w != nullptr) *w = image_width;
    if (h != nullptr) *h = image_height;

    // 创建DX11纹理
    ID3D11Texture2D* pTexture = nullptr;
    Texture2D textureSRV = nullptr;

    // 创建纹理描述
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 匹配stbi_load加载的RGBA8格式
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    // 设置子资源数据
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem = data;
    initData.SysMemPitch = image_width * 4; // RGBA = 4字节每像素
    initData.SysMemSlicePitch = image_height * initData.SysMemPitch;

    // 创建纹理
    HRESULT hr = MainGuiData.pd3dDevice->CreateTexture2D(&desc, &initData, &pTexture);
    if (FAILED(hr))
    {
        PrintLog("[Render] 创建纹理失败，id: "s + id, PL_ERROR);
        return nullptr;
    }

    // 创建着色器资源视图
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = MainGuiData.pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &textureSRV);
    pTexture->Release(); // 创建SRV后释放纹理引用

    if (FAILED(hr))
    {
        PrintLog("[Render] 创建着色器资源视图失败，id: "s + id, PL_ERROR);
        return nullptr;
    }
    return textureSRV;
}

struct TextureInfo
{
    Texture2D texture;
    int width;
    int height;
};
shared_mutex TextureMutex;
map<string, TextureInfo> TextureMap;
Texture2D LoadTextureFromMemory(const unsigned char* buffer, int len, const char* id, bool CacheDisabled, int* w, int* h)
{
    // 检查缓存（读锁：与 GetInfo 等线程的写入、主线程的读取并发安全）
    if (!CacheDisabled)
    {
        shared_lock<shared_mutex> lock(TextureMutex);
        auto it = TextureMap.find(id);
        if (it != TextureMap.end())
        {
            if (w != nullptr) *w = it->second.width;
            if (h != nullptr) *h = it->second.height;
            return it->second.texture;
        }
    }

    // 加载图片
    int image_width = 0;
    int image_height = 0;
    int nrChannels = 0;

    unsigned char* image_data = stbi_load_from_memory(buffer, len, &image_width, &image_height, &nrChannels, 4); // 强制加载为4通道(RGBA)
    if (image_data == nullptr)
    {
        PrintLog("[Render] 从内存加载图片失败，id: "s + id, PL_ERROR);
        return nullptr;
    }

    Texture2D textureSRV = LoadTexture(image_data, image_width, image_height, w, h, id);

    // 释放图片数据
    stbi_image_free(image_data);

    // 添加到缓存（写锁：防止与并发读取/写入互相破坏 map 结构）
    if (!CacheDisabled && textureSRV != nullptr)
    {
        lock_guard<shared_mutex> lock(TextureMutex);
        TextureMap[id].texture = textureSRV;
        TextureMap[id].width = image_width;
        TextureMap[id].height = image_height;
    }

    return textureSRV;
}

Texture2D LoadTextureFromFile(const string& filename, bool CacheDisabled, int* w, int* h)
{
    // 假设文件没了就使用缓存，不影响...
    // 检查缓存
    if (!CacheDisabled)
    {
        shared_lock<shared_mutex> lock(TextureMutex);
        auto it = TextureMap.find(filename);
        if (it != TextureMap.end())
        {
            if (w != nullptr) *w = it->second.width;
            if (h != nullptr) *h = it->second.height;
            return it->second.texture;
        }
    }
    // 检查文件是否存在，
    if (_access(filename.c_str(), 0) != 0)
    {
        return nullptr;
    }

    // 加载图片
    int image_width = 0;
    int image_height = 0;
    int nrChannels = 0;

    unsigned char* image_data = stbi_load(filename.c_str(), &image_width, &image_height, &nrChannels, 4); // 强制加载为4通道(RGBA)
    if (image_data == nullptr)
    {
        PrintLog("[Render] 加载图片失败: " + filename, PL_ERROR);
        return nullptr;
    }

    Texture2D textureSRV = LoadTexture(image_data, image_width, image_height, w, h, filename.c_str());

    // 释放图片数据
    stbi_image_free(image_data);

    // 添加到缓存
    if (!CacheDisabled && textureSRV != nullptr)
    {
        lock_guard<shared_mutex> lock(TextureMutex);
        TextureMap[filename].texture = textureSRV;
        TextureMap[filename].width = image_width;
        TextureMap[filename].height = image_height;
    }
   
    return textureSRV;
}

// 清理纹理缓存的函数
void ClearTextureCache()
{
    lock_guard<shared_mutex> lock(TextureMutex);
    for (auto& pair : TextureMap)
    {
        if (pair.second.texture != nullptr)
            pair.second.texture->Release();
    }
    TextureMap.clear();
}

auto GetGuiColor(ImGuiStyle& Style, float alpha)
{
    static auto Colors = Style.Colors;
    
    Colors[ImGuiCol_WindowBg] = ImVec4{ 0.06f, 0.05f, 0.09f, alpha };
    Colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.12f, 0.10f, 0.16f, alpha };
    Colors[ImGuiCol_ChildBg] = ImVec4{ 0.15f, 0.13f, 0.20f, alpha };

    // Border
    Colors[ImGuiCol_Border] = ImVec4{ 0.25f, 0.20f, 0.32f, 0.28f };
    Colors[ImGuiCol_BorderShadow] = ImVec4{ 0.25f, 0.20f, 0.32f, 0.28f };

    // Text
    Colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
    Colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };

    // Headers
    Colors[ImGuiCol_Header] = ImVec4{ 0.13f, 0.13f, 0.17f, alpha };
    Colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, alpha };
    Colors[ImGuiCol_HeaderActive] = ImVec4{ 0.16f, 0.16f, 0.21f, alpha };

    // Buttons
    Colors[ImGuiCol_Button] = ImVec4{ 0.13f, 0.13f, 0.17f, alpha };
    Colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.21f, 0.22f, 0.27f, alpha };
    Colors[ImGuiCol_ButtonActive] = ImVec4{ 0.17f, 0.17f, 0.22f, alpha };

    Colors[ImGuiCol_CheckMark] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };

    // Popups
    Colors[ImGuiCol_PopupBg] = ImVec4{ 0.1f, 0.1f, 0.13f, alpha };

    // Slider
    Colors[ImGuiCol_SliderGrab] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.58f };
    Colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.58f };

    // Frame BG
    Colors[ImGuiCol_FrameBg] = ImVec4{ 0.10f, 0.10, 0.14, alpha };
    Colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.26f, 0.27f, 0.38f, alpha };
    Colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.2f, 0.2f, 0.25f, alpha };

    // Tabs
    // 普通态：基础暗色，保持沉静
    Colors[ImGuiCol_Tab] = ImVec4{ 0.13f, 0.12f, 0.17f, alpha };
    // 悬停态：明显提亮，并引入紫色调，提示可交互
    Colors[ImGuiCol_TabHovered] = ImVec4{ 0.26f, 0.23f, 0.35f, alpha };
    // 激活态：最亮，带有强烈的紫色色相，一眼就能看出被选中
    Colors[ImGuiCol_TabActive] = ImVec4{ 0.36f, 0.30f, 0.50f, alpha };
    // 失焦普通态：极度压暗，退居二线
    Colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.10f, 0.09f, 0.13f, alpha };
    // 失焦激活态：保持比未选中亮一点，但不刺眼
    Colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.17f, 0.16f, 0.23f, alpha };

    // Title
    Colors[ImGuiCol_TitleBg] = ImVec4{ 0.16f, 0.16f, 0.21f, alpha };
    Colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, alpha };
    Colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.16f, 0.16f, 0.21f, alpha };

    // Scrollbar
    Colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.1f, 0.1f, 0.13f, alpha };
    Colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.16f, 0.16f, 0.21f, alpha };
    Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, alpha };
    Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.24f, 0.24f, 0.32f, alpha };

    // Seperator
    Colors[ImGuiCol_Separator] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };
    Colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };
    Colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 1.0f };

    // Resize Grip
    Colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
    Colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.29f };
    Colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 0.29f };

    // Modal
    Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    return Colors;
}

auto GetGuiStyle(ImGuiStyle& Style)
{
    Style.ChildRounding = 4.0f;
    Style.FrameRounding = 4.0f;
    Style.GrabRounding = 4.0f;
    Style.PopupRounding = 4.0f;
    Style.ScrollbarRounding = 6.0f;
    Style.TabRounding = 4.0f;
    Style.WindowRounding = 4.0f;

    Style.FramePadding.y = 4.0f;
    Style.ScrollbarSize = 8.0f;

    Style.ChildBorderSize = 1.0f;
    Style.WindowBorderSize = 1.0f;

    Style.FrameBorderSize = 0.0f;
    Style.PopupBorderSize = 0.0f;
    Style.TabBorderSize = 0.0f;
    Style.TabBarBorderSize = 0.0f;
    Style.SeparatorTextBorderSize = 0.0f;
    return Style;
}

struct ProgressBar_element
{
    float progress;
};

void DrawProgressBar(const char* label, float progress, ImVec2 size, ImVec4 Color, ImVec4 bgColor, float thickness)
{
    PushID(label);
    ImGuiID id = GetID(label);

    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    BeginChild(label, { size.x + 2, size.y + 2 }, true,
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoCollapse  |
        ImGuiWindowFlags_NoScrollWithMouse);
    PopStyleVar();//ImGuiStyleVar_WindowPadding
    // 确保进度值在0到1之间
    progress = ImClamp(progress, 0.0f, 1.0f);

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImVec2 AnimPos = CalcWidgetsAnimVert(id);
    SetWindowPos(AnimPos);

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // 计算绘制区域
    float off = thickness / 2;
    ImVec2 pos = AnimPos;
    ImVec2 halfSize = { size.x / 2 - off, size.y / 2 - off };
    ImVec2 center = ImVec2(pos.x + halfSize.x + off, pos.y + halfSize.y + off);

    // 计算圆的半径
    float radius = size.x < size.y ? halfSize.x : halfSize.y;

    // 开始绘制列表
    ImDrawList* draw_list = window->DrawList;

    ImVec2 ChildSize = window->Size;
    draw_list->AddRectFilled(pos, { ChildSize.x + pos.x, ChildSize.y + pos.y }, GetColorU32(bgColor), 4);

    // 绘制半圆的边框
    const int circle_segments = 60;                  // 分段数，越多越平滑
    const float angle_max = 0.0f;                    // 最大角度
    const float angle_min = -180.0f;                 // 最小角度
    const float angle_range = angle_max - angle_min; // 角度范围

    // 绘制半圆背景
    for (int i = 0; i < circle_segments; i++)
    {
        const float a1 = (i / (float)circle_segments) * angle_range + angle_min;
        const float a2 = ((i + 1) / (float)circle_segments) * angle_range + angle_min;
        draw_list->AddLine(
            ImVec2(center.x + ImCos(a1 * IM_PI / 180.0f) * radius, center.y + ImSin(a1 * IM_PI / 180.0f) * radius),
            ImVec2(center.x + ImCos(a2 * IM_PI / 180.0f) * radius, center.y + ImSin(a2 * IM_PI / 180.0f) * radius),
            GetColorU32(ImGuiCol_FrameBg), thickness);
    }

    static map <ImGuiID, ProgressBar_element> anim;
    auto it_anim = anim.find(id);
    if (it_anim == anim.end())
    {
        anim.insert({ id, { 0 } });
        it_anim = anim.find(id);
    }
    float speed = 0.1f * (1.0f - GetIO().DeltaTime);

    it_anim->second.progress = ImLerp(it_anim->second.progress, progress, speed);

    // 绘制进度指示器
    for (int i = 0; i < circle_segments; i++)
    {
        const float a1 = (i / (float)circle_segments) * it_anim->second.progress * angle_range + angle_min;
        const float a2 = ((i + 1) / (float)circle_segments) * it_anim->second.progress * angle_range + angle_min;
        if (a2 > a1)
        {
            ImVec2 p1 = ImVec2(center.x + ImCos(a1 * IM_PI / 180.0f) * radius, center.y + ImSin(a1 * IM_PI / 180.0f) * radius);
            ImVec2 p2 = ImVec2(center.x + ImCos(a2 * IM_PI / 180.0f) * radius, center.y + ImSin(a2 * IM_PI / 180.0f) * radius);

            draw_list->AddLine(p1, p2, GetColorU32(Color), thickness);
        }
    }

    // 绘制标签
    ImVec2 textSize = CalcTextSize(label);

    SetCursorPos({
        GetCursorPosX() + (size.x / 2) - (GetFontSize() / 2),
        GetCursorPosY() + (size.y / 3) });

    Text("%d%%", (int)(progress * 100.0f));
    string st = label;
    if (st.find("##") != 0)
    {
        SetCursorPosX((GetWindowSize().x - textSize.x) / 2);
        Text("%s", label);
    }

    EndChild();
    PopID();
}

/*void InputKey(const char* label, int* key)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImGuiID id = GetID(label);
    static ImGuiID ActiveID = 0;
    static map<const char*, bool> IsInit;

    if (!IsInit[label])
    {
        IsInit[label] = true;
        PostMessage(GCHWND, WM_KEYDOWN, *key, 0);
        PostMessage(GCHWND, WM_KEYUP, *key, 0);
    }

    char* KeyName = nullptr;
    if (ActiveID == id)
        KeyName = (char*)u8"请按下按键";
    ImGuiKey kk = (ImGuiKey)*key;
    if (*key == VK_ESCAPE)
        *key = 0;
    if (*key == 5 || *key == 6)
        KeyName = (*key == 6) ? (char*)(config.UseChinese ? u8"鼠标前侧键" : "Forward Button") : (char*)(config.UseChinese ? u8"鼠标后侧键" : "Back Button ");
    else
        KeyName = (char*)GetKeyName(kk);

    //InputText(label, KeyName, sizeof(KeyName) + 2, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
    PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ChildBg));
    PushStyleColor(ImGuiCol_ButtonHovered, GetColorU32(ImGuiCol_ScrollbarGrabHovered));
    PushStyleColor(ImGuiCol_ButtonActive, GetColorU32(ImGuiCol_ScrollbarGrabActive));
    if (Button((KeyName + "##"s + label).c_str(), ImVec2(CalcItemWidth(), CalcTextSize(KeyName).y + style.FramePadding.y * 2.0f)))
        ActiveID = id;
    PopStyleColor(3);
    if (ActiveID == id)
    {
        
        KeyName = (char*)u8"请按下按键";
        int keys = GetKeyPressed();
        if (keys != 0)
        {
            *key = keys;
            ActiveID = 0;
        }
    }
}*/

static int GetKeyPressed()
{
    int keys = 0;
    for (int i = 5; i < 512; i++)
    {
        if (IsKeyPressed((ImGuiKey)i))
            return i;
    }
    return 0;
}

void TabsText(const char* Text)
{
    SetWindowFontScale(1.4f);
    BulletText(Text);
    SameLine();
    Separator();
    SetWindowFontScale(1.0f);
}

//=================================后端相关=================================
/*
vector<HWND> CCRR_TargetHwnds;
BOOL CALLBACK CCRR_Callback(HWND hwnd, LPARAM lParam)
{
    CCRR_TargetHwnds.push_back(hwnd);
    return TRUE; // 继续枚举
}

bool ClipCoveredRenderRect(HWND hwnd)
{
    CCRR_TargetHwnds.clear();
    EnumWindows(CCRR_Callback, 0);

    // 遍历所有窗口，求出所有窗口的 交集区域 和 被遮挡的区域面积
    for (HWND TargetHwnd : CCRR_TargetHwnds)
    {
        if (TargetHwnd == hwnd || !IsWindowVisible(TargetHwnd))
            continue;
       
        RECT SelfRect{};
        RECT TargetRect{};
        GetWindowRect(hwnd, &SelfRect);
        GetWindowRect(TargetHwnd, &TargetRect);
        POINT TargetSize = {
            TargetRect.right - TargetRect.left,
            TargetRect.bottom - TargetRect.top
        };

        // 计算其他窗口和当前窗口的交集区域
        RECT Intersect{};
        IntersectRect(&Intersect, &SelfRect, &TargetRect);
        // 添加渲染遮挡区域
        //PushClipRect(ImVec2(Intersect.left, Intersect.top), ImVec2(Intersect.right, Intersect.bottom), false);

        // 计算被遮挡的区域面积
        TargetSize.x -= max(0, TargetRect.right - SelfRect.right);
        TargetSize.x -= max(0, SelfRect.left - TargetRect.left);
        TargetSize.y -= max(0, TargetRect.bottom - SelfRect.bottom);
        TargetSize.y -= max(0, SelfRect.top - TargetRect.top);

        int CoveredSize = TargetSize.x * TargetSize.y;
        if ((SelfRect.right - SelfRect.left) * (SelfRect.bottom - SelfRect.top) * 0.9 <= CoveredSize)
            return true; // 几乎完全被遮挡
    }

    return false;
}*/

bool CreateDeviceD3D(HWND hWnd)
{
    // 设置交换链
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 4;// 开启 4x MSAA
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &MainGuiData.pSwapChain, &MainGuiData.pd3dDevice, &featureLevel, &MainGuiData.pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // 如果硬件不可用，就尝试高性能WARP软件驱动程序。
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &MainGuiData.pSwapChain, &MainGuiData.pd3dDevice, &featureLevel, &MainGuiData.pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();

    // ============ 创建全局点采样器 ============
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;   // 点采样（无模糊）
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(MainGuiData.pd3dDevice->CreateSamplerState(&sampDesc, &MainGuiData.pPointSampler)))
        return false;   // 采样器创建失败
    return true;
}

static void SetSamplerCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    // cmd->UserCallbackData 存储了我们传入的 Sampler 指针
    ID3D11SamplerState* pSampler = (ID3D11SamplerState*)cmd->UserCallbackData;

    if (MainGuiData.pd3dDeviceContext && pSampler)
    {
        // 绑定采样器到 Pixel Shader 的第 0 号槽位
        // ImGui 默认使用 Slot 0
        MainGuiData.pd3dDeviceContext->PSSetSamplers(0, 1, &pSampler);
    }
}

void CreateSamplers()
{
    ID3D11Device* pd3dDevice = MainGuiData.pd3dDevice; // 获取你的设备指针

    // 1. 创建像素风格采样器
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // 关键：设置为点采样
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;   // 或者 CLAMP，看你需求
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    pd3dDevice->CreateSamplerState(&samplerDesc, &MainGuiData.pPointSampler);

    // 2. 创建普通线性采样器 (用于恢复默认状态)
    // 如果不恢复，后面的文字渲染也会变成像素风
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // 线性过滤
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    pd3dDevice->CreateSamplerState(&samplerDesc, &MainGuiData.pLinearSampler);
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (MainGuiData.pSwapChain)
    {
        MainGuiData.pSwapChain->Release();
        MainGuiData.pSwapChain = nullptr;
    }
    if (MainGuiData.pd3dDeviceContext)
    {
        MainGuiData.pd3dDeviceContext->Release();
        MainGuiData.pd3dDeviceContext = nullptr;
    }
    if (MainGuiData.pd3dDevice)
    {
        MainGuiData.pd3dDevice->Release();
        MainGuiData.pd3dDevice = nullptr;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    MainGuiData.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    MainGuiData.pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &MainGuiData.mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (MainGuiData.mainRenderTargetView)
    {
        MainGuiData.mainRenderTargetView->Release();
        MainGuiData.mainRenderTargetView = nullptr;
    }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif
#define WM_GC_SAVE_INI  (WM_USER + 1145)

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD WINAPI WindowProcessMessagesThread(LPVOID lpParam)
{
    string name = "  ";
    IsCreateWindowSussce = false;
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
    wcex.lpszClassName = name.c_str();
    wcex.hIconSm = hIcon;

    WindowWidth = IsFirstRun ? 600 : 800;
    WindowHeight = IsFirstRun ? 400 : 500;
    {
        RegisterClassEx(&wcex);
        GCHWND = CreateWindow(wcex.lpszClassName,
            name.c_str(), WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_CAPTION & WS_POPUP /* & WS_EX_LAYERED*/ & WS_EX_ACCEPTFILES,
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
        PrintLog("[Render]创建窗口成功！", PL_INFO);
    }
    IsCreateWindowSussce = true;

    PrintLog("[Render]开始消息循环", PL_INFO);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
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

        if (GCExit)
        {
            PrintLog("[Render]消息循环线程退出", PL_INFO);
            break;
        }
    }
    return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CREATE:
    {
        ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
        ChangeWindowMessageFilter(0x0049, MSGFLT_ADD); //0x0049 = WM_COPYGLOBALDATA，因为这个没有公开。
        break;
    }
    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED)
            return 0;
        MainGuiData.ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        MainGuiData.ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    }
    case WM_SYSCOMMAND:
    {
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    }
    case WM_DESTROY:
    case WM_CLOSE:
    {
        if (IsFirstRun)
        {
            DelFile(ConfigFile.c_str());
            GCExit = true;
        }
        ShowGui = false;
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        UINT numFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        for (UINT i = 0; i < numFiles; i++)
        {
            char temp[MAX_PATH];
            DragQueryFile(hDrop, i, temp, MAX_PATH);
            string file = temp;
            string NewFile;

            if (file.find(".zip") != string::npos)
            {
                string filename = file;
                GetStringFileName(filename, true);

                NewFile = ScriptPath + "\\" + filename;

                if (CopyFile(file.c_str(), NewFile.c_str(), false) == 0)
                {
                    PrintLog("复制脚本文件失败:" + file + " -> " + NewFile, PL_ERROR);
                    RenderTip(u8"复制脚本文件失败:" + file, 1.5f, PT_ERROR);
                }
                else
                {
                    string filename = file;
                    GetStringFileName(filename);
                    RenderTip(u8"添加脚本文件成功:" + filename, 1.5f, PT_INFO);
                }
                LastTab = CurrentTab;
                NextTab = Tab::Script;
                static auto RefreshScriptFun = []()
                    {
                        RefushScriptList();
                    };
                thread FunThread(RefreshScriptFun);
                FunThread.detach();
            }
            else
            {
                PrintLog("不支持的脚本格式:" + file, PL_WARNING);
                RenderTip(u8"不支持的脚本格式", 1.5f, PT_WARNING);
            }
        }
        DragFinish(hDrop);
        return 0;
    }
    }

    //===============================处理窗口拖拽===============================
    static POINT DragDeviation = { -1, -1 };
    static bool IsActiveDrag = false;
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        POINT pt;
        GetCursorPos(&pt);
        POINT client = pt;
        ScreenToClient(GCHWND, &client);
        if (client.x >= 0 && client.x <= WindowWidth && client.y >= 0 && client.y <= 32)
        {
            IsActiveDrag = true;
            DragDeviation = client;
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (IsActiveDrag)
        {
            POINT pt;
            GetCursorPos(&pt);
            SetWindowPos(GCHWND, NULL,
                pt.x - DragDeviation.x, pt.y - DragDeviation.y,
                0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
        }
        return 0;
    }
    case WM_CAPTURECHANGED: // 防止捕获被外部窗口夺走时，还在跟随拖拽
    case WM_LBUTTONUP:
    {
        IsActiveDrag = false;
        DragDeviation = { -1, -1 };
        return 0;
    }
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
