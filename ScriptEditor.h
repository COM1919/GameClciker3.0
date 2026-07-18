#pragma once
#include "framework.h"
#include "ImGui/TextEditor/TextEditor.h" 
#include <string>

using namespace std;

namespace ScriptEditor
{
    TextEditor editor;

    const string DefaultScriptCode =
        u8R"(-- 模板示例代码
local cps = 12
local IniFile = "awa.ini"

-- 主函数
function main()
    if IsScriptKeyClick() then
        -- 脚本被点击
    end
    if GetKeyDown(32) then
        -- 空格被按下
    end
end

-- 渲染UI的入口函数
function render()
    Text("&b设置CPS")
    -- 固定搭配写法
    cps = SliderInt("##script_jumpcps", cps, 2, 30, "%d&dCPS")
end

function saveConfig()
    SetIniFile("设置", "cps", cps, IniFile)
end

function initScript()
    cps = GetIniFile("设置", "cps", cps, IniFile)
    SetScriptLoopDelay(1)
end
)";
    void InitEditor()
    {
        // 直接使用官方 Lua 定义即可，因为我们在 TextEditor.cpp 里已经把自定义 API 塞进去了
        editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());

        // 设置颜色主题
        editor.SetPalette(TextEditor::GetDarkPalette());

        // 基础设置
        editor.SetShowWhitespaces(false);
        editor.SetTabSize(4);

        // 设置文本
        editor.SetText(DefaultScriptCode);
    }

    void Render()
    {
        static bool IsInit = false;
        if (!IsInit)
        {
            ScriptEditor::InitEditor();
            IsInit = true;
        }

        ImVec2 size = GetContentRegionAvail();
        editor.Render("##GCScriptEditor", size, false);
    }
}
