$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
function GitBlobToFile($spec, $out) {
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo.FileName = 'git'
    $p.StartInfo.Arguments = '--no-pager show ' + $spec
    $p.StartInfo.RedirectStandardOutput = $true
    $p.StartInfo.UseShellExecute = $false
    $p.StartInfo.WorkingDirectory = $root
    [void]$p.Start()
    $fs = [System.IO.File]::Create($out)
    $p.StandardOutput.BaseStream.CopyTo($fs)
    $fs.Close()
    $p.WaitForExit()
}
GitBlobToFile 'HEAD:Definitons.h' (Join-Path $root '_head_Definitons.h')
GitBlobToFile 'HEAD:ConfigParser.h' (Join-Path $root '_head_ConfigParser.h')
GitBlobToFile 'HEAD:Script.h' (Join-Path $root '_head_Script.h')
GitBlobToFile 'HEAD:GC.cpp' (Join-Path $root '_head_GC.cpp')
$utf8 = New-Object System.Text.UTF8Encoding($false, $true)
$gbk = [System.Text.Encoding]::GetEncoding(936)
foreach ($f in '_head_Definitons.h','_head_ConfigParser.h','_head_Script.h','_head_GC.cpp') {
    $p = Join-Path $root $f
    $b = [System.IO.File]::ReadAllBytes($p)
    $isU8 = $true; $u8bad = 0
    try { $s = $utf8.GetString($b); foreach ($ch in $s.ToCharArray()) { if ([int]$ch -eq 0xFFFD) { $u8bad++ } } } catch { $isU8 = $false }
    $u8seq = [byte[]](0xE4,0xBD,0xBF,0xE7,0x94,0xA8)
    $gbkseq = [byte[]](0xCA,0xB9,0xD3,0xC3)
    function HasSeq($b, $seq) { for ($i=0; $i -le $b.Length-$seq.Length; $i++) { $ok=$true; for ($j=0;$j -lt $seq.Length;$j++){ if($b[$i+$j] -ne $seq[$j]){$ok=$false;break} }; if($ok){return $true} }; return $false }
    Write-Output ("{0} bytes={1} validUTF8={2} UFFFD={3} hasUTF8_使用={4} hasGBK_使用={5}" -f $f, $b.Length, $isU8, $u8bad, (HasSeq $b $u8seq), (HasSeq $b $gbkseq))
}
