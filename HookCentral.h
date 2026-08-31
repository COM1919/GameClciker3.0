#pragma once
#include "KeyState.h"    // HandleKeyState / HandleMouseState
#include "MainClick.h"   // HandleClickerControl
#include "MouseSound.h"  // HandleKeybdSound / HandleMouseSound

#include <deque>
#include <mutex>
#include <condition_variable>
#include <ctime>

// ============================================================
// 中央分发钩子（Hook 合并 + 异步分发）
// ------------------------------------------------------------
// 钩子线程（WH_MOUSE_LL / WH_KEYBOARD_LL 回调）只做两件事：
// 1) 把事件数据（拷贝！lParam 指针仅在回调期间有效）入队；
// 2) 立即返回并 CallNextHookEx。
// 真正的业务处理（各 Handle）全部由 HookDispatchThread 这一个
// 线程统一执行 —— 避免在低级钩子回调里做耗时工作（系统对钩子
// 回调有超时限制，超时会被静默移除），同时所有 Handle 串行执行
// 于同一线程内（同一线程内天然互斥，配合“同一线程”的要求）。
//
// 分发关系：
//   鼠标事件 -> HandleMouseState + HandleMouseSound + HandleClickerControl
//   键盘事件 -> HandleKeyState  + HandleKeybdSound
//
// 说明：单线程内无法真正并行执行两个 Handle；若后续需要并行，
// 可改为每类 Handle 一个独立消费者线程（但声音类 Handle 会并发
// 访问 OpenAL，需额外串行化），当前实现按“同一线程 + 异步”执行。
// ============================================================

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
	srand((unsigned)time(NULL));   // 声音随机音量/音调种子
	while (!GCExit)
	{
		HookEventData ev;
		{
			std::unique_lock<std::mutex> lock(g_HookEventMutex);
			g_HookEventCV.wait(lock, [] { return !g_HookEventQueue.empty() || GCExit; });
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