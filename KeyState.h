#pragma once
#include "Definitons.h"
#include "HelperCode.h"

enum Click
{
	Right = VK_RBUTTON,
	Left = VK_LBUTTON
};

struct KeyState // look版
{
	atomic<bool> isDown{ false };        // 总状态（物理+模拟）
	atomic<bool> isHumanDown{ false };   // 仅人类输入
};

array<KeyState, 256> KeyStateMap;
mutex ListLock;
LRPair<deque<ULONGLONG>> AllClickTimeList;
LRPair<deque<ULONGLONG>> HumanClickTimeList;
// 修复：AllClickTimeList/HumanClickTimeList 的 push_back/erase/[0] 分布在
// AddLeftClickCPS/AddRightClickCPS（连点线程+钩子分发线程调用）与
// HandleCPSRecord（独立线程 erase/pop_front）之间，原代码无锁 → deque 扩容时
// 另一线程迭代/访问即悬垂 → 0xC0000005。新增互斥锁保护。
shared_mutex AllClickTimeListMutex;
shared_mutex HumanClickTimeListMutex;
// 瞬时CPS：最近一次/上一次的实际点击时间戳（受 AllClickTimeListMutex 保护）。
// HandleCPSRecord 用"最近两次点击的间隔"推算瞬时CPS，与1秒窗口统计的 RRecordsCPS 形成对比曲线。
LRPair<ULONGLONG> LastClickStamp = {};
LRPair<ULONGLONG> PrevClickStamp = {};

inline void HandleMouseState(int nCode, WPARAM wParam, LPARAM lParam); // 鼠标按键状态分发处理器（由统一鼠标钩子 MouseHookProc 调用）
inline void HandleKeyState(int nCode, WPARAM wParam, LPARAM lParam);   // 键盘按键状态分发处理器（由统一键盘钩子 KeybdHookProc 调用）
DWORD WINAPI HandleCPSRecord(LPVOID lpParam);
void AddRightClickCPS();
void AddLeftClickCPS();
inline bool GetKeyDown(int key, bool only_human = false, int mode = GK_API);
inline bool GetKeyUp(int key, bool only_human = false, int mode = GK_API);
inline bool GetKeyClick(int key, bool only_human = false, int mode = GK_API);
inline void SetKeyDown(int key, int mode = SC_POST_MESSAGE, bool countStat = true);
inline void SetKeyUp(int key, int mode = SC_POST_MESSAGE);
inline void SetKeyClick(int key, int sleep = 20, int mode = SC_POST_MESSAGE);

void AddLeftClickCPS()
{
	{
		shared_lock lock(CurrentCPSMutex);
		CurrentCPS.left++;
	}
	{
		// 修复：push_back 与 HandleCPSRecord 的 erase/[0] 并发 → 加写锁
		lock_guard lock(AllClickTimeListMutex);
		const ULONGLONG now = GetNowMs();
		AllClickTimeList.left.push_back(now + 1000);
		// 同步维护"最近两次点击时间"，供 HandleCPSRecord 推算瞬时CPS
		PrevClickStamp.left = LastClickStamp.left;
		LastClickStamp.left = now;
	}
}

void AddRightClickCPS()
{
	{
		shared_lock lock(CurrentCPSMutex);
		CurrentCPS.right++;
	}
	{
		// 修复：同上
		lock_guard lock(AllClickTimeListMutex);
		const ULONGLONG now = GetNowMs();
		AllClickTimeList.right.push_back(now + 1000);
		PrevClickStamp.right = LastClickStamp.right;
		LastClickStamp.right = now;
	}
}

