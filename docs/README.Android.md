![Kodi Logo](resources/banner_slim.png)

# Android build guide
This guide has been tested with Ubuntu 16.04 (Xenial) x86_64. It is meant to cross-compile Kodi for Android using **[Kodi's unified depends build system](../tools/depends/README.md)**. Please read it in full before you proceed to familiarize yourself with the build procedure.

It should work if you're using macOS. If that is the case, read **[macOS specific prerequisites](#35-macos-specific-prerequisites)** first.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Install the required packages](#2-install-the-required-packages)**
3. **[Prerequisites](#3-prerequisites)**  
  3.1. **[Extract Android SDK and NDK](#31-extract-android-sdk-and-ndk)**  
  3.2. **[Configure Android SDK](#32-configure-android-sdk)**  
  3.3. **[Set up the Android toolchain](#33-set-up-the-android-toolchain)**  
  3.4. **[Create a key to sign debug APKs](#34-create-a-key-to-sign-debug-apks)**  
  3.5. **[macOS specific prerequisites](#35-macos-specific-prerequisites)**
4. **[Get the source code](#4-get-the-source-code)**
5. **[Build tools and dependencies](#5-build-tools-and-dependencies)**
6. **[Build binary add-ons](#6-build-binary-add-ons)**
7. **[Build Kodi](#7-build-kodi)**
8. **[Package](#8-package)**
9. **[Install](#9-install)**
10. **[Debugging Kodi](#10-debugging-kodi)**

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
 
**NOTE:** Linux is user friendly... It's just very particular about who its friends are.  
**TIP:** Algorithm is what developers call code they do not want to explain.  
**WARNING:** Developers don't change light bulbs. It's a hardware problem.

**[back to top](#table-of-contents)** | **[back to section top](#1-document-conventions)**

## 2. Install the required packages
Install build dependencies needed to cross-compile Kodi for Android:
```
sudo apt install autoconf build-essential curl default-jdk gawk git gperf lib32stdc++6 lib32z1 lib32z1-dev libcurl4-openssl-dev unzip zlib1g-dev
```
**NOTE:** If you're running a 32bit Debian/Ubuntu distribution,  remove `lib32stdc++6 lib32z1 lib32z1-dev` from the command.

**[back to top](#table-of-contents)**

## 3. Prerequisites
Building Kodi for Android requires Android NDK revision 20. For the SDK just use the latest available.

* **[Android SDK](https://developer.android.com/studio/index.html)** (Look for `Get just the command line tools`)
* **[Android NDK](https://developer.android.com/ndk/downloads/index.html)**

### 3.1. Extract Android SDK and NDK
Create needed directories:
```
mkdir -p $HOME/android-tools/android-sdk-linux
```

Extract Android SDK:
```
unzip $HOME/Downloads/sdk-tools-linux-4333796.zip -d $HOME/android-tools/android-sdk-linux
```

**NOTE:** Since we're using the latest SDK available, filename can change over time. Adapt the `unzip` command accordingly.

Extract Android NDK:
```
unzip $HOME/Downloads/android-ndk-r20-linux-x86_64.zip -d $HOME/android-tools
```

### 3.2. Configure Android SDK
Before Android SDK can be used, you need to accept the licenses and configure it:
```
cd $HOME/android-tools/android-sdk-linux/tools/bin
./sdkmanager --licenses
./sdkmanager platform-tools
./sdkmanager "platforms;android-28"
./sdkmanager "build-tools;28.0.3"
```

### 3.3. Create a key to sign debug APKs
All packages must be signed. The following command will generate a self-signed debug key. If the result is a cryptic error, it probably just means a debug key already existed.

```
keytool -genkey -keystore ~/.android/debug.keystore -v -alias androiddebugkey -dname "CN=Android Debug,O=Android,C=US" -keypass android -storepass android -keyalg RSA -keysize 2048 -validity 10000
```

### 3.4. macOS specific prerequisites
* **[Java Development Kit (JDK)](http://www.oracle.com/technetwork/java/javase/downloads/index.html)** installed.
* Normal macOS installations filesystem is case insensitive but compiling for Android requires a case sensitive filesystem. Generate a writeable hdd image and format it with hfs+ (case sensitive) issuing
  * `hdiutil create -type UDIF -fs 'Case-sensitive Journaled HFS+' -size 20g -volname android-dev $HOME/android-dev.dmg`
* Whenever you want to compile/develop you need to mount the image
  * `open ~/android-dev.dmg`
* Once you have your hdd image with case sensitive hfs+ file system execute all the steps inside of this filesystem. You need to adapt all paths in this guide so that they match your local environment. As an example here is a configure line that demonstrates possible paths:
  * `./configure --with-tarballs=/Users/Shared/xbmc-depends/tarballs --host=arm-linux-androideabi --with-sdk-path=/Volumes/android-dev/android/android-sdk-macosx --with-ndk-path=/Volumes/android-dev/android/android-ndk-r20 --prefix=/Volumes/android-dev/android/xbmc-depends`
  
**[back to top](#table-of-contents)** | **[back to section top](#3-prerequisites)**

## 4. Get the source code
Change to your `home` directory:
```
cd $HOME
```

Clone Kodi's current master branch:
```
git clone https://github.com/xbmc/xbmc kodi
```

## 5. Build tools and dependencies
Prepare to configure build:
```
cd $HOME/kodi/tools/depends
./bootstrap
```

**TIP:** Look for comments starting with `Or ...` and only execute the command(s) you need.

Configure build for aarch64:
```
./configure --with-tarballs=$HOME/android-tools/xbmc-tarballs --host=aarch64-linux-android --with-sdk-path=$HOME/android-tools/android-sdk-linux --with-ndk-path=$HOME/android-tools/android-ndk-r20 --prefix=$HOME/android-tools/xbmc-depends
```

Or configure build for arm:
```
./configure --with-tarballs=$HOME/android-tools/xbmc-tarballs --host=arm-linux-androideabi --with-sdk-path=$HOME/android-tools/android-sdk-linux --with-ndk-path=$HOME/android-tools/android-ndk-r20 --prefix=$HOME/android-tools/xbmc-depends
```

Or configure build for x86:
```
./configure --with-tarballs=$HOME/android-tools/xbmc-tarballs --host=i686-linux-android --with-sdk-path=$HOME/android-tools/android-sdk-linux --with-ndk-path=$HOME/android-tools/android-ndk-r20 --prefix=$HOME/android-tools/xbmc-depends
```

Build tools and dependencies:
```
make -j$(getconf _NPROCESSORS_ONLN)
```

**TIP:** By adding `-j<number>` to the make command, you can choose how many concurrent jobs will be used and expedite the build process. It is recommended to use `-j$(getconf _NPROCESSORS_ONLN)` to compile on all available processor cores. The build machine can also be configured to do this automatically by adding `export MAKEFLAGS="-j$(getconf _NPROCESSORS_ONLN)"` to your shell config (e.g. `~/.bashrc`).

**WARNING:** Look for the `Dependencies built successfully.` success message. If in doubt run a single threaded `make` command until the message appears. If the single make fails, clean the specific library by issuing `make -C target/<name_of_failed_lib> distclean` and run `make`again.


**[back to top](#table-of-contents)** | **[back to section top](#5-build-tools-and-dependencies)**

## 6. Build binary add-ons
You can find a complete list of available binary add-ons **[here](https://github.com/xbmc/repo-binary-addons)**.

Change to Kodi's source code directory:
```
cd $HOME/kodi
```

Build all add-ons:
```
make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons
```

Build specific add-ons:
```
make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu"
```

Build a specific group of add-ons:
```
make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.*"
```

**[back to top](#table-of-contents)**

## 7. Build Kodi
Configure CMake build:
```
cd $HOME/kodi
make -C tools/depends/target/cmakebuildsys
```

**TIP:** BUILD_DIR can be provided as an argument to cmakebuildsys. This allows you to provide an alternate build location. Change all paths onwards as required if BUILD_DIR option used.
```
mkdir $HOME/kodi-build
make -C tools/depends/target/cmakebuildsys BUILD_DIR=$HOME/kodi-build
```

Build Kodi:
```
cd $HOME/kodi/build
make -j$(getconf _NPROCESSORS_ONLN)
```

**[back to top](#table-of-contents)**

## 8. Package
CMake generates a target called `apk` which will package Kodi ready for distribution.

Create package:
```
make apk
```

Generated `apk` file will be inside `$HOME/kodi`.

**[back to top](#table-of-contents)**

## 9. Install
Connect your Android device to your computer through USB and enable the `Unknown sources` option in your device settings.

Make sure `adb` is installed:
```
sudo apt install adb
```

Install Kodi:
```
cd $HOME/kodi-android
adb devices
adb -s <device-id> install -r <generated-apk-name-here>.apk
```

The *device-id* can be retrieved from the list returned by the `adb devices` command and is the first value in the row representing your device.

**[back to top](#table-of-contents)**

## 10. Debugging Kodi
To be able to see what is happening while running Kodi you need to enable `USB debugging` in your device settings (already enabled in the Android Emulator).

Access the log output of your Android device:
```
adb -s <device-id> logcat
```

Install a new build over the existing one:
```
adb -e install -r images/xbmcapp-debug.apk
```

Launch Kodi on Android Emulator without the GUI:
```
adb shell am start -a android.intent.action.MAIN -n org.xbmc.xbmc/android.app.NativeActivity
```

Kill a misbehaving Kodi:
```
adb shell ps | grep org.xbmc | awk '{print $2}' | xargs adb shell kill
```

Filter logcat messages by a specific tag (e.g. `Kodi`):
```
adb logcat -s Kodi:V
```

Enable CheckJNI (**before** starting the Kodi):
```
adb shell setprop debug.checkjni 1
```

**NOTE:** These commands assume that current directory is `tools/android/packaging` and that the proper SDK/NDK paths are set.

GDB can be used to debug, though the support is rather primitive. Rather than using gdb directly, you will need to use ndk-gdb which wraps it. Do **not** trust the `-p/--project` switches, as they do not work. Instead you will need to `cd` to `tools/android/packaging/xbmc` and execute it from there.
```
 ndk-gdb --start --delay=0
```

This will open the installed version of Kodi and break. The warnings can be ignored as we have the appropriate paths already setup.

**[back to top](#table-of-contents)** | **[back to section top](#10-debugging-kodi)**

