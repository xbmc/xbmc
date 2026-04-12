# Push local branch to origin/Leia using PAT from .git/organizerro_pat.txt
param(
    [string] $LocalBranch = "leia",
    [string] $RemoteBranch = "Leia"
)
$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path (Split-Path $ScriptDir -Parent) -Parent
$PatPath = Join-Path $RepoRoot ".git\organizerro_pat.txt"
$pat = (Get-Content -Raw $PatPath).Trim()
$remote = "https://x-access-token:${pat}@github.com/OrganizerRo/xbmc.git"
git -C $RepoRoot push $remote "${LocalBranch}:${RemoteBranch}"
