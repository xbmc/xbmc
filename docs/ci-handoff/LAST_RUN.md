# Last workflow run

Fetched with `tools\ci\run-ghworkflowfails.ps1` (see `README.md`).

## Run 42 — failure (fixed in follow-up commits)

- **Run id:** `24316389176` — commit `cbd870d0f5` (MSYS2_PATH_TYPE inherit; drop redundant FFmpeg `--enable-cross-compile`)
- **Symptom:** FFmpeg configure passed the MSVC `cl` test, then **`ERROR: openssl not found`**.
- **Cause:** `ffmpeg_options.txt` still had `--disable-openssl` while `buildffmpeg.sh` added `--enable-openssl`; configure needs MSVC-style OpenSSL import libs on the runner (not only MSYS2 mingw packages).

## Run 41 — failure (addressed)

- **Run id:** `24314996453` — commit `931775454e` (tried `--enable-cross-compile` for `cl`)
- **Symptom:** `cl is unable to create an executable file` / `C compiler test failed` during FFmpeg configure.
- **Fix (cbd870d0f5):** Set **`MSYS2_PATH_TYPE=inherit`** on the FFmpeg build step so `bash --login` keeps PATH from `vcvars32.bat` (`cl`, `link`). Removed redundant unconditional `--enable-cross-compile` in `buildffmpeg.sh`.

## Next verification

- **Run in flight:** `24316531770` — https://github.com/OrganizerRo/xbmc/actions/runs/24316531770 — commit `f8b83c0` — Win32 OpenSSL (Shining Light) silent install to `C:\OpenSSL-Win32`, `OPENSSL_ROOT_DIR` + include/`lib/VC` paths in `buildffmpeg.sh`, and `do_removeOption "--disable-openssl"`.
- Re-run: `.\tools\ci\run-ghworkflowfails.ps1` after the run finishes if it fails.
- **DoD:** green job, `kodi.exe` in artifact, addons step OK, release asset when that path runs.
