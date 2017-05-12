# Kodi for Apple iOS

## TOC

1. [Introduction](#1-introduction)
2. [Getting the source code](#2-getting-the-source-code)
3. [Install build dependencies](#3-install-build-dependencies)
   1. [Install Xcode](#31-install-xcode)
   2. [Install Kodi build depends](#32-install-kodi-build-depends)
   3. [Compile Kodi binary addons](#33-compile-kodi-binary-addons)
4. [How to compile Kodi](#4-how-to-compile-kodi)
   1. [Using Xcode (or xcodebuild)](#41-using-xcode-or-xcodebuild)
   2. [Compilation using command-line (make)](#42-compilation-using-command-line-make)
5. [Packaging](#5-packaging)
6. [Gesture Handling on iPad/iphone/iPod touch](#6-gesture-handling-on-ipadiphoneipod-touch)
7. [Usage on un-jailbroken devices](#7-usage-on-un-jailbroken-devices)
8. [References](#8-references)

## 1 Introduction

This is a platform port of Kodi for the Apple iOS operating system.
Starting with Kodi v18 the build system has been migrated from native Xcode to
CMake (and generated project files).

There are 3 ways to build Kodi for iOS:

- Xcode IDE (easiest as it presents the build system in a GUI environment)
- command-line with xcodebuild
- command-line with make

Kodi for iOS is composed of a main binary with numerous dynamic libraries and
codecs that support a multitude of music and video formats.

The minimum version of iOS you need to run(!) Kodi is 6.0 atm.

- On Mavericks (OSX 10.9.x) we recommend using Xcode 6.1.
- On Yosemite (OSX 10.10.x) we recommend using Xcode 6.4.
- On El Capitan (OSX 10.11.x) we recommend using Xcode 7.x or Xcode 8.x.
- On Sierra (macOS 10.12.x) we recommend using Xcode 8.x.

## 2 Getting the source code

    cd $HOME
    git clone git://github.com/xbmc/xbmc.git Kodi

## 3 Install build dependencies

### 3.1 Install Xcode

Install the Xcode version recommended for your macOS version. You can download
it either from the macOS AppStore (Xcode) or from the Apple Developer Homepage.

As far as we know the compilation for iOS should work with the following
constellations of Xcode and macOS versions (to be updated once we know more):

1. XCode 6.0.1 against iOS SDK 8.0 on 10.9 (Mavericks)
2. XCode 6.1.0 against iOS SDK 8.1 on 10.10 (Yosemite)
3. XCode 6.3.0 against iOS SDK 8.3 on 10.10 (Yosemite)
4. Xcode 6.4.0 against iOS SDK 8.4 on 10.10 (Yosemite)
5. Xcode 7.x against iOS SDK 9.x on 10.10 (Yosemite)
6. Xcode 7.x against iOS SDK 9.x on 10.11 (El Capitan)
7. Xcode 7.x against iOS SDK 9.x on 10.12 (Sierra)
8. Xcode 8.x against iOS SDK 10.x (El Capitan)
9. Xcode 8.x against iOS SDK 10.x (Sierra)

The preferred iOS SDK Version is 8.1.

### 3.2 Install Kodi build depends

Kodi requires a set of build dependencies to be built and installed before you
will be able to build the Kodi main binary. These often just called *depends*
are installed using the commands described below (with the latest iOS SDK found
on your system).

In order to speedup compilation it is recommended to use `make -j$(getconf
_NPROCESSORS_ONLN)` instead of `make` to compile on all available processor
cores. The build machine can also be configured to do this automatically by
adding `export MAKEFLAGS="-j(getconf _NPROCESSORS_ONLN)"` to your shell config
(e.g. `~/.bashrc`).

#### 3.2.a Compiling as 32 bit armv7 libraries (recommended for most users)

    cd $HOME/Kodi
    cd tools/depends
    ./bootstrap
    ./configure --host=arm-apple-darwin
    make

#### 3.2.b Compiling as 64 bit arm64 libraries

    cd $HOME/Kodi
    cd tools/depends
    ./bootstrap
    ./configure --host=arm-apple-darwin --with-cpu=arm64
    make

#### 3.3.c Advanced topics

The dependencies are built into `tools/depends` and installed into
`/Users/Shared/xbmc-depends`.

**ADVANCED developers only**: If you want to specify an iOS SDK version (if
multiple versions are installed) - then append it to the configure line
above. The example below would use the iOS SDK 8.0:

    ./configure --host=arm-apple-darwin --with-sdk=8.0

### 3.3 Compile Kodi binary addons

Kodi maintains a set of binary addons (PVR clients, Visualizations, Audio DSP
plugins and more). They can be built as shown below:

    cd $HOME/Kodi
    cd tools/depends
    make -C target/binary-addons

**NOTE**: If you only want to build specific addons you can specify like this:

    cd $HOME/Kodi
    cd tools/depends
    make -C target/binary-addons ADDONS="pvr.hts pvr.dvblink"

## 4 How to compile Kodi

### 4.1 Using Xcode (or xcodebuild)

#### 4.1.1 Generate CMake project files

Before you can use Xcode to build Kodi, the Xcode project has to be generated
with CMake. Note that CMake is compiled as parts of the depends doesn't have
to be installed separately. Also a Toolchain-file has been generated with is
used to configure CMake.

    mkdir $HOME/Kodi/build
    cd $HOME/Kodi/build
    /Users/Shared/xbmc-depends/buildtools-native/bin/cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=/Users/Shared/xbmc-depends/iphoneos9.3_armv7-target/share/Toolchain.cmake ..

The toolchain file location differs depending on your iOS and SDK version and
you have to replace `iphoneos9.3_armv7` in the filename above with the correct
file on your system. Check the directory content to get the filename.

#### 4.1.2 Compilation using Xcode

Start Xcode and open the Kodi project (kodi.xcodeproj) located in
`$HOME/Kodi/build`.

If you have selected a specific iOS SDK Version in step 3.2 then you might need
to adapt the active target to use the same iOS SDK version. Else build will fail.

Be sure to select a device configuration. Building for simulator is not
supported.

The build process will take a long time when building the first time.
You can see the progress in "Build Results". There are a large number of static
and dynamic libraries that will need to be built. Once these are built,
subsequent builds will be faster.

After the build, you can run Kodi for iOS from Xcode.

Alternatively, you can also build via Xcode from the command-line with
xcodebuild, triggered by CMake:

    cd $HOME/Kodi/build
    cmake --build . --config "Debug" -- -verbose -jobs $(getconf _NPROCESSORS_ONLN)

You can specify `Release` instead of `Debug` as a configuration.

### 4.2 Compilation using command-line (make)

CMake is also able to generate a Makefile based project that can be used to
compile with make:

    cd $HOME/Kodi
    make -C tools/depends/target/cmakebuildsys
    make -C build

## 5 Packaging

CMake generate a target called `deb` which will package Kodi.app for
distribution.

After Kodi has been build, the target can be triggered with by selecting it in
Xcode, or if using makefiles by issuing:

    make deb

On jailbroken devices the resulting deb file can be copied to the iOS device
via ssh/scp and then be installed manually. For this you need to SSH into the
iOS device and issue

    dpkg -i <name of the deb file>

## 6 Gesture Handling on iPad/iPhone/iPod touch

| Gesture                                  | Action                     |
| ---------------------------------------- | -------------------------- |
| Double finger swipe left                 | Back                       |
| Double finger tap/single finger long tap | Right mouse                |
| Single finger tap                        | Left mouse                 |
| Panning, and flicking                    | For navigating in lists    |
| Dragging                                 | For scrollbars and sliders |
| Zoom gesture                             | In the picture viewer      |

Gestures can be adapted in [system/keymaps/touchscreen.xml](https://github.com/xbmc/xbmc/blob/master/system/keymaps/touchscreen.xml).

## 7 Usage on un-jailbroken devices

If you are a developer with an official apple code signing identity you can
deploy Kodi via xcode to work on it on non-jailbroken devices. For this to
happen you just need to alter the Xcode project by setting your codesign
identity. Just select the "iPhone Developer" shortcut. It's also important
that you select the signing on all 4 spots in the project settings. After that
the last buildstep in our support script will do a full sign of all binaries
and the bundle with the given identity (all `*.viz`, `*.pvr`, `*.so` files
Xcode doesn't know anything about). This should allow you to deploy Kodi to all
non-jailbroken devices which you can deploy normal apps to. In that case (Kodi
will be sandboxed like any other app) - all Kodi files are then located in the
sandboxed *Documents* folder and can be easily accessed via iTunes file
sharing. Keep in mind that no hardware acceleration will be possible without
jailbreaking when using iOS < Version 8.

From Xcode7 on this approach is also available for non paying app developers
(apple allows self signing from now on).

## 6 References

- [cmake/README.md](https://github.com/xbmc/xbmc/tree/master/cmake/README.md)
- [tools/depends/README](https://github.com/xbmc/xbmc/tree/master/tools/depends/README)
- [iOS section in forum.kodi.tv](http://forum.kodi.tv/forumdisplay.php?fid=137)
