#pragma once
#include "maths.h"
#include "MainClick.h"
#include "ALFamework/Famewrok.h"
#include "json/picojson.h"
#include <Dbt.h>

#include <unordered_map>

unordered_map<int, int> KeyScanToVk = {
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
	// 功能键区
	{3639, VK_SNAPSHOT}, {70, VK_SCROLL}, {3653, VK_PAUSE},
	{3666, VK_INSERT}, {3667, VK_DELETE}, {3655, VK_HOME}, {3663, VK_END}, {3657, VK_PRIOR}, {3665, VK_NEXT},
	{57416, VK_UP}, {57419, VK_LEFT}, {57421, VK_RIGHT}, {57424, VK_DOWN},
	// Shift/Ctrl/Alt
	{42, VK_SHIFT}, {54, VK_SHIFT}, {29, VK_CONTROL}, {3613, VK_CONTROL}, {56, VK_MENU}, {3640, VK_MENU},
	{3675, VK_LWIN}, {3676, VK_RWIN}, {3677, VK_APPS},
	// 数字小键盘
	{69, VK_NUMLOCK}, {3637, VK_DIVIDE}, {55, VK_MULTIPLY}, {74, VK_SUBTRACT},
	{78, VK_ADD}, {3612, VK_RETURN}, {83, VK_DECIMAL},
	{79, VK_NUMPAD1}, {80, VK_NUMPAD2}, {81, VK_NUMPAD3}, {75, VK_NUMPAD4},
	{76, VK_NUMPAD5}, {77, VK_NUMPAD6}, {71, VK_NUMPAD7}, {72, VK_NUMPAD8}, {73, VK_NUMPAD9}, {82, VK_NUMPAD0},
	// Windows 特殊键 (61000+)
	{61010, VK_INSERT}, {61011, VK_DELETE}, {60999, VK_HOME}, {61007, VK_END},
	{61001, VK_PRIOR}, {61009, 0x22}, {61000, VK_UP}, {61003, VK_LEFT}, {61005, VK_RIGHT}, {61008, VK_DOWN}
};
struct SoundRange
{
	int begin;
	int offeset;
};
array<SoundRange, 256> VkToSoundRange;
unordered_map<string, array<ALuint, 256>> KeybdSoundSourceList;
vector<ALuint*> KeybdSoundBufferList;
array<ALuint, 256> CurrentKeybdSource;
ALCcontext* g_pALContext = nullptr;   // 保存全局 context

unordered_map<string, unordered_map<string, ALuint>> MouseSoundSourceList;
unordered_map<string, ALuint> CurrentMouseSource;
vector<ALuint*> MouseSoundBufferList;

// 函数定义
LRESULT CALLBACK KeybdSoundHook(int nCode, WPARAM wParam, LPARAM lParam);
DWORD WINAPI ClickSound(LPVOID lpParam);
bool InitOpenAL();
void LoadSounds();
void DownloadKeybdSounds();
bool ParseMouseSoundConfig(string& name, unordered_map<string, string>& defines, const string& file);
bool ParseKeybdSoundConfig(string& name, unordered_map<string, SoundRange>& defines, const string& file);
void ShutdownOpenAL();
bool LoadWaveData(const string& fileName, ALuint* buffer);
void BindSouce(ALuint* source, ALuint* buffer, bool loop);
void CreateSouce(ALuint* source, bool loop);
DWORD WINAPI MouseSoundThread(LPVOID lpParam);

