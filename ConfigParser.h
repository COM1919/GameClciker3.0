#pragma once
#include "Log.h"
#include "HelperCode.h"

//======================= 内存 INI（快速读写） ===========================
// Win32 的 profile API 每次调用都会重新打开并解析整个文件，SaveConfig/LoadConfig
// 要做几百次这种调用，这正是切换预设卡顿的原因。这个轻量存储
// 只解析一次、写入一次，并在后台线程写回序列化文本。
struct IniMemData
{
	vector<string> SecOrder;
	map<string, vector<string>> KeyOrder;
	map<string, map<string, string>> Values;

	void Clear() { SecOrder.clear(); KeyOrder.clear(); Values.clear(); }

	void Set(const string& sec, const string& key, const string& val)
	{
		if (Values.find(sec) == Values.end())
			SecOrder.push_back(sec);
		map<string, string>& kv = Values[sec];
		if (kv.find(key) == kv.end())
			KeyOrder[sec].push_back(key);
		kv[key] = val;
	}

	string Get(const string& sec, const string& key, const string& def) const
	{
		auto s = Values.find(sec);
		if (s == Values.end()) return def;
		auto k = s->second.find(key);
		return (k == s->second.end()) ? def : k->second;
	}

	bool Load(const string& path)
	{
		Clear();
		ifstream f(path, ios::binary);
		if (!f.is_open()) return false;
		string line, cur;
		while (getline(f, line))
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			size_t p = line.find_first_not_of(" \t");
			if (p == string::npos) continue;
			if (line[p] == ';' || line[p] == '#') continue;
			if (line[p] == '[')
			{
				size_t e = line.find(']', p);
				if (e != string::npos) cur = line.substr(p + 1, e - p - 1);
				continue;
			}
			size_t eq = line.find('=', p);
			if (eq == string::npos) continue;
			string key = line.substr(p, eq - p);
			string val = line.substr(eq + 1);
			while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
			size_t vs = val.find_first_not_of(" \t");
			size_t ve = val.find_last_not_of(" \t");
			val = (vs == string::npos) ? string() : val.substr(vs, ve - vs + 1);
			Set(cur, key, val);
		}
		return true;
	}

	string Serialize() const
	{
		string out;
		for (const string& sec : SecOrder)
		{
			out += "[" + sec + "]\n";
			auto ko = KeyOrder.find(sec);
			if (ko != KeyOrder.end())
			{
				auto sv = Values.find(sec);
				for (const string& key : ko->second)
					out += key + "=" + sv->second.at(key) + "\n";
			}
			out += "\n";
		}
		return out;
	}
};

inline thread_local IniMemData* g_pIniSave = nullptr;
inline thread_local IniMemData* g_pIniLoad = nullptr;

void SaveConfig(const string& file = ConfigFile);
void LoadConfig(const string& file = ConfigFile);
void SavePresetConfig(const string& file);
void LoadPresetConfig(const string& file);
void SaveGlobalConfig();
void LoadGlobalConfig();
int CreateFile(const string& fileName);
void ClearFile(const string& fileName);
bool DeleteDirectory(const string& path);
int DelFile(const string& file);
inline vector<string> ScanPresets();
inline bool DeletePreset(const string& name);
inline bool AddPreset(const string& name);
inline void SwitchPreset(const string& from, const string& to);
void SavePresetConfig(const string& file);
void SaveConfig(const string& file);
void LoadPresetConfig(const string& file);
void LoadGlobalConfig();
void LoadConfig(const string& file);
inline string GetPresetFile(const string& name);
inline void IniWrite(const char* sec, const char* key, const char* val);
inline DWORD IniRead(const char* sec, const char* key, const char* def, char* buf, UINT sz);
inline UINT IniReadInt(const char* sec, const char* key, int def);
inline void IniFlushFile(const string& file, const string& text);
void SaveInstalledScriptList();

void ClearFile(const string& fileName)
{
	ofstream file(fileName, ios::trunc);
	file.clear();
	file.close();
	PrintLog("[File] 已清空文件：" + fileName, PL_INFO);
}

int DelFile(const string& file)
{
	// 检查文件是否存在
	DWORD attr = GetFileAttributesA(file.c_str());
	if (attr == INVALID_FILE_ATTRIBUTES)
		return 0;   // 不存在视为成功

	// 清除只读、系统、隐藏等属性
	SetFileAttributesA(file.c_str(), FILE_ATTRIBUTE_NORMAL);

	if (DeleteFileA(file.c_str()))
	{
		PrintLog("[File] 已删除文件：" + file, PL_INFO);
		return 0;
	}
	else
	{
		int err = GetLastError();
		PrintLog("[Error] 删除文件失败：" + file + "，错误码：" + to_string(err), PL_ERROR);
		return err;
	}
}

bool DeleteDirectory(const string& path)
{
	DWORD attr = GetFileAttributesA(path.c_str());

	// 路径不存在 -> 视为成功（与 fs::remove_all 行为一致）
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		PrintLog("[File] 已删除文件夹：" + path, PL_INFO);
		return true;
	}

	// 若路径指向文件，转交给 DelFile 处理
	if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
		return DelFile(path) == 0;

	vector<string> dirs;          // 所有待删除的目录（含根目录）
	vector<string> stack;         // 工作栈
	stack.push_back(path);

	while (!stack.empty())
	{
		string cur = stack.back();
		stack.pop_back();

		// 记录当前目录（最后统一删除）
		dirs.push_back(cur);

		// 构造搜索模式
		string searchPattern = cur + "\\*";
		WIN32_FIND_DATAA fd;
		HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &fd);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			DWORD err = GetLastError();
			if (err == ERROR_FILE_NOT_FOUND)   // 空目录，无文件需处理
				continue;
			else
			{
				PrintLog("[Error] 无法访问目录：" + cur + "，错误码：" + to_string(err), PL_ERROR);
				return false;
			}
		}

		do
		{
			// 跳过 "." 和 ".."
			if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
				continue;

			string fullPath = cur + "\\" + fd.cFileName;

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// 子目录压入栈，稍后遍历
				stack.push_back(fullPath);
			}
			else
			{
				// 删除文件：清除属性后删除
				SetFileAttributesA(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
				if (!DeleteFileA(fullPath.c_str()))
				{
					DWORD err = GetLastError();
					PrintLog("[Error] 删除文件失败：" + fullPath + "，错误码：" + to_string(err), PL_ERROR);
					FindClose(hFind);
					return false;
				}
			}
		} while (FindNextFileA(hFind, &fd));

		FindClose(hFind);
	}

	sort(dirs.begin(), dirs.end(),
		[](const string& a, const string& b)
		{
			return a.length() > b.length();
		});

	for (const string& d : dirs)
	{
		SetFileAttributesA(d.c_str(), FILE_ATTRIBUTE_NORMAL);
		if (!RemoveDirectoryA(d.c_str()))
		{
			DWORD err = GetLastError();
			PrintLog("[Error] 删除目录失败：" + d + "，错误码：" + to_string(err), PL_ERROR);
			return false;
		}
	}

	PrintLog("[File] 已删除文件夹：" + path, PL_INFO);
	return true;
}

