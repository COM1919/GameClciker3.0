#pragma once
#include "Definitons.h"
#include "helperCode.h"

DWORD WINAPI NetworkClientThread(LPVOID lpParam);
bool DownloadURLFilesWithProgress(const string& url, const string& filename, const string& title);
bool DownloadURLFiles(const string& url, const string& filename);
int ReadURLDatas(const string& url, string& datas);
int ReadURLDatas(const string& url, vector<char>& datas);
bool ReadURLDatasRetry(const string& url, string& content, int retry);
bool DownloadURLFilesRetry(const string& url, const string& filename, int retry);

class InternetOpenManager
{
public:
    HINTERNET hInternet;
    HINTERNET hConnect;

    bool Init()
    {
        hInternet = InternetOpen("GameClicker3.0 InternetOpen", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet)
        {
            PrintLog("[Download] InternetOpen错误：" + to_string(GetLastError()), PL_ERROR);
            return false;
        }
        return true;
    }

    bool OpenURL(const string& url)
    {
        hConnect = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);

        if (!hConnect)
        {
            PrintLog("[Download] 无法打开URL " + url + "，错误：" + to_string(GetLastError()), PL_ERROR);
            InternetCloseHandle(hInternet);
            return false;
        }
        return true;
    }

    int InternetReadFile(LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead)
    {
        return ::InternetReadFile(hConnect, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
    }

    int ReadDatas(vector<char>& datas)
    {
        datas.clear();
        const int bufferSize = 4096;
        char buffer[bufferSize];
        DWORD bytesRead;
        while (InternetReadFile(buffer, bufferSize, &bytesRead) && bytesRead > 0)
            datas.insert(datas.end(), buffer, buffer + bytesRead);
        DWORD error = GetLastError();
        if (error != ERROR_SUCCESS && error != ERROR_NO_MORE_FILES)
        {
            PrintLog("[Download] 读取网络数据错误：" + to_string(error), PL_ERROR);
            return -1;
        }
        return static_cast<int>(datas.size());
    }

    int ReadString(string& str)
    {
        str.clear(); // 这里非常非常非常非常的重要
        const int bufferSize = 4096;
        char buffer[bufferSize];  // 不需要初始化全0
        DWORD bytesRead;
        while (InternetReadFile(buffer, bufferSize, &bytesRead) && bytesRead > 0)
            str.append(buffer, bytesRead);  // 只添加实际读取的字节数，而不是str += buffer;
        // 检查是否读取完成或因错误退出
        DWORD error = GetLastError();
        if (error != ERROR_SUCCESS && error != ERROR_NO_MORE_FILES)
        {
            PrintLog("[Download] 读取网络数据错误：" + to_string(error), PL_ERROR);
            return -1;
        }
        return static_cast<int>(str.size());// 以上的一切原先的代码都会导致读取的数据不完整，甚至结尾不是\0，，ReadURLAllDatas也是一样的
    }

    ~InternetOpenManager()
    {
        if (hConnect) InternetCloseHandle(hConnect);
        if (hInternet) InternetCloseHandle(hInternet);
    }
};

bool DownloadURLFilesWithProgress(const string& url, const string& filename, const string& title)
{
    InternetOpenManager internet;
    if (!internet.Init())
        return false;
    if (!internet.OpenURL(url))
        return false;

    // 获取文件总大小
    DWORD contentLength = 0;
    DWORD lengthSize = sizeof(contentLength);
    HttpQueryInfo(internet.hConnect, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &lengthSize, NULL);

    // 打开进度条
    {
        std::lock_guard<std::shared_mutex> lock(ProgressBarMsg::Mtx);
        ProgressBarMsg::IsOpen = true;
        ProgressBarMsg::ReturnCode = 0;
        ProgressBarMsg::TitleName = title;
        ProgressBarMsg::MaxSize = contentLength / 1024.0 / 1024.0; // MB
        ProgressBarMsg::ReadSize = 0.0;
    }

    FILE* file;
    fopen_s(&file, filename.c_str(), "wb");
    if (!file)
    {
        {
            std::lock_guard<std::shared_mutex> lock(ProgressBarMsg::Mtx);
            ProgressBarMsg::IsOpen = false;
        }
        return false;
    }

    const int bufferSize = 4096;
    char buffer[bufferSize]{};
    DWORD bytesRead;
    while (internet.InternetReadFile(buffer, bufferSize, &bytesRead) && bytesRead > 0)
    {
        std::lock_guard<std::shared_mutex> lock(ProgressBarMsg::Mtx);
        fwrite(buffer, 1, bytesRead, file);
        ProgressBarMsg::ReadSize += bytesRead / 1024.0 / 1024.0; // MB

        if (ProgressBarMsg::ReturnCode == 1)  // 用户点了取消
            break;
    }
    fclose(file);

    {
        std::lock_guard<std::shared_mutex> lock(ProgressBarMsg::Mtx);
        bool success = (ProgressBarMsg::ReturnCode != 1) && (ProgressBarMsg::ReadSize > 0);
        ProgressBarMsg::IsOpen = false;
        return success;
    }
}

