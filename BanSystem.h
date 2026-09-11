#pragma once
#include "Network.h"
#include <sddl.h>

// 写入注册表函数
bool WriteToRegistry(const string& subKey, const string& valueName, const string& valueData)
{
    HKEY hKey;
    // 打开或创建注册表项
    LONG result = RegCreateKeyEx(
        HKEY_CURRENT_USER,                  // 根键
        subKey.c_str(),                     // 子键路径
        0,                                  // 保留（必须为0）
        NULL,                               // 默认安全描述符
        REG_OPTION_NON_VOLATILE,            // 持久化键
        KEY_WRITE,                          // 写入权限
        NULL,                               // 安全属性
        &hKey,                              // 返回的键句柄
        NULL                                // 键存在标志
    );

    if (result != ERROR_SUCCESS)
        return false;

    // 写入字符串值
    result = RegSetValueEx(
        hKey,                              // 键句柄
        valueName.c_str(),                 // 值名称
        0,                                 // 保留（必须为0）
        REG_SZ,                            // 数据类型（字符串）
        reinterpret_cast<const BYTE*>(valueData.c_str()), // 数据
        static_cast<DWORD>((valueData.length() + 1) * sizeof(wchar_t)) // 数据长度
    );

    RegCloseKey(hKey); // 关闭键句柄

    if (result != ERROR_SUCCESS)
        return false;

    return true;
}

void BanCurrentUser(time_t banTime, string reason)
{
    time_t UnbanTime = time(NULL) + banTime;
    string SubKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\Systems";
    string ValueName = "SentMaliciousObject"; //乱编的键名，为了隐蔽性
    string ValueData;

    if (banTime < 0)
        ValueData = -1;
    else
        ValueData = to_string(GetTickCount64() + UnbanTime * 1000);

    if (WriteToRegistry(SubKey, ValueName, ValueData))
        client.Send("return true");
    else
        client.Send("return false");

    PrintLog("用户已被封禁,ID:" + client_id, PL_WARNING);
    string msg = 
        UseChinese ? ("你已被禁止使用GC！\n原因："
        + reason
        + "\n解封时间："
        + FormatTime(UnbanTime)
        + "\n\nQQ群：188304966 | 915344208 | 752173087"
        + "\n作者QQ：3451392772"
        + "\n论坛：https://bbs.mcgc.top"):
        ("You have been banned from using GC!\nReason: "
        + reason
        + "\nUnban time: "
        + FormatTime(UnbanTime)
        + "\n\nQQ group: 188304966 | 915344208 | 752173087"
        + "\nAuthor QQ: 3451392772"
        + "\nForum: https://bbs.mcgc.top");
    MessageBox(NULL, msg.c_str(), UseChinese ? "GC连点器" : "GameClicker", MB_OK | MB_ICONWARNING);

    IsGCExit = true;
}