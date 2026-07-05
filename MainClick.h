#pragma once
#include "ShareCode.h"
#include "KeyState.h"
#include "random.h"
#include "CodeModules.h"
#include "ConfigParser.h"
#include "Definitons.h"
#include "Log.h"
#include "maths.h"
#include <ctime>

#define MS_ERROR       1
#define MS_SEARCHING   2
#define MS_SUCCESS     3
#define MS_NONE        4

LRPair<float> ClickerDynamicDelay;

bool SwitchMode_LClick = false;
bool SwitchMode_RClick = false;

POINT GetGameWindowCenter(HWND hwnd);
DWORD WINAPI ClickerControl(LPVOID lpParam);
DWORD WINAPI MouseShakeMove(LPVOID lpParam);
DWORD WINAPI GameGuiThread(LPVOID lpParam);
DWORD WINAPI RecordsCPS(LPVOID lpParam);
POINT UseMouseShakeAlgorithm(POINT& view, POINT center);
LRESULT CALLBACK ClickerControlHook(int nCode, WPARAM wParam, LPARAM lParam);
void BeginSleep(const char* flag);
void EndSleep(const char* flag, int time);
DWORD WINAPI RClickRun(LPVOID lpParam);
DWORD WINAPI LClickRun(LPVOID lpParam);
/*
DWORD WINAPI ItemBarSwitch(LPVOID lpParam)
{
	HWND backup_hwnd = GameHwnd;
	while (!GCExit)
	{
		IsSearchState = MS_NONE;
		if (!IsMemSearch || GetForegroundWindow() != GameHwnd || GameHwnd == 0)
		{
			Sleep(20);
			continue;
		}

		IsSearchState = MS_SEARCHING;
		backup_hwnd = GameHwnd;

		RenderTip(u8"2秒后开始内存搜索", 1.5f, PT_INFO);

		Sleep(2000);

		PrintLog("[MemSearch]开始寻找物品栏的数值地址", PL_INFO);
		RenderTip(u8"开始内存搜索", 2.0f, PT_WARNING);
		MessageBeep(MB_ICONINFORMATION);
		void* address = 0;

		DWORD GamePid;
		GetWindowThreadProcessId(GameHwnd, &GamePid);
		
		MemorySearch mem(GamePid);
		{
			mem.LoadProcess();
			if (!mem.IsSuccess())
			{
				PrintLog("[MemSearch]加载游戏进程失败！", PL_ERROR);
				RenderTip(u8"无法打开进程: " + to_string(GamePid), 2.0f, PT_ERROR);
				IsSearchState = MS_ERROR;
				IsMemSearch = false;
				Sleep(100);
				continue;
			}

			// 开始寻找地址
			int value = 8;
			SetKeyClick(config.ItemEnableClickerKey[value], 80);
			Sleep(100);
			int err = mem.SearchMemory(&value, sizeof(int));
			if (err <= 0)
			{
				PrintLog("[MemSearch]第一次内存搜索出现错误！错误代码:" + to_string(err), PL_ERROR);
				RenderTip(u8"第一次内存搜索出现错误！", 2.0f, PT_ERROR);
				MessageBeep(MB_ICONERROR);
				IsSearchState = MS_ERROR;
				IsMemSearch = false;
			}
			vector<void*> results = mem.GetSearchResults();

			int CountAgain = 0;
			size_t size = results.size();
			size_t LastSize = 0;
			PrintLog("[MemSearch]第一次搜索结果数量: " + to_string(size), PL_INFO);

			while (size > 2 && MemSearchCount < 3)
			{
				if (LastSize == size)
					MemSearchCount++;
				else
					MemSearchCount = 0;

				CountAgain++;

				if (GetForegroundWindow() != GameHwnd)
				{
					PrintLog("[MemSearch]游戏已窗口切换，停止寻找物品栏的数值地址。", PL_INFO);
					RenderTip(u8"你在搜索中途切换了游戏窗口，搜索失败。", 2.0f, PT_ERROR);
					MessageBeep(MB_ICONERROR);
					IsSearchState = MS_ERROR;
					break;
				}

				value = Random(0, 8);
				SetKeyClick(config.ItemEnableClickerKey[value], 80);
				Sleep(100);
				int err = mem.SearchMemoryAgain(&value, sizeof(int));
				if (err < 0)
				{
					PrintLog("[MemSearch]内存搜索中途出现错误！错误代码:" + to_string(err), PL_WARNING);
					RenderTip(u8"在第 " + to_string(CountAgain) + u8" 内存搜索中途出现错误！\n将尝试继续搜索。", 2.0f, PT_WARNING);
					MessageBeep(MB_ICONWARNING);
				}
				else
				{
					results = mem.GetSearchResults();
					size = results.size();
					PrintLog("[MemSearch]已完成内存搜索的第 " + to_string(CountAgain) + " 次，搜索结果数量:" + to_string(size), PL_INFO);
					RenderTip(u8"已完成第 " + to_string(CountAgain) + u8" 次搜索。", 1.5f, PT_INFO);
					MessageBeep(MB_ICONINFORMATION);
				}
				LastSize = size;
			}

			if (!results.empty() && results.size() < 100)
			{
				address = results[0];
				char szAddress[64] = { 0 };
				wsprintf(szAddress, "0x%p", address);
				PrintLog("[MemSearch]已经找到物品栏的数值地址" + string(szAddress), PL_INFO);
				RenderTip(u8"搜索成功！", 2.0f, PT_INFO);
				MessageBeep(MB_ICONINFORMATION);
				IsSearchState = MS_SUCCESS;
			}
			else
			{
				PrintLog("[MemSearch]未找到物品栏的数值地址。", PL_ERROR);
				RenderTip(u8"未找到物品栏的数值地址，搜索失败！", 2.0f, PT_ERROR);
				MessageBeep(MB_ICONERROR);
				IsSearchState = MS_ERROR;
			}
		}

		PrintLog("[MemSearch]开始监控物品栏位置内存。", PL_INFO);
		MemSearchCount = 0;
		IsMemSearch = false;
		while (!GCExit && IsSearchState != MS_ERROR)
		{
			if (IsSetItembarPos)
			{
				mem.WriteMemoryValue(address, WillSetItembarPos);
				WillSetItembarPos = 0;
				IsSetItembarPos = false;
			}

			ItemBarPos = mem.GetAddressValue<int>(address);

			config.EnableRightClicker = config.ItemEnableClicker.right[ItemBarPos];
			config.EnableLeftClicker = config.ItemEnableClicker.left[ItemBarPos];

			if (backup_hwnd != GameHwnd)
			{
				backup_hwnd = GameHwnd;
				PrintLog("[MemSearch]游戏已窗口切换，等待确定重新搜索。", PL_INFO);
				RenderTip(u8"你重新选择了游戏窗口，需要重新搜索！", 4.0f, PT_WARNING);
				IsMemSearch = false;
				break;
			}
			Sleep(MemSearchSleep);
		}
	}
	PrintLog("[MemSearch]游戏内存读取线程退出", PL_INFO);
	return 0;
}
*/
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

