#Requires -Version 5.1
<#
.SYNOPSIS
    将 GC连点器 打包为 MSIX 安装包。

.DESCRIPTION
    特点：
      * 不修改项目源码与工程文件，所有处理都在临时 staging 副本上进行。
      * 自动收集主程序与「GC封装文件」（运行时必选依赖）以及同目录的运行库 DLL。
      * 自动把主程序内嵌 manifest 的 requireAdministrator 改为 asInvoker：
        MSIX 应用不能默认以管理员运行（不利于商店审核），
        程序会在运行时自行用 runas 请求提权（见 GetInfo.h 中的更新逻辑）。
      * 自动生成 AppxManifest.xml 与各尺寸图标。
      * 使用 makeappx 打包，产出两个未签名包：
          - 商店包（<名称>_<版本>_<架构>.msix）：Publisher 为商店保留身份，用于上传 Microsoft Store。
          - 本地测试包（<名称>_<版本>_<架构>_localtest.msix）：Publisher 附加未签名 OID，
            供本机安装测试（Windows 强制要求，否则安装报 0x80073D2C）。
        本地安装测试请使用同目录下的 install-unsigned.cmd（自动提权、开启开发者模式并安装测试包）。
      * 声明微软商店标准的 VC++ 运行库框架依赖（Microsoft.VCLibs.140.00.UWPDesktop），
        包内不再自带 VC++ 运行库 DLL（安装时由框架包提供，符合商店规范）。
      * 注入 PSF（Package Support Framework）：修正打包应用的工作目录（默认是 System32，
        会导致相对路径资源加载失败），并把程序对自身安装目录的写入重定向到用户可写位置。

.PARAMETER Version
    MSIX 版本号，必须为四段式（如 1.0.0.0）。留空时自动生成：取主程序版本、
    已有产物与已安装版本中的最大者并递增末段，无需每次手动输入。

.PARAMETER Configuration
    编译配置，默认 Release。

.PARAMETER Platform
    目标平台，x64 或 Win32，默认 x64。

.PARAMETER SourceDir
    主程序所在目录，默认 <项目>\x64\Release。

.PARAMETER RuntimeDir
    「GC封装文件」目录，默认 <项目>\GC封装文件。

.PARAMETER OutputDir
    MSIX 输出目录，默认本脚本同级的 output 目录。

.PARAMETER Build
    打包前先用 MSBuild 重新编译项目。

.PARAMETER KeepStaging
    保留临时 staging 目录，便于排查问题。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\build-msix.ps1 -Version 1.0.1.0

.EXAMPLE
    # 产出未签名包（可上传商店），本地安装用同目录的 install-unsigned.cmd
    powershell -ExecutionPolicy Bypass -File .\build-msix.ps1
#>
[CmdletBinding()]
param(
    [string]   $Version             = '',
    [string]   $Configuration       = 'Release',
    [ValidateSet('x64', 'Win32')]
    [string]   $Platform            = 'x64',
    [string]   $SourceDir,
    [string]   $RuntimeDir,
    [string]   $OutputDir,
    [string]   $IconSource,
    [string]   $WideIconSource,
    [switch]   $Build,
    [switch]   $KeepStaging
)

$ErrorActionPreference = 'Stop'
$ProgressPreference    = 'SilentlyContinue'

# ============================ 商店身份信息（固定不变） ============================
$IdentityName         = 'Euxstudio.GameClicker3'
$PackageFamilyName    = 'Euxstudio.GameClicker3_gwer11b85jr02'   # Package Family Name
$Publisher            = 'CN=61A042EF-46FF-46B2-AF26-BB187D95DF79'
# 未签名包专用 OID：Windows 只允许 Publisher 带此标记的未签名包被注册（否则报 0x80073D2C）。
# 仅用于本地测试包，商店包的 Publisher 不能带它。
$UnsignedOid          = 'OID.2.25.311729368913984317654407730594956997722=1'
$PublisherDisplayName = 'COM1919'
$DisplayName          = 'GC连点器3'
$Description          = 'GC连点器 3.0'
$MinWinVersion        = '10.0.17763.0'
$MaxWinVersionTested  = '10.0.26100.0'

