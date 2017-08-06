TOC
1. Introduction
2. Getting the source code
3. Installing the required libraries and headers
4. How to compile
   4.4 Binary addons
   4.5 Test suite
5. How to run
6. Uninstalling

-----------------------------------------------------------------------------
1. Introduction
-----------------------------------------------------------------------------

A graphics-adapter with OpenGL acceleration is highly recommended.
24/32 bitdepth is required along with OpenGL.

Note to new Linux users:
All lines that are prefixed with the '$' character are commands,
that need to be typed into a terminal window / console. The '$' equals the prompt.
Note: The '$' character itself should NOT be typed as part of the command.

-----------------------------------------------------------------------------
2. Getting the source code
-----------------------------------------------------------------------------

You will have to grab the source code of course, here we use git as example.
First install the git package provided by your distribution.
Then from a terminal, type:

.0  $ cd $HOME
.1  $ git clone git://github.com/xbmc/xbmc.git kodi

Note: You can clone any specific branch.

.1  $ git clone -b <branch> git://github.com/xbmc/xbmc.git kodi

-----------------------------------------------------------------------------
3. Installing the required libraries and headers
-----------------------------------------------------------------------------

You will then need the required libraries. The following is the list of packages
that are used to build Kodi packages on Debian/Ubuntu (with all supported
external libraries enabled).

OpenSuse
Since commit b2fa60d7c6fba907ecadc0cf36a3531985c4c367
On OpenSuse there is a need for an extra dependency: libunistring-devel

Full build dependency list is:
make cmake autoconf automake gcc gcc-c++ libtool gettext-devel patch boost-devel glew-devel
libmysqlclient-devel libass-devel libmpeg2-devel libmad-devel libjpeg-devel libsamplerate-devel libogg-devel
libvorbis-devel libmodplug-devel libcurl-devel flac-devel libbz2-devel libtiff-devel lzo-devel libyajl-devel
fribidi-devel sqlite3-devel libpng12-devel pcre-devel libcdio-devel libjasper-devel
libmicrohttpd-devel libsmbclient-devel python-devel gperf nasm tinyxml-devel libtag-devel libbluray-devel
libnfs-devel shairplay-devel swig libvdpau-devel libavahi-devel libcec-devel libdvdread-devel libva-devel libplist-devel
libxst-devel alsa-devel libpulse-devel libXrandr-devel libXrender-devel randrproto-devel renderproto-devel libssh-devel
libudev-devel libpcap-devel libgudev-1_0-devel libcap-ng-devel libcap-devel ccache doxygen capi4linux-devel liblcms2-devel
libunistring-devel


Build-Depends: autoconf, automake, autopoint, autotools-dev, cmake, curl,
  default-jre, gawk, gperf, libao-dev, libasound2-dev,
  libass-dev (>= 0.9.8), libavahi-client-dev, libavahi-common-dev, libbluetooth-dev,
  libbluray-dev (>= 0.9.3), libbz2-dev, libcap-dev,
  libcdio-dev, libcec-dev, libcurl4-openssl-dev | libcurl4-gnutls-dev | libcurl-dev,
  libcwiid-dev, libdbus-1-dev, libegl1-mesa-dev, libfontconfig-dev, libfreetype6-dev,
  libfribidi-dev, libgif-dev (>= 4.1.6), libgl1-mesa-dev | libgl-dev, libglu1-mesa-dev | libglu-dev,
  libiso9660-dev, libjpeg-dev, libltdl-dev, liblzo2-dev, libmicrohttpd-dev,
  libmpcdec-dev, libmysqlclient-dev, libnfs-dev,
  libpcre3-dev, libplist-dev, libpng12-dev | libpng-dev, libpulse-dev,
  libshairplay-dev, libsmbclient-dev, libsqlite3-dev, libssh-dev, libssl-dev, libswscale-dev,
  libtag1-dev (>= 1.8), libtinyxml-dev (>= 2.6.2), libtool, libudev-dev,
  libusb-dev, libva-dev, libvdpau-dev, libxml2-dev,
  libxmu-dev, libxrandr-dev, libxslt1-dev, libxt-dev, libyajl-dev (>=2.0), lsb-release,
  nasm [!amd64], python-dev, python-imaging, python-support, swig, uuid-dev, yasm,
  zip, zlib1g-dev

