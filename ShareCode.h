#pragma once
#include "maths.h"
#include "zlib.h"
#include "zip.h"
#include "ConfigParser.h"
#include "CodeModules.h"
#include "Log.h"
#include <zlib.h>
#include <unzip.h>
#include <iomanip>
#include <sstream>
#include <psapi.h>
#include <algorithm>
#include <TlHelp32.h>
#include <malloc.h>
#pragma comment(lib, "psapi.lib")

#define PT_INFO 1
#define PT_WARNING 2
#define PT_ERROR 3

string GetQQNumber();
bool IsNumeric(const string& str);
int RenderMessageBoxEx(string message, const char* title, map<string, UINT> button, map<string, ImVec4> colors, bool wating);
int RenderMessageBox(string message, const char* title, UINT type, bool wating);
void RenderTip(string msg, float timeout, UINT type);
void FindFilesWithExtension(const string& directory, const string& extension, vector<string>& files);
bool IsContainsString(const string& str, const vector<string>& vec);
string GetOpenFileName();
string StringToUTF8(const string& str);
string UTF8ToString(const string& str);
bool CreateDirRecursive(const string& path);
bool CheckScriptPermission(const string& filename, vector<string>& BeUsedFuncs);
int GetMemUsageRate(int sleep);
int GetGpuUsageRate(int sleep);
int GetCpuUsageRate(int sleep);
void FindDirectorys(const string& root, vector<string>& dirs);
double GetThreadCpuUsage(HANDLE hThread, DWORD sleep = 1000);
DWORD GetSystemLanguage();
bool DownloadURLFiles(const string& url, const string& filename);
bool DownloadURLFilesWithProgress(const string& url, const string& filename, const string& title);
bool ReadURLDatas(const string& url, vector<char>& datas);
bool ReadURLDatas(const string& url, string& datas);
void GetStringFileName(string& str, bool KeepSuffix = false);
void PackageLogFile();
bool IsCoveredWindow(HWND hwnd, float rate = 0.8f);
void SetClassStyle(bool IsEnable);
bool ZipFiles(const vector<pair<string, string>>& files, const string& zipFilename);
int UnZipFile(const char* zipFilePath, const char* destDir);
bool CopyTextToClipboard(const string& text);
void ClearProcessMemory();
string SelectFile(const char* filter);
void UpdateScriptFromShop();
void ShaderLog(string str);
void CheckUpdateScriptFromShop(ScriptInfo& script);
bool AddToStartup(const string& file = "0");
bool RemoveStartup(const string& file = "0");
bool AddFollowStartup(const string& file);
int RefushScriptShopList(vector<ScriptInfo>& scripts, int paper, bool clear);
bool RemoveFollowStartup();
bool DeleteDirectoryRecursively(const string& dirPath);
static DWORD WINAPI SafeThreadEntry(LPVOID lpContext);
HANDLE CreateThreadSafe(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPDWORD lpThreadId, const string& name);
bool IsCursorHidden();
void RefushScriptList();
void GetScriptsInfo(vector<ScriptInfo>& ScriptList, const string& TempPath);
string GetProcessPathFromPID(DWORD processId);

void CheckUpdateScriptFromShop(ScriptInfo& script)
{
	for (const ScriptInfo& NewScript : ScriptShopList)
	{
		if (NewScript.name == script.name)
		{
			if (NewScript.ver > script.ver)
				script.IsHasUpdate = true;
			break;
		}
	}
}

