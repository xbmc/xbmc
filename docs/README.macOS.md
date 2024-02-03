![Kodi Logo](resources/banner_slim.png)

# macOS build guide
This guide has been tested using Xcode 11.3.1 running on MacOS 10.14.4 (Mojave). Please note this combination is the only version our CI system builds. The minimum OS requirement for this version of Xcode is MacOS 10.14.4. Other combinations may work but we provide no assurances that other combinations will build correctly and run identically to Team Kodi releases. It is meant to build Kodi for macOS using **[Kodi's unified depends build system](../tools/depends/README.md)**. Please read it in full before you proceed to familiarize yourself with the build procedure.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Configure and build tools and dependencies](#4-configure-and-build-tools-and-dependencies)**  
  4.1. **[Advanced Configure Options](#41-advanced-configure-options)**  
5. **[Build binary add-ons](#5-build-binary-add-ons)**
6. **[Build Kodi](#6-build-kodi)**  
  6.1. **[Build with Xcode](#61-build-with-xcode)**  
  6.2. **[Build with xcodebuild](#62-build-with-xcodebuild)**  
  6.3. **[Build with make](#63-build-with-make)**
7. **[Run Kodi](#7-run-kodi)**  
  7.1. **[Built with Xcode or xcodebuild](#71-built-with-xcode-or-xcodebuild)**  
  7.2. **[Built with make](#72-built-with-make)**
8. **[Package](#8-package)**
9. **[Install](#9-install)**

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
* **[Java Development Kit (JDK)](http://www.oracle.com/technetwork/java/javase/downloads/index.html)**
* **[Xcode](https://developer.apple.com/xcode/)**. Install it from the AppStore or from the **[Apple Developer Homepage](https://developer.apple.com/)**.
* Device with **OSX 10.14 or newer** to run Kodi after build.

Building for OSX/macOS should work with the following constellations of Xcode and OSX/macOS versions:
  * Xcode 12.4 against MacOSX SDK 11.1 on 10.15.7 (Catalina)(recommended)(CI)
  * Xcode 13.x against MacOSX SDK 12.3 on 12.x (Monterey)(recommended)

Team Kodi CI infrastructure is limited, and therefore we only have the single combination tested. Newer xcode/macos combinations generally should work, however the team does not actively test/use pre-release versions, so use with caution. Earlier versions may work, however we dont actively support them, so use with caution.
> [!WARNING]  
> Start Xcode after installation finishes. You need to accept the licenses and install missing components.

**[back to top](#table-of-contents)**

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

## 4. Configure and build tools and dependencies
Kodi can be built as either a 32bit or 64bit program. The dependencies are built in `$HOME/kodi/tools/depends` and installed into `/Users/Shared/xbmc-depends`.

> [!TIP]
> Look for comments starting with `Or ...` and only execute the command(s) you need.

> [!NOTE]  
> `--with-platform` is mandatory for all Apple platforms

Configure build (x86 intel):
```
cd $HOME/kodi/tools/depends
./bootstrap
./configure --host=x86_64-apple-darwin --with-platform=macos
```

Configure build (apple silicon):
```
cd $HOME/kodi/tools/depends
./bootstrap
./configure --host=aarch64-apple-darwin --with-platform=macos
```

Build tools and dependencies:
```
make -j$(getconf _NPROCESSORS_ONLN)
```

> [!TIP]
> By adding `-j<number>` to the make command, you can choose how many concurrent jobs will be used and expedite the build process. It is recommended to use `-j$(getconf _NPROCESSORS_ONLN)` to compile on all available processor cores. The build machine can also be configured to do this automatically by adding `export MAKEFLAGS="-j$(getconf _NPROCESSORS_ONLN)"` to your shell config (e.g. `~/.bashrc`).

> [!WARNING]  
> Look for the `Dependencies built successfully.` success message. If in doubt run a single threaded `make` command until the message appears. If the single make fails, clean the specific library by issuing `make -C target/<name_of_failed_lib> distclean` and run `make`again.

> [!NOTE]  
> **Advanced developers** may want to specify an SDK version (if multiple versions are installed) in the configure line(s) shown above. The example below would use SDK 10.14:

```
./configure --host=x86_64-apple-darwin --with-platform=macos --with-sdk=10.14
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

**Apple Specific:**

```
--with-sdk=<sdknumber>
```
  specify sdk platform version.

**[back to top](#table-of-contents)** | **[back to section top](#4-configure-and-build-tools-and-dependencies)**

## 5. Build binary add-ons
You can find a complete list of available binary add-ons **[here](https://github.com/xbmc/repo-binary-addons)**.

Change to Kodi's source code directory:
```
cd $HOME/kodi
```

There are multiple possibilities to choose which addons are built. The following 3 examples will give an idea.

(1) Build all add-ons:
```
make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons
```
OR

(2) Build specific add-ons:
```
make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu"
```
OR

(3) Build a specific group of add-ons:
```
make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.*"
```
For additional information on regular expression usage for ADDONS_TO_BUILD, view ADDONS_TO_BUILD section located at [Kodi add-ons CMake based buildsystem](../cmake/addons/README.md)

**[back to top](#table-of-contents)**

## 6. Build Kodi
Before you can use Xcode to build Kodi, the Xcode project has to be generated with CMake. CMake is built as part of the dependencies and doesn't have to be installed separately. A toolchain file is also generated and is used to configure CMake.

### 6.1. Build with Xcode
Create an out-of-source build directory:
```
mkdir $HOME/kodi-build
```
Generate Xcode project as per configure command in **[Configure and build tools and dependencies](#4-configure-and-build-tools-and-dependencies)**:
```
make -C tools/depends/target/cmakebuildsys BUILD_DIR=$HOME/kodi-build GEN=Xcode
```

> [!TIP]
> BUILD_DIR can be omitted, and project will be created in $HOME/kodi/build
Change all relevant paths onwards if omitted.

Additional cmake arguments can be supplied via the CMAKE_EXTRA_ARGUMENTS command line variable

**Alternatively**

Change to build directory:
```
cd $HOME/kodi-build
```

Generate Xcode project (x86_64 intel):
```
/Users/Shared/xbmc-depends/x86_64-darwin17.5.0-native/bin/cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=/Users/Shared/xbmc-depends/macosx10.14_x86_64-target-debug/share/Toolchain.cmake ../kodi
```

> [!WARNING]  
> The toolchain file location differs depending on SDK version. You have to replace `x86_64-darwin17.5.0-native` and `macosx10.14_x86_64-target-debug` in the paths above with the correct ones on your system.

You can check `Users/Shared/xbmc-depends` directory content with:
```
ls -l /Users/Shared/xbmc-depends
```

**Start Xcode, open the Kodi project file** (`kodi.xcodeproj`) located in `$HOME/kodi-build` and hit `Build`.

> [!WARNING]  
> If you have selected a specific SDK version in **[step 4](#4-configure-and-build-tools-and-dependencies)** then you might need to adapt the active target to use the same SDK version, otherwise build will fail. Be sure to select a device configuration. Building for simulator is **not** supported.

### 6.2. Build with xcodebuild
Alternatively, you can also build via Xcode from the command-line with `xcodebuild`, triggered by CMake:

Build Kodi:
```
cd $HOME/kodi-build
xcodebuild -config "Debug" -jobs $(getconf _NPROCESSORS_ONLN)
```

> [!TIP]
> You can specify Release instead of Debug as -config parameter.

**Alternatively**

Build Kodi:
```
/Users/Shared/xbmc-depends/x86_64-darwin17.5.0-native/bin/cmake --build . --config "Debug" -- -verbose -jobs $(getconf _NPROCESSORS_ONLN)
```

> [!TIP]
> You can specify `Release` instead of `Debug` as `--config` parameter.

### 6.3. Build with make
CMake is also able to generate makefiles that can be used to build with make.

Change to Kodi's source code directory:
```
cd $HOME/kodi
```

Generate makefiles:
```
make -C tools/depends/target/cmakebuildsys
```

> [!TIP]
> BUILD_DIR can be provided as an argument to cmakebuildsys. This allows you to provide an alternate build location. Change all paths onwards as required if BUILD_DIR option used.
```
mkdir $HOME/kodi-build
make -C tools/depends/target/cmakebuildsys BUILD_DIR=$HOME/kodi-build
```

Build Kodi:
```
make -j$(getconf _NPROCESSORS_ONLN) -C build
```

**[back to top](#table-of-contents)** | **[back to section top](#6-build-kodi)**

## 7. Run Kodi
### 7.1. Built with Xcode or xcodebuild
After build finishes, you can run Kodi from Xcode or from terminal.

Run `Debug` config from terminal:
```
$HOME/kodi-build/Debug/kodi.bin
```

Run `Release` config from terminal:
```
$HOME/kodi-build/Release/kodi.bin
```

### 7.2. Built with make
After build finishes, you can run Kodi from terminal:
```
$HOME/kodi/build/kodi.bin
```

**[back to top](#table-of-contents)**

## 8. Package
CMake generates a target called `dmg` which will package Kodi ready for distribution. After Kodi has been built, the target can be triggered by selecting it in Xcode active scheme or manually running

```
cd $HOME/kodi-build
xcodebuild -target dmg
````
**OR**
```
cd $HOME/kodi-build
/Users/Shared/xbmc-depends/x86_64-darwin17.5.0-native/bin/cmake --build . --target "dmg" --config "Debug"
```

Generated `dmg` file will be inside `$HOME/kodi-build/tools/darwin/packaging/osx/`.

Alternatively, if you built using make:
```
cd $HOME/kodi-build
make dmg
```

Generated `dmg` file will be inside `$HOME/kodi-build/tools/darwin/packaging/osx/`.

**[back to top](#table-of-contents)**

## 9. Install
Kodi can be installed like any other app.

**[back to top](#table-of-contents)**

