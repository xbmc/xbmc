![Kodi Logo](resources/banner_slim.png)

# Linux build guide
This is the general Linux build guide. Please read it in full before you proceed to familiarize yourself with the build procedure.

Several distribution **[specific build guides](README.md)** are available.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Get the source code](#2-get-the-source-code)**
3. **[Install the required packages](#3-install-the-required-packages)**  
  3.1. **[Build missing dependencies](#31-build-missing-dependencies)**  
  3.2. **[Enable internal dependencies](#32-enable-internal-dependencies)**
4. **[Build Kodi](#4-build-kodi)**  
  4.1. **[Configure build](#41-configure-build)**  
  4.2. **[Build](#42-build)**
5. **[Build binary add-ons](#5-build-binary-add-ons)**  
  5.1. **[In-tree building of binary add-ons](#51-in-tree-building-of-binary-add-ons)**  
  5.2. **[Out-of-tree building of binary add-ons](#52-out-of-tree-building-of-binary-add-ons)**
6. **[Run Kodi](#6-run-kodi)**
7. **[Uninstall Kodi](#7-uninstall-kodi)**
8. **[Test suite](#8-test-suite)**

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

## 2. Get the source code
First install the `git` package provided by your distribution. How to do it can be found with a quick search in your favorite search engine.

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
The following is the list of packages that are used to build Kodi on Debian/Ubuntu (with all supported external libraries enabled).

**NOTE:** Kodi requires a compiler with C++14 support, i.e. gcc >= 4.9 or clang >= 3.4

* autoconf, automake, autopoint, gettext, autotools-dev, cmake, curl, default-jre | openjdk-6-jre | openjdk-7-jre, gawk, gcc (>= 4.9) | gcc-4.9, g++ (>= 4.9) | g++-4.9, cpp (>= 4.9) | cpp-4.9, flatbuffers, gdc, gperf, libasound2-dev | libasound-dev, libass-dev (>= 0.9.8), libavahi-client-dev, libavahi-common-dev, libbluetooth-dev, libbluray-dev, libbz2-dev, libcdio-dev, libcec4-dev | libcec-dev, libp8-platform-dev, libcrossguid-dev, libcurl4-openssl-dev | libcurl4-gnutls-dev | libcurl-dev, libcwiid-dev, libdbus-1-dev, libegl1-mesa-dev, libenca-dev, libflac-dev, libfontconfig-dev, libfmt3-dev | libfmt-dev, libfreetype6-dev, libfribidi-dev, libfstrcmp-dev, libgcrypt-dev, libgif-dev (>= 5.0.5), libgles2-mesa-dev [armel] | libgl1-mesa-dev | libgl-dev, libglew-dev, libglu1-mesa-dev | libglu-dev, libgnutls-dev | libgnutls28-dev, libgpg-error-dev, libiso9660-dev, libjpeg-dev, liblcms2-dev, liblirc-dev, libltdl-dev, liblzo2-dev, libmicrohttpd-dev, libmysqlclient-dev, libnfs-dev, libogg-dev, libomxil-bellagio-dev [armel], libpcre3-dev, libplist-dev, libpng12-dev | libpng-dev, libpulse-dev, libshairplay-dev, libsmbclient-dev, libsqlite3-dev, libssl-dev, libtag1-dev (>= 1.8) | libtag1x8, libtiff5-dev | libtiff-dev | libtiff4-dev, libtinyxml-dev, libtool, libudev-dev, libva-dev, libvdpau-dev, libvorbis-dev, libxkbcommon-dev, libxmu-dev, libxrandr-dev, libxslt1-dev | libxslt-dev, libxt-dev, waylandpp-dev | netcat, wayland-protocols | wipe, lsb-release, meson (>= 0.47.0), nasm, ninja-build, python3-dev, python3-pil | python-imaging, python-support | python3-minimal, rapidjson-dev, swig, unzip, uuid-dev, yasm, zip, zlib1g-dev

### 3.1. Build missing dependencies
Some packages may be missing or outdated in older distributions. Notably `crossguid, libfmt, waylandpp, wayland-protocols, etc.` are known to be outdated or missing. Fortunately there is an easy way to build individual dependencies with **[Kodi's unified depends build system](../tools/depends/README.md)**.

Change to Kodi's source code directory:
```
cd $HOME/kodi
```

Build and install crossguid:
```
sudo make -C tools/depends/target/crossguid PREFIX=/usr/local
```

Build and install flatbuffers:
```
sudo make -C tools/depends/target/flatbuffers PREFIX=/usr/local
```

Build and install libfmt:
```
sudo make -C tools/depends/target/libfmt PREFIX=/usr/local
```

Build and install wayland-protocols:
```
sudo make -C tools/depends/target/wayland-protocols PREFIX=/usr/local
```

Build and install waylandpp:
```
sudo make -C tools/depends/target/waylandpp PREFIX=/usr/local
```

**WARNING:** Building `waylandpp` has some dependencies of its own, namely `scons, libwayland-dev (>= 1.11.0) and libwayland-egl1-mesa`

**TIP:** Complete list of dependencies is available **[here](https://github.com/xbmc/xbmc/tree/master/tools/depends/target)**.

### 3.2. Enable internal dependencies
Some dependencies can be configured to build before Kodi. That's the case with `flatbuffers`, `crossguid, libfmt, rapidjson and dav1d`. To enable the internal build of a dependency, append `-DENABLE_INTERNAL_<DEPENDENCY_NAME>=ON` to the configure command below. For example, configuring an X11 build with internal `fmt` would become `cmake ../kodi -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_INTERNAL_FMT=ON` instead of `cmake ../kodi -DCMAKE_INSTALL_PREFIX=/usr/local`.

**[back to top](#table-of-contents)** | **[back to section top](#3-installing-the-required-packages)**

## 4. Build Kodi
### 4.1. Configure build
If you get a `Could NOT find...` error message during CMake configuration step, take a note of the missing dependencies and either install them from repositories (if available) or **[build the missing dependencies manually](#31-build-missing-dependencies)**.

Create an out-of-source build directory:
```
mkdir $HOME/kodi-build
```

**TIP:** Look for comments starting with `Or ...` and only execute the command(s) you need.

Change to build directory:
```
cd $HOME/kodi-build
```

Configure build for X11:
```
cmake ../kodi -DCMAKE_INSTALL_PREFIX=/usr/local -DX11_RENDER_SYSTEM=gl
```

**NOTE:** You can use `gles` instead of `gl` if you want to build with `GLES`.

Or configure build for Wayland:
```
cmake ../kodi -DCMAKE_INSTALL_PREFIX=/usr/local -DCORE_PLATFORM_NAME=wayland -DWAYLAND_RENDER_SYSTEM=gl
```

**NOTE:** You can use `gles` instead of `gl` if you want to build with `GLES`.

Or configure build for GBM:
```
cmake ../kodi -DCMAKE_INSTALL_PREFIX=/usr/local -DCORE_PLATFORM_NAME=gbm -DGBM_RENDER_SYSTEM=gles
```

**NOTE:** You can use `gl` instead of `gles` if you want to build with `GL`.

### 4.2. Build
```
cmake --build . -- VERBOSE=1 -j$(getconf _NPROCESSORS_ONLN)
```
**TIP:** By adding `-j<number>` to the make command, you can choose how many concurrent jobs will be used and expedite the build process. It is recommended to use `-j$(getconf _NPROCESSORS_ONLN)` to compile on all available processor cores. The build machine can also be configured to do this automatically by adding `export MAKEFLAGS="-j$(getconf _NPROCESSORS_ONLN)"` to your shell config (e.g. `~/.bashrc`).

After the build process completes successfully you can test your shiny new Kodi build while in the build directory:
```
./kodi-x11
```

Or if you built for Wayland:
```
./kodi-wayland
```

Or if you built for GBM:
```
./kodi-gbm
```

**WARNING:** User running `kodi-gbm` needs to be part of `input` and `video` groups. Otherwise you'll have to use `sudo`.

Add user to input and video groups:
```
sudo usermod -a -G input,video <username>
```

You will need to log out and log back in to see the new groups added to your user. Check groups your user belongs to with:
```
groups
```

If everything was OK during your test you can now install the binaries to their place, in this example */usr/local*.
```
sudo make install
```

This will install Kodi in the prefix provided in **[section 4.1](#41-configure-build)**.

**TIP:** To override Kodi's install location, use `DESTDIR=<path>`. For example:
```
sudo make install DESTDIR=$HOME/kodi
```

**[back to top](#table-of-contents)** | **[back to section top](#4-build-kodi)**

## 5. Build binary add-ons
You can find a complete list of available binary add-ons **[here](https://github.com/xbmc/repo-binary-addons)**.

In the following, two approaches to building binary add-ons are described.
While the workflow of in-tree building is more automated,
it is only supported as long as `-DCMAKE_INSTALL_PREFIX=/usr/local` is not changed from it's default of `/usr/local`.
Thus when changing `DCMAKE_INSTALL_PREFIX`, you must follow the out-of-tree building instructions.

### 5.1. In-tree building of binary add-ons

Change to Kodi's source code directory:
```
cd $HOME/kodi
```

Build all add-ons:
```
sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons PREFIX=/usr/local
```

Build specific add-ons:
```
sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons PREFIX=/usr/local ADDONS="audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu"
```

Build a specific group of add-ons:
```
sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons PREFIX=/usr/local ADDONS="pvr.*"
```

**NOTE:** `PREFIX=/usr/local` should match Kodi's `-DCMAKE_INSTALL_PREFIX=` prefix used in **[section 4.1](#41-configure-build)**.

**[back to top](#table-of-contents)**


### 5.2. Out-of-tree building of binary add-ons

You can find a complete list of available binary add-ons **[here](https://github.com/xbmc/repo-binary-addons)**.
Exemplary, to install `pvr.demo`, follow below steps.
For other addons, simply adapt the repository based on the information found in the `.txt` associated with the respective addon **[here](https://github.com/xbmc/repo-binary-addons)**

Some addons have dependencies.
You must install all required dependencies of an addon before installing the addon.
Required dependencies can be found by checking the `depends` folder and
it's subdirectories in the repository of the respective addons.

A number of addons require the the `p8-platform` and `kodi-platform` add-ons.
Note that dependencies on `p8-platform` and `kodi-platform` are typically not declared in the `depends` folder.
They are only declared in the `CMakeLists.txt` file of the respective addon (e.g. via `find_package(p8-platform REQUIRED)`).
Below we demonstrate how to build these two.
First, the platform addon:

```
cd ~/src/
git clone https://github.com/xbmc/platform.git
cd ~/src/platform/
cmake -DCMAKE_INSTALL_PREFIX=/usr/local
make && make install
```

Then the kodi-platform add-on:

```
cd ~/src/
git clone https://github.com/xbmc/kodi-platform.git
cd ~/src/kodi-platform/
cmake -DCMAKE_INSTALL_PREFIX=/usr/local
make && make install
```

Finally, to install pvr.demo

```
cd ~/src
git clone https://github.com/kodi-pvr/pvr.demo.git
cd ~/src/pvr.demo/
cmake -DCMAKE_INSTALL_PREFIX=/usr/local
make && make install
```

**NOTE:** `-DCMAKE_INSTALL_PREFIX=` should match Kodi's `-DCMAKE_INSTALL_PREFIX=` prefix used in **[section 4.1](#41-configure-build)**.

## 6. Run Kodi
If you chose to install Kodi using `/usr` or `/usr/local` as the `-DCMAKE_INSTALL_PREFIX=`, you can just issue *kodi* in a terminal session.

If you changed `-DCMAKE_INSTALL_PREFIX=` to install Kodi into some non-standard location, you will have to run Kodi directly:
```
<CMAKE_INSTALL_PREFIX>/bin/kodi
```

To run Kodi in *portable* mode (useful for testing):
```
<CMAKE_INSTALL_PREFIX>/bin/kodi -p
```

**[back to top](#table-of-contents)**

## 7. Uninstall Kodi
```
sudo make uninstall
```
**WARNING:**: If you reran CMakes' configure step with a different `-DCMAKE_INSTALL_PREFIX=`, you will need to rerun configure with the correct path for this step to work correctly.

If you would like to also remove any settings and third-party addons (skins, scripts, etc.) and Kodi configuration files, you should also run:
```
rm -rf ~/.kodi
```

**[back to top](#table-of-contents)**

## 8. Test suite
Kodi has a test suite which uses the Google C++ Testing Framework. This framework is provided directly in Kodi's source tree.

Build and run Kodi's test suite:
```
make check
```

Build Kodi's test suite without running it:
```
make kodi-test
```

Run Kodi's test suite manually:
```
./kodi-test
```

Show Kodi's test suite *help* notes:
```
./kodi-test --gtest_help
```

Useful options:
```
--gtest_list_tests
  List the names of all tests instead of running them.
  The name of TEST(Foo, Bar) is "Foo.Bar".

--gtest_filter=POSITIVE_PATTERNS[-NEGATIVE_PATTERNS]
  Run only the tests whose name matches one of the positive patterns but
  none of the negative patterns. '?' matches any single character; '*'
  matches any substring; ':' separates two patterns.
```

**[back to top](#table-of-contents)**