# 微软商店推荐的 VC++ 运行库框架包依赖（x64 桌面应用使用 UWPDesktop 变体）
$VCLibsName       = 'Microsoft.VCLibs.140.00.UWPDesktop'
$VCLibsMinVersion = '14.0.30035.0'
$VCLibsPublisher  = 'CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US'
# 上述框架包已提供的 DLL，打包时从包内移除，避免与应用本地副本重复（符合商店规范）
$VCRuntimeDlls = @(
    'concrt140.dll', 'msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll',
    'msvcp140_atomic_wait.dll', 'msvcp140_codecvt_ids.dll',
    'vccorlib140.dll', 'vcamp140.dll', 'vcomp140.dll',
    'vcruntime140.dll', 'vcruntime140_1.dll', 'vcruntime140_threads.dll'
)

$ExeName              = 'GameClciker3.0.exe'
$ExeLauncher          = 'PsfLauncher64.exe'   # PSF 启动器（作为包入口，用于修正工作目录与把写入重定向到可写位置）
$ApplicationId        = 'GCGameClicker'       # AppxManifest 中的 Application Id（config.json 须一致）
$AppDataDir           = 'GameClicker'         # 程序在自身目录下创建的数据文件夹（需重定向）

# ============================ 路径推导 ============================
$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
if (-not $SourceDir)  { $SourceDir  = Join-Path $ProjectRoot (Join-Path $Platform $Configuration) }
if (-not $RuntimeDir) { $RuntimeDir = Join-Path $ProjectRoot 'GC封装文件' }
if (-not $OutputDir)  { $OutputDir  = Join-Path $ScriptDir 'output' }

$ExePath = Join-Path $SourceDir $ExeName
if (-not (Test-Path $ExePath)) {
    throw "找不到主程序：$ExePath`n请先编译 $Platform|$Configuration，或使用 -Build 参数，或用 -SourceDir 指定目录。"
}

# 版本号：未显式指定时自动生成，无需每次手动输入。
# 取「主程序文件版本」「output 已有产物」「本机已安装版本」中的最大者，末段 +1，
# 保证每次产出都高于上一次，可直接覆盖安装。
if (-not $Version) {
    $candidates = New-Object System.Collections.Generic.List[version]

    $fv = (Get-Item $ExePath).VersionInfo.FileVersion
    if ($fv -and ($fv -match '(\d+)\.(\d+)\.(\d+)\.(\d+)')) {
        $candidates.Add([version]"$($matches[1]).$($matches[2]).$($matches[3]).$($matches[4])")
    }

    Get-ChildItem $OutputDir -Filter '*.msix' -File -ErrorAction SilentlyContinue | ForEach-Object {
        if ($_.BaseName -match '_(\d+)\.(\d+)\.(\d+)\.(\d+)_') {
            $candidates.Add([version]"$($matches[1]).$($matches[2]).$($matches[3]).$($matches[4])")
        }
    }

    try {
        Get-AppxPackage -Name $PackageFamilyName -ErrorAction SilentlyContinue | ForEach-Object {
            $candidates.Add([version]$_.Version)
        }
    } catch { }

    if ($candidates.Count -eq 0) { $candidates.Add([version]'0.0.0.0') }
    $latest  = $candidates | Sort-Object -Descending | Select-Object -First 1
    $Version = "$($latest.Major).$($latest.Minor).$($latest.Build).$($latest.Revision + 1)"
}
if ($Version -notmatch '^\d+\.\d+\.\d+\.\d+$') {
    throw "Version 必须为四段式数字，例如 1.0.0.0，当前值：$Version"
}

$StagingDir = Join-Path $OutputDir '_staging'
$AssetsDir  = Join-Path $StagingDir 'Assets'
# 未签名包（用于发布 Microsoft Store；本地安装请用 install-unsigned.cmd）
$OutputMsix = Join-Path $OutputDir "$([System.IO.Path]::GetFileNameWithoutExtension($ExeName))_${Version}_${Platform}.msix"

# ============================ 工具函数 ============================
function Write-Step([int]$Index, [int]$Total, [string]$Text) {
    Write-Host ""
    Write-Host "[$Index/$Total] $Text" -ForegroundColor Cyan
}

