$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
function GitOut($args) {
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo.FileName = 'git'
    $p.StartInfo.Arguments = $args
    $p.StartInfo.RedirectStandardOutput = $true
    $p.StartInfo.RedirectStandardError = $true
    $p.StartInfo.UseShellExecute = $false
    $p.StartInfo.WorkingDirectory = $root
    [void]$p.Start()
    $out = $p.StandardOutput.ReadToEnd()
    $p.WaitForExit()
    return $out
}
Write-Output '--- branches/refs ---'
Write-Output (GitOut 'branch -a')
Write-Output (GitOut 'stash list')
Write-Output (GitOut 'log --oneline --all -8')
Write-Output '--- all Definitons.h blobs (hash size) ---'
$objs = GitOut 'rev-list --all --objects'
foreach ($line in ($objs -split "`n")) {
    if ($line -match '^\s*([0-9a-f]{40})\s+Definitons\.h\s*$') { Write-Output $Matches[1] }
}
