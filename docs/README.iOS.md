![Kodi Logo](resources/banner_slim.png)

# iOS build guide
This guide has been tested using Xcode 11.3.1 running on MacOS 10.14.4 (Mojave). Please note this combination is the only version our CI system builds. The minimum OS requirement for this version of Xcode is MacOS 10.14.4. Other combinations may work but we provide no assurances that other combinations will build correctly and run identically to Team Kodi releases. It is meant to cross-compile Kodi for iOS using **[Kodi's unified depends build system](../tools/depends/README.md)**. Please read it in full before you proceed to familiarize yourself with the build procedure.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Configure and build tools and dependencies](#4-configure-and-build-tools-and-dependencies)**  
  4.1. **[Advanced Configure Options](#41-advanced-configure-options)**  
5. **[Build binary add-ons](#5-build-binary-add-ons)**  
  5.1. **[Independent Add-on building](#51-independent-add-on-building)**  
  5.2. **[Xcode project building](#52-xcode-project-building)**  
6. **[Build Kodi](#6-build-kodi)**  
  6.1. **[Generate Project Files](#61-generate-project-files)**  
  6.2. **[Build with Xcode](#62-build)**  
7. **[Package](#7-package)**
8. **[Install](#8-install)**
9. **[Gesture Handling](#9-gesture-handling)**

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
* Device with **iOS 11.0 or newer** to install Kodi after build.

Building for iOS should work with the following constellations of Xcode and macOS versions:
  * Xcode 12.4 against iOS SDK 14.4 on 10.15.7 (Catalina)(recommended)(CI)
  * Xcode 13.x against iOS SDK 15.5 on 12.x (Monterey)(recommended)

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
Kodi can be built as a 64bit program for iOS. The dependencies are built in `$HOME/kodi/tools/depends` and installed into `/Users/Shared/xbmc-depends`.

> [!TIP]
> Look for comments starting with `Or ...` and only execute the command(s) you need.

> [!NOTE]  
> `--with-platform` is mandatory for all Apple platforms

Configure build:
```
cd $HOME/kodi/tools/depends
./bootstrap
./configure --host=aarch64-apple-darwin --with-platform=ios
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
> **Advanced developers** may want to specify an iOS SDK version (if multiple versions are installed) in the configure line(s) shown above. The example below would use the iOS SDK 11.0:

```
./configure --host=aarch64-apple-darwin --with-platform=ios --with-sdk=11.0
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

## 5.1. Independent Add-on building

Change to Kodi's source code directory:
```
cd $HOME/kodi
```

Build all add-ons:
```
make -C tools/depends/target/binary-addons
```

Build specific add-ons:
```
make -C tools/depends/target/binary-addons ADDONS="audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu"
```

Build a specific group of add-ons:
```
make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.*"
```
For additional information on regular expression usage for ADDONS_TO_BUILD, view ADDONS_TO_BUILD section located here [Kodi add-ons CMake based buildsystem](../cmake/addons/README.md)

## 5.2. Xcode project building

Binary addons will be built as a dependency in the Xcode project. You can choose the addons 
you wish to build during the Xcode project generation step

Generate Xcode project to build specific add-ons:
```sh
make -C tools/depends/target/cmakebuildsys CMAKE_EXTRA_ARGUMENTS="-DENABLE_XCODE_ADDONBUILD=ON -DADDONS_TO_BUILD='audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu'"
```

Generate Xcode project to build a specific group of add-ons:
```sh
make -C tools/depends/target/cmakebuildsys CMAKE_EXTRA_ARGUMENTS="-DENABLE_XCODE_ADDONBUILD=ON -DADDONS_TO_BUILD='pvr.*'"
```

For additional information on regular expression usage for ADDONS_TO_BUILD, view ADDONS_TO_BUILD section located at [Kodi add-ons CMake based buildsystem](../cmake/addons/README.md)

Generate Xcode project to build all add-ons automatically:
```sh
make -C tools/depends/target/cmakebuildsys CMAKE_EXTRA_ARGUMENTS="-DENABLE_XCODE_ADDONBUILD=ON"
```

> [!TIP]
> If you wish to not automatically build addons added to your xcode project, omit
`-DENABLE_XCODE_ADDONBUILD=ON`. The target will be added to the project, but the dependency
 will not be set to automatically build  

> [!TIP]
> Binary add-ons added to the generated Xcode project can be built independently of 
the Kodi app by selecting the scheme/target `binary-addons` in the Xcode project.
You can also build the binary-addons target via xcodebuild. This will not build the Kodi
App, but will build any/all binary addons added for the project Generation.

```sh
xcodebuild -config "Debug" -target binary-addons
```

**[back to top](#table-of-contents)**

## 6. Build Kodi

## 6.1. Generate Project Files

Before you can use Xcode to build Kodi, the Xcode project has to be generated with CMake. CMake is built as part of the dependencies and doesn't have to be installed separately. A toolchain file is also generated and is used to configure CMake.

Create an out-of-source build directory:
```
mkdir $HOME/kodi-build
```
Generate Xcode project as per configure command in **[Configure and build tools and dependencies](#4-configure-and-build-tools-and-dependencies)**:
```
make -C tools/depends/target/cmakebuildsys BUILD_DIR=$HOME/kodi-build
```

> [!TIP]
> BUILD_DIR can be omitted, and project will be created in $HOME/kodi/build
Change all relevant paths onwards if omitted.

Additional cmake arguments can be supplied via the CMAKE_EXTRA_ARGUMENTS command line variable

Alternatively:
`
Generate Xcode project for ARM 64bit (**recommended**):
```
cd $HOME/kodi-build
/Users/Shared/xbmc-depends/x86_64-darwin17.5.0-native/bin/cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=/Users/Shared/xbmc-depends/iphoneos11.3_arm64-target-debug/share/Toolchain.cmake $HOME/kodi
```

> [!WARNING]  
> The toolchain file location differs depending on your iOS and SDK version. You have to replace `x86_64-darwin15.6.0-native` and `iphoneos11.3_arm64-target-debug` in the paths above with the correct ones on your system.

You can check `Users/Shared/xbmc-depends` directory content with:
```
ls -l /Users/Shared/xbmc-depends
```
## 6.2 Build 

**Start Xcode, open the Kodi project file** (`kodi.xcodeproj`) located in `$HOME/kodi-build` and hit `Build`.

> [!WARNING]  
> If you have selected a specific iOS SDK Version in step 4 then you might need to adapt the active target to use the same iOS SDK version, otherwise build will fail. Be sure to select a device configuration. Building for simulator is not supported.

**Alternatively**, you can also build via Xcode from the command-line with `xcodebuild`:

Build Kodi:
```
cd $HOME/kodi-build
xcodebuild -config "Debug" -jobs $(getconf _NPROCESSORS_ONLN)
```

> [!TIP]
> You can specify Release instead of Debug as -config parameter.

**[back to top](#table-of-contents)** | **[back to section top](#6-build-kodi)**

## 7. Package
CMake generates a target called `deb` which will package Kodi ready for distribution. After Kodi has been built, the target can be triggered by selecting it in Xcode active scheme or manually running

```
cd $HOME/kodi-build
xcodebuild -target deb
```

**Alternatively**

```
cd $HOME/kodi-build
/Users/Shared/xbmc-depends/x86_64-darwin17.5.0-native/bin/cmake --build . --target "deb" --config "Debug"
```

**[back to top](#table-of-contents)**

## 8. Install
On jailbroken devices the resulting deb file can be copied to the iOS device via *ssh/scp* and installed manually. You need to SSH into the iOS device and issue:
```
dpkg -i <name of the deb file>
```

If you are a developer with an official Apple code signing identity you can deploy Kodi via Xcode to work on it on non-jailbroken devices. For this to work you need to alter the Xcode project by setting your codesign identity. Just select the *iPhone Developer* shortcut.
It's also important that you select the signing on all 4 spots in the project settings. After the last buildstep, our support script will do a full sign of all binaries and bundle them with the given identity, including all the `*.viz`, `*.pvr`, `*.so`, etc. files Xcode doesn't know anything about. This should allow you to deploy Kodi to all non-jailbroken devices the same way you deploy normal apps to.
In that case Kodi will be sandboxed like any other app. All Kodi files are then located in the sandboxed *Documents* folder and can be easily accessed via iTunes file sharing.

From Xcode7 on this approach is also available for non paying app developers (Apple allows self signing from now on).

**[back to top](#table-of-contents)**

## 9. Gesture Handling

| Gesture                                  | Action                     |
| ---------------------------------------- | -------------------------- |
| Double finger swipe left                 | Back                       |
| Double finger tap/single finger long tap | Right mouse                |
| Single finger tap                        | Left mouse                 |
| Panning, and flicking                    | For navigating in lists    |
| Dragging                                 | For scrollbars and sliders |
| Zoom gesture                             | In the picture viewer      |

Gestures can be adapted in **[system/keymaps/touchscreen.xml](https://github.com/xbmc/xbmc/blob/master/system/keymaps/touchscreen.xml)**.

**[back to top](#table-of-contents)**

