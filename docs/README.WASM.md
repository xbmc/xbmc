![Kodi Logo](resources/banner_slim.png)

# WebAssembly (WASM) build guide
**This guide is a work in progress.** It currently documents only how to build Kodi's **tools and dependencies** for the `wasm32-unknown-emscripten` target. Building the Kodi application itself and binary add-ons for WASM is not yet supported and will be documented once the respective bring-up work lands.

The guide is meant to cross-compile Kodi's dependencies for WebAssembly using **[Kodi's unified depends build system](../tools/depends/README.md)**. Please read it in full before you proceed to familiarize yourself with the build procedure.

This guide has been tested only on **macOS** with **Emscripten SDK 5.0.6**. Other host operating systems and other Emscripten versions may work but are currently unverified.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
  2.1. **[Install the Emscripten SDK](#21-install-the-emscripten-sdk)**
  2.2. **[Activate the Emscripten environment](#22-activate-the-emscripten-environment)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Build tools and dependencies](#4-build-tools-and-dependencies)**
  4.1. **[Advanced Configure Options](#41-advanced-configure-options)**

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
* The **[Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)** at version **5.0.6** (installable via Homebrew — see **[2.1. Install the Emscripten SDK](#21-install-the-emscripten-sdk)**).

Install Xcode Command Line Tools and the remaining host tools:
```
xcode-select --install
```

### 2.1. Install the Emscripten SDK
Install Emscripten with Homebrew (recommended on macOS):
```
brew install emscripten
```

> [!WARNING]
> Kodi's WASM bring-up has only been tested against Emscripten **5.0.6**. Other versions may work but are unverified; if you hit an unexpected build failure, please try 5.0.6 before reporting it. You can check the version shipped by Homebrew with `brew info emscripten`.

**Alternatively**, install **emsdk 5.0.6** manually if you need to pin an exact version or use a different host OS:
```
git clone https://github.com/emscripten-core/emsdk.git $HOME/emsdk
cd $HOME/emsdk
./emsdk install 5.0.6
./emsdk activate 5.0.6
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
