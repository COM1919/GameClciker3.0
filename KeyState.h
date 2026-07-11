#pragma once
#include "Definitons.h"

enum Click
{
	Right = VK_RBUTTON,
	Left = VK_LBUTTON
};
struct KeyState
{
	bool isDown;
	bool isHumanDown;
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

LRESULT CALLBACK KeyStateHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
		int vk = pKeyBoard->vkCode;
		if (vk > 0 && vk < 256)
		{
			const bool IsInjected = (pKeyBoard->flags & LLMHF_INJECTED) == 0;// 是否是人类按下
			const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
			if (IsInjected) // 如果不是人类点击的，就不设置isHumanDown。这样可以正确记录人类还没松开鼠标
				KeyStateMap[vk].isHumanDown = isDown; // 鼠标模式的BUG就是因为以前的代码，现在是正确的了
			KeyStateMap[vk].isDown = isDown;
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
		}
		if (vk > 0 && vk < 256)
		{
			MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
			const bool IsInjected = (pMouseStruct->flags & LLMHF_INJECTED) == 0; // 是否是人类按下
			if (IsInjected) // 如果不是人类点击的，就不设置isHumanDown。这样可以正确记录人类还没松开鼠标
				KeyStateMap[vk].isHumanDown = isDown;
			KeyStateMap[vk].isDown = isDown;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

bool GetKeyClick(int key, bool only_human, int mode)
{
	if (key <= 0 && key > 256)
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
	if (mode == GK_BOTH)
		mode = only_human ? GK_HOOK : GK_API;
	if (only_human)
		mode = GK_HOOK;
	switch (mode)
	{
		case GK_API:
			return GetAsyncKeyState(key) & 0x8000;
		case GK_HOOK:
			if (only_human)
				return KeyStateMap[key].isHumanDown;
			else
				return KeyStateMap[key].isDown;
	}
	return false;
}

bool GetKeyUp(int key, bool only_human, int mode)
{
	if (key <= 0)
		return false;
	static thread_local bool PrevKeyState[512] = { false };
	static thread_local bool PrevKeyStateHuman[512] = { false };
	if (only_human)
	{
		bool CurrKeyState = GetKeyDown(key, only_human, mode);
		bool IsReleased = PrevKeyStateHuman[key] && !CurrKeyState;
		PrevKeyStateHuman[key] = CurrKeyState;
		return IsReleased;
	}
	else
	{
		bool CurrKeyState = GetKeyDown(key, only_human, mode);
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
			case Click::Left: dwFlags = MOUSEEVENTF_LEFTDOWN; break;
			case Click::Right: dwFlags = MOUSEEVENTF_RIGHTDOWN; break;
			case VK_MBUTTON: dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
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
		if (key < 7) // 鼠标
		{
			DWORD dwFlags = 0;
			switch (key)
			{
			case Click::Left:   dwFlags = MOUSEEVENTF_LEFTDOWN; break;
			case Click::Right:  dwFlags = MOUSEEVENTF_RIGHTDOWN; break;
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
			INPUT inputs[2] = {};
			// 按下
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = key;
			// 释放（如果你需要独立释放，可把释放放在 SetKeyUp 中）
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = key;
			inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(2, inputs, sizeof(INPUT));
		}
		break;
	}
	case SC_POST_MESSAGE:
	{
		HWND hwnd = GetForegroundWindow();
		if (key == Click::Left || key == Click::Right)
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			if (key == Click::Left)
				PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(point.x, point.y));
			else
				PostMessage(hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(point.x, point.y));
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
		{
			keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
		}
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
		if (!hwnd) return;

		if (key == Click::Left || key == Click::Right)
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			UINT msg = (key == Click::Left) ? WM_LBUTTONUP : WM_RBUTTONUP;
			PostMessage(hwnd, msg, 0, MAKELPARAM(point.x, point.y));
		}
		else  // 键盘释放
		{
			// 构造标准 WM_KEYUP 的 lParam
			LPARAM lParam = 0;
			// bit 29-24: 扫描码（可用 MapVirtualKey 获取）
			UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);
			lParam |= (scanCode << 16);
			// bit 30: 原键状态（释放前处于按下状态，设为 1）
			lParam |= (1 << 30);
			// bit 31: 上下文码（一般为 0）
			// 其他位暂不设置（扩展键需要额外判断，此处简化）
			PostMessage(hwnd, WM_KEYUP, key, lParam);
		}
		break;
	}
	}
}

void SetKeyClick(int key, int sleep, int mode)
{
	if (sleep <= 0 && mode == SC_SEND_INPUT)
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
		INPUT input[2]{};
		int type = (key < 7) ? INPUT_MOUSE : INPUT_KEYBOARD;
		
		input[0].type = type;
		input[0].ki.wVk = key;
		input[0].ki.dwFlags = KEYEVENTF_KEYUP;

		input[1].type = type;
		input[1].ki.wVk = key;
		input[1].ki.dwFlags = 0;
		SendInput(2, input, sizeof(INPUT));
	}
	else
	{
		SetKeyDown(key, mode);
		if (sleep > 0)
			Sleep(sleep);
		SetKeyUp(key, mode);
	}
}