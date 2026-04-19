# Kodi Leia (v18) — Windows Build Dependencies & Runtime Requirements

This document lists every DLL and runtime dependency that **kodi.exe** (Win32 / x86) needs in order to run,
where each file should be placed relative to the executable, and where it comes from during the CI build.

> **Source**: successful GitHub Actions build run for the `Leia` branch
> (`build-windows.yaml`), CMake configure log, and the dependency packages
> downloaded from `mirrors.kodi.tv/build-deps/win32/`.

---

## 1. Directory layout expected at runtime

Kodi resolves paths via `special://` protocol. On Windows:

| Special path | Resolves to |
|---|---|
| `special://xbmc/` | Directory containing `kodi.exe` |
| `special://xbmcbin/` | Same as `special://xbmc/` on Win32 |

So the expected directory tree next to `kodi.exe` is:

```
<app_root>/
├── kodi.exe
├── *.dll                        ← runtime dependency DLLs (see §2)
├── *.jar                        ← libbluray Java menu support
├── libdvdnav.dll                ← DVD navigation (built from source)
├── system/
│   ├── Python/                  ← Python 2.7 runtime
│   │   ├── DLLs/
│   │   ├── Lib/
│   │   └── Lib/site-packages/
│   ├── certs/                   ← TLS certificates
│   ├── keymaps/
│   ├── keyboardlayouts/
│   ├── library/
│   ├── settings/
│   ├── shaders/                 ← DirectX HLSL shaders
│   ├── addon-manifest.xml
│   ├── colors.xml
│   ├── peripherals.xml
│   ├── playercorefactory.xml
│   ├── IRSSmap.xml
│   └── X10-Lola-IRSSmap.xml
├── addons/                      ← built-in + binary add-ons
│   ├── skin.estuary/            (or skin.estouchy)
│   ├── metadata.themoviedb.org.python/
│   ├── pvr.*/                   ← PVR binary add-ons
│   ├── audiodecoder.*/
│   ├── inputstream.*/
│   └── ...
├── media/                       ← splash screen, icons, fonts
├── userdata/                    ← default user-data templates
└── sounds/                      ← UI sound resources (if present)
```

---

## 2. DLLs that must be beside kodi.exe

These are loaded at startup or at runtime from `special://xbmcbin/`.

### 2a. Pre-built dependency DLLs (from mirrors.kodi.tv packages)

The Leia dependency packages extract their DLLs into the source-tree's `system/` directory.
CMake's `export-files` target copies them to the build-tree directory beside `kodi.exe`.

| DLL | Source package | Purpose |
|---|---|---|
| `python27.dll` | python-2.7.13-win32-vc140 | Python 2.7 runtime (dynamically linked via import lib `python27.lib`) |
| `sqlite3.dll` | sqlite-3.10.2-win32-vc140 | SQLite database (**delay-loaded**) |
| `dnssd.dll` | dnssd-765.50.9-win32-vc140 | Bonjour / mDNS discovery (**delay-loaded**) |
| `libxslt.dll` | libxslt-1.1.29-win32-vc140 | XSLT transforms (**delay-loaded**) |
| `libxml2.dll` | libxml2-2.9.4-win32-vc140 | XML parsing |
| `libcurl.dll` | curl-7.59.0-Win32-v140 | HTTP / network |
| `libeay32.dll` | openssl-1.0.2g-win32-vc140 | OpenSSL crypto |
| `ssleay32.dll` | openssl-1.0.2g-win32-vc140 | OpenSSL SSL/TLS |
| `libmysql.dll` | mysql-connector-c-6.1.6-win32-vc140 | MySQL connector |
| `libmicrohttpd-dll.dll` | libmicrohttpd-0.9.55-win32-vc140 | Embedded web server |
| `libbluray.dll` | libbluray-1.0.2-win32-vc140 | Blu-ray disc playback |
| `cec.dll` | libcec-4.0.2-Win32-v141 | HDMI-CEC control |
| `nfs.dll` | libnfs-3.0.0-Win32-v141 | NFS file access |
| `libass.dll` | libass-d18a5f1-win32-vc140 | ASS/SSA subtitle rendering |
| `freetype6.dll` | freetype-db5a22-win32-vc140 | Font rendering |
| `libfribidi.dll` | libfribidi-0.19.2-win32 | BiDi text layout |
| `libcdio.dll` | libcdio-0.9.3-win32-vc140 | CD-ROM access |
| `libiconv.dll` | libiconv-1.14-win32-vc140 | Character encoding conversion |
| `zlib1.dll` | zlib-1.2.8-win32-vc140 | Compression (delay-loaded as `zlib.dll`) |
| `libpng16.dll` | libpng-1.6.21-win32-vc140 | PNG image decoding |
| `jpeg62.dll` / `turbojpeg.dll` | libjpeg-turbo-1.4.90-win32-vc140 | JPEG image decoding |
| `giflib.dll` | giflib-5.1.4-win32-vc140 | GIF image decoding |
| `expat.dll` | expat-2.2.0-win32-vc140 | XML parsing (Expat) |
| `pcre.dll` / `pcrecpp.dll` | pcre-8.37-win32-vc140 | Regular expressions |
| `CrossGuid.dll` | crossguid-8f399e-win32-vc140 | GUID generation |
| `tag.dll` | taglib-1.11.1-win32-vc140 | Audio tag metadata |
| `tinyxmlSTL.dll` | tinyxmlstl-2.6.2-win32-vc140 | TinyXML with STL |
| `lcms2.dll` | lcms2-2.8-win32-vc140 | ICC color management |
| `fstrcmp.dll` | fstrcmp-0.7-Win32-v141 | Fuzzy string comparison |
| `shairplay.dll` | shairplay-0.9.0-win32-vc140 | AirPlay support |
| `libplist.dll` | libplist-1.13.0-win32-vc140 | Apple plist parsing |
| `lzo2.dll` | lzo-2.09-win32-vc140 | LZO compression |
| `EasyHook32.dll` | easyhook-2.7.5870.0-win32-vc140 | DLL injection / hooking |
| `libyajl.dll` | libyajl-2.0.1-win32 | JSON parsing |
| `libssh.dll` | libssh-0.7.0-win32-vc140 | SSH/SFTP file access |

