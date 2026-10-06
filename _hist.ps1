$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
function GitBlob($spec) {
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo.FileName = 'git'
    $p.StartInfo.Arguments = '--no-pager show ' + $spec
    $p.StartInfo.RedirectStandardOutput = $true
    $p.StartInfo.UseShellExecute = $false
    $p.StartInfo.WorkingDirectory = $root
    [void]$p.Start()
    $ms = New-Object System.IO.MemoryStream
    $p.StandardOutput.BaseStream.CopyTo($ms)
    $p.WaitForExit()
    return $ms.ToArray()
}
function HasSeq($b, $seq) { for ($i=0; $i -le $b.Length-$seq.Length; $i++) { $ok=$true; for ($j=0;$j -lt $seq.Length;$j++){ if($b[$i+$j] -ne $seq[$j]){$ok=$false;break} }; if($ok){return $true} }; return $false }
$u8 = [byte[]](0xE4,0xBD,0xBF,0xE7,0x94,0xA8)
$gb = [byte[]](0xCA,0xB9,0xD3,0xC3)
$fffd = [byte[]](0xEF,0xBF,0xBD)
foreach ($c in '206be89','cd1cfa2','996eeb9','HEAD') {
    foreach ($f in 'Definitons.h','Script.h','ConfigParser.h','GC.cpp') {
        $b = GitBlob ($c + ':' + $f)
        $n = 0
        for ($i=0; $i -le $b.Length-3; $i++) { if ($b[$i] -eq 0xEF -and $b[$i+1] -eq 0xBF -and $b[$i+2] -eq 0xBD) { $n++ } }
        Write-Output ("{0} {1}: bytes={2} hasUTF8_使用={3} hasGBK_使用={4} EFBFBD={5}" -f $c, $f, $b.Length, (HasSeq $b $u8), (HasSeq $b $gb), $n)
    }
}
