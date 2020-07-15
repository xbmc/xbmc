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
6. **[Docker](#6-docker)**
7. **[Troubleshooting](#7-troubleshooting)**  
  7.1. **[ImportError: No module named \_sysconfigdata\_nd](#71-importerror-no-module-named-_sysconfigdata_nd)**  
  7.2. **[Errors connecting to any internet (TLS) service](#72-errors-connecting-to-any-internet-tls-service)**

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


## 6. Docker

If you encounter issues with the previous instructions, or if you don't have a proper system for cross-compiling Kodi, it's also possible to use a [Docker](https://www.docker.com/) image to perform the build. This method, although it should work just like the build instructions mentioned above, is **not** supported. Therefore, issues related specifically to Docker should **not** be opened.

Here is an example Dockerfile, summarizing basically all the instructions described above (/!\ may not be up to date with the actual instructions!). **Please read the comments as they describe things you NEED to change and/or consider before building.**

```Dockerfile
# Change 'latest' to the officially supported version of Ubuntu for cross-compilation
FROM ubuntu:latest

RUN apt-get update && apt-get upgrade -y
RUN apt-get -y install autoconf bison build-essential curl default-jdk gawk git gperf libcurl4-openssl-dev zlib1g-dev file

# The 'HOME' variable doesn't really matter - it is only the location of the files within the image
ARG HOME=/home/pi
# This is the location kodi will be built for, that means you will have to put the built files in
# this directory afterwards. It is important because many paths end up hardcoded during the build.
ARG PREFIX=/opt/kodi

RUN mkdir $PREFIX
WORKDIR $HOME

# Replace 'master' with whichever branch/tag you wish to build - be careful with nightly builds!
RUN git clone -b master https://github.com/xbmc/xbmc kodi --depth 1
RUN git clone https://github.com/raspberrypi/tools --depth=1
RUN git clone https://github.com/raspberrypi/firmware --depth=1

WORKDIR $HOME/kodi/tools/depends
RUN ./bootstrap

# Change this if you're building on a RPi1, as described above
RUN ./configure --host=arm-linux-gnueabihf --prefix=$PREFIX --with-toolchain=$HOME/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf --with-firmware=$HOME/firmware --with-platform=raspberry-pi2 --disable-debug

RUN make -j$(getconf _NPROCESSORS_ONLN)

WORKDIR $HOME/kodi
# This step builds all the binary addons.
# Kodi - at its core - works fine without them, however they are used by many other addons.
# Therefore, it is recommended to simply compile all of them.
RUN make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons
RUN make -C tools/depends/target/cmakebuildsys

WORKDIR $HOME/kodi/build
RUN make -j$(getconf _NPROCESSORS_ONLN)

RUN make install
RUN tar zfc /kodi.tar.gz $PREFIX
```

You can then build the image, and afterwards retrieve the build files from a dummy container:

```bash
docker build -t kodi_build . 
docker run --name some-temp-container-name kodi_build /bin/bash
docker cp some-temp-container-name:/kodi.tar.gz ./
docker rm some-temp-container-name
```

You should now have a file `kodi.tar.gz` in your current directory. Now you need to uncompress this file in the `$PREFIX` directory (as mentioned in the Dockerfile) of your Raspberry. Note that the archive contains multiple directories in its root, but only the `raspberry-pi2-release` (or `raspberry-pi-release`) is needed, so you can delete the others safely. If you encounter problems, please take a look at the [Troubleshooting](#7-troubleshooting) section below before filing an issue.

**[back to top](#table-of-contents)**

## 7. Troubleshooting

### 7.1 ImportError: No module named \_sysconfigdata\_nd

This is caused by an issue with a python package. The solution is to simply add a missing symlink so the library can be found, i.e.:

```bash
ln -s /usr/lib/python2.7/plat-arm-linux-gnueabihf/_sysconfigdata_nd.py /usr/lib/python2.7/
```

### 7.2 Errors connecting to any internet (TLS) service

First, you should enable debug logging (instructions [here](https://kodi.wiki/view/Log_file)). Then you need to check the logs and find what the source of your problem is. If, when trying to access TLS services (e.g. when installing an addon), the connection fails and your log contains entries such as:

```log
# note that those logs appear when enabling component-specific logs -> libcurl
2019-05-19 17:18:39.570 T:1854288832   DEBUG: Curl::Debug - TEXT: SSL certificate problem: unable to get local issuer certificate
2019-05-19 17:18:39.570 T:1854288832   DEBUG: Curl::Debug - TEXT: Closing connection 0

# this is part of the regular Kodi logs
2019-05-19 17:18:39.570 T:1854288832   ERROR: CCurlFile::FillBuffer - Failed: Peer certificate cannot be authenticated with given CA certificates(60)
```

Then, you need to define the environment variable `SSL_CERT_FILE` so it points to your system's certificate file. Depending on how you start Kodi, putting this line in your in your `.profile` file should fix this issue:

```bash
export SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt
```

Note that you need to define this variable *before* starting Kodi. For example, if you start Kodi on startup through a crontab, your `.profile` will *not* be sourced.

**[back to top](#table-of-contents)**
