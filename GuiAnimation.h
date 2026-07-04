#pragma once


static std::map <ImGuiID, float> SMA_Anim;
static std::map <ImGuiID, bool> SMA_Start;
static std::map <ImGuiID, bool> SMA_End;
static bool SMA_IsStart = false;
static bool SMA_Disable = false;
static unsigned long SMA_Seed = 1919;

static unsigned int SMA_rand()
{
    SMA_Seed = SMA_Seed * 1103515245 + 12345;
    return (unsigned  int)(SMA_Seed / 65536) % 32768;
}

static void SMA_srand(unsigned int seed)
{
    SMA_Seed = seed;

}

// COM1919Ìí¼ÓµÄShowMoveAnimationº¯Êý
static ImVec2 ShowMoveAnimation(ImGuiID id)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (SMA_Disable)
        return window->DC.CursorPos;

    if (SMA_IsStart)
        SMA_Start[id] = true;

    if (!SMA_IsStart && !SMA_Start[id])
        return window->DC.CursorPos;

    auto it_anim = SMA_Anim.find(id);
    if (it_anim == SMA_Anim.end())
    {
        SMA_Anim.insert({ id, { ImGui::GetIO().DisplaySize.y } });
        it_anim = SMA_Anim.find(id);
    }

    const ImVec2 pos = {
        window->DC.CursorPos.x - window->Pos.x + window->Scroll.x,
        window->DC.CursorPos.y - window->Pos.y + window->Scroll.y
    };
    float PosXRate = 1.0f - (pos.x / ImGui::GetIO().DisplaySize.x);
    float speed = 0.4f * ImMax(0.2f, PosXRate) * (1.0f - ImGui::GetIO().DeltaTime);
    it_anim->second = ImLerp(it_anim->second, pos.y, speed);
    if (ImAbs(it_anim->second - pos.y) <= 2)
    {
        SMA_End[id] = true;
        SMA_Start[id] = false;
    }

    if (SMA_End[id])
        return window->DC.CursorPos;
    else
        return ImVec2(window->Pos.x - window->Scroll.x + pos.x, window->Pos.y - window->Scroll.y + it_anim->second);
}

static ImVec2 ShowMoveAnimationVertical(ImGuiID id)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (SMA_Disable)
        return window->DC.CursorPos;

    if (SMA_IsStart)
        SMA_Start[id] = true;

    if (!SMA_IsStart && !SMA_Start[id])
        return window->DC.CursorPos;

    auto it_anim = SMA_Anim.find(id);
    if (it_anim == SMA_Anim.end())
    {
        SMA_Anim.insert({ id, { ImGui::GetIO().DisplaySize.x } });
        it_anim = SMA_Anim.find(id);
    }

    const ImVec2 pos = {
        window->DC.CursorPos.x - window->Pos.x + window->Scroll.x,
        window->DC.CursorPos.y - window->Pos.y + window->Scroll.y
    };
    float PosYRate = 1.0f - (pos.y / ImGui::GetIO().DisplaySize.y);
    float speed = 0.4f * ImMax(0.2f, PosYRate) * (1.0f - ImGui::GetIO().DeltaTime);
    it_anim->second = ImLerp(it_anim->second, pos.x, speed);
    if (ImAbs(it_anim->second - pos.x) <= 2)
    {
        SMA_End[id] = true;
        SMA_Start[id] = false;
    }

    if (SMA_End[id])
        return window->DC.CursorPos;
    else
        return ImVec2(window->Pos.x - window->Scroll.x + it_anim->second, window->Pos.y - window->Scroll.y + pos.y);
}