void PlayEnableSound()
{
	PlaySound(Sound_Enable.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

void PlayDisableSound()
{
	PlaySound(Sound_Disable.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

bool DeleteDirectoryRecursively(const string& dirPath)
{
	string searchPath = dirPath + "\\*.*";
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE) // 如果文件夹为空或无法访问，尝试直接删除目录本身
		return RemoveDirectory(dirPath.c_str()) == TRUE;

	do
	{
		const string fileName = findData.cFileName;
		// 跳过当前目录 "." 和上级目录 ".."
		if (fileName == "." || fileName == "..")
			continue;

		string fullPath = dirPath + "\\" + fileName;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// 如果是子目录，递归删除
			if (!DeleteDirectoryRecursively(fullPath))
			{
				FindClose(hFind);
				return false;
			}
		}
		else
		{
			// 如果是文件，直接删除
			// 可以先移除只读等属性
			SetFileAttributes(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
			if (!DeleteFile(fullPath.c_str()))
			{
				PrintLog("[File] 删除文件夹失败: " + fullPath + "，错误: " + to_string(GetLastError()), PL_ERROR);
				FindClose(hFind);
				return false;
			}
		}
	} while (FindNextFile(hFind, &findData) != 0);
	FindClose(hFind);

	// 最后，删除现在应该为空的父目录
	if (!RemoveDirectory(dirPath.c_str()))
	{
		PrintLog("[File] 删除文件夹失败: " + dirPath + "，错误: " + to_string(GetLastError()), PL_ERROR);
		return false;
	}

	PrintLog("[File] 删除文件夹成功: " + dirPath, PL_ERROR);
	return true;
}

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
		return datas.size();
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
		return str.size();// 以上的一切原先的代码都会导致读取的数据不完整，甚至结尾不是\0，，ReadURLAllDatas也是一样的
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
	pb_message.open = true;
	pb_message.ret_code = 0;
	pb_message.title = title;
	pb_message.maxmb = contentLength / 1024.0 / 1024.0; // MB
	pb_message.mb = 0.0;

	FILE* file;
	fopen_s(&file, filename.c_str(), "wb");
	if (!file)
	{
		pb_message.open = false;
		return false;
	}

	const int bufferSize = 4096;
	char buffer[bufferSize]{};
	DWORD bytesRead;
	while (internet.InternetReadFile(buffer, bufferSize, &bytesRead) && bytesRead > 0)
	{
		fwrite(buffer, 1, bytesRead, file);
		pb_message.mb += bytesRead / 1024.0 / 1024.0; // MB

		if (pb_message.ret_code == 1)  // 用户点了取消
			break;
	}
	fclose(file);

	bool success = (pb_message.ret_code != 1) && (pb_message.mb > 0);
	pb_message.open = false;

	return success;
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
	DWORD bytesRead;
	while (internet.InternetReadFile(buffer, bufferSize, &bytesRead) && bytesRead > 0)
		fwrite(buffer, 1, bytesRead, file);
	fclose(file);

	PrintLog("[Download] 已下载" + url + " 到 " + filename, PL_INFO);
	return true;
}

bool ReadURLDatas(const string& url, string& datas)
{
	InternetOpenManager internet;
	if (!internet.Init())
		return false;
	if (!internet.OpenURL(url))
		return false;
	internet.ReadString(datas);
	return datas.size();
}

bool ReadURLDatas(const string& url, vector<char>& datas)
{
	InternetOpenManager internet;
	if (!internet.Init())
		return false;
	if (!internet.OpenURL(url))
		return false;
	internet.ReadDatas(datas);
	return datas.size();
}

int RefushScriptShopList(vector<ScriptInfo>& scripts, int paper, bool clear) // 返回总页数
{
	static vector<string> ScriptList;
	if (ScriptList.empty() || clear)
	{
		string content;
		if (!ReadURLDatas(URL_GITEE_SCRIPTS, content))
		{
			PrintLog("[Init] Gitee: 获取脚本列表失败", PL_ERROR);
			return 0;
		}
		SplitString(content, "\r\n", ScriptList);
	}

	const int PaperSize = 10; // 一页10个
	const int BeginCount = (paper - 1) * PaperSize;
	const int EndCount = BeginCount + PaperSize;
	scripts.clear();
	int count = 0;
	for (const string& script : ScriptList)
	{
		count++;
		if (BeginCount >= count || EndCount < count)
			continue;
		const string filename = ScriptShopDir + "\\" + script + ".gcdat";
		// 如果文件已存在，直接跳过下载
		if (_access(filename.c_str(), 0) != 0)
		{
			const string url = URL_GITEE_SC_ROOT + script + ".zip";
			if (!DownloadURLFiles(url, filename))
			{
				PrintLog("[Script] Gitee: 下载脚本失败: " + url, PL_WARNING);
				continue;
			}
		}
		ScriptInfo info;
		info.file = filename;
		scripts.push_back(info);
	}
	GetScriptsInfo(scripts, ScriptShopTemp);
	for (const ScriptInfo& info : scripts)
		rename(info.file.c_str(), (ScriptShopDir + "\\" + info.name).c_str()); // 防止文件名和脚本名字不同，所以强制文件名字为脚本名字
	return (ScriptList.size() + PaperSize - 1) / PaperSize; // 这是向上取整的经典算法：(总数 + 每页大小 - 1) / 每页大小
}

// 定义一个结构体来打包所有需要传递给线程的信息
struct ThreadContext
{
	LPTHREAD_START_ROUTINE ThreadFunc; // 的线程函数
	LPVOID Parameter;                  // 线程函数的参数
	string name;                       // 线程名称，方便调试
};

static DWORD WINAPI SafeThreadEntry(LPVOID lpContext)
{
	ThreadContext* pContext = static_cast<ThreadContext*>(lpContext);
	if (!pContext || !pContext->ThreadFunc)
		return -1919; // 无效上下文
	DWORD ExitCode = 0;
	try
	{
		ExitCode = pContext->ThreadFunc(pContext->Parameter);
		PrintLog("[Thread] " + pContext->name + "线程结束", PL_INFO);
	}
	catch (const exception& e)
	{
		ExceptionHandling(GetLastError(), e.what() + " - 位置：["s + pContext->name + "]");
	}
	catch (...)
	{
		ExceptionHandling(GetLastError(), "未知 - 位置：["s + pContext->name + "]");
	}
	delete pContext;
	pContext = nullptr;
	return ExitCode;
}

HANDLE CreateThreadSafe(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPDWORD lpThreadId, const string& name)
{
	// 在堆上创建上下文，确保新线程能访问到
	ThreadContext* pContext = new ThreadContext();
	pContext->ThreadFunc = lpStartAddress;
	pContext->Parameter = lpParameter;
	pContext->name = name;
	HANDLE hThread = CreateThread(NULL, 0, SafeThreadEntry, pContext, 0, lpThreadId);
	if (hThread == NULL)
	{
		delete pContext;
		pContext = nullptr;
		PrintLog("[Thread] " + name + "线程创建失败", PL_ERROR);
	}
	else
		PrintLog("[Thread] " + name + "线程已启动", PL_INFO);
	return hThread;
}

void ClearProcessMemory()
{
	// 强制 CRT 释放未使用的堆内存回操作系统
	size_t freed = _heapmin();
	PrintLog("[Mem] 已强制 CRT 释放 " + to_string(freed) + "kb 未使用的堆内存", PL_INFO);

	// 获取当前进程句柄
	HANDLE hProcess = GetCurrentProcess();
	// 使用 -1 和 -1 作为参数，表示将工作集缩减到最小值
	// 这会尽可能多地修剪进程的物理内存占用
	SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1);
	PrintLog("[Mem] 已强制缩减工作集，来减少内存使用", PL_INFO);
}

string SelectFile(const char* filter)
{
	OPENFILENAMEA ofn;       // 公共对话框结构
	CHAR szFile[260] = { 0 }; // 用于接收文件名的缓冲区

	// 初始化 OPENFILENAME 结构
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetActiveWindow(); // 设置父窗口句柄
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1; // 默认显示第一个过滤器
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL; // 可以设置初始目录，例如 "C:\\"
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	// 显示“打开”对话框
	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		// 用户选择了文件，返回文件路径
		return ofn.lpstrFile;
	}

	// 用户取消了操作
	return "";
}