DWORD WINAPI HandleCPSRecord(LPVOID lpParam)
{
	ULONGLONG EndTime = GetNowMs() + 50;
	RRecordsCPS.reserve(MAX_RECORDS);
	LRecordsCPS.reserve(MAX_RECORDS);

	while (!IsGCExit)
	{
		// 统计实际人类点击的CPS，忽略连点器
		if (GetKeyClick(Click::Left, true))
		{
			app_status.MouseClickCount.left++;
			{
				unique_lock<shared_mutex> lock(HumanCPSMutex);
				HumanCPS.left++;
			}
			{
				// 修复：push_back 与下方 erase/[0] 并发 → 加写锁
				lock_guard lock(HumanClickTimeListMutex);
				HumanClickTimeList.left.push_back(GetNowMs() + 1000);
			}
			// 因为综合CPS包括人类CPS，所以需要在人类CPS处理的这里，加上综合CPS处理
			// 因为非人类CPS处理，是由SetKey...系列函数的内部处理
			AddLeftClickCPS();
		}
		if (GetKeyClick(Click::Right, true))
		{
			app_status.MouseClickCount.right++;
			{
				unique_lock<shared_mutex> lock(HumanCPSMutex);
				HumanCPS.right++;
			}
			{
				// 修复：同上
				lock_guard lock(HumanClickTimeListMutex);
				HumanClickTimeList.right.push_back(GetNowMs() + 1000);
			}
			// 上同
			AddRightClickCPS();
		}

		// 连点器CPS 衰减计算
		{
			// 修复：erase/[0] 与 AddLeft/RightClickCPS 的 push_back 并发 → 加写锁
			lock_guard lock(AllClickTimeListMutex);
			if (AllClickTimeList.left.size() > 0)
			{
				if (GetNowMs() >= AllClickTimeList.left[0])
				{
					{
						unique_lock<shared_mutex> lock2(CurrentCPSMutex);
						CurrentCPS.left--;
					}
					AllClickTimeList.left.erase(AllClickTimeList.left.begin());
				}
			}
			if (AllClickTimeList.right.size() > 0)
			{
				if (GetNowMs() >= AllClickTimeList.right[0])
				{
					{
						unique_lock<shared_mutex> lock2(CurrentCPSMutex);
						CurrentCPS.right--;
					}
					AllClickTimeList.right.erase(AllClickTimeList.right.begin());
				}
			}
		}
		// 人类CPS 衰减计算
		{
			// 修复：同上，erase/[0] 与 push_back 并发 → 加写锁
			lock_guard lock(HumanClickTimeListMutex);
			if (HumanClickTimeList.left.size() > 0)
			{
				if (GetNowMs() >= HumanClickTimeList.left[0])
				{
					{
						unique_lock<shared_mutex> lock2(HumanCPSMutex);
						HumanCPS.left--;
					}
					HumanClickTimeList.left.erase(HumanClickTimeList.left.begin());
				}
			}
			if (HumanClickTimeList.right.size() > 0)
			{
				if (GetNowMs() >= HumanClickTimeList.right[0])
				{
					{
						unique_lock<shared_mutex> lock2(HumanCPSMutex);
						HumanCPS.right--;
					}
					HumanClickTimeList.right.erase(HumanClickTimeList.right.begin());
				}
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
				if (RRecordsInstantCPS.size() >= MAX_RECORDS)
					RRecordsInstantCPS.erase(RRecordsInstantCPS.begin());
				if (LRecordsInstantCPS.size() >= MAX_RECORDS)
					LRecordsInstantCPS.erase(LRecordsInstantCPS.begin());

				{
					shared_lock lock(CurrentCPSMutex);
					RRecordsCPS.push_back(CurrentCPS.right);
					LRecordsCPS.push_back(CurrentCPS.left);
				}

				// 瞬时CPS：用"最近两次实际点击的间隔"推算，与上面的 1 秒窗口统计值同步记录，
				// 供"信息"页曲线对比（蓝色=正常CPS，橙色=瞬时CPS）。
				{
					const ULONGLONG now = GetNowMs();
					int rInstant = 0, lInstant = 0;
					lock_guard lock(AllClickTimeListMutex);
					// 有效前提：已有两次点击、且最近一次点击在 500ms 内（否则视为已停止，记 0）
					if (LastClickStamp.right > PrevClickStamp.right && (now - LastClickStamp.right) <= 500)
						rInstant = static_cast<int>(1000ULL / (LastClickStamp.right - PrevClickStamp.right));
					if (LastClickStamp.left > PrevClickStamp.left && (now - LastClickStamp.left) <= 500)
						lInstant = static_cast<int>(1000ULL / (LastClickStamp.left - PrevClickStamp.left));
					RRecordsInstantCPS.push_back(static_cast<float>(rInstant));
					LRecordsInstantCPS.push_back(static_cast<float>(lInstant));
				}
			}
			RecordsCPSMutex.unlock();

			EndTime = GetNowMs() + 50;
		}

		// 清理辅助模式短窗的过期项，避免无限增长。
		// 本线程每 10ms 跑一次，估算窗口（1000ms）外项在此被丢弃。
		// 该窗口必须 ≥ GetHumanCPSFast 的估算窗口，否则间隔样本不足会导致估值失真。
		{
			const ULONGLONG now = GetNowMs();
			const ULONGLONG minStamp = (now >= 1000) ? (now - 1000) : 0;
			lock_guard lock(FastHumanCPS.Mtx);
			while (!FastHumanCPS.Stamps.left.empty() && FastHumanCPS.Stamps.left.front() < minStamp)
				FastHumanCPS.Stamps.left.pop_front();
			while (!FastHumanCPS.Stamps.right.empty() && FastHumanCPS.Stamps.right.front() < minStamp)
				FastHumanCPS.Stamps.right.pop_front();
		}
		Sleep(10);
	}
	return 0;
}

