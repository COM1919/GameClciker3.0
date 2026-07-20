#pragma once
#include "framework.h"
#include "ImGui/TextEditor/TextEditor.h" 
#include <string>

using namespace std;

namespace ScriptEditor
{
    TextEditor editor;

    const string DefaultScriptCodeChinese =
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

    const string DefaultScriptCodeEnglish =
        R"(-- Template example code
local cps = 12
local IniFile = "awa.ini"

-- Main function
function main()
    if IsScriptKeyClick() then
        -- Script key clicked
    end
    if GetKeyDown(32) then
        -- Space key pressed
    end
end

-- Entry function for rendering UI
function render()
    Text("&bSet CPS")
    -- Fixed pairing syntax
    cps = SliderInt("##script_jumpcps", cps, 2, 30, "%d&dCPS")
end

function saveConfig()
    SetIniFile("Settings", "cps", cps, IniFile)
end

function initScript()
    cps = GetIniFile("Settings", "cps", cps, IniFile)
    SetScriptLoopDelay(1)
end
)";

    const TextEditor::Palette GetVisualStudioDarkPalette()
    {
        return { {
                // 0. Default (普通文本)
                IM_COL32(0xD4, 0xD4, 0xD4, 0xFF),
                // 1. Keyword (关键字)
                IM_COL32(0x56, 0x9C, 0xD6, 0xFF),   // #569CD6
                // 2. Number
                IM_COL32(0xB5, 0xCE, 0xA8, 0xFF),   // #B5CEA8
                // 3. String
                IM_COL32(0xCE, 0x91, 0x78, 0xFF),   // #CE9178
                // 4. CharLiteral
                IM_COL32(0xCE, 0x91, 0x78, 0xFF),
                // 5. Punctuation
                IM_COL32(0xD4, 0xD4, 0xD4, 0xFF),
                // 6. Preprocessor
                IM_COL32(0xBD, 0x63, 0x86, 0xFF),   // #BD6386
                // 7. Identifier (普通变量/未注册标识符) → 青色
                IM_COL32(0x9C, 0xDC, 0xFE, 0xFF),   // #9CDCFE
                // 8. KnownIdentifier (已知标识符/自定义API) → 黄色
                IM_COL32(0xDC, 0xDC, 0xAA, 0xFF),   // #DCDCAA
                // 9. PreprocIdentifier
                IM_COL32(0xDC, 0xDC, 0xAA, 0xFF),
                // 10. Comment
                IM_COL32(0x6A, 0x99, 0x55, 0xFF),   // #6A9955
                // 11. MultiLineComment
                IM_COL32(0x6A, 0x99, 0x55, 0xFF),
                // 12. Background
                IM_COL32(0x1E, 0x1E, 0x1E, 0xFF),
                // 13. Cursor
                IM_COL32(0xAE, 0xAF, 0xAD, 0xFF),
                // 14. Selection
                IM_COL32(0x26, 0x4F, 0x78, 0xFF),
                // 15. ErrorMarker
                IM_COL32(0xF1, 0x4C, 0x4C, 0x60),
                // 16. Breakpoint
                IM_COL32(0xF4, 0x47, 0x47, 0xFF),
                // 17. LineNumber
                IM_COL32(0x85, 0x85, 0x85, 0xFF),
                // 18. CurrentLineFill
                IM_COL32(0x2A, 0x2D, 0x2E, 0xFF),
                // 19. CurrentLineFillInactive
                IM_COL32(0x2A, 0x2D, 0x2E, 0x00),
                // 20. CurrentLineEdge
                IM_COL32(0x33, 0x33, 0x33, 0xFF)
            } };
    }

    void InitEditor()
    {
        editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GCLua());
        editor.SetPalette(GetVisualStudioDarkPalette());
        editor.SetShowWhitespaces(false);
        editor.SetTabSize(4);
    }

    void Render()
    {
        ImVec2 size = GetContentRegionAvail();
        editor.Render("##GCScriptEditor", size, false);
    }
}
