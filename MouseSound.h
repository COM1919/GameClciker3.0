#pragma once
#include "maths.h"
#include "MainClick.h"

#include <OpenAL/ALFamework/Famewrok.h>
#include <Dbt.h>

// 部分 Windows SDK 的 mmdeviceapi.h 未导出 GUID_DEVINTERFACE_AUDIO 符号，这里显式提供其标准值
// （{2FDEFC66-FA64-11D0-A5D6-28DB04C10000}），用于 RegisterDeviceNotification 监听声卡插拔；
// GUID_DEVINTERFACE_VOLUME（耳机/扬声器端点）已由 <Dbt.h> 提供。
static const GUID AudioDevInterfaceGuid = { 0x2fdafc66, 0xfa64, 0x11d0, { 0xa5, 0xd6, 0x28, 0xdb, 0x04, 0xc1, 0x00, 0x00 } };

static inline unordered_map<int, int> g_KeyScanToVk = {
	// 主键盘区 (1-13, 15-41, 43-53, 57-68, 87-88)
	{1, VK_ESCAPE},
	{59, VK_F1}, {60, VK_F2}, {61, VK_F3}, {62, VK_F4}, {63, VK_F5}, {64, VK_F6},
	{65, VK_F7}, {66, VK_F8}, {67, VK_F9}, {68, VK_F10}, {87, VK_F11}, {88, VK_F12}, {41, VK_OEM_3},
	{2, '1'}, {3, '2'}, {4, '3'}, {5, '4'}, {6, '5'}, {7, '6'}, {8, '7'}, {9, '8'}, {10, '9'}, {11, '0'},
	{12, VK_OEM_MINUS}, {13, VK_OEM_PLUS}, {14, VK_BACK}, {15, VK_TAB}, {58, VK_CAPITAL},
	{30, 'A'}, {48, 'B'}, {46, 'C'}, {32, 'D'}, {18, 'E'}, {33, 'F'}, {34, 'G'}, {35, 'H'},
	{23, 'I'}, {36, 'J'}, {37, 'K'}, {38, 'L'}, {50, 'M'}, {49, 'N'}, {24, 'O'}, {25, 'P'},
	{16, 'Q'}, {19, 'R'}, {31, 'S'}, {20, 'T'}, {22, 'U'}, {47, 'V'}, {17, 'W'}, {45, 'X'}, {21, 'Y'}, {44, 'Z'},
	{26, VK_OEM_4}, {27, VK_OEM_6}, {43, VK_OEM_5},
	{39, VK_OEM_1}, {40, VK_OEM_7}, {28, VK_RETURN},
	{51, VK_OEM_COMMA}, {52, VK_OEM_PERIOD}, {53, VK_OEM_2}, {57, VK_SPACE},
	// 功能键区（扩展扫描码统一为标准 16 位形式 0xE0xx；兼容旧声音包中的 0x0Exx 写法见 NormalizeScanCode）
	{57399, VK_SNAPSHOT}, {70, VK_SCROLL}, {3653, VK_PAUSE},  // 0xE037=PrintScreen（旧值 3639）；Pause 无标准 0xE0 前缀，保留原值
	{57426, VK_INSERT}, {57427, VK_DELETE}, {57415, VK_HOME}, {57423, VK_END}, {57417, VK_PRIOR}, {57425, VK_NEXT},
	{57416, VK_UP}, {57419, VK_LEFT}, {57421, VK_RIGHT}, {57424, VK_DOWN},
	// Shift/Ctrl/Alt（扩展键标准值：右Ctrl 0xE01D=57373、右Alt 0xE038=57400，旧包中曾写为 3613/3640）
	{42, VK_SHIFT}, {54, VK_SHIFT}, {29, VK_CONTROL}, {57373, VK_CONTROL}, {56, VK_MENU}, {57400, VK_MENU},
	{57435, VK_LWIN}, {57436, VK_RWIN}, {57437, VK_APPS},
	// 数字小键盘（扩展键标准值：除号 0xE035=57397、小键盘回车 0xE01C=57372，旧包中曾写为 3637/3612）
	{69, VK_NUMLOCK}, {57397, VK_DIVIDE}, {55, VK_MULTIPLY}, {74, VK_SUBTRACT},
	{78, VK_ADD}, {57372, VK_RETURN}, {83, VK_DECIMAL},
	{79, VK_NUMPAD1}, {80, VK_NUMPAD2}, {81, VK_NUMPAD3}, {75, VK_NUMPAD4},
	{76, VK_NUMPAD5}, {77, VK_NUMPAD6}, {71, VK_NUMPAD7}, {72, VK_NUMPAD8}, {73, VK_NUMPAD9}, {82, VK_NUMPAD0},
	// Windows 特殊键 (61000+)
	{61010, VK_INSERT}, {61011, VK_DELETE}, {60999, VK_HOME}, {61007, VK_END},
	{61001, VK_PRIOR}, {61009, 0x22}, {61000, VK_UP}, {61003, VK_LEFT}, {61005, VK_RIGHT}, {61008, VK_DOWN}
};

