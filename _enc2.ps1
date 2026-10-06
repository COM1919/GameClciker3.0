$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$back = Join-Path $root '_gbk_bak'
$gbk = [System.Text.Encoding]::GetEncoding(936)
$utf8 = New-Object System.Text.UTF8Encoding($false, $true)
function Dump($label, $lines, $idx, $enc) {
    if ($enc -eq 'u8') { $chars = ($lines[$idx].ToCharArray()) } else { $chars = ($lines[$idx].ToCharArray()) }
    Write-Output ("{0}[{1}] {2}" -f $label, ($idx+1), (($chars | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' '))
}
# GC.cpp as UTF-8: locate the line containing ASCII marker 'Statistics'
$b = [System.IO.File]::ReadAllBytes((Join-Path $back 'GC.cpp'))
$s = $utf8.GetString($b)
$lines = $s -split "`n"
for ($i=0; $i -lt $lines.Length; $i++) {
    if ($lines[$i].Contains('u8"Statistics"')) { Dump 'GC.cpp-U8' $lines $i 'u8'; break }
}
# GBK decode of same line (to show mojibake)
$sg = $gbk.GetString($b)
$linesg = $sg -split "`n"
for ($i=0; $i -lt $linesg.Length; $i++) {
    if ($linesg[$i].Contains('"Statistics"')) { Dump 'GC.cpp-GBK' $linesg $i 'gbk'; break }
}
# round-trip loss check
$rt = $gbk.GetBytes($sg)
Write-Output ("GC.cpp gbk-roundtrip len orig={0} rt={1}" -f $b.Length, $rt.Length)
# Definitons.h line with UsageDuration
$b2 = [System.IO.File]::ReadAllBytes((Join-Path $back 'Definitons.h'))
$s2 = $utf8.GetString($b2)
$l2 = $s2 -split "`n"
for ($i=0; $i -lt $l2.Length; $i++) { if ($l2[$i].Contains('UsageDuration')) { Dump 'Definitons.h-U8' $l2 $i 'u8' } }
foreach ($i in 691..698) { Dump 'Definitons.h-U8' $l2 ($i-1) 'u8' }
# ConfigParser.h line with StatsRegPath
$b3 = [System.IO.File]::ReadAllBytes((Join-Path $back 'ConfigParser.h'))
$s3 = $utf8.GetString($b3)
$l3 = $s3 -split "`n"
for ($i=0; $i -lt $l3.Length; $i++) { if ($l3[$i].Contains('ClickerClickCount.left =')) { Dump 'ConfigParser.h-U8' $l3 $i 'u8' } }