static LPARAM MakeKeyboardLParam(UINT vk, bool IsKeyUp)
{
	LPARAM lParam = 0;
	UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);

	// 位 0-15: 重复计数（设为 1）
	lParam |= 0x00000001;
	// 位 16-23: 扫描码
	lParam |= (scanCode << 16);
	// 位 24: 扩展键标志（如果是扩展键，置 1）
	if (scanCode == 0x45 || // Insert
		scanCode == 0x46 || // Delete
		scanCode == 0x47 || // Home
		scanCode == 0x48 || // Up
		scanCode == 0x49 || // PageUp
		scanCode == 0x4B || // Left
		scanCode == 0x4D || // Right
		scanCode == 0x4F || // End
		scanCode == 0x50 || // Down
		scanCode == 0x51 || // PageDown
		vk == VK_RCONTROL ||// 右 Ctrl
		vk == VK_RMENU      // 右 Alt
		)
	{
		lParam |= 0x01000000; // 设置扩展键位
	}
	// 位 29: 上下文码（Alt 按下时置 1，一般设为 0）
	// 位 30: 原键状态（释放时为 1，按下时为 0）
	if (IsKeyUp)
	{
		lParam |= 0xC0000000; // 设置位 30 和 31（位31保留，但通常设置为1）
		// 更标准的写法：
		lParam |= (1 << 30);  // 原键状态为按下（释放前是按下的）
	}
	// 位 31: 保留，通常为 0，但某些消息中为 1，不影响
	return lParam;
}

inline void HandleKeyState(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return;

	KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
	int vk = pKeyBoard->vkCode;
	if (vk > 0 && vk < 256)
	{
		bool isDown = false;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
			isDown = true;
		else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
			isDown = false;
		else
			return;
		const bool isHumanInput = (pKeyBoard->flags & LLMHF_INJECTED) == 0;// 是否是人类按下
		KeyStateMap[vk].isDown.store(isDown, memory_order_relaxed);
		if (isHumanInput)// 如果不是人类点击的，就不设置isHumanDown。这样可以正确记录人类还没松开鼠标
			KeyStateMap[vk].isHumanDown.store(isDown, memory_order_relaxed); // 鼠标模式的BUG就是因为以前的代码，现在是正确的了
	}
}