DWORD WINAPI ClickerControl(LPVOID lpParam)
{
	HANDLE RClickerThread = CreateThreadSafe(RClickRun, NULL, 0, "RClickRun");
	HANDLE LClickerThread = CreateThreadSafe(LClickRun, NULL, 0, "LClickRun");
	while (!GCExit)
	{
		{
			static ULONGLONG LastTime = GetTickCount64() + 3000;

			if (LastTime <= GetTickCount64()) // 3秒一次
			{
				// 自动清理本进程内存
				size_t MemSize = 0;
				PROCESS_MEMORY_COUNTERS pmc;
				if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
					MemSize = pmc.WorkingSetSize / (1024 * 1024); // 单位转换为MB
				if (MemSize > 240)
					ClearProcessMemory();
				LastTime = GetTickCount64() + 3000;
			}
		}

		if (ReloadClicker)
		{
			IsClickerSuccess = false;
			ReloadClicker = false;
			
			// 没什么用，保留就行
			srand(time(NULL));

			SaveIni(IniPos.c_str());
			InitRandom();
			ClickerDynamicDelay.right = 1000.0f / static_cast<int>(max(config.RCPS, 0));
			ClickerDynamicDelay.left = 1000.0f / static_cast<int>(max(config.LCPS, 0));
			IsClickerSuccess = true;
			PrintLog("[Clicker] 重新加载连点成功", PL_INFO);
		}

		// 一些快捷键处理
		if (GetKeyClick(config.SetGameWindowKey))
		{
			GameHwnd = GetForegroundWindow();
			PrintLog("[Clicker] 已设置游戏窗口", PL_INFO);
		}
		if (GetKeyClick(config.HideClickerKey))
		{
			PrintLog("[Clicker] 已隐藏GC窗口", PL_INFO);
			ShowWindow(GCHWND, SW_HIDE);
		}
		if (GetForegroundWindow() != GCHWND && config.SelfDestructionKey > 6 && GetKeyDown(config.SelfDestructionKey))
		{
			GCExit = true;
			PrintLog("[Clicker] 程序退出，用户按下了自毁键", PL_INFO);
			return 0;
		}

		// 统一处理IsOpenGui和IsMoving
		const bool IsJumping = GetKeyDown(VK_SPACE);
		IsMoving = GetKeyDown('W') || GetKeyDown('S') || GetKeyDown('D') || GetKeyDown('A') || IsJumping;
		{
			// 用0来初始化，启动了GC就会初始化IsOpenGui和IsCursorHidden的内部
			static ULONGLONG LastTime = 0;
			// 每40ms检测一次
			if (LastTime < GetTickCount64())
			{
				IsOpenGui = !IsCursorHidden(); // 不要忘记感叹号（not），已经出现过一次这个BUG了
				LastTime = GetTickCount64() + 40;
			}
		}

		// 仅移动连点 & 仅跳跃连点
		if (config.UseROnlyMoveClicker && config.AllowSetRClicker)
		{
			const bool Temp = IsClickering.right || IsMoving;
			if (config.UseDisableMoveOnStopMove)
				config.EnableRightClicker = IsMoving;
			else if (VarHasChanged(Temp, "ClickerControl_Temp1")) // 使用VarHasChanged防止无法手动关闭或者其他控制（防止一直写变量）
				config.EnableRightClicker = Temp;
		}
		if (config.UseLOnlyMoveClicker && config.AllowSetLClicker)
		{
			const bool Temp = IsClickering.left || IsMoving;
			if (config.UseDisableMoveOnStopMove)
				config.EnableLeftClicker = IsMoving;
			else if(VarHasChanged(Temp, "ClickerControl_Temp2"))
				config.EnableLeftClicker = Temp;
		}

		// GUI检测 - 注意和（仅移动连点 & 仅跳跃连点）的顺序，GUI检测权重要大，所以放这个后面
		if (config.UseGUIDetection && VarHasChanged(IsOpenGui, "IsOpenGui"))
		{
			if (config.AllowSetLClicker)
				config.EnableLeftClicker = !IsOpenGui;
			if (config.AllowSetRClicker)
				config.EnableRightClicker = !IsOpenGui;
		}
		
		// 物品栏检测
		for (int i = 0; i < 9; i++)
		{
			if (GetKeyClick(config.ItemEnableClickerKey[i]))
				ItemBarPos = i;
		}

		// 物品栏开关 & 物品栏预设CPS切换
		if (config.UseItembarSwitch && VarHasChanged(ItemBarPos, "ItemBarPos"))
		{
			Clamp(ItemBarPos, 0, 8);
			if (config.AllowSetRClicker)
				config.EnableRightClicker = config.ItemEnableClicker.right[ItemBarPos];
			if (config.AllowSetLClicker)
				config.EnableLeftClicker = config.ItemEnableClicker.left[ItemBarPos];

			if (!config.UseBrutalMode)
			{
				if (config.ItemBarEnablePresetCPS.right[ItemBarPos])
					config.RCPS = config.ItemBarPresetCPS.right[ItemBarPos];
				if (config.ItemBarEnablePresetCPS.left[ItemBarPos])
					config.LCPS = config.ItemBarPresetCPS.left[ItemBarPos];
			}
		}

		// 仅跳跃触发连点
		const bool IsJumpHasChanged = VarHasChanged(IsJumping, "IsJumping");
		if (IsJumpHasChanged && config.UseROnlyJumpClicker && !config.UseROnlyMoveClicker && config.AllowSetRClicker)
			config.EnableRightClicker = IsJumping;
		if (IsJumpHasChanged && config.UseLOnlyJumpClicker && !config.UseLOnlyMoveClicker && config.AllowSetLClicker)
			config.EnableLeftClicker = IsJumping;

		// 快速切换CPS
		for (int i = 0; i < config.QuickSwitchTotal; i++)
		{
			if (GetKeyClick(config.QuickSwitchKey[i]))
			{
				config.RCPS = config.RQuickSwitch[i];
				config.LCPS = config.LQuickSwitch[i];
				ClickerDynamicDelay.left = round(1000.0f / max(config.LCPS, 0));
				ClickerDynamicDelay.right = round(1000.0f / max(config.RCPS, 0)); 
				PlayEnableSound();
			}
		}

		// CPS波动
		if (config.UseRFluctuation)
		{
			const int cps = Random(config.RCPS - config.RFluctuationCPS, config.RCPS + config.RFluctuationCPS, config.MouseShake_RandomGen, config.MouseShake_RandomDist);
			ClickerDynamicDelay.right = round(1000.0f / max(cps, 0));
		}
		else
			ClickerDynamicDelay.right = round(1000.0f / max(config.RCPS, 0));
		if (config.UseLFluctuation)
		{
			const int cps = Random(config.LCPS - config.LFluctuationCPS, config.LCPS + config.LFluctuationCPS, config.MouseShake_RandomGen, config.MouseShake_RandomDist);
			ClickerDynamicDelay.left = round(1000.0f / max(cps, 0));
		}
		else
			ClickerDynamicDelay.left = round(1000.0f / max(config.LCPS, 0));

		// 过热保护
		if (config.UseOverheatProtec)
		{
			static ULONGLONG OverTime = GetTickCount64() + 100;
			static bool IsTrigger = false;
			if (GetTickCount64() >= OverTime)
			{
				if (IsClickering.right || IsClickering.left)
				{
					if (OverheatProtecTimer >= config.OverheatTime)
					{
						if (config.AllowSetLClicker) config.EnableLeftClicker = false;
						if (config.AllowSetRClicker) config.EnableRightClicker = false;
						IsTrigger = true;
						if (config.UseOHPPlaySound)
							MessageBeep(MB_ICONWARNING);
					}
					else
						OverheatProtecTimer += 0.1f;
				}
				else
				{
					if (config.UseOHPResetTimerOnStop)
						OverheatProtecTimer = 0;
					else if (OverheatProtecTimer >= 0)
						OverheatProtecTimer -= config.OHPTimerTimeLossSpeed;
					if (OverheatProtecTimer < 0) OverheatProtecTimer = 0;
				}
				if (OverheatProtecTimer == 0)
				{
					if (config.UseOHPResetTimerOnStop)
					{
						if (config.AllowSetLClicker) config.EnableLeftClicker = true;
						if (config.AllowSetRClicker) config.EnableRightClicker = true;
						IsTrigger = false;
						if (config.UseOHPPlaySound)
							MessageBeep(MB_ICONINFORMATION);
					}
					else
					{
						bool RClickerState = false;
						bool LClickerState = false;
						switch (config.RClickerMode)
						{
						case C_MOUSESMODE:
							RClickerState = GetKeyDown(VK_RBUTTON); break;
						case C_ASSISTMODE:
							RClickerState = config.RCPS >= config.AssistModeCPSThreshold; break;
						case C_BUTTONMODE:
							RClickerState = GetKeyDown(config.RKEY); break;
						case C_SWITCHMODE:
							RClickerState = SwitchMode_RClick; break;
						}
						switch (config.LClickerMode)
						{
						case C_MOUSESMODE:
							LClickerState = GetKeyDown(VK_LBUTTON); break;
						case C_ASSISTMODE:
							LClickerState = config.LCPS >= config.AssistModeCPSThreshold; break;
						case C_BUTTONMODE:
							LClickerState = GetKeyDown(config.LKEY); break;
						case C_SWITCHMODE:
							LClickerState = SwitchMode_LClick; break;
						}

						if (!RClickerState && !LClickerState)
						{
							if (config.AllowSetLClicker && !IsOpenGui) config.EnableLeftClicker = true;
							if (config.AllowSetRClicker && !IsOpenGui) config.EnableRightClicker = true;
							if (config.UseOHPPlaySound)
								MessageBeep(MB_ICONINFORMATION);
						}
					}
				}
				OverTime = GetTickCount64() + 100;
			}
		}

		{
			static ULONGLONG LastTime = GetTickCount64();
			// RecordTime为1小时
			static const ULONGLONG RecordTime = static_cast<ULONGLONG>(1 * 1000 * 60 * 60);
			if (GetTickCount64() - LastTime >= RecordTime)
				app_status.UseTime += 0.1; // 10秒
		}
		//处理溢出(没用，+0.1，一百年都到不了)
		//if (LMouseClickCount >= LDBL_MAX - 2) LMouseClickCount = 0;
		/*
		* 关于人类在打游戏时点击鼠标的次数，根据不同的数据源，结果有所不同。以下是一些相关的信息：
		* 1. 普通人使用鼠标的日常点击次数大约是1000次左右。如果是用于办公或玩游戏，这个数字可能会增加1.5到2倍，大约在1500次左右。
		* 2. 在玩游戏时，正常人的手速大约是每秒5到8次，这包括鼠标点击和键盘敲击。特别是在玩即时战略游戏如星际争霸或魔兽争霸时，每分钟操作数（APM）可以达到110左右，这反映了玩家的操作速度。
		* 3. 对于专业玩家来说，APM（每分钟操作数）通常在350到400次左右，这意味着平均每秒大约有6到7次鼠标点击和键盘敲击。
		* 基于这些数据，我们可以估算，如果一个玩家每天玩游戏10小时，每分钟点击鼠标350次，那么一天内的点击次数将是350次/分钟 × 60分钟/小时 × 10小时/天 = 210,000次。这样的点击频率下，
		* unsigned long long 类型（通常为64位，最大值约为18,446,744,073,709,551,615）会在大约 86,542,663.67 天后溢出。
		* 这远远超过了实际可能的游戏时间，因此在使用 unsigned long long 类型记录鼠标点击次数时，溢出的风险非常低。
		*/

		// 仅限游戏中连点
		if (config.UseInGameOnlyClicker)
		{
			HWND hwnd = GetForegroundWindow();
			if (VarHasChanged(hwnd, "ClickerControl_ForegroundWindow") && hwnd != GameHwnd)
			{
				if (config.AllowSetLClicker) config.EnableLeftClicker = false;
				if (config.AllowSetRClicker) config.EnableRightClicker = false;
			}
			else
			{
				if (config.AllowSetLClicker) config.EnableLeftClicker = true;
				if (config.AllowSetRClicker) config.EnableRightClicker = true;
			}
		}

		// 强制开关连点 - 这个权重大于一切
		if (GetKeyClick(config.EnableRightClickerKey))
			config.EnableRightClicker = !config.EnableRightClicker;
		if (GetKeyClick(config.EnableLeftClickerKey))
			config.EnableLeftClicker = !config.EnableLeftClicker;

		// 播放开启或者关闭的提示音
		if (VarHasChanged(config.EnableRightClicker, "config.EnableRightClicker"))
		{
			if (config.EnableRightClicker) PlayEnableSound();
			else PlayDisableSound();
		}
		if (VarHasChanged(config.EnableLeftClicker, "config.EnableLeftClicker"))
		{
			if (config.EnableLeftClicker) PlayEnableSound();
			else PlayDisableSound();
		}

		Sleep(10);
	}
	PrintLog("[Clicker]连点控制线程退出", PL_INFO);
	return 0;
}

