$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$gbk = [System.Text.Encoding]::GetEncoding(936)
$raw = [System.IO.File]::ReadAllText((Join-Path $root '_patch_strings.txt'), [System.Text.Encoding]::UTF8)
$raw = $raw.TrimStart([char]0xFEFF)
$raw = $raw -replace "`r", ""
function Sec([string]$name) {
    $pat = "(?s)@@" + [regex]::Escape($name) + "@@\n(.*?)(?=\n@@|\z)"
    $m = [regex]::Match($raw, $pat)
    if (-not $m.Success) { throw ("section not found: " + $name) }
    return $m.Groups[1].Value.TrimEnd([char]10)
}
function Apply([string]$file, [string]$old, [string]$new) {
    $p = Join-Path $root $file
    $s = $gbk.GetString([System.IO.File]::ReadAllBytes($p))
    if (-not $s.Contains($old)) { throw ("pattern not found in " + $file) }
    $count = 0
    $idx = 0
    while (($idx = $s.IndexOf($old, $idx)) -ge 0) { $count++; $idx += $old.Length }
    $s2 = $s.Replace($old, $new)
    [System.IO.File]::WriteAllBytes($p, $gbk.GetBytes($s2))
    Write-Output ("OK " + $file + " replaced=" + $count)
}
Apply 'Definitons.h' (Sec 'p2old') (Sec 'p2new')
Apply 'MainClick.h' (Sec 'p1old') (Sec 'p1new')
Apply 'GC.cpp' (Sec 'p3old') (Sec 'p3new')
Apply 'GC.cpp' (Sec 'p4old') (Sec 'p4new')
Write-Output 'DONE'