// 兼容旧声音包格式：扩展扫描码标准 16 位表示为 0xE0xx，但旧包中曾被写成 0x0Exx（如右 Ctrl 57373 写成 3613）
static inline int NormalizeScanCode(int scan)
{
	if (((scan & 0xFF00) == 0x0E00) && (scan & 0x00FF) != 0)
		return 0xE000 | (scan & 0x00FF);
	return scan;
}

static struct SoundRange
{
	int begin;
	int offeset;
};

unordered_map<string, array<ALuint, 256>> g_KeybdSoundSourceList;
vector<ALuint> g_KeybdSoundBufferList; // 修复：原为 vector<ALuint*>，保存局部变量的地址（悬垂指针），改为存值
array<ALuint, 256> g_CurrentKeybdSource;
ALCcontext* g_pALContext = nullptr;

// Hook thread (HandleMouseSound/KeybdSound) calls alcMakeContextCurrent(g_pALContext),
// which makes the context current on the hook thread and sets NULL on ClickSound thread.
// Call this on ClickSound thread before al* calls (after locking g_ALSoundMtx) to restore.
inline void EnsureALContextCurrent()
{
	if (g_pALContext && alcGetCurrentContext() != g_pALContext)
		alcMakeContextCurrent(g_pALContext);
}

std::recursive_mutex g_ALSoundMtx;   // 序列化所有OpenAL上下文/源访问（挂钩线程与ClickSound线程）    保存全局 context

atomic<bool> g_IsLoadedSounds = false;  //音效是否加载完成
atomic<bool> g_IsAudioDeviceChanged = false; // 音频设备变更标志：由主窗口 WndProc（WM_DEVICECHANGE）置位，ClickSound 线程统一重建
HDEVNOTIFY g_hAudioDevNotify = nullptr;  // 音频设备（声卡）变化通知句柄，WndProc WM_CREATE/WM_DESTROY 中注册/注销
HDEVNOTIFY g_hVolumeDevNotify = nullptr; // 卷设备（耳机/扬声器端点）变化通知句柄，同上

unordered_map<string, unordered_map<string, ALuint>> g_MouseSoundSourceList;
unordered_map<string, ALuint> g_CurrentMouseSource;
vector<ALuint> g_MouseSoundBufferList; // 修复：原为 vector<ALuint*>，保存局部变量的地址（悬垂指针），改为存值

// 函数定义
inline void HandleKeybdSound(int nCode, WPARAM wParam, LPARAM lParam);   // 键盘按键声音分发处理器（由统一键盘钩子 KeybdHookProc 调用）
DWORD WINAPI ClickSound(LPVOID lpParam);
bool InitOpenAL();
void LoadSounds();
void DownloadKeybdSounds();
bool LoadMouseSoundPack(const string& dir);
bool LoadKeybdSoundPack(const string& dir);
void ShutdownOpenAL();
bool LoadWaveData(const string& fileName, ALuint* buffer);
void BindSouce(ALuint* source, ALuint* buffer, bool loop);
void CreateSouce(ALuint* source, bool loop);
inline void HandleMouseSound(int nCode, WPARAM wParam, LPARAM lParam);   // 鼠标点击声音分发处理器（由统一鼠标钩子 MouseHookProc 调用）

void DownloadKeybdSounds()
{
	if (_access(Sound_KeybdPath.c_str(), 0) != 0)
		CreateDirectory(Sound_KeybdPath.c_str(), NULL);

	bool IsSuccess = DownloadURLFilesWithProgress(
		URL_GITEE_SOUNDS, Tmp_KeySounds,
		config.UseChinese ? u8"正在下载声音包..." : "Downloading sound pack..."
	);

	if (!IsSuccess)
	{
		PrintLog("键盘声音下载失败或已取消", PL_INFO);
		RenderTip(config.UseChinese ? u8"下载失败或已取消" : "Download failed or cancelled", 3.0f, PT_ERROR);
		return;
	}

	{
		lock_guard<shared_mutex> lock(ProgressBarMsg::Mtx);
		ProgressBarMsg::IsOpen = true;
		ProgressBarMsg::TitleName = config.UseChinese ? u8"正在解压声音包..." : "Extracting sound pack...";
		ProgressBarMsg::MaxSize = 1.0;
		ProgressBarMsg::ReadSize = 0.0;
		ProgressBarMsg::IsUnzip = true;
	}

	int ret = UnZipFile(Tmp_KeySounds.c_str(), Sound_KeybdPath.c_str());
	{
		lock_guard<shared_mutex> lock(ProgressBarMsg::Mtx);
		ProgressBarMsg::IsOpen = false;
		ProgressBarMsg::IsUnzip = false;
	}

	if (ret != 0)
	{
		PrintLog("解压失败，错误代码：" + to_string(ret), PL_ERROR);
		RenderTip((config.UseChinese ? u8"解压失败，错误代码："s : "Extract failed: "s) + to_string(ret), 3.0f, PT_ERROR);
		return;
	}

	PrintLog("下载键盘声音资源成功", PL_INFO);
	RenderTip(config.UseChinese ? u8"键盘声音包下载完成" : "Keybd sound pack installed", 4.0f, PT_INFO);
}

