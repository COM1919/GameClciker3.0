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
        PrintLog("用户协议已显示", PL_INFO);
    }
    
    if (IsKeyPressed(ImGuiKey_F3))
    {
        DebugInfo = !DebugInfo;
    }

    if (!StopClicker)
        StopClicker = true;

    static int Tab = 0;

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

    Begin(config.UseChinese ? u8"GC连点器 - 新手向导" : "GameClicker - Newbie Guide", NULL,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove);
    SetWindowSize({ 600, 400 });
    SetWindowPos({ 0, 0 });

    if (DebugInfo)
    {
        ImVec2 LastPos = GetCursorPos();
        SetCursorPos({ 0, 400 - 30 });
        Text(u8"%.1f FPS", io.Framerate);
        SetCursorPos(LastPos);
    }

    if (!IsDownloadRes)
    {
        switch (Step)
        {
            case 1:
            {
                SetCursorPos(ImVec2(2, 2));

                BeginChild(u8"##ua_child", ImVec2(596, 260), true, ImGuiWindowFlags_HorizontalScrollbar);
                SetWindowFontScale(0.8f);
                Text(UserAgreement.c_str());
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
                BeginDisabled(config.UseMinimalistMode);
                {
                    Checkbox(config.UseChinese ? u8"精简模式" : "Concise mode", &config.UseConciseMode);
                }
                EndDisabled();

                Checkbox(config.UseChinese ? u8"极简模式" : "Minimal mode", &config.UseMinimalistMode);

                if (VarHasChanged(config.UseMinimalistMode, "UseMinimalistMode") && config.UseMinimalistMode)
                {
                    RenderMessageBox(config.UseChinese ? u8"打开极简功能后，大部分功能将隐藏。\n开启极简功能后，将会重置连点设置。" : "After opening the minimalist function, most functions will be hidden. \nEnabling the minimalist function will reset the clicker settings.", config.UseChinese ? u8"警告" : "Warning", MB_OK, false);
                    config.UseConciseMode = true;
                }

                Text(config.UseChinese ? u8"右键连点按键" : "RClicker Key");
                SameLine();
                SetNextItemWidth(100);
                InputKey("##RClickKey", &config.RKEY, config.UseChinese ? u8"请按下按键" : "Press a key");
                if (config.RClickerMode & C_MOUSESMODE || config.RClickerMode & C_ASSISTMODE)
                    AttachTooltip(config.UseChinese ? u8"&b辅助模式 &f或 &b鼠标模式&f不需要设置快捷键" : "AssistMode or MouseMode does not need to set a shortcut key");

                Text(config.UseChinese ? u8"左键连点按键" : "LClicker Key");
                SameLine();
                SetNextItemWidth(100);
                InputKey("##LClickKey", &config.LKEY, config.UseChinese ? u8"请按下按键" : "Press a key");

                Text(config.UseChinese ? u8"右键CPS" : "RClicker CPS");
                AttachTooltip(config.UseChinese ? u8"CPS指每秒连点次数，越高越快，\n一般右键设置 20CPS 就可以。" : "CPS means the number of clicks per second, the higher the faster, \ngenerally the left key sets 14CPS can be used.");
                SameLine();
                SetNextItemWidth(300);
                SliderInt(u8"##rcps", &config.RCPS, MIN_CPS, MAX_CPS, "%dCPS", ImGuiSliderFlags_AlwaysClamp);

                Text(config.UseChinese ? u8"左键CPS" : "LClicker CPS");
                AttachTooltip(config.UseChinese ? u8"CPS指每秒连点次数，越高越快，\n一般左键设置 14CPS 就可以。" : "CPS means the number of clicks per second, the higher the faster, \ngenerally the right key sets 20CPS can be used.");
                SameLine();
                SetNextItemWidth(300);
                SliderInt(u8"##lcps", &config.LCPS, MIN_CPS, MAX_CPS, "%dCPS", ImGuiSliderFlags_AlwaysClamp);


                const float CenterOffsetX = (GetWindowWidth() - 600) * 0.5f;
                const float CenterOffsetY = (GetWindowHeight() - 400) * 0.5f;
                const float ButtonGroupWidth = 70 + 70 + GetStyle().ItemSpacing.x;
                SetCursorPos(ImVec2(CenterOffsetX + (600 - ButtonGroupWidth) * 0.5f, 400 - 40 - 6));

                if (Button(config.UseChinese ? u8"上一步" : "Back", ImVec2(70, 40)))
                    NextStep--;
                SameLine();
                if (Button(config.UseChinese ? u8"下一步" : "Next", ImVec2(70, 40)))
                    NextStep++;
                break;
            }
            case 3:
            {
                static const Texture2D image_texture = LoadTextureFromFile(Bmp_GCLogo);

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
                if (Button(config.UseChinese ? u8"启动" : "start", ImVec2(135, 45 )))
                    state = 1;
                PopStyleColor();

                break;
            }
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