struct FPOINT {
	float x;
	float y;
	void operator = (POINT pt) { x = pt.x; y = pt.y; }
};

POINT UseMouseShakeAlgorithm(POINT& view, POINT center)
{
	RECT ShakeRect{};
	ShakeRect.left =   center.x - config.MouseShakeRect;
	ShakeRect.top =    center.y - config.MouseShakeRect;
	ShakeRect.right =  center.x + config.MouseShakeRect;
	ShakeRect.bottom = center.y + config.MouseShakeRect;
	POINT MousePos{};

	if (config.UseMSPreventViewSliding)
	{
		POINT points{};
		if (view.x < -config.MouseShakeRect) view.x = -config.MouseShakeRect;
		if (view.y < -config.MouseShakeRect) view.y = -config.MouseShakeRect;
		if (view.x > config.MouseShakeRect)  view.x = config.MouseShakeRect;
		if (view.y > config.MouseShakeRect)  view.y = config.MouseShakeRect;

		// 计算随机范围
		POINT min = { -(view.x + config.MouseShakeRect), -(view.y + config.MouseShakeRect) };
		POINT max = { config.MouseShakeRect - view.x, config.MouseShakeRect - view.y };

		// 设置随机后的坐标
		points = {
			Random(min.x, max.x, config.MouseShake_RandomGen, config.MouseShake_RandomDist),
			Random(min.y, max.y, config.MouseShake_RandomGen, config.MouseShake_RandomDist)
		};
		MousePos = { center.x + points.x, center.y + points.y };
		// 设置视角，加上偏移
		view.x += points.x;
		view.y += points.y;
	}
	else
	{
		// 设置随机后的坐标
		MousePos = {
			Random(ShakeRect.left, ShakeRect.right, config.MouseShake_RandomGen, config.MouseShake_RandomDist),
			Random(ShakeRect.top, ShakeRect.bottom, config.MouseShake_RandomGen, config.MouseShake_RandomDist)
		};
	}

	return MousePos;
}

