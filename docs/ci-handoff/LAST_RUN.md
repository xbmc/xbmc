# Last CI runs (build-windows.yaml on `Leia`)

## Run 45 — failure (`2d72756`, run 24316974790)

- **URL:** https://github.com/OrganizerRo/xbmc/actions/runs/24316974790
- **Symptom:** FFmpeg configure: `ERROR: zlib requested but not found` → make fails → `failed to compile avcodec.lib`.
- **Cause:** `buildffmpeg.sh` adds `-I/depends/win32/include` and `-LIBPATH:/depends/win32/lib`, but no fstab mount for `/depends/win32/` and no deps were downloaded before the FFmpeg build step. `download-dependencies.bat` (which fetches zlib from mirrors.kodi.tv) ran *after* FFmpeg. Also, the later "Download build dependencies" step referenced nonexistent `DownloadBuildDeps.bat`.
- **Fix pushed:** (1) add MSYS2 fstab mounts for `/build`, `/downloads`, `/local32`, `/depends/win32`; (2) run `download-dependencies.bat` before FFmpeg build; (3) fix DownloadBuildDeps.bat → correct script name.

## Run 40 — failure (`2c25319`)

- **URL:** https://github.com/OrganizerRo/xbmc/actions/runs/24314881111  
- **Symptom:** `vcvars32` OK; FFmpeg configure still `C compiler test failed` with hint: *If cl is a cross-compiler, use --enable-cross-compile*.  
- **Next fix pushed:** add `--enable-cross-compile` in `buildffmpeg.sh` when using `--toolchain=msvc`.

## Run 39 — failure (`de7b484`)

- **URL:** https://github.com/OrganizerRo/xbmc/actions/runs/24314784404  
- **Symptom:** FFmpeg `configure` — `C compiler test failed` after patches applied (download + `/downloads` fix worked).  
- **Cause:** `buildffmpeg.sh` passes `--toolchain=msvc`; `cl.exe` was not on PATH in the MSYS2 bash session.  
- **Fix pushed:** `2c25319040` — `vcvars32.bat` via `vswhere` in the same `cmd` step before `bash --login` MinGW script.

## Earlier (for the next agent)

| Run | Commit | Failure |
|-----|--------|---------|
| 36 | symlink step | `ln` cannot create `/xbmc` on runner |
| 37–38 | MinGW | Missing `/downloads`; patch strip; then MSVC for configure |
| 35 | 172957a | Wrong script path `win32/make-mingwlibs.sh` (fixed to `windows/`) |

**PAT:** only `C:\src\xbmc\.git\organizerro_pat.txt` for API/git.