bool DownloadURLFiles(const string& url, const string& filename)
{
    InternetOpenManager internet;
    if (!internet.Init())
        return false;
    if (!internet.OpenURL(url))
        return false;

    FILE* file;
    fopen_s(&file, filename.c_str(), "wb");
    if (!file)
    {
        PrintLog("[Download] 无法打开文件 " + filename + "，错误：" + to_string(GetLastError()), PL_ERROR);
        return false;
    }

    const int bufferSize = 4096;
    char buffer[bufferSize]{};
    DWORD bytesRead = 0;
    DWORD totalRead = 0;
    bool ok = true;
    while (internet.InternetReadFile(buffer, bufferSize, &bytesRead) && bytesRead > 0)
    {
        totalRead += bytesRead;
        if (fwrite(buffer, 1, bytesRead, file) != bytesRead)
        {
            ok = false;
            break;
        }
    }
    if (ferror(file) != 0)
        ok = false;
    if (ok && totalRead == 0)
        ok = false;
    fclose(file);

    // 修复：原代码无条件 return true——网络读取失败/写入失败也会报"已下载"，产生空或不完整的文件，
    // 后续 CopyFile/解压会在错误文件上继续。改为真实反映读取结果。
    if (ok)
    {
        PrintLog("[Download] 已下载" + url + " 到 " + filename, PL_INFO);
        return true;
    }
    PrintLog("[Download] 下载失败: " + url, PL_ERROR);
    DeleteFileA(filename.c_str());
    return false;
}

int ReadURLDatas(const string& url, string& datas)
{
    InternetOpenManager internet;
    if (!internet.Init())
        return -1;
    if (!internet.OpenURL(url))
        return -1;
    if (internet.ReadString(datas) < 0)
        return -1;
    return static_cast<int>(datas.size());
}

int ReadURLDatas(const string& url, vector<char>& datas)
{
    InternetOpenManager internet;
    if (!internet.Init())
        return -1;
    if (!internet.OpenURL(url))
        return -1;
    if (internet.ReadDatas(datas) < 0)
        return -1;
    return static_cast<int>(datas.size());
}

bool ReadURLDatasRetry(const string& url, string& content, int retry)
{
    for (int i = 0; i < retry; i++)
    {
        if (ReadURLDatas(url, content) > 0)
            return true;
        if (i + 1 < retry)
        {
            PrintLog("[ScriptMarket] 读取失败，重试 " + to_string(i + 1) + "/" + to_string(retry) + ": " + url, PL_WARNING);
            Sleep(400 * (i + 1));
        }
    }
    return false;
}

bool DownloadURLFilesRetry(const string& url, const string& filename, int retry)
{
    for (int i = 0; i < retry; i++)
    {
        if (DownloadURLFiles(url, filename))
            return true;
        if (i + 1 < retry)
        {
            PrintLog("[ScriptMarket] 下载失败，重试 " + to_string(i + 1) + "/" + to_string(retry) + ": " + url, PL_WARNING);
            Sleep(400 * (i + 1));
        }
    }
    return false;
}

