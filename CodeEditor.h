#pragma once
#include "Definitons.h"
#include "GC.h"
#include <unordered_set>

class CodeEditor {
public:
    CodeEditor();
    void Render(const char* title = "Lua Code Editor", bool* p_open = nullptr);
    void SetText(const string& code);
    string GetText() const { return text; }
    void SetLanguage(const string& lang) { language = lang; }

private:
    string text;
    string language;
    bool textChanged;

    // 光标和选择状态
    struct CursorState {
        int position;
        int selectionStart;
        int selectionEnd;
        bool hasSelection;
        CursorState() : position(0), selectionStart(0), selectionEnd(0), hasSelection(false) {}
        CursorState(int p, int s, int e, bool h) : position(p), selectionStart(s), selectionEnd(e), hasSelection(h) {}
    } cursor;

    // 语法高亮
    struct TextColor {
        ImVec4 color;
        int start;
        int end;
        TextColor(ImVec4 c, int s, int e) : color(c), start(s), end(e) {}
    };
    vector<TextColor> coloredRanges;

    // 自动完成
    struct Suggestion {
        string text;
        string description;
        Suggestion(const string& t, const string& d) : text(t), description(d) {}
    };
    vector<Suggestion> suggestions;
    bool showSuggestions;
    int selectedSuggestion;

    // 缩进管理
    int tabSize;
    bool autoIndent;

    // 滚动状态
    float scrollX, scrollY;

    // 渲染状态
    ImVec2 textScreenPos;
    float charWidth, lineHeight;

    void HandleKeyboardInput();
    void HandleMouseInput();
    void ApplySyntaxHighlighting();
    void HandleAutoComplete();
    void HandleAutoIndent();
    void HandleBracketMatching(char c);
    void DrawTextWithSyntaxHighlighting();
    void DrawCursor();
    void DrawLineNumbers();
    void DrawAutoCompletePopup();

    // 工具函数
    int GetCurrentLineStart(int pos) const;
    int GetCurrentLineEnd(int pos) const;
    string GetCurrentLineText(int pos) const;
    string GetWordBeforeCursor() const;
    void InsertTextAtCursor(const string& str);
    void DeleteSelection();

    // Lua特定函数
    void SetupLuaKeywords();
    bool IsLuaKeyword(const string& word) const;
    bool IsLuaBuiltin(const string& word) const;

    // 关键字集合
    unordered_set<string> luaKeywords;
    unordered_set<string> luaBuiltins;
    unordered_set<string> luaTypes;
};

// 内置Lua补全数据
static const vector<pair<string, string>> LUA_SUGGESTIONS = {
    {"function", "function name() ... end"},
    {"if", "if condition then ... end"},
    {"for", "for i = 1, n do ... end"},
    {"while", "while condition do ... end"},
    {"repeat", "repeat ... until condition"},
    {"local", "local variable = value"},
    {"return", "return value"},
    {"end", "end block"},
    {"then", "then block"},
    {"do", "do block"},
    {"else", "else block"},
    {"elseif", "elseif condition then"},
    {"print", "print(value)"},
    {"pairs", "pairs(table)"},
    {"ipairs", "ipairs(table)"},
    {"table", "table library"},
    {"string", "string library"},
    {"math", "math library"},
    {"io", "io library"},
    {"os", "os library"}
};

CodeEditor::CodeEditor() {
    text = R"(-- Lua代码示例
function factorial(n)
    if n == 0 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

function print_table(t)
    for k, v in pairs(t) do
        print(k, v)
    end
end

local numbers = {1, 2, 3, 4, 5}
local sum = 0

for i, num in ipairs(numbers) do
    sum = sum + num
end

print("总和:", sum)
)";

    language = "lua";
    textChanged = false;
    cursor = { 0, 0, 0, false };
    showSuggestions = false;
    selectedSuggestion = 0;
    tabSize = 4;
    autoIndent = true;
    scrollX = scrollY = 0.0f;
    charWidth = 8.0f;
    lineHeight = ImGui::GetTextLineHeight();

    SetupLuaKeywords();
}