DWORD WINAPI MouseShakeMove(LPVOID lpParam)
{
	POINT view{0, 0};
	FPOINT MousePos{};
	FPOINT AnimPos{};
	HWND hwnd = 0;
	bool moving = false;

	const int ScreenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	const int ScreenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;

	while (!GCExit)
	{
		BeginSleep("MousShake");

		if (!config.UseMouseShake)
		{
			view = { 0, 0 };
			Sleep(100);
			continue;
		}

		if ((IsClickerDown.right || IsClickerDown.left) && GetForegroundWindow() != GCHWND)
		{
			POINT pt{};
			GetCursorPos(&pt);
			if (!moving)
				AnimPos = pt;

			// 不能使用相对坐标(设置中心为0)来一点鼠标，因为anime会导致移动多次，因为SendInput移动是相对位置
			MousePos = UseMouseShakeAlgorithm(view, pt);
			moving = true;
		}

		if (IsOpenGui)
			view = { 0, 0 };

		if (moving)
		{
			input.mi.dwFlags = (MOUSEEVENTF_MOVE | (config.UseMouseShakeRawInput ? MOUSEEVENTF_ABSOLUTE : 0));
			if (config.UseMSSmoothSpeed)
			{
				const float speed = (config.MouseShakeSpeed / 1000.0f) * (1.0f - (1.0f / 60.0f));

				AnimPos.x = Lerp(AnimPos.x, MousePos.x, speed);
				AnimPos.y = Lerp(AnimPos.y, MousePos.y, speed);

				POINT pt{};
				GetCursorPos(&pt);
				input.mi.dx = config.UseMouseShakeRawInput ? 
					(AnimPos.x * 65535) / ScreenWidth : round(AnimPos.x) - pt.x;
				input.mi.dy = config.UseMouseShakeRawInput ? 
					(AnimPos.y * 65535) / ScreenHeight : round(AnimPos.y) - pt.y;
				
				SendInput(1, &input, sizeof(INPUT));
				if (abs(AnimPos.x - MousePos.x) <= 1 && abs(AnimPos.y - MousePos.y) <= 1)
					moving = false;
			}
			else
			{
				POINT pt{};
				GetCursorPos(&pt);
				input.mi.dx = config.UseMouseShakeRawInput ?
					(MousePos.x * 65535) / ScreenWidth : round(AnimPos.x) - pt.x;
				input.mi.dy = config.UseMouseShakeRawInput ?
					(MousePos.y * 65535) / ScreenHeight : round(AnimPos.y) - pt.y;
				SendInput(1, &input, sizeof(INPUT));
				moving = false;
			}
		}
		EndSleep("MousShake", 1000 / 60);
	}
	PrintLog("[Clicker] 鼠标抖动线程退出", PL_INFO);
	return 0;
}

