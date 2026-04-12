# Runs ghworkflowfails.js (or any Node script) with GITHUB_TOKEN from the canonical PAT file.
# Usage: .\tools\ci\run-ghworkflowfails.ps1 -ScriptPath Z:\ghworkflowfails.js
param(
    [Parameter(Mandatory = $true)]
    [string] $ScriptPath,
    [string] $PatPath = $(Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) ".git\organizerro_pat.txt")
)
$ErrorActionPreference = "Stop"
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
& node $ScriptPath @args
