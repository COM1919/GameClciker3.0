$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$gbk = [System.Text.Encoding]::GetEncoding(936)
$encThrow = [System.Text.Encoding]::GetEncoding(936, [System.Text.EncoderFallback]::ExceptionFallback, [System.Text.DecoderFallback]::ExceptionFallback)
$utf8s = New-Object System.Text.UTF8Encoding($false, $true)
foreach ($f in 'GC.cpp','Definitons.h','ConfigParser.h','Script.h','UDPSocket.h') {
    $p = Join-Path $root $f
    $b = [System.IO.File]::ReadAllBytes($p)
    $s = $utf8s.GetString($b)
    $unm = 0
    foreach ($ch in $s.ToCharArray()) {
        try { [void]$encThrow.GetBytes([string]$ch) } catch { $unm++ }
    }
    $out = $gbk.GetBytes($s)
    $fffd = 0
    for ($i = 0; $i -le $b.Length - 3; $i++) { if ($b[$i] -eq 0xEF -and $b[$i+1] -eq 0xBF -and $b[$i+2] -eq 0xBD) { $fffd++ } }
    [System.IO.File]::WriteAllBytes($p, $out)
    Write-Output ("{0}: utf8={1} -> gbk={2} unmappable={3} UFFFD_in_src={4}" -f $f, $b.Length, $out.Length, $unm, $fffd)
}
Write-Output 'CONVERT DONE'