void LoadSounds()
{
	// Lock order: ConfigAccessMutex -> g_ALSoundMtx (same as ClickSound loop) to avoid deadlock,
	// and protects config.MouseSoundType/KeybdSoundType writes during reload.
	lock_guard<shared_mutex> cfgLock(ConfigAccessMutex);
	lock_guard<recursive_mutex> alLock(g_ALSoundMtx);
	EnsureALContextCurrent();
	PrintLog("[Sound] 开始加载音频", PL_INFO);

	vector<string> dirs;

	FindDirectorys(Sound_MousePath, dirs);
	for (const string& dir : dirs)
	{
		const string PackPath = Sound_MousePath + "\\" + dir;
		if (!LoadMouseSoundPack(PackPath))
			continue;
	}
	FindDirectorys(Sound_KeybdPath, dirs); // 里面包含dirs.clear()
	for (const string& dir : dirs)
	{
		const string PackPath = Sound_KeybdPath + "\\" + dir;
		if (!LoadKeybdSoundPack(PackPath))
			continue;
	}

	// 设置默认值
	if (!MouseSoundList.empty())
		config.MouseSoundType = MouseSoundList.at(0);
	else
		config.MouseSoundType = "None";
	if (!KeybdSoundList.empty())
		config.KeybdSoundType = KeybdSoundList.at(0);
	else
		config.KeybdSoundType = "None";
	// 设置当前的声源值
	g_CurrentMouseSource = g_MouseSoundSourceList[config.MouseSoundType];
	g_CurrentKeybdSource = g_KeybdSoundSourceList[config.KeybdSoundType];
}

bool LoadMouseSoundPack(const string& dir)
{
	const string ConfigFile = dir + "\\config.json";

	string PackName;

	string LogContext;
	bool IsError = false;

	ifstream fs(ConfigFile);
	if (!fs.is_open())
	{
		LogContext += "读取声音配置文件失败: " + ConfigFile + "\n";
		IsError = true;
		goto putoutlog;
	}

	try
	{
		picojson::value v;
		string err = picojson::parse(v, fs);
		if (!err.empty())
		{
			LogContext += "解析声音配置文件失败: " + err + "\n";
			IsError = true;
			goto putoutlog;
		}

		if (!v.is<picojson::object>())
		{
			LogContext += "解析声音配置文件失败: 根元素不是对象\n";
			IsError = true;
			goto putoutlog;
		}

		picojson::object& obj = v.get<picojson::object>();

		if (!obj.count("name") || !obj["name"].is<std::string>())
		{
			LogContext += "解析声音配置文件失败: 缺少name字段或类型错误\n";
			IsError = true;
			goto putoutlog;
		}
		PackName = obj["name"].get<std::string>();

		if (!obj.count("defines") || !obj["defines"].is<picojson::object>())
		{
			LogContext += "解析声音配置文件失败: 无效的defines（不是object或不存在）\n";
			IsError = true;
			goto putoutlog;
		}
		picojson::object& defObj = obj["defines"].get<picojson::object>();

		auto& SourceMap = g_MouseSoundSourceList[PackName];
		for (auto& pair : defObj)
		{
			// 值必须是字符串（文件名）
			if (!pair.second.is<std::string>())
			{
				LogContext += "键 " + pair.first + " 的值不是字符串类型，已跳过\n";
				continue;
			}
			const string& FileName = pair.second.get<std::string>();
			const string SoundFile = dir + "\\" + FileName;

			ALuint buffer = 0;
			if (!LoadWaveData(SoundFile, &buffer))
				LogContext += "加载声音文件失败: " + SoundFile + "，已跳过\n";

			ALuint source;
			CreateSouce(&source, false);
			BindSouce(&source, &buffer, false);

			SourceMap[pair.first] = source;
			g_MouseSoundBufferList.push_back(buffer);

			LogContext += "已加载wav文件:" + SoundFile + "\n";
		}

		IsError = false;
	}
	catch (const exception& e)
	{
		LogContext += "JSON处理异常: " + string(e.what()) + "\n";
		IsError = true;
		goto putoutlog;
	}

putoutlog:
	if (IsError)
	{
		PrintLog("[Sound] 处理鼠标声音包时错误：" + ConfigFile + "\n" + LogContext, PL_ERROR);
		return false;
	}
	PrintLog("[Sound] 已处理鼠标声音包：" + ConfigFile + "\n" + LogContext, PL_INFO);
	MouseSoundList.push_back(PackName);
	return true;
}

