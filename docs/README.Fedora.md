![Kodi Logo](resources/banner_slim.png)

# Fedora build guide
This guide has been tested with Fedora 36 1.1 x86_64. Please read it in full before you proceed to familiarize yourself with the build procedure.

Several other distributions have **[specific build guides](README.md)** and a general **[Linux build guide](README.Linux.md)** is also available.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Get the source code](#2-get-the-source-code)**
3. **[Install the required packages](#3-install-the-required-packages)**
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
sudo dnf install git
```

Clone Kodi's current master branch:
```
cd $HOME
git clone https://github.com/xbmc/xbmc kodi
```

**[back to top](#table-of-contents)**

## 3. Install the required packages
If you get a `package not found` type of message with the below command, remove the offending package(s) from the install list and reissue the command. Take a note of the missing dependencies and, after a successful step completion, **[build the missing dependencies manually](README.Linux.md#31-build-missing-dependencies)**.

> [!NOTE]  
> Kodi requires a compiler with C++17 support, i.e. gcc >= 7 or clang >= 5

Install build dependencies:
```
sudo dnf install alsa-lib-devel autoconf automake avahi-compat-libdns_sd-devel avahi-devel bluez-libs-devel bzip2-devel cmake curl dbus-devel flatbuffers flatbuffers-devel fmt-devel fontconfig-devel freetype-devel fribidi-devel fstrcmp-devel gawk gcc gcc-c++ gettext gettext-devel giflib-devel gperf gtest-devel java-11-openjdk-headless jre lcms2-devel libao-devel libass-devel libbluray-devel libcap-devel libcdio-devel libcec-devel libcurl-devel libidn2-devel libjpeg-turbo-devel libmicrohttpd-devel libmpc-devel libnfs-devel libplist-devel libpng12-devel libsmbclient-devel libtool libtool-ltdl-devel libudev-devel libunistring libunistring-devel libusb-devel libuuid-devel libva-devel libvdpau-devel libxkbcommon-devel libxml2-devel libXmu-devel libXrandr-devel libxslt-devel libXt-devel lirc-devel lzo-devel make mariadb-devel mesa-libEGL-devel mesa-libGL-devel mesa-libGLU-devel mesa-libGLw-devel mesa-libOSMesa-devel nasm openssl-devel openssl-libs patch pcre-devel pulseaudio-libs-devel python3-devel python3-pillow rapidjson-devel shairplay-devel spdlog-devel sqlite-devel swig taglib-devel tinyxml-devel tinyxml2-devel trousers-devel uuid-devel zlib-devel
```

> [!WARNING]  
> Make sure you copy paste the entire line or you might receive an error or miss a few dependencies.

Building for Wayland requires some extra packages:
```
sudo dnf install mesa-libGLES-devel wayland-devel waylandpp-devel wayland-protocols-devel
```

Similarly, building for GBM also requires some extra packages:
```
sudo dnf install libinput-devel libxkbcommon-devel mesa-libGLES-devel mesa-libgbm-devel
```

Optional packages that you might want to install for extra functionality (generating doxygen documentation, for instance):
```
sudo dnf install doxygen mariadb-devel
```

> [!NOTE]  
> For developers and anyone else who builds frequently it is recommended to install `ccache` to expedite subsequent builds of Kodi.

You can install it with:
```
sudo dnf install ccache
```

> [!TIP]
> If you have multiple computers at home, `distcc` will distribute build workloads of C and C++ code across several machines on a network. Team Kodi may not be willing to give support if problems arise using such a build configuration.

You can install it with:
```
sudo dnf install distcc
```

## 4. Build Kodi
See the general **[Linux build guide](README.Linux.md)** for reference.

**[back to top](#table-of-contents)**

