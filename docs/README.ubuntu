TOC
1. Introduction
2. Getting the source code
3. Installing the required Ubuntu packages
4. How to compile
5. Uninstalling

-----------------------------------------------------------------------------
1. Introduction
-----------------------------------------------------------------------------

A graphics-adapter with OpenGL acceleration is highly recommended.
24/32 bitdepth is required along with OpenGL.

Note to new Linux users.
All lines that are prefixed with the '$' character are commands, that need to be typed
into a terminal window / console (similar to the command prompt for Windows).
Note that the '$' character itself should NOT be typed as part of the command.

-----------------------------------------------------------------------------
2. Getting the source code
-----------------------------------------------------------------------------

.0  $ cd $HOME
.1  $ git clone git://github.com/xbmc/xbmc.git kodi

Note: You can clone any specific branch.

.1  $ git clone -b <branch> git://github.com/xbmc/xbmc.git kodi

-----------------------------------------------------------------------------
3. Installing the required Ubuntu packages
-----------------------------------------------------------------------------

Two methods exist to install the required Ubuntu packages:

[NOTICE] For supported older Ubuntu versions, some packages might be outdated.
         For those, you can either compile them manually, or use our backports
         available from our official stable PPA:

         http://launchpad.net/~team-xbmc/+archive/ppa

-----------------------------------------------------------------------------
3a. Use a single command to get all build dependencies
-----------------------------------------------------------------------------
[NOTICE] Supported on Ubuntu >= 11.10 (oneiric)

You can get all build dependencies used for building the packages on the PPA

Add the unstable PPA:

For <= 12.04 lts:
    $ sudo apt-get install python-software-properties
    $ sudo add-apt-repository ppa:team-xbmc/xbmc-nightly

For >= 14.04 lts:
    $ sudo apt-get install software-properties-common
    $ sudo add-apt-repository -s ppa:team-xbmc/xbmc-nightly

Add build-depends PPA:
    $ sudo add-apt-repository ppa:team-xbmc/xbmc-ppa-build-depends

Here is the magic command to get the build dependencies (used to compile the version on the PPA).
    $ sudo apt-get update
    $ sudo apt-get build-dep kodi

Optional: If you do not want Kodi to be installed via PPA, you can removed the PPAs again:
    $ sudo add-apt-repository -r ppa:team-xbmc/xbmc-nightly
    $ sudo add-apt-repository -r ppa:team-xbmc/xbmc-ppa-build-depends

Note: Do not use "aptitude" for the build-dep command. It doesn't resolve everything properly.
      For developers and anyone else who compiles frequently it is recommended to use ccache
    $ sudo apt-get install ccache

Tip: For those with multiple computers at home is to try out distcc
    (fully unsupported from Kodi of course)
    $ sudo apt-get install distcc

-----------------------------------------------------------------------------
3b. Alternative: Manual dependency installation
-----------------------------------------------------------------------------

For Ubuntu (all versions >= 7.04):
    $ sudo apt-get install automake bison build-essential cmake curl cvs \
      default-jre fp-compiler gawk gdc gettext git-core gperf libasound2-dev libass-dev \
      libbz2-dev libcap-dev libcdio-dev libcurl3 \
      libcurl4-openssl-dev libdbus-1-dev libfontconfig-dev libegl1-mesa-dev libfreetype6-dev \
      libfribidi-dev libgif-dev libiso9660-dev libjpeg-dev liblzo2-dev \
      libmicrohttpd-dev libmodplug-dev libmysqlclient-dev libnfs-dev \
      libpcre3-dev libplist-dev libpng-dev libpulse-dev libsdl2-dev libsmbclient-dev \
      libsqlite3-dev libssh-dev libssl-dev libtinyxml-dev libtool libudev-dev libusb-dev \
      libva-dev libvdpau-dev libxml2-dev libxmu-dev libxrandr-dev \
      libxrender-dev libxslt1-dev libxt-dev libyajl-dev mesa-utils nasm pmount python-dev \
      python-imaging python-sqlite swig unzip uuid-dev yasm zip zlib1g-dev

For >= 10.10:
    $ sudo apt-get install autopoint libltdl-dev

For >= 12.04 lts: Backport for Precise of libyajl2
    $ sudo add-apt-repository ppa:team-xbmc/xbmc-nightly
    $ sudo apt-get install libyajl-dev

Note: Ubuntu Precise users also need a upgraded GCC, else compile will fail.

For >= 12.10:
    $ sudo apt-get install libtag1-dev

On 8.10 and older versions, libcurl is outdated and thus Kodi will not compile properly.
In this case you will have to manually compile the latest version.
    $ wget http://curl.sourceforge.net/download/curl-7.19.7.tar.gz
    $ tar -xzf curl-7.19.7.tar.gz
    $ cd curl-7.19.7
    $ ./configure --disable-ipv6 --without-libidn --disable-ldap --prefix=/usr
    $ make
    $ sudo make install

For <= 12.04
Kodi needs a new version of taglib other than what is available at this time.
Use prepackaged from the Kodi build-depends PPA.
0.  $ sudo apt-get install libtag1-dev

We also supply a Makefile in lib/taglib to make it easy to install into /usr/local.
1.  $ sudo apt-get remove libtag1-dev
    $ make -C lib/taglib
    $ sudo make -C lib/taglib install

[NOTICE] crossguid / libcrossguid-dev all Linux destributions.
Kodi now requires crossguid which is not available in Ubuntu repositories at this time.
If build-deps PPA doesn't provide a pre-packaged version for your distribution, see (1.) below.

Use prepackaged from the Kodi build-depends PPA.
0.  $ sudo apt-get install libcrossguid-dev

We also supply a Makefile in tools/depends/target/crossguid
to make it easy to install into /usr/local.
1.  $ make -C tools/depends/target/crossguid PREFIX=/usr/local

Unless you are proficient with how Linux libraries and versions work, do not
try to provide it yourself, as you will likely mess up for other programs.

-----------------------------------------------------------------------------
4. How to compile
-----------------------------------------------------------------------------
See README.linux

-----------------------------------------------------------------------------
4.1. Test Suite
-----------------------------------------------------------------------------
See README.linux

-----------------------------------------------------------------------------
5. Uninstalling
-----------------------------------------------------------------------------
Remove any PPA installed Kodi.
    $ sudo apt-get remove kodi* xbmc*

See README.linux/Uninstalling for removing compiled versions of Kodi.
EOF