void CodeEditor::Render(const char* title, bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    // 编辑器工具栏
    if (ImGui::Button("运行")) { /* 运行代码 */ }
    ImGui::SameLine();
    if (ImGui::Button("清除")) { text.clear(); cursor.position = 0; }
    ImGui::SameLine();
    ImGui::Checkbox("自动缩进", &autoIndent);
    ImGui::SameLine();
    ImGui::Text("行: %d", count(text.begin(), text.end(), '\n') + 1);

    ImGui::Separator();

    // 编辑器区域
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImVec2 size = ImGui::GetContentRegionAvail();

    // 处理输入
    HandleKeyboardInput();
    HandleMouseInput();

    // 应用语法高亮
    ApplySyntaxHighlighting();

    // 开始绘制编辑器
    ImGui::BeginChild("EditorScroll", size, false,
        ImGuiWindowFlags_HorizontalScrollbar |
        ImGuiWindowFlags_AlwaysVerticalScrollbar);

    textScreenPos = ImGui::GetCursorScreenPos();

    // 绘制行号
    DrawLineNumbers();

    ImGui::SameLine();

    // 绘制代码文本
    DrawTextWithSyntaxHighlighting();

    // 绘制光标
    DrawCursor();

    // 显示自动完成
    if (showSuggestions && !suggestions.empty()) {
        DrawAutoCompletePopup();
    }

    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::End();
}

void CodeEditor::SetText(const string& code) {
    text = code;
    cursor.position = 0;
    cursor.hasSelection = false;
    textChanged = true;
}

void CodeEditor::HandleKeyboardInput() {
    if (!ImGui::IsWindowFocused()) return;

    ImGuiIO& io = ImGui::GetIO();

    // 处理字符输入
    for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
        unsigned int c = io.InputQueueCharacters[n];
        if (c > 0 && c < 0x10000) {
            if (cursor.hasSelection) {
                DeleteSelection();
            }

            // 处理自动匹配符号
            if (c == '(' || c == '[' || c == '{' || c == '"' || c == '\'') {
                HandleBracketMatching(static_cast<char>(c));
            }
            else {
                text.insert(cursor.position, 1, static_cast<char>(c));
                cursor.position++;
            }
            textChanged = true;
        }
    }

    // 处理特殊按键
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (cursor.hasSelection) {
            DeleteSelection();
        }
        else if (cursor.position > 0) {
            text.erase(cursor.position - 1, 1);
            cursor.position--;
        }
        textChanged = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (cursor.hasSelection) {
            DeleteSelection();
        }
        else if (cursor.position < text.length()) {
            text.erase(cursor.position, 1);
        }
        textChanged = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (cursor.hasSelection) {
            DeleteSelection();
        }
        HandleAutoIndent();
        textChanged = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        if (cursor.hasSelection) {
            // TODO: 多行缩进
        }
        else {
            text.insert(cursor.position, string(tabSize, ' '));
            cursor.position += tabSize;
        }
        textChanged = true;
    }

    // 方向键
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (cursor.position > 0) cursor.position--;
        cursor.hasSelection = false;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (cursor.position < text.length()) cursor.position++;
        cursor.hasSelection = false;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        // 简单的向上移动一行
        int lineStart = GetCurrentLineStart(cursor.position);
        if (lineStart > 0) {
            int prevLineEnd = lineStart - 1;
            int prevLineStart = GetCurrentLineStart(prevLineEnd);
            int column = cursor.position - lineStart;
            cursor.position = min(prevLineStart + column, prevLineEnd);
        }
        cursor.hasSelection = false;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        // 简单的向下移动一行
        int lineEnd = GetCurrentLineEnd(cursor.position);
        if (lineEnd < text.length()) {
            int nextLineStart = lineEnd + 1;
            int nextLineEnd = GetCurrentLineEnd(nextLineStart);
            int column = cursor.position - GetCurrentLineStart(cursor.position);
            cursor.position = min(nextLineStart + column, nextLineEnd);
        }
        cursor.hasSelection = false;
    }

    // 自动完成触发
    if (ImGui::IsKeyPressed(ImGuiKey_Space) && io.KeyCtrl) {
        HandleAutoComplete();
    }

    // 选择文本 (Shift + 方向键)
    if (io.KeyShift) {
        // 简化实现 - 实际需要更复杂的选择逻辑
    }
}