int CreateFile(const string& fileName)
{
	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		PrintLog("[File] 已创建文件：" + fileName, PL_INFO);
		CloseHandle(hFile);
		return 0;
	}
	CloseHandle(hFile);
	PrintLog("[File] 创建文件失败：" + fileName, PL_INFO);
	return -1;
}

// 序列化与文件写入都在调用线程完成（由调用方按需放到后台线程），避免在持锁时起线程。
inline void IniFlushFile(const string& file, const string& text)
{
	ofstream f(file, ios::binary | ios::trunc);
	if (f.is_open())
	{
		f.write(text.data(), (streamsize)text.size());
		f.close();
	}
}

// 与 Win32 profile API 参数列表完全一致的替代函数。
inline void IniWrite(const char* sec, const char* key, const char* val)
{
	if (g_pIniSave)
		g_pIniSave->Set(sec ? sec : "", key ? key : "", val ? val : "");
}

inline DWORD IniRead(const char* sec, const char* key, const char* def, char* buf, UINT sz)
{
	string v = def ? def : "";
	if (g_pIniLoad) v = g_pIniLoad->Get(sec ? sec : "", key ? key : "", v);
	if (buf && sz)
	{
		size_t n = v.size();
		if (n >= (size_t)sz) n = (size_t)sz - 1;
		for (size_t i = 0; i < n; ++i) buf[i] = v[i];
		buf[n] = '\0';
	}
	return (DWORD)v.size();
}

inline UINT IniReadInt(const char* sec, const char* key, int def)
{
	string v = to_string(def);
	if (g_pIniLoad) v = g_pIniLoad->Get(sec ? sec : "", key ? key : "", v);
	return (UINT)atoi(v.c_str());
}