bool LoadKeybdSoundPack(const string& dir)
{
	const string ConfigFile = dir + "\\config.json";

	string PackName;

	string LogContext;
	bool IsError = false;

	ifstream fs(ConfigFile);
	if (!fs.is_open())
	{
		LogContext += "读取声音配置文件失败: " + ConfigFile + "\n";
		IsError = true;
		goto putoutlog;
	}

	try
	{
		picojson::value v;
		const string& err = picojson::parse(v, fs);
		if (!err.empty())
		{
			LogContext += "解析声音配置文件失败: " + err + "\n";
			IsError = true;
			goto putoutlog;
		}

		if (!v.is<picojson::object>())
		{
			LogContext += "解析声音配置文件失败: 根元素不是对象\n";
			IsError = true;
			goto putoutlog;
		}

		picojson::object& obj = v.get<picojson::object>();

		if (!obj.count("name") || !obj["name"].is<std::string>())
		{
			LogContext += "解析声音配置文件失败: 缺少name字段或类型错误\n";
			IsError = true;
			goto putoutlog;
		}
		PackName = obj["name"].get<std::string>();

		if (!obj.count("key_define_type") || !obj["key_define_type"].is<std::string>())
		{
			LogContext += "解析声音配置文件失败: 缺少key_define_type字段或类型错误\n";
			IsError = true;
			goto putoutlog;
		}
		const string& def_type = obj["key_define_type"].get<std::string>();

		if (obj.count("defines") && obj["defines"].is<picojson::object>())
		{
			picojson::object& defObj = obj["defines"].get<picojson::object>();
			if (def_type == "single") // 单文件定义，通过range切割
			{
				const string& FilePath = dir + "\\" + obj["sound"].get<std::string>();
				// 检查音频文件是否为支持的格式
				{
					// 提取文件扩展名（小写）
					string ext = FilePath;
					size_t dotPos = ext.find_last_of('.');
					if (dotPos != string::npos)
					{
						ext = ext.substr(dotPos + 1);
						transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
					}
					else
						ext = ""; // 无扩展名
					if (ext != "wav" && ext != "mp3" && ext != "ogg")
					{
						LogContext += "不支持的文件类型: " + FilePath + " (仅支持 wav/mp3/ogg)\n";
						IsError = true;
						goto putoutlog;
					}
				}
				WaveInfo info;
				if (!ALFWGetWaveInfo(FilePath.c_str(), info))
				{
					LogContext += "加载声音文件失败: " + FilePath + "，已跳过\n";
					IsError = true;
					goto putoutlog;
				}
				auto& SourceList = g_KeybdSoundSourceList[PackName];
				for (auto& pair : defObj)
				{
					// 检查值是否为数组
					if (pair.second.is<picojson::array>())
					{
						auto& arr = pair.second.get<picojson::array>();
						// 确保数组有2个元素且都是数字
						if (arr.size() == 2 && arr[0].is<double>() && arr[1].is<double>())
						{
							int begin = static_cast<int>(arr[0].get<double>());
							int offeset = static_cast<int>(arr[1].get<double>());
							int scanKey;
							try { scanKey = stoi(pair.first); }
							catch (...) { LogContext += "Scan转Vk时出现错误，已跳过\n"; continue; }
							auto itScan = g_KeyScanToVk.find(NormalizeScanCode(scanKey));
							if (itScan == g_KeyScanToVk.end())
								itScan = g_KeyScanToVk.find(scanKey);  // 兼容无标准扩展表示的特殊键（如 Pause=3653）
							if (itScan == g_KeyScanToVk.end())
							{
								LogContext += "未识别的扫描码: " + pair.first + "，已跳过\n";
								continue;
							}
							int vk = itScan->second;
							ALuint buffer;
							alGenBuffers(1, &buffer);

							// 配置文件里的 begin/offeset 单位是毫秒，需要转成 sample
							int StartSample = begin * info.freq / 1000;
							int NumSamples = offeset * info.freq / 1000;
							if (NumSamples <= 0)
								NumSamples = 1; // 至少 1 个 sample

							if (!ALFWSplitDataToBuffer(buffer, info, StartSample, NumSamples)) // 分割
							{
								LogContext += "分割音频时，分割的范围超出音频范围: " + FilePath + "\n";
								alDeleteBuffers(1, &buffer);
								IsError = true;
								goto putoutlog;
							}

							ALuint source;
							CreateSouce(&source, false);
							BindSouce(&source, &buffer, false);
							SourceList[vk] = source;
							g_KeybdSoundBufferList.push_back(buffer);
						}
						else
							LogContext += "键 " + pair.first + " 的数组格式不正确\n";
					}
					else
						LogContext += "键 " + pair.first + " 的值不是数组\n";
				}
			} 
			else if (def_type == "multi") // 多文件定义，加载不同的文件
			{
				auto& SourceList = g_KeybdSoundSourceList[PackName];
				for (auto& pair : defObj)
				{
					// 检查值是否为字符串（字符串是音频文件名字）
					if (pair.second.is<std::string>())
					{
						const string& FilePath = dir + "\\" + pair.second.get<std::string>();
						// 检查音频文件是否为支持的格式
						{
							// 提取文件扩展名（小写）
							string ext = FilePath;
							size_t dotPos = ext.find_last_of('.');
							if (dotPos != string::npos)
							{
								ext = ext.substr(dotPos + 1);
								transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
							}
							else
								ext = ""; // 无扩展名
							if (ext != "wav" && ext != "mp3" && ext != "ogg")
							{
								LogContext += "不支持的文件类型: " + FilePath + " (仅支持 wav/mp3/ogg)\n";
								continue;
							}
						}
						WaveInfo info; // 里面包含了buffer的数据
						if (!ALFWGetWaveInfo(FilePath.c_str(), info))
							LogContext += "加载声音文件失败: " + FilePath + "，已跳过\n";
						else
						{
							int scanKey;
							try { scanKey = stoi(pair.first); }
							catch (...) { LogContext += "Scan转Vk时出现错误，已跳过\n"; continue; }
							auto itScan = g_KeyScanToVk.find(NormalizeScanCode(scanKey));
							if (itScan == g_KeyScanToVk.end())
								itScan = g_KeyScanToVk.find(scanKey);  // 兼容无标准扩展表示的特殊键（如 Pause=3653）
							if (itScan == g_KeyScanToVk.end())
							{
								LogContext += "未识别的扫描码: " + pair.first + "，已跳过\n";
								continue;
							}
							int vk = itScan->second;

							ALuint buffer;
							alGenBuffers(1, &buffer);

							ALuint source;
							CreateSouce(&source, false);
							BindSouce(&source, &buffer, false);
							SourceList[vk] = source;
							g_KeybdSoundBufferList.push_back(buffer);
						}
					}
					else
						LogContext += "键 " + pair.first + " 的值不是字符串类型\n";
				}
			}
			else
				LogContext += "key_define_type定义了一个无效的类型：" + def_type + "\n";
		}
		else
		{
			LogContext += "解析声音配置文件失败: 无效的defines（不是object或不存在）\n";
			IsError = true;
			goto putoutlog;
		}

		IsError = false;
	}
	catch (const exception& e)
	{
		LogContext += "JSON处理异常: " + string(e.what()) + "\n";
		IsError = true;
		goto putoutlog;
	}
putoutlog:
	if (IsError)
	{
		PrintLog("在处理声音包时错误：" + ConfigFile + LogContext, PT_ERROR);
		return false;
	}
	PrintLog("已处理声音包：" + ConfigFile + LogContext, PT_ERROR);
	KeybdSoundList.push_back(PackName); // 添加到列表，这样用户才能选择
	return true;
}