bool CheckScriptPermission(const string& filename, vector<string>& BeUsedFuncs)
{
	// 一行一行
	vector<string> script;
	{
		// 打开文件
		ifstream file(filename);
		if (!file.is_open())
		{
			PrintLog("[ScriptCheckPerm] 无法打开脚本文件: " + filename, PL_ERROR);
			return false; // 取消脚本继续加载
		}
		for (string line; getline(file, line);)
			script.push_back(line);
		file.close();
	}
	// 查找字符串，判断是否调用需要权限的函数
	// Lua注释状态
	bool inSingleComment = false;      // 单行注释状态（实际上每行重新开始）
	bool inMultiLineComment = false;   // 多行注释状态
	bool inString = false;             // 是否在字符串中
	bool inLongString = false;         // 是否在多行长字符串中
	char stringDelimiter = '\0';       // 当前字符串的分隔符 ' 或 "
	int longStringLevel = 0;           // 长字符串的等级 [[]] = 0, [=[ ]=] = 1, 等等

	// 遍历每一行
	for (size_t lineNum = 0; lineNum < script.size(); lineNum++)
	{
		const string& currentLine = script[lineNum];
		// 重置单行注释状态（每行开始时不在单行注释中）
		inSingleComment = false;

		// 遍历当前行的每个字符
		for (size_t i = 0; i < currentLine.length(); i++)
		{
			char c = currentLine[i];
			char next = (i + 1 < currentLine.length()) ? currentLine[i + 1] : '\0';

			// ============================================ 处理多行注释开始 ============================================
			if (!inString && !inLongString && !inSingleComment && !inMultiLineComment)
			{
				// 检查是否是 --[[ 或 --[=[ 等多行注释开始
				if (c == '-' && next == '-')
				{
					// 检查后面是否是长字符串或多行注释
					if (i + 2 < currentLine.length() && currentLine[i + 2] == '[')
					{
						// 找到 [ 的位置
						size_t bracketPos = i + 2;
						int level = 0;

						// 计算长括号的等级
						while (bracketPos < currentLine.length() && currentLine[bracketPos] == '=')
						{
							level++;
							bracketPos++;
						}

						// 检查是否是开括号
						if (bracketPos < currentLine.length() && currentLine[bracketPos] == '[')
						{
							inMultiLineComment = true;
							longStringLevel = level;
							i = bracketPos; // 跳过已处理的部分
							continue;
						}
					}
					// 如果是普通单行注释
					else
					{
						inSingleComment = true;
						i++; // 跳过下一个字符
						continue;
					}
				}
			}

			// ============================================ 处理多行注释结束 ============================================
			if (inMultiLineComment && !inString && !inLongString)
			{
				// 检查是否是 ]=] 或 ]] 等多行注释结束
				if (c == ']')
				{
					// 检查匹配的长括号结束
					size_t pos = i;
					int level = 0;

					// 计算 = 的数量
					while (pos + 1 < currentLine.length() && currentLine[pos + 1] == '=')
					{
						level++;
						pos++;
					}

					// 检查是否是匹配的结束括号
					if (pos + 1 < currentLine.length() && currentLine[pos + 1] == ']' && level == longStringLevel)
					{
						inMultiLineComment = false;
						longStringLevel = 0;
						i = pos + 1; // 跳过已处理的部分
						continue;
					}
				}
			}

			// ============================================ 处理字符串开始 ============================================
			if (!inMultiLineComment && !inSingleComment)
			{
				// 普通字符串开始 ' 或 "
				if (!inString && !inLongString && (c == '\'' || c == '"'))
				{
					inString = true;
					stringDelimiter = c;
					continue;
				}

				// 长字符串开始 [[ 或 [=[ 等
				if (!inString && !inLongString && c == '[')
				{
					// 检查是否是长字符串
					int level = 0;
					size_t pos = i + 1;

					// 计算 = 的数量
					while (pos < currentLine.length() && currentLine[pos] == '=')
					{
						level++;
						pos++;
					}

					// 检查是否是开括号
					if (pos < currentLine.length() && currentLine[pos] == '[')
					{
						inLongString = true;
						longStringLevel = level;
						i = pos; // 跳过已处理的部分
						continue;
					}
				}

				// ============================================ 处理字符串结束 ============================================
								// 普通字符串结束
				if (inString && c == stringDelimiter)
				{
					// 检查是否是转义字符
					if (i == 0 || currentLine[i - 1] != '\\')
					{
						inString = false;
						stringDelimiter = '\0';
					}
					continue;
				}

				// 长字符串结束
				if (inLongString && c == ']')
				{
					// 检查匹配的长括号结束
					size_t pos = i;
					int level = 0;

					// 计算 = 的数量
					while (pos + 1 < currentLine.length() && currentLine[pos + 1] == '=')
					{
						level++;
						pos++;
					}

					// 检查是否是匹配的结束括号
					if (pos + 1 < currentLine.length() && currentLine[pos + 1] == ']' && level == longStringLevel)
					{
						inLongString = false;
						longStringLevel = 0;
						i = pos + 1; // 跳过已处理的部分
						continue;
					}
				}
			}

			// ============================================ 检查限制函数（不在注释和字符串中） ============================================
			if (!inMultiLineComment && !inSingleComment && !inString && !inLongString)
			{
				// 检查每个限制函数
				for (const string& func : ScriptRestrictedFuncs)
				{
					// 当前位置开始的子串
					if (currentLine.substr(i, func.length()) == func)
					{
						// 确保是完整的函数名（后面跟着非字母数字字符或括号）
						size_t afterFuncPos = i + func.length();

						// 检查函数名边界
						bool isWordStart = (i == 0) || !isalnum(currentLine[i - 1]);
						bool isWordEnd = (afterFuncPos >= currentLine.length()) || !isalnum(currentLine[afterFuncPos]);

						if (isWordStart && isWordEnd)
						{
							// 进一步检查是否是函数调用（后面有括号）
							// 跳过可能的空格
							size_t checkPos = afterFuncPos;
							while (checkPos < currentLine.length() && (currentLine[checkPos] == ' ' || currentLine[checkPos] == '\t'))
								checkPos++;

							// 检查是否有括号
							if (checkPos < currentLine.length() && currentLine[checkPos] == '(')
							{
								BeUsedFuncs.push_back(func);
								PrintLog("[ScriptCheckPerm] 脚本 " + filename + " 使用了需要权限的函数: " + func, PL_WARNING);
								PrintLog("[ScriptCheckPerm] 行 " + to_string(lineNum + 1) + ": " + currentLine, PL_WARNING);
								return false;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

void GetScriptsInfo(vector<ScriptInfo>& ScriptList, const string& TempPath)
{
	if (_access(TempPath.c_str(), 0) != 0)
		CreateDirectory(TempPath.c_str(), NULL);

	vector<ScriptInfo> TempList;
	for (const ScriptInfo& info : ScriptList)
	{
		// 此时script.file是压缩包路径，最终我们要将其修改为解压后的lua文件路径
		string filename = info.file;
		GetStringFileName(filename); // 移除后缀

		ScriptInfo script;
		script.name = filename;

		string dir = TempPath + "\\" + filename;
		if (_access(dir.c_str(), 0) != 0)
			CreateDirectory(dir.c_str(), NULL);
		int ret = UnZipFile(info.file.c_str(), dir.c_str());
		if (ret != 0)
		{
			PrintLog("[Script] 解压文件失败，错误代码：" + ret, PL_ERROR);
			RenderTip((config.UseChinese ? u8"解压文件失败，错误代码："s + to_string(ret) : "Failed to extract file, error code: "s + to_string(ret)), 2.0f, PT_ERROR);
			MessageBeep(MB_ICONERROR);
			continue;
		}

		string BmpFile = dir + "\\" + "icon.bmp";
		{
			if (_access(BmpFile.c_str(), 0) != 0)
			{
				string str = dir + "\\" + "icon.png";
				if (_access(str.c_str(), 0) == 0)
					BmpFile = str;
				str = dir + "\\" + "icon.jpg";
				if (_access(str.c_str(), 0) == 0)
					BmpFile = str;
			}
		}

		string InfoFile = dir + "\\" + "info.ini";
		string motd;
		string author;
		string title;
		{
			char* buffer = new char[MAX_BUF];

			GetPrivateProfileString("script", "author", config.UseChinese ? u8"无" : "None", buffer, MAX_BUF, InfoFile.c_str());
			author = buffer;
			GetPrivateProfileString("script", "motd", config.UseChinese ? u8"什么都没有" : "Nothing", buffer, MAX_BUF, InfoFile.c_str());
			motd = buffer;
			//移除字符串中的换行符
			{
				size_t pos = 0;
				while ((pos = motd.find("\n", pos)) != string::npos)
					motd.erase(pos, 1);
			}
			GetPrivateProfileString("script", "title", info.name.c_str(), buffer, MAX_BUF, InfoFile.c_str());
			title = buffer;
			script.ver = GetPrivateProfileInt("script", "ver", 1, InfoFile.c_str());
			
			GetPrivateProfileString("script", "link", "", buffer, MAX_BUF, InfoFile.c_str());
			if (strlen(buffer) > 0)
			{
				script.link = buffer;
				if (script.link.empty() || script.link.find("https://") == string::npos)
					script.link.clear();
#ifdef GC_MORE_DEBUG_INFO
				PrintLog("[Script] 脚本" + script.name + "的链接为：" + script.link, PL_INFO);
#endif
			}

			delete[] buffer;
			buffer = nullptr;
		}
		vector<string> files;
		FindFilesWithExtension(dir, "lua", files);
		if (files.size() > 1)
		{
			PrintLog("[Script] 在脚本 " + filename + "发现多个lua文件，无法正确识别！", PL_ERROR);
			continue;
		}
		else if (files.empty())
		{
			PrintLog("[Script] 在脚本 " + filename + "没有发现lua文件", PL_ERROR);
			continue;
		}
		script.info = StringToUTF8(motd + (config.UseChinese ? u8"\n&7脚本作者：" : "\nAuthor: ") + author);
		script.file = files.at(0);
		script.name = StringToUTF8(filename); //StringToUTF8默认有try
		script.title = StringToUTF8(title);
		script.texture = BmpFile;

		PrintLog("[Script] 找到脚本文件: " + script.file, PL_INFO);
		TempList.push_back(script);

		CheckScriptPermission(script.file, script.RestrictedFuncs);
		const int count = script.RestrictedFuncs.size();
		for (int i = 0; i < count; i++) // 不能是<=，因为size比实际大1
		{
			if (i != 0) // 直接使用=，相当于开头就清空了RestrictedFuncsStr
				script.RestrictedFuncsStr = ", ";
			script.RestrictedFuncsStr += script.RestrictedFuncs[i];
		}
	}
	ScriptList.clear();
	ScriptList = TempList;
}

void RefushScriptList()
{
	{
		vector<string> files;
		FindFilesWithExtension(ScriptPath, "zip", files);
		vector<ScriptInfo> scripts;
		for (string& file : files)
		{
			ScriptInfo script;
			script.file = file;
			scripts.push_back(script);
		}
		GetScriptsInfo(scripts, ScriptTemp);
		// 写入锁
		lock_guard<shared_mutex> lock(AvaiableScriptsMtx);
		AvaiableScripts = scripts;
	}
	PrintLog("[Script] 刷新脚本列表成功，共 " + to_string(AvaiableScripts.size()) + " 个脚本文件", PL_INFO);
}

bool CopyTextToClipboard(const string& text)
{
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
		if (hMem)
		{
			char* pchData = (char*)GlobalLock(hMem);
			strcpy_s(pchData, text.size() + 1, text.c_str());
			GlobalUnlock(hMem);
			SetClipboardData(CF_TEXT, hMem);
		}
		CloseClipboard();
		return true;
	}
	return false;
}

void SetClassStyle(bool IsEnable)
{
	BOOL isCompositionEnabled;
	if (SUCCEEDED(DwmIsCompositionEnabled(&isCompositionEnabled)) && isCompositionEnabled)
	{
		// 扩展窗口框架到客户区域
		MARGINS margins = { -1, -1, -1, -1 };
		DwmExtendFrameIntoClientArea(GCHWND, &margins);

		// 启用客户区域的模糊效果
		DWM_BLURBEHIND bb = { 0 };
		bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
		bb.fEnable = IsEnable;
		bb.hRgnBlur = NULL; // 使用整个客户区域
		DwmEnableBlurBehindWindow(GCHWND, &bb);

		// 设置系统背景类型
		UINT attribute = DWMSBT_MAINWINDOW;
		DwmSetWindowAttribute(GCHWND, DWMWA_SYSTEMBACKDROP_TYPE, &attribute, sizeof(attribute));

		// 设置窗口角落在透明效果下的偏好
		DwmSetWindowAttribute(GCHWND, DWMWA_WINDOW_CORNER_PREFERENCE, &attribute, sizeof(attribute));
	}
}

bool ZipFiles(const vector<pair<string, string>>& files, const string& zipFilename)
{
	zipFile zf = zipOpen(zipFilename.c_str(), APPEND_STATUS_CREATE);
	if (!zf)
	{
		PrintLog("无法创建ZIP文件", PL_ERROR);
		return false;
	}

	for (const auto& file : files)
	{
		const char* srcFilename = file.first.c_str();
		const char* zipEntryName = file.second.c_str();

		zip_fileinfo zi{};
		zi.tmz_date.tm_year = 0;
		zi.tmz_date.tm_mon = 0;
		zi.tmz_date.tm_mday = 0;
		zi.tmz_date.tm_hour = 0;
		zi.tmz_date.tm_min = 0;
		zi.tmz_date.tm_sec = 0;
		zi.dosDate = 0;
		zi.internal_fa = 0;
		zi.external_fa = 0;

		int err = zipOpenNewFileInZip(zf, zipEntryName, &zi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
		if (err != ZIP_OK)
		{
			zipClose(zf, NULL);
			PrintLog("无法在ZIP文件中打开新文件 " + string(zipEntryName) + ", 错误码: " + to_string(err), PL_ERROR);
			return false;
		}

		ifstream src(srcFilename, std::ios::binary);
		if (!src.is_open())
		{
			zipCloseFileInZip(zf);
			zipClose(zf, NULL);
			PrintLog("无法打开源文件 " + string(srcFilename), PL_ERROR);
			return false;
		}

		char buffer[1024];
		while (src.read(buffer, sizeof(buffer)))
			zipWriteInFileInZip(zf, buffer, src.gcount());

		if (src.gcount() > 0)
			zipWriteInFileInZip(zf, buffer, src.gcount());

		src.close();
		zipCloseFileInZip(zf);
	}

	zipClose(zf, NULL);
	return true;
}

void GetStringFileName(string& str, bool KeepSuffix)
{
	if (!KeepSuffix)
	{
		size_t dotPosition = str.find('.');
		if (dotPosition != string::npos)
			str.erase(dotPosition);
	}
	size_t lastBackslashPos = str.rfind('\\');
	if (lastBackslashPos != string::npos)
		str = str.substr(lastBackslashPos + 1);
}

void PackageLogFile()
{
	time_t rawtime;
	time(&rawtime);

	struct tm timeinfo;
	localtime_s(&timeinfo, &rawtime);

	char buffer[80];
	strftime(buffer, sizeof(buffer), LogFileZip.c_str(), &timeinfo);

	vector<pair<string, string>> files ={
		{ LogFile, "gc.log"},
		{ GameGuiLogFile, "gamegui.log" }
	};

	if(ZipFiles(files, LogPath + "\\" + buffer))
		PrintLog("日志文件打包成功", PL_INFO);
	else
		PrintLog("日志文件打包失败", PL_ERROR);
}

bool CreateDirRecursive(const string& path)
{
	if (_access(path.c_str(), 0) == 0)
		return true;
	size_t pos = path.find_last_of("/\\");
	if (pos != string::npos && pos > 2)
	{
		if (!CreateDirRecursive(path.substr(0, pos)))
			return false;
	}
	return CreateDirectoryA(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

int UnZipFile(const char* zipFilePath, const char* destDir)
{
	unzFile zipfile = unzOpen(zipFilePath);
	if (zipfile == NULL)
	{
		PrintLog("[Zip] Failed to open Zip file", PL_ERROR);
		return -1;
	}

	// 获取 ZIP 文件信息
	unz_global_info global_info;
	if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
	{
		PrintLog("[Zip] Failed to get global info", PL_ERROR);
		unzClose(zipfile);
		return -1;
	}

	// 循环处理 ZIP 文件中的每个条目
	for (uLong i = 0; i < global_info.number_entry; ++i)
	{
		// 获取当前条目的信息
		unz_file_info file_info;
		char filename[256];
		if (unzGetCurrentFileInfo(zipfile, &file_info, filename, sizeof(filename), NULL, 0, NULL, 0) != UNZ_OK)
		{
			PrintLog("[Zip] Failed to get file info", PL_ERROR);
			unzClose(zipfile);
			return -1;
		}

		// 检查是否为目录
		if (file_info.external_fa & 0x10)
		{
			// 创建目录 - 递归创建父目录
			string dir_path = destDir;
			dir_path += "\\";
			dir_path += filename;
			if (!CreateDirRecursive(dir_path))
			{
				PrintLog("[Zip] Failed to create directory", PL_ERROR);
				unzClose(zipfile);
				return -1;
			}
		}
		else
		{
			if (unzOpenCurrentFile(zipfile) != UNZ_OK)
			{
				PrintLog("[Zip] Failed to open current file", PL_ERROR);
				continue; // 跳过当前文件，继续下一个
			}

			// 提取完整的文件路径，并创建所需的目录
			string full_path = destDir;
			full_path += "/";
			full_path += filename;

			// 创建所有必要的目录
			size_t pos = 0;
			do
			{
				pos = full_path.find_first_of("/\\", pos + 1);
				if (pos != string::npos)
				{
					string dir_path = full_path.substr(0, pos);
					if (_access(dir_path.c_str(), 0) != 0 && !CreateDirectory(dir_path.c_str(), NULL))
					{
						PrintLog("[Zip] Failed to create directory", PL_ERROR);
						unzCloseCurrentFile(zipfile);
						// 不要在这里关闭 zipfile，因为循环还没有完成
						continue; // 跳过当前文件，继续下一个
					}
				}
			} while (pos != string::npos);

			FILE* out{};
			fopen_s(&out, full_path.c_str(), "wb");
			if (out == NULL)
			{
				PrintLog("[Zip] Failed to open destination file", PL_ERROR);
				unzCloseCurrentFile(zipfile);
				continue; // 跳过当前文件，继续下一个
			}

			vector<char> buffer(MAX_BUF);
			int error = UNZ_OK;
			do
			{
				error = unzReadCurrentFile(zipfile, buffer.data(), buffer.size());
				if (error < 0)
				{
					PrintLog("[Zip] Failed to read file", PL_ERROR);
					break;
				}
				if (error > 0)
					fwrite(buffer.data(), error, 1, out);
			} while (error > 0);

			fclose(out);
			unzCloseCurrentFile(zipfile);
		}

		// 移动到下一个条目
		if ((i + 1) < global_info.number_entry)
		{
			if (unzGoToNextFile(zipfile) != UNZ_OK)
			{
				PrintLog("[Zip] Failed to go to next file", PL_ERROR);
				unzClose(zipfile);
				return -1;
			}
		}
	}

	// 关闭 ZIP 文件
	unzClose(zipfile);

	return 0; // 解压成功
}

// 获取线程的内核时间和用户时间
static bool GetThreadCpuTimes(HANDLE hThread, double& kernelTime, double& userTime)
{
	FILETIME ftCreation, ftExit, ftKernel, ftUser;
	if (GetThreadTimes(hThread, &ftCreation, &ftExit, &ftKernel, &ftUser))
	{
		kernelTime = FileTimeToDouble(&ftKernel);
		userTime = FileTimeToDouble(&ftUser);
		return true;
	}
	return false;
}

// 计算线程的CPU占用率
double GetThreadCpuUsage(HANDLE hThread, DWORD sleep)
{
	FILETIME ftIdle, ftKernel, ftUser;
	double idleTime1, kernelTime1, userTime1, threadKernelTime1, threadUserTime1;
	double idleTime2, kernelTime2, userTime2, threadKernelTime2, threadUserTime2;

	// 第一次获取系统和线程的CPU时间
	if (!GetSystemTimes(&ftIdle, &ftKernel, &ftUser) ||
		!GetThreadCpuTimes(hThread, threadKernelTime1, threadUserTime1))
		return -1.0; // 表示错误

	idleTime1 = FileTimeToDouble(&ftIdle);
	kernelTime1 = FileTimeToDouble(&ftKernel);
	userTime1 = FileTimeToDouble(&ftUser);

	// 等待指定时间
	Sleep(sleep);

	// 第二次获取系统和线程的CPU时间
	if (!GetSystemTimes(&ftIdle, &ftKernel, &ftUser) ||
		!GetThreadCpuTimes(hThread, threadKernelTime2, threadUserTime2))
		return -1.0; // 表示错误

	idleTime2 = FileTimeToDouble(&ftIdle);
	kernelTime2 = FileTimeToDouble(&ftKernel);
	userTime2 = FileTimeToDouble(&ftUser);

	// 计算系统总CPU时间差
	double totalSystemTime1 = kernelTime1 + userTime1;
	double totalSystemTime2 = kernelTime2 + userTime2;
	double totalSystemTimeDelta = totalSystemTime2 - totalSystemTime1;

	// 计算线程总CPU时间差
	double threadCpuTime1 = threadKernelTime1 + threadUserTime1;
	double threadCpuTime2 = threadKernelTime2 + threadUserTime2;
	double threadCpuTimeDelta = threadCpuTime2 - threadCpuTime1;

	// 计算CPU占用率
	if (totalSystemTimeDelta == 0.0)
		return 0.0; // 避免除以零

	double cpuUsage = (threadCpuTimeDelta / totalSystemTimeDelta) * 100.0;
	return cpuUsage;
}

DWORD GetSystemLanguage()
{
	// 获取系统默认语言信息
	LCID lcid = GetSystemDefaultLCID();
	return PRIMARYLANGID(LANGIDFROMLCID(lcid));
}

/**
 * 判断 hTop 是否直接或间接位于 hBottom 之上
 * 原理：获取 hBottom 紧邻的上层窗口，如果是 hTop，则判定为 Top 在 Bottom 之上
 */
static bool IsZOrderAbove(HWND hTop, HWND hBottom)
{
	if (hTop == nullptr || hBottom == nullptr || hTop == hBottom)
		return false;
	// 获取 hBottom 正下方（Z-order 前面）的那个窗口
	HWND hPrev = GetWindow(hBottom, GW_HWNDPREV);
	// 如果紧邻下方的窗口正是 hTop，说明 hTop 在 hBottom 上面
	return (hPrev == hTop);
}

bool IsCoveredWindow(HWND hwnd, float rate)
{
	static map<HWND, bool> StateList;
	HWND TegetHwnd = GetForegroundWindow();
	if (TegetHwnd == hwnd || !IsWindowVisible(TegetHwnd) || TegetHwnd == hwnd)
		return false;
	if (!IsZOrderAbove(TegetHwnd, hwnd))
		return false;
	RECT SelfRect{};
	RECT TegetRect{};
	GetWindowRect(hwnd, &SelfRect);
	GetWindowRect(TegetHwnd, &TegetRect);
	POINT TegetSize = {
		TegetRect.right - TegetRect.left,
		TegetRect.bottom - TegetRect.top
	};
	TegetSize.x -= max(0, TegetRect.right - SelfRect.right);
	TegetSize.x -= max(0, SelfRect.left - TegetRect.left);
	TegetSize.y -= max(0, TegetRect.bottom - SelfRect.bottom);
	TegetSize.y -= max(0, SelfRect.top - TegetRect.top);

	int CoveredSize = TegetSize.x * TegetSize.y;
	bool IsCovered = (SelfRect.right - SelfRect.left) * (SelfRect.bottom - SelfRect.top) * rate <= CoveredSize;

	if (IsCovered == false)
	{
		StateList.erase(TegetHwnd);
		return false;
	}
	else
		StateList[TegetHwnd] = IsCovered;

	for (const auto& pair : StateList)
	{
		if (pair.second != true)
			return false;
	}
	return true;
}

bool UseRunAdmin()
{
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	BOOL b = AllocateAndInitializeSid(&NtAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &AdministratorsGroup);
	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
			b = FALSE;
		FreeSid(AdministratorsGroup);
	}
	return b == TRUE;
}

void ShaderLog(string str)
{
	static ofstream fLog(ShaderLogFile);
	fLog << str;
}

int GetMemUsageRate(int sleep)
{
	MEMORYSTATUSEX memInfo{};
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
	DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

	Sleep(sleep);

	int nMemoryUseRate = (int)((double)physMemUsed / (double)totalPhysMem * 100.0);
	return nMemoryUseRate;
}

int GetGpuUsageRate(int sleep)
{
	Sleep(sleep);
	return 0;
}

int GetCpuUsageRate(int sleep)
{
	FILETIME ftIdle, ftKernel, ftUser;
	if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser))
	{
		double fOldCPUIdleTime = FileTimeToDouble(&ftIdle);
		double fOldCPUKernelTime = FileTimeToDouble(&ftKernel);
		double fOldCPUUserTime = FileTimeToDouble(&ftUser);
		Sleep(sleep);
		if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser))
		{
			double fCPUIdleTime = FileTimeToDouble(&ftIdle);
			double fCPUKernelTime = FileTimeToDouble(&ftKernel);
			double fCPUUserTime = FileTimeToDouble(&ftUser);
			int nCPUUseRate = (int)(100.0 - (fCPUIdleTime - fOldCPUIdleTime) / (fCPUKernelTime - fOldCPUKernelTime + fCPUUserTime - fOldCPUUserTime) * 100.0);
			return nCPUUseRate;
		}
	}
	return 0;
}

bool IsCursorHidden()
{
    CURSORINFO ci = { sizeof(CURSORINFO) };
    if (!GetCursorInfo(&ci))
        return false;
    static const auto IsDefaultCursor = [&]() -> bool
        {
            static const array<LPCSTR, 13> SysIds = {
               IDC_ARROW, IDC_IBEAM, IDC_WAIT, IDC_CROSS,
               IDC_UPARROW, IDC_SIZEALL, IDC_SIZENWSE,
               IDC_SIZENESW, IDC_SIZENS, IDC_SIZEWE,
               IDC_HAND, IDC_NO, IDC_APPSTARTING
            };
            // static来缓存加载后的位图，不需要重复加载，这里用到了lambda表达式
            static const array<HCURSOR, 13> SysCursorList = {
                []() {
                    array<HCURSOR, 13> cursors{};
                    unsigned int index = 0;
                    for (auto id : SysIds)
                    {
                        cursors[index] = LoadCursor(nullptr, id);
                        index++;
                    }
                    return cursors;
                }()
            };

            for (auto it : SysCursorList)
            {
                if (ci.hCursor == it)
                    return true;
            }
            return false;
        };
    return !IsDefaultCursor();
}

// 判断字符串是否为UTF-8编码
static bool is_utf8(const string& str)
{
	int i = 0;
	int n = str.length();
	while (i < n)
	{
		unsigned char c = str[i];
		int num_bytes = 0;

		if (c >= 0x00 && c <= 0x7F)
		{ // 0xxxxxxx
			i++;
			continue;
		}
		else if ((c & 0xE0) == 0xC0)
		{ // 110xxxxx
			num_bytes = 1;
		}
		else if ((c & 0xF0) == 0xE0)
		{ // 1110xxxx
			num_bytes = 2;
		}
		else if ((c & 0xF8) == 0xF0)
		{ // 11110xxx
			num_bytes = 3;
		}
		else
			return false; // 不是UTF-8编码

		for (int j = 0; j < num_bytes; j++)
		{
			i++;
			if (i >= n || (str[i] & 0xC0) != 0x80)
				return false; // 不是UTF-8编码
		}
		i++;
	}
	return true;
}

string StringToUTF8(const string& str)
{
	if (str.empty())
		return "";

	static map<string, string> cache;
	static mutex cache_mutex;
	if (str.size() > 16)
	{
		lock_guard<mutex> lock(cache_mutex);
		auto it = cache.find(str);
		if (it != cache.end())
			return it->second;
	}

	if (is_utf8(str))
	{
		lock_guard<mutex> lock(cache_mutex);
		cache[str] = str;
		return str;
	}

	try
	{
		// 按照 StringConverter 的风格实现 ACP → UTF-8 转换
		int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
		if (len <= 0) return str;

		wstring wstr(len, L'\0');
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], len);

		// 移除末尾的空字符
		if (!wstr.empty() && wstr[wstr.length() - 1] == L'\0')
			wstr.resize(wstr.length() - 1);

		// 宽字符转 UTF-8
		len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return str;

		string utf8(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);

		// 移除末尾的空字符
		if (!utf8.empty() && utf8[utf8.length() - 1] == '\0')
			utf8.resize(utf8.length() - 1);

		lock_guard<mutex> lock(cache_mutex);
		cache[str] = utf8;
		return utf8;
	}
	catch(...)
	{ }
	return str;
}

string UTF8ToString(const string& utf8_str)
{
	if (utf8_str.empty())
		return "";

	static map<string, string> cache;
	static mutex cache_mutex;
	if (utf8_str.size() > 16)
	{
		lock_guard<mutex> lock(cache_mutex);
		auto it = cache.find(utf8_str);
		if (it != cache.end())
			return it->second;
	}

	try
	{
		// 按照 StringConverter 的风格实现 UTF-8 → ACP 转换
		int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
		if (len <= 0) return utf8_str;

		wstring wstr(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wstr[0], len);

		// 移除末尾的空字符
		if (!wstr.empty() && wstr[wstr.length() - 1] == L'\0')
			wstr.resize(wstr.length() - 1);

		// 宽字符转 ACP
		len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return utf8_str;

		string acp(len, '\0');
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &acp[0], len, nullptr, nullptr);

		// 移除末尾的空字符
		if (!acp.empty() && acp[acp.length() - 1] == '\0')
			acp.resize(acp.length() - 1);

		lock_guard<mutex> lock(cache_mutex);
		cache[utf8_str] = acp;
		return acp;
	}
	catch(...)
	{ }
	return utf8_str;
}

void FindDirectorys(const string& root, vector<string>& dirs)
{
	dirs.clear();

	string pattern = root;
	if (pattern.back() != '\\' && pattern.back() != '/')
		pattern += '\\';
	pattern += "*";

	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFile(pattern.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (strcmp(findData.cFileName, ".") != 0 &&
			strcmp(findData.cFileName, "..") != 0)
		{
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				dirs.push_back(findData.cFileName);
		}
	} while (FindNextFile(hFind, &findData));

	FindClose(hFind);
}

void FindFilesWithExtension(const string& directory, const string& extension, vector<string>& files)
{
	string searchPattern = directory + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	string targetExt = extension;
	transform(targetExt.begin(), targetExt.end(), targetExt.begin(), ::tolower);

	do
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue; // 跳过目录

		string fileName = findData.cFileName;
		size_t dotPos = fileName.find_last_of('.');

		string fileExt;
		if (dotPos != string::npos)
			fileExt = fileName.substr(dotPos + 1);
		else
			fileExt = "";

		transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);

		if (fileExt == targetExt)
		{
			string fullPath = directory;
			if (!fullPath.empty() && fullPath.back() != '\\')
				fullPath += '\\';
			fullPath += fileName;
			files.push_back(fullPath);
		}
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);
}

