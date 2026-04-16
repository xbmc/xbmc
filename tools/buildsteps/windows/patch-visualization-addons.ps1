<#
.SYNOPSIS
    Patches addon C/C++ source files for MSVC compatibility.

.DESCRIPTION
    1. visualization.milkdrop / visualization.milkdrop2 — nseel-compiler.c
       MSVC 14.44+ treats __floor and __ceil as built-in intrinsic functions that cannot
       be redefined (error C2169 / C7552). The EEL2 compiler embedded in these addons
       declares and takes the address of these functions locally.
       Wraps those declarations and function-pointer assignments with
       #if !defined(_MSC_VER) guards so MSVC skips them and uses the standard
       library floor/ceil functions. Non-MSVC compilers (GCC, Clang) are unaffected.

    2. audiodecoder.ncsf — INFOSection.cpp
       Uses std::runtime_error without including <stdexcept> (error C2039/C3861).
       Injects #include <stdexcept> after the last existing #include in the file.

    The script is idempotent and exits with a non-zero code if any found file
    still contains unguarded problematic patterns after processing.

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

$exitCode = 0

# Returns the trimmed last line of $list, or "" if the list is empty.
function Get-LastOutLine([System.Collections.Generic.List[string]]$list) {
    if ($list.Count -gt 0) { $list[$list.Count - 1].Trim() } else { "" }
}

# ---------------------------------------------------------------------------
# Patch 1: visualization nseel-compiler.c — guard __floor/__ceil for MSVC
# ---------------------------------------------------------------------------
$nseelPatches = @(
    @{ addon = "visualization.milkdrop";  relpath = "lib\vis_milkdrop\ns-eel2\nseel-compiler.c" },
    @{ addon = "visualization.milkdrop2"; relpath = "lib\vis_milk2\ns-eel2\nseel-compiler.c"  }
)

