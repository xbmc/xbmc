# Runs ghworkflowfails.js (or any Node script) with GITHUB_TOKEN from the canonical PAT file.
# Usage: .\tools\ci\run-ghworkflowfails.ps1 -ScriptPath Z:\ghworkflowfails.js
param(
    [Parameter(Mandatory = $true)]
    [string] $ScriptPath,
    [string] $PatPath = "",
    [string] $WorkflowId = "build-windows.yaml"
)
$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path (Split-Path $ScriptDir -Parent) -Parent
if ([string]::IsNullOrEmpty($PatPath)) {
    $PatPath = Join-Path $RepoRoot ".git\organizerro_pat.txt"
}
if (-not (Test-Path $PatPath)) {
    Write-Error "PAT file not found: $PatPath"
}
$pat = (Get-Content -Raw $PatPath).Trim()
if ([string]::IsNullOrWhiteSpace($pat)) {
    Write-Error "PAT file is empty: $PatPath"
}
$env:GITHUB_TOKEN = $pat
$env:GH_TOKEN = $pat
if (-not $env:GITHUB_OWNER) { $env:GITHUB_OWNER = "OrganizerRo" }
if (-not $env:GITHUB_REPO) { $env:GITHUB_REPO = "xbmc" }
if ($args.Count -eq 0) {
    & node $ScriptPath --token $pat --owner $env:GITHUB_OWNER --repo $env:GITHUB_REPO --workflow-id $WorkflowId
} else {
    & node $ScriptPath @args
}