function Get-SdkTool {
    param([Parameter(Mandatory)][string]$Name)
    $roots = New-Object System.Collections.Generic.List[string]
    $kitsRoot = $null
    try {
        $kitsRoot = (Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots' -ErrorAction SilentlyContinue).KitsRoot10
    } catch { }
    if ($kitsRoot) { $roots.Add((Join-Path $kitsRoot 'bin')) }
    $roots.Add("${env:ProgramFiles(x86)}\Windows Kits\10\bin")
    $roots.Add("$env:ProgramFiles\Windows Kits\10\bin")

    foreach ($r in $roots) {
        if (-not (Test-Path $r)) { continue }
        $cand = Get-ChildItem $r -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object { Join-Path $_.FullName "x64\$Name" } |
                Where-Object { Test-Path $_ } |
                Select-Object -First 1
        if ($cand) { return $cand }
    }
    throw "未找到 Windows SDK 工具 $Name。请安装 Windows 10/11 SDK。"
}

function Save-ScaledPng {
    param(
        [Parameter(Mandatory)][System.Drawing.Image]$Src,
        [Parameter(Mandatory)][int]$Width,
        [Parameter(Mandatory)][int]$Height,
        [Parameter(Mandatory)][string]$Out,
        [switch]$Stretch
    )
    $bmp = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $g.PixelOffsetMode   = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $g.Clear([System.Drawing.Color]::Transparent)
        if ($Stretch) {
            $g.DrawImage($Src, [System.Drawing.Rectangle]::new(0, 0, $Width, $Height))
        } else {
            $ratio = [Math]::Min($Width / $Src.Width, $Height / $Src.Height)
            $nw = [int][Math]::Round($Src.Width * $ratio)
            $nh = [int][Math]::Round($Src.Height * $ratio)
            $x = [int](($Width - $nw) / 2)
            $y = [int](($Height - $nh) / 2)
            $g.DrawImage($Src, [System.Drawing.Rectangle]::new($x, $y, $nw, $nh))
        }
    } finally {
        $g.Dispose()
    }
    $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

# 将主程序内嵌 manifest 的提权级别改为 asInvoker（只作用于 staging 副本）
function Set-ExeAsInvoker {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$MtExe,
        [Parameter(Mandatory)][string]$TempDir
    )
    $mfPath = Join-Path $TempDir '_patch_exe.manifest'
    & $MtExe -nologo "-inputresource:$Path;#1" "-out:$mfPath" | Out-Null
    if (-not (Test-Path $mfPath)) { throw "无法从主程序提取内嵌 manifest。" }

    $xml = [System.IO.File]::ReadAllText($mfPath)
    if ($xml -match 'requireAdministrator|highestAvailable') {
        $xml = $xml -replace 'requireAdministrator', 'asInvoker' -replace 'highestAvailable', 'asInvoker'
        [System.IO.File]::WriteAllText($mfPath, $xml, (New-Object System.Text.UTF8Encoding($false)))
        & $MtExe -nologo "-manifest" $mfPath "-outputresource:$Path;#1" | Out-Null
        Write-Host "  已把主程序内嵌 manifest 的提权级别改为 asInvoker（不改动项目原始文件）。" -ForegroundColor Yellow
    } else {
        Write-Host "  主程序 manifest 未声明管理员权限，无需修改。" -ForegroundColor DarkGray
    }
}

# ============================ 开始 ============================
$totalSteps = 7
Write-Host "==============================================" -ForegroundColor Green
Write-Host " GC连点器 MSIX 打包脚本" -ForegroundColor Green
Write-Host "==============================================" -ForegroundColor Green
Write-Host " 主程序    : $ExePath"
Write-Host " 依赖目录  : $RuntimeDir"
Write-Host " 版本号    : $Version"
Write-Host " 输出目录  : $OutputDir"

# ---------- 0. 可选：重新编译 ----------
if ($Build) {
    Write-Step 0 $totalSteps "使用 MSBuild 重新编译 $Platform|$Configuration"
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { throw "未找到 vswhere.exe，无法自动定位 MSBuild。请手动编译后再运行本脚本。" }
    $vsPath = & $vswhere -latest -property installationPath
    $msbuild = Join-Path $vsPath 'MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path $msbuild)) { throw "未找到 MSBuild：$msbuild" }
    $sln = Get-ChildItem -Path $ProjectRoot -Filter '*.sln' | Select-Object -First 1
    if (-not $sln) { throw "未在项目目录找到 .sln 文件。" }
    & $msbuild $sln.FullName "/p:Configuration=$Configuration" "/p:Platform=$Platform" /m /nologo /v:m
    if ($LASTEXITCODE -ne 0) { throw "编译失败，退出码 $LASTEXITCODE。" }
    if (-not (Test-Path $ExePath)) { throw "编译完成但未找到主程序：$ExePath" }
}

