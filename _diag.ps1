$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$back = Join-Path $root '_gbk_bak'
function BomCheck($bytes) {
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { return 'UTF8-BOM' }
    if ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) { return 'UTF16LE-BOM' }
    return 'no-BOM'
}
$strictUtf8 = New-Object System.Text.UTF8Encoding($false, $true)
foreach ($f in 'Definitons.h','MainClick.h','GC.cpp','ConfigParser.h') {
    foreach ($dir in @($back, $root)) {
        $p = Join-Path $dir $f
        $b = [System.IO.File]::ReadAllBytes($p)
        $tag = if ($dir -eq $back) { 'BAK ' } else { 'CUR ' }
        $isU8 = $true
        try { [void]$strictUtf8.GetString($b) } catch { $isU8 = $false }
        Write-Output ("{0}{1} bytes={2} {3} strictUTF8={4}" -f $tag, $f, $b.Length, (BomCheck $b), $isU8)
    }
}
Write-Output '--- CUR GC.cpp line 1257 codepoints ---'
$gbk = [System.Text.Encoding]::GetEncoding(936)
$s = $gbk.GetString([System.IO.File]::ReadAllBytes((Join-Path $root 'GC.cpp')))
$lines = $s -split "`n"
Write-Output ("cur line1257: " + (($lines[1256].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' '))
$sb = $gbk.GetString([System.IO.File]::ReadAllBytes((Join-Path $back 'GC.cpp')))
$linesb = $sb -split "`n"
Write-Output ("bak line1257: " + (($linesb[1256].ToCharArray() | ForEach-Object { '{0:X4}' -f [int]$_ }) -join ' '))
Write-Output ("bak lines=" + $linesb.Length + " cur lines=" + $lines.Length)
