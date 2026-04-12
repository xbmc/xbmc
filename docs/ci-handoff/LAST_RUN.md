# Last workflow run

Fetched with `tools\ci\run-ghworkflowfails.ps1` (see `README.md`).

## Run 44 — failure (addressed in `c5aeaac`)

- **Run id:** `24316561851` — commit `cd33acf` (docs-only on top of OpenSSL work)
- **Symptom:** Still **`ERROR: openssl not found`** after OpenSSL install step passed `ssl.h`.
- **Likely cause:** `OPENSSL_ROOT_DIR` not visible inside **`bash --login`** when only set via Actions `env:`; **`lib/VC`** layout may be nested (`lib/VC/x86/MD`, etc.), not only `lib/VC` or `lib/VC/static`.

## Fix pushed (`c5aeaac`)

- **`bash --login -e -c "export OPENSSL_ROOT_DIR=C:/OpenSSL-Win32 && exec ...make-mingwlibs.sh..."`** so the variable is set in the same shell as FFmpeg configure.
- **`buildffmpeg.sh`:** fallback `OPENSSL_ROOT_DIR` if `/c/OpenSSL-Win32` exists; **probe** multiple dirs for `libssl.lib` / `libcrypto.lib` and add each matching `-LIBPATH`.
- **Install step:** fail fast if **`libssl.lib`** is missing under the install root (full vs Light installer).

## Older context

- **Run 41:** `MSYS2_PATH_TYPE=inherit` for `cl` / PATH (see earlier commits).
- **DoD:** green job, `kodi.exe` in artifact, addons OK, release asset when that path runs.

## Next

- Watch the latest **`Leia`** run for `c5aeaac`; on failure run `.\tools\ci\run-ghworkflowfails.ps1` and check the new **Found: …libssl.lib** / verification lines in the log.