inline void HandleMouseState(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return;

	int vk = -1145;
	bool isDown = false;
	switch (wParam)
	{
	case WM_LBUTTONDOWN: vk = VK_LBUTTON; isDown = true;  break;
	case WM_RBUTTONDOWN: vk = VK_RBUTTON; isDown = true;  break;
	case WM_MBUTTONDOWN: vk = VK_MBUTTON; isDown = true;  break;
	case WM_LBUTTONUP:   vk = VK_LBUTTON; isDown = false; break;
	case WM_RBUTTONUP:   vk = VK_RBUTTON; isDown = false; break;
	case WM_MBUTTONUP:   vk = VK_MBUTTON; isDown = false; break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	{
		MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
		if (HIWORD(pMouse->mouseData) == XBUTTON1) vk = VK_XBUTTON1;
		else if (HIWORD(pMouse->mouseData) == XBUTTON2) vk = VK_XBUTTON2;
		else break;
		isDown = (wParam == WM_XBUTTONDOWN);
		break;
	}
	}
	if (vk > 0 && vk < 256)
	{
		MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
		const bool IsInjected = (pMouseStruct->flags & LLMHF_INJECTED) == 0; // 是否是人类按下
		KeyStateMap[vk].isDown.store(isDown, memory_order_relaxed);
		if (IsInjected) // 如果不是人类点击的，就不设置isHumanDown。这样可以正确记录人类还没松开鼠标
		{
			KeyStateMap[vk].isHumanDown.store(isDown, memory_order_relaxed);
			// 辅助模式事件计数：钩子是事件驱动的，每次真实"按下"事件都 ++。
			// 这样辅助模式不再依赖 GetKeyClick 的采样频率（后者仍按电平边沿工作，
			// 仅用于"按住状态"等其他用途），彻底消除低频采样下的漏检。
			// 仅在"按下"事件（isDown=true）时计数，抬起不计，避免重复计数。
			if (isDown)
			{
				if (vk == Click::Left)       HumanPressCount.left.fetch_add(1, memory_order_relaxed);
				else if (vk == Click::Right) HumanPressCount.right.fetch_add(1, memory_order_relaxed);

				// 同时维护短窗时间戳队列（用于 GetHumanCPSFast 的 250ms 滑动窗口估计）
				const ULONGLONG now = GetNowMs();
				if (vk == Click::Left)
				{
					lock_guard lock(FastHumanCPS.Mtx);
					FastHumanCPS.Stamps.left.push_back(now);
				}
				else if (vk == Click::Right)
				{
					lock_guard lock(FastHumanCPS.Mtx);
					FastHumanCPS.Stamps.right.push_back(now);
				}
			}
		}
	}
}

// 辅助模式专用：原子读取并清零"人类按下事件计数"。
// 调用方（辅助模式循环）每次只消费"自上次消费以来"发生的真实按下次数。
// 返回值即被消费的事件数。GetKeyClick 的 API 行为不变。
inline int ConsumeHumanPresses(int click_vk)
{
	if (click_vk == Click::Left)  return HumanPressCount.left.exchange(0,  memory_order_acq_rel);
	if (click_vk == Click::Right) return HumanPressCount.right.exchange(0, memory_order_acq_rel);
	return 0;
}