string GetOpenFileName()
{
	string path;
	OPENFILENAME open{};// 公共对话框结构。
	char file[MAX_PATH]{};// 用来保存获取文件名称的缓冲区。 
	ZeroMemory(&open, sizeof(OPENFILENAME)); // 初始化选择文件对话框
	open.lStructSize = sizeof(OPENFILENAME);//指定这个结构的大小&#xff0c;以字节为单位。
	open.lpstrFile = file;//打开的文件的全路径
	open.lpstrFile[0] = '\0'; //第一个字符串是过滤器描述的显示字符串
	open.nMaxFile = MAX_PATH;  //指定lpstrFile缓冲的大小&#xff0c;以TCHARs为单位
	open.lpstrFilter = "音频文件(*wav)\0*wav\0\0"; //打开文件类型
	open.nFilterIndex = 1;  //指定在文件类型控件中当前选择的过滤器的索引
	open.lpstrFileTitle = NULL; // 指向接收选择的文件的文件名和扩展名的缓冲&#xff08;不带路径信息&#xff09;。这个成员可以是NULL。
	open.nMaxFileTitle = 0;  //指定lpstrFileTitle缓冲的大小&#xff0c;以TCHARs为单位
	open.lpstrInitialDir = NULL;  //指向以空字符结束的字符串&#xff0c;可以在这个字符串中指定初始目录。
	open.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;//位标记的设置&#xff0c;你可以使用来初始化对话框
	if (GetOpenFileName(&open))  // 显示打开选择文件对话框。  
	{
		return open.lpstrFile;
	}
	return "";
}

