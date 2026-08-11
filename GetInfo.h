#pragma once
#include "GC.h"
#include "Network.h"
#include <WinInet.h>
#include <urlmon.h>
#include <zlib.h>
#include <unzip.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "wininet.lib")

static inline void ltrim(string& s);
DWORD WINAPI Updata(LPVOID lpParam);
DWORD WINAPI GetInfo(LPVOID lpParam);

// 去除字符串前导空白
static inline void ltrim(string& s)
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
        }));
}

DWORD WINAPI GetInfo(LPVOID lpParam)
{
    string content;
    PrintLog("[Init] 获取使用的服务器类型", PL_INFO);
    if (ReadURLDatas(URL_LZY_SERTYPE, content) > 0)
    {
        content = RemoveSubString(content, "<br />");
        content = ExtractString(content, "=ser=", "=endser=");
        UseServerOfType = (content.find("1") != string::npos) ? 1 : 2;
    }
    else
        UseServerOfType = 1;

    if (UseServerOfType == 1) // gitee
    {
        if (!ReadURLDatas(URL_GITEE_INFO, content))
        {
            PrintLog("[Init] Gitee: 获取 赞助 列表失败", PL_ERROR);
            return 0;
        }
        else
        {
            string SpoonsText = ExtractString(content, "=spoons=", "=spoons-end=");
            SplitString(SpoonsText, "\n", SponsorList);
        }
        if (!ReadURLDatas(URL_GITEE_NOTICE, NoticeStr))
        {
            PrintLog("[Init] Gitee: 获取公告失败", PL_ERROR);
            return 0;
        }
        if (!ReadURLDatas(URL_GITEE_POPUP, content))
        {
            PrintLog("[Init] Gitee: 获取弹窗信息失败", PL_ERROR);
            return 0;
        }
        else
        {
            if (!content.empty() && content != "empty" && content != "null")
                RenderMessageBox(StringToUTF8(content), config.UseChinese ? u8"弹窗公告" : "Popup Notice", MB_OK);
        }
    }

    if(UseServerOfType == 2) // lzy
    {
        if (ReadURLDatas(URL_LZY_NOTICE, content) <= 0)
        {
            PrintLog("[Init] Lzy: 获取公告失败", PL_ERROR);
            return 0;
        }
        else
        {
            NoticeStr = ExtractString(content, "=n=", "=nend=");
            NoticeStr = RemoveSubString(NoticeStr, "<br />");
            NoticeStr = RemoveSubString(NoticeStr, "</span>");
            NoticeStr = RemoveSubString(NoticeStr, "<span style=\"color:#81bf7e\">");
            ReplaceString(NoticeStr, "&amp;", "&");
        }
        if (ReadURLDatas(URL_LZY_SPONS, content) <= 0)
        {
            PrintLog("[Init] Lzy: 获取赞助信息失败", PL_ERROR);
            return 0;
        }
        else
        {
            content = ExtractString(content, "=namelist=", "=namelistend=");
            content = RemoveSubString(content, "<br />");
            content = RemoveSubString(content, "</span>");
            content = RemoveSubString(content, "<span style=\"color:#81bf7e\">");
            ReplaceString(content, "&amp;", "&");
            // 写入锁
            lock_guard<shared_mutex> lock(SponsorListMutex);
            SplitString(content, "\n", SponsorList);
        }

        if (ReadURLDatas(URL_LZY_POPUP, content) <= 0)
        {
            PrintLog("[Init] Lzy: 获取弹窗信息失败", PL_ERROR);
            return 0;
        }
        else
        {
            string msg = ExtractString(content, "=popup=", "=popupend=");
            msg = RemoveSubString(msg, "<br />");
            msg = RemoveSubString(msg, "</span>");
            msg = RemoveSubString(msg, "<span style=\"color:#81bf7e\">");
            ReplaceString(msg, "&amp;", "&");
            if (msg != "empyt" && msg.size() > 2)
                RenderMessageBox(StringToUTF8(msg), u8"弹窗公告", MB_OK);
        }
    }
    {
        lock_guard<shared_mutex> lock(InitSuccessTimeMutex);
        if (InitSuccessTime == 0 || InitSuccessTime < GetNowMs())
            InitSuccessTime = GetNowMs();
    }
    return 0;
}