// 辅助模式专用：用最近两次真实"按下"的间隔估算当前人类 CPS。
// 改为计算瞬时 CPS，判定门槛更贴合真实手速。
inline int GetHumanCPSFast(int click_vk)
{
	const ULONGLONG now = GetNowMs();
	const ULONGLONG windowMs = 1000;  // 估算窗口（毫秒），与 HandleCPSRecord 的修剪窗口一致
	const ULONGLONG minStamp = (now >= windowMs) ? (now - windowMs) : 0;

	lock_guard lock(FastHumanCPS.Mtx);
	const deque<ULONGLONG>& q = (click_vk == Click::Left) ? FastHumanCPS.Stamps.left : FastHumanCPS.Stamps.right;
	int& lastCPS = (click_vk == Click::Left) ? FastHumanCPS.LastCPS.left : FastHumanCPS.LastCPS.right;

	// q 递增有序，从队尾（最新）往前只需取"最近两次"样本
	ULONGLONG newest = 0, prev = 0;
	int n = 0;
	for (auto it = q.rbegin(); it != q.rend(); ++it)
	{
		if (*it < minStamp)
			break;
		if (n == 0)
			newest = *it;   // 最新一次按下
		else
			prev = *it;     // 次新一次按下
		++n;
		if (n >= 2)
			break;
	}

	// 长时间没有新按下就视为已停止，清空估值
	if (n == 0 || (now - newest) > 500)
	{
		lastCPS = 0;
		return 0;
	}
	// 仅 1 个样本：无法得到间隔，返回 0
	if (n < 2 || newest <= prev)
	{
		lastCPS = 0;
		return 0;
	}
	// 用最近两次点击的间隔估算瞬时 CPS
	lastCPS = static_cast<int>(1000ULL / (newest - prev));
	return lastCPS;
}

inline bool GetKeyClick(int key, bool only_human, int mode)
{
	if (key <= 0 || key >= 256)  // 虚拟键码有效范围 1~255
		return false;
	static bool thread_local PrevKeyState[256]{};
	static bool thread_local PrevKeyStateHuman[256]{};
	if (only_human)
	{
		bool CurrKeyState = GetKeyDown(key, true, mode);
		if (PrevKeyStateHuman[key] != CurrKeyState)
		{
			PrevKeyStateHuman[key] = CurrKeyState;
			return CurrKeyState;
		}
	}
	else
	{
		bool CurrKeyState = GetKeyDown(key, false, mode);
		if (PrevKeyState[key] != CurrKeyState)
		{
			PrevKeyState[key] = CurrKeyState;
			return CurrKeyState;
		}
	}
	return false;
}

inline bool GetKeyDown(int key, bool only_human, int mode)
{
	if (only_human) // 只有钩子才能区分人类输入
		return KeyStateMap[key].isHumanDown;
	// 非 only_human 时，根据 mode 选择 API 或钩子
	if (mode == GK_HOOK)
		return KeyStateMap[key].isDown;
	else // 当为 GK_BOTH 时，优先使用GK_API！
		return (GetAsyncKeyState(key) & 0x8000) != 0;
}

inline bool GetKeyUp(int key, bool only_human, int mode)
{
	if (key <= 0 || key >= 256)  // 虚拟键码有效范围 1~255
		return false;
	static thread_local bool PrevKeyState[256]{};
	static thread_local bool PrevKeyStateHuman[256]{};
	if (only_human)
	{
		bool CurrKeyState = GetKeyDown(key, true, mode);
		bool IsReleased = PrevKeyStateHuman[key] && !CurrKeyState;
		PrevKeyStateHuman[key] = CurrKeyState;
		return IsReleased;
	}
	else
	{
		bool CurrKeyState = GetKeyDown(key, false, mode);
		bool IsReleased = PrevKeyState[key] && !CurrKeyState;
		PrevKeyState[key] = CurrKeyState;
		return IsReleased;
	}
	return false;
}