DWORD WINAPI RecordsCPS(LPVOID lpParam)
{
	ULONGLONG EndTime = GetNowMs() + 50;
	RRecordsCPS.reserve(MAX_RECORDS);
	LRecordsCPS.reserve(MAX_RECORDS);

	LRPair<deque<ULONGLONG>> AllTimeList;
	LRPair<deque<ULONGLONG>> HumanTimeList;
	while (!GCExit)
	{
		// 综合统计CPS，包括连点器和人类点击
		//if (IsHumanClick.left || IsClickerDown.left)
		if (GetKeyClick(VK_LBUTTON))
		{
			CurrentCPS.left++;
			AllTimeList.left.push_back(GetNowMs() + 1000);
		}
		//if (IsHumanClick.right || IsClickerDown.right)
		if (GetKeyClick(VK_RBUTTON))
		{
			CurrentCPS.right++;
			AllTimeList.right.push_back(GetNowMs() + 1000);
		}
		// 统计实际人类点击的CPS，忽略连点器
		//if (IsHumanClick.left)
		if (GetKeyClick(VK_LBUTTON, true))
		{
			app_status.MouseClickCount.left++;

			HumanCPS.left++;
			HumanTimeList.left.push_back(GetNowMs() + 1000);
		}
		//if (IsHumanClick.right)
		if (GetKeyClick(VK_RBUTTON, true))
		{
			app_status.MouseClickCount.right++;

			HumanCPS.right++;
			HumanTimeList.right.push_back(GetNowMs() + 1000);
		}

		// 连点器CPS 衰减计算
		if (AllTimeList.left.size() > 0)
		{
			if (GetNowMs() >= AllTimeList.left[0])
			{
				CurrentCPS.left--;
				AllTimeList.left.erase(AllTimeList.left.begin());
			}
		}
		if (AllTimeList.right.size() > 0)
		{
			if (GetNowMs() >= AllTimeList.right[0])
			{
				CurrentCPS.right--;
				AllTimeList.right.erase(AllTimeList.right.begin());
			}
		}
		// 人类CPS 衰减计算
		if (HumanTimeList.left.size() > 0)
		{
			if (GetNowMs() >= HumanTimeList.left[0])
			{
				HumanCPS.left--;
				HumanTimeList.left.erase(HumanTimeList.left.begin());
			}
		}
		if (HumanTimeList.right.size() > 0)
		{
			if (GetNowMs() >= HumanTimeList.right[0])
			{
				HumanCPS.right--;
				HumanTimeList.right.erase(HumanTimeList.right.begin());
			}
		}

		if (GetNowMs() > EndTime)
		{
			RecordsCPSMutex.lock();
			{
				if (RRecordsCPS.size() >= MAX_RECORDS)
					RRecordsCPS.erase(RRecordsCPS.begin());
				if (LRecordsCPS.size() >= MAX_RECORDS)
					LRecordsCPS.erase(LRecordsCPS.begin());

				RRecordsCPS.push_back(CurrentCPS.right);
				LRecordsCPS.push_back(CurrentCPS.left);
			}
			RecordsCPSMutex.unlock();

			EndTime = GetNowMs() + 50;
		}
		IsClickerDown.left = false;
		IsClickerDown.right = false;

		Sleep(2);
	}
	return 0;
}