//timeout单位为秒
void RenderTip(string msg, float timeout, UINT type)
{
	auto pos = find_if(pt_message.begin(), pt_message.end(), [&msg](PopTipMsg a)
		{
			return a.message == msg;
		});
	if (pos == pt_message.end())
	{
		PopTipMsg pt;
		pt.message = msg;
		pt.timeout = timeout;
		pt.type = type;
		pt_message.push_back(pt);
	}
}

int RenderMessageBox(string message, const char* title, UINT type, bool wating = true)
{
	// 利用lock的特性来确保每次只有一个线程在操作弹窗
	// 减少性能开销，因为每次最多是一个弹窗在处理，之前代码会导致所有弹窗都在处理
	// 真是一个奇妙的解决办法    MBControlMutex不是保护变量的线程竞争的！！
	int ret = 0;
	MBControlMutex.lock();
	{
		// 写入锁
		lock_guard<shared_mutex> lock(MBChangeMutex);
		mb_message.open = true;
		mb_message.message = message;
		mb_message.title = title;
		mb_message.type = type;
		mb_message.ret_code = -1;
	}
	MBControlMutex.unlock();

	auto func = [&]() {
		while (mb_message.ret_code == -1 && mb_message.open)
			Sleep(3);
		ret = mb_message.ret_code;
		};
	if (!wating)
	{
		thread th(func);
		th.detach();
	}
	else
		func();
	return ret;
}

