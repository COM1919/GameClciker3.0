#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

class UDPSocket
{
private:
    SOCKET sock;
    struct sockaddr_in server_addr {};
    static constexpr char PACKET_HEADER[] = "GCUDP";
    static constexpr int HEADER_SIZE = sizeof(PACKET_HEADER) - 1; // 减去null终止符
    // 修复：sock 成员被网络线程（NetworkClientThread）与主线程（退出时 Send("exit")）并发访问，
    // 加互斥锁保护。Send/SendTo 用 try_lock（拿不到锁就放弃发送——退出通知尽力而为，不阻塞退出）；
    // 网络线程内的操作（Receive/CreateSocket/CloseSocket）用 lock_guard（独占，无竞争时零开销）。
    mutable mutex sockMtx;

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

    // 初始化Winsock；成功返回 true。原失败日志（PrintLog）因循环依赖移至调用处（Network.h）处理。
    bool Init()
    {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    int CreateSocket()
    {
        lock_guard<mutex> lock(sockMtx);
        // 修复：重连时旧 socket 可能未关闭（NetworkClientThread 断开后不主动 CloseSocket），
        // 直接覆盖会泄漏系统资源。先自查关闭。
        if (sock != INVALID_SOCKET)
        {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        // 创建UDP socket
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET)
            return 1;

        return 0;
    }

    int SetTarget(const string& ip, int port)
    {
        lock_guard<mutex> lock(sockMtx);
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
        // try_lock：网络线程可能在 Receive/操作中，主线程（退出时）拿不到锁则放弃发送，不阻塞退出
        unique_lock<mutex> lock(sockMtx, try_to_lock);
        if (!lock.owns_lock() || sock == INVALID_SOCKET)
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
        unique_lock<mutex> lock(sockMtx, try_to_lock);
        if (!lock.owns_lock() || sock == INVALID_SOCKET)
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
        lock_guard<mutex> lock(sockMtx);
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
                return (int)str.size();   // 修复：显式转换消除 size_t→int 截断警告
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
        lock_guard<mutex> lock(sockMtx);
        if (sock == INVALID_SOCKET)
            return "-1";

        // 设置超时（可选）
        if (timeoutMs > 0)
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

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
        lock_guard<mutex> lock(sockMtx);
        if (sock != INVALID_SOCKET)
        {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        WSACleanup();
    }
};