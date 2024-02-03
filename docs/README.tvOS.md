![Kodi Logo](resources/banner_slim.png)

# tvOS build guide
This guide has been tested using Xcode 11.3.1 running on MacOS 10.15.2 (Catalina). Please note this combination is the only version our CI system builds. The minimum OS requirement for this version of Xcode is MacOS 10.14.4. Other combinations may work but we provide no assurances that other combinations will build correctly and run identically to Team Kodi releases. It is meant to cross-compile Kodi for tvOS 11+ (AppleTV 4/4K) using **[Kodi's unified depends build system](../tools/depends/README.md)**. Please read it in full before you proceed to familiarize yourself with the build procedure.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Configure and build tools and dependencies](#4-configure-and-build-tools-and-dependencies)**  
  4.1. **[Advanced Configure Options](#41-advanced-configure-options)**  
5. **[Generate Kodi Build files](#5-generate-kodi-build-files)**  
  5.1. **[Generate XCode Project Files](#51-generate-xcode-project-files)**  
  5.2. **[Build with Xcode](#62-build)**  
6. **[Build Kodi](#6-build-kodi)**  
  6.1. **[Build with Xcode](#61-build-with-xcode)**  
  6.2. **[Build with xcodebuild](#62-build-with-xcodebuild)**  
7. **[Packaging to distribute as deb](#7-packaging-to-distribute-as-deb)**  
  7.1. **[Package via Xcode](#71-package-via-xcode)**  
  7.2. **[Package via Xcodebuild](#72-package-via-xcodebuild)**  
8. **[Signing](#8-signing)**  
  8.1. **[Signing using a developer account](#81-signing-using-a-developer-account)**  
  8.2. **[Using iOS App Signer to install](#82-using-ios-app-signer-to-install)**  
9. **[Install](#9-install)**  
  9.1. **[Jailbroken devices](#91-jailbroken-devices)**  
  9.2. **[Using Xcode to install](#92-using-xcode-to-install)**  

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
* Device with **tvOS 11.0 or newer** to install Kodi after build.

Building for tvOS should work with the following combinations of Xcode and macOS versions:
  * Xcode 12.4 against tvOS SDK 14.3 on 10.15.7 (Catalina)(recommended)(CI)
  * Xcode 13.x against tvOS SDK 15.4 on 12.x (Monterey)(recommended)

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
Kodi can be built as a 64bit program only for tvOS. The dependencies are built in `$HOME/kodi/tools/depends` and installed into `/Users/Shared/xbmc-depends`.
> [!NOTE]  
> `--with-platform` is mandatory for all Apple platforms

Configure build:
```
cd $HOME/kodi/tools/depends
./bootstrap
./configure --host=aarch64-apple-darwin --with-platform=tvos
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
> **Advanced developers** may want to specify an tvOS SDK version (if multiple versions are installed) in the configure line(s) shown above. The example below would use the tvOS SDK 11.0:

```
./configure --host=aarch64-apple-darwin --with-platform=tvos --with-sdk=11.0
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

**[back to top](#table-of-contents)**

## 5. Generate Kodi Build files
Before you can use Xcode to build Kodi, the Xcode project has to be generated with CMake. CMake is built as part of the dependencies and doesn't have to be installed separately. A toolchain file is also generated and is used to configure CMake.
Default behaviour will not build binary addons. To add addons to your build go to **[Add Binary Addons to Project](#52-add-binary-addons-to-project)**

## 5.1. Generate XCode Project Files

Before you can use Xcode to build Kodi, the Xcode project has to be generated with CMake. CMake is built as part of the dependencies and doesn't have to be installed separately. A toolchain file is also generated and is used to configure CMake.

Create an out-of-source build directory:
```
mkdir $HOME/kodi-build
```

Generate Xcode project for TVOS:
```
make -C tools/depends/target/cmakebuildsys BUILD_DIR=$HOME/kodi-build
```

> [!TIP]
> BUILD_DIR can be omitted, and project will be created in $HOME/kodi/build
Change all relevant paths onwards if omitted.

Additional cmake arguments can be supplied via the CMAKE_EXTRA_ARGUMENTS command line variable

An example to set signing settings in xcode project:
````
make -C tools/depends/target/cmakebuildsys CMAKE_EXTRA_ARGUMENTS="-DPLATFORM_BUNDLE_IDENTIFIER='tv.kodi.kodi' -DCODE_SIGN_IDENTITY='iPhone Developer: *** (**********)' -DPROVISIONING_PROFILE_APP='tv.kodi.kodi' -DPROVISIONING_PROFILE_TOPSHELF='tv.kodi.kodi.Topshelf'"
````
Available Signing arguments

PLATFORM_BUNDLE_IDENTIFIER - bundle ID (used for the app, top shelf and entitlements)  
DEVELOPMENT_TEAM - dev team ID  **OR** CODE_SIGN_IDENTITY - certificate name  
PROVISIONING_PROFILE_APP - provprofile name for the app  
PROVISIONING_PROFILE_TOPSHELF - provprofile name for the top shelf  

## 5.2. Add Binary Addons to Project

> [!TIP]
> If you wish to add signing settings automatically, look at **[Generate XCode Project Files](#51-generate-xcode-project-files)** for the additional `CMAKE_EXTRA_ARGUMENTS`

You can find a complete list of available binary add-ons **[here](https://github.com/xbmc/repo-binary-addons)**.

Binary addons will be built as a dependency in the Xcode project. You can choose the addons you wish to build during the Xcode project generation step

Generate Xcode project to build specific add-ons:
```
make -C tools/depends/target/cmakebuildsys CMAKE_EXTRA_ARGUMENTS="-DENABLE_XCODE_ADDONBUILD=ON -DADDONS_TO_BUILD='audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu'"
```

Generate Xcode project to build a specific group of add-ons:
```
make -C tools/depends/target/cmakebuildsys CMAKE_EXTRA_ARGUMENTS="-DENABLE_XCODE_ADDONBUILD=ON -DADDONS_TO_BUILD='pvr.*'"
```
For additional information on regular expression usage for ADDONS_TO_BUILD, view ADDONS_TO_BUILD section located at [Kodi add-ons CMake based buildsystem](../cmake/addons/README.md)

Generate Xcode project to build all add-ons automatically:
```
make -C tools/depends/target/cmakebuildsys CMAKE_EXTRA_ARGUMENTS="-DENABLE_XCODE_ADDONBUILD=ON"
```

> [!TIP]
> If you wish to not automatically build addons added to your xcode project, omit `-DENABLE_XCODE_ADDONBUILD=ON`. The target will be added to the project, but the dependency will not be set to automatically build

> [!TIP]
> Binary add-ons added to the generated Xcode project can be built independently of the Kodi app by selecting the scheme/target `binary-addons` in the Xcode project.
You can also build the binary-addons target via xcodebuild. This will not build the Kodi App, but will build any/all binary addons added for the project Generation.
```
xcodebuild -config "Debug" -target binary-addons
```
**[back to top](#table-of-contents)** | **[back to section top](#5-generate-kodi-build-files)**

## 6. Build

### 6.1. Build with Xcode

Start Xcode, open the Kodi project file created in **[Generate Kodi Build files](#5-generate-kodi-build-files)**

> [!TIP]
> (`kodi.xcodeproj`) is located in `$HOME/kodi-build`

Once the project has loaded, select `Generic TvOs Device` (or your actual connected device if you have it connected) and hit `Build`.

This will create a `Kodi.app` file located in `$HOME/kodi-build/build/Debug-appletvos`. This App can be deployed via Xcode to an AppleTV via `Window -> Devices and Simulators -> Select device and click +`

> [!TIP]
> If you build as a release target, the location of the `Kodi.app` will be `$HOME/kodi-build/build/Release-appletvos`

> [!WARNING]  
> If you have selected a specific tvOS SDK Version in step 4 then you might need to adapt the active target to use the same tvOS SDK version, otherwise build will fail. Be sure to select a device configuration.

> [!WARNING]  
> Building for simulator is NOT supported.

### 6.2. Build with xcodebuild
Alternatively, you can also build via Xcode from the command-line with `xcodebuild`, triggered by CMake:

Change to build directory:
```
cd $HOME/kodi-build
xcodebuild -config "Debug" -jobs $(getconf _NPROCESSORS_ONLN)
```

This will create a `Kodi.app` file located in `$HOME/kodi-build/build/Debug-appletvos`. This App can be deployed via Xcode to an AppleTV via `Window -> Devices and Simulators -> Select device and click +`

> [!TIP]
> You can specify Release instead of Debug as -config parameter.

> [!TIP]
> If you build as a release target, the location of the `Kodi.app` will be `$HOME/kodi-build/build/Release-appletvos`

**[back to top](#table-of-contents)** | **[back to section top](#6-build)**

## 7. Packaging to distribute as deb
CMake generates a target called `deb` which will package Kodi ready for distribution. After Kodi has been built, the target can be triggered by selecting it in Xcode active scheme or manually running

## 7.1. Package via Xcode

Start Xcode, open the Kodi project file created in **[Generate XCode Project Files](#51-generate-xcode-project-files)**

> [!TIP]
> (`kodi.xcodeproj`) is located in `$HOME/kodi-build`

Click on `Product` in the top menu bar, and then go to `Scheme`, then select `deb`

Hit `Build`

> [!TIP]
> The generated package will be located at $HOME/kodi-build/tools/darwin/packaging/tvos.

## 7.2. Package via Xcodebuild

Change to build directory:
```
cd $HOME/kodi-build
xcodebuild -target deb
```

> [!TIP]
> The generated package will be located at $HOME/kodi-build/tools/darwin/packaging/tvos.

**[back to top](#table-of-contents)**

## 8. Signing

> [!TIP]
> If your device is jailbroken, you can go direct to **[Installing on Jailbroken Device](#91-jailbroken-devices)**

## 8.1. Signing using a developer account

For this to work you need to alter the Xcode project by setting your codesign identity or supplying credentials during
xcode generation.
Note that using a free developer account the signing will need to be reapplied every 7 days.

  1. Open the Xcode project in Xcode as above (requires Xcode 7 or later)
  2. Select Xcode->Preferences and select Accounts
    * Hit the + sign to add an Apple ID account and Login.
  2. Next select the kodi build target
  3. Under the `General` tab, enter a unique bundle identifier and check the box to `Automatically Manage Signing`.
  4. Select your team under `Automatically Manage Signing`.

## An important note on Code Signing
It's also important that you select the signing on all 4 spots in the project settings. After the last buildstep, our support script will do a full sign of all binaries and bundle them with the given identity, including all the `*.viz`, `*.pvr`, `*.so`, etc. files Xcode doesn't know anything about. This should allow you to deploy Kodi to all non-jailbroken devices the same way you deploy normal apps to.
In that case Kodi will be sandboxed like any other app. All Kodi files are then located in the sandboxed *Documents* folder and can be easily accessed via iTunes file sharing.

## 8.2. Using iOS App Signer to install

  1. Build the deb target via xcodebuild or Xcode as per **[Build Kodi](#6-build-kodi)**
  2. Open iOS Appsigner
  3. Browse to $HOME/kodi/build/tools/darwin/packaging/tvos for your input file
  4. Select your signing certificate
  5. Select your provisioning profile
  6. Click start and select save location for the ipa file
  7. Run Xcode -> Window -> Devices and Simulators
  8. Select your Apple TV you setup in earlier for Wireless connecting press the +
  9. Find your ipa file and click open.

**[back to top](#table-of-contents)**

## 9. Install

There are a number of different methods that can be used to install kodi on an AppleTV 4/4K.

## 9.1. Jailbroken devices
On jailbroken devices the resulting deb file created from **[Packaging to distribute as deb](#7-packaging-to-distribute-as-deb)** can be copied to the tvOS device via *ssh/scp* and installed manually. You need to SSH into the tvOS device and issue:
```
dpkg -i <name of the deb file>
```

## 9.2. Using Xcode to install

Whether you have paid or free developer account you can deploy Kodi via Xcode to work on a non-jailbroken devices.

## Wirelessly connecting to AppleTV 4/4K
The Apple TV 4K cannot be connected to mac via a cable so the connection must be wireless to XCode to add the application.

  1. Make sure your Mac and your Apple TV are on the same network.
  2. Choose `Window->Devices and Simulators`, then in the window that appears, click Devices.
  3. On your Apple TV, open the Settings app and choose `Remotes and Devices->Remote App and Devices`.
  4. The Apple TV searches for possible devices including the Mac. (If you have any Firewall or Internet security, disable/turn off to allow searching.)
  5. On your Mac, select the Apple TV in the Devices pane. The pane for the Apple TV is displayed and shows the current status of the connection request.
  6. Enter the verification code displayed on your AppleTV into the Device window pane for the device and click Connect.

Xcode sets up the Apple TV for wireless debugging and pairs with the device.
Once your Apple TV has been connected in Xcode, you can deploy either the **[Deb](#7-packaging-to-distribute-as-deb)** or **[App](#6-build)** file.

  1. Choose Window > Devices and Simulators, then in the window that appears, click Devices.
  2. On your Mac, select the Apple TV in the Devices pane.
  3. Click the + symbol under `installed apps` and navigate to and select: `$HOME/kodi-build/build/Debug-appletvos/Kodi.app` and then `Open`.

**[back to top](#table-of-contents)**
