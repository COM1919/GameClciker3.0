#pragma once
#include "Definitons.h"
#include "ShareCode.h"

DWORD WINAPI NetworkClientThread(LPVOID lpParam);

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
        if (!client.Init())
        {
            PrintLog("[Server]WSA初始化失败", PL_ERROR);
            Sleep(1000);
            continue;
        }
        if (client.CreateSocket() != 0)
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
            string ip = client.ResolveDomain(ServerIP);
            if (ip == "0")
            {
                PrintLog("[Server] 服务器域名解析失败", PL_ERROR);
                return 1;
            }
            
            client.SetTarget(ip, ServerPort);
            err = client.Send("connect gc " + MachineCode + " " + version);
            if (err > 0)
            {
                string buf;
                int try_count = 16;
                while (client.Receive(buf) <= 0 && try_count > 0)
                    try_count--;
                // 修复：client_id 由网络线程写、主渲染线程每帧读（string 非原子），加锁保护
                {
                    lock_guard<mutex> lock(ClientIDMtx);
                    client_id = buf;
                }
                PrintLog("[Server] 当前客户端ID：" + client_id, PL_INFO);
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
            if (client.Receive(buf) > 0)
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
                    client.Send("online");
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
            int sleep = Random(1000 * 5, 1000 * 16);
            while (sleep >= 5)
            {
                if (IsGCExit)
                {
                    client.CloseSocket();
                    return 0;
                }
                Sleep(5);
                sleep -= 5;
            }
        }
    }
    return 0;
}