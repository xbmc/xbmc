![Kodi Logo](resources/banner_slim.png)

# Windows build guide
This guide has been tested with Windows 10 Pro x64, version 21H2, build 19044.1415. Please read it in full before you proceed to familiarize yourself with the build procedure.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
3. **[Get the source code](#3-get-the-source-code)**
4. **[Set up the build environment](#4-set-up-the-build-environment)**
5. **[Build Kodi automagically](#5-build-kodi-automagically)**
6. **[Build Kodi manually](#6-build-kodi-manually)**

## 1. Document conventions
This guide assumes you are using `Developer Command Prompt for VS 2022`, also known as `terminal`, `console`, `command-line` or simply `cli`. Commands need to be run at the terminal, one at a time and in the provided order.

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

**Example:** Clone Kodi's current Matrix branch:
```
git clone -b Matrix https://github.com/xbmc/xbmc kodi
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
To build Kodi:
* **Windows** 64bit OS, Windows 8.1 or above (allows build of x64, win32, uwp, arm)
* **[CMake](https://cmake.org/download/)** (version 3.20 or greater is required to build Kodi, version 3.21 or greater to build with Visual Studio 2022)
* **[Git for Windows](https://gitforwindows.org/)**
* **[Java Runtime Environment (JRE)](http://www.oracle.com/technetwork/java/javase/downloads/index.html)**
* **[Nullsoft scriptable install system (NSIS)](http://nsis.sourceforge.net/Download)** (Only needed if you want to generate an installer file)
* **[Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)** or **[Visual Studio 2019](https://visualstudio.microsoft.com/vs/older-downloads/)** (Community Edition is fine)

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
After install finishes, add java's executable file path to your `PATH` **[environment variable](http://www.java.com/en/download/help/path.xml)**. Should be similar to `C:\Program Files\Java\jre1.8.0_311\bin`.

### NSIS install notes
Default options are fine.

### Visual Studio 2022/2019 install notes
Start the Visual Studio installer and click **Workloads** select
* Under **Desktop & Mobile** section select
  * `Desktop development with C++`
  * `Universal Windows Platform development` (if compiling for UWP or UWP-ARM)

Click in **Individual components** select
* Under **Compilers, build tools and runtimes** section select
  * `MSVC v142/3 - VS 2019/22 C++ ARM build tools (Latest)` (if compiling for UWP-ARM)

Hit `Install`. Yes, it will download and install almost 8GB of stuff for x64 only or up to 20GB if everything is selected for UWP / UWP-ARM as well.

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

> [!WARNING]  
> The scripts may fail if you have a space in the path to the bat files.

Kodi can be built as either a normal 32bit or 64bit program, UWP 32bit and 64bit and UWP ARM 32bit. Unless there is a reason to prefer 32bit builds, we advise you to build Kodi for 64bit.

> [!TIP]
> Look for comments starting with `Or ...` and only execute the command(s) you need.

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

> [!TIP]
> Look for the `All formed packages ready!` success message. If you see the message `ERROR: Not all formed packages are ready!`, execute the command again until you see the success message.

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

> [!NOTE]  
> To generate an exact replica of the official Kodi Windows installer, some additional steps are required:

Build built-in add-ons (peripheral.joystick only) with command line:
```
make-addons.bat peripheral.joystick
```

Build the installer with the command line:
```
BuildSetup.bat nobinaryaddons clean
```

`BuildSetup.bat` without parameters also builds all the Kodi add-ons that are not needed because they are not included in the installer and the process is very time consuming.

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
cmake -G "Visual Studio 17 2022" -A x64 -T host=x64 %userprofile%\kodi
```

Or configure build for 32bit:
```
cmake -G "Visual Studio 17 2022" -A Win32 -T host=x64 %userprofile%\kodi
```

Or configure build for UWP 64bit:
```
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -T host=x64 %userprofile%\kodi
```

Or configure build for UWP 32bit:
```
cmake -G "Visual Studio 17 2022" -A Win32 -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -T host=x64 %userprofile%\kodi
```

Or configure build for UWP ARM 32bit:
```
cmake -G "Visual Studio 17 2022" -A ARM -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -T host=x64 %userprofile%\kodi
```

**Visual Studio 2019:**

Replace:
```
-G "Visual Studio 17 2022"
```

With:
```
-G "Visual Studio 16 2019"
```

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