[NOTICE] crossguid / libcrossguid-dev all Linux distributions.
Kodi now requires crossguid which is not available in Ubuntu repositories at this time.
We supply a Makefile in tools/depends/target/crossguid
to make it easy to install into /usr/local.
    $ make -C tools/depends/target/crossguid PREFIX=/usr/local
    
[NOTICE] yail got changed for rapidjson which is not available in OpenSuse repositories.
To build rapidjson for OpenSuse you must get the source from github and configure, build and install them yourself.
Next lines explain how to do that.
First download the sources from github to your local disk.

    $ git clone https://github.com/miloyip/rapidjson
    $ cd rapidjson

To get the files of thirdparty submodules (google test).

    $ git submodule update --init

Create directory called build in rapidjson source directory.
    $ mkdir build

Change to build directory
    $ cd build

Run the following command to configure your build.
It will default to /usr/local
    $ cmake ..

Run the following command to build the sources for installation.
    $ cmake --build .

Tip: By adding -j<number> to the make command, you describe how many
     concurrent jobs will be used, it will speed up the build process.
     So for octacore the command is:

    $ cmake --build . -- -j8

Run the following command to install the builded sources.
    $ sudo make install

Tip: By adding -j<number> to the make command, you describe how many
     concurrent jobs will be used, it will speed up the build process.
     So for octacore the command is:

    $ sudo make install -j8

If everything went well it is installed to "/usr/local".

Tip: To override the location that the module is installed, use DESTDIR=<path>.
For example.

    $ make install DESTDIR=/usr


Note: For developers and anyone else who compiles frequently it is recommended to use ccache.
    $ sudo apt-get install ccache

    $ sudo apt-get build-dep kodi


-----------------------------------------------------------------------------
4. How to compile
-----------------------------------------------------------------------------
Cmake build instructions V18.0 and higher OpenSuse

Create and change to build directory 
    $ mkdir kodi-build && cd kodi-build
    $ cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
    $ cmake --build . -- VERBOSE=1
    
Tip: By adding -j<number> to the make command, you describe how many
     concurrent jobs will be used, it will speed up the build process.
     So for quadcore the command is:
    
    $ cmake --build . -- VERBOSE=1 -j4

If the build process completes succesfully you would want to test if it is working.
Still in the build directory type the following:

    $ ./kodi.bin

If everything was okay during your test you can now install the binaries to their place
in this example "/usr/local".

    $ sudo make install

This will install Kodi in the prefix provided in 4.1 as well as a launcher script.

Tip: By adding -j<number> to the make command, you describe how many
     concurrent jobs will be used. So for dualcore the command is:

    $ sudo make install -j2

Tip: To override the location that Kodi is installed, use PREFIX=<path>.
For example.

    $ make install DESTDIR=$HOME/kodi

-----------------------------------------------------------------------------
4.4. Binary addons - compile
-----------------------------------------------------------------------------

From v14 with commit 4090a5f a new API for binary addons is available.
You can compile all addons or only specific addons by specifying e.g. ADDONS="audioencoder.foo pvr.bar audiodecoder.baz"

.0  All addons
    $ make -C tools/depends/target/binary-addons PREFIX=/<system prefix added on step 4.1>

.1  Specific addons
    $ make -C tools/depends/target/binary-addons PREFIX=/<system prefix added on step 4.1> ADDONS="audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu"

ADSP addons:
    adsp.basic, adsp.biquad.filters, adsp.freesurround

Audio decoders:
    audiodecoder.modplug, audiodecoder.nosefart, audiodecoder.sidplay, audiodecoder.snesapu,
    audiodecoder.stsound, audiodecoder.timidity, audiodecoder.vgmstream

