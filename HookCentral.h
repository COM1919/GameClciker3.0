#pragma once
#include "KeyState.h"
#include "MainClick.h"
#include "ClickSound.h"

#include <deque>
#include <mutex>
#include <condition_variable>
#include <ctime>

struct HookEventData
{
	int nCode = 0;
	WPARAM wParam = 0;
	bool isMouse = false;
	KBDLLHOOKSTRUCT kb{};   // 键盘钩子数据（回调返回后原指针失效，必须拷贝）
	MSLLHOOKSTRUCT ms{};    // 鼠标钩子数据（同上）
};

static std::deque<HookEventData> g_HookEventQueue;
static std::mutex g_HookEventMutex;
static std::condition_variable g_HookEventCV;

// 钩子回调入队（只拷贝数据 + 唤醒分发线程，不做任何业务逻辑）
inline void PushHookEvent(const HookEventData& ev)
{
	{
		std::lock_guard<std::mutex> lock(g_HookEventMutex);
		g_HookEventQueue.push_back(ev);
	}
	g_HookEventCV.notify_one();
}

// 分发线程入口：唯一执行各 Handle 的线程
DWORD WINAPI HookDispatchThread(LPVOID lpParam)
{
	while (!IsGCExit)
	{
		HookEventData ev;
		{
			std::unique_lock<std::mutex> lock(g_HookEventMutex);
			// 带超时等待：退出时没有人再投递钩子事件，无超时的 wait 会永久阻塞，
			// 导致退出流程每次都要等满 3 秒超时才强杀进程。
			g_HookEventCV.wait_for(lock, std::chrono::milliseconds(100), [] { return !g_HookEventQueue.empty() || IsGCExit; });
			if (g_HookEventQueue.empty())
				continue;   // GCExit 唤醒，直接退出
			ev = std::move(g_HookEventQueue.front());
			g_HookEventQueue.pop_front();
		}

		// 中央分发：同一种类型的多个 Handle 顺序执行（共享同一份事件数据副本）
		if (ev.isMouse)
		{
			LPARAM lp = (LPARAM)&ev.ms;
			HandleMouseState(ev.nCode, ev.wParam, lp);      // 鼠标按键 -> KeyStateMap 按键状态
			HandleMouseSound(ev.nCode, ev.wParam, lp);      // 鼠标按键 -> 鼠标点击声音
			HandleClickerControl(ev.nCode, ev.wParam, lp);  // 鼠标滚轮 -> 连点器物品栏切换
		}
		else
		{
			LPARAM lp = (LPARAM)&ev.kb;
			HandleKeyState(ev.nCode, ev.wParam, lp);   // 键盘按键 -> KeyStateMap 按键状态
			HandleKeybdSound(ev.nCode, ev.wParam, lp); // 键盘按键 -> 按键声音
		}
	}
	return 0;
}

// 统一鼠标钩子（WH_MOUSE_LL）：只入队，不处理
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		HookEventData ev;
		ev.nCode = nCode;
		ev.wParam = wParam;
		ev.isMouse = true;
		ev.ms = *(MSLLHOOKSTRUCT*)lParam;   // 拷贝：回调返回后 lParam 指向的缓冲区随即失效
		PushHookEvent(ev);
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// 统一键盘钩子（WH_KEYBOARD_LL）：只入队，不处理
LRESULT CALLBACK KeybdHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		HookEventData ev;
		ev.nCode = nCode;
		ev.wParam = wParam;
		ev.isMouse = false;
		ev.kb = *(KBDLLHOOKSTRUCT*)lParam;   // 拷贝：回调返回后 lParam 指向的缓冲区随即失效
		PushHookEvent(ev);
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}