inline void SetKeyDown(int key, int mode, bool countStat)
{
	// countStat=false 时只注入按键，不计入连点次数与 CPS（用于停止连点后恢复鼠标状态）
	if (countStat)
	{
		if (key == Click::Left)
		{
			app_status.ClickerClickCount.left++;
			AddLeftClickCPS();
		}
		else if (key == Click::Right)
		{
			app_status.ClickerClickCount.right++;
			AddRightClickCPS();
		}
	}

	switch (mode)
	{
	case SC_MOUSE_EVENT:
	{
		if (key < 7)
		{
			DWORD dwFlags = 0;
			switch (key)
			{
			case Click::Left:  dwFlags = MOUSEEVENTF_LEFTDOWN;   break;
			case Click::Right: dwFlags = MOUSEEVENTF_RIGHTDOWN;  break;
			case VK_MBUTTON:   dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
			default: return; // key无效，直接返回
			}
			mouse_event(dwFlags, 0, 0, 0, 0);
		}
		else
			keybd_event(key, 0, 0, 0);
		break;
	}
	default:
	case SC_SEND_INPUT:
	{
		if (key == VK_XBUTTON1 || key == VK_XBUTTON2)
		{
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_XDOWN;
			input.mi.mouseData = (key == VK_XBUTTON1) ? XBUTTON1 : XBUTTON2;
			SendInput(1, &input, sizeof(INPUT));
		}
		else if (key <= 4) // 鼠标中键、左右键
		{
			DWORD dwFlags = 0;
			switch (key)
			{
			case Click::Left:   dwFlags = MOUSEEVENTF_LEFTDOWN;   break;
			case Click::Right:  dwFlags = MOUSEEVENTF_RIGHTDOWN;  break;
			case VK_MBUTTON:    dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
			default: return;
			}
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = dwFlags;
			SendInput(1, &input, sizeof(INPUT));
		}
		else // 键盘
		{
			INPUT input = {};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			input.ki.dwFlags = 0; // 仅按下
			SendInput(1, &input, sizeof(INPUT));
		}
		break;
	}
	case SC_POST_MESSAGE:
	{
		HWND hwnd = GetForegroundWindow();
		if (!hwnd) return;

		if (key == Click::Left || key == Click::Right)
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			if (key == Click::Left)
				PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
			else
				PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(point.x, point.y));
			KeyStateMap[key].isDown.store(true, memory_order_relaxed);
		}
		else
			PostMessage(hwnd, WM_KEYDOWN, key, 0);
		break;
	}
	}
}

inline void SetKeyUp(int key, int mode)
{
	switch (mode)
	{
	case SC_MOUSE_EVENT:
	{
		if (key < 7)
		{
			DWORD dwFlags = 0;
			switch (key)
			{
			case Click::Left:   dwFlags = MOUSEEVENTF_LEFTUP;   break;
			case Click::Right:  dwFlags = MOUSEEVENTF_RIGHTUP;  break;
			case VK_MBUTTON:    dwFlags = MOUSEEVENTF_MIDDLEUP; break;
			default: return;
			}
			mouse_event(dwFlags, 0, 0, 0, 0);
		}
		else
			keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
		break;
	}
	default:
	case SC_SEND_INPUT:
	{
		if (key < 7)
		{
			DWORD dwFlags = 0;
			switch (key)
			{
			case Click::Left:   dwFlags = MOUSEEVENTF_LEFTUP;   break;
			case Click::Right:  dwFlags = MOUSEEVENTF_RIGHTUP;  break;
			case VK_MBUTTON:    dwFlags = MOUSEEVENTF_MIDDLEUP; break;
			default: return;
			}
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = dwFlags;
			SendInput(1, &input, sizeof(INPUT));
		}
		else  // 键盘释放
		{
			INPUT input = {};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = key;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
		}
		break;
	}
	case SC_POST_MESSAGE:
	{
		HWND hwnd = GetForegroundWindow();
		if (!hwnd)
			return;

		if (key == Click::Left || key == Click::Right)
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			PostMessage(hwnd, (key == Click::Left) ? WM_LBUTTONUP : WM_RBUTTONUP, 0, MAKELPARAM(point.x, point.y));
			KeyStateMap[key].isDown.store(false, memory_order_relaxed);
		}
		else
			PostMessage(hwnd, WM_KEYUP, key, MakeKeyboardLParam(key, true));
		break;
	}
	}
}