void SavePresetConfig(const string& file)
{
	IniMemData ini;
	g_pIniSave = &ini;
	PrintLog("[Config] 写入配置文件：" + ConfigFile, PL_INFO);
	const char* CIniPos = file.c_str();
	if (_access(CIniPos, 0) != 0)
		CreateFile(CIniPos);

    // Config Version
    IniWrite("Config", "Version", to_string(ConfigVersion).c_str());

	// BasicSettings section
	IniWrite("BasicSettings", "leftKeyShortcut", to_string(config.LKEY).c_str());
	IniWrite("BasicSettings", "rightKeyShortcut", to_string(config.RKEY).c_str());
	IniWrite("BasicSettings", "leftClickerCps", to_string(config.LCPS).c_str());
	IniWrite("BasicSettings", "rightClickerCps", to_string(config.RCPS).c_str());
	IniWrite("BasicSettings", "mainClickSleep", to_string(config.MainClickSleep).c_str());
	IniWrite("BasicSettings", "rightMultipleClickValue", to_string(config.RMultipleClickValue).c_str());
	IniWrite("BasicSettings", "leftMultipleClickValue", to_string(config.LMultipleClickValue).c_str());
	IniWrite("BasicSettings", "rightEmptyRateValue", to_string(config.REmptyRateValue).c_str());
	IniWrite("BasicSettings", "leftEmptyRateValue", to_string(config.LEmptyRateValue).c_str());
	IniWrite("BasicSettings", "rightClickTime", to_string(config.RCTime).c_str());
	IniWrite("BasicSettings", "rightClickSleep", to_string(config.RCSleep).c_str());
	IniWrite("BasicSettings", "leftClickTime", to_string(config.LCTime).c_str());
	IniWrite("BasicSettings", "leftClickSleep", to_string(config.LCSleep).c_str());
	IniWrite("BasicSettings", "rightClickerMode", to_string(config.RClickerMode).c_str());
	IniWrite("BasicSettings", "leftClickerMode", to_string(config.LClickerMode).c_str());
	IniWrite("BasicSettings", "rightFluctuationCps", to_string(config.RFluctuationCPS).c_str());
	IniWrite("BasicSettings", "leftFluctuationCps", to_string(config.LFluctuationCPS).c_str());
	IniWrite("BasicSettings", "rightPressDuration", to_string(config.RPressDuration).c_str());
	IniWrite("BasicSettings", "leftPressDuration", to_string(config.LPressDuration).c_str());
	IniWrite("BasicSettings", "enableRightClickerKey", to_string(config.EnableRightClickerKey).c_str());
	IniWrite("BasicSettings", "enableLeftClickerKey", to_string(config.EnableLeftClickerKey).c_str());
	IniWrite("BasicSettings", "chatBarKey", to_string(config.ChatBarKey).c_str());
	IniWrite("BasicSettings", "bagBarKey", to_string(config.BagBarKey).c_str());
	IniWrite("BasicSettings", "mouseShakeSpeed", to_string(config.MouseShakeSpeed).c_str());
	IniWrite("BasicSettings", "mouseShakeRect", to_string(config.MouseShakeRect).c_str());
	IniWrite("BasicSettings", "quickSwitchTotal", to_string(config.QuickSwitchTotal).c_str());
	IniWrite("BasicSettings", "assistModeCpsThreshold", to_string(config.AssistModeCPSThreshold).c_str());
	IniWrite("BasicSettings", "useOverheatThrottle", to_string(config.UseOverheatThrottle).c_str());
	IniWrite("BasicSettings", "overheatMinCps", to_string(config.Overheat_MinCPS).c_str());
	IniWrite("BasicSettings", "overheatReachTime", to_string(config.Overheat_Time).c_str());
	IniWrite("BasicSettings", "overheatThrottleTime", to_string(config.Overheat_ThrottleTime).c_str());
	IniWrite("BasicSettings", "selfDestructionKey", to_string(config.SelfDestructionKey).c_str());
	IniWrite("BasicSettings", "leftFluctuationRandMode", to_string(config.UseLFluctuationRandMode).c_str());
	IniWrite("BasicSettings", "rightFluctuationRandMode", to_string(config.UseRFluctuationRandMode).c_str());
	IniWrite("BasicSettings", "rightStartClickerDelay", to_string(config.RStartClickerDelay).c_str());
	IniWrite("BasicSettings", "leftStartClickerDelay", to_string(config.LStartClickerDelay).c_str());
	IniWrite("BasicSettings", "setGameWindowKey", to_string(config.SetGameWindowKey).c_str());
	IniWrite("BasicSettings", "hideClickerKey", to_string(config.HideClickerKey).c_str());
	IniWrite("BasicSettings", "overheatTime", to_string(config.OverheatTime).c_str());
	IniWrite("BasicSettings", "overheatCooldownTime", to_string(config.OverheatCooldownTime).c_str());
	IniWrite("BasicSettings", "overheatSoundType", to_string(config.OverheatSoundType).c_str());
	IniWrite("BasicSettings", "simulateClickMode", to_string(config.SimulateClickMode).c_str());
	IniWrite("BasicSettings", "clickerPriorityMode", to_string(config.ClickerPriorityMode).c_str());

	// CPSWaves section
	int count = 0;
	for (int i = 0; i < 2; i++)
	{
		const char* name = (i == 0) ? "leftWave" : "rightWave";
		const vector<WaveParam>& waves = (i == 0) ? config.CPSWaves.left : config.CPSWaves.right;
		for (const WaveParam w : waves)
		{
			char data[256];
			sprintf_s(data, "%s,%.5f,%.5f,%.5f", w.enabled ? "TRUE" : "FALSE", w.amplitude, w.frequency, w.phase);
			IniWrite("CPSWaves", (name + to_string(count)).c_str(), data);
			count++;
		}
	}

	// MouseSoundSettings section
	IniWrite("MouseSoundSettings", "mouseSoundVolume", to_string(config.MouseSoundVolume).c_str());
	IniWrite("MouseSoundSettings", "keybdSoundVolume", to_string(config.KeybdSoundVolume).c_str());
	IniWrite("MouseSoundSettings", "noiseVolume", to_string(config.NoiseVolume).c_str());
	IniWrite("MouseSoundSettings", "mouseSoundType", config.MouseSoundType.c_str());
	IniWrite("MouseSoundSettings", "mouseSoundSleep", to_string(config.MouseSoundSleep).c_str());

	// FeatureSwitches section - bool values
	IniWrite("FeatureSwitches", "useRightEmptyRate", config.UseREmptyRate ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftEmptyRate", config.UseLEmptyRate ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightFluctuation", config.UseRFluctuation ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftFluctuation", config.UseLFluctuation ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftClickTime", config.UseLClickTime ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightClickTime", config.UseRClickTime ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useGuiDetection", config.UseGUIDetection ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftPressInterval", config.UseLClickPressTime ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightPressInterval", config.UseRClickPressTime ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightAutoPressDuration", config.UseRAutoPressDuration ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftAutoPressDuration", config.UseLAutoPressDuration ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useStartWithWindows", config.UseStartWithWindows ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "enableRightClicker", config.EnableRightClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "enableLeftClicker", config.EnableLeftClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useBroadcasterMode", config.UseBroadcasterMode ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useAutoGuiDetect", config.UseAutoGuiDetect ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useMouseSound", config.UseMouseSound ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useKeybdSound", config.UseKeybdSound ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightOnlyMoveClicker", config.UseROnlyMoveClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftOnlyMoveClicker", config.UseLOnlyMoveClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useDisableMoveOnStopMove", config.UseDisableMoveOnStopMove ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useMouseShake", config.UseMouseShake ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useMouseShakeRawInput", config.UseMouseShakeRawInput ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useMsPreventViewSliding", config.UseMSPreventViewSliding ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useMsSmoothSpeed", config.UseMSSmoothSpeed ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useItembarUseMemSearch", config.UseItembarUseMemSearch ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useChinese", config.UseChinese ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useDriverProtect", config.UseDriverProtect ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useHiddenProcess", config.UseHiddenProcess ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useSuspendNetease", config.UseSuspendNetease ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "allowSetLeftClicker", config.AllowSetLClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "allowSetRightClicker", config.AllowSetRClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useEnableGameGui", config.UseEnableGameGui ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightOnlyJumpClicker", config.UseROnlyJumpClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftOnlyJumpClicker", config.UseLOnlyJumpClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRestTimeOnStopClicker", config.UseRestTimeOnStopClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useBrutalMode", config.UseBrutalMode ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLClickerTrigInter", config.UseLClickerTrigInter ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRClickerTrigInter", config.UseRClickerTrigInter ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useOldMethodToTrigInter", config.UseOldMethodToTrigInter ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightStartClickerDelay", config.UseRStartClickerDelay ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftStartClickerDelay", config.UseLStartClickerDelay ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useFilterRightSingleClick", config.UseFilterRSingleClick ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useFilterLeftSingleClick", config.UseFilterLSingleClick ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "usePerformanceMode", config.UsePerformanceMode ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useRightCpsChangeTrend", config.UseRCPSChangeTrend ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useLeftCpsChangeTrend", config.UseLCPSChangeTrend ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useOverheatProtection", config.UseOverheatProtec ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "bedrockEditionAdaptation", config.UseBEAdaptation ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "inGameOnlyClicker", config.UseInGameOnlyClicker ? "TRUE" : "FALSE");
	IniWrite("FeatureSwitches", "useItembarSwitch", config.UseItembarSwitch ? "TRUE" : "FALSE");

	// QuickSwitchSettings section - arrays
	string rQuickSwitchStr, lQuickSwitchStr, quickSwitchKeyStr;
	for (int i = 0; i < MAX_QUICK_SWITCH; i++)
	{
		rQuickSwitchStr += to_string(config.RQuickSwitch[i]);
		lQuickSwitchStr += to_string(config.LQuickSwitch[i]);
		quickSwitchKeyStr += to_string(config.QuickSwitchKey[i]);
		if (i < MAX_QUICK_SWITCH - 1)
		{
			rQuickSwitchStr += ",";
			lQuickSwitchStr += ",";
			quickSwitchKeyStr += ",";
		}
	}
	IniWrite("QuickSwitchSettings", "rightQuickSwitch", rQuickSwitchStr.c_str());
	IniWrite("QuickSwitchSettings", "leftQuickSwitch", lQuickSwitchStr.c_str());
	IniWrite("QuickSwitchSettings", "quickSwitchKey", quickSwitchKeyStr.c_str());

	// ItemBarSettings section - arrays and LRPair arrays
	string itemEnableClickerKeyStr;
	for (int i = 0; i < 10; i++)
	{
		itemEnableClickerKeyStr += to_string(config.ItemBarKey[i]);
		if (i < 9) itemEnableClickerKeyStr += ",";
	}
	IniWrite("ItemBarSettings", "itemEnableClickerKey", itemEnableClickerKeyStr.c_str());

	string itemBarPresetCpsLeftStr, itemBarPresetCpsRightStr;
	string itemBarEnablePresetCpsLeftStr, itemBarEnablePresetCpsRightStr;
	string itemEnableClickerLeftStr, itemEnableClickerRightStr;
	for (int i = 0; i < 10; i++)
	{
		itemBarPresetCpsLeftStr += to_string(config.ItemBarPresetCPS.left[i]);
		itemBarPresetCpsRightStr += to_string(config.ItemBarPresetCPS.right[i]);
		itemBarEnablePresetCpsLeftStr += config.ItemBarEnablePresetCPS.left[i] ? "TRUE" : "FALSE";
		itemBarEnablePresetCpsRightStr += config.ItemBarEnablePresetCPS.right[i] ? "TRUE" : "FALSE";
		itemEnableClickerLeftStr += config.ItemEnableClicker.left[i] ? "TRUE" : "FALSE";
		itemEnableClickerRightStr += config.ItemEnableClicker.right[i] ? "TRUE" : "FALSE";
		if (i < 9)
		{
			itemBarPresetCpsLeftStr += ",";
			itemBarPresetCpsRightStr += ",";
			itemBarEnablePresetCpsLeftStr += ",";
			itemBarEnablePresetCpsRightStr += ",";
			itemEnableClickerLeftStr += ",";
			itemEnableClickerRightStr += ",";
		}
	}
	IniWrite("ItemBarSettings", "itemBarPresetCpsLeft", itemBarPresetCpsLeftStr.c_str());
	IniWrite("ItemBarSettings", "itemBarPresetCpsRight", itemBarPresetCpsRightStr.c_str());
	IniWrite("ItemBarSettings", "itemBarEnablePresetCpsLeft", itemBarEnablePresetCpsLeftStr.c_str());
	IniWrite("ItemBarSettings", "itemBarEnablePresetCpsRight", itemBarEnablePresetCpsRightStr.c_str());
	IniWrite("ItemBarSettings", "itemEnableClickerLeft", itemEnableClickerLeftStr.c_str());
	IniWrite("ItemBarSettings", "itemEnableClickerRight", itemEnableClickerRightStr.c_str());

	// RandomSettings section
	IniWrite("RandomSettings", "mouseShakeRandomDist", to_string(config.MouseShake_RandomDist).c_str());
	IniWrite("RandomSettings", "emptyRateRandomDist", to_string(config.EmptyRate_RandomDist).c_str());
	IniWrite("RandomSettings", "emptyRateRandomMode", to_string(config.EmptyRate_RandomMode).c_str());

	// Vector保存
	string rCpsTrendStr, lCpsTrendStr;
	for (size_t i = 0; i < config.RCPSChangeTrendList.size(); i++)
	{
		rCpsTrendStr += to_string(config.RCPSChangeTrendList[i]);
		if (i < config.RCPSChangeTrendList.size() - 1) rCpsTrendStr += ",";
	}
	for (size_t i = 0; i < config.LCPSChangeTrendList.size(); i++)
	{
		lCpsTrendStr += to_string(config.LCPSChangeTrendList[i]);
		if (i < config.LCPSChangeTrendList.size() - 1) lCpsTrendStr += ",";
	}
	IniWrite("RandomSettings", "rightCpsChangeTrendList", rCpsTrendStr.c_str());
	IniWrite("RandomSettings", "leftCpsChangeTrendList", lCpsTrendStr.c_str());

	g_pIniSave = nullptr;
	IniFlushFile(CIniPos, ini.Serialize());
}

