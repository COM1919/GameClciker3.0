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
	bool WasEmpty = false;
	{
		std::lock_guard<std::mutex> lock(g_HookEventMutex);
		// 只有"队列由空变非空"时才唤醒分发线程。高频输入时事件往往在上一条
		// 尚未处理完就到齐，此时分发线程没有睡，notify_one 纯属多余的系统调用。
		WasEmpty = g_HookEventQueue.empty();
		g_HookEventQueue.push_back(ev);
	}
	if (WasEmpty)
		g_HookEventCV.notify_one();
}

// 分发线程入口：唯一执行各 Handle 的线程
DWORD WINAPI HookDispatchThread(LPVOID lpParam)
{
	// 输入事件的业务处理（按键状态/CPS/连点控制）对延迟敏感，
	// 略高于普通线程可避免在高负载时被渲染/脚本线程长时间挤出 CPU。
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

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
	if (nCode < 0)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

	// 白名单：只有这些消息参与业务（按键状态 / 点击声音 / 滚轮切物品栏）。
	//
	// 关键性能点：WM_MOUSEMOVE 对连点器毫无用途，但 WH_MOUSE_LL 会对每一个硬件鼠标
	// 事件回调一次。高 DPI（1000+ 报告率）鼠标每秒可产生上千次回调，而低级钩子的
	// 回调是串行运行在系统输入线程（RIT）上的——RIT 必须等本回调返回才继续分发
	// 后续的鼠标/键盘事件。旧代码把每条移动事件都拷贝进队列，使 RIT 长期被占住：
	//   * 游戏收到的原始鼠标事件密度下降 -> 视角灵敏度异常、移动卡顿；
	//   * 键盘事件同样排在 RIT 后面 -> 按键偶发不触发（如 Caps Lock/Shift 丢失，
	//     表现为大小写错乱）；
	//   * GC 自己的 UI 点击消息被拖后 -> 点击后很久才响应。
	// 因此在任何拷贝/加锁之前直接放行，回调耗时降到常数级（约一次 switch）。
	switch (wParam)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEWHEEL:
		break;
	default:
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	HookEventData ev;
	ev.nCode = nCode;
	ev.wParam = wParam;
	ev.isMouse = true;
	ev.ms = *(MSLLHOOKSTRUCT*)lParam;   // 拷贝：回调返回后 lParam 指向的缓冲区随即失效
	PushHookEvent(ev);
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// 统一键盘钩子（WH_KEYBOARD_LL）：只入队，不处理
LRESULT CALLBACK KeybdHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

	// 同上：只关心按下/抬起，其余消息直接放行，避免无谓的拷贝与入队
	if (wParam != WM_KEYDOWN && wParam != WM_KEYUP &&
		wParam != WM_SYSKEYDOWN && wParam != WM_SYSKEYUP)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

	HookEventData ev;
	ev.nCode = nCode;
	ev.wParam = wParam;
	ev.isMouse = false;
	ev.kb = *(KBDLLHOOKSTRUCT*)lParam;   // 拷贝：回调返回后 lParam 指向的缓冲区随即失效
	PushHookEvent(ev);
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}