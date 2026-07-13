#pragma once
#include "Log.h"

void SaveIni(const string& file);
void GetIni(const string& file);
int CreateFile(const string& fileName);
void ClearFile(const string& fileName);
bool DeleteDirectory(const string& path);
int DelFile(const string& file);

bool DeleteDirectory(const string& path)
{
	try
	{
		fs::remove_all(path);
		PrintLog("[File] 已删除文件夹：" + path, PL_INFO);
		return true;
	}
	catch (const fs::filesystem_error& e)
	{
		PrintLog("[Error] 删除文件夹失败：" + path + "，原因：" + e.what(), PL_ERROR);
		return false;
	}
}

int DelFile(const string& file)
{
	fs::path p(file);
	// 如果文件不存在，视为删除成功
	if (!fs::exists(p))
		return 0;
	try
	{
		// 移除只读、系统、隐藏等属性，确保可以删除
		fs::permissions(p, fs::perms::all, fs::perm_options::add);
		bool removed = fs::remove(p);
		if (removed)
		{
			PrintLog("[File] 已删除文件：" + file, PL_INFO);
			return 0;
		}
		else
		{
			// 理论上 exists 为 true 但 remove 返回 false 极少见
			PrintLog("[Error] 删除文件失败（未知原因）：" + file, PL_ERROR);
			return -1;
		}
	}
	catch (const fs::filesystem_error& e)
	{
		int err = e.code().value();
		PrintLog("[Error] 删除文件失败：" + file + "，错误码：" + to_string(err), PL_ERROR);
		return err;
	}
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

void ClearFile(const string& fileName)
{
	ofstream file(fileName, ios::trunc);
	file.clear();
	file.close();
	PrintLog("[File] 已清空文件：" + fileName, PL_INFO);
}

void SaveIni(const string& file)
{
	PrintLog("[Config] 写入配置文件：" + file, PL_INFO);
	const char* CIniPos = file.c_str();
	if (_access(file.c_str(), 0) != 0)
		CreateFile(file);

    // Config Version
    WritePrivateProfileString("Config", "Version", to_string(ConfigVersion).c_str(), CIniPos);
	if (IsNeedReshowAgreement == false) // 不同意就不写入
		WritePrivateProfileString("Config", "AgreementVersion", to_string(AgreementVersion).c_str(), CIniPos);

	// BasicSettings section
	WritePrivateProfileString("BasicSettings", "leftKeyShortcut", to_string(config.LKEY).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightKeyShortcut", to_string(config.RKEY).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftClickerCps", to_string(config.LCPS).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightClickerCps", to_string(config.RCPS).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "mainClickSleep", to_string(config.MainClickSleep).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightMultipleClickValue", to_string(config.RMultipleClickValue).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftMultipleClickValue", to_string(config.LMultipleClickValue).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightEmptyRateValue", to_string(config.REmptyRateValue).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftEmptyRateValue", to_string(config.LEmptyRateValue).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightClickTime", to_string(config.RCTime).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightClickSleep", to_string(config.RCSleep).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftClickTime", to_string(config.LCTime).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftClickSleep", to_string(config.LCSleep).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightClickerMode", to_string(config.RClickerMode).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftClickerMode", to_string(config.LClickerMode).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightFluctuationCps", to_string(config.RFluctuationCPS).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftFluctuationCps", to_string(config.LFluctuationCPS).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightPressDuration", to_string(config.RPressDuration).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftPressDuration", to_string(config.LPressDuration).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "enableRightClickerKey", to_string(config.EnableRightClickerKey).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "enableLeftClickerKey", to_string(config.EnableLeftClickerKey).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "chatBarKey", to_string(config.ChatBarKey).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "bagBarKey", to_string(config.BagBarKey).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "mouseShakeSpeed", to_string(config.MouseShakeSpeed).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "mouseShakeRect", to_string(config.MouseShakeRect).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "quickSwitchTotal", to_string(config.QuickSwitchTotal).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "assistModeCpsThreshold", to_string(config.AssistModeCPSThreshold).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "selfDestructionKey", to_string(config.SelfDestructionKey).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftFluctuationRandMode", to_string(config.UseLFluctuationRandMode).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightFluctuationRandMode", to_string(config.UseRFluctuationRandMode).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "rightStartClickerDelay", to_string(config.RStartClickerDelay).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "leftStartClickerDelay", to_string(config.LStartClickerDelay).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "setGameWindowKey", to_string(config.SetGameWindowKey).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "hideClickerKey", to_string(config.HideClickerKey).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "overheatTime", to_string(config.OverheatTime).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "ohpTimerTimeLossSpeed", to_string(config.OHPTimerTimeLossSpeed).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "simulateClickMode", to_string(config.SimulateClickMode).c_str(), CIniPos);
	WritePrivateProfileString("BasicSettings", "clickerPriorityMode", to_string(config.ClickerPriorityMode).c_str(), CIniPos);

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
			WritePrivateProfileString("CPSWaves", (name + to_string(count)).c_str(), data, CIniPos);
			count++;
		}
	}

	// MouseSoundSettings section
	WritePrivateProfileString("MouseSoundSettings", "mouseSoundVolume", to_string(config.MouseSoundVolume).c_str(), CIniPos);
	WritePrivateProfileString("MouseSoundSettings", "keybdSoundVolume", to_string(config.KeybdSoundVolume).c_str(), CIniPos);
	WritePrivateProfileString("MouseSoundSettings", "noiseVolume", to_string(config.NoiseVolume).c_str(), CIniPos);
	WritePrivateProfileString("MouseSoundSettings", "mouseSoundType", config.MouseSoundType.c_str(), CIniPos);
	WritePrivateProfileString("MouseSoundSettings", "mouseSoundSleep", to_string(config.MouseSoundSleep).c_str(), CIniPos);

	// FeatureSwitches section - bool values
	WritePrivateProfileString("FeatureSwitches", "useRightEmptyRate", config.UseREmptyRate ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftEmptyRate", config.UseLEmptyRate ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightFluctuation", config.UseRFluctuation ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftFluctuation", config.UseLFluctuation ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftClickTime", config.UseLClickTime ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightClickTime", config.UseRClickTime ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useGuiDetection", config.UseGUIDetection ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftPressInterval", config.UseLPressInterval ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightPressInterval", config.UseRPressInterval ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightAutoPressDuration", config.UseRAutoPressDuration ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftAutoPressDuration", config.UseLAutoPressDuration ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useStartWithWindows", config.UseStartWithWindows ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "enableRightClicker", config.EnableRightClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "enableLeftClicker", config.EnableLeftClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useBroadcasterMode", config.UseBroadcasterMode ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useAutoGuiDetect", config.UseAutoGuiDetect ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useMouseSound", config.UseMouseSound ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useKeybdSound", config.UseKeybdSound ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightOnlyMoveClicker", config.UseROnlyMoveClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftOnlyMoveClicker", config.UseLOnlyMoveClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useDisableMoveOnStopMove", config.UseDisableMoveOnStopMove ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useMouseShake", config.UseMouseShake ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useMouseShakeRawInput", config.UseMouseShakeRawInput ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useMsPreventViewSliding", config.UseMSPreventViewSliding ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useMsSmoothSpeed", config.UseMSSmoothSpeed ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useItembarUseMemSearch", config.UseItembarUseMemSearch ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useChinese", config.UseChinese ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useDriverProtect", config.UseDriverProtect ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useHiddenProcess", config.UseHiddenProcess ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useSuspendNetease", config.UseSuspendNetease ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useAutoResetOnDisable", config.UseAutoResetOnDisable ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "allowSetLeftClicker", config.AllowSetLClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "allowSetRightClicker", config.AllowSetRClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useConciseMode", config.UseConciseMode ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useMinimalistMode", config.UseMinimalistMode ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useEnableGameGui", config.UseEnableGameGui ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightOnlyJumpClicker", config.UseROnlyJumpClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftOnlyJumpClicker", config.UseLOnlyJumpClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRestTimeOnStopClicker", config.UseRestTimeOnStopClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useBrutalMode", config.UseBrutalMode ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useBreakBlocksWithClicker", config.UseBreakBlocksWithClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useItemWithClicker", config.UseItemWithClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightStartClickerDelay", config.UseRStartClickerDelay ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftStartClickerDelay", config.UseLStartClickerDelay ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useFilterRightSingleClick", config.UseFilterRSingleClick ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useFilterLeftSingleClick", config.UseFilterLSingleClick ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "usePerformanceMode", config.UsePerformanceMode ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useRightCpsChangeTrend", config.UseRCPSChangeTrend ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useLeftCpsChangeTrend", config.UseLCPSChangeTrend ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useOverheatProtection", config.UseOverheatProtec ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useOhpResetTimerOnStop", config.UseOHPResetTimerOnStop ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useOhpPlaySound", config.UseOHPPlaySound ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "bedrockEditionAdaptation", config.UseBEAdaptation ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "inGameOnlyClicker", config.UseInGameOnlyClicker ? "TRUE" : "FALSE", CIniPos);
	WritePrivateProfileString("FeatureSwitches", "useItembarSwitch", config.UseItembarSwitch ? "TRUE" : "FALSE", CIniPos);

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
	WritePrivateProfileString("QuickSwitchSettings", "rightQuickSwitch", rQuickSwitchStr.c_str(), CIniPos);
	WritePrivateProfileString("QuickSwitchSettings", "leftQuickSwitch", lQuickSwitchStr.c_str(), CIniPos);
	WritePrivateProfileString("QuickSwitchSettings", "quickSwitchKey", quickSwitchKeyStr.c_str(), CIniPos);

	// ItemBarSettings section - arrays and LRPair arrays
	string itemEnableClickerKeyStr;
	for (int i = 0; i < 10; i++)
	{
		itemEnableClickerKeyStr += to_string(config.ItemEnableClickerKey[i]);
		if (i < 9) itemEnableClickerKeyStr += ",";
	}
	WritePrivateProfileString("ItemBarSettings", "itemEnableClickerKey", itemEnableClickerKeyStr.c_str(), CIniPos);

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
	WritePrivateProfileString("ItemBarSettings", "itemBarPresetCpsLeft", itemBarPresetCpsLeftStr.c_str(), CIniPos);
	WritePrivateProfileString("ItemBarSettings", "itemBarPresetCpsRight", itemBarPresetCpsRightStr.c_str(), CIniPos);
	WritePrivateProfileString("ItemBarSettings", "itemBarEnablePresetCpsLeft", itemBarEnablePresetCpsLeftStr.c_str(), CIniPos);
	WritePrivateProfileString("ItemBarSettings", "itemBarEnablePresetCpsRight", itemBarEnablePresetCpsRightStr.c_str(), CIniPos);
	WritePrivateProfileString("ItemBarSettings", "itemEnableClickerLeft", itemEnableClickerLeftStr.c_str(), CIniPos);
	WritePrivateProfileString("ItemBarSettings", "itemEnableClickerRight", itemEnableClickerRightStr.c_str(), CIniPos);

	// RandomSettings section
	WritePrivateProfileString("RandomSettings", "mouseShakeRandomDist", to_string(config.MouseShake_RandomDist).c_str(), CIniPos);
	WritePrivateProfileString("RandomSettings", "mouseShakeRandomGen", to_string(config.MouseShake_RandomGen).c_str(), CIniPos);
	WritePrivateProfileString("RandomSettings", "emptyRateRandomDist", to_string(config.EmptyRate_RandomDist).c_str(), CIniPos);
	WritePrivateProfileString("RandomSettings", "emptyRateRandomGen", to_string(config.EmptyRate_RandomGen).c_str(), CIniPos);
	WritePrivateProfileString("RandomSettings", "fluctuationRandomDist", to_string(config.Fluctuation_RandomDist).c_str(), CIniPos);
	WritePrivateProfileString("RandomSettings", "fluctuationRandomGen", to_string(config.Fluctuation_RandomGen).c_str(), CIniPos);

	// AdvancedSettings section
	WritePrivateProfileString("AdvancedSettings", "memSearchSleep", to_string(config.MemSearchSleep).c_str(), CIniPos);

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
	WritePrivateProfileString("AdvancedSettings", "rightCpsChangeTrendList", rCpsTrendStr.c_str(), CIniPos);
	WritePrivateProfileString("AdvancedSettings", "leftCpsChangeTrendList", lCpsTrendStr.c_str(), CIniPos);

	//-----------------------统计信息-----------------------
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, StatsRegPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
		PrintLog("[Config] 无法创建注册表项", PL_ERROR);
	else
	{
		static const array<void*, 6> Datas = {
			&app_status.ClickerClickCount.left, &app_status.ClickerClickCount.right, &app_status.MouseClickCount.left,
			&app_status.MouseClickCount.right, &app_status.UseTime, &app_status.StartCount
		};

		int IdCount = 0;
		for (void* pdata : Datas)
		{
			if (RegSetValueEx(hKey, to_string(IdCount).c_str(), 0, REG_DWORD, (LPBYTE)pdata, sizeof(pdata)) != ERROR_SUCCESS)
				PrintLog("[Config] 读取注册表项时发生错误", PL_ERROR);
			IdCount++;
		}
		RegCloseKey(hKey);
	}

	//--------------------个性化设置------------------------------
	CIniPos = CustomIniPos.c_str();
	if (_access(CIniPos, 0) != 0)
		CreateFile(CIniPos);
	WritePrivateProfileString("个性化", "BgOffsetX", to_string(BgOffsetX).c_str(), CIniPos);
	WritePrivateProfileString("个性化", "BgOffsetY", to_string(BgOffsetY).c_str(), CIniPos);
	WritePrivateProfileString("个性化", "BgScale", to_string(BgScale).c_str(), CIniPos);
	WritePrivateProfileString("个性化", "BgLightness", to_string(BgLightness).c_str(), CIniPos);
	WritePrivateProfileString("个性化", "GuiItemAlpha", to_string(GuiItemAlpha).c_str(), CIniPos);
	WritePrivateProfileString("个性化", "BgAutoResize", (BgAutoResize) ? "TRUE" : "FALSE", CIniPos);

	//--------------------脚本自启动------------------------------
	if (_access(ScriptInstallList.c_str(), 0) != 0)
		CreateFile(ScriptInstallList);
	// 创建或清空
	fstream ScriptFile(ScriptInstallList, ios::trunc);
	if (ScriptFile.is_open())
	{
		// 读取锁
		shared_lock<shared_mutex> read_lock(InstalledScriptsMtx);
		for (const auto& script : InstalledScripts)
			ScriptFile << script.name << endl;
		ScriptFile.close();
		PrintLog("[Config] 已保存 " + to_string(InstalledScripts.size() - 1) + " 个脚本到自启动列表", PL_INFO);
	}
}