void LoadSounds()
{
	PrintLog("[Sound] 开始加载音频", PL_INFO);

	vector<string> dirs;
	FindDirectorys(Sound_MousePath, dirs);
	for (const string& dir : dirs)
	{
		const string file = Sound_MousePath + "\\" + dir + "\\config.json";
		string name;
		unordered_map<string, string> defines;
		if (!ParseMouseSoundConfig(name, defines, file))
			continue;
		for (auto& pair : defines)
		{
			ALuint buffer = 0;
			const string SoundFile = Sound_MousePath + "\\" + dir + "\\" + pair.second;

			if (!LoadWaveData(SoundFile, &buffer))
			{
				static bool IsShow = false;
				if (!IsShow)
				{
					//RenderMessageBox(config.UseChinese ? u8"无法加载一些鼠标声音的文件" : "Cannot load some mouse sound files", config.UseChinese ? u8"错误" : "Error", MB_OK, false);
					IsShow = true;
				}
				PrintLog("[Sound] 加载声音文件失败: " + SoundFile, PL_ERROR);
				continue;
			}

			ALuint source;
			CreateSouce(&source, false);
			BindSouce(&source, &buffer, false);
			MouseSoundSourceList[name][pair.first] = source;
			MouseSoundBufferList.push_back(&buffer);
			PrintLog("[Sound] 已加载wav文件:" + SoundFile, PL_INFO);
		}
		MouseSoundList.push_back(name);
	}

	FindDirectorys(Sound_KeybdPath, dirs); // 里面包含dirs.clear()
	for (const string& dir : dirs)
	{
		const string file = Sound_KeybdPath + "\\" + dir + "\\config.json";
		string name;
		unordered_map<string, SoundRange> defines;
		if (!ParseKeybdSoundConfig(name, defines, file))
			continue;
		const string SoundFile = Sound_KeybdPath + "\\" + dir + "\\sound.wav";

		WaveInfo info; // 里面包含了buffer的数据
		if (!ALFWGetWaveInfo(SoundFile.c_str(), info))
		{
			static bool IsShow = false;
			if (!IsShow)
			{
				//RenderMessageBox(config.UseChinese ? (u8"无法加载键盘声音文件: " + dir) : ("Cannot load keyboard sound file: " + dir), config.UseChinese ? u8"错误" : "Error", MB_OK, false);
				IsShow = true;
			}
			PrintLog("[Sound] 加载声音文件失败: " + SoundFile, PL_ERROR);
			continue;
		}

		for (auto& pair : defines)
		{
			int vk;
			try
			{
				vk = KeyScanToVk[stoi(pair.first)];
			}
			catch (...)
			{
				PrintLog("[Sound] 加载声音配置文件失败: 无效的defines", PL_ERROR);
				continue;
			}
			ALuint buffer;
			alGenBuffers(1, &buffer);

			// 配置文件里的 begin/offeset 单位是毫秒，需要转成 sample
			int startSample = pair.second.begin * info.freq / 1000;
			int numSamples = pair.second.offeset * info.freq / 1000;
			if (numSamples <= 0)
				numSamples = 1; // 至少 1 个 sample

			if (!ALFWSplitBuffer(buffer, info, startSample, numSamples)) // 分割
			{
				PrintLog("[Sound] 分割声音文件失败: " + SoundFile, PL_ERROR);
				alDeleteBuffers(1, &buffer);
				continue;
			}

			ALuint source;
			CreateSouce(&source, false);
			BindSouce(&source, &buffer, false);
			KeybdSoundSourceList[name][vk] = source;
			KeybdSoundBufferList.push_back(&buffer);
		}
		KeybdSoundList.push_back(name);
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
	CurrentMouseSource = MouseSoundSourceList[config.MouseSoundType];
	CurrentKeybdSource = KeybdSoundSourceList[config.KeybdSoundType];
}

void DownloadKeybdSounds()
{
	if (_access(Tmp_TempPath.c_str(), 0) != 0)
		CreateDirectory(Tmp_TempPath.c_str(), NULL);
	if (_access(Sound_KeybdPath.c_str(), 0) != 0)
		CreateDirectory(Sound_KeybdPath.c_str(), NULL);

	bool ok = DownloadURLFilesWithProgress(
		URL_GITEE_SOUNDS, Tmp_KeySounds,
		config.UseChinese ? u8"正在下载声音包..." : "Downloading sound pack..."
	);

	if (!ok)
	{
		PrintLog("键盘声音下载失败或已取消", PL_INFO);
		RenderTip(config.UseChinese ? u8"下载失败或已取消" : "Download failed or cancelled", 3.0f, PT_ERROR);
		return;
	}

	// 解压
	pb_message.open = true;
	pb_message.title = config.UseChinese ? u8"正在解压声音包..." : "Extracting sound pack...";
	pb_message.maxmb = 1.0;
	pb_message.mb = 0.0;
	pb_message.unzip = true;

	int ret = UnZipFile(Tmp_KeySounds.c_str(), Sound_KeybdPath.c_str());
	pb_message.open = false;
	pb_message.unzip = false;

	if (ret != 0)
	{
		PrintLog("解压失败，错误代码：" + to_string(ret), PL_ERROR);
		RenderTip((config.UseChinese ? u8"解压失败，错误代码："s : "Extract failed: "s) + to_string(ret), 3.0f, PT_ERROR);
		return;
	}

	PrintLog("下载键盘声音资源成功", PL_INFO);
	RenderTip(config.UseChinese ? u8"键盘声音包下载完成" : "Keybd sound pack installed", 4.0f, PT_INFO);
}

bool ParseMouseSoundConfig(string& name, unordered_map<string, string>& defines, const string& file)
{
	ifstream fs(file);
	if (!fs.is_open())
	{
		PrintLog("[Sound] 读取声音配置文件失败: " + file, PL_ERROR);
		return false;
	}
	try
	{
		picojson::value v;
		string err = picojson::parse(v, fs);
		if (!err.empty())
		{
			PrintLog("[Sound] 解析声音配置文件失败: " + err, PL_ERROR);
			return false;
		}

		if (!v.is<picojson::object>())
		{
			PrintLog("[Sound] 解析声音配置文件失败: 根元素不是对象", PL_ERROR);
			return false;
		}

		picojson::object& obj = v.get<picojson::object>();

		if (!obj.count("name") || !obj["name"].is<std::string>())
		{
			PrintLog("[Sound] 解析声音配置文件失败: 缺少name字段或类型错误", PL_ERROR);
			return false;
		}
		name = obj["name"].get<std::string>();

		if (obj.count("defines") && obj["defines"].is<picojson::object>())
		{
			picojson::object& defObj = obj["defines"].get<picojson::object>();

			for (auto& pair : defObj)
			{
				if (pair.second.is<std::string>())
					defines[pair.first] = pair.second.get<std::string>();
				else
					PrintLog("[Sound] 警告: defines中的值不是字符串类型", PL_WARNING);
			}
		}
		else
		{
			PrintLog("[Sound] 解析声音配置文件失败: 无效的defines（不是object或不存在）", PL_ERROR);
			return false;
		}

		if (defines.empty())
			PrintLog("[Sound] 警告: 没有解析到任何有效的defines配置", PL_WARNING);
		else
			PrintLog("[Sound] 成功加载配置: " + name + ", 定义了" + to_string(defines.size()) + "个声音", PL_INFO);
		return true;
	}
	catch (const exception& e)
	{
		PrintLog("[Sound] JSON处理异常: " + string(e.what()), PL_ERROR);
		return false;
	}
	return false;
}

bool ParseKeybdSoundConfig(string& name, unordered_map<string, SoundRange>& defines, const string& file)
{
	ifstream fs(file);
	if (!fs.is_open())
	{
		PrintLog("[Sound] 读取声音配置文件失败: " + file, PL_ERROR);
		return false;
	}
	try
	{
		picojson::value v;
		string err = picojson::parse(v, fs);
		if (!err.empty())
		{
			PrintLog("[Sound] 解析声音配置文件失败: " + err, PL_ERROR);
			return false;
		}

		if (!v.is<picojson::object>())
		{
			PrintLog("[Sound] 解析声音配置文件失败: 根元素不是对象", PL_ERROR);
			return false;
		}

		picojson::object& obj = v.get<picojson::object>();

		if (!obj.count("name") || !obj["name"].is<std::string>())
		{
			PrintLog("[Sound] 解析声音配置文件失败: 缺少name字段或类型错误", PL_ERROR);
			return false;
		}
		name = obj["name"].get<std::string>();

		if (obj.count("defines") && obj["defines"].is<picojson::object>())
		{
			picojson::object& defObj = obj["defines"].get<picojson::object>();

			for (auto& pair : defObj)
			{
				// 检查值是否为数组
				if (pair.second.is<picojson::array>())
				{
					auto& arr = pair.second.get<picojson::array>();
					// 确保数组有2个元素且都是数字
					if (arr.size() == 2 && arr[0].is<double>() && arr[1].is<double>())
					{
						defines[pair.first].begin = static_cast<int>(arr[0].get<double>());
						defines[pair.first].offeset = static_cast<int>(arr[1].get<double>());
					}
					else
						PrintLog("[Sound] 警告: 键 " + pair.first + " 的数组格式不正确", PL_WARNING);
				}
				else
					PrintLog("[Sound] 警告: 键 " + pair.first + " 的值不是数组", PL_WARNING);
			}
		}
		else
		{
			PrintLog("[Sound] 解析声音配置文件失败: 无效的defines（不是object或不存在）", PL_ERROR);
			return false;
		}

		if (defines.empty())
			PrintLog("[Sound] 警告: 没有解析到任何有效的defines配置", PL_WARNING);
		else
			PrintLog("[Sound] 成功加载配置: " + name + ", 定义了" + to_string(defines.size()) + "个声音", PL_INFO);
		return true;
	}
	catch (const exception& e)
	{
		PrintLog("[Sound] JSON处理异常: " + string(e.what()), PL_ERROR);
		return false;
	}
	return false;
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
	ALCcontext* pContext;
	ALCdevice* pDevice;

	pContext = alcGetCurrentContext();
	pDevice = alcGetContextsDevice(pContext);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(pContext);
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

	PrintLog("[Sound]开始初始化OpenAL", PL_INFO);
	if (!InitOpenAL())
	{
		PrintLog("[Sound]无法初始化OpenAL", PL_ERROR);
		RenderTip(u8"[Sound]无法初始化鼠标声音(OpenAL)", 2.0f, PT_ERROR);
		ShutdownOpenAL();
		return -1;
	}

	ALuint NoisBuffer;
	ALuint NoisSource;

	if (!LoadWaveData(Sound_Noise, &NoisBuffer))
	{
		RenderTip(u8"[Sound]无法初始化无法加载wav文件:" + Sound_Noise, 2.0f, PT_ERROR);
		PrintLog("[Sound]无法加载wav文件:" + Sound_Noise, PL_ERROR);
		return -2;
	}

	CreateSouce(&NoisSource, true);
	BindSouce(&NoisSource, &NoisBuffer, true);
	
	// 加载声音
	LoadSounds();

	if(config.UseMouseSound)
		alSourcePlay(NoisSource);
	SoundIsOpen = true;
	CreateThreadSafe(MouseSoundThread, NULL, 0, "MouseSoundThread");

	{
		lock_guard<shared_mutex> lock(InitSuccessTimeMutex);
		if (InitSuccessTime == 0 || InitSuccessTime < GetNowMs())
			InitSuccessTime = GetNowMs();
	}

	while (!GCExit)
	{
		if (VarHasChanged(config.UseMouseSound, "EnableUseMouseSound"))
		{
			if(config.UseMouseSound)
				alSourcePlay(NoisSource);
			else
				alSourceStop(NoisSource);
		}

		if (VarHasChanged(config.NoiseVolume, "NoiseVolume", true))
			alSourcef(NoisSource, AL_GAIN, config.NoiseVolume / 100.0f);

		if (VarHasChanged(config.MouseSoundVolume, "MouseSoundVolume", true))
		{
			float volume = config.MouseSoundVolume / 100.0f;
			for (auto& sources : MouseSoundSourceList)
			{
				for (auto& source : sources.second)
					alSourcef(source.second, AL_GAIN, volume);
			}
		}
		if (VarHasChanged(config.MouseSoundType, "MouseSoundType", true))
		{
			auto it = MouseSoundSourceList.find(config.MouseSoundType);
			if (it == MouseSoundSourceList.end())
				CurrentMouseSource = MouseSoundSourceList["none"];
			else
				CurrentMouseSource = it->second;
		}

		if (VarHasChanged(config.KeybdSoundVolume, "KeybdSoundVolume", true))
		{
			float volume = (config.KeybdSoundVolume / 100.0f) * 1.2f;
			for (auto& sources : KeybdSoundSourceList)
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
			auto it = KeybdSoundSourceList.find(config.KeybdSoundType);
			if (it == KeybdSoundSourceList.end())
				CurrentKeybdSource = KeybdSoundSourceList["none"];
			else
				CurrentKeybdSource = it->second;
		}
		Sleep(500);
	}

	for (auto& sources : MouseSoundSourceList)
	{
		for (auto& source : sources.second)
			alDeleteSources(1, &source.second);
	}
	for (auto& buffer : MouseSoundBufferList)
		alDeleteBuffers(1, buffer);

	for (auto& buffer : KeybdSoundBufferList)
		alDeleteBuffers(1, buffer);

	for (auto& sources : KeybdSoundSourceList)
	{
		for (auto& source : sources.second)
		{
			if (source > 0)
				alDeleteSources(1, &source);
		}
	}

	if(!ALFWShutdownOpenAL())
		PrintLog("[Sound] 无法关闭OpenAL", PL_WARNING);

	ShutdownOpenAL();
	ALFWShutdown();

	PrintLog("[Sound] OpenAL已关闭", PL_INFO);
	return 0;
}

DWORD WINAPI MouseSoundThread(LPVOID lpParam)
{
	srand(time(NULL));
	while (!GCExit)
	{
		if (!config.UseMouseSound || !SoundIsOpen)
		{
			Sleep(100);
			continue;
		}

		auto PlayWithVariation = [&](ALuint source)
		{
			if (source == 0)
				return;

			float baseVol = config.MouseSoundVolume / 100.0f;
			float volVariation = 0.1f; // +-10%
			float randomVol = baseVol * (1.0f + volVariation * ((rand() % 1000) / 500.0f - 1.0f));
			randomVol = max(0.0f, min(2.0f, randomVol)); // 限制在 0~2.0
			alSourcef(source, AL_GAIN, randomVol);

			// 音调随机 +-20%，1.0 为原始音调
			float pitchVariation = 0.2f;
			float randomPitch = 1.0f + pitchVariation * ((rand() % 1000) / 500.0f - 1.0f);
			randomPitch = max(0.4f, min(1.6f, randomPitch)); // 限制在 0.4~1.6
			alSourcef(source, AL_PITCH, randomPitch);

			alSourcePlay(source);
		};

		if (GetKeyClick(1, false, GK_HOOK))
			PlayWithVariation(CurrentMouseSource["1"]);
		if (GetKeyClick(2, false, GK_HOOK))
			PlayWithVariation(CurrentMouseSource["2"]);

		if (GetKeyUp(1, false, GK_HOOK))
			PlayWithVariation(CurrentMouseSource["01"]);
		if (GetKeyUp(2, false, GK_HOOK))
			PlayWithVariation(CurrentMouseSource["02"]);

		Sleep(config.MouseSoundSleep);
	}
	return 0;
}

LRESULT CALLBACK KeybdSoundHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		if ((config.UseKeybdSound || config.UseMouseSound) && SoundIsOpen)
		{
			switch (wParam)
			{
				case DBT_DEVICEREMOVECOMPLETE:
				{
					PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
					if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
					{
						PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
						if (IsEqualGUID(pDevInf->dbcc_classguid, GUID_DEVINTERFACE_VOLUME) && wParam == DBT_DEVICEARRIVAL)
						{
							thread th([]() {
								PrintLog("[Sound] 默认音频设备被切换，正在重新打开音频设备", PL_INFO);
								RenderTip(config.UseChinese ? u8"正在切换音频设备" : "Switching audio device", 3.0f, PT_INFO);
								SoundIsOpen = false;
								ShutdownOpenAL();
								Sleep(1000);
								InitOpenAL();
								SoundIsOpen = true;
								});
							th.detach();
						}
					}
					break;
				}
			}
			if (config.UseKeybdSound)
			{
				if (g_pALContext && alcGetCurrentContext() != g_pALContext)
					alcMakeContextCurrent(g_pALContext);

				KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
				int vk = pKeyBoard->vkCode;
				if (vk < 0 || vk >= 256)
					return CallNextHookEx(NULL, nCode, wParam, lParam);

				static bool KeyPhysicalState[256] = { false };

				if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
				{
					// 只有当前物理状态为 false（之前确实没有按下）时，才触发声音
					if (!KeyPhysicalState[vk])
					{
						KeyPhysicalState[vk] = true;// 标记为已按下
						if (CurrentKeybdSource[vk] > 0)
							alSourcePlay(CurrentKeybdSource[vk]);
					}
					// 如果KeyPhysicalState[vk]已经是true，直接忽略（无论系统 flags 是什么）
				}
				else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
					KeyPhysicalState[vk] = false;
			}
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}