void SaveInstalledScriptList()
{
	if (_access(ScriptInstallList.c_str(), 0) != 0)
		CreateFile(ScriptInstallList);
	fstream ScriptFile(ScriptInstallList, ios::out | ios::trunc);
	if (!ScriptFile.is_open())
	{
		PrintLog("[Config] 打开已安装脚本配置失败：" + ScriptInstallList, PL_ERROR);
		return;
	}
	// 先在锁内取出快照，避免边写文件边持锁
	vector<pair<string, int>> list;
	{
		shared_lock read_lock(ScriptData::InstalledMtx);
		for (const auto& script : ScriptData::Installed)
		{
			int key = 0;
			{
				shared_lock stateLock(ScriptData::StatesMtx);
				auto it = ScriptData::States.find(script.file);
				if (it != ScriptData::States.end())
					key = it->second.key;
			}
			list.push_back({ script.name, key });
		}
	}
	for (const auto& item : list)
		ScriptFile << item.first << ':' << item.second << endl;
	ScriptFile.close();
	PrintLog("[Config] 已保存 " + to_string(list.size()) + " 个已安装脚本", PL_INFO);
}

void SaveGlobalConfig()
{
	IniMemData ini;
	g_pIniSave = &ini;
	const char* CIniPos = CustomConfigFile.c_str();
	if (_access(CIniPos, 0) != 0)
		CreateFile(CIniPos);

	if (IsNeedReshowAgreement == false) // 不同意就不写入
		IniWrite("Config", "AgreementVersion", to_string(AgreementVersion).c_str());

	//-----------------------统计信息-----------------------
	{
		// 保存前先把本会话已使用时间计入总时长
		FlushUsageDuration();

		HKEY hKey = NULL;
		const LSTATUS StatsCreateRet = RegCreateKeyEx(HKEY_CURRENT_USER, StatsRegPath.c_str(), 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
		if (StatsCreateRet != ERROR_SUCCESS)
		{
			// 注册表不可用时只记录日志，不打断配置保存
			PrintLog("[Stats] 创建注册表统计项失败，错误码：" + to_string(StatsCreateRet), PL_ERROR);
		}
		else
		{
			// 写入注册表统计项，失败只记录日志
			auto WriteStat = [hKey](const char* name, DWORD type, const void* data, DWORD size)
			{
				const LSTATUS ret = RegSetValueEx(hKey, name, 0, type, (const BYTE*)data, size);
				if (ret != ERROR_SUCCESS)
					PrintLog(string("[Stats] 写入注册表统计项 ") + name + " 失败，错误码：" + to_string(ret), PL_ERROR);
			};

			ULONGLONG val = app_status.ClickerClickCount.left;
			WriteStat("0", REG_QWORD, &val, sizeof(val));

			val = app_status.ClickerClickCount.right;
			WriteStat("1", REG_QWORD, &val, sizeof(val));

			val = app_status.MouseClickCount.left;
			WriteStat("2", REG_QWORD, &val, sizeof(val));

			val = app_status.MouseClickCount.right;
			WriteStat("3", REG_QWORD, &val, sizeof(val));

			unsigned int uval = app_status.StartCount;
			WriteStat("4", REG_DWORD, &uval, sizeof(uval));

			// UsageDuration -> 转为字符串存储
			stringstream ss;
			ss << fixed << setprecision(18) << (long double)app_status.UsageDuration;
			string str = ss.str();
			WriteStat("5", REG_SZ, str.c_str(), (DWORD)(str.size() + 1));

			RegCloseKey(hKey);
		}
	}

	//--------------------个性化设置------------------------------
	CIniPos = CustomConfigFile.c_str();
	if (_access(CIniPos, 0) != 0)
		CreateFile(CIniPos);
	IniWrite("个性化", "BgOffsetX", to_string(BgOffsetX).c_str());
	IniWrite("个性化", "BgOffsetY", to_string(BgOffsetY).c_str());
	IniWrite("个性化", "BgScale", to_string(BgScale).c_str());
	IniWrite("个性化", "BgLightness", to_string(BgLightness).c_str());
	IniWrite("个性化", "GuiItemAlpha", to_string(GuiItemAlpha).c_str());
	IniWrite("个性化", "BgAutoResize", (BgAutoResize) ? "TRUE" : "FALSE");

	g_pIniSave = nullptr;
	IniFlushFile(CustomConfigFile, ini.Serialize());
}

void SaveConfig(const string& file)
{
	SavePresetConfig(file);
	SaveGlobalConfig();
}

void LoadPresetConfig(const string& file)
{
	IniMemData ini;
	ini.Load(file);
	g_pIniLoad = &ini;
	PrintLog("[Config] 读取配置文件：" + ConfigFile, PL_INFO);
	const char* CIniPos = file.c_str();
	char* buffer = new char[MAX_BUF];
	string st;

	//配置文件版本号
	IniRead("Config", "Version", "-1", buffer, MAX_BUF);
	const int ver = stoi(buffer);
	if (ver != ConfigVersion)
	{
		DelFile(file);
		CreateFile(file);
		SaveConfig(file);
	}

	if (_access(CIniPos, 0) == 0 && ver == ConfigVersion)
	{
		// BasicSettings section
		config.LKEY                     = IniReadInt("BasicSettings", "leftKeyShortcut", config.LKEY);
		config.RKEY                     = IniReadInt("BasicSettings", "rightKeyShortcut", config.RKEY);
		config.LCPS                     = IniReadInt("BasicSettings", "leftClickerCps", config.LCPS);
		config.RCPS                     = IniReadInt("BasicSettings", "rightClickerCps", config.RCPS);
		config.MainClickSleep           = IniReadInt("BasicSettings", "mainClickSleep", config.MainClickSleep);
		config.RMultipleClickValue      = IniReadInt("BasicSettings", "rightMultipleClickValue", config.RMultipleClickValue);
		config.LMultipleClickValue      = IniReadInt("BasicSettings", "leftMultipleClickValue", config.LMultipleClickValue);
		config.REmptyRateValue          = IniReadInt("BasicSettings", "rightEmptyRateValue", config.REmptyRateValue);
		config.LEmptyRateValue          = IniReadInt("BasicSettings", "leftEmptyRateValue", config.LEmptyRateValue);
		config.RCTime                   = IniReadInt("BasicSettings", "rightClickTime", config.RCTime);
		config.RCSleep                  = IniReadInt("BasicSettings", "rightClickSleep", config.RCSleep);
		config.LCTime                   = IniReadInt("BasicSettings", "leftClickTime", config.LCTime);
		config.LCSleep                  = IniReadInt("BasicSettings", "leftClickSleep", config.LCSleep);
		config.RClickerMode             = IniReadInt("BasicSettings", "rightClickerMode", config.RClickerMode);
		config.LClickerMode             = IniReadInt("BasicSettings", "leftClickerMode", config.LClickerMode);
		config.RFluctuationCPS          = IniReadInt("BasicSettings", "rightFluctuationCps", config.RFluctuationCPS);
		config.LFluctuationCPS          = IniReadInt("BasicSettings", "leftFluctuationCps", config.LFluctuationCPS);
		config.RPressDuration           = IniReadInt("BasicSettings", "rightPressDuration", config.RPressDuration);
		config.LPressDuration           = IniReadInt("BasicSettings", "leftPressDuration", config.LPressDuration);
		config.EnableRightClickerKey    = IniReadInt("BasicSettings", "enableRightClickerKey", config.EnableRightClickerKey);
		config.EnableLeftClickerKey     = IniReadInt("BasicSettings", "enableLeftClickerKey", config.EnableLeftClickerKey);
		config.ChatBarKey               = IniReadInt("BasicSettings", "chatBarKey", config.ChatBarKey);
		config.BagBarKey                = IniReadInt("BasicSettings", "bagBarKey", config.BagBarKey);
		config.MouseShakeSpeed          = IniReadInt("BasicSettings", "mouseShakeSpeed", config.MouseShakeSpeed);
		config.MouseShakeRect           = IniReadInt("BasicSettings", "mouseShakeRect", config.MouseShakeRect);
		config.QuickSwitchTotal         = IniReadInt("BasicSettings", "quickSwitchTotal", config.QuickSwitchTotal);
		config.AssistModeCPSThreshold   = IniReadInt("BasicSettings", "assistModeCpsThreshold", config.AssistModeCPSThreshold);
		config.UseOverheatThrottle      = IniReadInt("BasicSettings", "useOverheatThrottle", config.UseOverheatThrottle) != 0;
		config.Overheat_MinCPS          = IniReadInt("BasicSettings", "overheatMinCps", config.Overheat_MinCPS);
		{
			// 注意：键名用 overheatReachTime，避免与既有"过热保护"功能的 overheatTime 键冲突
			IniRead("BasicSettings", "overheatReachTime", to_string(config.Overheat_Time).c_str(), buffer, MAX_BUF);
			config.Overheat_Time = stof(buffer);
			IniRead("BasicSettings", "overheatThrottleTime", to_string(config.Overheat_ThrottleTime).c_str(), buffer, MAX_BUF);
			config.Overheat_ThrottleTime = stof(buffer);
		}
		config.SelfDestructionKey       = IniReadInt("BasicSettings", "selfDestructionKey", config.SelfDestructionKey);
		config.UseLFluctuationRandMode  = IniReadInt("BasicSettings", "leftFluctuationRandMode", config.UseLFluctuationRandMode);
		config.UseRFluctuationRandMode  = IniReadInt("BasicSettings", "rightFluctuationRandMode", config.UseRFluctuationRandMode);
		config.RStartClickerDelay       = IniReadInt("BasicSettings", "rightStartClickerDelay", config.RStartClickerDelay);
		config.LStartClickerDelay       = IniReadInt("BasicSettings", "leftStartClickerDelay", config.LStartClickerDelay);
		config.SetGameWindowKey         = IniReadInt("BasicSettings", "setGameWindowKey", config.SetGameWindowKey);
		config.HideClickerKey           = IniReadInt("BasicSettings", "hideClickerKey", config.HideClickerKey);

		// 注意：OverheatTime 和 OHPTimerTimeLossSpeed 是浮点数，不能直接用 GetPrivateProfileInt
		IniRead("BasicSettings", "overheatTime", to_string(config.OverheatTime).c_str(), buffer, MAX_BUF);
		config.OverheatTime = stof(buffer);
		IniRead("BasicSettings", "overheatCooldownTime", to_string(config.OverheatCooldownTime).c_str(), buffer, MAX_BUF);
		config.OverheatCooldownTime = stof(buffer);
		IniRead("BasicSettings", "overheatSoundType", to_string(config.OverheatSoundType).c_str(), buffer, MAX_BUF);
		config.OverheatSoundType = stof(buffer);

		config.SimulateClickMode = IniReadInt("BasicSettings", "simulateClickMode", config.SimulateClickMode);
		config.HideClickerKey = IniReadInt("BasicSettings", "hideClickerKey", config.HideClickerKey);
		config.ClickerPriorityMode = IniReadInt("BasicSettings", "clickerPriorityMode", config.ClickerPriorityMode);

	
	//==================================================CPSWaves section==================================================
		for (int i = 0; i < 2; i++)
		{
			const char* name = (i == 0) ? "leftWave" : "rightWave";

			int count = 0;
			while (true)
			{
				DWORD len = IniRead("CPSWaves", (name + to_string(count)).c_str(), "", buffer, MAX_BUF);
				if (len <= 0)
					break;
				vector<string> tokens;
				SplitString(buffer, ",", tokens);
				try
				{
					WaveParam w;
					w.enabled = (tokens[0] == "TRUE");
					w.amplitude = stof(tokens[1]);
					w.frequency = stof(tokens[2]);
					w.phase = stof(tokens[3]);
					if (i == 0)
						config.CPSWaves.left.push_back(w);
					else
						config.CPSWaves.right.push_back(w);
				}
				catch (...)
				{
					PrintLog("[Config] 尝试读取CPS Waves时stof失败，已跳过，序号：" + to_string(count), PL_ERROR);
				}
				count++;
				if (count >= 24)
				{
					PrintLog("[Config] 读取CPS Waves时数量超过24，已忽略后续读取。", PL_WARNING);
					break;
				}
			}
			if (count <= 0)
			{
				for (int i = 0; i < 8; i++)
				{
					WaveParam w1;
					w1.amplitude = RandGentor.Random(90000, 115000) / 100000.0f; // 0.9 - 1.15
					w1.frequency = RandGentor.Random(20000, 600000) / 100000.0f; // 0.2 - 6
					w1.phase = RandGentor.Random(-314159265, 314159265) / 100000000.f; // -pi ~ pi
					w1.enabled = true;
					config.CPSWaves.left.push_back(w1);

					WaveParam w2;
					w2.amplitude = RandGentor.Random(60000, 200000) / 100000.0f; // 0.6 - 2
					w2.frequency = RandGentor.Random(80000, 400000) / 100000.0f; // 0.8 - 4.0
					w2.phase = RandGentor.Random(-314159265, 314159265) / 100000000.f; // -pi ~ pi
					w2.enabled = true;
					config.CPSWaves.right.push_back(w2);
				}
			}
		}

		//==================================================MouseSoundSettings section==================================================
		config.MouseSoundVolume = IniReadInt("MouseSoundSettings", "mouseSoundVolume", config.MouseSoundVolume);
		config.KeybdSoundVolume = IniReadInt("MouseSoundSettings", "keybdSoundVolume", config.KeybdSoundVolume);
		config.NoiseVolume = IniReadInt("MouseSoundSettings", "noiseVolume", config.NoiseVolume);

		// MouseSoundType 是字符串，需要保留 GetPrivateProfileString
		IniRead("MouseSoundSettings", "rightMouseSoundType", config.MouseSoundType.c_str(), buffer, MAX_BUF);
		config.MouseSoundType = buffer;

		config.MouseSoundSleep = IniReadInt("MouseSoundSettings", "mouseSoundSleep", config.MouseSoundSleep);

		//==================================================FeatureSwitches section==================================================
		IniRead("FeatureSwitches", "useRightEmptyRate", config.UseREmptyRate ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseREmptyRate = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftEmptyRate", config.UseLEmptyRate ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLEmptyRate = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightFluctuation", config.UseRFluctuation ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRFluctuation = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftFluctuation", config.UseLFluctuation ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLFluctuation = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftClickTime", config.UseLClickTime ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLClickTime = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightClickTime", config.UseRClickTime ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRClickTime = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useGuiDetection", config.UseGUIDetection ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseGUIDetection = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftPressInterval", config.UseLClickPressTime ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLClickPressTime = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightPressInterval", config.UseRClickPressTime ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRClickPressTime = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightAutoPressDuration", config.UseRAutoPressDuration ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRAutoPressDuration = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftAutoPressDuration", config.UseLAutoPressDuration ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLAutoPressDuration = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useStartWithWindows", config.UseStartWithWindows ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseStartWithWindows = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "enableRightClicker", config.EnableRightClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.EnableRightClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "enableLeftClicker", config.EnableLeftClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.EnableLeftClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useBroadcasterMode", config.UseBroadcasterMode ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseBroadcasterMode = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useAutoGuiDetect", config.UseAutoGuiDetect ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseAutoGuiDetect = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useMouseSound", config.UseMouseSound ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseMouseSound = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useKeybdSound", config.UseKeybdSound ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseKeybdSound = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightOnlyMoveClicker", config.UseROnlyMoveClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseROnlyMoveClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftOnlyMoveClicker", config.UseLOnlyMoveClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLOnlyMoveClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useDisableMoveOnStopMove", config.UseDisableMoveOnStopMove ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseDisableMoveOnStopMove = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useMouseShake", config.UseMouseShake ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseMouseShake = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useMouseShakeRawInput", config.UseMouseShakeRawInput ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseMouseShakeRawInput = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useMsPreventViewSliding", config.UseMSPreventViewSliding ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseMSPreventViewSliding = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useMsSmoothSpeed", config.UseMSSmoothSpeed ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseMSSmoothSpeed = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useItembarUseMemSearch", config.UseItembarUseMemSearch ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseItembarUseMemSearch = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useChinese", config.UseChinese ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseChinese = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useDriverProtect", config.UseDriverProtect ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseDriverProtect = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useHiddenProcess", config.UseHiddenProcess ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseHiddenProcess = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useSuspendNetease", config.UseSuspendNetease ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseSuspendNetease = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "allowSetLeftClicker", config.AllowSetLClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.AllowSetLClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "allowSetRightClicker", config.AllowSetRClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.AllowSetRClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useEnableGameGui", config.UseEnableGameGui ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseEnableGameGui = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightOnlyJumpClicker", config.UseROnlyJumpClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseROnlyJumpClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftOnlyJumpClicker", config.UseLOnlyJumpClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLOnlyJumpClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRestTimeOnStopClicker", config.UseRestTimeOnStopClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRestTimeOnStopClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useBrutalMode", config.UseBrutalMode ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseBrutalMode = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLClickerTrigInter", config.UseLClickerTrigInter ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLClickerTrigInter = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRClickerTrigInter", config.UseRClickerTrigInter ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRClickerTrigInter = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useOldMethodToTrigInter", config.UseOldMethodToTrigInter ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseOldMethodToTrigInter = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightStartClickerDelay", config.UseRStartClickerDelay ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRStartClickerDelay = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftStartClickerDelay", config.UseLStartClickerDelay ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLStartClickerDelay = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useFilterRightSingleClick", config.UseFilterRSingleClick ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseFilterRSingleClick = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useFilterLeftSingleClick", config.UseFilterLSingleClick ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseFilterLSingleClick = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "usePerformanceMode", config.UsePerformanceMode ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UsePerformanceMode = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useRightCpsChangeTrend", config.UseRCPSChangeTrend ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseRCPSChangeTrend = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useLeftCpsChangeTrend", config.UseLCPSChangeTrend ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseLCPSChangeTrend = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useOverheatProtection", config.UseOverheatProtec ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseOverheatProtec = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "bedrockEditionAdaptation", config.UseBEAdaptation ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseBEAdaptation = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "inGameOnlyClicker", config.UseInGameOnlyClicker ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseInGameOnlyClicker = st.find("TRUE") != string::npos;
		IniRead("FeatureSwitches", "useItembarSwitch", config.UseItembarSwitch ? "TRUE" : "FALSE", buffer, MAX_BUF);
		st = buffer;
		config.UseItembarSwitch = st.find("TRUE") != string::npos;

        //==================================================QuickSwitchSettings==================================================
        IniRead("QuickSwitchSettings", "rightQuickSwitch", "", buffer, MAX_BUF);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < MAX_QUICK_SWITCH)
            {
                config.RQuickSwitch[i++] = stoi(token);
            }
        }

        IniRead("QuickSwitchSettings", "leftQuickSwitch", "", buffer, MAX_BUF);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < MAX_QUICK_SWITCH)
            {
                config.LQuickSwitch[i++] = stoi(token);
            }
        }

        IniRead("QuickSwitchSettings", "quickSwitchKey", "", buffer, MAX_BUF);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < MAX_QUICK_SWITCH)
                config.QuickSwitchKey[i++] = stoi(token);
        }

        //==================================================ItemBarSettings section==================================================
        IniRead("ItemBarSettings", "itemEnableClickerKey", "", buffer, MAX_BUF);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
				config.ItemBarKey[i++] = stoi(token);
		}

		//==================================================LRPair arrays==================================================
		IniRead("ItemBarSettings", "itemBarPresetCpsLeft", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			stringstream ss(buffer);
			string token;
			int i = 0;
			while (getline(ss, token, ',') && i < 10)
				config.ItemBarPresetCPS.left[i++] = stoi(token);
		}

		IniRead("ItemBarSettings", "itemBarPresetCpsRight", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			stringstream ss(buffer);
			string token;
			int i = 0;
			while (getline(ss, token, ',') && i < 10)
				config.ItemBarPresetCPS.right[i++] = stoi(token);
		}

		IniRead("ItemBarSettings", "itemBarEnablePresetCpsLeft", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			stringstream ss(buffer);
			string token;
			int i = 0;
			while (getline(ss, token, ',') && i < 10)
				config.ItemBarEnablePresetCPS.left[i++] = token.find("TRUE") != string::npos;
		}

		IniRead("ItemBarSettings", "itemBarEnablePresetCpsRight", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			stringstream ss(buffer);
			string token;
			int i = 0;
			while (getline(ss, token, ',') && i < 10)
				config.ItemBarEnablePresetCPS.right[i++] = token.find("TRUE") != string::npos;
		}

		IniRead("ItemBarSettings", "itemEnableClickerLeft", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			stringstream ss(buffer);
			string token;
			int i = 0;
			while (getline(ss, token, ',') && i < 10)
				config.ItemEnableClicker.left[i++] = token.find("TRUE") != string::npos;
		}

		IniRead("ItemBarSettings", "itemEnableClickerRight", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			stringstream ss(buffer);
			string token;
			int i = 0;
			while (getline(ss, token, ',') && i < 10)
				config.ItemEnableClicker.right[i++] = token.find("TRUE") != string::npos;
		}

		//==================================================RandomSettings section==================================================
		IniRead("RandomSettings", "mouseShakeRandomDist", to_string(config.MouseShake_RandomDist).c_str(), buffer, MAX_BUF);
		config.MouseShake_RandomDist = stoi(buffer);
		IniRead("RandomSettings", "emptyRateRandomDist", to_string(config.EmptyRate_RandomDist).c_str(), buffer, MAX_BUF);
		config.EmptyRate_RandomDist = stoi(buffer);
		IniRead("RandomSettings", "emptyRateRandomMode", to_string(config.EmptyRate_RandomMode).c_str(), buffer, MAX_BUF);
		config.EmptyRate_RandomMode = stoi(buffer);
		// 修复：移除 STIFF 模式后仅 0~2 有效；旧配置文件里的 1(STIFF)/2(FATIGUE)/3(BIONIC) 数值错位，越界时重置
		if (config.EmptyRate_RandomMode < 0 || config.EmptyRate_RandomMode >= 3)
			config.EmptyRate_RandomMode = RandomBoolMode_UNIFORM;

		//==================================================Vector读取==================================================
		IniRead("RandomSettings", "rightCpsChangeTrendList", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			config.RCPSChangeTrendList.clear();
			stringstream ss(buffer);
			string token;
			while (getline(ss, token, ','))
				config.RCPSChangeTrendList.push_back(stoi(token));
		}

		IniRead("RandomSettings", "leftCpsChangeTrendList", "", buffer, MAX_BUF);
		if (strlen(buffer) > 0)
		{
			config.LCPSChangeTrendList.clear();
			stringstream ss(buffer);
			string token;
			while (getline(ss, token, ','))
				config.LCPSChangeTrendList.push_back(stoi(token));
		}
	}
	g_pIniLoad = nullptr;
	delete[] buffer;
	buffer = nullptr;
}