DWORD WINAPI NetworkClientThread(LPVOID lpParam)
{
    while (UseServerOfType < 0)
        Sleep(100);
    string content;
    if (ReadURLDatas((UseServerOfType == 1) ? URL_LZY_CONNECT : URL_GITEE_CONNECT, content) > 0)
    {
        if (UseServerOfType == 1)
        {
            content = RemoveSubString(content, "<br />");
            content = ExtractString(content, "=connect=", "=connectend=");
        }
        AllowConnectServer = (content.find("true") != string::npos);
    }
    else
        AllowConnectServer = false;
    if (!AllowConnectServer)
        return 0;

    string MachineCode = GetMachineCode();
    while (!IsGCExit)
    {
        PrintLog("[Server] 正在连接服务器...", PL_INFO);
        // Init 已改为返回 bool（UDPSocket 类移入 UDPSocket.h，无法依赖 Log），失败日志在此处理
        if (!NetworkClient.Init())
        {
            PrintLog("[Server]WSA初始化失败", PL_ERROR);
            Sleep(1000);
            continue;
        }
        if (NetworkClient.CreateSocket() != 0)
        {
            PrintLog("[Server] 创建socket失败", PL_ERROR);
            Sleep(1000);
            continue;
        }

        ULONGLONG LastHBTime = 0;
        int err = 0;
        bool no_err = false;
        while (!IsGCExit)
        {
            string ip = NetworkClient.ResolveDomain(ServerIP);
            if (ip == "0")
            {
                PrintLog("[Server] 服务器域名解析失败", PL_ERROR);
                return 1;
            }
            
            NetworkClient.SetTarget(ip, ServerPort);
            err = NetworkClient.Send("connect gc " + MachineCode + " " + version);
            if (err > 0)
            {
                string str;
                int try_count = 16;
                while (NetworkClient.Receive(str) <= 0 && try_count > 0)
                    try_count--;

                string CurClientID;
                {
                    lock_guard<mutex> lock(ClientIDMtx);
                    ClientID = str;
                }

                PrintLog("[Server] 当前客户端ID：" + str, PL_INFO);
                break;
            }
            else
            {
                if (!no_err)
                {
                    PrintLog("[Server] 服务器连接失败，错误代码:" + to_string(err), PL_ERROR);
                    PrintLog("[Server] 将尝试重复连接服务器，直到成功", PL_INFO);
                    no_err = true;
                }
            }
        }

        PrintLog("[Server] 服务器连接成功！", PL_INFO);

        LastHBTime = GetNowMs();
        IsServerClosed = false;
        while (!IsServerClosed)
        {
            if (!AllowConnectServer)
                return 0;

            if (GetNowMs() - LastHBTime > (ULONGLONG)40 * 1000) // 40秒
            {
                PrintLog("[Server] 服务器太久没有发送心跳·消息，已视为服务器关闭", PL_WARNING);
                IsServerClosed = true;
                break;
            }

            // 服务器发送的默认就是UTF-8
            string buf;
            if (NetworkClient.Receive(buf) > 0)
            {
#ifdef GC_SERVER_DEBUG
                PrintLog("[Server] 来自服务器消息:" + buf, PL_WARNING);
#endif //GC_SERVER_DEBUG

                vector<string> tokens;
                SplitString(buf, " ", tokens);
                // 修复：buf 可能为纯空白/无效内容（SplitString 返回空列表），tokens[0] 会越界崩溃
                if (tokens.empty())
                    continue;

                if (tokens[0] == "hb")
                {
                    NetworkClient.Send("online");
                    LastHBTime = GetNowMs();
                    continue;
                }

                if (tokens[0] == "close")
                {
                    PrintLog("[Server] 服务器关闭，正在等待重新连接", PL_WARNING);
                    IsServerClosed = true;
                    continue;
                }

                if (tokens[0] == "msg")
                {
                    if (tokens.size() >= 2)
                    {
                        RenderMessageBox(tokens[1], u8"服务器发来消息", MB_OK, false);
                        MessageBeep(MB_OK);
                    }
                }
                else if (tokens[0] == "abort")
                {
                    IsGCExit = true;
                    break;
                }
            }
            Sleep(10);
        }
        if (!IsGCExit)
        {
            // 设置随机延迟，防止多个客户端同时请求
            RandomGenerator rng;
            int sleep = rng.Random(1000 * 5, 1000 * 16);
            while (sleep >= 5)
            {
                if (IsGCExit)
                {
                    NetworkClient.CloseSocket();
                    return 0;
                }
                Sleep(5);
                sleep -= 5;
            }
        }
    }
    return 0;
}