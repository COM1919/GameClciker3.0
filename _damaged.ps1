$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$utf8s = New-Object System.Text.UTF8Encoding($false, $true)
function ShowDamagedStrings($f) {
    $b = [System.IO.File]::ReadAllBytes((Join-Path $root $f))
    $s = $utf8s.GetString($b)
    $lines = $s -split "`n"
    Write-Output ("===== " + $f + " lines with damaged text inside quotes (max 12) =====")
    $shown = 0
    for ($i = 0; $i -lt $lines.Length -and $shown -lt 12; $i++) {
        $line = $lines[$i]
        if ($line.IndexOf([char]0xFFFD) -lt 0) { continue }
        $q = $false; $hasBadInStr = $false
        foreach ($ch in $line.ToCharArray()) {
            if ($ch -eq [char]0x22) { $q = -not $q }
            elseif ($ch -eq [char]0xFFFD -and $q) { $hasBadInStr = $true }
        }
        if ($hasBadInStr) {
            $vis = ''
            foreach ($ch in $line.ToCharArray()) {
                $code = [int]$ch
                if ($code -eq 0xFFFD) { $vis += '<BAD>' }
                elseif ($code -gt 127) { $vis += '<CN>' }
                else { $vis += $ch }
            }
            $shown++
            $vis = $vis.TrimEnd([char]13)
            Write-Output ("L{0}: {1}" -f ($i+1), $vis)
        }
    }
}
ShowDamagedStrings 'Definitons.h'
ShowDamagedStrings 'Script.h'
ShowDamagedStrings 'ConfigParser.h'
