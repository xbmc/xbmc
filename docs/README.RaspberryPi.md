![Kodi Logo](resources/banner_slim.png)

# Raspberry Pi build guide
This guide has been tested with Ubuntu 16.04 (Xenial) x86_64 and 18.04 (Bionic). It is meant to cross-compile Kodi for the Raspberry Pi using **[Kodi's unified depends build system](../tools/depends/README.md)**. Please read it in full before you proceed to familiarize yourself with the build procedure.

If you're looking to build Kodi natively using **[Raspbian](https://www.raspberrypi.org/downloads/raspbian/)**, you should follow the **[Ubuntu guide](README.Ubuntu.md)** instead. Several other distributions have **[specific guides](README.md)** and a general **[Linux guide](README.Linux.md)** is also available.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Install the required packages](#2-install-the-required-packages)**
3. **[Get the source code](#3-get-the-source-code)**  
  3.1. **[Get Raspberry Pi tools and firmware](#31-get-raspberry-pi-tools-and-firmware)**
4. **[Build tools and dependencies](#4-build-tools-and-dependencies)**
5. **[Build Kodi](#5-build-kodi)**

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
**NOTE:** Kodi requires a compiler with C++14 support, i.e. gcc >= 4.9 or clang >= 3.4

Install build dependencies needed to cross-compile Kodi for the Raspberry Pi:
```
sudo apt install autoconf bison build-essential curl default-jdk gawk git gperf libcurl4-openssl-dev zlib1g-dev
```

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

### 3.1. Get Raspberry Pi tools and firmware
Clone Raspberry Pi tools:
```
git clone https://github.com/raspberrypi/tools --depth=1
```

Clone Raspberry Pi firmware:
```
git clone https://github.com/raspberrypi/firmware --depth=1
```

**[back to top](#table-of-contents)**

## 4. Build tools and dependencies
Create target directory:
```
mkdir $HOME/kodi-rpi
```

Prepare to configure build:
```
cd $HOME/kodi/tools/depends
./bootstrap
```

**TIP:** Look for comments starting with `Or ...` and only execute the command(s) you need.

Configure build for Raspberry Pi 1:
```
./configure --host=arm-linux-gnueabihf --prefix=$HOME/kodi-rpi --with-toolchain=$HOME/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf --with-firmware=$HOME/firmware --with-platform=raspberry-pi --disable-debug
```

Or configure build for Raspberry Pi 2 and 3:
```
./configure --host=arm-linux-gnueabihf --prefix=$HOME/kodi-rpi --with-toolchain=$HOME/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf --with-firmware=$HOME/firmware --with-platform=raspberry-pi2 --disable-debug
```

Build tools and dependencies:
```
make -j$(getconf _NPROCESSORS_ONLN)
```

**TIP:** By adding `-j<number>` to the make command, you can choose how many concurrent jobs will be used and expedite the build process. It is recommended to use `-j$(getconf _NPROCESSORS_ONLN)` to compile on all available processor cores. The build machine can also be configured to do this automatically by adding `export MAKEFLAGS="-j$(getconf _NPROCESSORS_ONLN)"` to your shell config (e.g. `~/.bashrc`).

**[back to top](#table-of-contents)** | **[back to section top](#4-build-tools-and-dependencies)**

## 5. Build Kodi
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

Install to target directory:
```
make install
```

After the build process is finished, you can find the files ready to be installed inside `$HOME/kodi-rpi`. Look for a directory called `raspberry-pi-release` or `raspberry-pi2-release`.

**[back to top](#table-of-contents)**