bool InitOpenAL()
{
	// 获取所有可用的音频设备
	const ALCchar* devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
	vector<string> deviceList;
	const ALCchar* device = devices, * next = devices + 1;
	size_t len = 0;

	while (device && *device != '\0' && next && *next != '\0')
	{
		deviceList.push_back(string(device));
		len = strlen(device);
		device += (len + 1);
		next += (len + 2);
	}

	// 如果默认设备不可用，尝试打开每个设备
	ALCdevice* pDevice = alcOpenDevice(NULL);
	if (!pDevice)
	{
		PrintLog("[Sound]无法打开默认音频设备，尝试打开其他设备", PL_WARNING);
		for (const auto& devName : deviceList)
		{
			pDevice = alcOpenDevice(devName.c_str());
			if (pDevice)
			{
				PrintLog("[Sound]成功打开音频设备: " + devName, PL_INFO);
				break;
			}
		}
	}
	else
		PrintLog("[Sound]成功打开默认音频设备", PL_INFO);

	if (!pDevice)
	{
		PrintLog("[Sound]无法打开音频设备。", PL_ERROR);
		return false;
	}

	// 创建上下文
	ALCcontext* pContext = alcCreateContext(pDevice, NULL);
	if (!pContext)
	{
		PrintLog("[Sound]无法创建上下文。", PL_ERROR);
		alcCloseDevice(pDevice);
		return false;
	}
	// 设置当前上下文
	alcMakeContextCurrent(pContext);
	g_pALContext = pContext;   // 保存全局 context
	return true;
}

