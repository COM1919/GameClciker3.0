$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$utf8s = New-Object System.Text.UTF8Encoding($false, $true)
foreach ($f in 'Definitons.h','ConfigParser.h','Script.h','GC.cpp','Test.cpp','UDPSocket.h','MainClick.h','Tray.h','KeyState.h') {
    $p = Join-Path $root $f
    $b = [System.IO.File]::ReadAllBytes($p)
    $nonAscii = 0
    foreach ($x in $b) { if ($x -gt 127) { $nonAscii++ } }
    $s = $null
    $isU8 = $true
    try { $s = $utf8s.GetString($b) } catch { $isU8 = $false }
    $inStr = 0; $outStr = 0; $example = ''
    if ($isU8) {
        foreach ($line in ($s -split "`n")) {
            if ($line.IndexOf([char]0xFFFD) -ge 0) {
                $q = $false; $pos = 0
                foreach ($ch in $line.ToCharArray()) {
                    if ($ch -eq [char]0x22) { $q = -not $q }
                    elseif ($ch -eq [char]0xFFFD) { if ($q) { $inStr++ } else { $outStr++ }; if ($example -eq '' -and $q) { $example = $line } }
                    $pos++
                }
            }
        }
    }
    Write-Output ("{0}: bytes={1} nonASCII={2} validUTF8={3} UFFFD_inString={4} UFFFD_inComment={5}" -f $f, $b.Length, $nonAscii, $isU8, $inStr, $outStr)
}