void CodeEditor::HandleMouseInput() {
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        // 简化实现 - 实际需要计算点击位置对应的文本位置
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 relativePos(mousePos.x - textScreenPos.x, mousePos.y - textScreenPos.y);

        // 简单估算光标位置
        int line = static_cast<int>(relativePos.y / lineHeight);
        int column = static_cast<int>(relativePos.x / charWidth);

        // 计算实际文本位置（简化版）
        int currentLine = 0;
        int currentCol = 0;
        cursor.position = 0;

        for (size_t i = 0; i < text.length() && currentLine < line; i++) {
            if (text[i] == '\n') {
                currentLine++;
                currentCol = 0;
            }
            else {
                currentCol++;
            }
            cursor.position++;

            if (currentLine == line && currentCol >= column) {
                break;
            }
        }

        cursor.hasSelection = false;
    }
}

void CodeEditor::ApplySyntaxHighlighting() {
    coloredRanges.clear();

    if (language != "lua") return;

    string currentWord;
    int wordStart = -1;
    bool inComment = false;
    bool inString = false;
    char stringChar = '\0';
    bool inMultiLineComment = false;

    for (int i = 0; i < text.length(); i++) {
        char c = text[i];

        if (inMultiLineComment) {
            if (c == ']' && i + 1 < text.length() && text[i + 1] == ']') {
                coloredRanges.push_back({ ImVec4(0.3f, 0.5f, 0.3f, 1.0f), wordStart, i + 2 });
                inMultiLineComment = false;
                i++; // 跳过第二个 ']'
            }
            continue;
        }

        if (inComment) {
            if (c == '\n') {
                coloredRanges.push_back({ ImVec4(0.3f, 0.5f, 0.3f, 1.0f), wordStart, i });
                inComment = false;
            }
            continue;
        }

        if (inString) {
            if (c == stringChar && (i == 0 || text[i - 1] != '\\')) {
                coloredRanges.push_back({ ImVec4(0.8f, 0.2f, 0.2f, 1.0f), wordStart, i + 1 });
                inString = false;
            }
            continue;
        }

        // 多行注释
        if (c == '-' && i + 1 < text.length() && text[i + 1] == '[' &&
            i + 2 < text.length() && text[i + 2] == '[') {
            wordStart = i;
            inMultiLineComment = true;
            i += 2; // 跳过 "[[""
            continue;
        }

        // 单行注释
        if (c == '-' && i + 1 < text.length() && text[i + 1] == '-') {
            wordStart = i;
            inComment = true;
            i++; // 跳过第二个 '-'
            continue;
        }

        if (c == '"' || c == '\'') {
            wordStart = i;
            inString = true;
            stringChar = c;
            continue;
        }

        if (isalpha(c) || c == '_') {
            if (currentWord.empty()) {
                wordStart = i;
            }
            currentWord += c;
        }
        else {
            if (!currentWord.empty()) {
                if (IsLuaKeyword(currentWord)) {
                    coloredRanges.push_back({ ImVec4(0.8f, 0.2f, 0.6f, 1.0f), wordStart, i });
                }
                else if (IsLuaBuiltin(currentWord)) {
                    coloredRanges.push_back({ ImVec4(0.2f, 0.5f, 0.8f, 1.0f), wordStart, i });
                }
                currentWord.clear();
            }

            // 数字高亮
            if (isdigit(c) && (i == 0 || !isalnum(text[i - 1]))) {
                int numStart = i;
                while (i < text.length() && (isdigit(text[i]) || text[i] == '.')) {
                    i++;
                }
                coloredRanges.push_back({ ImVec4(0.1f, 0.7f, 0.3f, 1.0f), numStart, i });
                i--; // 补偿循环的i++
            }
        }
    }
}