void GetIni(const string& file)
{
	PrintLog("[Config] 读取配置文件：" + file, PL_INFO);
	const char* CIniPos = file.c_str();
	char* buffer = new char[MAX_BUF];
	string st;

	//配置文件版本号
	GetPrivateProfileString("Config", "Version", "-1", buffer, MAX_BUF, CIniPos);
	int ver1 = stoi(buffer);
	if (ver1 != ConfigVersion)
	{
		DelFile(file);
		CreateFile(file);
		SaveIni(file);
	}

	GetPrivateProfileString("Config", "AgreementVersion", "-1", buffer, MAX_BUF, CIniPos);
	int ver2 = stoi(buffer);
	if (ver2 != AgreementVersion)
		IsNeedReshowAgreement = true;

	if (_access(CIniPos, 0) == 0 && ver1 == ConfigVersion)
	{
		// BasicSettings section
		config.LKEY                     = GetPrivateProfileInt("BasicSettings", "leftKeyShortcut", config.LKEY, CIniPos);
		config.RKEY                     = GetPrivateProfileInt("BasicSettings", "rightKeyShortcut", config.RKEY, CIniPos);
		config.LCPS                     = GetPrivateProfileInt("BasicSettings", "leftClickerCps", config.LCPS, CIniPos);
		config.RCPS                     = GetPrivateProfileInt("BasicSettings", "rightClickerCps", config.RCPS, CIniPos);
		config.MainClickSleep           = GetPrivateProfileInt("BasicSettings", "mainClickSleep", config.MainClickSleep, CIniPos);
		config.RMultipleClickValue      = GetPrivateProfileInt("BasicSettings", "rightMultipleClickValue", config.RMultipleClickValue, CIniPos);
		config.LMultipleClickValue      = GetPrivateProfileInt("BasicSettings", "leftMultipleClickValue", config.LMultipleClickValue, CIniPos);
		config.REmptyRateValue          = GetPrivateProfileInt("BasicSettings", "rightEmptyRateValue", config.REmptyRateValue, CIniPos);
		config.LEmptyRateValue          = GetPrivateProfileInt("BasicSettings", "leftEmptyRateValue", config.LEmptyRateValue, CIniPos);
		config.RCTime                   = GetPrivateProfileInt("BasicSettings", "rightClickTime", config.RCTime, CIniPos);
		config.RCSleep                  = GetPrivateProfileInt("BasicSettings", "rightClickSleep", config.RCSleep, CIniPos);
		config.LCTime                   = GetPrivateProfileInt("BasicSettings", "leftClickTime", config.LCTime, CIniPos);
		config.LCSleep                  = GetPrivateProfileInt("BasicSettings", "leftClickSleep", config.LCSleep, CIniPos);
		config.RClickerMode             = GetPrivateProfileInt("BasicSettings", "rightClickerMode", config.RClickerMode, CIniPos);
		config.LClickerMode             = GetPrivateProfileInt("BasicSettings", "leftClickerMode", config.LClickerMode, CIniPos);
		config.RFluctuationCPS          = GetPrivateProfileInt("BasicSettings", "rightFluctuationCps", config.RFluctuationCPS, CIniPos);
		config.LFluctuationCPS          = GetPrivateProfileInt("BasicSettings", "leftFluctuationCps", config.LFluctuationCPS, CIniPos);
		config.RPressDuration           = GetPrivateProfileInt("BasicSettings", "rightPressDuration", config.RPressDuration, CIniPos);
		config.LPressDuration           = GetPrivateProfileInt("BasicSettings", "leftPressDuration", config.LPressDuration, CIniPos);
		config.EnableRightClickerKey    = GetPrivateProfileInt("BasicSettings", "enableRightClickerKey", config.EnableRightClickerKey, CIniPos);
		config.EnableLeftClickerKey     = GetPrivateProfileInt("BasicSettings", "enableLeftClickerKey", config.EnableLeftClickerKey, CIniPos);
		config.ChatBarKey               = GetPrivateProfileInt("BasicSettings", "chatBarKey", config.ChatBarKey, CIniPos);
		config.BagBarKey                = GetPrivateProfileInt("BasicSettings", "bagBarKey", config.BagBarKey, CIniPos);
		config.MouseShakeSpeed          = GetPrivateProfileInt("BasicSettings", "mouseShakeSpeed", config.MouseShakeSpeed, CIniPos);
		config.MouseShakeRect           = GetPrivateProfileInt("BasicSettings", "mouseShakeRect", config.MouseShakeRect, CIniPos);
		config.QuickSwitchTotal         = GetPrivateProfileInt("BasicSettings", "quickSwitchTotal", config.QuickSwitchTotal, CIniPos);
		config.AssistModeCPSThreshold   = GetPrivateProfileInt("BasicSettings", "assistModeCpsThreshold", config.AssistModeCPSThreshold, CIniPos);
		config.SelfDestructionKey       = GetPrivateProfileInt("BasicSettings", "selfDestructionKey", config.SelfDestructionKey, CIniPos);
		config.UseLFluctuationRandMode  = GetPrivateProfileInt("BasicSettings", "leftFluctuationRandMode", config.UseLFluctuationRandMode, CIniPos);
		config.UseRFluctuationRandMode  = GetPrivateProfileInt("BasicSettings", "rightFluctuationRandMode", config.UseRFluctuationRandMode, CIniPos);
		config.RStartClickerDelay       = GetPrivateProfileInt("BasicSettings", "rightStartClickerDelay", config.RStartClickerDelay, CIniPos);
		config.LStartClickerDelay       = GetPrivateProfileInt("BasicSettings", "leftStartClickerDelay", config.LStartClickerDelay, CIniPos);
		config.SetGameWindowKey         = GetPrivateProfileInt("BasicSettings", "setGameWindowKey", config.SetGameWindowKey, CIniPos);
		config.HideClickerKey           = GetPrivateProfileInt("BasicSettings", "hideClickerKey", config.HideClickerKey, CIniPos);

		// 注意：OverheatTime 和 OHPTimerTimeLossSpeed 是浮点数，不能直接用 GetPrivateProfileInt
		GetPrivateProfileString("BasicSettings", "overheatTime", to_string(config.OverheatTime).c_str(), buffer, MAX_BUF, CIniPos);
		config.OverheatTime = stof(buffer);
		GetPrivateProfileString("BasicSettings", "ohpTimerTimeLossSpeed", to_string(config.OHPTimerTimeLossSpeed).c_str(), buffer, MAX_BUF, CIniPos);
		config.OHPTimerTimeLossSpeed = stof(buffer);

		config.SimulateClickMode = GetPrivateProfileInt("BasicSettings", "simulateClickMode", config.SimulateClickMode, CIniPos);
		config.HideClickerKey = GetPrivateProfileInt("BasicSettings", "hideClickerKey", config.HideClickerKey, CIniPos);
		config.ClickerPriorityMode = GetPrivateProfileInt("BasicSettings", "clickerPriorityMode", config.ClickerPriorityMode, CIniPos);

		//==================================================CPSWaves section==================================================
		for (int i = 0; i < 2; i++)
		{
			const char* name = (i == 0) ? "leftWave" : "rightWave";

			int count = 0;
			while (true)
			{
				GetPrivateProfileString("CPSWaves", (name + to_string(count)).c_str(), "", buffer, MAX_BUF, CIniPos);
				if (strlen(buffer) <= 0)
					break;
				vector<string> tokens;
				SplitString(buffer, ",", tokens);
				try
				{
					WaveParam w;
					w.amplitude = stof(tokens[0]);
					w.frequency = stof(tokens[1]);
					w.phase = stof(tokens[2]);
					w.enabled = (tokens[3] == "TRUE");
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
					WaveParam w;
					w.amplitude = Random(5000, 30000) / 10000.0f; // 0.2 - 3.0
					w.frequency = Random(100, 15000) / 10000.0f; // 0.01 - 1.5
					w.phase = Random(-31415926535, 31415926535) / 1000000000.f; // -pi - pi
					w.enabled = true;
					if (i == 0)
						config.CPSWaves.left.push_back(w);
					else
						config.CPSWaves.right.push_back(w);
				}
			}
		}

		//==================================================MouseSoundSettings section==================================================
		config.MouseSoundVolume = GetPrivateProfileInt("MouseSoundSettings", "mouseSoundVolume", config.MouseSoundVolume, CIniPos);
		config.KeybdSoundVolume = GetPrivateProfileInt("MouseSoundSettings", "keybdSoundVolume", config.KeybdSoundVolume, CIniPos);
		config.NoiseVolume = GetPrivateProfileInt("MouseSoundSettings", "noiseVolume", config.NoiseVolume, CIniPos);

		// MouseSoundType 是字符串，需要保留 GetPrivateProfileString
		GetPrivateProfileString("MouseSoundSettings", "rightMouseSoundType", config.MouseSoundType.c_str(), buffer, MAX_BUF, CIniPos);
		config.MouseSoundType = buffer;

		config.MouseSoundSleep = GetPrivateProfileInt("MouseSoundSettings", "mouseSoundSleep", config.MouseSoundSleep, CIniPos);

		//==================================================FeatureSwitches section==================================================
		GetPrivateProfileString("FeatureSwitches", "useRightEmptyRate", config.UseREmptyRate ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseREmptyRate = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftEmptyRate", config.UseLEmptyRate ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLEmptyRate = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightFluctuation", config.UseRFluctuation ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseRFluctuation = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftFluctuation", config.UseLFluctuation ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLFluctuation = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftClickTime", config.UseLClickTime ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLClickTime = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightClickTime", config.UseRClickTime ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseRClickTime = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useGuiDetection", config.UseGUIDetection ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseGUIDetection = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftPressInterval", config.UseLPressInterval ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLPressInterval = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightPressInterval", config.UseRPressInterval ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseRPressInterval = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightAutoPressDuration", config.UseRAutoPressDuration ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseRAutoPressDuration = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftAutoPressDuration", config.UseLAutoPressDuration ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLAutoPressDuration = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useStartWithWindows", config.UseStartWithWindows ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseStartWithWindows = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "enableRightClicker", config.EnableRightClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.EnableRightClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "enableLeftClicker", config.EnableLeftClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.EnableLeftClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useBroadcasterMode", config.UseBroadcasterMode ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseBroadcasterMode = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useAutoGuiDetect", config.UseAutoGuiDetect ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseAutoGuiDetect = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useMouseSound", config.UseMouseSound ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseMouseSound = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useKeybdSound", config.UseKeybdSound ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseKeybdSound = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightOnlyMoveClicker", config.UseROnlyMoveClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseROnlyMoveClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftOnlyMoveClicker", config.UseLOnlyMoveClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLOnlyMoveClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useDisableMoveOnStopMove", config.UseDisableMoveOnStopMove ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseDisableMoveOnStopMove = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useMouseShake", config.UseMouseShake ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseMouseShake = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useMouseShakeRawInput", config.UseMouseShakeRawInput ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseMouseShakeRawInput = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useMsPreventViewSliding", config.UseMSPreventViewSliding ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseMSPreventViewSliding = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useMsSmoothSpeed", config.UseMSSmoothSpeed ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseMSSmoothSpeed = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useItembarUseMemSearch", config.UseItembarUseMemSearch ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseItembarUseMemSearch = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useChinese", config.UseChinese ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseChinese = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useDriverProtect", config.UseDriverProtect ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseDriverProtect = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useHiddenProcess", config.UseHiddenProcess ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseHiddenProcess = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useSuspendNetease", config.UseSuspendNetease ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseSuspendNetease = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useAutoResetOnDisable", config.UseAutoResetOnDisable ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseAutoResetOnDisable = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "allowSetLeftClicker", config.AllowSetLClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.AllowSetLClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "allowSetRightClicker", config.AllowSetRClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.AllowSetRClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useConciseMode", config.UseConciseMode ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseConciseMode = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useMinimalistMode", config.UseMinimalistMode ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseMinimalistMode = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useEnableGameGui", config.UseEnableGameGui ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseEnableGameGui = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightOnlyJumpClicker", config.UseROnlyJumpClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseROnlyJumpClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftOnlyJumpClicker", config.UseLOnlyJumpClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLOnlyJumpClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRestTimeOnStopClicker", config.UseRestTimeOnStopClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseRestTimeOnStopClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useBrutalMode", config.UseBrutalMode ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseBrutalMode = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useBreakBlocksWithClicker", config.UseBreakBlocksWithClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseBreakBlocksWithClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useItemWithClicker", config.UseItemWithClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseItemWithClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightStartClickerDelay", config.UseRStartClickerDelay ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseRStartClickerDelay = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftStartClickerDelay", config.UseLStartClickerDelay ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLStartClickerDelay = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useFilterRightSingleClick", config.UseFilterRSingleClick ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseFilterRSingleClick = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useFilterLeftSingleClick", config.UseFilterLSingleClick ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseFilterLSingleClick = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "usePerformanceMode", config.UsePerformanceMode ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UsePerformanceMode = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useRightCpsChangeTrend", config.UseRCPSChangeTrend ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseRCPSChangeTrend = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useLeftCpsChangeTrend", config.UseLCPSChangeTrend ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseLCPSChangeTrend = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useOverheatProtection", config.UseOverheatProtec ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseOverheatProtec = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useOhpResetTimerOnStop", config.UseOHPResetTimerOnStop ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseOHPResetTimerOnStop = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useOhpPlaySound", config.UseOHPPlaySound ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseOHPPlaySound = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "bedrockEditionAdaptation", config.UseBEAdaptation ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseBEAdaptation = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "inGameOnlyClicker", config.UseInGameOnlyClicker ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseInGameOnlyClicker = st.find("TRUE") != string::npos;
		GetPrivateProfileString("FeatureSwitches", "useItembarSwitch", config.UseItembarSwitch ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos);
		st = buffer;
		config.UseItembarSwitch = st.find("TRUE") != string::npos;

        //==================================================QuickSwitchSettings==================================================
        GetPrivateProfileString("QuickSwitchSettings", "rightQuickSwitch", "", buffer, MAX_BUF, CIniPos);
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

        GetPrivateProfileString("QuickSwitchSettings", "leftQuickSwitch", "", buffer, MAX_BUF, CIniPos);
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

        GetPrivateProfileString("QuickSwitchSettings", "quickSwitchKey", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < MAX_QUICK_SWITCH)
            {
                config.QuickSwitchKey[i++] = stoi(token);
            }
        }

        //==================================================ItemBarSettings section==================================================
        GetPrivateProfileString("ItemBarSettings", "itemEnableClickerKey", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
            {
                config.ItemEnableClickerKey[i++] = stoi(token);
            }
        }

        //==================================================LRPair arrays==================================================
        GetPrivateProfileString("ItemBarSettings", "itemBarPresetCpsLeft", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
            {
                config.ItemBarPresetCPS.left[i++] = stoi(token);
            }
        }

        GetPrivateProfileString("ItemBarSettings", "itemBarPresetCpsRight", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
            {
                config.ItemBarPresetCPS.right[i++] = stoi(token);
            }
        }

        GetPrivateProfileString("ItemBarSettings", "itemBarEnablePresetCpsLeft", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
            {
                config.ItemBarEnablePresetCPS.left[i++] = token.find("TRUE") != string::npos;
            }
        }

        GetPrivateProfileString("ItemBarSettings", "itemBarEnablePresetCpsRight", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
            {
                config.ItemBarEnablePresetCPS.right[i++] = token.find("TRUE") != string::npos;
            }
        }

        GetPrivateProfileString("ItemBarSettings", "itemEnableClickerLeft", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
            {
                config.ItemEnableClicker.left[i++] = token.find("TRUE") != string::npos;
            }
        }

        GetPrivateProfileString("ItemBarSettings", "itemEnableClickerRight", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            stringstream ss(buffer);
            string token;
            int i = 0;
            while (getline(ss, token, ',') && i < 10)
            {
                config.ItemEnableClicker.right[i++] = token.find("TRUE") != string::npos;
            }
        }

        //==================================================RandomSettings section==================================================
        GetPrivateProfileString("RandomSettings", "mouseShakeRandomDist", to_string(config.MouseShake_RandomDist).c_str(), buffer, MAX_BUF, CIniPos);
        config.MouseShake_RandomDist = stoi(buffer);
        GetPrivateProfileString("RandomSettings", "mouseShakeRandomGen", to_string(config.MouseShake_RandomGen).c_str(), buffer, MAX_BUF, CIniPos);
        config.MouseShake_RandomGen = stoi(buffer);
        GetPrivateProfileString("RandomSettings", "emptyRateRandomDist", to_string(config.EmptyRate_RandomDist).c_str(), buffer, MAX_BUF, CIniPos);
        config.EmptyRate_RandomDist = stoi(buffer);
        GetPrivateProfileString("RandomSettings", "emptyRateRandomGen", to_string(config.EmptyRate_RandomGen).c_str(), buffer, MAX_BUF, CIniPos);
        config.EmptyRate_RandomGen = stoi(buffer);
        GetPrivateProfileString("RandomSettings", "fluctuationRandomDist", to_string(config.Fluctuation_RandomDist).c_str(), buffer, MAX_BUF, CIniPos);
        config.Fluctuation_RandomDist = stoi(buffer);
        GetPrivateProfileString("RandomSettings", "fluctuationRandomGen", to_string(config.Fluctuation_RandomGen).c_str(), buffer, MAX_BUF, CIniPos);
        config.Fluctuation_RandomGen = stoi(buffer);

        //==================================================AdvancedSettings section==================================================
        GetPrivateProfileString("AdvancedSettings", "memSearchSleep", to_string(config.MemSearchSleep).c_str(), buffer, MAX_BUF, CIniPos);
        config.MemSearchSleep = stoi(buffer);

        //==================================================Vector读取==================================================
        GetPrivateProfileString("AdvancedSettings", "rightCpsChangeTrendList", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            config.RCPSChangeTrendList.clear();
            stringstream ss(buffer);
            string token;
            while (getline(ss, token, ','))
                config.RCPSChangeTrendList.push_back(stoi(token));
        }

        GetPrivateProfileString("AdvancedSettings", "leftCpsChangeTrendList", "", buffer, MAX_BUF, CIniPos);
        if (strlen(buffer) > 0)
        {
            config.LCPSChangeTrendList.clear();
            stringstream ss(buffer);
            string token;
            while (getline(ss, token, ','))
                config.LCPSChangeTrendList.push_back(stoi(token));
        }
	}
	//==================================================统计信息==================================================
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, StatsRegPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
		PrintLog("[Config] 无法创建注册表项", PL_ERROR);
	else
	{
		static const array<void*, 6> Datas = {
			&app_status.ClickerClickCount.left, &app_status.ClickerClickCount.right, &app_status.MouseClickCount.left, 
			&app_status.MouseClickCount.right, &app_status.UseTime, &app_status.StartCount
		};

		int IdCount = 0;
		for (void* pdata : Datas)
		{
			DWORD RegDataSize;
			if (RegQueryValueEx(hKey, to_string(IdCount).c_str(), NULL, NULL, (LPBYTE)pdata, &RegDataSize) != ERROR_SUCCESS)
				PrintLog("[Config] 读取注册表项时发生错误", PL_ERROR);
			if (RegDataSize != sizeof(pdata))
				PrintLog("[Config] 注册表项数据长度不匹配", PL_ERROR);
			IdCount++;
		}
		RegCloseKey(hKey);
	}

	//--------------------个性化设置------------------------------
	CIniPos = CustomIniPos.c_str();
	if (_access(CIniPos, 0) == 0)
	{
		GetPrivateProfileString("个性化", "BgOffsetX", to_string(BgOffsetX).c_str(), buffer, MAX_BUF, CIniPos);
		BgOffsetX = stoi(buffer);
		GetPrivateProfileString("个性化", "BgOffsetY", to_string(BgOffsetY).c_str(), buffer, MAX_BUF, CIniPos);
		BgOffsetY = stoi(buffer);
		GetPrivateProfileString("个性化", "BgScale", to_string(BgScale).c_str(), buffer, MAX_BUF, CIniPos);
		BgScale = stoi(buffer);
		GetPrivateProfileString("个性化", "BgLightness", to_string(BgLightness).c_str(), buffer, MAX_BUF, CIniPos);
		BgLightness = stoi(buffer);
		GetPrivateProfileString("个性化", "GuiItemAlpha", to_string(GuiItemAlpha).c_str(), buffer, MAX_BUF, CIniPos);
		GuiItemAlpha = stoi(buffer);
		GetPrivateProfileString("个性化", "BgAutoResize", (BgAutoResize) ? "TRUE" : "FALSE", buffer, MAX_BUF, CIniPos); st = buffer;
		BgAutoResize = st.find("TRUE") != string::npos;
	}

	//--------------------脚本自启动------------------------------
	PrintLog("[Config] 找到 " + to_string(AvaiableScripts.size() - 1) + " 个可用脚本", PL_INFO);
	if (!AvaiableScripts.empty())
	{
		ifstream ScriptFile(ScriptInstallList.c_str());
		if (ScriptFile.is_open())
		{
			string line;
			while (getline(ScriptFile, line))
			{
				// 写入锁
				lock_guard<shared_mutex> lock1(AvaiableScriptsMtx);
				lock_guard<shared_mutex> lock2(InstalledScriptsMtx);
				for (auto& asc : AvaiableScripts)
				{
					asc.IsInstalled = true;
					if (asc.name == line)
						InstalledScripts.push_back(asc);
				}
			}
		}
	}

	delete[] buffer;
	buffer = nullptr;
}