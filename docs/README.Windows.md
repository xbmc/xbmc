![Kodi Logo](resources/banner_slim.png)

# Windows build guide
This guide has been tested with Windows 10 Pro x64, version 1709, build 16299.334. Please read it in full before you proceed to familiarize yourself with the build procedure.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Set up the build environment](#4-set-up-the-build-environment)**
5. **[Build Kodi automagically](#5-build-kodi-automagically)**
6. **[Build Kodi manually](#6-build-kodi-manually)**

## 1. Document conventions
This guide assumes you are using `Developer Command Prompt for VS 2017`, also known as `terminal`, `console`, `command-line` or simply `cli`. Commands need to be run at the terminal, one at a time and in the provided order.

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
To build Kodi:
* **[CMake](https://cmake.org/download/)**
* **[Git for Windows](https://gitforwindows.org/)**
* **[Java Runtime Environment (JRE)](http://www.oracle.com/technetwork/java/javase/downloads/index.html)**
* **[Nullsoft scriptable install system (NSIS)](http://nsis.sourceforge.net/Download)** (Only needed if you want to generate an installer file)
* **[Visual Studio 2017](https://visualstudio.microsoft.com/vs/older-downloads/)** (Community Edition is fine)

To run Kodi you need a relatively recent CPU with integrated GPU or discrete GPU with up-to-date graphics device-drivers installed from the manufacturer's website.
* **[AMD](https://support.amd.com/en-us/download)**
* **[Intel](https://downloadcenter.intel.com/product/80939/Graphics-Drivers)**
* **[NVIDIA](http://www.nvidia.com/Download/index.aspx)**

### CMake install notes
All install screens should remain at their default values with the exception of the following.
* Under **Install options** change default to `Add CMake to system PATH for all users` or `Add CMake to system PATH for current user` (whichever you prefer).

### Git for Windows install notes
All install screens should remain at their default values with the exception of the following two.
* Under **Choosing the default editor used by Git** change default to `Use Notepad++ as Git's default editor` or your favorite editor.
* Under **Adjust your PATH environment** change default to `Use Git and optional Unix tools from the Windows Command Prompt`.

### JRE install notes
Default options are fine.
After install finishes, add java's executable file path to your `PATH` **[environment variable](http://www.java.com/en/download/help/path.xml)**. Should be similar to `C:\Program Files (x86)\Java\jre1.8.0_251\bin`.

### NSIS install notes
Default options are fine.

### Visual Studio 2017 install notes
Start the VS2017 installer and click `Individual components`.
* Under **Compilers, build tools and runtimes** select
  * `Msbuild`
  * `VC++ 2017 version 15.x v14.x latest v141 tools`
  * `Visual C++ 2017 Redistributable Update`
  * `Visual C++ compilers and libraries for ARM` (if compiling for ARM or UWP)
  * `Visual C++ compilers and libraries for ARM64` (if compiling for ARM64 or UWP)
  * `Visual C++ runtime for UWP` (if compiling for UWP)
  * `Windows Universal CRT SDK`
* Under **Development activities** select
  * `Visual Studio C++ core features`
* Under **SDKs, libraries, and frameworks** select
  * `Windows 10 SDK (10.0.x.0) for Desktop C++ [x86 and x64]`
  * `Windows 10 SDK (10.0.x.0) for UWP: C++`

Hit `Install`. Yes, it will download and install almost 7GB of stuff.

This is all you need to do a *normal* Kodi build for 32 or 64bit. Building for UWP (Universal Windows Platform) requires the above listed and quite a lot more.

Under `Workloads` select
 * `Universal Windows Platform development`
 * `Desktop development with C++`

Hit `Install`. It will download and install an extra 12GB of whatever for a grand total of almost 20GB. Yes, seriously!

**[back to top](#table-of-contents)** | **[back to section top](#2-prerequisites)**

## 3. Get the source code
Change to your `home` directory:
```
cd %userprofile%
```

Clone Kodi's current master branch:
```
git clone https://github.com/xbmc/xbmc kodi
```

**[back to top](#table-of-contents)**

## 4. Set up the build environment
To set up the build environment, several scripts must be called.

**WARNING:** The scripts may fail if you have a space in the path to the bat files.

Kodi can be built as either a normal 32bit or 64bit program, UWP 32bit and 64bit and UWP ARM 32bit. Unless there is a reason to prefer 32bit builds, we advise you to build Kodi for 64bit.

**TIP:** Look for comments starting with `Or ...` and only execute the command(s) you need.

Change to the 64bit build directory (**recommended**):
```
cd %userprofile%\kodi\tools\buildsteps\windows\x64
```

Or change to the 32bit build directory:
```
cd %userprofile%\kodi\tools\buildsteps\windows\win32
```

Or change to the UWP 64bit build directory:
```
cd %userprofile%\kodi\tools\buildsteps\windows\x64-uwp
```

Or change to the UWP 32bit build directory:
```
cd %userprofile%\kodi\tools\buildsteps\windows\win32-uwp
```

Or change to the UWP ARM 32bit build directory:
```
cd %userprofile%\kodi\tools\buildsteps\windows\arm-uwp
```

Download dependencies:
```
download-dependencies.bat
```
**TIP:** Look for the `All formed packages ready!` success message. If you see the message `ERROR: Not all formed packages are ready!`, execute the command again until you see the success message.

Download and setup the build environment for libraries:
```
download-msys2.bat
```

Build FFmpeg:
```
make-mingwlibs.bat
```

**[back to top](#table-of-contents)** | **[back to section top](#4-set-up-the-build-environment)**

## 5. Build Kodi automagically
If all you want is to build a Kodi package ready to install, execute the command below and you're done. If you want to find out more about building, ignore this step and continue reading. Or execute the command below, grab some coffee and keep reading. Building takes a while anyway.

Build a package ready to install:
```
BuildSetup.bat
```

*Normal* 32bit and 64bit builds generate an `exe` file ready to run, located at `%userprofile%\kodi\kodi-build\Debug` or `%userprofile%\kodi\kodi-build\Release`, depending on the build config. An installer `exe` file, located at `%userprofile%\kodi\project\Win32BuildSetup`, is also generated.

UWP builds generate `msix`, `appxsym` and `cer` files, located at `%userprofile%\kodi\project\UWPBuildSetup`. You can install them following this **[guide](https://kodi.wiki/view/HOW-TO:Install_Kodi_for_Universal_Windows_Platform)**.

**[back to top](#table-of-contents)**

## 6. Build Kodi manually
Change to your `home` directory:
```
cd %userprofile%
```

Create an out-of-source build directory:
```
mkdir kodi-build
```

Change to build directory:
```
cd kodi-build
```

Configure build for 64bit (**recommended**):
```
cmake -G "Visual Studio 15 Win64" -T host=x64 %userprofile%\kodi
```

Or configure build for 32bit:
```
cmake -G "Visual Studio 15" -T host=x64 %userprofile%\kodi
```

Or configure build for UWP 64bit:
```
cmake -G "Visual Studio 15 Win64" -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -T host=x64 %userprofile%\kodi
```

Or configure build for UWP 32bit:
```
cmake -G "Visual Studio 15" -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -T host=x64 %userprofile%\kodi
```

Or configure build for UWP ARM 32bit:
```
cmake -G "Visual Studio 15 ARM" -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -T host=x64 %userprofile%\kodi
```

**WARNING:** `-T host=x64` requires CMake version >= 3.8. If your version is older, drop `-T host=x64` from the command.

Build Kodi:
Build a `Debug` binary:
```
cmake --build . --config "Debug"
```

Or build a `Release` binary:
```
cmake --build . --config "Release"
```

*Normal* 32bit and 64bit builds generate an `exe` file ready to run, located at `%userprofile%\kodi-build\Debug` or `%userprofile%\kodi-build\Release`, depending on the build config.
UWP builds generate `msix`, `appxsym` and `cer` files, located inside directories at `%userprofile%\kodi-build\AppPackages\kodi\`. You can install them following this **[guide](https://kodi.wiki/view/HOW-TO:Install_Kodi_for_Universal_Windows_Platform)**.


**[back to top](#table-of-contents)** | **[back to section top](#6-build-kodi-manually)**
