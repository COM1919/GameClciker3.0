#pragma once
#include "Definitons.h"
#include "Log.h"
#include "PipeSystem.h"
#include "ShareCode.h"

#define GAMEGUI_ERROR -1
#define GAMEGUI_ALREADY 0
#define GAMEGUI_SUCCES 1

int GameGui_ErrorCode = 0;

DWORD WINAPI GameGuiThread(LPVOID lpParam)
{
	while (!GCExit)
	{
		if (config.UseEnableGameGui)
		{
			DWORD pid = 0;
			DWORD threadId = GetWindowThreadProcessId(GameHwnd, &pid);

			if (pid == NULL)
			{
				PrintLog("[GameGui]无法获取游戏窗口的进程PID，错误码:" + to_string(GetLastError()), PL_ERROR);
				config.UseEnableGameGui = false;
				continue;
			}

			AnonymousPipe pipe;
			if (pipe.CreatePipe())
			{
				PrintLog("[GameGui]无法创建通信(Error ID:2)", PL_ERROR);
				RenderTip(config.UseChinese ? u8"初始化通信失败" : "Init pipe failed", 2.0f, PT_ERROR);
				config.UseEnableGameGui = false;
				continue;
			}
			else
			{
				RenderTip(config.UseChinese ? u8"初始化通信成功" : "Init pipe success", 2.0f, PT_INFO);
				//pipe.Send();

				// 等待客户端连接
				//BOOL result = ConnectNamedPipe(hPipe, NULL);
				//if (!result)
				{
					PrintLog("[GameGui]等待客户端连接失败，错误码:" + to_string(GetLastError()), PL_ERROR);
					//CloseHandle(hPipe);
					return 2;
				}

				while (!GCExit)
				{
					GameGuiPipeData data;
					data.IsClickering.left = IsClickering.left;
					data.LMouseClickCount = app_status.MouseClickCount.left;
					data.RClickerClickCount = app_status.ClickerClickCount.right;
					data.LClickerClickCount = app_status.ClickerClickCount.left;
					data.RMouseClickCount = app_status.MouseClickCount.right;
					data.UseClickerTime = app_status.UseTime;
					data.StartTime = StartTime;
					data.RRecordsCPS = RRecordsCPS;
					data.LRecordsCPS = LRecordsCPS;
					{
						// 修复：防御性加读锁，避免与 RecordsPCRate 线程的写入并发
						shared_lock<shared_mutex> lock(PerformanceMutex);
						data.CpuRates = CpuRates;
						data.GpuRates = GpuRates;
						data.MemRates = MemRates;
					}
					{
						shared_lock<shared_mutex> lock(CurrentCPSMutex);
						data.RCPS = CurrentCPS.right;
						data.LCPS = CurrentCPS.left;
					}
					data.IsClickering = IsClickering;
					data.version = version;

					DWORD bytesWritten;
					//result = WriteFile(hPipe, &data, sizeof(data), &bytesWritten, NULL);

					//if (!result || bytesWritten != sizeof(data))
					{
						PrintLog("[GameGui]发送数据失败，错误码:" + to_string(GetLastError()), PL_ERROR);
						break;
					}
					Sleep(5);
				}
			}
		}
		Sleep(30);
	}
	return 0;
}