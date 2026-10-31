![Kodi Logo](resources/banner_slim.png)

# WebAssembly (WASM) build guide
**This guide is a work in progress.** Kodi builds, runs in a browser and packages as a Tizen web app for Samsung TVs, but the port is still being brought up — see **[Known Incomplete Areas](#known-incomplete-areas)**.

The guide covers cross-compiling Kodi's dependencies and the application itself for the `wasm32-unknown-emscripten` target using **[Kodi's unified depends build system](../tools/depends/README.md)**, running the result in a browser, and packaging it for a Samsung TV. Please read it in full before you proceed to familiarize yourself with the build procedure.

Installing the Tizen SDK, pairing a TV and creating the Samsung signing certificates are one-time steps documented separately in **[README.Tizen.md](README.Tizen.md)**. Binary add-ons are out of scope; they are not built for WASM.

This guide targets **Emscripten SDK 6.0.13**. The host setup instructions have been tested only on **macOS**; other host operating systems are currently unverified.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
  2.1. **[Install the Emscripten SDK](#21-install-the-emscripten-sdk)**
  2.2. **[Activate the Emscripten environment](#22-activate-the-emscripten-environment)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Build tools and dependencies](#4-build-tools-and-dependencies)**
  4.1. **[Advanced Configure Options](#41-advanced-configure-options)**
5. **[Build Kodi](#5-build-kodi)**
6. **[Run in a browser](#6-run-in-a-browser)**
7. **[Package and install on a Samsung TV](#7-package-and-install-on-a-samsung-tv)**
  7.1. **[Stage the Tizen web app](#71-stage-the-tizen-web-app)**
  7.2. **[Build the WGT package](#72-build-the-wgt-package)**
  7.3. **[Install on the TV](#73-install-on-the-tv)**
  7.4. **[Launch, debug and remove](#74-launch-debug-and-remove)**

## 1. Document conventions
This guide assumes you are using `terminal`, also known as `console`, `command-line` or simply `cli`. Commands need to be run at the terminal, one at a time and in the provided order.

This is a comment that provides context:
```
this is a command
this is another command
and yet another one
```

**Example:** Clone Kodi's current master branch:
```
git clone https://github.com/xbmc/xbmc kodi
```

Commands that contain strings enclosed in angle brackets denote something you need to change to suit your needs.
```
git clone -b <branch-name> https://github.com/xbmc/xbmc kodi
```

**Example:** Clone Kodi's current Krypton branch:
```
git clone -b Krypton https://github.com/xbmc/xbmc kodi
```

Several different strategies are used to draw your attention to certain pieces of information. In order of how critical the information is, these items are marked as a note, tip, or warning. For example:

> [!NOTE]
> Linux is user friendly... It's just very particular about who its friends are.

> [!TIP]
> Algorithm is what developers call code they do not want to explain.

> [!WARNING]
> Developers don't change light bulbs. It's a hardware problem.

**[back to top](#table-of-contents)** | **[back to section top](#1-document-conventions)**

## 2. Prerequisites
* **macOS** with **Xcode Command Line Tools** installed.
* **[Homebrew](https://brew.sh/)** (or **[MacPorts](https://www.macports.org/)**) to install the host build tools: **[Git](https://git-scm.com/)**, **[CMake](https://cmake.org/)** (3.20 or newer), **[Python 3](https://www.python.org/)**, `autoconf`, `automake`, `libtool`, `pkg-config`, `gperf` and GNU `make`.
* The **[Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)** at version **6.0.13** (see **[2.1. Install the Emscripten SDK](#21-install-the-emscripten-sdk)**).

Install Xcode Command Line Tools and the remaining host tools:
```
xcode-select --install
```

### 2.1. Install the Emscripten SDK
Install Emscripten with Homebrew (recommended on macOS):
```
brew install emscripten
```

> [!NOTE]
> Check the version shipped by Homebrew with `brew info emscripten`. To use the documented version **6.0.13** exactly, install it with emsdk as shown below.

> [!NOTE]
> The Tizen sockets backend needs Emscripten **6.0.10** or later, whose `freeaddrinfo()` frees every result; see **[docs/wasm/SOCKETS.md](wasm/SOCKETS.md)**. CMake warns about older versions.

**Alternatively**, install **Emscripten 6.0.13** with emsdk if you need to pin an exact version or use a different host OS:
```
git clone https://github.com/emscripten-core/emsdk.git $HOME/emsdk
cd $HOME/emsdk
./emsdk install 6.0.13
./emsdk activate 6.0.13
```

### 2.2. Activate the Emscripten environment
The Emscripten toolchain (`emcc`, `em++`, `emconfigure`, `emmake`, ...) must be on `PATH` for the depends build to pick it up.

If you installed via Homebrew, `emcc` is already on `PATH` and no extra step is required. Verify it is reachable:
```
emcc --version
```

If you installed emsdk manually, source the environment script in **every** shell you use to build Kodi:
```
source $HOME/emsdk/emsdk_env.sh
```

> [!TIP]
> The `emsdk_env.sh` script only exports variables for the current shell. Re-run it (or add it to your shell's startup file) whenever you open a new terminal.

**[back to top](#table-of-contents)** | **[back to section top](#2-prerequisites)**

## 3. Get the source code
Change to your `home` directory:
```
cd $HOME
```

Clone Kodi's current master branch:
```
git clone https://github.com/xbmc/xbmc kodi
```

**[back to top](#table-of-contents)**

## 4. Build tools and dependencies
The WASM depends are built in `$HOME/kodi/tools/depends` and installed under the prefix you pass to `configure`. This guide uses `$HOME/kodi-wasm-depends`.

> [!NOTE]
> `--host=wasm32-unknown-emscripten` and `--with-platform=wasm` are both required. The Emscripten SDK must be activated first (see **[2.2. Activate the Emscripten environment](#22-activate-the-emscripten-environment)**).

Prepare to configure build:
```
cd $HOME/kodi/tools/depends
./bootstrap
```

Configure build:
```
./configure \
  --host=wasm32-unknown-emscripten \
  --with-platform=wasm \
  --prefix=$HOME/kodi-wasm-depends \
  --disable-debug
```

Build tools and dependencies:
```
make -j$(getconf _NPROCESSORS_ONLN)
```

> [!TIP]
> By adding `-j<number>` to the make command, you can choose how many concurrent jobs will be used and expedite the build process. It is recommended to use `-j$(getconf _NPROCESSORS_ONLN)` to compile on all available processor cores. The build machine can also be configured to do this automatically by adding `export MAKEFLAGS="-j$(getconf _NPROCESSORS_ONLN)"` to your shell config (e.g. `~/.bashrc`).

> [!WARNING]
> Look for the `Dependencies built successfully.` success message. If in doubt run a single threaded `make` command until the message appears. If the single make fails, clean the specific library by issuing `make -C target/<name_of_failed_lib> distclean` and run `make` again.

Once the build finishes, the per-library install trees and the generated toolchain files (`Toolchain.cmake`, `config.site`, and the Meson cross file) are available under the WASM depends prefix:
```
$HOME/kodi-wasm-depends/wasm32-unknown-emscripten-release/
```

### 4.1. Advanced Configure Options


**All platforms:**

```
--prefix=<path>
```
  install path for built tools and dependencies

```
--with-toolchain=<path>
```
  specify path to toolchain. Auto set for android. Defaults to xcode root for darwin, /usr for linux

```
--enable-debug=<yes:no>
```
  enable debugging information (default is yes)

```
--disable-ccache
```
  disable ccache

```
--with-tarballs=<path>
```
  path where tarballs will be saved [prefix/xbmc-tarballs]

```
--with-cpu=<cpu>
```
  optional. specify target cpu. guessed if not specified

```
--with-linker=<linker>
```
  specify linker to use. (default is ld)

```
--with-platform=<platform>
```
  target platform

```
--enable-gplv3=<yes:no>
```
  enable gplv3 components. (default is yes)

```
--with-target-cflags=<cflags>
```
  C compiler flags (target)

```
--with-target-cxxflags=<cxxflags>
```
  C++ compiler flags (target)

```
--with-target-ldflags=<ldflags>
```
  linker flags. Use e.g. for -l<lib> (target)

```
--with-ffmpeg-options
```
  FFmpeg configure options, e.g. --enable-vaapi (target)

**WASM Specific:**

For WASM, the toolchain (`emcc`/`em++`) is picked up from the active Emscripten SDK environment. No WASM-only configure switches are required beyond `--host=wasm32-unknown-emscripten` and `--with-platform=wasm`.

**[back to top](#table-of-contents)** | **[back to section top](#4-build-tools-and-dependencies)**

## 5. Build Kodi
Generate the build files. CMake and the toolchain file both come from the depends build, so nothing extra has to be installed:
```
make -C $HOME/kodi/tools/depends/target/cmakebuildsys BUILD_DIR=$HOME/kodi/build-wasm
```

> [!TIP]
> `BUILD_DIR` can be omitted, in which case the project is generated in `$HOME/kodi/build`. Adjust the paths below accordingly. Extra CMake arguments can be passed with `CMAKE_EXTRA_ARGUMENTS`.

> [!WARNING]
> If you intend to package for a TV, export `TIZEN_SDK` **before** this step. The Tizen packaging targets are only created when CMake finds the `tz` CLI while configuring — see **[7. Package and install on a Samsung TV](#7-package-and-install-on-a-samsung-tv)**.

Build:
```
make -C $HOME/kodi/build-wasm -j$(getconf _NPROCESSORS_ONLN)
```

The build produces three files in the build directory:

| File | Contents |
|---|---|
| `kodi.js` | Emscripten loader and JS glue |
| `kodi.wasm` | Compiled Kodi |
| `kodi.data` | Virtual filesystem image: `addons`, `media`, `system` and `userdata` mounted at `/kodi`, plus the Python standard library |

**[back to top](#table-of-contents)**

## 6. Run in a browser
Kodi's WASM build uses threads, so it needs `SharedArrayBuffer`, which browsers only expose to cross-origin isolated pages. `tools/wasm/serve.py` is a small static server that sends the required COOP/COEP headers, and proxies http(s) sources that do not send CORS headers of their own at `/proxy?u=<url-encoded>`. Kodi only routes its cross-origin requests through the proxy when configured with `-DENABLE_WASM_DEV_PROXY=ON`:
```
cd $HOME/kodi/build-wasm
cp ../tools/wasm/kodi.html .
python3 ../tools/wasm/serve.py
```

Open the address the server prints, `http://127.0.0.1:8080/kodi.html`. Pass a port to use another one, or a `host:port` to also serve this directory as a media source to another Kodi on the LAN:
```
python3 ../tools/wasm/serve.py 0.0.0.0:8080
```

Keep opening the launcher from `127.0.0.1` or `localhost`: over plain http, any other address is not a secure context, so the browser withholds `SharedArrayBuffer`.

> [!NOTE]
> WebGL 2 is required. Audio only starts after a first user interaction (keyboard, mouse or touch), which is the browser's autoplay policy rather than a Kodi limitation.

On TitanOS, the shared launcher loads the [Titan SDK](https://docs.titanos.tv/titan-sdk-cdn)
and queries device information before starting Kodi. System Information reports
the brand, hardware platform and firmware; the OS version comes from the
`TitanOS/` user-agent token. The network page also shows the MAC address from
`Product.mac` when the SDK supplies a valid address at startup. SDK initialization
has a five-second timeout and falls back to generic browser information on
failure. Ordinary browsers and Samsung TVs do not load the Titan SDK.

This integration covers device information, not a complete TitanOS port. It
requires a `TitanOS/` user-agent token or an already loaded `window.TitanSDK`.
Older TVs without either use the browser fallback. The browser requirements
above still apply; TitanOS hardware compatibility has not been verified.
Screensaver inhibition uses the browser Screen Wake Lock API where available.

**[back to top](#table-of-contents)**

## 7. Package and install on a Samsung TV
The Tizen web app wrapper is built from templates in `tools/wasm/tizen/`, described in **[its own README](../tools/wasm/tizen/README.md)**.

This section assumes **[README.Tizen.md](README.Tizen.md)** has been followed: the SDK is installed, the TV is paired (`sdb devices` lists it), and a Samsung signing profile is active.

`tz` is located through these environment variables, in order, before falling back to `PATH`:

- `TIZEN_CLI_PATH`
- `TIZEN_TOOLS_PATH`
- `TIZEN_SDK`
- `TIZEN_SDK_ROOT`

> [!WARNING]
> The lookup happens at configure time. If CMake was configured without `tz` in reach it prints `WASM: tz CLI not found` and the `package_tizen_wgt`, `install_tizen_wgt` and `inspect_tizen` targets do not exist (`inspect_tizen` also needs `sdb`). Export the variable and re-run the generation step from **[5. Build Kodi](#5-build-kodi)**.

### 7.1. Stage the Tizen web app
```
make -C $HOME/kodi/build-wasm package_tizen
```

This assembles a complete Tizen web project in `$HOME/kodi/build-wasm/tizen_packaging/Kodi/`: the template files, `index.html` (a copy of the shared browser shell `tools/wasm/kodi.html`), `media/splash.jpg`, and the three build artifacts. The directory can also be opened directly in Tizen Studio.

Staging needs no SDK — only the two steps below do.

### 7.2. Build the WGT package
```
make -C $HOME/kodi/build-wasm package_tizen_wgt
```

The signed package is written to:
```text
$HOME/kodi/build-wasm/tizen_packaging/Kodi/Debug/Kodi.wgt
```

Signing uses whichever `tz` profile is currently active — `tizen_web_project.yaml` deliberately leaves `signing_profile` empty. Check it with `tz security-profiles list`.

### 7.3. Install on the TV
```
make -C $HOME/kodi/build-wasm install_tizen_wgt
```

With more than one device connected, name the target at configure time with either:

| CMake variable | Environment fallback | `tz install` argument |
|---|---|---|
| `WASM_TIZEN_INSTALL_SERIAL` | `TIZEN_TARGET_SERIAL` | `--serial` |
| `WASM_TIZEN_INSTALL_TARGET` | `TIZEN_TARGET_NAME` | `--target` |

The serial wins when both are set. Because the values are read while configuring, change them with `cmake -D...` on the existing build directory rather than exporting into the build shell.

### 7.4. Launch, debug and remove
Launching and removing go through `tz` directly, and the two take the package ID in different forms:
```
tz run -p kodiplayer              # package ID alone
tz uninstall -p kodiplayer.Kodi   # package ID and the yaml output_name
```

Neither accepts the `<tizen:application>` ID from `config.xml`. The IDs come from `tools/wasm/tizen/config.xml` (`<tizen:application package="...">`) and the `output_name` in `tizen_web_project.yaml`.

With more than one device connected, add `--serial <serial>` or `--target <name>` to each `tz` command, as for `tz install`.

Retail TVs expose no application log over `sdb`. To get console output from the WASM runtime, launch in debug mode, which runs the app under the Web Inspector:
```
tz run -d -p kodiplayer
```

Debug mode makes the TV open a DevTools port for the app; it changes on every launch and has to be forwarded over `sdb` before Chrome can reach it. `tools/wasm/tizen/inspect.sh` does the whole sequence — launch, parse the port, forward it to `localhost:7011` — and is also available as a build target, which selects the TV by `WASM_TIZEN_INSTALL_SERIAL` only:
```
cmake --build $HOME/kodi/build-wasm --target inspect_tizen
# or directly, with an optional device serial from `sdb devices`:
TIZEN_TARGET_SERIAL=192.168.1.51:26101 tools/wasm/tizen/inspect.sh
```

Then, in Chrome on the development machine, open `chrome://inspect`, click **Configure...** next to *Discover network targets*, add `localhost:7011`, and click **inspect** on the Kodi target. This uses Chrome's own DevTools frontend; the page the TV serves at `http://localhost:7011/` lists the same targets but ships a DevTools frontend old enough to render blank in a current Chrome.

What the inspector gives you on a retail TV:
* **Console** — everything Kodi logs (`SetLogTarget("console")`). Kodi is already running when the inspector attaches, so startup output is missed.
* **Performance / Memory** — main-thread activity and JS heap. GPU memory is not exposed; infer it from growth in frame time or from the app being killed.
* **Application → Storage** — the IndexedDB the Emscripten filesystem persists userdata into.

Configure the build with `-DENABLE_WASM_PROFILING=ON` to preserve wasm function names for profiling in DevTools.

Under the hood the script runs `sdb shell 0 debug kodiplayer.Kodi`, which prints `port: <n>`, followed by `sdb forward tcp:7011 tcp:<n>`. Override the app ID with `KODI_TIZEN_APP_ID` and the local port with `KODI_TIZEN_INSPECT_PORT`.

If the launch reports `with debug 0` (`tz`) or answers `closed` (`sdb`), the TV declined to open a debug port even though the app is installed and signed with a Samsung developer certificate. Check on the TV that Developer mode is on and its Host PC IP is this machine's address, then reboot the TV; a 2025 Tizen 9.0 set still refused after both, and the cause there is not yet understood.

**[back to top](#table-of-contents)** | **[back to section top](#7-package-and-install-on-a-samsung-tv)**

## Audio and video playback

Audio goes out through a Web Audio `AudioWorklet` (`-sAUDIO_WORKLET` and `-sWASM_WORKERS`, which need the same COOP/COEP headers as pthreads). Video decoding uses the browser's WebCodecs decoder for streams it supports, and FFmpeg otherwise.

Two browser rules show up as behaviour:

- Audio only starts after the first user interaction (keyboard, mouse or touch); this is the browser's autoplay policy.
- `-sSINGLE_FILE` is not compatible with Emscripten's Audio Worklet path.

For manual validation use at least one H.264 MP4 and one VP9 WebM sample and check that playback starts in sync, that pause/resume and repeated seeks keep lip-sync, and that end of stream drains cleanly.

## Samsung TV information

On a Samsung TV the wasm build reads the Samsung product APIs (`webapis.network`, `webapis.productinfo`), which `tools/wasm/kodi.html` loads from `$WEBAPIS/webapis/webapis.js` only when it detects the Tizen runtime. Their privileges, `network.public` and `productinfo`, are declared in `tools/wasm/tizen/config.xml`; both are public-level, so the Public distributor certificate from **[README.Tizen.md](README.Tizen.md)** is sufficient.

With them, **Settings > System information** shows:

- **Network**: link state, MAC address, IP address, subnet mask, gateway and both DNS servers of the active connection. The hostname reported to other devices (for example in the default device name) is the TV name from the network API.
- **Summary**: the operating system as `Tizen <platform version>`.
- **Hardware**: the TV model and firmware version.

The connection type (Ethernet or Wi-Fi with SSID, signal level and band), the IP mode (DHCP or static) and whether the gateway is reachable are written to `kodi.log` whenever they change. The Samsung API does not report the link speed or the supported audio and video codecs.

In an ordinary browser none of these APIs exist; the network page then shows loopback placeholders and the connection state follows `navigator.onLine`.

## Known Incomplete Areas

- Some dependency recipes still need WASM validation/tuning in `tools/depends/target/*`.
- Binary add-ons are not built for the WASM target.
