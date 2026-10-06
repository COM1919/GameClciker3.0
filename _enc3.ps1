$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$utf8 = New-Object System.Text.UTF8Encoding($false, $true)
$gbk = [System.Text.Encoding]::GetEncoding(936)
$files = Get-ChildItem -Path $root -File -Include *.h,*.cpp,*.c -Recurse | Where-Object { $_.FullName -notmatch '\\node_modules\\' -and $_.FullName -notmatch '\\_gbk_bak\\' }
Write-Output 'FILE | bytes | validUTF8 | UFFFD(as UTF8) | gbkDecodeUFFFD'
foreach ($f in $files) {
    $b = [System.IO.File]::ReadAllBytes($f.FullName)
    $isU8 = $true
    $u8bad = 0
    try {
        $s = $utf8.GetString($b)
        foreach ($ch in $s.ToCharArray()) { if ([int]$ch -eq 0xFFFD) { $u8bad++ } }
    } catch { $isU8 = $false }
    $gbkbad = 0
    try {
        $sg = $gbk.GetString($b)
        foreach ($ch in $sg.ToCharArray()) { if ([int]$ch -eq 0xFFFD) { $gbkbad++ } }
    } catch { $gbkbad = -1 }
    $rel = $f.FullName.Substring($root.Length + 1)
    Write-Output ("{0} | {1} | {2} | {3} | {4}" -f $rel, $b.Length, $isU8, $u8bad, $gbkbad)
}