int RenderMessageBoxEx(string message, const char* title, map<string, UINT> button, map<string, ImVec4> colors, bool wating = true)
{
	int ret = 0;
	MBControlMutex.lock();
	{
		// 写入锁
		lock_guard<shared_mutex> lock(MBChangeMutex);
		mb_message.open = true;
		mb_message.message = message;
		mb_message.title = title;
		mb_message.type = -1;
		mb_message.ret_code = -1;
		mb_message.button = button;
		mb_message.colors = colors;
	}
	MBControlMutex.unlock();
	auto func = [&]() {
		if (wating)
		{
			while (mb_message.ret_code == -1 && mb_message.open)
				Sleep(3);
		}
		ret = mb_message.ret_code;
		};
	if (!wating)
	{
		thread th(func);
		th.detach();
	}
	else
		func();
	return ret;
}

string GetHwid()
{
	HW_PROFILE_INFO hwProfileInfo;
	if (GetCurrentHwProfile(&hwProfileInfo))
		return hwProfileInfo.szHwProfileGuid;
	return "0";
}

bool IsNumeric(const string& str)
{
	return all_of(str.begin(), str.end(), [](unsigned char c)
		{
			return isdigit(c);
		});
}

string GetQQNumber()
{
	fstream File(QQNumFile, ios::in | ios::out);
	char* buf{};
	bool IsRead = (bool)File.getline(buf, 0);
	string QQNumber = buf;
	if (QQNumber.empty() || !IsRead || !File.is_open() || IsNumeric(QQNumber))
	{
		TCHAR szText[MAX_PATH] = { 0 };
		TCHAR szQQNumber[MAX_PATH] = { 0 };
		HWND hWnd = FindWindow("CTXOPConntion_Class", NULL);
		if (hWnd)
		{
			GetWindowText(hWnd, szText, MAX_PATH);
		}

		int len = wcslen((wchar_t*)szText) * sizeof(TCHAR);
		do
		{
			len--;
		} while (szText[len] != '_');
		lstrcpy(szQQNumber, &szText[len + 1]);
		File.write(buf, 0);
	}
	File.close();
	return QQNumber;
}