# ---------- 1. 准备 staging ----------
Write-Step 1 $totalSteps "清理并创建 staging 目录"
if (Test-Path $StagingDir) { Remove-Item $StagingDir -Recurse -Force }
New-Item -ItemType Directory -Path $StagingDir   -Force | Out-Null
New-Item -ItemType Directory -Path $AssetsDir    -Force | Out-Null
New-Item -ItemType Directory -Path $OutputDir    -Force | Out-Null
Write-Host "  $StagingDir"

# ---------- 2. 收集文件 ----------
Write-Step 2 $totalSteps "复制主程序与依赖文件"
Copy-Item $ExePath (Join-Path $StagingDir $ExeName) -Force
Write-Host "  + $ExeName"

if (Test-Path $RuntimeDir) {
    Copy-Item (Join-Path $RuntimeDir '*') $StagingDir -Recurse -Force
    Write-Host "  + GC封装文件 (res / sound / DLL 等，已展开到包根目录)" -ForegroundColor Green
} else {
    Write-Warning "未找到 GC封装文件 目录：$RuntimeDir（程序可能无法运行）"
}

# 补充 SourceDir 中 GC封装文件 未包含的运行库 DLL
Get-ChildItem -Path $SourceDir -Filter '*.dll' -File -ErrorAction SilentlyContinue | ForEach-Object {
    $dst = Join-Path $StagingDir $_.Name
    if (-not (Test-Path $dst)) { Copy-Item $_.FullName $dst -Force; Write-Host "  + $($_.Name)" }
}

# VC++ 运行库 DLL 改由 VCLibs 框架包提供（见 AppxManifest 的 PackageDependency），
# 从包内移除，避免与应用本地副本重复，符合微软商店规范
$removedVc = @()
foreach ($dll in $VCRuntimeDlls) {
    $p = Join-Path $StagingDir $dll
    if (Test-Path $p) { Remove-Item $p -Force; $removedVc += $dll }
}
if ($removedVc.Count) {
    Write-Host "  - 移除 VC++ 运行库 DLL（由 VCLibs 框架提供）：$($removedVc -join ', ')" -ForegroundColor DarkGray
}

# ---------- 注入 PSF（Package Support Framework）----------
# 打包应用默认工作目录是 System32，且安装目录只读。程序用相对路径读取 res/sound，
# 并把配置、日志等写入自身目录，因此需要 PSF 修正「工作目录」与「写入权限」两个问题。
$PsfDir   = Join-Path $ScriptDir 'psf'
$psfFiles = @($ExeLauncher, 'PsfRuntime64.dll', 'PsfRunDll64.exe', 'FileRedirectionFixup.dll')
foreach ($f in $psfFiles) {
    $src = Join-Path $PsfDir $f
    if (-not (Test-Path $src)) { throw "缺少 PSF 组件：$src（请确认 packaging\psf 目录完整）" }
    Copy-Item $src $StagingDir -Force
}
Write-Host "  + PSF 组件：$($psfFiles -join ', ')" -ForegroundColor Green

$psfConfig = @"
{
  "applications": [
    {
      "id": "$ApplicationId",
      "executable": "$ExeName",
      "workingDirectory": "."
    }
  ],
  "processes": [
    {
      "executable": "$([System.IO.Path]::GetFileNameWithoutExtension($ExeName))",
      "fixups": [
        {
          "dll": "FileRedirectionFixup.dll",
          "config": {
            "redirectedPaths": {
              "packageRelative": [
                { "base": "$AppDataDir", "patterns": [ ".*" ] },
                { "base": ".", "patterns": [ ".*\\.log" ] }
              ]
            }
          }
        }
      ]
    }
  ]
}
"@
[System.IO.File]::WriteAllText((Join-Path $StagingDir 'config.json'), $psfConfig, (New-Object System.Text.UTF8Encoding($false)))
Write-Host "  + config.json（工作目录=包根；写入重定向：$AppDataDir 目录与 *.log）"

# ---------- 3. 处理 UAC ----------
Write-Step 3 $totalSteps "调整主程序提权级别（asInvoker）"
$mt = Get-SdkTool 'mt.exe'
Set-ExeAsInvoker -Path (Join-Path $StagingDir $ExeName) -MtExe $mt -TempDir $OutputDir