### 2b. MinGW-built DLLs (built from source in CI)

| DLL | Source | Purpose |
|---|---|---|
| `libdvdnav.dll` | Built via `make-mingwlibs.sh` → `buildlibdvd.sh` | DVD navigation (`DllPaths_win32.h`) |

> **Note**: FFmpeg is built with `--disable-shared` (static linking), so there are no FFmpeg DLLs to ship.

### 2c. Delay-loaded DLLs

Defined in `cmake/scripts/windows/ArchSetup.cmake`:

```text
libmariadb.dll  libxslt.dll  dnssd.dll  dwmapi.dll  sqlite3.dll  d3dcompiler_47.dll
```

These are loaded on demand. Kodi won't crash if they're absent, but the corresponding feature will be unavailable.

### 2d. Windows system DLLs (linked, not shipped)

From `ArchSetup.cmake` `DEPLIBS`:

```text
bcrypt  d3d11.dll  DInput8.dll  DSound.dll  winmm.dll  Mpr.dll
Iphlpapi.dll  ws2_32.dll  PowrProf.dll  setupapi.dll  Shlwapi.dll
dwmapi.dll  dxguid.dll  RuntimeObject.dll  DelayImp.lib
```

These are provided by Windows itself and do not need to be shipped.

### 2e. Visual C++ Redistributable

Kodi links against the MSVC 2015+ runtime. Users must install the
**Visual C++ Redistributable for Visual Studio 2015–2022 (x86)** which provides:

- `msvcp140.dll`
- `vcruntime140.dll`
- `ucrtbase.dll`

---

## 3. Python 2.7 runtime

Kodi Leia embeds Python 2.7. The distribution is expected at `system/Python/` relative
to `kodi.exe` (see `xbmc/platform/win32/PlatformWin32.cpp`):

```
system/Python/
├── DLLs/        ← compiled extension modules (.pyd)
├── Lib/         ← standard library
└── Lib/site-packages/
    ├── PIL/     ← Pillow (from pillow-3.1.0-win32-vc140.7z)
    └── Crypto/  ← PyCryptodome (from pycryptodome-3.4.3-win32.7z)
```

The `python27.dll` itself must be beside `kodi.exe` (not inside `system/Python/`).

---

## 4. Data directories

| Directory | Source (cmake installdata) | Contents |
|---|---|---|
| `system/keymaps/` | `common/common.txt` | Keyboard & remote mappings |
| `system/settings/` | `common/common.txt` | Settings definitions XML |
| `system/shaders/` | `common/common.txt` | DirectX HLSL shaders |
| `system/library/` | `common/common.txt` | Smart playlist templates |
| `system/keyboardlayouts/` | `common/common.txt` | Keyboard layouts |
| `system/certs/` | `common/certificates.txt` | TLS CA certificates |
| `addons/` | `common/addons.txt` | Built-in addons (skins, scrapers, etc.) |
| `media/` | `common/common.txt` | Splash screen, icons, fonts |
| `userdata/` | `common/common.txt` | Default user config templates |

---

## 5. How the CI build assembles these

### 5a. Dependency packages (mirrors.kodi.tv)

`tools/buildsteps/windows/download-dependencies.bat` downloads packages listed in
`project/BuildDependencies/scripts/0_package.native-win32.list` (build tools) and
`project/BuildDependencies/scripts/0_package.target-win32.list` (runtime libraries)
and extracts them to `project/BuildDependencies/win32/`.

