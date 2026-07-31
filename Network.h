#pragma once
#include "Definitons.h"
#include "ShareCode.h"

DWORD WINAPI NetworkClientThread(LPVOID lpParam);

class UDPSocket
{
private:
    SOCKET sock;
    struct sockaddr_in server_addr {};
    static constexpr char PACKET_HEADER[] = "GCUDP";
    static constexpr int HEADER_SIZE = sizeof(PACKET_HEADER) - 1; // 减去null终止符

public:
    UDPSocket() : sock(INVALID_SOCKET) {};

    ~UDPSocket()
    {
        CloseSocket();
    }

    // 创建带有自定义头部的数据包
    vector<unsigned char> CreatePacket(const string& data) const
    {
        vector<unsigned char> packet;
        // 添加包头
        packet.insert(packet.end(), PACKET_HEADER, PACKET_HEADER + HEADER_SIZE);
        // 添加数据
        packet.insert(packet.end(), data.begin(), data.end());
        return packet;
    }

    // 解析数据包，返回数据和是否有效
    pair<string, bool> ParsePacket(const vector<unsigned char>& packet) const
    {
        if (packet.size() < HEADER_SIZE)
            return { "", false };

        // 检查包头
        if (equal(PACKET_HEADER, PACKET_HEADER + HEADER_SIZE, packet.begin()))
        {
            // 提取数据部分
            string data(packet.begin() + HEADER_SIZE, packet.end());
            return { data, true };
        }

        return { "", false };
    }

    static string ResolveDomain(const string& domain)
    {
        addrinfo hints{};
        addrinfo* res;
        char ipstr[INET6_ADDRSTRLEN];

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if (getaddrinfo(domain.c_str(), NULL, &hints, &res) != 0)
            return "0";

        // 遍历所有结果并获取第一个IPv4地址
        for (addrinfo* p = res; p != NULL; p = p->ai_next)
        {
            void* addr;

            if (p->ai_family == AF_INET)
            {
                // IPv4
                sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
                addr = &(ipv4->sin_addr);
                inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
                freeaddrinfo(res);
                return ipstr;
            }
        }

        freeaddrinfo(res);
        return "0";
    }

    void Init()
    {
        // 初始化Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            PrintLog("[Server]WSA初始化失败", PL_ERROR);
    }

    int CreateSocket()
    {
        // 创建UDP socket
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET)
            return 1;

        return 0;
    }

    int SetTarget(const string& ip, int port)
    {
        if (sock == INVALID_SOCKET)
            return 1;

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0)
            return 2;

        return 0;
    }

    // 使用缓存地址
    int Send(const string& message)
    {
        if (sock == INVALID_SOCKET)
            return -1;

        auto packet = CreatePacket(message);
        return sendto(sock,
            reinterpret_cast<const char*>(packet.data()),
            packet.size(), 0,
            reinterpret_cast<sockaddr*>(&server_addr),
            sizeof(server_addr));
    }

    // 手动指定地址
    int SendTo(const string& message, const string& ip, int port) const
    {
        if (sock == INVALID_SOCKET)
            return -1;

        sockaddr_in target_addr{};
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &target_addr.sin_addr);

        auto packet = CreatePacket(message);
        return sendto(sock,
            reinterpret_cast<const char*>(packet.data()),
            packet.size(), 0,
            reinterpret_cast<sockaddr*>(&target_addr),
            sizeof(target_addr));
    }

    int Receive(string& str, int timeoutMs = 6000) const
    {
        if (sock == INVALID_SOCKET)
            return -1;

        // 设置超时（可选）
        if (timeoutMs > 0)
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

        vector<unsigned char> buf(512);
        sockaddr_in from_addr{};
        int from_len = sizeof(from_addr);

        int len = recvfrom(sock,
            reinterpret_cast<char*>(buf.data()), buf.size(), 0,
            reinterpret_cast<sockaddr*>(&from_addr), &from_len);

        if (len > 0)
        {
            buf.resize(len);
            auto pack = ParsePacket(buf);
            if (pack.second)
            {
                str = pack.first;
                return str.size();
            }
            else
                return -2; // 无效的数据包（包头不匹配）
        }
        else if (len == 0)
            return -3; // 连接关闭
        else
            return -4; // 错误或超时
    }

    string ReceiveFrom(string& fromIp, int& fromPort, int timeoutMs = 0) const
    {
        if (sock == INVALID_SOCKET)
            return "-1";

        // 设置超时（可选）
        if (timeoutMs > 0)
        {
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
        }

        vector<unsigned char> buf(512);
        sockaddr_in from_addr{};
        int from_len = sizeof(from_addr);

        int len = recvfrom(sock,
            reinterpret_cast<char*>(buf.data()),
            buf.size(), 0,
            reinterpret_cast<sockaddr*>(&from_addr),
            &from_len);

        if (len > 0)
        {
            buf.resize(len);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(from_addr.sin_addr), ipStr, INET_ADDRSTRLEN);
            fromIp = ipStr;
            fromPort = ntohs(from_addr.sin_port);

            buf.resize(len);
            auto pack = ParsePacket(buf);
            if (pack.second)
                return pack.first;
            else
                return "-2"; // 无效的数据包（包头不匹配）
        }
        else if (len == 0)
            return "0"; // 连接关闭
        else
            return "-1"; // 错误或超时
    }

    void CloseSocket()
    {
        if (sock != INVALID_SOCKET)
        {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        WSACleanup();
    }
};

UDPSocket client;
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
    while (!GCExit)
    {
        PrintLog("[Server] 正在连接服务器...", PL_INFO);
        client.Init();
        if (client.CreateSocket() != 0)
        {
            PrintLog("[Server] 创建socket失败", PL_ERROR);
            Sleep(1000);
            continue;
        }

        ULONGLONG LastHBTime = 0;
        int err = 0;
        bool no_err = false;
        while (!GCExit)
        {
            string ip = client.ResolveDomain(ServerIP);
            if (ip == "0")
            {
                PrintLog("[Server] 服务器域名解析失败", PL_ERROR);
                RenderTip(u8"服务器域名解析失败，请检查网络连接或域名是否正确", 4.5f, PT_ERROR);
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
                client_id = buf;
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

                if (tokens[0] == "hb")
                {
                    client.Send("online");
                    client.Send("online"); // 发2次保险
                    LastHBTime = GetNowMs();
                    continue;
                }

                if (tokens[0] == "close")
                {
                    PrintLog("[Server] 服务器关闭，正在等待重新连接", PL_WARNING);
                    IsServerClosed = true;
                    continue;
                }

                if (!tokens.empty())
                {
                    if (tokens[0] == "msg")
                    {
                        if (tokens.size() >= 2)
                        {
                            PrintLog("[Server] 服务器发来消息:" + tokens[1], PL_WARNING);
                            RenderMessageBox(tokens[1], u8"服务器发来消息", MB_OK, false);
                            MessageBeep(MB_OK);
                        }
                    }
                    else if (tokens[0] == "abort")
                    {
                        PrintLog("[Server] 服务器要求强制退出", PL_WARNING);
                        MessageBeep(MB_ICONERROR);
                        GCExit = true;
                        break;
                    }
                }
            }
            else
            {
                PrintLog("[Server] 服务器已关闭，正在尝试重新连接", PL_WARNING);
                IsServerClosed = true;
            }
            Sleep(10);
        }
        if (!GCExit)
        {
            // 设置随机延迟，防止多个客户端同时请求
            unsigned int sleep = Random(1000 * 5, 1000 * 16);
            while (sleep >= 0)
            {
                if (GCExit)
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