void LoadGlobalConfig()
{
	IniMemData ini;
	ini.Load(CustomConfigFile);
	g_pIniLoad = &ini;
	const char* CIniPos = CustomConfigFile.c_str();
	char* buffer = new char[MAX_BUF];
	string st;

	IniRead("Config", "AgreementVersion", "-1", buffer, MAX_BUF);
	int ver2 = stoi(buffer);
	if (ver2 != AgreementVersion)
		IsNeedReshowAgreement = true;

	//==================================================统计信息==================================================
	{
		HKEY hKey = NULL;
		// 读取改用 RegOpenKeyEx：键不存在（首次运行）时不创建空键
		const LSTATUS StatsOpenRet = RegOpenKeyEx(HKEY_CURRENT_USER, StatsRegPath.c_str(), 0, KEY_READ, &hKey);
		if (StatsOpenRet != ERROR_SUCCESS)
		{
			// 首次运行或注册表被清理时没有统计项属正常，只记录日志
			PrintLog("[Stats] 打开注册表统计项失败（首次运行属正常），错误码：" + to_string(StatsOpenRet), PL_WARNING);
		}
		else
		{
			// 读取固定长度的统计项；失败（值不存在除外）只记录日志
			auto ReadStat = [hKey](const char* name, DWORD expectType, void* out, DWORD expectSize) -> bool
			{
				DWORD type = 0, size = expectSize;
				const LSTATUS ret = RegQueryValueEx(hKey, name, NULL, &type, (LPBYTE)out, &size);
				if (ret == ERROR_SUCCESS && type == expectType && size == expectSize)
					return true;
				if (ret != ERROR_FILE_NOT_FOUND)
					PrintLog(string("[Stats] 读取注册表统计项 ") + name + " 失败，错误码：" + to_string(ret), PL_WARNING);
				return false;
			};

			ULONGLONG temp = 0;
			if (ReadStat("0", REG_QWORD, &temp, sizeof(temp))) app_status.ClickerClickCount.left = temp;
			if (ReadStat("1", REG_QWORD, &temp, sizeof(temp))) app_status.ClickerClickCount.right = temp;
			if (ReadStat("2", REG_QWORD, &temp, sizeof(temp))) app_status.MouseClickCount.left = temp;
			if (ReadStat("3", REG_QWORD, &temp, sizeof(temp))) app_status.MouseClickCount.right = temp;

			unsigned int tempUInt = 0;
			if (ReadStat("4", REG_DWORD, &tempUInt, sizeof(tempUInt))) app_status.StartCount = tempUInt;

			// UsageDuration 以字符串存储：用足够大的缓冲区读取，避免原 64 字节缓冲被截断
			char durationBuf[128] = { 0 };
			DWORD type5 = 0, size5 = sizeof(durationBuf);
			const LSTATUS ret5 = RegQueryValueEx(hKey, "5", NULL, &type5, (LPBYTE)durationBuf, &size5);
			if (ret5 == ERROR_SUCCESS && type5 == REG_SZ)
			{
				try
				{
					app_status.UsageDuration = std::stold(durationBuf);
				}
				catch (...)
				{
					PrintLog("[Stats] 使用时长字符串解析失败：" + string(durationBuf), PL_WARNING);
				}
			}
			else if (ret5 == ERROR_MORE_DATA)
			{
				PrintLog("[Stats] 注册表中的使用时长字符串过长，已跳过", PL_WARNING);
			}
			else if (ret5 != ERROR_FILE_NOT_FOUND)
			{
				PrintLog("[Stats] 读取注册表统计项 5 失败，错误码：" + to_string(ret5), PL_WARNING);
			}

			RegCloseKey(hKey);
		}
	}

	//--------------------个性化设置------------------------------
	CIniPos = CustomConfigFile.c_str();
	if (_access(CIniPos, 0) == 0)
	{
		IniRead("个性化", "BgOffsetX", to_string(BgOffsetX).c_str(), buffer, MAX_BUF);
		BgOffsetX = stoi(buffer);
		IniRead("个性化", "BgOffsetY", to_string(BgOffsetY).c_str(), buffer, MAX_BUF);
		BgOffsetY = stoi(buffer);
		IniRead("个性化", "BgScale", to_string(BgScale).c_str(), buffer, MAX_BUF);
		BgScale = stoi(buffer);
		IniRead("个性化", "BgLightness", to_string(BgLightness).c_str(), buffer, MAX_BUF);
		BgLightness = stoi(buffer);
		IniRead("个性化", "GuiItemAlpha", to_string(GuiItemAlpha).c_str(), buffer, MAX_BUF);
		GuiItemAlpha = stoi(buffer);
		IniRead("个性化", "BgAutoResize", (BgAutoResize) ? "TRUE" : "FALSE", buffer, MAX_BUF); st = buffer;
		BgAutoResize = st.find("TRUE") != string::npos;
	}

	g_pIniLoad = nullptr;
	delete[] buffer;
	buffer = nullptr;
}