The `get_formed.cmd` script **re-arranges** old-format packages during extraction:
- `project/BuildDependencies/lib/` → `lib/` (flattened)
- `system/*.dll` → `bin/*.dll` (DLLs moved to bin directory)
- `Win32/` or `x64/` → flattened to root

After extraction and re-arrangement, the flat structure is:
- `project/BuildDependencies/win32/lib/` — static/import libraries (`.lib`)
- `project/BuildDependencies/win32/include/` — headers
- `project/BuildDependencies/win32/bin/` — runtime DLLs (`.dll`) and Python runtime
- `project/BuildDependencies/tools/` — native build tools (doxygen, swig, etc.)

### 5b. CMake export-files target

`CMakeLists.txt` calls `copy_files_from_filelist_to_buildtree()` which reads
`cmake/installdata/windows/dlls.txt`, `cmake/installdata/windows/python.txt`,
`cmake/installdata/windows/addons.txt`, `cmake/installdata/windows/irss.txt`,
`cmake/installdata/common/common.txt`, `cmake/installdata/common/addons.txt`,
and `cmake/installdata/common/certificates.txt`.

Files are copied to `${CMAKE_BINARY_DIR}/` (= `kodi-build/`) — the **build tree root**,
NOT to the MSVC config subdirectory. With Visual Studio multi-config generators,
the directory layout after building is:

```
kodi-build/                         ← CMAKE_BINARY_DIR
├── RelWithDebInfo/
│   └── kodi.exe                    ← MSVC puts the executable here
├── *.dll                           ← dependency DLLs (from dlls.txt)
├── *.jar                           ← libbluray JARs (from dlls.txt)
├── system/
│   ├── Python/                     ← Python 2.7 runtime (from python.txt)
│   ├── keymaps/                    ← from common/common.txt
│   ├── settings/                   ← from common/common.txt
│   ├── shaders/                    ← from common/common.txt
│   ├── library/                    ← from common/common.txt
│   ├── keyboardlayouts/            ← from common/common.txt
│   ├── certs/                      ← from common/certificates.txt
│   ├── IRSSmap.xml                 ← from windows/irss.txt
│   └── X10-Lola-IRSSmap.xml       ← from windows/irss.txt
├── addons/                         ← built-in addons (from common/addons.txt)
├── media/                          ← splash, icons, fonts (from common/common.txt)
├── userdata/                       ← default user config (from common/common.txt)
└── sounds/                         ← UI sounds (from common/common.txt)
```

### 5c. Packaging

The "Collect build output" step must collect from **both** locations:
1. `kodi-build/RelWithDebInfo/` — for `kodi.exe`
2. `kodi-build/` root — for DLLs, `system/`, `addons/`, `media/`, `userdata/`, `sounds/`

It also merges binary addon build output from `project/Win32BuildSetup/BUILD_WIN32/addons/`
into `staging/addons/`.

The "Package release zip" step creates `kodi-windows-x86.zip` from the staging directory.

---

## 6. Known issues and fixes (as of this writing)

| Issue | Root cause | Fix applied |
|---|---|---|
| Dependency DLLs missing from build | `get_formed.cmd` on our branch was outdated — didn't re-arrange old packages | Synced `get_formed.cmd` from Leia branch (moves `system/*.dll` → `bin/*.dll`) |
| Wrong package lists used | Old `get_formed.cmd` read `0_package.list`; new one reads `0_package.native-*.list` + `0_package.target-*.list` | Updated `get_formed.cmd` and package list files |
| `WGET` variable not set | `download-dependencies.bat` was missing `SET WGET=%BUILD_DEPS_PATH%\bin\wget` | Synced `download-dependencies.bat` from Leia branch |
| `libdvdnav.dll` not in build tree | cmake ExternalProject installs to `kodi-build/build/libdvd/bin/` but `FindLibDvd.cmake` copies from `${DEPENDS_PATH}/bin/` | Added fallback copy in workflow collect step |
| `/D_CRT_NONSTDC_NO_DEPRECATE` missing | CFlagOverrides.cmake/CXXFlagOverrides.cmake didn't include it | Added to both override files |
| Binary addons not in release zip | Addon binaries built to `project/Win32BuildSetup/BUILD_WIN32/addons/` not merged into staging | Collect step now merges addon binaries into `staging/addons/` |
| DLLs/system/media missing from staging | Collect step only copied from `kodi-build/RelWithDebInfo/` which only has `kodi.exe`; DLLs and data dirs are at `kodi-build/` root | Collect step now copies kodi.exe from RelWithDebInfo/ and DLLs/system/media/userdata/addons/sounds from kodi-build/ root |
