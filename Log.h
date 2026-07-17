#pragma once
#include "Definitons.h"

using namespace std;

#define PL_INFO     0
#define PL_WARNING  1
#define PL_ERROR    2

#ifdef GC_DEBUG_CMD
static void SetConsoleColor(WORD color)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
}
#endif // GC_DEBUG_CMD

struct LogItem
{
    string str;
    UINT type = 0;
    time_t time = 0;
    LogItem(const string& str, UINT type, time_t time) : str(str), type(type), time(time) {}
    LogItem() = default;
};

static string LogFilePath;

mutex LogQueueMtx; // 更轻量的互斥锁
condition_variable LogQueueCv;  // 条件变量
static deque<LogItem> LogQueue; // erase开头元素和push back都比vector快！！！

void InitLogSystem(const string& filename);
void PrintLog(const string& str, UINT type);
static ULONGLONG GetFolderSize(const string& FolderPath);
static string FormatTime(time_t time);
static void WriteLogThread();
void DeleteOldLog();

void InitLogSystem(const string& filename)
{
    LogFilePath = filename;
    thread th1(WriteLogThread);
    th1.detach();
}

static void WriteLogThread()
{
    static fstream fLog(LogFilePath, ios::app);
    while (!GCExit)
    {
        if (LogQueue.size() > 0)
        {
            LogItem item;
            {
                unique_lock<mutex> lock(LogQueueMtx);
                // 条件等待：队列为空时释放锁并阻塞，直到被唤醒且队列非空
                LogQueueCv.wait(lock, [] { return !LogQueue.empty(); });

                // 取出队首元素
                item = move(LogQueue.front());
                LogQueue.pop_front();
            } // 锁在此处释放，不影响后续耗时的 IO 操作

            // 无锁区域：处理日志输出（耗时操作）
            string TypeStr = (item.type == PL_INFO) ? "info" : (item.type == PL_WARNING) ? "warning" : "error";
            string TimeStr = FormatTime(item.time);

            string text = "[" + TimeStr + "] " + TypeStr + " : " + item.str + "\n";
            fLog << text;
            fLog.flush();

#ifdef GC_DEBUG_CMD
            switch (item.type)
            {
            case PL_INFO:
                SetConsoleColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                break;
            case PL_WARNING:
                SetConsoleColor(FOREGROUND_GREEN | FOREGROUND_RED);
                break;
            case PL_ERROR:
                SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
                break;
            }
            cout << text;
            cout.flush();
#endif // GC_DEBUG_CMD

#ifdef GC_DEBUG_VS
            OutputDebugString(text.c_str());
#endif // GC_DEBUG_VS

            Sleep(1);
        }
        else
            Sleep(50); // 空闲时每秒20次
    }
}

static string FormatTime(time_t time)
{
    string TimeStr;
    tm ptm;
    localtime_s(&ptm, &time);

    TimeStr += to_string(ptm.tm_year + 1900);
    TimeStr += "/";
    TimeStr += to_string(ptm.tm_mon + 1);
    TimeStr += "/";
    TimeStr += to_string(ptm.tm_mday);
    TimeStr += " ";
    TimeStr += to_string(ptm.tm_hour);
    TimeStr += ":";
    TimeStr += to_string(ptm.tm_min);
    TimeStr += ":";
    TimeStr += to_string(ptm.tm_sec);
    return TimeStr;
}

void PrintLog(const string& str, UINT type)
{
    {
        lock_guard<mutex> lock(LogQueueMtx);  // 独占锁（RAII）
        LogQueue.push_back(LogItem(str, type, time(0)));
    }
    LogQueueCv.notify_one();  // 唤醒一个消费者线程
}

static ULONGLONG GetFolderSize(const string& FolderPath)
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(FolderPath.c_str(), &findFileData);

    ULONGLONG totalSize = 0;

    if (hFind == INVALID_HANDLE_VALUE)
    {
        PrintLog("[Log]无法获取Logs文件夹的大小", PL_ERROR);
        return 0;
    }

    do
    {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findFileData.nFileSizeLow;
            fileSize.HighPart = findFileData.nFileSizeHigh;
            totalSize += fileSize.QuadPart;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return totalSize;
}

void DeleteOldLog()
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(LogFilePath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        PrintLog("[Log]无法获取Logs文件夹的文件", PL_WARNING);

    do
    {
        LPCSTR filePath = findFileData.cFileName;
        WIN32_FILE_ATTRIBUTE_DATA fileAttrData;
        if (GetFileAttributesEx(filePath, GetFileExInfoStandard, &fileAttrData))
        {
            SYSTEMTIME fileTime;
            FileTimeToSystemTime(&fileAttrData.ftLastWriteTime, &fileTime);

            SYSTEMTIME currentTime;
            GetSystemTime(&currentTime);

            // 计算文件修改时间是否超过1天
            FILETIME currentTimeFileTime, fileTimeFileTime;
            SystemTimeToFileTime(&currentTime, &currentTimeFileTime);
            SystemTimeToFileTime(&fileTime, &fileTimeFileTime);

            ULARGE_INTEGER currentULL, fileULL;
            currentULL.LowPart = currentTimeFileTime.dwLowDateTime;
            currentULL.HighPart = currentTimeFileTime.dwHighDateTime;
            fileULL.LowPart = fileTimeFileTime.dwLowDateTime;
            fileULL.HighPart = fileTimeFileTime.dwHighDateTime;

            // 1天的纳秒数
            ULONGLONG oneDayNS = 24ULL * 60 * 60 * 1000 * 1000 * 10;
            if (currentULL.QuadPart - fileULL.QuadPart > oneDayNS)
            {
                // 删除文件
                if (!DeleteFile(filePath))
                    PrintLog("[Log]删除过时的日志文件 "s + filePath + " 时，出现错误", PL_ERROR);
                else
                    PrintLog("[Log]已删除过时的日志文件："s + filePath, PL_ERROR);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return;
}