void LoadConfig(const string& file)
{
	LoadPresetConfig(file);
	LoadGlobalConfig();
}

inline string GetPresetFile(const string& name)
{
	if (name == PresetDefaultName)
		return ConfigFile;
	return PresetDir + "\\" + UTF8ToString(name) + ".ini";
}

inline vector<string> ScanPresets()
{
	vector<string> names = { PresetDefaultName };
	WIN32_FIND_DATAA fd;
	string pattern = PresetDir + "\\*.ini";
	HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE)
	{
		vector<string> custom;
		constexpr int MaxCount = 32; // 限制最多读取 32 个自定义预设，避免无限循环
		int i = 0;
		do
		{
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			string fname = fd.cFileName;
			size_t dot = fname.rfind('.');
			if (dot != string::npos)
				fname = fname.substr(0, dot);
			if (!fname.empty())
				custom.push_back(fname);
			++i;
		} while (FindNextFile(h, &fd) && i < MaxCount);
		FindClose(h);

		if (i >= MaxCount)
			PrintLog("[Preset] 扫描预设时，预设数量超过最大数量限制 32 个，已忽略后续读取。", PL_WARNING);

		sort(custom.begin(), custom.end());
		for (const string& n : custom)
			names.push_back(StringToUTF8(n));
	}
	return names;
}

