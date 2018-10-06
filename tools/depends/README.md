![Kodi Logo](../../docs/resources/banner_slim.png)

# Kodi's Unified Depends Build System
This builds native tools and library dependencies for platforms that do not provide them. It is used on our continuous integration system, **[jenkins](http://jenkins.kodi.tv/)**. A nice side effect is that it allows us to use the same tools and library versions across all platforms.

In terms of build system usage, largest percentage is Autotools, followed by CMake and, in rare cases, hand crafted Makefiles. Tools and libraries versions are picked for a reason. If you feel the urge to start bumping them, be prepared for robust testing. Some tools and libraries need patching, most do not.

That said, we try to stay fairly current with used versions and send patches upstream whenever possible.


* **Autotools driven tools and libraries** tend to just work **provided** the author(s) followed proper Autotools format. Execute `./bootstrap`, followed by `./configure --[...]` and you're all set. If `./configure --[...]` gives you problems, try `./autoreconf -vif` before `./configure --[...]`.
Some authors do silly things and only a `config.site` can correct the errors. Watch for this in the config.site(.in) file(s). It is the only way to handle bad Autotools behaviour.

* **CMake driven tools and libraries** also tend to just work. Setup CMake flags correctly and go. On rare cases, you might need to diddle the native CMake setup.

* **Hand crafted Makefiles driven tools and libraries** typically require manual sed tweaks or patching. May give you nightmares.

## Usage Examples
Paths below are examples. If you want to build Kodi, follow our **[build guides](../../docs/README.md)**.
### All platforms
`./bootstrap`
### Darwin
**macOS (i386)**  
`./configure --host=i386-apple-darwin`

**macOS (x86_64)**  
`./configure --host=x86_64-apple-darwin`

**iOS (armv7)**  
`./configure --host=arm-apple-darwin`

**iOS (arm64)**  
`./configure --host=arm-apple-darwin --with-cpu=arm64`

**tvOS**  
`./configure --host=arm-apple-darwin --with-platform=tvos`

**NOTE:** You can target the same `--prefix=` path. Each setup will be done in an isolated directory. The last configure/make you do is the one used for Kodi/Xcode.
 
### Android
**arm**  
`./configure --with-tarballs=$HOME/android-tools/xbmc-tarballs --host=arm-linux-androideabi --with-sdk-path=$HOME/android-tools/android-sdk-linux --with-ndk-path=$HOME/android-tools/android-ndk-r18 --with-toolchain=$HOME/android-tools/arm-linux-androideabi-vanilla/android-21 --prefix=$HOME/android-tools/xbmc-depends`

**aarch64**  
`./configure --with-tarballs=$HOME/android-tools/xbmc-tarballs --host=aarch64-linux-android --with-sdk-path=$HOME/android-tools/android-sdk-linux --with-ndk-path=$HOME/android-tools/android-ndk-r18 --with-toolchain=$HOME/android-tools/aarch64-linux-android-vanilla/android-21 --prefix=$HOME/android-tools/xbmc-depends`

**x86**  
`./configure --with-tarballs=$HOME/android-tools/xbmc-tarballs --host=i686-linux-android --with-sdk-path=$HOME/android-tools/android-sdk-linux --with-ndk-path=$HOME/android-tools/android-ndk-r18 --with-toolchain=$HOME/android-tools/x86-linux-android-vanilla/android-21 --prefix=$HOME/android-tools/xbmc-depends`

### Linux
**ARM (codesourcery/lenaro/etc)**  
`./configure --with-toolchain=/opt/toolchains/my-example-toolchain/ --prefix=/opt/xbmc-deps --host=arm-linux-gnueabi`

**Raspberry Pi**  
`./configure --with-platform=raspberry-pi --host=arm-linux-gnueabihf --prefix=/opt/xbmc-deps --with-tarballs=/opt/xbmc-tarballs --with-toolchain=/opt/rbp-dev/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf --with-firmware=/opt/rbp-dev/firmware --build=i686-linux`

**Native**  
`./configure --with-toolchain=/usr --prefix=/opt/xbmc-deps --host=x86_64-linux-gnu`

Cross compiling is a PITA.


# List of dependencies
Below is the list of currently used dependencies including the upstream location

| library | license | website | upstream | alternative |
| --------| ------- | ------- | -------- | ----------- |
| alsa-lib | | [link](https://www.alsa-project.org/main/index.php/Main_Page) | [link](http://git.alsa-project.org/?p=alsa-lib.git;a=summary)
| boblight | | | [link](https://github.com/bobo1on1/boblight)
| bzip2 |
| crossguid |  | [link](https://github.com/graeme-hill/crossguid) | [git](https://github.com/graeme-hill/crossguid)
| curl |  | [link](https://curl.haxx.se/) | [git](https://github.com/curl/curl)
| dbus |  | [link](https://dbus.freedesktop.org/) | [link](https://cgit.freedesktop.org/dbus/dbus/)
| dummy-libxbmc |  | [link]() | [link]()
| expat |  | [link](https://libexpat.github.io/) | [git](https://github.com/libexpat/libexpat)
| ffmpeg |  | [link](https://www.ffmpeg.org/) | [git](https://git.ffmpeg.org/gitweb/ffmpeg.git) | [git](https://github.com/xbmc/FFmpeg)
| flatbuffers |  | [link](https://google.github.io/flatbuffers/) | [link](http://github.com/google/flatbuffers)
| fontconfig |  | [link](https://www.fontconfig.org/) | [git](https://gitlab.freedesktop.org/fontconfig/fontconfig) 
| freetype2 |  | [link](https://www.freetype.org/) | [site](https://download.savannah.gnu.org/releases/freetype/) 
| fribidi |   | [link](https://github.com/fribidi/fribidi) | [git](https://github.com/fribidi/fribidi) 
| gettext |   | [link](https://www.gnu.org/software/gettext/) | [ftp](http://ftp.gnu.org/pub/gnu/gettext/) 
| gmp |   | [link](https://gmplib.org/)
| gnutls |   | [link](https://www.gnutls.org/) | [link](https://gitlab.com/gnutls/gnutls/blob/master/) 
| iosentitlements 
| libamcodec
| libandroidjni |   | [link](https://github.com/xbmc/libandroidjni) | [git](https://github.com/xbmc/libandroidjni) 
| libass |   | [link](https://github.com/libass/libass) | [link](https://github.com/libass/libass) | [link](https://github.com/xbmc/libass)
| libbluray |   | [link](https://www.videolan.org/developers/libbluray.html) | [link](https://code.videolan.org/videolan/libbluray) | [link](https://github.com/xbmc/libbluray) 
| libcdio |   | [link](http://www.gnu.org/s/libcdio/) | [link](http://git.savannah.gnu.org/cgit/libcdio.git) 
| libcdio-gplv3 |   | [link](http://www.gnu.org/s/libcdio/) | [link](http://git.savannah.gnu.org/cgit/libcdio.git) 
| libcec |   | [link]() | [link](https://github.com/Pulse-Eight/libcec)| [link](https://github.com/xbmc/libcec)
| libdvdcss |   | [link](https://www.videolan.org/developers/libdvdcss.html) | [link](https://code.videolan.org/videolan/libdvdcss) | [link](https://github.com/xbmc/libdvdcss)
| libdvdnav |    | [link](https://www.videolan.org/developers/libdvdnav.html) | [link](https://code.videolan.org/videolan/libdvdnav) | [link](https://github.com/xbmc/libdvdnav)
| libdvdread |   | [link](https://www.videolan.org/developers/libdvdnav.html) | [link](https://code.videolan.org/videolan/libdvdread) | [link](https://github.com/xbmc/libdvdread) 
| libevdev |   | [link](https://www.freedesktop.org/wiki/Software/libevdev/) | [link](http://www.freedesktop.org/software/libevdev/) 
| libffi |   | [link](http://sourceware.org/libffi/) | [link](http://github.com/libffi/libffi) 
| libfmt |   | [link](https://github.com/fmtlib/fmt) | [link](https://github.com/fmtlib/fmt) 
| libfstrcmp 
| libgcrypt |   | [link](https://www.gnupg.org/software/libgcrypt/index.html) | [link](https://www.gnupg.org/ftp/gcrypt/libgcrypt/) 
| libgpg-error |   | [link](https://gnupg.org/software/libgpg-error/index.html) | [link](https://gnupg.org/ftp/gcrypt/libgpg-error/) 
| libiconv
| libinput |   | [link](https://www.freedesktop.org/wiki/Software/libinput/) | [link](https://gitlab.freedesktop.org/libinput/libinput/) 
| libjpeg-turbo |   | [link](https://libjpeg-turbo.org/) | [link](https://github.com/libjpeg-turbo/libjpeg-turbo) 
| liblzo2 |   | [link](http://www.oberhumer.com/opensource/lzo/) | [link](http://www.oberhumer.com/opensource/lzo/download/) 
| libmicrohttpd |   | [link](https://www.gnu.org/software/libmicrohttpd/)
| libnfs |   | [link](https://github.com/sahlberg/libnfs) | [link](https://github.com/sahlberg/libnfs) 
| libplist |   | [link](https://github.com/libimobiledevice/libplist) | [link](https://github.com/libimobiledevice/libplist) 
| libpng |   | [link](http://www.libpng.org/pub/png/libpng.html) | [link](https://sourceforge.net/projects/libpng/files/) 
| libsdl |   | [link](https://www.libsdl.org/)
| libshairplay |   | [link](https://github.com/juhovh/shairplay) | [link](https://github.com/juhovh/shairplay) 
| libudev |   | [link](https://www.freedesktop.org/software/systemd/man/libudev.html)
| libusb |   | [link](https://libusb.info/) | [link](https://github.com/libusb/libusb) 
| libuuid |   | [link](https://linux.die.net/man/3/libuuid) | [link](https://git.kernel.org/pub/scm/fs/ext2/e2fsprogs.git) 
| libxkbcommon |   | [link](https://xkbcommon.org/) | [link](https://github.com/xkbcommon/libxkbcommon) 
| libxml2 |   | [link](http://xmlsoft.org/)
| libxslt |   | [link](http://xmlsoft.org/libxslt/)
| libzip |   | [link](https://libzip.org/) | [link](https://github.com/nih-at/libzip/) 
| mariadb |   | [link](https://mariadb.org/) | [link](https://github.com/mariadb/server) 
| mtdev |   | [link](http://bitmath.org/code/mtdev/)
| nettle
| openssl |   | [link](https://www.openssl.org/) | [link](https://github.com/openssl/openssl) 
| p8-platform
| pcre |   | [link](https://www.pcre.org/) | [link](https://vcs.pcre.org/pcre2/) 
| platform
| python27
| pythonmodule-pil
| pythonmodule-pycryptodome 
| pythonmodule-setuptools
| rapidjson |   | [link](http://rapidjson.org/index.html) | [link](https://github.com/Tencent/rapidjson/) 
| samba
| samba-gplv3
| sqlite3 |   | [link](https://www.sqlite.org/) | [link](https://www.sqlite.org/cgi/src/doc/trunk/README.md) 
| taglib | | [link](https://taglib.org/) | [link](https://github.com/taglib/taglib) 
| tinyxml |   | [link](http://www.grinninglizard.com/tinyxml/) | [link](https://sourceforge.net/projects/tinyxml/files/tinyxml/) 
| wayland 
| wayland-protocols
| waylandpp
| zlib |   | [link](https://zlib.net/) | [link](https://github.com/madler/zlib) 