void ShutdownOpenAL()
{
	ALCcontext* pContext = alcGetCurrentContext();
	if (!pContext)   // 修复：判空，防止对 NULL context 操作（未定义行为）
	{
		g_pALContext = nullptr;
		return;
	}

	ALCdevice* pDevice = alcGetContextsDevice(pContext);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(pContext);
	if (pDevice)
		alcCloseDevice(pDevice);

	g_pALContext = nullptr; 
}

bool LoadWaveData(const string& fileName, ALuint* buffer)
{
	alGenBuffers(1, buffer);
	return ALFWLoadWaveToBuffer(fileName.c_str(), *buffer);
}

void CreateSouce(ALuint* source, bool loop = true)
{
	alGenSources(1, source);
	// 源声音的位置
	ALfloat sourcePosition[] = { 0.0f, 0.0f, 0.0f };
	// 源声音的速度
	ALfloat sourceVelocity[] = { 0.0f, 0.0f, 0.0f };
	// 源声音的方向
	ALfloat sourceDirection[] = { 0.0f, 0.0f, 0.0f };

	alSourcef(*source, AL_PITCH, 1.0f);
	alSourcef(*source, AL_GAIN, 1.0f);
	alSourcefv(*source, AL_POSITION, sourcePosition);
	alSourcefv(*source, AL_VELOCITY, sourceVelocity);
	alSourcefv(*source, AL_DIRECTION, sourceDirection);
	alSourcei(*source, AL_LOOPING, loop);
}

void BindSouce(ALuint* source, ALuint* buffer, bool loop = true)
{
	alSourcei(*source, AL_BUFFER, *buffer);
	alSourcei(*source, AL_LOOPING, loop);
}