# ---------- 4. 生成图标 ----------
Write-Step 4 $totalSteps "生成应用图标资源"
Add-Type -AssemblyName System.Drawing
$sqSource = $null
if ($IconSource) { $sqSource = $IconSource } else {
    $cand = @(
        (Join-Path $RuntimeDir 'res\pics\ico\GCIcon.png'),
        (Join-Path $ProjectRoot 'GC.png'),
        (Join-Path $ProjectRoot 'GCicon.ico')
    )
    $sqSource = $cand | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $sqSource) { throw "未找到可用的图标源，请用 -IconSource 指定一张 PNG。" }

$wideSource = $null
if ($WideIconSource) { $wideSource = $WideIconSource } else {
    $cand = @(
        (Join-Path $ProjectRoot 'GC.png'),
        (Join-Path $RuntimeDir 'res\pics\Logo.png')
    )
    $wideSource = $cand | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $wideSource) { $wideSource = $sqSource }

$imgSq   = [System.Drawing.Image]::FromFile($sqSource)
$imgWide = [System.Drawing.Image]::FromFile($wideSource)
try {
    Save-ScaledPng -Src $imgSq   -Width 50  -Height 50  -Out (Join-Path $AssetsDir 'StoreLogo.png')        -Stretch
    Save-ScaledPng -Src $imgSq   -Width 44  -Height 44  -Out (Join-Path $AssetsDir 'Square44x44Logo.png')  -Stretch
    Save-ScaledPng -Src $imgSq   -Width 71  -Height 71  -Out (Join-Path $AssetsDir 'Square71x71Logo.png')  -Stretch
    Save-ScaledPng -Src $imgSq   -Width 150 -Height 150 -Out (Join-Path $AssetsDir 'Square150x150Logo.png') -Stretch
    Save-ScaledPng -Src $imgSq   -Width 310 -Height 310 -Out (Join-Path $AssetsDir 'Square310x310Logo.png') -Stretch
    Save-ScaledPng -Src $imgWide -Width 310 -Height 150 -Out (Join-Path $AssetsDir 'Wide310x150Logo.png')
} finally {
    $imgSq.Dispose()
    $imgWide.Dispose()
}
Write-Host "  方形图标源: $sqSource"
Write-Host "  宽幅图标源: $wideSource"

# ---------- 5. 生成 AppxManifest.xml ----------
Write-Step 5 $totalSteps "生成 AppxManifest.xml"
$manifestTemplate = @'
<?xml version="1.0" encoding="utf-8"?>
<Package
  xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
  xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
  xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
  IgnorableNamespaces="uap rescap">
  <Identity Name="{NAME}" Publisher="{PUBLISHER}" Version="{VERSION}" ProcessorArchitecture="{ARCH}" />
  <Properties>
    <DisplayName>{DISPLAYNAME}</DisplayName>
    <PublisherDisplayName>{PUBLISHERDISPLAYNAME}</PublisherDisplayName>
    <Logo>Assets\StoreLogo.png</Logo>
    <Description>{DESCRIPTION}</Description>
  </Properties>
  <Dependencies>
    <TargetDeviceFamily Name="Windows.Desktop" MinVersion="{MINVER}" MaxVersionTested="{MAXVER}" />
    <PackageDependency Name="{VCLIBSNAME}" MinVersion="{VCLIBSVER}" Publisher="{VCLIBSPUB}" />
  </Dependencies>
  <Resources>
    <Resource Language="zh-CN" />
    <Resource Language="en-US" />
  </Resources>
  <Capabilities>
    <rescap:Capability Name="runFullTrust" />
  </Capabilities>
  <Applications>
    <Application Id="{APPID}" Executable="{LAUNCHER}" EntryPoint="Windows.FullTrustApplication">
      <uap:VisualElements
        DisplayName="{DISPLAYNAME}"
        Description="{DESCRIPTION}"
        BackgroundColor="transparent"
        Square150x150Logo="Assets\Square150x150Logo.png"
        Square44x44Logo="Assets\Square44x44Logo.png">
        <uap:DefaultTile
          Wide310x150Logo="Assets\Wide310x150Logo.png"
          Square310x310Logo="Assets\Square310x310Logo.png"
          Square71x71Logo="Assets\Square71x71Logo.png" />
      </uap:VisualElements>
    </Application>
  </Applications>
</Package>
'@