Audio encoders:
    audioencoder.flac, audioencoder.lame, audioencoder.vorbis, audioencoder.wav

Inputstream addons:
    inputstream.mpd

Peripheral addons:
    peripheral.joystick

PVR addons:
    pvr.argustv, pvr.demo, pvr.dvblink, pvr.dvbviewer, pvr.filmon, pvr.hdhomerun, pvr.hts, pvr.iptvsimple,
    pvr.mediaportal.tvserver,pvr.mythtv, pvr.nextpvr, pvr.njoy, pvr.pctv, pvr.stalker, pvr.vbox, pvr.vdr.vnsi,
    pvr.vuplus, pvr.wmc

Screensavers:
    screensaver.asteroids, screensaver.biogenesis, screensaver.greynetic, screensaver.matrixtrails,
    screensaver.pingpong, screensaver.pyro, screensavers.rsxs, screensaver.stars

Visualizations
    visualization.fishbmc, visualization.goom, visualization.projectm, visualization.shadertoy
    visualization.spectrum, visualization.vsxu, visualization.waveform

-----------------------------------------------------------------------------
4.5. Test suite
-----------------------------------------------------------------------------

Kodi has a test suite which uses the Google C++ Testing Framework.
This framework is provided directly in Kodi's source tree.
It has very little requirements, in order to build and run.
See the README file for the framework at 'lib/gtest/README' for specific requirements.

To compile and run Kodi's test suite:
The configure option '--enable-gtest' is enabled by default during the configure stage.
Once configured, to build the testsuite, type the following:

    $ make check

To compile the test suite without running it, type the following.

    $ make testsuite

The test suite program can be run manually as well.
The name of the test suite program is 'kodi-test' and will build in the Kodi source tree.
To bring up the 'help' notes for the program, type the following:

    $ ./kodi-test --gtest_help

The most useful options are,

    --gtest_list_tests
      List the names of all tests instead of running them.
	  The name of TEST(Foo, Bar) is "Foo.Bar".

    --gtest_filter=POSTIVE_PATTERNS[-NEGATIVE_PATTERNS]
      Run only the tests whose name matches one of the positive patterns but
      none of the negative patterns. '?' matches any single character; '*'
      matches any substring; ':' separates two patterns.

Note: If the '--enable-gtest' option is not set during the configure stage,
the make targets 'check,' 'testsuite,' and 'testframework' will simply show a message saying
the framework has not been configured, and then silently succeed (i.e. it will not return an error).

-----------------------------------------------------------------------------
5. How to run
-----------------------------------------------------------------------------

How to run Kodi depends on the type of installation you have done.
It is possible to run Kodi without the requirement to install Kodi anywhere else.
In this case, type the following from the top source directory.

    $ ./kodi.bin

Or run in 'portable' mode

    $ ./kodi.bin -p

If you chose to install Kodi using '/usr' or '/usr/local' as the PREFIX,
you can just issue 'kodi' in a terminal session.

If you have overridden PREFIX to install Kodi into some non-standard location,
you will have to run Kodi by directly running 'kodi.bin'.

For example:

    $ $HOME/kodi/usr/lib/kodi/kodi.bin

You should still run the wrapper via
    $ $PREFIX/bin/kodi

If you wish to use VDPAU decoding you will now have to change the Render Method
in Settings->Videos->Player from "Auto Detect" to "VDPAU".

-----------------------------------------------------------------------------
6. Uninstalling
-----------------------------------------------------------------------------

Prepend "sudo" to commands, if your user doesn't have write permission to the install directory.

Note: If you have rerun configure with a different prefix,
you will either need to rerun configure with the correct prefix for this step to work correctly.

    $ make uninstall
.0  $ sudo make uninstall

If you would like to also remove any settings and 3rd party addons (skins, scripts, etc)
you should also run:

.1  $ rm -rf ~/.kodi

EOF