// from/to 显式传入：不在这里读写全局 CurrentName，避免后台线程与 UI 线程竞争该 string。
// 由 UI 线程负责更新 PresetConfig::CurrentName。
inline void SwitchPreset(const string& from, const string& to)
{
	if (from == to)
		return;
	SavePresetConfig(GetPresetFile(from));
	string target = GetPresetFile(to);
	if (_access(target.c_str(), 0) != 0)
	{
		CreateFile(target);
		SavePresetConfig(target);
	}
	LoadPresetConfig(target);
	PrintLog("[Preset] 切换预设配置到: " + to, PL_INFO);
}

inline bool AddPreset(const string& name)
{
	if (name.empty() || name == PresetDefaultName)
		return false;
	if (name.find_first_of("\\/:*?\"<>|") != string::npos)
		return false;
	string file = GetPresetFile(name);
	if (_access(file.c_str(), 0) == 0)
		return false;
	CreateDirectory(PresetDir.c_str(), NULL);
	CreateFile(file);
	SavePresetConfig(file);
	// 更新预设配置列表
	PresetConfig::Presets.push_back(name);
	PrintLog("[Preset] 创建预设: " + name, PL_INFO);
	return true;
}

inline bool DeletePreset(const string& name)
{
	if (name == PresetDefaultName)
		return false;
	string file = GetPresetFile(name);
	if (DelFile(file) != 0)
		return false;
	// 更新预设配置列表
	for (auto it = PresetConfig::Presets.begin(); it != PresetConfig::Presets.end(); ++it)
	{
		if (*it == name)
		{
			PresetConfig::Presets.erase(it);
			break;
		}
	}
	PrintLog("[Preset] 删除预设: " + name, PL_INFO);
	return true;
}