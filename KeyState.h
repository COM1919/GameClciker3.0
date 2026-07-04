#pragma once
#include "Definitons.h"

enum Click
{
	Right = VK_RBUTTON,
	Left = VK_LBUTTON
};
struct KeyState
{
	bool state;
	bool isInjected;
};
struct KeyEvent
{
	int vk;
	bool isDown;
	bool isInjected; // 人类点击
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
/*
using KeyStateCallback = void(*)(KeyEvent, void*);
struct KeyStateEntry
{
	void* instance;
	KeyStateCallback cb;
};
vector<KeyStateEntry> KeyStateCallbackList;

class KeyStateManager
{
private:
	array<KeyState, 256> isNowDown{};
	array<KeyState, 256> isClick{};
	array<KeyState, 256> isReleased{};

	void OnKeyEvent(int vk, bool isDown, bool isInjected)
	{
		if (vk < 0 || vk >= 256)
			return;

		KeyState prev = isNowDown[vk];
		isNowDown[vk].state = isDown;
		isNowDown[vk].isInjected = isInjected;
		bool injected = isInjected || prev.isInjected;
		if (isDown && !prev.state)
		{
			isClick[vk].isInjected = injected;
			isClick[vk].state = true;
		}
		if (!isDown && prev.state)
		{
			isReleased[vk].isInjected = injected;
			isReleased[vk].state = true;
		}
		cout << "asd" << endl;
	}
	static void StaticForward(KeyEvent e, void* instance)
	{
		((KeyStateManager*)instance)->OnKeyEvent(e.vk, e.isDown, e.isInjected);
	}
public:
	KeyStateManager()
	{
		KeyStateCallbackList.push_back({ this, &StaticForward });
	}
	~KeyStateManager()
	{
		auto it = find_if(KeyStateCallbackList.begin(), KeyStateCallbackList.end(), [&](KeyStateEntry& h) {
			return h.cb == StaticForward && h.instance == this;
			});
		if (it != KeyStateCallbackList.end())
			KeyStateCallbackList.erase(it);
	}

	inline bool IsVkInRange     (int vk)const{return vk >= 0         && vk;                                                 }
	bool        GetKeyDown      (int vk)const{return IsVkInRange(vk) && isNowDown  [vk].state;                              }
	bool        GetKeyClick     (int vk)const{return IsVkInRange(vk) && isClick    [vk].state;                              }
	bool        GetKeyUp        (int vk)const{return IsVkInRange(vk) && isReleased [vk].state;                              }

	bool        GetHumanKeyDown (int vk)const{return IsVkInRange(vk) && !isNowDown [vk].isInjected && isNowDown [vk].state;}
	bool        GetHumanKeyClick(int vk)const{return IsVkInRange(vk) && !isClick   [vk].isInjected && isClick   [vk].state;}
	bool        GetHumanKeyUp   (int vk)const{return IsVkInRange(vk) && !isReleased[vk].isInjected && isReleased[vk].state;}

	void UpdataState()
	{
		memset(isClick.data(), 0, isClick.size());
		memset(isReleased.data(), 0, isReleased.size());
		auto it = find_if(isClick.begin(), isClick.end(), [&](KeyState& h) {
			if (h.state)
				cout << "adfsvhijnadfsv" << endl;
			return h.state;
			});
	}
};*/
LRESULT CALLBACK KeyStateHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		/*
		KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
		KeyEvent ks;
		ks.isInjected = (pKeyBoard->flags & LLMHF_INJECTED) == 0; // 是否是人类按下
		ks.vk = pKeyBoard->vkCode;
		ks.isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
		for (auto it : KeyStateCallbackList)
		{
			if (it.cb)
				it.cb(ks, it.instance);
		}*/
		KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
		int vk = pKeyBoard->vkCode;
		if (vk > 0 && vk < 256)
		{
			KeyStateMap[vk].isInjected = (pKeyBoard->flags & LLMHF_INJECTED) == 0; // 是否是人类按下
			KeyStateMap[vk].state = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseStateHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		/*
		KeyEvent ks;
		switch (wParam)
		{
			case WM_LBUTTONDOWN: ks.vk = VK_LBUTTON; ks.isDown = true; break;
			case WM_RBUTTONDOWN: ks.vk = VK_RBUTTON; ks.isDown = true; break;
			case WM_MBUTTONDOWN: ks.vk = VK_MBUTTON; ks.isDown = true; break;
			case WM_LBUTTONUP:   ks.vk = VK_LBUTTON; ks.isDown = false; break;
			case WM_RBUTTONUP:   ks.vk = VK_RBUTTON; ks.isDown = false; break;
			case WM_MBUTTONUP:   ks.vk = VK_MBUTTON; ks.isDown = false; break;
		}
		if (ks.vk != -1)
		{
			MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
			ks.isInjected = (pMouseStruct->flags & LLMHF_INJECTED) == 0; // 是否是人类按下
			for (auto it : KeyStateCallbackList)
			{
				if (it.cb)
					it.cb(ks, it.instance);
			}
		}*/
		int vk = -1145;
		bool isDown = false;
		switch (wParam)
		{
			case WM_LBUTTONDOWN: vk = VK_LBUTTON; isDown = true; break;
			case WM_RBUTTONDOWN: vk = VK_RBUTTON; isDown = true; break;
			case WM_MBUTTONDOWN: vk = VK_MBUTTON; isDown = true; break;
			case WM_LBUTTONUP:   vk = VK_LBUTTON; isDown = false; break;
			case WM_RBUTTONUP:   vk = VK_RBUTTON; isDown = false; break;
			case WM_MBUTTONUP:   vk = VK_MBUTTON; isDown = false; break;
		}
		if (vk > 0 && vk < 256)
		{
			MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
			KeyStateMap[vk].isInjected = (pMouseStruct->flags & LLMHF_INJECTED) == 0; // 是否是人类按下
			KeyStateMap[vk].state = isDown;
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
		case GK_API: return GetAsyncKeyState(key) & 0x8000;
		case GK_HOOK:
			if (only_human)
				return KeyStateMap[key].state && KeyStateMap[key].isInjected;
			else
				return KeyStateMap[key].state;
			break;
	}
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
		if (key == Click::Left || key == Click::Right)
			mouse_event((key == Click::Left) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
		else
			keybd_event(key, 0, 0, 0);
		break;
	}
	default:
	case SC_SEND_INPUT:
	{
		INPUT input{};
		input.type = (key == Click::Left || key == Click::Right) ? INPUT_MOUSE : INPUT_KEYBOARD;

		input.ki.wVk = key;
		input.ki.dwFlags = 0;
		SendInput(1, &input, sizeof(INPUT));
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
		if (key == Click::Left || key == Click::Right)
			mouse_event((key == Click::Left) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
		else
			keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
		break;
	}
	default:
	case SC_SEND_INPUT:
	{
		INPUT input{};
		input.type = (key == Click::Left || key == Click::Right) ? INPUT_MOUSE : INPUT_KEYBOARD;

		input.ki.wVk = key;
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
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
				PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
			else
				PostMessage(hwnd, WM_RBUTTONUP, 0, MAKELPARAM(point.x, point.y));
		}
		else
			PostMessage(hwnd, WM_KEYUP, key, 0);
		break;
	}
	}
}

void SetKeyClick(int key, int sleep, int mode)
{
	if (sleep <= 0 && mode == SC_SEND_INPUT)
	{
		INPUT input[2]{};
		int type = (key == Click::Left || key == Click::Right) ? INPUT_MOUSE : INPUT_KEYBOARD;
		
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