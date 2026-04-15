<#
.SYNOPSIS
    Patches visualization addon C source files for MSVC intrinsic function compatibility.

.DESCRIPTION
    MSVC 14.44+ treats __floor and __ceil as built-in intrinsic functions that cannot
    be redefined (error C2169 / C7552). The EEL2 compiler embedded in visualization.milkdrop
    and visualization.milkdrop2 declares and takes the address of these functions locally.

    This script wraps those declarations and function-pointer assignments with
    #if !defined(_MSC_VER) guards so MSVC skips them and falls back to the standard
    library floor/ceil functions. Non-MSVC compilers (GCC, Clang) are unaffected.

    The script is idempotent: if the guards are already present the file is left unchanged.

.PARAMETER BuildDir
    Path to the cmake/addons/build directory that contains the downloaded addon sources.
    Defaults to $env:GITHUB_WORKSPACE\cmake\addons\build when run in CI.

.EXAMPLE
    .\patch-visualization-addons.ps1
    .\patch-visualization-addons.ps1 -BuildDir "C:\kodi\cmake\addons\build"
#>
param(
    [string]$BuildDir = "$env:GITHUB_WORKSPACE\cmake\addons\build"
)

$patches = @(
    @{ addon = "visualization.milkdrop";  relpath = "lib\vis_milkdrop\ns-eel2\nseel-compiler.c" },
    @{ addon = "visualization.milkdrop2"; relpath = "lib\vis_milk2\ns-eel2\nseel-compiler.c"  }
)

$anyPatched = $false

foreach ($p in $patches) {
    $src = Join-Path $BuildDir "$($p.addon)\$($p.relpath)"

    if (-not (Test-Path $src)) {
        Write-Warning "Skipping patch for $($p.addon): source not found at $src"
        continue
    }

    $lines = [System.IO.File]::ReadAllLines($src)
    $out   = [System.Collections.Generic.List[string]]::new()
    $i = 0
    $changed = $false

    # Helper: return the trimmed last line already added to $out, or "" if none.
    function Get-LastOutLine { if ($out.Count -gt 0) { $out[$out.Count - 1].Trim() } else { "" } }

    while ($i -lt $lines.Count) {
        $line    = $lines[$i]
        $trimmed = $line.TrimStart()
        $indent  = $line.Substring(0, $line.Length - $trimmed.Length)

        # Guard adjacent __floor/__ceil forward-declarations.
        # MSVC 14.44+ treats these as intrinsics (C2169 / C7552).
        # Idempotency: skip if the preceding line already contains the guard.
        if ($trimmed -match '^double __floor\(double x\);' -and
            $i + 1 -lt $lines.Count -and
            $lines[$i + 1].TrimStart() -match '^double __ceil\(double x\);') {

            if ((Get-LastOutLine) -ne '#if !defined(_MSC_VER)') {
                $out.Add('#if !defined(_MSC_VER)')
                $out.Add($line)
                $out.Add($lines[$i + 1])
                $out.Add('#endif')
                $changed = $true
            } else {
                # Already guarded — copy lines verbatim.
                $out.Add($line)
                $out.Add($lines[$i + 1])
            }
            $i += 2
            continue
        }

        # Guard __floor function-pointer assignment; substitute floor on MSVC.
        if ($trimmed -match '^func->func_ptr\s*=\s*\(void\s*\*\)\s*__floor\s*;') {
            if ((Get-LastOutLine) -ne '#if !defined(_MSC_VER)') {
                $out.Add('#if !defined(_MSC_VER)')
                $out.Add($line)
                $out.Add('#else')
                $out.Add("${indent}func->func_ptr = (void*)floor;")
                $out.Add('#endif')
                $changed = $true
            } else {
                $out.Add($line)
            }
            $i++
            continue
        }

        # Guard __ceil function-pointer assignment; substitute ceil on MSVC.
        if ($trimmed -match '^func->func_ptr\s*=\s*\(void\s*\*\)\s*__ceil\s*;') {
            if ((Get-LastOutLine) -ne '#if !defined(_MSC_VER)') {
                $out.Add('#if !defined(_MSC_VER)')
                $out.Add($line)
                $out.Add('#else')
                $out.Add("${indent}func->func_ptr = (void*)ceil;")
                $out.Add('#endif')
                $changed = $true
            } else {
                $out.Add($line)
            }
            $i++
            continue
        }

        $out.Add($line)
        $i++
    }

    if ($changed) {
        [System.IO.File]::WriteAllLines($src, $out)
        Write-Host "Patched: $src"
        $anyPatched = $true
    } else {
        Write-Host "Already patched (no changes needed): $src"
    }
}

if (-not $anyPatched) {
    Write-Host "No files were modified."
}
