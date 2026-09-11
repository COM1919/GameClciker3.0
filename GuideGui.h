#pragma once
#include "GC.h"

auto HGuiColor = 0;

ImVec4 hclear_color = (ImVec4)ImColor(0, 0, 0, 255);

int RenderGuide(ImGuiIO& io)
{
    static int state = 0;
    static bool init = false;
    if (!init)
    {
        init = true;
        PrintLog("[GuideRender] 用户协议已显示", PL_INFO);
    }
    
    IsStopClicker = true;

    static ImGuiStyle& Style = GetStyle();
    static auto GuiColor = Style.Colors;
    static bool IsInit = false;

    static int Step = 1;
    static int NextStep = 1;

    if (!IsInit)
    {
        Style = GetGuiStyle(Style);
        if (Step == 0)
        {
            Style.FramePadding = ImVec2(40.0f, 40.0f);
        }
        GuiColor = GetGuiColor(Style);
        IsInit = true;
    }

    EndAnimation();

    Begin(u8"GameClicker", NULL,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
    SetWindowSize(ImVec2(600, 400));
    SetWindowPos(ImVec2(0, 0));

    switch (Step)
    {
    case 1:
    {
        SetCursorPos(ImVec2(2, 2));

        BeginChild(u8"##ua_child", ImVec2(596, 260), true, ImGuiWindowFlags_HorizontalScrollbar);
        SetWindowFontScale(0.8f);
        if (config.UseChinese)
            TextUnformatted(AgreementCNString.c_str(), AgreementCNString.c_str() + AgreementCNString.size());
        else
            TextUnformatted(AgreementENString.c_str(), AgreementENString.c_str() + AgreementENString.size());
        SetWindowFontScale(1.0f);
        EndChild();

        Text(config.UseChinese ? u8"&c本软件完全免费！！！\n&c如果你是购买的本软件，可以举报。" : "This software is completely free! \nIf you have purchased this software, you can report it.");

        static bool IsCheck = false;
        Checkbox(config.UseChinese ? u8"我已阅读并同意用户协议" : "I have read and agree to the user agreement", &IsCheck);

        if (Button(config.UseChinese ? u8"不同意" : "Decline", ImVec2(70, 40)))
            state = -1;
        SameLine();
        BeginDisabled(!IsCheck);
        {
            if (Button(config.UseChinese ? u8"继续" : " Next", ImVec2(70, 40)) && IsCheck)
                NextStep++;
        }
        EndDisabled();
        break;
    }
    case 2:
    {
        static const Texture2D image_texture = LoadTextureFromFile(Pics_GCLogo);

        const static float posx = (600 - 198) / 2.0f;
        const static float target = (400 - 73) / 2.8f;

        float speed = 0.15f * (1.0f - GetIO().DeltaTime);

        static float posy = 300;
        posy = ImLerp(posy, target, speed);

        static float alpha = 10.0f;
        alpha = ImLerp(alpha, 0.0f, speed);

        ImVec4 button_color(
            GuiColor[ImGuiCol_Button].x,
            GuiColor[ImGuiCol_Button].y,
            GuiColor[ImGuiCol_Button].z,
            5 - alpha
        );

        ImVec4 bmp_color(
            GuiColor[ImGuiCol_WindowBg].x,
            GuiColor[ImGuiCol_WindowBg].y,
            GuiColor[ImGuiCol_WindowBg].z,
            alpha
        );

        SetCursorPos(ImVec2(posx, posy));
        ImGui::Image((void*)image_texture, ImVec2(198, 73));
        GetWindowDrawList()->AddRectFilled(ImVec2(posx, posy), ImVec2(posx + 198, posy + 73),
            ColorConvertFloat4ToU32(bmp_color));

        PushStyleColor(ImGuiCol_Button, button_color);
        SetCursorPos(ImVec2((600 - 135) / 2.0f, (400 - 45) / 1.8f));
        if (Button(config.UseChinese ? u8"启动" : "start", ImVec2(135, 45)))
            state = 1;
        PopStyleColor();

        break;
    }
    }

    static bool IsAnime = false;
    if (VarHasChanged(NextStep, "HelpNextStep"))
    {
        ResetAnimationPos();
        BeginAnimation();
        IsAnime = true;
    }

    if (IsAnime)
    {
        float speed = 0.15f * (1.0f - GetIO().DeltaTime);
        const int TagerAlpha = 2;
        static float AnimAlpha = 0;
        AnimAlpha = ImLerp<float>(AnimAlpha, TagerAlpha, speed);

        GetForegroundDrawList()->
            AddRectFilled(ImVec2(0, 0), ImVec2(600, 400), GetColorU32(ImVec4(0, 0, 0, (AnimAlpha >= 1.0f) ? 2.0f - AnimAlpha : AnimAlpha)), 1);

        if (AnimAlpha >= 0.85f)
            Step = NextStep;
        if (AnimAlpha >= 1.85f)
        {
            IsAnime = false;
            AnimAlpha = 0;
        }
    }

    End();

    RenderControl();
    return state;
}