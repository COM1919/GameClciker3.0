$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$back = Join-Path $root '_gbk_bak'
$u8 = [byte[]](0xE4,0xBD,0xBF,0xE7,0x94,0xA8)      # 使用 in UTF-8
$gbkBytes = [byte[]](0xCA,0xB9,0xD3,0xC3)          # 使用 in GBK
function HasSeq($b, $seq) {
    for ($i = 0; $i -le $b.Length - $seq.Length; $i++) {
        $ok = $true
        for ($j = 0; $j -lt $seq.Length; $j++) { if ($b[$i+$j] -ne $seq[$j]) { $ok = $false; break } }
        if ($ok) { return $true }
    }
    return $false
}
foreach ($f in 'Definitons.h','MainClick.h','GC.cpp','ConfigParser.h','Tray.h','helperCode.h') {
    $p = Join-Path $back $f
    if (-not (Test-Path $p)) { $p = Join-Path $root $f }
    $b = [System.IO.File]::ReadAllBytes($p)
    Write-Output ("{0} bytes={1} hasUTF8_使用={2} hasGBK_使用={3}" -f $f, $b.Length, (HasSeq $b $u8), (HasSeq $b $gbkBytes))
}
