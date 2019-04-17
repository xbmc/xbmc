![Kodi Logo](resources/banner_slim.png)

# tvOS build guide
This guide has been tested with macOS 10.13.4(17E199) High Sierra and 10.14.4(18E226) Mojave on Xcode 9.4.1(9F2000) and Xcode 10.2(10E125). It is meant to cross-compile Kodi for tvOS 11+ (AppleTV 4/4K) using **[Kodi's unified depends build system](../tools/depends/README.md)**. Please read it in full before you proceed to familiarize yourself with the build procedure.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Configure and build tools and dependencies](#4-configure-and-build-tools-and-dependencies)**
5. **[Build binary add-ons](#5-build-binary-add-ons)**
6. **[Build Kodi](#6-build-kodi)**
  6.1. **[Build with Xcode](#61-build-with-xcode)**
  6.2. **[Build with xcodebuild](#62-build-with-xcodebuild)**
  6.3. **[Build with make](#63-build-with-make)**
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

**NOTE:** Linux is user friendly... It's just very particular about who its friends are.
**TIP:** Algorithm is what developers call code they do not want to explain.
**WARNING:** Developers don't change light bulbs. It's a hardware problem.

**[back to top](#table-of-contents)** | **[back to section top](#1-document-conventions)**

## 2. Prerequisites
* **[Java Development Kit (JDK)](http://www.oracle.com/technetwork/java/javase/downloads/index.html)**
* **[Xcode](https://developer.apple.com/xcode/)**. Install it from the AppStore or from the **[Apple Developer Homepage](https://developer.apple.com/)**.
* Device with **tvOS 11.0 or newer** to install Kodi after build.

Building for tvOS should work with the following constellations of Xcode and macOS versions:
  * Xcode 9.x against tvOS SDK 11.x on 10.12.x (Sierra)
  * Xcode 9.x against tvOS SDK 11.x on 10.13.x (High Sierra)(recommended)
  * Xcode 9.x against tvOS SDK 11.x on 10.14.x (Mojave)(recommended)
  * Xcode 10.x against tvOS SDK 12.x on 10.14.x (Mojave)(recommended)

**WARNING:** Start Xcode after installation finishes. You need to accept the licenses and install missing components.

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
Kodi can be built as a 64bit program only for tvOS. The dependencies are built in `$HOME/kodi/tools/depends` and installed into `/Users/Shared/xbmc-depends`.

**TIP:** Look for comments starting with `Or ...` and only execute the command(s) you need.

Configure build for 64bit:
```
cd $HOME/kodi/tools/depends
./bootstrap
./configure --host=arm-apple-darwin --with-platform=tvos
```

Build tools and dependencies:
```
make -j$(getconf _NPROCESSORS_ONLN)
```

**TIP:** By adding `-j<number>` to the make command, you can choose how many concurrent jobs will be used and expedite the build process. It is recommended to use `-j$(getconf _NPROCESSORS_ONLN)` to compile on all available processor cores. The build machine can also be configured to do this automatically by adding `export MAKEFLAGS="-j$(getconf _NPROCESSORS_ONLN)"` to your shell config (e.g. `~/.bashrc`).

**WARNING:** Look for the `Dependencies built successfully.` success message. If in doubt run a single threaded `make` command until the message appears. If the single make fails, clean the specific library by issuing `make -C target/<name_of_failed_lib> distclean` and run `make`again.

**NOTE:** **Advanced developers** may want to specify an tvOS SDK version (if multiple versions are installed) in the configure line(s) shown above. The example below would use the tvOS SDK 11.0:
```
./configure --host=arm-apple-darwin --with-platform=tvos --with-sdk=11.0
```

**[back to top](#table-of-contents)** | **[back to section top](#4-configure-and-build-tools-and-dependencies)**

## 5. Build binary add-ons
You can find a complete list of available binary add-ons **[here](https://github.com/xbmc/repo-binary-addons)**.

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

**[back to top](#table-of-contents)**

## 6. Build Kodi
Before you can use Xcode to build Kodi, the Xcode project has to be generated with CMake. CMake is built as part of the dependencies and doesn't have to be installed separately. A toolchain file is also generated and is used to configure CMake.

### 6.1. Build with Xcode
Create an out-of-source build directory:
```
mkdir $HOME/kodi-build
```

Change to build directory:
```
cd $HOME/kodi-build
```

Generate Xcode project for ARM 64bit (**recommended**):
```
/Users/Shared/xbmc-depends/x86_64-darwin18.5.0-native/bin/cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=/Users/Shared/xbmc-depends/appletvos12.2_arm64-target-debug/share/Toolchain.cmake ../kodi
```

**WARNING:** The toolchain file location differs depending on your tvOS and SDK version. You have to replace `x86_64-darwin18.5.0-native` and `appletvos12.2_arm64-target-debug` in the paths above with the correct ones on your system.

You can check `Users/Shared/xbmc-depends` directory content with:
```
ls -l /Users/Shared/xbmc-depends
```

**Start Xcode, open the Kodi project file** (`kodi.xcodeproj`) located in `$HOME/kodi-build`, select `Generic TvOs Device` (or your actual connected device if you have it connected) and hit `Build`.

**WARNING:** If you have selected a specific tvOS SDK Version in step 4 then you might need to adapt the active target to use the same tvOS SDK version, otherwise build will fail. Be sure to select a device configuration. Building for simulator is not supported.

### 6.2. Build with xcodebuild
Alternatively, you can also build via Xcode from the command-line with `xcodebuild`, triggered by CMake:

Change to build directory:
```
cd $HOME/kodi-build
```

Build Kodi:
```
/Users/Shared/xbmc-depends/x86_64-darwin18.5.0-native/bin/cmake --build . --config "Debug" -- -verbose -jobs $(getconf _NPROCESSORS_ONLN)
```

**TIP:** You can specify `Release` instead of `Debug` as `--config` parameter.

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

Build Kodi:
```
make -j$(getconf _NPROCESSORS_ONLN) -C build
```

**[back to top](#table-of-contents)** | **[back to section top](#6-build-kodi)**

## 7. Package
CMake generates a target called `deb` which will package Kodi ready for distribution. After Kodi has been built, the target can be triggered by selecting it in Xcode active scheme or manually running

```
cd $HOME/kodi-build
/Users/Shared/xbmc-depends/x86_64-darwin18.5.0-native/bin/cmake --build . --target "deb" --config "Debug"
```

The generated package will be located at $HOME/kodi-build/tools/darwin/packaging/tvos.

Alternatively, if you built using makefiles issue:
```
cd $HOME/kodi/build
make deb
```

**[back to top](#table-of-contents)**

## 8. Install

There are a few different methods that can be used to install kodi on an AppleTV 4/4K.

### Jailbroken devices
On jailbroken devices the resulting deb file can be copied to the tvOS device via *ssh/scp* and installed manually. You need to SSH into the tvOS device and issue:
```
dpkg -i <name of the deb file>
```

### Using Code Signing instead

Whether you have paid or free developer account you can deploy Kodi via Xcode to work on a non-jailbroken devices.

#### Wirelessly connecting to AppleTV 4/4K
The Apple TV 4K cannot be connected to mac via a cable so the connection must be wireless to XCode to add the application.

  1. Make sure your Mac and your Apple TV are on the same network.
  2. Choose `Window->Devices and Simulators`, then in the window that appears, click Devices.
  3. On your Apple TV, open the Settings app and choose `Remotes and Devices->Remote App and Devices`.
  4. The Apple TV searches for possible devices including the Mac. (If you have any Firewall or Internet security, disable/turn off to allow searching.)
  5. On your Mac, select the Apple TV in the Devices pane. The pane for the Apple TV is displayed and shows the current status of the connection request.
  6. Enter the verification code displayed on your AppleTV into the Device window pane for the device and click Connect.

Xcode sets up the Apple TV for wireless debugging and pairs with the device.

#### Signing using a paid developer accounts
For this to work you need to alter the Xcode project by setting your codesign identity.

#### Signing using a free developer account

Note that using a free developer account the signing will need to be reapplied every 7 days.

  1. Open the Xcode project in Xcode as above (requires Xcode 7 or later)
  2. Select Xcode->Preferences and select Accounts
    * Hit the + sign to add an Apple ID accoumt and Login.
  2. Next select the kodi build target
  3. Under the `General` tab, enter a unique bundle identifer and check the box to `Automatically Manage Signing`.
  4. Select your team under `Automatically Manage Signing`.

#### An important note on Code Signing
It's also important that you select the signing on all 4 spots in the project settings. After the last buildstep, our support script will do a full sign of all binaries and bundle them with the given identity, including all the `*.viz`, `*.pvr`, `*.so`, etc. files Xcode doesn't know anything about. This should allow you to deploy Kodi to all non-jailbroken devices the same way you deploy normal apps to.
In that case Kodi will be sandboxed like any other app. All Kodi files are then located in the sandboxed *Documents* folder and can be easily accessed via iTunes file sharing.

### Installing on AppleTV
There are two options for deplying to your AppleTV 4/4K. The first is just by using Run in XCode for a debugging sessions.

Note that if you get a App Verification Failed error message when trying to to use `Run` you can delete two files in the created Kodi.app.

 * `rm -rf $HOME/kodi-build/build/Debug-appletvos/Kodi.app/_CodeSignature`
 * `rm -f $HOME/kodi-build/build/Debug-appletvos/Kodi.app/embedded.*provision`

The alternative is to deploy the output of the `deb` target. To do this:

  1. Choose Window > Devices and Simulators, then in the window that appears, click Devices.
  2. On your Mac, select the Apple TV in the Devices pane.
  3. Click the + symbol under `installed apps` and navigate to and select: `$HOME/kodi-build/build/Debug-appletvos/Kodi.app` and then `Open`.

**[back to top](#table-of-contents)**

