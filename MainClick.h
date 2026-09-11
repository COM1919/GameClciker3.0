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

bool SwitchMode_LClick = false;// 开个模式的临时变量，仅仅用于开关模式的连点控制
bool SwitchMode_RClick = false;

DWORD WINAPI ClickerControl(LPVOID lpParam);
DWORD WINAPI MouseShakeMove(LPVOID lpParam);
DWORD WINAPI GameGuiThread(LPVOID lpParam);
POINT MouseShakeAlgorithm(POINT& view, POINT center);
inline void HandleClickerControl(int nCode, WPARAM wParam, LPARAM lParam); // 连点器滚轮逻辑分发处理器（由统一鼠标钩子 MouseHookProc 调用）
void BeginSleep(const char* flag);
void EndSleep(const char* flag, int time);
DWORD WINAPI RClickRun(LPVOID lpParam);
DWORD WINAPI LClickRun(LPVOID lpParam);

DWORD WINAPI ClickerControl(LPVOID lpParam)
{
	HANDLE RClickerThread = CreateThreadSafe(RClickRun, NULL, 0, "RClickRun");
	HANDLE LClickerThread = CreateThreadSafe(LClickRun, NULL, 0, "LClickRun");

	while (!IsGCExit)
	{
		// 修复：循环体持续读取 config（快捷键/CPS/模式等），持读锁保证与主线程 config 重置（写锁）互斥
		shared_lock<shared_mutex> cfgLock(ConfigAccessMutex);
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

		HWND CurrForegroundHwnd = GetForegroundWindow(); // 防止重复获取

		static HWND LastHwnd = NULL;
		if (LastHwnd == GCHWND && LastHwnd != CurrForegroundHwnd)
			IsReloadClicker = true;
		LastHwnd = CurrForegroundHwnd;

		if (IsReloadClicker)
		{
			IsClickerSuccess = false;
			IsReloadClicker = false;
			
			// 没什么用，保留就行
			srand(time(NULL));

			SaveConfig();
			RenderTip(config.UseChinese ? u8"已保存配置" : "Successfully saved config", 4.0f, PT_INFO);

			InitRandom();
			ClickerDynamicDelay.right = 1000.0f / static_cast<int>(max(config.RCPS, 0));
			ClickerDynamicDelay.left = 1000.0f / static_cast<int>(max(config.LCPS, 0));
			IsClickerSuccess = true;
			PrintLog("[Clicker] 重新加载连点成功", PL_INFO);
		}

		// 一些快捷键处理
		if (GetKeyClick(config.SetGameWindowKey))
		{
			GameHwnd = CurrForegroundHwnd;
			PrintLog("[Clicker] 已设置游戏窗口", PL_INFO);
		}
		if (GetKeyClick(config.HideClickerKey))
		{
			PrintLog("[Clicker] 已隐藏GC窗口", PL_INFO);
			ShowWindow(GCHWND, SW_HIDE);
		}
		if (CurrForegroundHwnd != GCHWND && config.SelfDestructionKey > 6 && GetKeyDown(config.SelfDestructionKey))
		{
			IsGCExit = true;
			PrintLog("[Clicker] 程序退出，用户按下了自毁键", PL_INFO);
			return 0;
		}

		// 统一处理IsOpenGui和IsMoving
		IsJumping = GetKeyDown(VK_SPACE);
		IsMoving = GetKeyDown('W') || GetKeyDown('S') || GetKeyDown('D') || GetKeyDown('A') || IsJumping;
		// GUI是否打开的检测
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

		{
			LRPair<vector<bool>> EnableFlags;
			// 仅移动连点
			if (config.UseROnlyMoveClicker)
				EnableFlags.right.push_back(IsMoving);
			if (config.UseLOnlyMoveClicker)
				EnableFlags.left.push_back(IsMoving);
			// 仅跳跃触发连点
			if (config.UseROnlyJumpClicker && !config.UseROnlyMoveClicker)
				EnableFlags.right.push_back(IsJumping);
			if (config.UseLOnlyJumpClicker && !config.UseLOnlyMoveClicker)
				EnableFlags.left.push_back(IsJumping);

			// GUI检测 - 注意和（仅移动连点 & 仅跳跃连点）的顺序，GUI检测权重要大，所以放这个后面
			if (config.UseGUIDetection)
			{
				EnableFlags.right.push_back(!IsOpenGui);
				EnableFlags.left.push_back(!IsOpenGui);
			}

			// 物品栏开关 & 物品栏预设CPS切换
			if (config.UseItembarSwitch)
			{
				Clamp(ItemBarPos, 0, 8);
				EnableFlags.right.push_back(config.ItemEnableClicker.right[ItemBarPos]);
				EnableFlags.left.push_back(config.ItemEnableClicker.left[ItemBarPos]);
				if (!config.UseBrutalMode && VarHasChanged(ItemBarPos, "ItemBarPos"))
				{
					if (config.ItemBarEnablePresetCPS.right[ItemBarPos])
						config.RCPS = config.ItemBarPresetCPS.right[ItemBarPos];
					if (config.ItemBarEnablePresetCPS.left[ItemBarPos])
						config.LCPS = config.ItemBarPresetCPS.left[ItemBarPos];

				}
			}
			// 过热保护
			if (config.UseOverheatProtec)
			{
				static ULONGLONG OverTime = GetTickCount64() + 100;
				static bool IsPlayedSound = false;
				if (GetTickCount64() >= OverTime)
				{
					if (IsClickering.right || IsClickering.left)
					{
						if (Overheat::Timer <= 0)
							Overheat::CooldownTimer = 0;
						if (Overheat::Timer >= config.OverheatTime)
						{
							EnableFlags.right.push_back(false);
							EnableFlags.left.push_back(false);
							{
								switch (config.OverheatSoundType)
								{
								case WARNING_NONE:
									break;
								default:
								case WARNING_SOUND:
								{
									if (!IsPlayedSound)
									{
										PlaySound(Sound_Warning.c_str(), NULL, SND_FILENAME | SND_ASYNC);
										IsPlayedSound = true;
									}
									break;
								}
								case WARNING_SYSTEM:
								{
									static ULONGLONG LastTime = 0;
									if (LastTime + 800 <= GetTickCount64())
									{
										MessageBeep(MB_ICONWARNING);
										LastTime = GetTickCount64();
									}
									break;
								}
								}
							}
						}
						else
							Overheat::Timer += 0.15f;
					}
					else
					{
						if (Overheat::Timer >= 0)
							Overheat::Timer = 0;
						if (Overheat::CooldownTimer <= config.OverheatCooldownTime)
							Overheat::CooldownTimer += 0.15f;
						if (Overheat::CooldownTimer >= config.OverheatCooldownTime)
						{
							Overheat::Timer = 0;
							Overheat::CooldownTimer = 0;
							EnableFlags.right.push_back(true);
							EnableFlags.left.push_back(true);
							IsPlayedSound = false;
							PlaySound(NULL, NULL, 0);
						}
					}
					OverTime = GetTickCount64() + 100;
				}
			}

			// 仅限游戏中连点
			if (config.UseInGameOnlyClicker)
			{
				const bool IsInGame = (CurrForegroundHwnd == GameHwnd);
				EnableFlags.right.push_back(IsInGame);
				EnableFlags.left.push_back(IsInGame);
			}

			if (config.AllowSetLClicker)
			{
				bool state = true;
				for (const bool& v : EnableFlags.left)
				{
					if (v == false) // 必须全部都是true，如果有一个false就说明需要关闭连点
						state = false;
				}
				if (!EnableFlags.left.empty())
					config.EnableLeftClicker = state;
			}
			if (config.AllowSetRClicker)
			{
				bool state = true;
				for (const bool& v : EnableFlags.right)
				{
					if (v == false) // 和上面同理
						state = false;
				}
				if (!EnableFlags.right.empty())
					config.EnableRightClicker = state;
			}

			// 强制开关连点 - 这个权重大于一切
			if (GetKeyClick(config.EnableRightClickerKey))
			{
				config.EnableRightClicker = !config.EnableRightClicker;
				if (config.EnableRightClicker) PlayEnableSound();
				else PlayDisableSound();
			}
			if (GetKeyClick(config.EnableLeftClickerKey))
			{
				config.EnableLeftClicker = !config.EnableLeftClicker;
				if (config.EnableLeftClicker) PlayEnableSound();
				else PlayDisableSound();
			}
		}
		
		// 物品栏切换的同步逻辑
		if (!IsOpenGui || (GCHWND == CurrForegroundHwnd)) //  GCHWND = CurrForegroundHwnd用于在GC设置里面耶可以通过快捷键切换，方便设置
		{
			for (int i = 0; i < 9; i++)
			{
				if (GetKeyClick(config.ItemBarKey[i]))
					ItemBarPos = i;
			}
		}

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
		/*
		static ULONGLONG LastRightTime = 0;
		if (GetTickCount64() - LastRightTime >= 900)
		{
			if (config.UseRFluctuation)
			{
				const int cps = Random(config.RCPS - config.RFluctuationCPS, config.RCPS + config.RFluctuationCPS, config.MouseShake_RandomGen, config.MouseShake_RandomDist);
				ClickerDynamicDelay.right = round(1000.0f / max(cps, 0));
				LastRightTime = GetTickCount64();
			}
			else
				ClickerDynamicDelay.right = round(1000.0f / max(config.RCPS, 0));
		}
		static ULONGLONG LastLeftTime = 0;
		if (GetTickCount64() - LastLeftTime >= 900)
		{
			if (config.UseLFluctuation)
			{
				const int cps = Random(config.LCPS - config.LFluctuationCPS, config.LCPS + config.LFluctuationCPS, config.MouseShake_RandomGen, config.MouseShake_RandomDist);
				ClickerDynamicDelay.left = round(1000.0f / max(cps, 0));
				LastLeftTime = GetTickCount64();
			}
			else
				ClickerDynamicDelay.left = round(1000.0f / max(config.LCPS, 0));
		}*/
		
		static LRPair<SmoothCPS> CPSSmooth;
		if (IsCPSWaveSettingChanged || VarHasChanged(config.LCPS, "LCPS", true))
		{
			CPSSmooth.left.BaseCPS = config.LCPS;
			CPSSmooth.left.MinCPS = config.LCPS - config.LFluctuationCPS;
			CPSSmooth.left.MaxCPS = config.LCPS + config.LFluctuationCPS;
			if (IsCPSWaveSettingChanged || CPSSmooth.left.GetWaveCount() <= 0)
				IsCPSWaveSettingChanged = true;
		}
		if (IsCPSWaveSettingChanged || VarHasChanged(config.RCPS, "RCPS", true))
		{
			CPSSmooth.right.BaseCPS = config.RCPS;
			CPSSmooth.right.MinCPS = config.RCPS - config.RFluctuationCPS;
			CPSSmooth.right.MaxCPS = config.RCPS + config.RFluctuationCPS;
			if (CPSSmooth.right.GetWaveCount() <= 0)
				IsCPSWaveSettingChanged = true;
		}
		if (IsCPSWaveSettingChanged)
		{
			CPSSmooth.left.SetWaveList(config.CPSWaves.left);
			CPSSmooth.right.SetWaveList(config.CPSWaves.right);
			IsCPSWaveSettingChanged = false;
		}

		if (IsClickering.right)
		{
			if (config.UseRFluctuation)
			{
				const float ResultCPS = CPSSmooth.right.CalcCPS();
				const int CPS = clamp(ResultCPS, config.RCPS - config.RFluctuationCPS, config.RCPS + config.RFluctuationCPS);
				ClickerDynamicDelay.right = int(1000.0f / CPS);
			}
			else if (VarHasChanged(config.RCPS, "config.LCPS", true))
				ClickerDynamicDelay.right = int(1000.0f / max(config.RCPS, 0));
		}
		if (IsClickering.left)
		{
			if (config.UseLFluctuation)
			{
				const float ResultCPS = CPSSmooth.left.CalcCPS();
				const int CPS = clamp(ResultCPS, config.LCPS - config.LFluctuationCPS, config.LCPS + config.LFluctuationCPS);
				ClickerDynamicDelay.left = int(1000.0f / CPS);
			}
			else if (VarHasChanged(config.LCPS, "config.LCPS", true))
				ClickerDynamicDelay.left = int(1000.0f / max(config.LCPS, 0));
		}

		// ===== 过热降频：连点持续达到过热点后，CPS 线性下降直到最低限制（停止连点即恢复）=====
		// 参考 CPS 波动的写法在 ClickerControl 主循环统一计算（连点线程只负责消费 ClickerDynamicDelay）。
		{
			// 用"最近点击时间"判断连点会话：IsClickering 是瞬时点击状态（空刀时为 false），
			// 不能直接用它计时，超过 IdleThreshold 无点击才视为会话结束。
			static ULONGLONG LastClickTime = 0;      // 最近一次实际点击的时间
			static ULONGLONG OverheatStartTime = 0;  // 当前连点会话的开始时间
			static const ULONGLONG IdleThreshold = 500;   // 超过该时长无点击 = 连点结束

			const bool IsClicking = IsClickering.right || IsClickering.left;
			if (IsClicking)
			{
				const ULONGLONG Now = GetTickCount64();
				if (Now - LastClickTime > IdleThreshold)   // 新的连点会话开始
					OverheatStartTime = Now;
				LastClickTime = Now;

				if (config.UseOverheatThrottle && config.Overheat_Time > 0.0f)
				{
					const float ElapsedSec = (Now - OverheatStartTime) / 1000.0f;
					if (ElapsedSec >= config.Overheat_Time)   // 到达过热点后才开始降频
					{
						// 在 Overheat_ThrottleTime 秒内线性降到 MinCPS（过热时优先于 CPS 波动）
						const float Throttle = min(1.0f, (ElapsedSec - config.Overheat_Time) / max(config.Overheat_ThrottleTime, 0.1f));
						const float RCurCPS = config.RCPS - (config.RCPS - config.Overheat_MinCPS) * Throttle;
						const float LCurCPS = config.LCPS - (config.LCPS - config.Overheat_MinCPS) * Throttle;
						ClickerDynamicDelay.right = 1000.0f / max(RCurCPS, 1.0f);
						ClickerDynamicDelay.left = 1000.0f / max(LCurCPS, 1.0f);
					}
				}
			}
		}

		// 拒绝默认模式下，快捷键为左/右键
		{
			if ((config.LClickerMode & C_BUTTONMODE || config.LClickerMode & C_SWITCHMODE) && config.LKEY == 1)
				config.LKEY = MAIN_L_KEY;
			if ((config.RClickerMode & C_BUTTONMODE || config.RClickerMode & C_SWITCHMODE) && config.RKEY == 2)
				config.RKEY = MAIN_R_KEY;
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

POINT MouseShakeAlgorithm(POINT& view, POINT center)
{
	RECT ShakeRect{};
	ShakeRect.left = center.x - config.MouseShakeRect;
	ShakeRect.top = center.y - config.MouseShakeRect;
	ShakeRect.right = center.x + config.MouseShakeRect;
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
			Random(min.x, max.x, GenerateGen_mersenne_twister, config.MouseShake_RandomDist),
			Random(min.y, max.y, GenerateGen_mersenne_twister, config.MouseShake_RandomDist)
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
			Random(ShakeRect.left, ShakeRect.right, GenerateGen_mersenne_twister, config.MouseShake_RandomDist),
			Random(ShakeRect.top, ShakeRect.bottom, GenerateGen_mersenne_twister, config.MouseShake_RandomDist)
		};
	}

	return MousePos;
}

DWORD WINAPI MouseShakeMove(LPVOID lpParam)
{
	POINT view{};
	FPOINT MousePos{};
	FPOINT AnimPos{};
	HWND hwnd = 0;
	bool IsMoving = false;

	const int ScreenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	const int ScreenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;

	while (!IsGCExit)
	{
		BeginSleep("MousShake");

		if (!config.UseMouseShake)
		{
			view = { 0, 0 };
			Sleep(100);
			continue;
		}

		static ULONGLONG LastTime = GetTickCount64();
		if ((
			(KeyStateMap[Click::Left].isDown && !KeyStateMap[Click::Left].isHumanDown) ||
			KeyStateMap[Click::Right].isDown && !KeyStateMap[Click::Right].isHumanDown) &&
			GetForegroundWindow() != GCHWND)
		{
			POINT pt{};
			GetCursorPos(&pt);
			if (!IsMoving)
				AnimPos = pt;
			// 不能使用相对坐标(设置中心为0)来一点鼠标，因为anime会导致移动多次，因为SendInput移动是相对位置
			MousePos = MouseShakeAlgorithm(view, pt);
			IsMoving = true;

			LastTime = GetTickCount64();
		}
		if (LastTime + 700 <= GetTickCount64())
			view = { 0, 0 };

		if (IsMoving)
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
					IsMoving = false;
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
				IsMoving = false;
			}
		}
		static const int sleep = round(1000 / 60);
		EndSleep("MousShake", sleep);
	}
	PrintLog("[Clicker] 鼠标抖动线程退出", PL_INFO);
	return 0;
}

