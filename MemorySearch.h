#pragma once
#include "Definitons.h"
#include "ShareCode.h"

class MemorySearch
{
private:
    DWORD m_processID = 0;
    HANDLE m_hProcess = 0;
    bool Success = true;
    vector<void*> m_searchResults;

    inline bool m_OpenProcess()
    {
        m_hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, m_processID);
        if (!m_hProcess)
        {
            Success = false;
            return false;
        }
        return true;
    }

public:
    MemorySearch(DWORD processID) : m_processID(processID) { }

    bool LoadProcess()
    {
        return m_OpenProcess();
    }

    ~MemorySearch()
    {
        if (m_hProcess)
            CloseHandle(m_hProcess);
    }

    // 搜索内存中指定值
    int SearchMemory(const void* value, size_t size)
    {
        if (!Success) return -1;
        try
        {
            m_searchResults.clear();
            vector<void*> results;

            SYSTEM_INFO si;
            GetSystemInfo(&si);
            MEMORY_BASIC_INFORMATION mbi;
            void* address = (void*)si.lpMinimumApplicationAddress;

            while (address < si.lpMaximumApplicationAddress)
            {
                if (VirtualQueryEx(m_hProcess, address, &mbi, sizeof(mbi)) == 0)
                    break;

                if (mbi.State == MEM_COMMIT && (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_READONLY))
                {
                    vector<char> buffer(mbi.RegionSize);
                    SIZE_T bytesRead;
                    if (ReadProcessMemory(m_hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead))
                    {
                        for (size_t i = 0; i < bytesRead - size; ++i)
                        {
                            if (memcmp(&buffer[i], value, size) == 0)
                            {
                                void* foundAddress = (void*)((uintptr_t)mbi.BaseAddress + i);
                                results.push_back(foundAddress);
                            }
                        }
                    }
                }
                address = (void*)((uintptr_t)mbi.BaseAddress + mbi.RegionSize);
            }

            m_searchResults = results; // 保存搜索结果
            return results.size();
        }
        catch (const exception& e)
        {
            PrintLog("内存搜索出现错误:"s + e.what(), PL_ERROR);
            return -2;
        }
        return -3;
    }

    // 再次搜索已搜索过的内存
    int SearchMemoryAgain(const void* newValue, size_t size)
    {
        if (!Success) return -1;
        try
        {
            vector<void*> results;

            for (void* address : m_searchResults)
            {
                vector<char> buffer(size);
                SIZE_T bytesRead;
                if (ReadProcessMemory(m_hProcess, address, buffer.data(), size, &bytesRead) && bytesRead == size)
                {
                    if (memcmp(buffer.data(), newValue, size) == 0)
                        results.push_back(address);
                }
            }

            m_searchResults = results; // 保存搜索结果
            return results.size();
        }
        catch (const exception& e)
        {
            PrintLog("内存搜索出现错误:"s + e.what(), PL_ERROR);
            return -2;
        }
        return -3;
    }

    template<typename T>
    bool WriteMemoryValue(void* address, const T& value)
    {
        if (!Success || !m_hProcess)
            return false;

        SIZE_T bytesWritten;
        BOOL result = WriteProcessMemory(m_hProcess, address, &value, sizeof(value), &bytesWritten);

        if (result == 0 || bytesWritten != sizeof(value))
        {
            // 处理错误，例如打印错误日志
            PrintLog("写入内存失败，错误代码: " + GetLastError(), PL_ERROR);
            return false;
        }

        return true;
    }

    // 获取搜索的结果
    const vector<void*>& GetSearchResults() const
    {
        return m_searchResults;
    }

    const bool IsSuccess() const
    {
        return Success;
    }

    template<typename T>
    T GetAddressValue(void* address)
    {
        if (!Success) return 0;
        // 读取内存中的值
        T value{};
        SIZE_T bytesRead;
        BOOL result = ReadProcessMemory(m_hProcess, address, &value, sizeof(value), &bytesRead);

        if (!result && bytesRead != sizeof(value))
            PrintLog("读取内存失败，错误代码: " + GetLastError(), PL_ERROR);
        return value;
    }
};