LRESULT CALLBACK ClickerControlHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		switch (wParam)
		{
			case WM_MOUSEWHEEL:
			{
				if (!IsOpenGui && GetForegroundWindow() != GCHWND) // 一定要加上判断IsOpenGui，这是误差产生的原因！
				{
					MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
					int wheelDelta = GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData) / WHEEL_DELTA;
					ItemBarPos -= wheelDelta; // 这里不需要%，因为下面会处理
					if (ItemBarPos < 0)
						ItemBarPos += 9;
					else
						ItemBarPos %= 9;
					break;
				}
			}
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD WINAPI RClickRun(LPVOID lpParam)
{
	while (!GCExit)
	{
		while (!IsClickerSuccess || StopClicker)
			Sleep(20);

		float ClickRates = (100 - (config.UseREmptyRate ? config.REmptyRateValue : 0)) / 100.0f;
		ULONGLONG LastTime = 0;
		ULONGLONG EndTime = GetTickCount64() + config.RCTime;
		ULONGLONG SlepTime = EndTime + config.RCSleep;
		bool IsEndTime = false;
		int AssistModeCount = -1;
		int apd = 0;

		while (!GCExit && !ReloadClicker && !StopClicker)
		{
			// 优先级处理
			if (config.ClickerPriorityMode == CP_LC && IsClickering.left)
			{
				Sleep(config.MainClickSleep);
				continue;
			}
			//=========================================默认模式=========================================
			if (config.RClickerMode & C_BUTTONMODE && !(config.RClickerMode & C_SWITCHMODE))
			{
				const bool IsKeyPressed = GetKeyDown(config.RKEY, config.UseHumanOnlyTrigger, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				// 连点间隔专属变量
				bool IsDelayEnd = false;
				if (!IsKeyPressed && LastTime != 0)
					LastTime = 0;

				if (!IsKeyPressed)
					IsDelayEnd = false;

				if (config.UseRClickTime && IsDelayEnd)
				{
					if (GetTickCount64() >= EndTime && IsEndTime || IsRestTime)
					{
						SlepTime = GetTickCount64() + config.RCSleep;
						IsEndTime = false;
					}
					if (GetTickCount64() >= SlepTime && !IsEndTime || IsRestTime)
					{
						EndTime = GetTickCount64() + config.RCTime;
						IsEndTime = true;
					}
				}
				if (config.EnableRightClicker && IsKeyPressed && RandomBool(ClickRates, config.EmptyRate_RandomGen, config.EmptyRate_RandomDist))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseRClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseRStartClickerDelay || LastTime + config.RStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.right = true;
						if (config.UseRPressInterval)
							apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right / 2) : (ClickerDynamicDelay.right - config.RPressDuration);
						SetKeyClick(Click::Right, config.UseRPressInterval ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						EndSleep("RightClicker", ClickerDynamicDelay.right);
						goto label_rc_end;
					}
					else IsClickering.right = false;
				}
				else
					IsClickering.right = false;
			}
			//=========================================鼠标模式=========================================
			if (config.RClickerMode & C_MOUSESMODE && !(config.RClickerMode & C_ASSISTMODE) && GCHWND != GetForegroundWindow())
			{
				static bool IsDownMouse = false;
				const bool IsKeyPressed = GetKeyDown(2, true, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				bool IsDelayEnd = false;
				if (!IsKeyPressed && LastTime != 0)
					LastTime = 0;

				if (!IsKeyPressed)
					IsDelayEnd = false;

				if (config.UseRClickTime && IsDelayEnd)
				{
					if (GetTickCount64() >= EndTime && IsEndTime || IsRestTime)
					{
						SlepTime = GetTickCount64() + config.RCSleep;
						IsEndTime = false;
					}
					if (GetTickCount64() >= SlepTime && !IsEndTime || IsRestTime)
					{
						EndTime = GetTickCount64() + config.RCTime;
						IsEndTime = true;
					}
				}
				if ((!config.EnableRightClicker || !IsKeyPressed) && IsDownMouse)
				{
					SetKeyUp(Click::Right, config.SimulateClickMode);
					IsDownMouse = false;
				}

				if (config.EnableRightClicker && IsKeyPressed && RandomBool(ClickRates, config.EmptyRate_RandomGen, config.EmptyRate_RandomDist))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if (  (!config.UseRClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && 
						  (!config.UseRStartClickerDelay || LastTime + config.RStartClickerDelay <= GetTickCount64()) &&
						  (!config.UseFilterRSingleClick || LastTime + FILTER_SC_SLEEP <= GetTickCount64())  )
					{
						IsClickering.right = true;
						if (config.UseRPressInterval)
							apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right / 2) : (ClickerDynamicDelay.right - config.RPressDuration);
						if (config.UseItemWithClicker)
						{
							SetKeyDown(Click::Right, config.SimulateClickMode);
							IsDownMouse = true;
						}
						else
							SetKeyClick(Click::Right, config.UseRPressInterval ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						EndSleep("RightClicker", ClickerDynamicDelay.right);
						goto label_rc_end;
					}
					else IsClickering.right = false;
				}
				else IsClickering.right = false;
			}
			//=========================================开关模式=========================================
			if (config.RClickerMode & C_SWITCHMODE && !(config.RClickerMode & C_BUTTONMODE))
			{
				// 这个模式特殊一点
				const bool IsTrigger = SwitchMode_RClick;
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsTrigger;
				bool IsDelayEnd = false;
				if (!IsTrigger && LastTime != 0)
					LastTime = 0;

				if (!IsTrigger)
					IsDelayEnd = false;

				if (config.UseRClickTime && IsDelayEnd)
				{
					if (GetTickCount64() >= EndTime && IsEndTime || IsRestTime)
					{
						SlepTime = GetTickCount64() + config.RCSleep;
						IsEndTime = false;
					}
					if (GetTickCount64() >= SlepTime && !IsEndTime || IsRestTime)
					{
						EndTime = GetTickCount64() + config.RCTime;
						IsEndTime = true;
					}
				}
				if (GetKeyClick(config.RKEY, config.UseHumanOnlyTrigger, config.ReadKeyStateMode))
					SwitchMode_RClick = !SwitchMode_RClick;
				if (SwitchMode_RClick && config.EnableRightClicker && RandomBool(ClickRates, config.EmptyRate_RandomGen, config.EmptyRate_RandomDist))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseRClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseRStartClickerDelay || LastTime + config.RStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.right = true;
						if (config.UseRPressInterval)
						apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right * 0.75f) : (ClickerDynamicDelay.right - config.RPressDuration);
						SetKeyClick(Click::Right, config.UseRPressInterval ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						EndSleep("RightClicker", ClickerDynamicDelay.right);
						goto label_rc_end;
					}
					else IsClickering.right = false;
				}
				else
					IsClickering.right = false;
			}
			//=========================================辅助模式=========================================
			if (config.RClickerMode & C_ASSISTMODE && !(config.RClickerMode & C_MOUSESMODE))
			{
				// 辅助模式没有连点间隙也没有延迟启动
				// 不使用HumanCPS是因为防止误关闭，门限CPS只需要判断开始的
				const bool IsClicker = CurrentCPS.right >= config.AssistModeCPSThreshold;
				const int ClickDealy = (1000 / max(HumanCPS.right, 1)) / max(config.RMultipleClickValue, 1);
				const bool IsClickKey = GetKeyClick(2, true, config.ReadKeyStateMode);
				if (!IsClicker && LastTime != 0)
					LastTime = 0;
				if (config.EnableRightClicker && ((AssistModeCount > 0) || (IsClickKey && IsClicker)))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if (!config.UseRStartClickerDelay || (LastTime + config.RStartClickerDelay) <= GetTickCount64())
					{
						if (IsClickKey)
							AssistModeCount += config.RMultipleClickValue;
						IsClickering.right = true;
						if (config.UseRPressInterval)
						apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right / 2) : (ClickerDynamicDelay.right - config.RPressDuration);
					SetKeyClick(Click::Right, config.UseRPressInterval ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						
						AssistModeCount--;
						EndSleep("RightClicker", ClickerDynamicDelay.right);
						goto label_rc_end;
					}
					else IsClickering.right = false;
				}
				else IsClickering.right = false;
			}
		label_rc_end:
			BeginSleep("RightClicker");
			if (!IsClickering.right || !config.EnableRightClicker || !SwitchMode_RClick)
				Sleep(config.MainClickSleep);
		}
	}
	PrintLog("[Clicker]右键连点线程退出", PL_INFO);
	return 0;
}

