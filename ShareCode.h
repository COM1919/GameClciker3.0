#pragma once
#include "maths.h"
#include "zlib.h"
#include "zip.h"
#include "CodeModules.h"
#include "Log.h"
#include <zlib.h>
#include <unzip.h>
#include <iomanip>
#include <sstream>
#include <psapi.h>
#include <pdh.h>
// PDH 错误码（如 PDH_MORE_DATA）定义在 pdhmsg.h，pdh.h 不包含它；__has_include 防御缺失该头的工具链
#if defined(__has_include)
#if __has_include(<pdhmsg.h>)
#include <pdhmsg.h>
#endif
#endif
#include <algorithm>
#include <TlHelp32.h>
#include <malloc.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "pdh.lib")

#define PT_INFO 1
#define PT_WARNING 2
#define PT_ERROR 3

int RenderMessageBoxEx(string message, const char* title, map<string, UINT> button, map<string, ImVec4> colors, bool wating);
int RenderMessageBox(string message, const char* title, UINT type, bool wating);
void RenderTip(string msg, float timeout, UINT type);
void FindFilesWithExtension(const string& directory, const string& extension, vector<string>& files);
string GetOpenFileName();
string StringToUTF8(const string& str, bool NoCach = false);
string UTF8ToString(const string& str, bool NoCach = false);
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
int ReadURLDatas(const string& url, vector<char>& datas);
int ReadURLDatas(const string& url, string& datas);
void GetStringFileName(string& str, bool KeepSuffix = false);
void PackageLogFile();
bool IsCoveredWindow(HWND hwnd, float rate = 0.8f);
void SetClassStyle(bool IsEnable);
bool ZipFiles(const vector<pair<string, string>>& files, const string& zipFilename);
int UnZipFile(const char* zipFilePath, const char* destDir);
bool CopyTextToClipboard(const string& text);
void ClearProcessMemory();
void UpdateScriptFromShop();
POINT GetGameWindowCenter(HWND hwnd);
void CheckUpdateScriptFromShop(ScriptInfo& script);
bool UpdateScriptFromShop(const ScriptInfo& script);
bool AddToStartup(const string& file = "0");
bool RemoveStartup(const string& file = "0");

POINT GetGameWindowCenter(HWND hwnd)
{
	RECT rc{};
	GetClientRect(hwnd, &rc);
	POINT pt = {
		static_cast<int>((rc.right - rc.left) / 2),
		static_cast<int>((rc.bottom - rc.top) / 2)
	};
	ClientToScreen(hwnd, &pt);
	return pt;
}

// 脚本商店专用：带 3 次重试的 URL 读取 / 下载（失败后递增退避再试）
static bool ReadURLDatasRetry(const string& url, string& content, int retry = 3)
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

static bool DownloadURLFilesRetry(const string& url, const string& filename, int retry = 3)
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
int RefushScriptShopList(vector<ScriptInfo>& scripts, int paper, bool clear, bool fromGitee = false);
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
	// 修复：此函数在 detach 线程中执行，与脚本商店刷新线程（GC.cpp 中 clear/重填 ScriptData::ShopList）
	// 及主线程遍历并发。原代码无锁，vector 扩容导致迭代器失效 -> 0xC0000005。加读锁。
	shared_lock<shared_mutex> lock(ScriptData::ShopListMtx);
	for (const ScriptInfo& NewScript : ScriptData::ShopList)
	{
		if (NewScript.name == script.name)
		{
			if (NewScript.ver > script.ver)
				script.IsHasUpdate = true;
			break;
		}
	}
}