void CodeEditor::HandleAutoComplete() {
    string currentWord = GetWordBeforeCursor();
    suggestions.clear();

    for (const auto& suggestion : LUA_SUGGESTIONS) {
        if (suggestion.first.find(currentWord) == 0) {
            suggestions.push_back({ suggestion.first, suggestion.second });
        }
    }

    // 添加Lua关键字
    for (const auto& keyword : luaKeywords) {
        if (keyword.find(currentWord) == 0) {
            suggestions.push_back({ keyword, "Lua keyword" });
        }
    }

    // 添加Lua内置函数
    for (const auto& builtin : luaBuiltins) {
        if (builtin.find(currentWord) == 0) {
            suggestions.push_back({ builtin, "Lua built-in function" });
        }
    }

    if (!suggestions.empty()) {
        showSuggestions = true;
        selectedSuggestion = 0;
    }
}

void CodeEditor::HandleAutoIndent() {
    if (!autoIndent) {
        text.insert(cursor.position, 1, '\n');
        cursor.position++;
        return;
    }

    int lineStart = GetCurrentLineStart(cursor.position);
    string currentLine = text.substr(lineStart, cursor.position - lineStart);

    // 计算当前缩进
    string indent;
    for (char c : currentLine) {
        if (c == ' ' || c == '\t') {
            indent += c;
        }
        else {
            break;
        }
    }

    // 检查是否需要增加缩进
    bool increaseIndent = false;
    for (int i = cursor.position - 1; i >= 0; i--) {
        if (text[i] == 't' && i >= 3 && text.substr(i - 3, 4) == "then") {
            increaseIndent = true;
            break;
        }
        if (text[i] == 'o' && i >= 1 && text.substr(i - 1, 2) == "do") {
            increaseIndent = true;
            break;
        }
        if (text[i] == 'n' && i >= 5 && text.substr(i - 5, 6) == "function") {
            increaseIndent = true;
            break;
        }
        if (text[i] == '{' || text[i] == '(' || text[i] == '[') {
            increaseIndent = true;
            break;
        }
        if (!isspace(text[i])) break;
    }

    if (increaseIndent) {
        indent += string(tabSize, ' ');
    }

    text.insert(cursor.position, "\n" + indent);
    cursor.position += 1 + indent.length();
}

void CodeEditor::HandleBracketMatching(char c) {
    unordered_map<char, char> bracketPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'}, {'"', '"'}, {'\'', '\''}
    };

    if (bracketPairs.find(c) != bracketPairs.end()) {
        char closing = bracketPairs[c];
        if (c == '"' || c == '\'') {
            text.insert(cursor.position, 2, c);
            cursor.position++;
        }
        else {
            text.insert(cursor.position, string(1, c) + string(1, closing));
            cursor.position++;
        }
    }
}

void CodeEditor::DrawTextWithSyntaxHighlighting() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // 先绘制所有默认颜色的文本
    ImGui::TextUnformatted(text.c_str());

    // 再在顶部绘制高亮部分
    for (const auto& range : coloredRanges) {
        if (range.start >= text.length() || range.end > text.length()) continue;

        // 计算高亮文本的位置
        string highlighted = text.substr(range.start, range.end - range.start);

        // 找到行和列位置
        int lineStart = 0;
        int col = 0;
        for (int i = 0; i < range.start; i++) {
            if (text[i] == '\n') {
                lineStart = i + 1;
                col = 0;
            }
            else {
                col++;
            }
        }

        int lineNum = count(text.begin(), text.begin() + range.start, '\n');
        float x = pos.x + ImGui::CalcTextSize(text.substr(lineStart, col).c_str()).x;
        float y = pos.y + lineNum * lineHeight;

        drawList->AddText(ImVec2(x, y), ImColor(range.color), highlighted.c_str());
    }
}

void CodeEditor::DrawCursor() {
    if (ImGui::IsWindowFocused()) {
        int lineNum = count(text.begin(), text.begin() + cursor.position, '\n');
        int lineStart = GetCurrentLineStart(cursor.position);
        int column = cursor.position - lineStart;

        ImVec2 pos = ImGui::GetCursorScreenPos();
        float x = pos.x + ImGui::CalcTextSize(text.substr(lineStart, column).c_str()).x;
        float y = pos.y + lineNum * lineHeight;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddLine(ImVec2(x, y), ImVec2(x, y + lineHeight), IM_COL32(255, 255, 255, 255));
    }
}

