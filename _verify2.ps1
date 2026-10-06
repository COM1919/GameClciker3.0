$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$utf8s = New-Object System.Text.UTF8Encoding($false, $true)
$gbk = [System.Text.Encoding]::GetEncoding(936)
$raw = ([System.IO.File]::ReadAllText((Join-Path $root '_patch_strings.txt'), [System.Text.Encoding]::UTF8)).TrimStart([char]0xFEFF) -replace "`r", ""
function Sec([string]$name) {
    $pat = "(?s)@@" + [regex]::Escape($name) + "@@\n(.*?)(?=\n@@|\z)"
    $m = [regex]::Match($raw, $pat)
    return $m.Groups[1].Value.TrimEnd([char]10)
}
function Load($f, $enc) {
    $b = [System.IO.File]::ReadAllBytes((Join-Path $root $f))
    if ($enc -eq 'u8') { return $utf8s.GetString($b) } else { return $gbk.GetString($b) }
}
function UfffdCount($s) { $n = 0; foreach ($ch in $s.ToCharArray()) { if ([int]$ch -eq 0xFFFD) { $n++ } }; return $n }
$d = Load 'Definitons.h' 'u8'
$m = Load 'MainClick.h' 'gbk'
$g = Load 'GC.cpp' 'u8'
Write-Output ("Definitons.h UFFFD={0} validGBK={1}" -f (UfffdCount $d), $false)
Write-Output ("MainClick.h UFFFD={0}" -f (UfffdCount $m))
Write-Output ("GC.cpp UFFFD={0}" -f (UfffdCount $g))
Write-Output ("Definitons has p2new = {0}" -f $d.Contains((Sec 'p2new')))
Write-Output ("MainClick has p1new = {0}" -f $m.Contains((Sec 'p1new')))
Write-Output ("GC has p3new = {0}" -f $g.Contains((Sec 'p3new')))
Write-Output ("GC has p4new = {0}" -f $g.Contains((Sec 'p4new')))
# Definitons.h: check anchor still present once and function present
function Cnt($s, $sub) { return ([regex]::Matches($s, [regex]::Escape($sub))).Count }
Write-Output ("Definitons 'namespace MessageBoxMsg' = {0}" -f (Cnt $d 'namespace MessageBoxMsg'))
Write-Output ("Definitons 'FlushUsageDuration' = {0}" -f (Cnt $d 'FlushUsageDuration'))
Write-Output ("GC 'FlushUsageDuration()' = {0}" -f (Cnt $g 'FlushUsageDuration()'))
Write-Output ("GC 'StartCount++' = {0}" -f (Cnt $g 'StartCount++'))
Write-Output ("GC 'ThisUseTime * 60' = {0}" -f (Cnt $g 'ThisUseTime * 60'))
Write-Output ("GC 'max(ThisUseTime, 1)' = {0}" -f (Cnt $g 'max(ThisUseTime, 1)'))
Write-Output ("MainClick 'UsageDuration += 0.1' = {0}" -f (Cnt $m 'UsageDuration += 0.1'))
$dl = $d -split "`n"
for ($i = 0; $i -lt $dl.Length; $i++) { if ($dl[$i].Contains((Sec 'p2mark'))) { Write-Output ("Definitons newline[{0}] cp: {1}" -f ($i+1), (($dl[$i].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' ')); break } }
$gl = $g -split "`n"
for ($i = 0; $i -lt $gl.Length; $i++) { if ($gl[$i].Contains((Sec 'p3mark'))) { Write-Output ("GC newline[{0}] cp: {1}" -f ($i+1), (($gl[$i].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' ')); break } }
