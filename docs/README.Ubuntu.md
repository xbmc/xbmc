![Kodi Logo](resources/banner_slim.png)

# Debian/Ubuntu build guide
This guide has been tested with Ubuntu 16.04.4 (Xenial) x86_64, 18.04 (Bionic) and 20.04 (Focal). Please read it in full before you proceed to familiarize yourself with the build procedure.

Several other distributions have **[specific build guides](README.md)** and a general **[Linux build guide](README.Linux.md)** is also available.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Get the source code](#2-get-the-source-code)**
3. **[Install the required packages](#3-install-the-required-packages)**  
  3.1. **[Get build dependencies automagically](#31-get-build-dependencies-automagically)**  
  3.2. **[Get build dependencies manually](#32-get-build-dependencies-manually)**   
  3.3. **[Ubuntu <= 18.04](#33-ubuntu--1804)**
4. **[Build Kodi](#4-build-kodi)**

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

## 2. Get the source code
Make sure `git` is installed:
```
sudo apt install git
```

Change to your `home` directory:
```
cd $HOME
```

Clone Kodi's current master branch:
```
git clone https://github.com/xbmc/xbmc kodi
```

**[back to top](#table-of-contents)**

## 3. Install the required packages
You can install the required packages using one of two methods: automagically or manually. Please use the former whenever possible.

> [!WARNING]  
> Oldest supported Ubuntu version is 16.04 (Xenial). It is possible to build on older Ubuntu releases but due to outdated packages it will require considerable fiddling. Sorry, you're on your own if you decide to go down that particular rabbit hole.

### 3.1. Get build dependencies automagically
Add Kodi's *nightly* PPA to grab dependencies:
```
sudo add-apt-repository -s ppa:team-xbmc/xbmc-nightly
```

If you're using Ubuntu 16.04, *build-depends* PPA is also required:
```
sudo add-apt-repository ppa:team-xbmc/xbmc-ppa-build-depends
sudo apt update
```

Super-duper magic command to get the build dependencies:
```
sudo apt build-dep kodi
```

> [!WARNING]  
> Do not use `aptitude` for the `build-dep` command. It doesn't resolve everything properly.

If at a later point you decide you do not want Kodi's PPAs on your system, removing them is as easy as:
```
sudo add-apt-repository -r ppa:team-xbmc/xbmc-nightly
sudo add-apt-repository -r ppa:team-xbmc/xbmc-ppa-build-depends
```

> [!NOTE]  
> For developers and anyone else who builds frequently it is recommended to install `ccache` to expedite subsequent builds of Kodi.

You can install it with:
```
sudo apt install ccache
```

> [!TIP]
> If you have multiple computers at home, `distcc` will distribute build workloads of C and C++ code across several machines on a network. Team Kodi may not be willing to give support if problems arise using such a build configuration.

You can install it with:
```
sudo apt install distcc
```

### 3.2. Get build dependencies manually
If you get a `package not found` type of message with the below command, remove the offending package(s) from the install list and reissue the command. Take a note of the missing dependencies and, after a successful step completion, **[build the missing dependencies manually](README.Linux.md#31-build-missing-dependencies)**.

> [!NOTE]  
> Kodi requires a compiler with C++17 support, i.e. gcc >= 7 or clang >= 5

Install build dependencies manually:
```
sudo apt install debhelper autoconf automake autopoint gettext autotools-dev cmake curl default-jre doxygen gawk gcc gdc gperf libasound2-dev libass-dev libavahi-client-dev libavahi-common-dev libbluetooth-dev libbluray-dev libbz2-dev libcdio-dev libp8-platform-dev libcrossguid-dev libcurl4-openssl-dev libcwiid-dev libdbus-1-dev libdrm-dev libegl1-mesa-dev libenca-dev libflac-dev libfmt-dev libfontconfig-dev libfreetype6-dev libfribidi-dev libfstrcmp-dev libgcrypt-dev libgif-dev libgles2-mesa-dev libgl1-mesa-dev libglu1-mesa-dev libgnutls28-dev libgpg-error-dev libgtest-dev libiso9660-dev libjpeg-dev liblcms2-dev libltdl-dev liblzo2-dev libmicrohttpd-dev libmysqlclient-dev libnfs-dev libogg-dev libpcre3-dev libplist-dev libpng-dev libpulse-dev libshairplay-dev libsmbclient-dev libspdlog-dev libsqlite3-dev libssl-dev libtag1-dev libtiff5-dev libtinyxml-dev libtinyxml2-dev libtool libudev-dev libunistring-dev libva-dev libvdpau-dev libvorbis-dev libxmu-dev libxrandr-dev libxslt1-dev libxt-dev lsb-release meson nasm ninja-build python3-dev python3-pil python3-pip rapidjson-dev swig unzip uuid-dev zip zlib1g-dev
```

> [!WARNING]  
> Make sure you copy paste the entire line or you might receive an error or miss a few dependencies.

If you're using Ubuntu 16.04, you also need to install:
```
sudo apt install libcec4-dev libfmt3-dev liblircclient-dev
```

If you're using Ubuntu 18.04 and later, you also need to install:
```
sudo apt install libcec-dev libfmt-dev liblirc-dev
```

Flatbuffers (Ubuntu < 20.04, from PPA repository):
```
sudo apt install flatbuffers-dev
```

Flatbuffers (Ubuntu >= 20.04):
```
sudo apt install libflatbuffers-dev
```

Building for Wayland requires some extra packages:
```
sudo apt install libglew-dev libwayland-dev libxkbcommon-dev waylandpp-dev wayland-protocols
```

Similarly, building for GBM also requires some extra packages:
```
sudo apt install libgbm-dev libinput-dev libxkbcommon-dev
```

Optional packages that you might want to install for extra functionality (generating doxygen documentation, for instance):
```
sudo apt install doxygen libcap-dev libsndio-dev libmariadbd-dev
```

**[back to top](#table-of-contents)**

### 3.3 Ubuntu <= 18.04
Ubuntu 18.04 and lower ship an outdated [Meson](https://mesonbuild.com/) and [nasm](https://nasm.us/) package. 
In order to compile Kodi with AV1 support, you have to manually update both Meson and nasm.

> This is a requirement by the [dav1d](https://code.videolan.org/videolan/dav1d) AV1 decoder used in ffmpeg.

#### Meson
```
pip3 install --user meson
```

Make sure `~/.local/bin` is in your PATH.

Verify your Meson version by running `meson -v`. The version displayed should be >= 0.47.0.

#### nasm (x86_64 / amd64)
```
wget https://mirrors.edge.kernel.org/ubuntu/pool/universe/n/nasm/nasm_2.14-1_amd64.deb && sudo apt install ./nasm_2.14-1_amd64.deb
```

Verify your nasm version by running `nasm -v`. The version displayed should be >= 2.14.

## 4. Build Kodi
See the general **[Linux build guide](README.Linux.md)** for reference.

**[back to top](#table-of-contents)**