DWORD WINAPI ClickSound(LPVOID lpParam)
{
	while (IsFirstRun)
		Sleep(1000);

	ALFWInit();

	{
		lock_guard<recursive_mutex> alLock(g_ALSoundMtx);
		EnsureALContextCurrent();
		if (!InitOpenAL())
		{
			PrintLog("[Sound] 无法初始化OpenAL", PL_ERROR);
			RenderTip(u8"[Sound] 无法初始化鼠标声音(OpenAL)", 2.0f, PT_ERROR);
			ShutdownOpenAL();
			return -1;
		}
		PrintLog("[Sound] 初始化OpenAL成功", PL_INFO);
	}

	ALuint NoisBuffer;
	ALuint NoisSource;

	if (!LoadWaveData(Sound_Noise, &NoisBuffer))
	{
		RenderTip(u8"无法加载wav文件:" + Sound_Noise, 2.0f, PT_ERROR);
		PrintLog("[Sound] 无法加载wav文件:" + Sound_Noise, PL_ERROR);
		return -2;
	}

	CreateSouce(&NoisSource, true);
	BindSouce(&NoisSource, &NoisBuffer, true);
	
	// 加载声音
	LoadSounds();

	if(config.UseMouseSound)
		alSourcePlay(NoisSource);
	g_IsLoadedSounds = true;
	// 鼠标点击声不再由 MouseSoundThread 轮询 GetKeyClick，改由统一鼠标钩子（MouseHookProc -> HandleMouseSound）事件驱动

	{
		lock_guard<shared_mutex> lock(InitSuccessTimeMutex);
		if (InitSuccessTime == 0 || InitSuccessTime < GetNowMs())
			InitSuccessTime = GetNowMs();
	}

	while (!IsGCExit)
	{
		// 修复：循环内持续读取 config（UseMouseSound/NoiseVolume/音量/类型等），持读锁与主线程 config 重置互斥
		shared_lock<shared_mutex> cfgLock(ConfigAccessMutex);
		if (VarHasChanged(config.UseMouseSound, "EnableUseMouseSound"))
		{
			std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
			EnsureALContextCurrent();
			if(config.UseMouseSound)
				alSourcePlay(NoisSource);
			else
				alSourceStop(NoisSource);
		}

		if (VarHasChanged(config.NoiseVolume, "NoiseVolume", true))
		{
			std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
			EnsureALContextCurrent();
			alSourcef(NoisSource, AL_GAIN, config.NoiseVolume / 100.0f);
		}

		if (VarHasChanged(config.MouseSoundVolume, "MouseSoundVolume", true))
		{
			std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
			EnsureALContextCurrent();
			float volume = config.MouseSoundVolume / 100.0f;
			for (auto& sources : g_MouseSoundSourceList)
			{
				for (auto& source : sources.second)
					alSourcef(source.second, AL_GAIN, volume);
			}
		}
		if (VarHasChanged(config.MouseSoundType, "MouseSoundType", true))
		{
			std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
			EnsureALContextCurrent();
			auto it = g_MouseSoundSourceList.find(config.MouseSoundType);
			if (it == g_MouseSoundSourceList.end())
				g_CurrentMouseSource.clear();   // 修复：找不到时静音，避免 operator[] 插入垃圾条目
			else
				g_CurrentMouseSource = it->second;
		}

		if (VarHasChanged(config.KeybdSoundVolume, "KeybdSoundVolume", true))
		{
			std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
			EnsureALContextCurrent();
			float volume = (config.KeybdSoundVolume / 100.0f) * 1.2f;
			for (auto& sources : g_KeybdSoundSourceList)
			{
				for (auto& source : sources.second)
				{
					if (source > 0)
						alSourcef(source, AL_GAIN, volume);
				}
			}
		}
		if (VarHasChanged(config.KeybdSoundType, "KeybdSoundType", true))
		{
			std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
			EnsureALContextCurrent();
			auto it = g_KeybdSoundSourceList.find(config.KeybdSoundType);
			if (it == g_KeybdSoundSourceList.end())
				g_CurrentKeybdSource.fill(0);   // 修复：找不到时静音，避免 operator[] 插入垃圾条目
			else
				g_CurrentKeybdSource = it->second;
		}
		if (g_IsAudioDeviceChanged.exchange(false))
		{
			std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
			EnsureALContextCurrent();
			// 修复：设备切换统一在 ClickSound 线程重建（唯一执行 OpenAL 生命周期管理的线程），
			// 避免 detach 线程并发 Shutdown/Init 与其它线程持有失效上下文的竞态
			g_IsLoadedSounds = false;
			ShutdownOpenAL();
			Sleep(1000);
			if (!InitOpenAL())
				PrintLog("[Sound] 重新打开音频设备失败", PL_ERROR);
			g_IsLoadedSounds = true;
		}

		Sleep(500);
	}

	{
		std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
		EnsureALContextCurrent();
		for (auto& sources : g_MouseSoundSourceList)
		{
			for (auto& source : sources.second)
				alDeleteSources(1, &source.second);
		}
	}
	{
		std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
		EnsureALContextCurrent();
		for (ALuint buffer : g_MouseSoundBufferList)
		{
			if (buffer != 0)
				alDeleteBuffers(1, &buffer);
		}
	}

	{
		std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
		EnsureALContextCurrent();
		for (auto& sources : g_KeybdSoundSourceList)
		{
			for (auto& source : sources.second)
			{
				if (source > 0)
					alDeleteSources(1, &source);
			}
		}
	}
	{
		std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
		EnsureALContextCurrent();
		for (ALuint buffer : g_KeybdSoundBufferList)
		{
			if (buffer != 0)
				alDeleteBuffers(1, &buffer);
		}
	}

	// 修复：噪声源/缓冲此前从未释放
	{
		std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
		EnsureALContextCurrent();
		alDeleteSources(1, &NoisSource);
		alDeleteBuffers(1, &NoisBuffer);
	}

	// 修复：原代码 ALFWShutdownOpenAL() 紧接着再调 ShutdownOpenAL()，会对同一上下文双重销毁；
	// 第二次时当前上下文已为 NULL，alcGetContextsDevice(NULL)/alcDestroyContext(NULL) 属未定义行为。
	// 统一只调用一次自带判空的 ShutdownOpenAL()。
	{
		std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
		EnsureALContextCurrent();
		ShutdownOpenAL();
	}
	ALFWShutdown();

	PrintLog("[Sound] OpenAL已关闭", PL_INFO);
	return 0;
}

inline void HandleMouseSound(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return;

	// 读 config 快照（短读锁，分发线程与主线程 config 重置互斥）
	bool UseMouseSoundNow = false;
	float BaseVolume = 0.0f;
	{
		shared_lock<shared_mutex> cfgLock(ConfigAccessMutex);
		UseMouseSoundNow = config.UseMouseSound;
		BaseVolume = config.MouseSoundVolume / 100.0f;
	}
	if (!UseMouseSoundNow || !g_IsLoadedSounds)
		return;

	std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);

	if (g_pALContext && alcGetCurrentContext() != g_pALContext)
		alcMakeContextCurrent(g_pALContext);

	auto PlayWithVariation = [&](ALuint source)
	{
		if (source == 0)
			return;

		float volVariation = 0.1f; // +-10%
		float randomVol = BaseVolume * (1.0f + volVariation * ((rand() % 1000) / 500.0f - 1.0f));
		randomVol = max(0.0f, min(2.0f, randomVol)); // 限制在 0~2.0
		alSourcef(source, AL_GAIN, randomVol);

		// 音调随机 +-20%，1.0 为原始音调
		float pitchVariation = 0.2f;
		float randomPitch = 1.0f + pitchVariation * ((rand() % 1000) / 500.0f - 1.0f);
		randomPitch = max(0.4f, min(1.6f, randomPitch)); // 限制在 0.4~1.6
		alSourcef(source, AL_PITCH, randomPitch);

		alSourcePlay(source);
	};

	// 修复：改用 find，避免 operator[] 在键不存在时反复插入默认空条目
	auto PlayMouse = [&](const string& key)
	{
		auto it = g_CurrentMouseSource.find(key);
		if (it != g_CurrentMouseSource.end())
			PlayWithVariation(it->second);
	};

	// 鼠标点击声音由统一鼠标钩子事件驱动（WM_LBUTTONDOWN/UP、WM_RBUTTONDOWN/UP），
	// 替代原 MouseSoundThread 的 GetKeyClick/GetKeyUp 轮询，按下/抬起事件一次触发一次
	switch (wParam)
	{
		case WM_LBUTTONDOWN: PlayMouse("1");  break;
		case WM_LBUTTONUP:   PlayMouse("01"); break;
		case WM_RBUTTONDOWN: PlayMouse("2");  break;
		case WM_RBUTTONUP:   PlayMouse("02"); break;
	}
	if (!config.UseOldMethodToTrigInter)
	{
		if (config.UseLClickerTrigInter)
		{
			if (wParam == WM_LBUTTONDOWN)
				PlayMouse("01");
		}
		if (config.UseRClickerTrigInter)
		{
			if (wParam == WM_RBUTTONDOWN)
				PlayMouse("02");
		}
	}
}