foreach ($p in $nseelPatches) {
    $src = Join-Path $BuildDir "$($p.addon)\$($p.relpath)"

    if (-not (Test-Path $src)) {
        Write-Warning "[$($p.addon)] source not found at $src — skipping"
        continue
    }

    $lines = [System.IO.File]::ReadAllLines($src)
    $out   = [System.Collections.Generic.List[string]]::new()
    $i = 0
    $changed = $false

    while ($i -lt $lines.Count) {
        $line    = $lines[$i]
        $trimmed = $line.TrimStart()
        $indent  = $line.Substring(0, $line.Length - $trimmed.Length)

        # Guard __floor forward-declaration.
        # Matches with or without 'static', flexible whitespace, any parameter spelling.
        # MSVC 14.44+ treats __floor/__ceil as intrinsics (C2169 / C7552).
        if ($trimmed -match '^(?:static\s+)?double\s+__floor\s*\(') {
            # Look ahead (skipping blanks) for an adjacent __ceil declaration.
            $j = $i + 1
            while ($j -lt $lines.Count -and $lines[$j].Trim() -eq '') { $j++ }
            $nextTrimmed = if ($j -lt $lines.Count) { $lines[$j].TrimStart() } else { '' }
            $hasCeilNext = $nextTrimmed -match '^(?:static\s+)?double\s+__ceil\s*\('

            if ((Get-LastOutLine $out) -ne '#if !defined(_MSC_VER)') {
                $out.Add('#if !defined(_MSC_VER)')
                $out.Add($line)
                if ($hasCeilNext) {
                    for ($k = $i + 1; $k -le $j; $k++) { $out.Add($lines[$k]) }
                    $i = $j + 1
                } else {
                    $i++
                }
                $out.Add('#endif')
                $changed = $true
            } else {
                # Already guarded — copy verbatim.
                $out.Add($line)
                if ($hasCeilNext) {
                    for ($k = $i + 1; $k -le $j; $k++) { $out.Add($lines[$k]) }
                    $i = $j + 1
                } else {
                    $i++
                }
            }
            continue
        }

        # Standalone __ceil declaration (when it appears without __floor immediately before).
        if ($trimmed -match '^(?:static\s+)?double\s+__ceil\s*\(') {
            if ((Get-LastOutLine $out) -ne '#if !defined(_MSC_VER)') {
                $out.Add('#if !defined(_MSC_VER)')
                $out.Add($line)
                $out.Add('#endif')
                $changed = $true
            } else {
                $out.Add($line)
            }
            $i++
            continue
        }

        # Guard __floor function-pointer assignment; substitute floor on MSVC.
        if ($trimmed -match '^func\s*->\s*func_ptr\s*=\s*\(void\s*\*\)\s*__floor\s*;') {
            if ((Get-LastOutLine $out) -ne '#if !defined(_MSC_VER)') {
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
        if ($trimmed -match '^func\s*->\s*func_ptr\s*=\s*\(void\s*\*\)\s*__ceil\s*;') {
            if ((Get-LastOutLine $out) -ne '#if !defined(_MSC_VER)') {
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
        Write-Host "[$($p.addon)] Patched: $src"
    } else {
        Write-Host "[$($p.addon)] No changes needed: $src"
    }

    # Post-patch verification: scan the written file for any __floor/__ceil reference
    # that sits outside every preprocessor conditional block (depth == 0).
    $verify = [System.IO.File]::ReadAllLines($src)
    $depth = 0
    $verifyFailed = $false
    for ($vi = 0; $vi -lt $verify.Count; $vi++) {
        $vt = $verify[$vi].Trim()
        if ($vt -match '^#if') { $depth++; continue }
        if ($vt -match '^#endif') { if ($depth -gt 0) { $depth-- }; continue }
        if ($depth -eq 0 -and ($vt -match '__floor' -or $vt -match '__ceil')) {
            Write-Error "[$($p.addon)] VERIFICATION FAILED: unguarded __floor/__ceil at line $($vi + 1): $($verify[$vi])"
            $verifyFailed = $true
        }
    }

    if ($verifyFailed) {
        $exitCode = 1
    } else {
        Write-Host "[$($p.addon)] Verification passed."
    }
}

# ---------------------------------------------------------------------------
# Patch 2: audiodecoder.ncsf INFOSection.cpp — inject #include <stdexcept>
# ---------------------------------------------------------------------------
$ncsfSrc = Join-Path $BuildDir "audiodecoder.ncsf\lib\SSEQPlayer\INFOSection.cpp"

if (-not (Test-Path $ncsfSrc)) {
    Write-Warning "[audiodecoder.ncsf] source not found at $ncsfSrc — skipping"
} else {
    $lines = [System.IO.File]::ReadAllLines($ncsfSrc)

    $hasStdexcept = $lines | Where-Object { $_ -match '#include\s*[<"]stdexcept[>"]' }
    if ($hasStdexcept) {
        Write-Host "[audiodecoder.ncsf] #include <stdexcept> already present — no changes needed."
    } else {
        # Insert after the last #include in the file (or at line 0 if none).
        $lastIncludeIdx = -1
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -match '^\s*#include\s') { $lastIncludeIdx = $i }
        }
        $insertAt = if ($lastIncludeIdx -ge 0) { $lastIncludeIdx + 1 } else { 0 }

        $out = [System.Collections.Generic.List[string]]::new($lines)
        $out.Insert($insertAt, '#include <stdexcept>')
        [System.IO.File]::WriteAllLines($ncsfSrc, $out)
        Write-Host "[audiodecoder.ncsf] Injected #include <stdexcept> at line $($insertAt + 1) in $ncsfSrc"
    }

    # Verify the include is present.
    $verify = [System.IO.File]::ReadAllLines($ncsfSrc)
    $verified = $verify | Where-Object { $_ -match '#include\s*[<"]stdexcept[>"]' }
    if (-not $verified) {
        Write-Error "[audiodecoder.ncsf] VERIFICATION FAILED: #include <stdexcept> not found after patching."
        $exitCode = 1
    } else {
        Write-Host "[audiodecoder.ncsf] Verification passed."
    }
}

exit $exitCode