bool AddToStartup(const string& file)
{
	HKEY hKey;
	char appPath[MAX_PATH];
	char appName[MAX_PATH]{};

	if (file == "0")
	{
		GetModuleFileName(NULL, appPath, MAX_PATH);

		// 从路径中提取程序名
		_splitpath_s(appPath, NULL, 0, NULL, 0, appName, MAX_PATH, NULL, 0);
	}
	LONG lResult = RegCreateKeyEx(HKEY_CURRENT_USER,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

	if (lResult == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, (file == "0") ? appName : file.c_str(), 0, REG_SZ, (const BYTE*)appPath, strlen(appPath));
		RegCloseKey(hKey);
		PrintLog("添加开机自动启动项成功："s + appName, PL_INFO);
		return true;
	}
	else
	{
		PrintLog("添加开机自动启动项失败："s + file, PL_ERROR);
		return false;
	}
}

bool RemoveStartup(const string& file)
{
	HKEY hKey;
	char appPath[MAX_PATH];
	char appName[MAX_PATH]{};

	if (file == "0")
	{
		GetModuleFileName(NULL, appPath, MAX_PATH);

		// 从路径中提取程序名
		_splitpath_s(appPath, NULL, 0, NULL, 0, appName, MAX_PATH, NULL, 0);
	}
	LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0, KEY_WRITE, &hKey);

	if (lResult == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, (file == "0") ? appName : file.c_str());
		RegCloseKey(hKey);
		PrintLog("删除开机自动启动项成功："s + appName, PL_INFO);
		return true;
	}
	else
	{
		PrintLog("删除开机自动启动项失败："s + file, PL_ERROR);
		return false;
	}
}