DWORD WINAPI Updata(LPVOID lpParam)
{
    while (UseServerOfType == -1)
        Sleep(200);
    
    PrintLog("[Updata] 检查版本", PL_INFO);
    string content;
    if (!ReadURLDatas((UseServerOfType == 2) ? URL_LZY_VER : URL_GITEE_UPDATA, content))
    {
        PrintLog("[Updata] 检查版本失败", PL_ERROR);
        return 2;
    }
    string NewVersionStr;
    string Updatalog;
    string DownloadURL;
    string ForceUpdata;
    bool IsForceUpdata = true;
    if (UseServerOfType == 2)
    {
        NewVersionStr = RemoveEnter     (ExtractString(content, "=ver=", "=verend="));
        Updatalog     = ExtractString   (content, "=udatalog=", "=udatalogend=");
        DownloadURL   = ExtractString   (content, "=downloadurl=", "=downloadurlend=");
        ForceUpdata   = ExtractString   (content, "=force=", "=forceend=");
        //后删除<br />，可以提升性能，因为没处理的HTML文件一堆<br />
        NewVersionStr = RemoveSubString(NewVersionStr, "<br />");
        DownloadURL   = RemoveSubString(DownloadURL, "<br />");
        DownloadURL   = ExtractString   (DownloadURL, "<span style=\"color:#81bf7e\">", "</span>");
        Updatalog     = RemoveSubString(Updatalog, "<br />");
        ForceUpdata   = RemoveSubString(ForceUpdata, "<br />");
        IsForceUpdata = ForceUpdata.find("1") != string::npos;
    }
    else
    {
        NewVersionStr = ExtractString(content, "<newver>", "</newver>");
        Updatalog     = ExtractString(content, "<udatalog>", "</udatalog>");
        DownloadURL   = ExtractString(content, "<downloadurl>", "</downloadurl>");
        IsForceUpdata = ExtractString(content, "<force>", "</force>").find("1") != string::npos;
    }
    int newVerH = 0;
    int newVerL = 0;
    int curVerH = 0;
    int curVerL = 0;
    {
        size_t dotPos = version.find('.');
        string majorVersion = version.substr(0, dotPos);
        string minorVersion = version.substr(dotPos + 1);
        try
        {
            curVerH = stoi(majorVersion);
            curVerL = stoi(minorVersion);
        }
        catch (...)
        {
            PrintLog("[Updata] version版本号格式错误:" + version, PL_ERROR);
            RenderTip(config.UseChinese ? u8"检查更新失败！" : "Check update failed!", 4.0f, PT_ERROR);
            return 1;
        }
    }
    {
        size_t dotPos = NewVersionStr.find('.');
        string majorVersion = NewVersionStr.substr(0, dotPos);
        string minorVersion = NewVersionStr.substr(dotPos + 1);
        try
        {
            newVerH = stoi(majorVersion);
            newVerL = stoi(minorVersion);
        }
        catch (...)
        {
            PrintLog("[Updata] NewVersion版本号格式错误:" + NewVersionStr, PL_ERROR);
            RenderTip(config.UseChinese ? u8"检查更新失败！" : "Check update failed!", 4.0f, PT_ERROR);
            return 2;
        }
    }

    if ((newVerL <= curVerL) && (newVerH <= curVerH))
    {
        PrintLog("[Updata] 当前版本已是最新版本。", PL_INFO);
        return 0;
    }

    PrintLog("等待窗口加载完后进行更新", PL_INFO);
    while (Starting) Sleep(100);
    PrintLog(("[Updata] 更新的版本号：" + NewVersionStr).c_str(), PL_INFO);
   
    map<string, UINT> button;
    map<string, ImVec4> colors;
    if (IsForceUpdata)
    {
        button[config.UseChinese ? u8"立即更新" : "Update now"] = IDOK;
        colors[config.UseChinese ? u8"立即更新" : "Update now"] = GuiColor[ImGuiCol_ButtonHovered];
    }
    else
    {
        button[config.UseChinese ? u8"现在更新" : "Update now"] = IDOK;
        button[config.UseChinese ? u8"下次一定" : "Later"] = IDCANCEL;
        colors[config.UseChinese ? u8"现在更新" : "Update now"] = GuiColor[ImGuiCol_ButtonHovered];
        colors[config.UseChinese ? u8"下次一定" : "Later"] = GuiColor[ImGuiCol_Button];
    }
    int lost_ver = newVerL - curVerL - 2;
    string msg = (config.UseChinese ? u8"发现新版本 " : "New version ") + NewVersionStr;
    if (newVerL - 2 >= curVerL)
    {
        if(config.UseChinese)
            msg += u8"\n&d你已经落后了 " + to_string(lost_ver) + u8" 个版本\n&f强烈建议更新！";
        else
            msg += "\n&dYou are behind " + to_string(lost_ver) + " versions\n&fIt is recommended to update!";
    }
    if (RenderMessageBoxEx(msg.c_str(), u8"更新提示", button, colors, true) == IDCANCEL)
    {
        RenderTip(config.UseChinese ? u8"你拒绝了更新\n在此版本遇到的BUG请勿反馈！" : "You refused the update\nPlease do not report bugs in this version!", 4.0f, PT_WARNING);
        PrintLog("[Updata] 用户拒绝更新，已经落后" + to_string(lost_ver) + "个版本", PL_WARNING);
        return 3;
    }

    string filename = DownloadPath + "\\" + DownloadName + NewVersionStr + ".exe";

    string temp_url;
    if (DownloadURL.find("lanzouu.com") == string::npos)
        temp_url = DownloadURL;
    else
        temp_url = URL_RESOLVE + DownloadURL;

    PrintLog("[Updata] 开始下载GC连点器到 " + filename, PL_INFO);
    bool IsDownloadSuccess = DownloadURLFiles(temp_url.c_str(), filename.c_str());
    if (_access(filename.c_str(), 0) != 0 || !IsDownloadSuccess)
    {
        RenderMessageBox(config.UseChinese ? u8"下载失败" : "Download failed", config.UseChinese ? u8"错误" : "Error", MB_OK);
        return -1;
    }

    char CurrentPath[MAX_PATH];

    if (GetCurrentDirectory(MAX_PATH, CurrentPath) == 0)
    {
        PrintLog("[Updata] 获取当前工作目录失败。", PL_ERROR);
        return -1;
    }
    string FilePath = CurrentPath;
    PrintLog(("[Updata] 当前GC连点器工作目录 " + FilePath).c_str(), PL_INFO);

    FilePath = (FilePath + "\\" + DownloadName + NewVersionStr + ".exe");
    MoveFile(filename.c_str(), FilePath.c_str());

    PrintLog("[Updata] 移动GC文件到 " + FilePath, PL_INFO);

    SHELLEXECUTEINFO ShExecInfo{};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = "runas"; // 以管理员身份运行
    ShExecInfo.lpFile = FilePath.c_str();
    ShExecInfo.lpParameters = ""; // 参数
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;

    // 提前关闭互斥量，防止被误检查为多开
    if (AntiMultiOpenMutex)
        CloseHandle(AntiMultiOpenMutex);
    AntiMultiOpenMutex = NULL; // 防止主函数再次关闭

    if (!ShellExecuteEx(&ShExecInfo))
    {
        PrintLog("[Updata] 无法打开更新后的GC连点器，错误代码：" + GetLastError() + string("文件：") + FilePath, PL_ERROR);
        MessageBox(NULL, config.UseChinese ? u8"无法打开更新后的GC连点器\n你可以手动打开新版本GC连点器。" : "Failed to open the updated GC Linker\nYou can manually open the new version of GameClicker.", config.UseChinese ? u8"错误" : "Error", MB_OK | MB_ICONERROR);
    }
    if (ShExecInfo.hProcess)
        CloseHandle(ShExecInfo.hProcess);

    GCExit = true;
    return 0;
}