#Requires -Version 5.1
<#
.SYNOPSIS
    安装（或更新）未签名的 GC连点器 MSIX 包，用于本地功能测试。

.DESCRIPTION
    未签名的 MSIX 需要「开发人员模式」才能安装。本脚本会自动：
      1. 定位 output 目录下最新的 .msix（也可用 -PackagePath 指定具体文件）；
      2. 若未开启开发人员模式，则请求管理员权限（UAC）自动开启；
      3. 卸载同名旧版本后安装新包（Add-AppxPackage -AllowUnsigned）。
    直接双击同目录的 install-unsigned.cmd 即可运行。

.PARAMETER PackagePath
    要安装的 .msix 路径；缺省时使用 output 目录下最新的 .msix。

.PARAMETER KeepData
    保留已安装版本的数据（默认会先卸载旧版本，以便干净地重装）。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\install-unsigned.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\install-unsigned.ps1 -PackagePath ..\output\GameClciker3.0_1.0.0.0_x64.msix
#>
[CmdletBinding()]
param(
    [string]$PackagePath,
    [switch]$KeepData
)

$ErrorActionPreference = 'Stop'

$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$OutputDir   = Join-Path $ScriptDir 'output'
$PackageName = 'Euxstudio.GameClicker_gwer11b85jr02'   # Package Family Name
$ApplicationId = 'GCGameClicker'                       # AppxManifest.xml 中的 Application Id

function Test-DeveloperMode {
    $key = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock' -ErrorAction SilentlyContinue
    return [bool]($key -and ($key.AllowDevelopmentWithoutDevLicense -eq 1 -or $key.AllowAllTrustedApps -eq 1))
}

function Enable-DeveloperMode {
    Write-Host "未开启「开发人员模式」，正在请求管理员权限开启（将出现 UAC 提示）..." -ForegroundColor Yellow
    $inner = @'
New-Item -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock' -Force | Out-Null
New-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock' -Name AllowDevelopmentWithoutDevLicense -PropertyType DWORD -Value 1 -Force | Out-Null
'@
    $tmp = Join-Path $env:TEMP 'gc_enable_devmode.ps1'
    [System.IO.File]::WriteAllText($tmp, $inner, (New-Object System.Text.UTF8Encoding($false)))
    Start-Process powershell -Verb RunAs -Wait -ArgumentList '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$tmp`""
    Remove-Item $tmp -Force -ErrorAction SilentlyContinue
    if (-not (Test-DeveloperMode)) {
        throw "开发人员模式开启失败。请手动在「设置 → 系统 → 开发者选项」中开启「开发人员模式」后重试。"
    }
    Write-Host "开发人员模式已开启。" -ForegroundColor Green
}

# ---------- 定位安装包 ----------
if (-not $PackagePath) {
    $latest = Get-ChildItem $OutputDir -Filter '*.msix' -File -ErrorAction SilentlyContinue |
              Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $latest) { throw "未在 $OutputDir 找到 .msix，请先运行 build-msix.cmd 打包。" }
    $PackagePath = $latest.FullName
}
if (-not (Test-Path $PackagePath)) { throw "找不到安装包：$PackagePath" }
$PackagePath = (Resolve-Path $PackagePath).Path

Write-Host "==============================================" -ForegroundColor Green
Write-Host " 安装 GC连点器（未签名包，用于本地测试）" -ForegroundColor Green
Write-Host "==============================================" -ForegroundColor Green
Write-Host " 安装包：$PackagePath"
Write-Host " 大小  ：$([Math]::Round((Get-Item $PackagePath).Length / 1MB, 2)) MB"

# ---------- 开发人员模式 ----------
if (-not (Test-DeveloperMode)) { Enable-DeveloperMode }

# ---------- 卸载旧版本 ----------
if (-not $KeepData) {
    $old = Get-AppxPackage -Name $PackageName -ErrorAction SilentlyContinue
    if ($old) {
        Write-Host "卸载旧版本 $($old.Version) ..."
        $old | Remove-AppxPackage -ErrorAction SilentlyContinue
    }
}

# ---------- 安装 ----------
Write-Host "正在安装..." -ForegroundColor Cyan
try {
    Add-AppxPackage -Path $PackagePath -AllowUnsigned -ForceApplicationShutdown
} catch {
    Write-Warning "安装失败：$($_.Exception.Message)"
    Write-Warning "若提示版本/签名冲突，可先执行：Remove-AppxPackage -Package $PackageName"
    Write-Warning "若提示缺少 Microsoft.VCLibs.140.00.UWPDesktop 框架包（错误 0x80073CF3），请先安装该框架包再重试。"
    throw
}

$installed = Get-AppxPackage -Name $PackageName -ErrorAction SilentlyContinue
Write-Host ""
if ($installed) {
    Write-Host "安装成功：$($installed.PackageFullName)" -ForegroundColor Green
    Write-Host "启动方式（开始菜单搜索「GC连点器」，或执行）：" -ForegroundColor Cyan
    Write-Host "   explorer.exe shell:AppsFolder\$($installed.PackageFamilyName)!$ApplicationId"
} else {
    Write-Host "安装命令已执行完成。" -ForegroundColor Green
}