DWORD WINAPI LClickRun(LPVOID lpParam)
{
	while (!GCExit)
	{
		while (!IsClickerSuccess || StopClicker)
			Sleep(10);

		float ClickRates = (100 - (config.UseLEmptyRate ? config.LEmptyRateValue : 0)) / 100.0f;
		ULONGLONG LastTime = 0;
		ULONGLONG EndTime = GetTickCount64() + config.LCTime;
		ULONGLONG SlepTime = EndTime + config.LCSleep;
		bool IsEndTime = false;
		int AssistModeCount = 0;
		int apd = 0;

		while (!GCExit && !ReloadClicker && !StopClicker)
		{
			// 优先级处理
			if (config.ClickerPriorityMode == CP_RC && IsClickering.right)
			{
				Sleep(config.MainClickSleep);
				continue;
			}
			//=========================================默认模式=========================================
			if (config.LClickerMode & C_BUTTONMODE && !(config.LClickerMode & C_SWITCHMODE))
			{
				const bool IsKeyPressed = GetKeyDown(config.LKEY, config.UseHumanOnlyTrigger, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				bool IsDelayEnd = false;
				if (!IsKeyPressed)
					IsDelayEnd = false;

				if (!IsKeyPressed && LastTime != 0)
					LastTime = 0;

				if (config.UseLClickTime && IsDelayEnd)
				{
					if (GetTickCount64() >= EndTime && IsEndTime || IsRestTime)
					{
						SlepTime = GetTickCount64() + config.LCSleep;
						IsEndTime = false;
					}
					if (GetTickCount64() >= SlepTime && !IsEndTime || IsRestTime)
					{
						EndTime = GetTickCount64() + config.LCTime;
						IsEndTime = true;
					}
				}
				if (config.EnableLeftClicker && IsKeyPressed && RandomBool(ClickRates, config.EmptyRate_RandomGen, config.EmptyRate_RandomDist))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseLClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseLStartClickerDelay || LastTime + config.LStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.left = true;
						if (config.UseLPressInterval)
							apd = config.UseLAutoPressDuration ? (ClickerDynamicDelay.left / 2) : (ClickerDynamicDelay.left - config.LPressDuration);
						SetKeyClick(Click::Left, config.UseLPressInterval ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						EndSleep("LeftClicker", ClickerDynamicDelay.left);	
						EndSleep("LeftClicker", ClickerDynamicDelay.left);	
						goto label_lc_end;
					}
					else IsClickering.left = false;
				}
				else IsClickering.left = false;
			}
			//=========================================鼠标模式=========================================
			if (config.LClickerMode & C_MOUSESMODE && !(config.LClickerMode & C_ASSISTMODE) && GCHWND != GetForegroundWindow())
			{
				static bool IsDownMouse = false;
				const bool IsKeyPressed = GetKeyDown(1, true, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				bool IsDelayEnd = false;
				if (!IsKeyPressed)
					IsDelayEnd = false;

				if (!IsKeyPressed && LastTime != 0)
					LastTime = 0;

				if (config.UseLClickTime && IsDelayEnd)
				{
					if (GetTickCount64() >= EndTime && IsEndTime || IsRestTime)
					{
						SlepTime = GetTickCount64() + config.LCSleep;
						IsEndTime = false;
					}
					if (GetTickCount64() >= SlepTime && !IsEndTime || IsRestTime)
					{
						EndTime = GetTickCount64() + config.LCTime;
						IsEndTime = true;
					}
				}
				static bool backup_left = config.EnableLeftClicker;
				if ((!config.EnableLeftClicker || IsKeyPressed) && IsDownMouse)
				{
					SetKeyUp(Click::Left, config.SimulateClickMode);
					IsDownMouse = false;
				}

				if (config.EnableLeftClicker && IsKeyPressed && RandomBool(ClickRates, config.EmptyRate_RandomGen, config.EmptyRate_RandomDist))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if (  (!config.UseLClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && 
						  (!config.UseLStartClickerDelay || LastTime + config.LStartClickerDelay <= GetTickCount64()) &&
						  (!config.UseFilterLSingleClick || LastTime + FILTER_SC_SLEEP <= GetTickCount64())  )
					{
						IsClickering.left = true;
						if (config.UseLPressInterval)
							apd = config.UseLAutoPressDuration ? (ClickerDynamicDelay.left / 2) : (ClickerDynamicDelay.left - config.LPressDuration);
						if (config.UseItemWithClicker)
						{
							SetKeyDown(Click::Left, config.SimulateClickMode);
							IsDownMouse = true;
						}
						else
							SetKeyClick(Click::Left, config.UseLPressInterval ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						EndSleep("LeftClicker", ClickerDynamicDelay.left);
						goto label_lc_end;
					}
					else IsClickering.left = false;
				}
				else IsClickering.left = false;
			}
			//=========================================开关模式=========================================
			if (config.LClickerMode & C_SWITCHMODE && !(config.LClickerMode & C_BUTTONMODE))
			{
				const bool IsTrigger = SwitchMode_LClick;
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsTrigger;
				bool IsDelayEnd = false;
				if (!IsTrigger)
					IsDelayEnd = false;
				else if ((!config.UseLStartClickerDelay || LastTime + config.LStartClickerDelay <= GetTickCount64()) && !IsDelayEnd)
					IsDelayEnd = true;

				if (!IsTrigger && LastTime != 0)
					LastTime = 0;

				if (config.UseLClickTime && IsDelayEnd)
				{
					if (GetTickCount64() >= EndTime && IsEndTime || IsRestTime)
					{
						SlepTime = GetTickCount64() + config.LCSleep;
						IsEndTime = false;
					}
					if (GetTickCount64() >= SlepTime && !IsEndTime || IsRestTime)
					{
						EndTime = GetTickCount64() + config.LCTime;
						IsEndTime = true;
					}
				}
				if (GetKeyClick(config.LKEY, config.UseHumanOnlyTrigger, config.ReadKeyStateMode))
					SwitchMode_LClick = !SwitchMode_LClick;
				if (SwitchMode_LClick && config.EnableLeftClicker && RandomBool(ClickRates, config.EmptyRate_RandomGen, config.EmptyRate_RandomDist))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseLClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseLStartClickerDelay || LastTime + config.LStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.left = true;
						if (config.UseLPressInterval)
							apd = config.UseLAutoPressDuration ? (ClickerDynamicDelay.left / 2) : (ClickerDynamicDelay.left - config.LPressDuration);
						SetKeyClick(Click::Left, config.UseLPressInterval ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						EndSleep("LeftClicker", ClickerDynamicDelay.left);
						goto label_lc_end;
					}
					else IsClickering.left = false;
				}
				else IsClickering.left = false;
			}
			//=========================================辅助模式=========================================
			if (config.LClickerMode & C_ASSISTMODE && !(config.LClickerMode & C_MOUSESMODE))
			{
				// 辅助模式没有连点间隙也没有延迟启动
				const bool IsClicker = CurrentCPS.left >= config.AssistModeCPSThreshold;
				// ClickDealy的CPS保持在AssistModeMinCPS以上，防止CurrentCPS太低导致连点时间过长
				const int ClickDealy = (1000 / max(HumanCPS.left, 1)) / max(config.LMultipleClickValue, 1);
				const bool IsClickKey = GetKeyClick(1, config.UseHumanOnlyTrigger, config.ReadKeyStateMode);
				if (!IsClicker && LastTime != 0)
					LastTime = 0;
				if (config.EnableLeftClicker && ((AssistModeCount > 0) || (IsClickKey && IsClicker)))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if (!config.UseLStartClickerDelay || (LastTime + config.LStartClickerDelay) <= GetTickCount64())
					{
						if (IsClickKey)
							AssistModeCount += config.LMultipleClickValue;
						IsClickering.left = true;
						if (config.UseLPressInterval)
							apd = config.UseLAutoPressDuration ? (ClickDealy * 0.75f) : (ClickDealy - config.LPressDuration);
						SetKeyClick(Click::Left, config.UseLPressInterval ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						
						AssistModeCount--;
						EndSleep("LeftClicker", ClickDealy);
						goto label_lc_end;
					}
					else IsClickering.left = false;
				}
				else IsClickering.left = false;
			}
		label_lc_end:
			BeginSleep("LeftClicker");
			if (!config.EnableLeftClicker || !IsClickering.left || SwitchMode_LClick)
				Sleep(config.MainClickSleep);
		}
	}
	PrintLog("[Clicker]左键连点线程退出", PL_INFO);
	return 0;
}