bool UpdateScriptFromShop(const ScriptInfo& script)
{
	// 依据脚本商店列表判断更新来源：市场源（URL）重新下载，Gitee 源复制缓存
	shared_lock<shared_mutex> lock(ScriptData::ShopListMtx);
	for (const ScriptInfo& NewScript : ScriptData::ShopList)
	{
		if (NewScript.name == script.name)
		{
			if (NewScript.file.compare(0, 7, "http://") == 0 || NewScript.file.compare(0, 8, "https://") == 0)
			{
				// 市场源：重新下载最新版本
				string id = NewScript.file.substr(NewScript.file.find_last_of('/') + 1);
				for (char& c : id)
					if (!isalnum((unsigned char)c)) c = '_';
				const string target = ScriptPath + "\\download_" + id + ".zip";
				return DownloadURLFilesRetry(NewScript.file, target);
			}
			// Gitee 源：复制缓存
			const string filename = ScriptShopDir + "\\" + NewScript.name + ".gcdat";
			const string target = ScriptPath + "\\" + NewScript.name + ".zip";
			return CopyFile(filename.c_str(), target.c_str(), false) != 0;
		}
	}
	return false;
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

// 辅助结构：存储市场脚本元数据
struct MarketScriptMeta {
	int scriptid;
	string title;
	string author;
	string motd;
	int ver;
	string iconurl;
	string bbsurl;
};
int RefushScriptShopList(vector<ScriptInfo>& scripts, int paper, bool clear, bool fromGitee)
{
	// 修复：static 缓存向量（GiteeScriptList/MarketScriptList）可能被多个 detach 线程
	//（快速切换下载源/翻页）并发访问 → 数据竞争（UB）。整个函数串行化；
	// 网络请求期间持锁会阻塞其他调用，但商场刷新是用户显式操作，可接受。
	static mutex ShopListMtx;
	lock_guard<mutex> lock(ShopListMtx);

	const int PaperSize = 10;
	const string baseUrl = "https://scripts.mcgc.top";

	// ---------- 静态缓存（分开存储两种源） ----------
	static vector<string> GiteeScriptList;            // Gitee 文本列表
	static vector<MarketScriptMeta> MarketScriptList; // 市场元数据列表

	// ======================== 市场源 ========================
	if (!fromGitee)
	{
		// 1拉取列表（若缓存为空或强制刷新）
		if (MarketScriptList.empty() || clear)
		{
			MarketScriptList.clear();
			const int FetchSize = 20;   // 每次请求条数
			int begin = 1;
			bool hasMore = true;

			while (hasMore)
			{
				int end = begin + FetchSize - 1;
				string url = baseUrl + "/api/app/list?begin=" + to_string(begin) + "&end=" + to_string(end);
				string content;

				if (!ReadURLDatasRetry(url, content))
				{
					PrintLog("[ScriptMarket] 获取列表失败", PL_ERROR);
					return 0;
				}

				// 解析 JSON
				picojson::value v;
				string err = picojson::parse(v, content);
				if (!err.empty())
				{
					PrintLog("[ScriptMarket] JSON 解析错误: " + err, PL_ERROR);
					return 0;
				}
				if (!v.is<picojson::object>())
				{
					PrintLog("[ScriptMarket] 响应不是对象", PL_ERROR);
					return 0;
				}
				auto& obj = v.get<picojson::object>();

				// 检查 success 和 code
				if (!obj.count("success") || !obj["success"].is<bool>() || !obj["success"].get<bool>())
				{
					PrintLog("[ScriptMarket] success为false", PL_ERROR);
					return 0;
				}
				if (!obj.count("code") || !obj["code"].is<double>() || (int)obj["code"].get<double>() != 0)
				{
					PrintLog("[ScriptMarket] code 非 0", PL_ERROR);
					return 0;
				}
				if (!obj.count("data") || !obj["data"].is<picojson::array>())
				{
					PrintLog("[ScriptMarket] data 缺失或非数组", PL_ERROR);
					return 0;
				}
				auto& data = obj["data"].get<picojson::array>();
				if (data.empty())
					break;  // 无更多数据

				// 提取元数据
				for (auto& item : data)
				{
					if (!item.is<picojson::object>()) continue;
					auto& it = item.get<picojson::object>();
					MarketScriptMeta meta;
					meta.scriptid = (int)it["scriptid"].get<double>();
					meta.title = it["title"].get<string>();
					meta.author = it["author"].get<string>();
					meta.motd = it["motd"].get<string>();
					string verstr = it["ver"].get<string>();
					verstr.erase(verstr.begin());
					try
					{
						meta.ver = stoi(verstr);
					}
					catch (...)
					{
						meta.ver = 0;
					}
					meta.iconurl = it["iconurl"].get<string>();
					meta.bbsurl = it["bbsurl"].get<string>();
					MarketScriptList.push_back(meta);
				}

				// 判断是否已到末尾
				if (data.size() < FetchSize)
					hasMore = false;
				else
					begin = end + 1;
			}
		}

		// 2. 计算总页数
		int totalCount = (int)MarketScriptList.size();
		int totalPages = (totalCount + PaperSize - 1) / PaperSize;
		if (totalCount == 0)
			return 0;

		// 3. 截取当前页
		int beginIdx = (paper - 1) * PaperSize;
		int endIdx = min(beginIdx + PaperSize, totalCount);
		scripts.clear();

		// 图标缓存目录
		if (_access(ScriptIconTemp.c_str(), 0) != 0)
			CreateDirectory(ScriptIconTemp.c_str(), NULL);

		// 图标下载任务（先收集，循环结束后并行下载，提高整体速度）
		struct IconDownloadTask { string url; string tempFile; string localPath; };
		vector<IconDownloadTask> iconTasks;
		for (int i = beginIdx; i < endIdx; ++i)
		{
			const auto& meta = MarketScriptList[i];

			ScriptInfo info;
			info.name = "download_" + to_string(meta.scriptid);
			info.title = meta.title;
			info.ver = meta.ver;
			info.link = meta.bbsurl;

			const string title = meta.title;
			const string author = meta.author;
			const string motd = meta.motd;
			const string bbslink = meta.bbsurl;

			// 构建描述信息（类似原 GetScriptsInfo 的拼接）
			info.info = StringToUTF8(meta.motd + (config.UseChinese ? u8"\n&7脚本作者：" : "\nAuthor: ") + meta.author);

			// 下载图标
			string localIconPath;
			if (!meta.iconurl.empty())
			{
				// 从 URL 中提取文件名（用于确定扩展名）
				string filename = meta.iconurl.substr(meta.iconurl.find_last_of('/') + 1);
				size_t dotPos = filename.find_last_of('.');
				string ext = (dotPos != string::npos) ? filename.substr(dotPos) : ".png"; // 默认 .png
				// 防止路径注入，只取扩展名，文件名用 scriptid
				string safeExt = ext;
				for (char& c : safeExt)
					if (!isalnum(c) && c != '.') c = '_';
				if (safeExt.empty() || safeExt[0] != '.') safeExt = ".png";

				localIconPath = ScriptIconTemp + "\\" + to_string(meta.scriptid) + safeExt;

				// 若图标文件不存在，则下载
				if (_access(localIconPath.c_str(), 0) != 0)
				{
					// 收集图标下载任务，稍后并行下载
					string tempFile = ScriptIconTemp + "\\" + to_string(meta.scriptid) + ".tmp";
					iconTasks.push_back({ meta.iconurl, tempFile, localIconPath });
				}
			}
			info.texture = localIconPath; // 可能为空
			// 注意：此时未下载脚本文件，info.file 就填为脚本下载链接
			info.file = "https://scripts.mcgc.top/download/" + to_string(meta.scriptid);
			scripts.push_back(info);
		}
		// 市场模式下 不调用 GetScriptsInfo，也不重命名
		// 并行下载图标（带重试），提高整体下载速度
		{
			vector<thread> threads;
			for (const auto& task : iconTasks)
			{
				threads.emplace_back([task]()
				{
					if (DownloadURLFilesRetry(task.url, task.tempFile))
					{
						if (rename(task.tempFile.c_str(), task.localPath.c_str()) != 0)
							_unlink(task.tempFile.c_str());
					}
				});
			}
			for (auto& t : threads)
				t.join();
		}

		return totalPages;
	}

	// ======================== Gitee 源（保留原逻辑） ========================
	if (GiteeScriptList.empty() || clear)
	{
		string content;
		if (!ReadURLDatasRetry(URL_GITEE_SCRIPTS, content))
		{
			PrintLog("[Init] Gitee: 获取脚本列表失败", PL_ERROR);
			return 0;
		}
		SplitString(content, "\r\n", GiteeScriptList);
	}

	const int BeginCount = (paper - 1) * PaperSize;
	const int EndCount = BeginCount + PaperSize;
	scripts.clear();
	int count = 0;
	for (const string& script : GiteeScriptList)
	{
		count++;
		if (BeginCount >= count || EndCount < count)
			continue;
		const string filename = ScriptShopDir + "\\" + script + ".gcdat";
		if (_access(filename.c_str(), 0) != 0) {
			const string url = URL_GITEE_SC_ROOT + script + ".zip";
			if (!DownloadURLFilesRetry(url, filename))
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
		rename(info.file.c_str(), (ScriptShopDir + "\\" + info.name).c_str());

	return (GiteeScriptList.size() + PaperSize - 1) / PaperSize;
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
		// 修复：解压前先清理旧目录，避免上次运行残留的文件（多个 lua / 旧图标）导致
		// "发现多个lua文件"误报或图标互相覆盖。
		DeleteDirectoryRecursively(dir.c_str());
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
				if (script.link.empty() || script.link.find("https://bbs.mcgc.top") == string::npos || script.link.find("https://scripts.mcgc.top") == string::npos)
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

		{
			const string DocsPath = dir + "\\readme.txt";
			if (_access(DocsPath.c_str(), 0) == 0)
				script.docs = DocsPath;
			else
				script.docs = "";
		}

		PrintLog("[Script] 找到脚本文件: " + script.file, PL_INFO);
		TempList.push_back(script);

		CheckScriptPermission(script.file, script.RestrictedFuncs);
		const int count = script.RestrictedFuncs.size();
		for (int i = 0; i < count; i++) // 不能是<=，因为size比实际大1
		{
			// 修复：原代码 i!=0 时用 "=" 覆盖整个字符串（把之前拼接的权限全部丢掉，只剩最后一个），
			// 应为追加分隔符
			if (i != 0)
				script.RestrictedFuncsStr += ", ";
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
		lock_guard<shared_mutex> lock(ScriptData::AvaiableMtx);
		ScriptData::Avaiable = scripts;
	}
	PrintLog("[Script] 刷新脚本列表成功，共 " + to_string(ScriptData::Avaiable.size()) + " 个脚本文件", PL_INFO);
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
		// 修复：原代码 str.find('.') 取的是【第一个】点号，若路径中目录名含点
		// （如 D:\...\GameClciker3.0\...\AuxScaffold.gcdat），会在 GameClciker3.0 处错误截断，
		// 导致所有脚本名都变成 "GameClciker3"、全部解压到同一目录（互相覆盖 → 图标错乱、
		// 多个 lua 误报、脚本无法识别）。改用 rfind('.') 取最后一个点（真正的扩展名前）。
		size_t dotPosition = str.rfind('.');
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
		{ LogFile, "gc.log"}
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
 * 判断 hTop 是否直接或间接位于 hBottom 之上（Z-order 中在 hBottom 前面）
 * 修复：循环向上查找，而非仅看紧邻
 */
static bool IsZOrderAbove(HWND hTop, HWND hBottom)
{
	if (hTop == nullptr || hBottom == nullptr || hTop == hBottom)
		return false;
	HWND hPrev = GetWindow(hBottom, GW_HWNDPREV);
	while (hPrev != nullptr)
	{
		if (hPrev == hTop)
			return true;
		hPrev = GetWindow(hPrev, GW_HWNDPREV);
	}
	return false;
}

bool IsCoveredWindow(HWND hwndTarget, float rate)
{
	// 1. 目标窗口必须可见
	if (!IsWindowVisible(hwndTarget))
		return false;

	// 2. 获取当前前台窗口
	HWND hwndForeground = GetForegroundWindow();
	if (hwndForeground == nullptr)
		return false;

	// 3. 如果前台窗口就是目标自身，或不可见，则不视为覆盖
	if (hwndForeground == hwndTarget || !IsWindowVisible(hwndForeground))
		return false;

	// 4. 检查前台窗口是否在Z序上高于目标窗口
	if (!IsZOrderAbove(hwndForeground, hwndTarget))
		return false;

	// 5. 获取两个窗口的矩形
	RECT rectTarget = { 0 };
	RECT rectForeground = { 0 };
	if (!GetWindowRect(hwndTarget, &rectTarget) || !GetWindowRect(hwndForeground, &rectForeground))
		return false;

	// 6. 计算交集矩形（标准矩形交集）
	RECT rectIntersect = { 0 };
	if (!IntersectRect(&rectIntersect, &rectTarget, &rectForeground))
	{
		// 无交集，覆盖面积为0
		return false;
	}

	// 7. 计算覆盖面积与目标窗口总面积
	long long areaIntersect = static_cast<long long>(rectIntersect.right - rectIntersect.left) *
		(rectIntersect.bottom - rectIntersect.top);
	long long areaTarget = static_cast<long long>(rectTarget.right - rectTarget.left) *
		(rectTarget.bottom - rectTarget.top);

	// 8. 判断覆盖比例是否达到阈值
	return static_cast<float>(areaIntersect) / areaTarget >= rate;
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

#include <Windows.h>
#include <pdh.h>
#include <vector>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "pdh.lib")

#include <Windows.h>
#include <pdh.h>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>   // 用于调试输出，正式使用可删除

#pragma comment(lib, "pdh.lib")

/**
 * 获取 GPU 3D 引擎占用率（%），与任务管理器的“3D”图表对齐。
 * 采用双采样确保准确性。
 * @param sleep 两次采样之间的间隔（毫秒），建议 200~500ms。
 * @return 0~100 的占用百分比，失败返回 0。
 */
int GetGpuUsageRate(int sleep)
{
	static bool gpuInitialized = false;
	static PDH_HQUERY gpuQuery = nullptr;
	static PDH_HCOUNTER gpuCounter = nullptr;

	if (!gpuInitialized)
	{
		if (PdhOpenQuery(NULL, 0, &gpuQuery) != ERROR_SUCCESS)
		{
			gpuInitialized = true;
			return 0;
		}

		// 监控所有引擎，后续筛选 3D
		if (PdhAddEnglishCounter(gpuQuery, "\\GPU Engine(*)\\Utilization Percentage", 0, &gpuCounter) != ERROR_SUCCESS)
		{
			PdhCloseQuery(gpuQuery);
			gpuQuery = nullptr;
			gpuCounter = nullptr;
			gpuInitialized = true;
			return 0;
		}

		gpuInitialized = true;
		return 0;
	}

	// 常规调用
	if (!gpuQuery || !gpuCounter)
		return 0;

	// 双采样：采集 -> 等待 -> 再采集（计算差值）
	if (PdhCollectQueryData(gpuQuery) != ERROR_SUCCESS)
		return 0;

	Sleep(sleep);

	if (PdhCollectQueryData(gpuQuery) != ERROR_SUCCESS)
		return 0;

	// 获取所有引擎的格式化数据
	DWORD dwBufferSize = 0;
	DWORD dwItemCount = 0;
	PDH_STATUS status = PdhGetFormattedCounterArray(gpuCounter, PDH_FMT_DOUBLE, &dwBufferSize, &dwItemCount, nullptr);

	if (status != ERROR_SUCCESS && status != PDH_MORE_DATA)
		return 0;
	if (dwBufferSize == 0 || dwItemCount == 0)
		return 0;

	vector<BYTE> buffer(dwBufferSize);
	PDH_FMT_COUNTERVALUE_ITEM* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buffer.data());

	status = PdhGetFormattedCounterArray(gpuCounter, PDH_FMT_DOUBLE, &dwBufferSize, &dwItemCount, items);
	if (status != ERROR_SUCCESS)
		return 0;

	// 筛选出 3D 引擎，取最大值（通常只有一个）
	double max3D = 0.0;
	bool found3D = false;

	for (DWORD i = 0; i < dwItemCount; ++i)
	{
		// 检查数据有效性
		if (items[i].FmtValue.CStatus != PDH_CSTATUS_VALID_DATA &&
			items[i].FmtValue.CStatus != PDH_CSTATUS_NEW_DATA)
			continue;

		string name(items[i].szName);
		// 只关心包含 "engtype_3D" 的引擎（不区分大小写，实际全小写）
		if (name.find("engtype_3D") != string::npos)
		{
			double value = items[i].FmtValue.doubleValue;
			if (value > max3D) max3D = value;
			found3D = true;
		}
	}

	// 如果没有找到任何 3D 引擎，返回 0
	if (!found3D)
		return 0;

	int result = static_cast<int>(round(max3D));
	result = (result < 0) ? 0 : (result > 100 ? 100 : result);
	return result;
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
bool is_utf8(const string& str)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
	size_t len = str.size();
	size_t i = 0;

	while (i < len) {
		unsigned char c = bytes[i];
		int num_bytes = 0;

		// 单字节 ASCII (0x00 ~ 0x7F)
		if (c <= 0x7F)
		{
			++i;
			continue;
		}
		// 双字节: 110xxxxx, 最小码点 U+0080, 最大 U+07FF
		else if ((c & 0xE0) == 0xC0)
		{
			num_bytes = 2;
			// 检查过短的编码（C0, C1）
			if (c <= 0xC1) return false;
		}
		// 三字节: 1110xxxx, 最小码点 U+0800, 最大 U+FFFF（排除代理区）
		else if ((c & 0xF0) == 0xE0)
		{
			num_bytes = 3;
			// 如果首字节是 0xE0，第二个字节必须 >= 0xA0（防止过度编码）
			// 但更精确的是检查整个序列
		}
		// 四字节: 11110xxx, 最小 U+10000, 最大 U+10FFFF
		else if ((c & 0xF8) == 0xF0)
		{
			num_bytes = 4;
			// 首字节 0xF4 时，后续字节必须 ≤ 0x8F 以保证码点 ≤ 0x10FFFF
		}
		else
			return false; // 非 UTF-8 首字节

		// 检查后续字节数量是否超出范围
		if (i + num_bytes > len)
			return false;

		// 检查后续字节必须为 10xxxxxx
		for (int j = 1; j < num_bytes; ++j)
		{
			unsigned char b = bytes[i + j];
			if ((b & 0xC0) != 0x80)
				return false;
		}

		// 根据首字节和后续字节检查码点范围
		if (num_bytes == 2)
		{
			// 双字节: 首字节 0xC2~0xDF，后续字节 0x80~0xBF
			unsigned char b1 = bytes[i + 1];
			if (c < 0xC2 || c > 0xDF)
				return false;
			// 最小码点 U+0080: (c & 0x1F)<<6 | (b1 & 0x3F) >= 0x80，但 C2 80 正好是 0x80，所以 C2 以上都合法
		}
		else if (num_bytes == 3)
		{
			unsigned char b1 = bytes[i + 1];
			unsigned char b2 = bytes[i + 2];
			// 首字节 0xE0 时，b1 必须 >= 0xA0 (防止 U+0800 以下过度编码)
			if (c == 0xE0 && b1 < 0xA0)
				return false;
			// 首字节 0xED 时，b1 必须 <= 0x9F (防止代理区 U+D800~U+DFFF)
			if (c == 0xED && b1 > 0x9F)
				return false;
			// 一般范围 E1~EC, EE~EF 都可以，但无需额外检查
		}
		else if (num_bytes == 4)
		{
			unsigned char b1 = bytes[i + 1];
			unsigned char b2 = bytes[i + 2];
			unsigned char b3 = bytes[i + 3];
			// 首字节 0xF0 时，b1 >= 0x90 (防止 U+10000 以下过度编码)
			if (c == 0xF0 && b1 < 0x90)
				return false;
			// 首字节 0xF4 时，b1 <= 0x8F (确保码点 ≤ U+10FFFF)
			if (c == 0xF4 && b1 > 0x8F)
				return false;
			// F1~F3 都合法
			if (c > 0xF4)
				return false; // F5~F7 无效
		}
		i += num_bytes;
	}
	return true;
}

string StringToUTF8(const string& str, bool NoCach)
{
	if (str.empty())
		return "";

	static map<string, string> cache;
	static mutex cache_mutex;
	if (!NoCach)
	{
		if (str.size() > 16)
		{
			lock_guard<mutex> lock(cache_mutex);
			auto it = cache.find(str);
			if (it != cache.end())
				return it->second;
		}
	}

	if (is_utf8(str))
	{
		if (!NoCach)
		{
			lock_guard<mutex> lock(cache_mutex);
			cache[str] = str;
		}
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

		if (!NoCach)
		{
			lock_guard<mutex> lock(cache_mutex);
			cache[str] = utf8;
		}
		return utf8;
	}
	catch(...)
	{ }
	return str;
}

string UTF8ToString(const string& utf8_str, bool NoCach)
{
	if (utf8_str.empty())
		return "";
	static map<string, string> cache;
	static mutex cache_mutex;
	if (!NoCach)
	{
		if (utf8_str.size() > 16)
		{
			lock_guard<mutex> lock(cache_mutex);
			auto it = cache.find(utf8_str);
			if (it != cache.end())
				return it->second;
		}
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

		if (!NoCach)
		{
			lock_guard<mutex> lock(cache_mutex);
			cache[utf8_str] = acp;
		}
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
	// 修复：原代码无锁 find_if/push_back。RenderTip 被 GetInfo/Network/ClickSound 等
	// 多个后台线程以及主线程调用，与渲染线程遍历 pt_message 并发时 vector 扩容，
	// 导致迭代器失效与访问已释放内存 → 0xC0000005。加写锁保护。
	lock_guard<shared_mutex> lock(PopTipMutex);
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
	// 真是一个奇妙的解决办法    ControlMtx不是保护变量的线程竞争的！！
	int ret = 0;
	{
		// 写入锁
		lock_guard<shared_mutex> lock(MessageBoxMsg::Mtx);
		MessageBoxMsg::IsOpen = true;
		MessageBoxMsg::Message = message;
		MessageBoxMsg::TitleName = title;
		MessageBoxMsg::ButtonType = type;
		MessageBoxMsg::ReturnCode = -1;
	}

	if (!wating)
		return 0;
	else
	{
		while (MessageBoxMsg::ReturnCode == -1 && MessageBoxMsg::IsOpen)
			Sleep(5);
		ret = MessageBoxMsg::ReturnCode;
	}
	return ret;
}

int RenderMessageBoxEx(string message, const char* title, map<string, UINT> button, map<string, ImVec4> colors, bool wating = true)
{
	int ret = 0;
	{
		// 写入锁
		lock_guard<shared_mutex> lock(MessageBoxMsg::Mtx);
		MessageBoxMsg::IsOpen = true;
		MessageBoxMsg::Message = message;
		MessageBoxMsg::TitleName = title;
		MessageBoxMsg::ButtonType = -1;
		MessageBoxMsg::ReturnCode = -1;
		MessageBoxMsg::BtnDefines = button;
		MessageBoxMsg::BtnColors = colors;
	}

	if (!wating)
		return 0;
	else
	{
		if (wating)
		{
			while (MessageBoxMsg::ReturnCode == -1 && MessageBoxMsg::IsOpen)
				Sleep(3);
		}
		ret = MessageBoxMsg::ReturnCode;
	}
	return ret;
}

string GetHwid()
{
	HW_PROFILE_INFO hwProfileInfo;
	if (GetCurrentHwProfile(&hwProfileInfo))
		return hwProfileInfo.szHwProfileGuid;
	return "0";
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