inline void HandleClickerControl(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return;

	switch (wParam)
	{
		case WM_MOUSEWHEEL:
		{
			HWND hwnd = GetForegroundWindow();
			if (!IsOpenGui && hwnd != GCHWND && (GameHwnd == NULL || hwnd == GameHwnd))
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

DWORD WINAPI RClickRun(LPVOID lpParam)
{
	PreciseSleep ptime;
	while (!IsGCExit)
	{
		while (!IsClickerSuccess || IsStopClicker)
			Sleep(20);

		RandomBoolManager rbm;
		if (config.UseREmptyRate)
		{
			rbm.SetErrorRate(config.REmptyRateValue / 100.0f);
			rbm.SetMode((RandomBoolMode)config.EmptyRate_RandomMode);
			rbm.SetDistribution(config.EmptyRate_RandomDist);
		}
		ULONGLONG EndTime = GetTickCount64() + config.RCTime;
		ULONGLONG SlepTime = EndTime + config.RCSleep;
		bool IsEndTime = false;
		int apd = 0;

		while (!IsGCExit && !IsReloadClicker && !IsStopClicker)
		{
			// 修复：点击循环内持续读取 config（模式/CPS/延迟等），持读锁与主线程 config 重置（写锁）互斥。
			// 注意：不能把锁放在外层循环入口——IsStopClicker 时线程会长期停在等待循环（Sleep(20)），
			// 会把读锁占死导致重置线程饿死。因此只覆盖真正读 config 的点击循环。
			shared_lock<shared_mutex> cfgLock(ConfigAccessMutex);

			// 优先级处理
			if (config.ClickerPriorityMode == CP_LC && IsClickering.left)
			{
				Sleep(config.MainClickSleep);
				continue;
			}
			//=========================================默认模式=========================================
			if (config.RClickerMode & C_BUTTONMODE && !(config.RClickerMode & C_SWITCHMODE))
			{
				static ULONGLONG LastTime = 0;
				const bool IsKeyPressed = GetKeyDown(config.RKEY, config.UseHumanOnlyTrigger, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				// 连点间隔专属变量
				bool IsDelayEnd = false;
				if (!IsKeyPressed && LastTime != 0)
					LastTime = 0;

				if (!IsKeyPressed)
				{
					IsDelayEnd = false;
					// 修复：连点结束（按键释放）时重置 RandomBoolManager 疲劳/马尔可夫状态，模拟休息恢复。
					// 用触发源（按键）而非 IsClickering 判断——空刀时 IsClickering 也为 false，会误重置。
					rbm.ResetFatigue();
				}

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
				if (config.EnableRightClicker && IsKeyPressed && (!config.UseREmptyRate || rbm.rand()))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseRClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseRStartClickerDelay || LastTime + config.RStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.right = true;
						if (config.UseRClickPressTime)
							apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right / 2) : (ClickerDynamicDelay.right - config.RPressDuration);
						SetKeyClick(Click::Right, config.UseRClickPressTime ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						ptime.EndSleep(ClickerDynamicDelay.right);
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
				static ULONGLONG LastTime = 0;
				static bool IsStopedClicker = false;
				const bool IsKeyPressed = GetKeyDown(2, true, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				bool IsDelayEnd = false;
				if (!IsKeyPressed && LastTime != 0)
					LastTime = 0;

				if (!IsKeyPressed)
				{
					IsDelayEnd = false;
					// 连点结束（鼠标键释放）时重置疲劳状态（同按钮模式）
					rbm.ResetFatigue();
				}

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

				if (config.EnableRightClicker && IsKeyPressed && (!config.UseREmptyRate || rbm.rand()))
				{
					const ULONGLONG time = GetTickCount64();
					if (LastTime == 0)
						LastTime = time;
					if (  (!config.UseRClickTime || (IsDelayEnd && time < EndTime)) &&
						  (!config.UseRStartClickerDelay || (LastTime + config.RStartClickerDelay <= time)) &&
						  (!config.UseFilterRSingleClick || (LastTime + FILTER_SC_SLEEP <= time))  )
					{
						IsClickering.right = true;
						if (config.UseRClickPressTime)
							apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right / 2) : (ClickerDynamicDelay.right - config.RPressDuration);
						if (config.UseRClickerTrigInter)
						{
							if (config.UseOldMethodToTrigInter)
								SetKeyUp(Click::Right, config.SimulateClickMode);
							SetKeyDown(Click::Right, config.SimulateClickMode);
						}
						else
							SetKeyClick(Click::Right, config.UseRClickPressTime ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						IsStopedClicker = false;
						ptime.EndSleep(ClickerDynamicDelay.right);
						goto label_rc_end;
					}
					else IsClickering.right = false;
				}
				else
				{
					IsClickering.right = false;
					// 恢复鼠标原始状态
					if (!IsStopedClicker)
					{
						IsStopedClicker = true;
						if (IsKeyPressed) if (!config.UseRClickerTrigInter)
							SetKeyDown(Click::Right, config.SimulateClickMode);
						else SetKeyUp(Click::Right, config.SimulateClickMode);
					}
				}
			}
			//=========================================开关模式=========================================
			if (config.RClickerMode & C_SWITCHMODE && !(config.RClickerMode & C_BUTTONMODE))
			{
				// 这个模式特殊一点
				static ULONGLONG LastTime = 0;
				const bool IsTrigger = SwitchMode_RClick;
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsTrigger;
				bool IsDelayEnd = false;
				if (!IsTrigger && LastTime != 0)
					LastTime = 0;

				if (!IsTrigger)
				{
					IsDelayEnd = false;
					// 修复：连点结束（开关关闭）时重置疲劳状态（同按钮模式）
					rbm.ResetFatigue();
				}

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
				if (SwitchMode_RClick && config.EnableRightClicker && (!config.UseREmptyRate || rbm.rand()))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseRClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseRStartClickerDelay || LastTime + config.RStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.right = true;
						if (config.UseRClickPressTime)
						apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right * 0.75f) : (ClickerDynamicDelay.right - config.RPressDuration);
						SetKeyClick(Click::Right, config.UseRClickPressTime ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						ptime.EndSleep(ClickerDynamicDelay.right);
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
				static ULONGLONG LastTime = 0;
				static int AssistModeCount = 0;

				int HumanCPSCopy;
				{
					shared_lock<shared_mutex> lock(HumanCPSMutex);
					HumanCPSCopy = HumanCPS.right;
				}

				const bool IsClicker = HumanCPSCopy >= config.AssistModeCPSThreshold;
				// ClickDealy的CPS保持在AssistModeMinCPS以上，防止CurrentCPS太低导致连点时间过长
				const int ClickDealy = min(1000 / max(HumanCPSCopy, 1), 500) / max(config.RMultipleClickValue, 1);
				const bool IsClickKey = GetKeyClick(Click::Right, true, config.ReadKeyStateMode);

				if (!IsClicker && LastTime != 0)
					LastTime = 0;

				if (config.EnableRightClicker && IsClicker)
				{
					if (IsClickKey)
						AssistModeCount += config.RMultipleClickValue;
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((AssistModeCount > 0) && (!config.UseRStartClickerDelay || (LastTime + config.RStartClickerDelay) <= GetTickCount64()))
					{
						IsClickering.right = true;
						if (config.UseRClickPressTime)
							apd = config.UseRAutoPressDuration ? (ClickerDynamicDelay.right / 2) : (ClickerDynamicDelay.right - config.RPressDuration);
						SetKeyClick(Click::Right, config.UseRClickPressTime ? (config.UseRAutoPressDuration ? apd : config.RPressDuration) : 0, config.SimulateClickMode);
						
						AssistModeCount--;
						ptime.EndSleep(ClickDealy);
						goto label_rc_end;
					}
					else IsClickering.right = false;
				}
				else IsClickering.right = false;
			}
		label_rc_end:
			ptime.BeginSleep();
			if (!IsClickering.right || !config.EnableRightClicker || ((config.RClickerMode & C_SWITCHMODE) && !SwitchMode_RClick))
				ptime.Sleep(config.MainClickSleep);
		}
	}
	PrintLog("[Clicker] 右键连点线程退出", PL_INFO);
	return 0;
}

DWORD WINAPI LClickRun(LPVOID lpParam)
{
	PreciseSleep ptime;
	while (!IsGCExit)
	{
		while (!IsClickerSuccess || IsStopClicker)
			Sleep(10);

		RandomBoolManager rbm;
		if (config.UseLEmptyRate)
		{
			// 修复：原代码误用 config.REmptyRateValue（右键空刀概率）——左键连点应使用左键空刀概率
			rbm.SetErrorRate(config.LEmptyRateValue / 100.0f);
			rbm.SetMode((RandomBoolMode)config.EmptyRate_RandomMode);
			rbm.SetDistribution(config.EmptyRate_RandomDist);
		}

		ULONGLONG EndTime = GetTickCount64() + config.LCTime;
		ULONGLONG SlepTime = EndTime + config.LCSleep;
		bool IsEndTime = false;
		int apd = 0;

		while (!IsGCExit && !IsReloadClicker && !IsStopClicker)
		{
			// 修复：点击循环内持续读取 config，持读锁与主线程 config 重置（写锁）互斥（同 RClickRun）
			shared_lock<shared_mutex> cfgLock(ConfigAccessMutex);

			// 优先级处理
			if (config.ClickerPriorityMode == CP_RC && IsClickering.right)
			{
				Sleep(config.MainClickSleep);
				continue;
			}
			//=========================================默认模式=========================================
			if (config.LClickerMode & C_BUTTONMODE && !(config.LClickerMode & C_SWITCHMODE))
			{
				static ULONGLONG LastTime = 0;
				const bool IsKeyPressed = GetKeyDown(config.LKEY, config.UseHumanOnlyTrigger, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				bool IsDelayEnd = false;
				if (!IsKeyPressed)
				{
					IsDelayEnd = false;
					// 修复：连点结束（按键释放）时重置疲劳状态（同 RClickRun）
					rbm.ResetFatigue();
				}

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
				if (config.EnableLeftClicker && IsKeyPressed && (!config.UseLEmptyRate || rbm.rand()))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseLClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseLStartClickerDelay || LastTime + config.LStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.left = true;
						if (config.UseLClickPressTime)
							apd = config.UseLAutoPressDuration ? (ClickerDynamicDelay.left / 2) : (ClickerDynamicDelay.left - config.LPressDuration);
						SetKeyClick(Click::Left, config.UseLClickPressTime ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						ptime.EndSleep(ClickerDynamicDelay.left);
						goto label_lc_end;
					}
					else IsClickering.left = false;
				}
				else IsClickering.left = false;
			}
			//=========================================鼠标模式=========================================
			if (config.LClickerMode & C_MOUSESMODE && !(config.LClickerMode & C_ASSISTMODE) && GCHWND != GetForegroundWindow())
			{
				static ULONGLONG LastTime = 0;
				static bool IsStopedClicker = false;
				const bool IsKeyPressed = GetKeyDown(Click::Left, true, config.ReadKeyStateMode);
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsKeyPressed;
				bool IsDelayEnd = false;
				if (!IsKeyPressed)
				{
					IsDelayEnd = false;
					// 连点结束（鼠标键释放）时重置疲劳状态（同 RClickRun）
					rbm.ResetFatigue();
				}

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

				if (config.EnableLeftClicker && IsKeyPressed && (!config.UseLEmptyRate || rbm.rand()))
				{
					const ULONGLONG time = GetTickCount64();
					if (LastTime == 0)
						LastTime = time;
					if (  (!config.UseLClickTime || (IsDelayEnd && time < EndTime)) && 
						  (!config.UseLStartClickerDelay || (LastTime + config.LStartClickerDelay <= time)) &&
						  (!config.UseFilterLSingleClick || (LastTime + FILTER_SC_SLEEP <= time))  )
					{
						IsClickering.left = true;
						if (config.UseLClickPressTime)
							apd = config.UseLAutoPressDuration ? (ClickerDynamicDelay.left / 2) : (ClickerDynamicDelay.left - config.LPressDuration);
						if (config.UseLClickerTrigInter)
						{
							if (config.UseOldMethodToTrigInter)
								SetKeyUp(Click::Left, config.SimulateClickMode);
							SetKeyDown(Click::Left, config.SimulateClickMode);
						}
						else
							SetKeyClick(Click::Left, config.UseLClickPressTime ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						IsStopedClicker = false;
						ptime.EndSleep(ClickerDynamicDelay.left);
						goto label_lc_end;
					}
					else IsClickering.left = false;
				}
				else
				{
					IsClickering.left = false;
					// 恢复鼠标原始状态
					if (!IsStopedClicker)
					{
						IsStopedClicker = true;
						if (IsKeyPressed) if (!config.UseLClickerTrigInter) SetKeyDown(Click::Left, config.SimulateClickMode);
						else SetKeyUp(Click::Left, config.SimulateClickMode);
					}
				}
			}
			//=========================================开关模式=========================================
			if (config.LClickerMode & C_SWITCHMODE && !(config.LClickerMode & C_BUTTONMODE))
			{
				static ULONGLONG LastTime = 0;
				const bool IsTrigger = SwitchMode_LClick;
				const bool IsRestTime = config.UseRestTimeOnStopClicker && !IsTrigger;
				bool IsDelayEnd = false;
				if (!IsTrigger)
				{
					IsDelayEnd = false;
					// 修复：连点结束（开关关闭）时重置疲劳状态（同 RClickRun）
					rbm.ResetFatigue();
				}
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
				if (SwitchMode_LClick && config.EnableLeftClicker && (!config.UseLEmptyRate || rbm.rand()))
				{
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((!config.UseLClickTime || (IsDelayEnd && GetTickCount64() < EndTime)) && (!config.UseLStartClickerDelay || LastTime + config.LStartClickerDelay <= GetTickCount64()))
					{
						IsClickering.left = true;
						if (config.UseLClickPressTime)
							apd = config.UseLAutoPressDuration ? (ClickerDynamicDelay.left / 2) : (ClickerDynamicDelay.left - config.LPressDuration);
						SetKeyClick(Click::Left, config.UseLClickPressTime ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						ptime.EndSleep(ClickerDynamicDelay.left);
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
				static ULONGLONG LastTime = 0;
				static int AssistModeCount = 0;

				int HumanCPSCopy;
				{
					shared_lock<shared_mutex> lock(HumanCPSMutex);
					HumanCPSCopy = HumanCPS.left;
				}

				const bool IsClicker = HumanCPSCopy >= config.AssistModeCPSThreshold;
				// ClickDealy的CPS保持在AssistModeMinCPS以上，防止CurrentCPS太低导致连点时间过长
				const int ClickDealy = min(1000 / max(HumanCPSCopy, 1), 500) / max(config.LMultipleClickValue, 1);
				const bool IsClickKey = GetKeyClick(Click::Left, true, config.ReadKeyStateMode);
				
				if (!IsClicker && LastTime != 0)
					LastTime = 0;

				if (config.EnableLeftClicker && IsClicker)
				{
					if (IsClickKey)
						AssistModeCount += config.LMultipleClickValue;
					if (LastTime == 0)
						LastTime = GetTickCount64();
					if ((AssistModeCount > 0) && (!config.UseLStartClickerDelay || (LastTime + config.LStartClickerDelay) <= GetTickCount64()))
					{
						IsClickering.left = true;
						if (config.UseLClickPressTime)
							apd = config.UseLAutoPressDuration ? (ClickDealy * 0.75f) : max(ClickDealy - config.LPressDuration, 1);
						SetKeyClick(Click::Left, config.UseLClickPressTime ? (config.UseLAutoPressDuration ? apd : config.LPressDuration) : 0, config.SimulateClickMode);
						
						AssistModeCount--;
						ptime.EndSleep(ClickDealy);
						goto label_lc_end;
					}
					else IsClickering.left = false;
				}
				else IsClickering.left = false;
			}
		label_lc_end:
			ptime.BeginSleep();
			if (!IsClickering.left || !config.EnableLeftClicker || ((config.RClickerMode & C_SWITCHMODE) && !SwitchMode_LClick))
				ptime.Sleep(config.MainClickSleep);
		}
	}
	PrintLog("[Clicker] 左键连点线程退出", PL_INFO);
	return 0;
}