void CodeEditor::DrawLineNumbers() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    int lineCount = count(text.begin(), text.end(), '\n') + 1;
    float lineNumWidth = ImGui::CalcTextSize("999").x + 10;

    // 绘制行号背景
    drawList->AddRectFilled(pos, ImVec2(pos.x + lineNumWidth, pos.y + lineCount * lineHeight),
        IM_COL32(40, 40, 40, 255));

    ImGui::BeginGroup();
    for (int i = 1; i <= lineCount; i++) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%3d", i);
    }
    ImGui::EndGroup();
}

void CodeEditor::DrawAutoCompletePopup() {
    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
    int lineNum = count(text.begin(), text.begin() + cursor.position, '\n');
    float popupY = cursorScreenPos.y + (lineNum + 1) * lineHeight;

    ImGui::SetNextWindowPos(ImVec2(cursorScreenPos.x, popupY));
    ImGui::SetNextWindowSize(ImVec2(300, 200));

    if (ImGui::Begin("Auto Complete", &showSuggestions,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove)) {
        for (int i = 0; i < suggestions.size(); i++) {
            bool isSelected = (i == selectedSuggestion);
            if (ImGui::Selectable(suggestions[i].text.c_str(), isSelected)) {
                // 替换当前单词
                string currentWord = GetWordBeforeCursor();
                int wordStart = cursor.position - currentWord.length();
                text.erase(wordStart, currentWord.length());
                text.insert(wordStart, suggestions[i].text);
                cursor.position = wordStart + suggestions[i].text.length();
                showSuggestions = false;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }

            // 显示描述
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", suggestions[i].description.c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::End();
    }
}

// 工具函数实现
int CodeEditor::GetCurrentLineStart(int pos) const {
    int start = pos;
    while (start > 0 && text[start - 1] != '\n') {
        start--;
    }
    return start;
}

int CodeEditor::GetCurrentLineEnd(int pos) const {
    int end = pos;
    while (end < text.length() && text[end] != '\n') {
        end++;
    }
    return end;
}

string CodeEditor::GetCurrentLineText(int pos) const {
    int start = GetCurrentLineStart(pos);
    int end = GetCurrentLineEnd(pos);
    return text.substr(start, end - start);
}

string CodeEditor::GetWordBeforeCursor() const {
    int end = cursor.position;
    int start = end;

    while (start > 0 && (isalnum(text[start - 1]) || text[start - 1] == '_')) {
        start--;
    }

    return text.substr(start, end - start);
}

void CodeEditor::InsertTextAtCursor(const string& str) {
    if (cursor.hasSelection) {
        DeleteSelection();
    }
    text.insert(cursor.position, str);
    cursor.position += str.length();
}

void CodeEditor::DeleteSelection() {
    if (!cursor.hasSelection) return;

    int start = min(cursor.selectionStart, cursor.selectionEnd);
    int end = max(cursor.selectionStart, cursor.selectionEnd);

    text.erase(start, end - start);
    cursor.position = start;
    cursor.hasSelection = false;
}

// Lua特定函数实现
void CodeEditor::SetupLuaKeywords() {
    luaKeywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "goto", "if", "in", "local", "nil", "not", "or",
        "repeat", "return", "then", "true", "until", "while"
    };

    luaBuiltins = {
        "print", "type", "pairs", "ipairs", "next", "tostring", "tonumber",
        "getmetatable", "setmetatable", "rawget", "rawset", "rawlen",
        "table", "string", "math", "io", "os", "debug", "coroutine"
    };

    luaTypes = {
        "nil", "boolean", "number", "string", "table", "function", "thread", "userdata"
    };
}

bool CodeEditor::IsLuaKeyword(const string& word) const {
    return luaKeywords.find(word) != luaKeywords.end();
}

bool CodeEditor::IsLuaBuiltin(const string& word) const {
    return luaBuiltins.find(word) != luaBuiltins.end();
}