inline void HandleKeybdSound(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return;

	bool UseKeybdSoundNow = false, UseMouseSoundNow = false;
	{
		// 低级键盘钩子与主线程并发读写 config，此前无锁存在数据竞争；这里一次性拷贝所需字段
		// （仅短暂持读锁，避免在钩子内长时间持锁影响系统超时移除钩子）
		shared_lock<shared_mutex> cfgLock(ConfigAccessMutex);
		UseKeybdSoundNow = config.UseKeybdSound;
		UseMouseSoundNow = config.UseMouseSound;
	}
	if ((UseKeybdSoundNow || UseMouseSoundNow) && g_IsLoadedSounds)
	{
		std::lock_guard<std::recursive_mutex> alLock(g_ALSoundMtx);
		// 说明：WH_KEYBOARD_LL 钩子收不到 DBT_* 设备消息（wParam 只会是 WM_KEYDOWN / WM_KEYUP 等），
		// 原“设备切换”分支是死代码且线程不安全，已移除。音频设备插拔现由主窗口 WndProc 的
		// WM_DEVICECHANGE + RegisterDeviceNotification 监听（见 GC.h），置 g_AudioDeviceChanged 后
		// 由 ClickSound 线程统一重建，不再多线程直接操作 OpenAL。
		if (UseKeybdSoundNow)
		{
			if (g_pALContext && alcGetCurrentContext() != g_pALContext)
				alcMakeContextCurrent(g_pALContext);

			KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
			int rawVk = (int)pKeyBoard->vkCode;
			if (rawVk < 0 || rawVk >= 256)
				return;

			// 修复（Ctrl/Alt/Shift 无声根因）：WH_KEYBOARD_LL 的 vkCode 对左右修饰键返回区分左右的
			// VK（VK_LCONTROL=0xA2、VK_RMENU=0xA5 等），而声音包加载时 KeyScanToVk 只映射为通用
			// VK（VK_CONTROL=0x11、VK_MENU=0x12），导致按 0xA2..0xA5 索引 CurrentKeybdSource 恒为 0
			// 而无声音。这里归一化为通用 VK 后再查表；物理状态仍按原始 vkCode 记录，左右键可独立触发。
			int vk = rawVk;
			switch (rawVk)
			{
				case VK_LSHIFT:   case VK_RSHIFT:   vk = VK_SHIFT;   break;
				case VK_LCONTROL: case VK_RCONTROL: vk = VK_CONTROL; break;
				case VK_LMENU:    case VK_RMENU:    vk = VK_MENU;    break;
			}

			static bool KeyPhysicalState[256] = { false };

			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
			{
				// 只有当前物理状态为 false（之前确实没有按下）时，才触发声音
				if (!KeyPhysicalState[rawVk])
				{
					KeyPhysicalState[rawVk] = true;// 标记为已按下

					ALuint src = g_CurrentKeybdSource[vk];
					// 当前声音包未定义该按键时，回退播放该声音包第一个有效的按键音频，避免静音
					if (src == 0)
					{
						for (int i = 1; i < 256; ++i)
						{
							if (g_CurrentKeybdSource[i] != 0)
							{
								src = g_CurrentKeybdSource[i];
								break;
							}
						}
					}
					if (src != 0)
						alSourcePlay(src);
				}
				// 如果KeyPhysicalState[rawVk]已经是true，直接忽略（无论系统 flags 是什么）
			}
			else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
				KeyPhysicalState[rawVk] = false;
		}
	}
}