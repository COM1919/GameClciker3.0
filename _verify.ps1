$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$gbk = [System.Text.Encoding]::GetEncoding(936)
foreach ($f in 'Definitons.h','MainClick.h','GC.cpp','ConfigParser.h') {
    $p = Join-Path $root $f
    $b = [System.IO.File]::ReadAllBytes($p)
    $s = $gbk.GetString($b)
    $rt = $gbk.GetBytes($s)
    $same = ($rt.Length -eq $b.Length)
    if ($same) { for ($i=0; $i -lt $b.Length; $i++) { if ($rt[$i] -ne $b[$i]) { $same = $false; break } } }
    $bad = 0
    foreach ($ch in $s.ToCharArray()) { if ([int]$ch -eq 0xFFFD) { $bad++ } }
    Write-Output ("{0} bytes={1} gbk_roundtrip={2} UFFFD={3}" -f $f, $b.Length, $same, $bad)
}
Write-Output '--- MainClick.h 393-403 ---'
$s = $gbk.GetString([System.IO.File]::ReadAllBytes((Join-Path $root 'MainClick.h')))
$lines = $s -split "`n"
foreach ($i in 392..402) { Write-Output ("[{0}] {1}" -f ($i+1), (($lines[$i].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' ')) }
Write-Output '--- Definitons.h 694-724 ---'
$s = $gbk.GetString([System.IO.File]::ReadAllBytes((Join-Path $root 'Definitons.h')))
$lines = $s -split "`n"
foreach ($i in 693..723) { Write-Output ("[{0}] {1}" -f ($i+1), (($lines[$i].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' ')) }
Write-Output '--- GC.cpp 305-312 ---'
$s = $gbk.GetString([System.IO.File]::ReadAllBytes((Join-Path $root 'GC.cpp')))
$lines = $s -split "`n"
foreach ($i in 304..311) { Write-Output ("[{0}] {1}" -f ($i+1), (($lines[$i].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' ')) }
Write-Output '--- GC.cpp 1291-1314 ---'
foreach ($i in 1290..1313) { Write-Output ("[{0}] {1}" -f ($i+1), (($lines[$i].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' ')) }