$manifest = $manifestTemplate `
    -replace '\{NAME\}',                 $IdentityName `
    -replace '\{PUBLISHER\}',            $Publisher `
    -replace '\{PUBLISHERDISPLAYNAME\}', $PublisherDisplayName `
    -replace '\{DISPLAYNAME\}',          $DisplayName `
    -replace '\{DESCRIPTION\}',          $Description `
    -replace '\{VERSION\}',              $Version `
    -replace '\{ARCH\}',                 $Platform `
    -replace '\{MINVER\}',               $MinWinVersion `
    -replace '\{MAXVER\}',               $MaxWinVersionTested `
    -replace '\{VCLIBSNAME\}',           $VCLibsName `
    -replace '\{VCLIBSVER\}',            $VCLibsMinVersion `
    -replace '\{VCLIBSPUB\}',            $VCLibsPublisher `
    -replace '\{APPID\}',                $ApplicationId `
    -replace '\{LAUNCHER\}',             $ExeLauncher `
    -replace '\{EXE\}',                  $ExeName

[System.IO.File]::WriteAllText((Join-Path $StagingDir 'AppxManifest.xml'), $manifest, (New-Object System.Text.UTF8Encoding($false)))
Write-Host "  Identity : $IdentityName  $Version  $Platform"
Write-Host "  Publisher: $Publisher"
Write-Host "  依赖     : $VCLibsName >= $VCLibsMinVersion"

# ---------- 6. 打包（未签名）----------
Write-Step 6 $totalSteps "使用 makeappx 打包（未签名）"
$makeappx = Get-SdkTool 'makeappx.exe'
if (Test-Path $OutputMsix) { Remove-Item $OutputMsix -Force }
& $makeappx pack /d $StagingDir /p $OutputMsix /o
if ($LASTEXITCODE -ne 0) { throw "makeappx 打包失败，退出码 $LASTEXITCODE。" }
Write-Host "  商店包已生成：$OutputMsix" -ForegroundColor Green

# ---------- 7. 打包本地测试包（Publisher 附加未签名 OID）----------
# Windows 只允许「Publisher 带特殊 OID」的未签名包被注册，否则报 0x80073D2C。
# 商店包的 Publisher 必须是商店保留身份的正式值（不能带 OID），二者无法共用一个包，
# 因此额外产出一个仅供本机安装测试的包。
Write-Step 7 $totalSteps "生成本地测试包（Publisher 附加未签名 OID）"
$TestMsix = Join-Path $OutputDir "$([System.IO.Path]::GetFileNameWithoutExtension($ExeName))_${Version}_${Platform}_localtest.msix"
$oldPublisherAttr = 'Publisher="' + $Publisher + '"'
$newPublisherAttr = 'Publisher="' + $Publisher + ', ' + $UnsignedOid + '"'
$testManifest = $manifest.Replace($oldPublisherAttr, $newPublisherAttr)
if ($testManifest -eq $manifest) { throw "未能为本地测试包写入未签名 OID。" }
[System.IO.File]::WriteAllText((Join-Path $StagingDir 'AppxManifest.xml'), $testManifest, (New-Object System.Text.UTF8Encoding($false)))
if (Test-Path $TestMsix) { Remove-Item $TestMsix -Force }
& $makeappx pack /d $StagingDir /p $TestMsix /o
if ($LASTEXITCODE -ne 0) { throw "makeappx 打包本地测试包失败，退出码 $LASTEXITCODE。" }
Write-Host "  本地测试包已生成：$TestMsix" -ForegroundColor Green

# ---------- 收尾 ----------
Remove-Item (Join-Path $OutputDir '_patch_exe.manifest') -Force -ErrorAction SilentlyContinue
if (-not $KeepStaging) { Remove-Item $StagingDir -Recurse -Force -ErrorAction SilentlyContinue }

Write-Host ""
Write-Host "==============================================" -ForegroundColor Green
Write-Host " 打包完成" -ForegroundColor Green
Write-Host "==============================================" -ForegroundColor Green
Write-Host " 商店包（发布商店）  : $OutputMsix"
Write-Host " 本地测试包（本机装）: $TestMsix"
Write-Host ""
Write-Host " 本地安装测试：" -ForegroundColor Cyan
Write-Host "   双击运行 install-unsigned.cmd 即可（自动提权、开启开发者模式并安装测试包）"
Write-Host " 卸载：" -ForegroundColor Cyan
Write-Host "   Remove-AppxPackage -Name $IdentityName"
Write-Host ""
Write-Host " 提示：未指定 -Version 时版本号会自动递增，直接重新运行本脚本即可。" -ForegroundColor DarkGray
