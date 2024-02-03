![Kodi Logo](resources/banner_slim.png)

# openSUSE build guide
This guide has been tested with openSUSE Tumbleweed x86_64. Please read it in full before you proceed to familiarize yourself with the build procedure.

Several other distributions have **[specific build guides](README.md)** and a general **[Linux build guide](README.Linux.md)** is also available.

**Do not use openSUSE Leap**. Wiser people than us decided that in 2018 `gcc v4.8.5` is the best **stable** release openSUSE Leap 42.3 can provide by default. Installing/using another release along side it is a real PITA.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Get the source code](#2-get-the-source-code)**
3. **[Install the required packages](#3-install-the-required-packages)**  
  3.1. **[Build missing dependencies](#31-build-missing-dependencies)**
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
sudo zypper install git
```

Clone Kodi's current master branch:
```
cd $HOME
git clone https://github.com/xbmc/xbmc kodi
```

**[back to top](#table-of-contents)**

## 3. Install the required packages
Add `opensuse-multimedia-libs` repository because some needed packages are non-OSS:
```
sudo zypper ar -f http://ftp.gwdg.de/pub/opensuse/repositories/multimedia:/libs/openSUSE_Tumbleweed/ opensuse-multimedia-libs
sudo zypper ref
```

> [!NOTE]  
> A message will ask you to accept the key. Enter `a`, the *trust always* option.

If you get a `package not found` type of message with the below command, remove the offending package(s) from the install list and reissue the command. Take a note of the missing dependencies and, after a successful step completion, **[build the missing dependencies manually](#31-build-missing-dependencies)**.

> [!NOTE]  
> Kodi requires a compiler with C++17 support, i.e. gcc >= 7 or clang >= 5

Install build dependencies:
```
sudo zypper install alsa-devel autoconf automake bluez-devel boost-devel capi4linux-devel ccache cmake doxygen flac-devel fribidi-devel fstrcmp-devel gcc gcc-c++ gettext-devel giflib-devel glew-devel googletest gperf java-openjdk libass-devel libavahi-devel libbluray-devel libbz2-devel libcap-devel libcap-ng-devel libcdio-devel libcec-devel libcurl-devel libdvdread-devel libgudev-1_0-devel libidn2-devel libjasper-devel libjpeg-devel liblcms2-devel libmad-devel libmicrohttpd-devel libmodplug-devel libmpeg2-devel libmysqlclient-devel libnfs-devel libogg-devel libpcap-devel libplist-devel libpng12-devel libpulse-devel libsamplerate-devel libsmbclient-devel libtag-devel libtiff-devel libtool libudev-devel libuuid-devel libva-devel libvdpau-devel libvorbis-devel libXrandr-devel libXrender-devel libxslt-devel lirc-devel lzo-devel make Mesa-libEGL-devel Mesa-libGLESv2-devel Mesa-libGLESv3-devel nasm patch pcre-devel python3-devel python3-Pillow randrproto-devel renderproto-devel shairplay-devel sqlite3-devel swig tinyxml-devel tinyxml2-devel
```

> [!WARNING]  
> Make sure you copy paste the entire line or you might receive an error or miss a few dependencies.

Building for Wayland requires some extra packages:
```
sudo zypper install wayland-devel libwayland-egl1 libwayland-egl-devel libxkbcommon-devel scons wayland-protocols-devel
```

Similarly, building for GBM also requires some extra packages:
```
sudo zypper install libgbm-devel libinput-devel libxkbcommon-devel
```

> [!WARNING]  
> Fedora repositories don't have install candidates for `libfmt`, `rapidjson` and `waylandpp`. See **[build missing dependencies manually](#31-build-missing-dependencies)** section before you proceed.

Optional packages that you might want to install for extra functionality (generating doxygen documentation, for instance):
```
sudo zypper install doxygen sndio-devel libmariadb-devel
```

> [!NOTE]  
> For developers and anyone else who builds frequently it is recommended to install `ccache` to expedite subsequent builds of Kodi.

You can install it with:
```
sudo zypper install ccache
```

> [!TIP]
> If you have multiple computers at home, `distcc` will distribute build workloads of C and C++ code across several machines on a network. Team Kodi may not be willing to give support if problems arise using such a build configuration.

You can install it with:
```
sudo zypper install distcc
```

### 3.1. Build missing dependencies
See the general **[Linux build guide](README.Linux.md)** for reference.

Change to Kodi's source code directory:
```
cd $HOME/kodi
```

Build and install missing dependencies from repositories (*flatbuffers*, *libfmt*, *rapidjson* and *waylandpp*):
```
sudo make -C tools/depends/target/flatbuffers PREFIX=/usr/local
sudo make -C tools/depends/target/libfmt PREFIX=/usr/local
sudo make -C tools/depends/target/rapidjson PREFIX=/usr/local
sudo make -C tools/depends/target/waylandpp PREFIX=/usr/local
```

**[back to top](#table-of-contents)** | **[back to section top](#3-install-the-required-packages)**

## 4. Build Kodi
See the general **[Linux build guide](README.Linux.md)** for reference.

**[back to top](#table-of-contents)**


