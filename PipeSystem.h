#pragma once
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <psapi.h>

class AnonymousPipe
{
private:
    HANDLE hReadPipe;
    HANDLE hWritePipe;
    HANDLE hProcess;
    DWORD dwProcessId;

    void Cleanup()
    {
        if (hReadPipe != INVALID_HANDLE_VALUE && hReadPipe != NULL)
        {
            CloseHandle(hReadPipe);
            hReadPipe = INVALID_HANDLE_VALUE;
        }
        if (hWritePipe != INVALID_HANDLE_VALUE && hWritePipe != NULL)
        {
            CloseHandle(hWritePipe);
            hWritePipe = INVALID_HANDLE_VALUE;
        }
        if (hProcess != INVALID_HANDLE_VALUE && hProcess != NULL)
        {
            CloseHandle(hProcess);
            hProcess = INVALID_HANDLE_VALUE;
        }
        dwProcessId = 0;
    }

public:
    AnonymousPipe() : hReadPipe(INVALID_HANDLE_VALUE),
        hWritePipe(INVALID_HANDLE_VALUE),
        hProcess(INVALID_HANDLE_VALUE),
        dwProcessId(0){}

    ~AnonymousPipe()
    {
        Cleanup();
    }

    // 创建管道（父进程端）
    bool CreatePipe()
    {
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (!::CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0))
            return false;

        // 确保读句柄不被子进程继承
        if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0))
        {
            Cleanup();
            return false;
        }

        return true;
    }

    // 获取读句柄（用于父进程读取）
    HANDLE GetReadHandle() const
    {
        return hReadPipe;
    }

    // 获取写句柄（用于子进程写入）
    HANDLE GetWriteHandle() const
    {
        return hWritePipe;
    }

    // 发送数据
    bool Send(const std::string& data)
    {
        if (hWritePipe == INVALID_HANDLE_VALUE || hWritePipe == NULL)
            return false;

        DWORD bytesWritten;
        BOOL success = WriteFile(hWritePipe, data.c_str(), data.length(), &bytesWritten, NULL);
        if (!success || bytesWritten != data.length())
            return false;

        return true;
    }

    // 接收数据
    bool Receive(std::string& data, DWORD timeoutMs = INFINITE)
    {
        if (hReadPipe == INVALID_HANDLE_VALUE || hReadPipe == NULL)
            return false;

        // 检查管道是否有数据可读
        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvailable, NULL))
            return false;

        if (bytesAvailable == 0)
        {
            // 如果没有数据且设置了超时，等待数据
            if (timeoutMs != 0)
            {
                DWORD waitResult = WaitForSingleObject(hReadPipe, timeoutMs);
                if (waitResult != WAIT_OBJECT_0)
                    return false; // 超时或错误

                // 再次检查可用字节数
                if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvailable, NULL))
                    return false;
            }

            if (bytesAvailable == 0)
            {
                data.clear();
                return true; // 没有数据，但不是错误
            }
        }

        // 读取数据
        std::vector<char> buffer(bytesAvailable + 1);
        DWORD bytesRead;
        BOOL success = ReadFile(hReadPipe, buffer.data(), bytesAvailable, &bytesRead, NULL);
        if (!success)
            return false;

        buffer[bytesRead] = '\0';
        data.assign(buffer.data(), bytesRead);
        return true;
    }

    // 获取对方进程ID
    DWORD GetPeerProcessId() const
    {
        return dwProcessId;
    }

    // 检查管道是否仍然连接
    bool IsConnected()
    {
        if (hReadPipe == INVALID_HANDLE_VALUE || hReadPipe == NULL)
            return false;

        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvailable, NULL))
            return false;

        return true;
    }

    // 关闭写端（用于通知对方没有更多数据）
    void CloseWriteEnd()
    {
        if (hWritePipe != INVALID_HANDLE_VALUE && hWritePipe != NULL)
        {
            CloseHandle(hWritePipe);
            hWritePipe = INVALID_HANDLE_VALUE;
        }
    }

    // 关闭读端
    void CloseReadEnd()
    {
        if (hReadPipe != INVALID_HANDLE_VALUE && hReadPipe != NULL)
        {
            CloseHandle(hReadPipe);
            hReadPipe = INVALID_HANDLE_VALUE;
        }
    }
};