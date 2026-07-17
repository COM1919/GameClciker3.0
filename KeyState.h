#pragma once
#include "Definitons.h"

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

LRESULT CALLBACK MouseStateHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyStateHookProc(int nCode, WPARAM wParam, LPARAM lParam);
bool GetKeyDown(int key, bool only_human = false, int mode = GK_API);
bool GetKeyUp(int key, bool only_human = false, int mode = GK_API);
bool GetKeyClick(int key, bool only_human = false, int mode = GK_API);
void SetKeyDown(int key, int mode = SC_POST_MESSAGE);
void SetKeyUp(int key, int mode = SC_POST_MESSAGE);
void SetKeyClick(int key, int sleep = 20, int mode = SC_POST_MESSAGE);

// 添加一个工具函数来生成正确的键盘 lParam     - 工具函数
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

LRESULT CALLBACK KeyStateHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
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
				return CallNextHookEx(NULL, nCode, wParam, lParam);
			const bool isHumanInput = (pKeyBoard->flags & LLMHF_INJECTED) == 0;// 是否是人类按下
			KeyStateMap[vk].isDown.store(isDown, memory_order_relaxed);
			if (isHumanInput)// 如果不是人类点击的，就不设置isHumanDown。这样可以正确记录人类还没松开鼠标
				KeyStateMap[vk].isHumanDown.store(isDown, memory_order_relaxed); // 鼠标模式的BUG就是因为以前的代码，现在是正确的了
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseStateHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
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
				KeyStateMap[vk].isHumanDown.store(isDown, memory_order_relaxed);
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

bool GetKeyClick(int key, bool only_human, int mode)
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

bool GetKeyDown(int key, bool only_human, int mode)
{
	if (only_human) // 只有钩子才能区分人类输入
		return KeyStateMap[key].isHumanDown;
	// 非 only_human 时，根据 mode 选择 API 或钩子
	if (mode == GK_HOOK)
		return KeyStateMap[key].isDown;
	else // 当为 GK_BOTH 时，优先使用GK_API！
		return (GetAsyncKeyState(key) & 0x8000) != 0;
}

bool GetKeyUp(int key, bool only_human, int mode)
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

void SetKeyDown(int key, int mode)
{
	if (key == Click::Left)
	{
		app_status.ClickerClickCount.left++;
		IsClickerDown.left = true;
	}
	else if (key == Click::Right)
	{
		app_status.ClickerClickCount.right++;
		IsClickerDown.right = true;
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
void SetKeyUp(int key, int mode)
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

void SetKeyClick(int key, int sleep, int mode)
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
				IsClickerDown.left = true;
			}
			else if (key == Click::Right)
			{
				app_status.ClickerClickCount.right++;
				IsClickerDown.right = true;
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
			IsClickerDown.left = true;

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
			IsClickerDown.right = true;

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