inline void SetKeyClick(int key, int sleep, int mode)
{
	switch (mode)
	{
	case SC_MOUSE_EVENT:
	{
		SetKeyDown(key, SC_MOUSE_EVENT); // 这个函数已经记录了鼠标按下的app status
		if (sleep > 0)
			Sleep(sleep);
		SetKeyUp(key, SC_MOUSE_EVENT);
		break;
	}
	default:
	case SC_SEND_INPUT:
	{
		if (key < 7) // 鼠标
		{
			if (key == Click::Left)
			{
				app_status.ClickerClickCount.left++;
				AddLeftClickCPS();
			}
			else if (key == Click::Right)
			{
				app_status.ClickerClickCount.right++;
				AddRightClickCPS();
			}
			INPUT inputs[2] = {};
			inputs[0].type = INPUT_MOUSE;
			inputs[1].type = INPUT_MOUSE;

			DWORD downFlag = 0, upFlag = 0;
			switch (key)
			{
			case Click::Left:  downFlag = MOUSEEVENTF_LEFTDOWN;   upFlag = MOUSEEVENTF_LEFTUP;   break;
			case Click::Right: downFlag = MOUSEEVENTF_RIGHTDOWN;  upFlag = MOUSEEVENTF_RIGHTUP;  break;
			case VK_MBUTTON:   downFlag = MOUSEEVENTF_MIDDLEDOWN; upFlag = MOUSEEVENTF_MIDDLEUP; break;
			default: return;
			}
			inputs[0].mi.dwFlags = downFlag;
			inputs[1].mi.dwFlags = upFlag;
			if (sleep < 0) // sleep 为 0 时，也分开调用，保证兼容性，如果sleep为负则合并调用，使用负数说明 调用者 明确了“合并调用”的态度
				SendInput(2, inputs, sizeof(INPUT));
			else
			{
				SendInput(1, &inputs[0], sizeof(INPUT));
				Sleep(sleep);
				SendInput(1, &inputs[1], sizeof(INPUT));
			}
		}
		else // 键盘
		{
			INPUT inputs[2] = {};
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = key;
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = key;
			inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
			if (sleep <= 0)
				SendInput(2, inputs, sizeof(INPUT));
			else
			{
				SendInput(1, &inputs[0], sizeof(INPUT));
				Sleep(sleep);
				SendInput(1, &inputs[1], sizeof(INPUT));
			}
		}
		break;
	}
	case SC_POST_MESSAGE:
	{
		HWND hwnd = GetForegroundWindow();
		if (!hwnd) return;
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(hwnd, &point);
		LPARAM lp = MAKELPARAM(point.x, point.y);

		if (key == Click::Left)
		{
			app_status.ClickerClickCount.left++;
			AddLeftClickCPS();

			KeyStateMap[Click::Left].isDown.store(true, memory_order_relaxed);
			PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp);
			if (sleep > 0)
				Sleep(sleep);
			KeyStateMap[Click::Left].isDown.store(false, memory_order_relaxed);
			PostMessage(hwnd, WM_LBUTTONUP, 0, lp);
		}
		else if (key == Click::Right)
		{
			app_status.ClickerClickCount.right++;
			AddRightClickCPS();

			KeyStateMap[Click::Right].isDown.store(true, memory_order_relaxed);
			PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, lp);
			if (sleep > 0)
				Sleep(sleep);
			KeyStateMap[Click::Right].isDown.store(false, memory_order_relaxed);
			PostMessage(hwnd, WM_RBUTTONUP, 0, lp);
		}
		else if (key == VK_XBUTTON1 || key == VK_XBUTTON2)
		{
			DWORD xbutton = (key == VK_XBUTTON1) ? XBUTTON1 : XBUTTON2;
			PostMessage(hwnd, WM_XBUTTONDOWN, MAKEWPARAM(MK_XBUTTON1, xbutton), lp);
			if (sleep > 0)
				Sleep(sleep);
			PostMessage(hwnd, WM_XBUTTONUP, MAKEWPARAM(MK_XBUTTON1, xbutton), lp);
			break;
		}
		else
		{
			PostMessage(hwnd, WM_KEYDOWN, key, MakeKeyboardLParam(key, false));
			if (sleep > 0)
				Sleep(sleep);
			PostMessage(hwnd, WM_KEYUP, key, MakeKeyboardLParam(key, true));
		}
		break;
	}
	}
}