/*
@echo off
(
    echo Set WshShell = CreateObject("WScript.Shell")
    echo Wscript.Sleep 100
    echo WshShell.Run "cmd /c %~f0", 0
    echo Set WshShell = Nothing
) > "%temp%\hide.vbs"
wscript "%temp%\hide.vbs"
exit /b

set target_process=chrome
set command_to_run="C:\Tools\MyApp.exe"

:check_process
tasklist /FI "IMAGENAME eq %target_process%.exe" 2>NUL | find /I "%target_process%.exe" >NUL
if %ERRORLEVEL% equ 0 (
    echo 目标进程已启动，执行命令...
    start "" %command_to_run%
    goto :eof
)

timeout /t 2 /nobreak >nul
goto :check_process
*/
bool AddFollowStartup(const string& file)
{
	char appPath[MAX_PATH];
	char appName[MAX_PATH]{};
	GetModuleFileName(NULL, appPath, MAX_PATH);
	// 从路径中提取程序名
	_splitpath_s(appPath, NULL, 0, NULL, 0, appName, MAX_PATH, NULL, 0);

	string bat;
	bat += \
"\
:: GC连点器“跟随游戏启动”功能，请勿乱动该文件！！！\n\
@echo off\n\
(\n\
	echo Set WshShell = CreateObject(\"WScript.Shell\")\n\
	echo Wscript.Sleep 100\n\
	echo WshShell.Run \"cmd /c %~f0\", 0\n\
	echo Set WshShell = Nothing\n\
	) > \"%temp%\hideGCFollowBat.vbs\"\n\
		wscript \"%temp%\hideGCFollowBat.vbs\"\n\
		exit / b\n\
		set target_process = " + file + "\n\
		set command_to_run = " + appName + "\n\
:check_process\n\
tasklist / FI \"IMAGENAME eq %target_process%.exe\" 2 > NUL | find / I \"%target_process%.exe\" > NUL\n\
if% ERRORLEVEL% equ 0 (\n\
	start "" % command_to_run %\n\
	goto :eof\n\
	)\n\
	timeout / t 2 / nobreak > nul\n\
	goto :check_process\n\
";

	ofstream ofs(GCFollowBatFile, ios::out);
	ofs << bat;
	ofs.close();
	PrintLog("[File]已创建" + GCFollowBatFile + "文件", PL_INFO);

	return AddToStartup(GCFollowBatFile);
}

bool RemoveFollowStartup()
{
	DeleteFile(GCFollowBatFile.c_str());
	return RemoveStartup(GCFollowBatFile);
}

string GetProcessPathFromPID(DWORD processId)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (hProcess == NULL)
	{
		PrintLog("[System]无法打开进程", PL_ERROR);
		return "0";
	}

	char path[MAX_PATH];
	DWORD size = GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH);
	if (size == 0)
	{
		PrintLog("[System]无法获取进程路径", PL_ERROR);
		CloseHandle(hProcess);
		return "0";
	}

	CloseHandle(hProcess);
	return path;
}