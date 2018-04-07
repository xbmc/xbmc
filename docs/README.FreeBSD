TOC
1. Introduction
2. Getting the source code
3. Installing the required libraries and headers
4. How to compile
   4.4 Binary addons
   4.5 Test suite
5. How to run
6. Uninstalling
7. Example basic setup of a KODI machine running on FreeBSD
   7.1 Setting up X Server
   7.2 Install SLiM login manager
   7.3 Solving the troubles with HDMI Audio
-----------------------------------------------------------------------------
1. Introduction
-----------------------------------------------------------------------------

A graphics-adapter with OpenGL acceleration is highly recommended.
24/32 bitdepth is required along with OpenGL.

Note to new FreeBSD users:
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

$ pkg install taglib gstreamer1-vaapi hal libcapn \
enca gawk gperf cmake zip nasm swig30 libssh openjdk8 libtool gettext-tools \
gmake pkgconf rapidjson mesa-libs doxygen glproto dri2proto dri3proto libass \
flac libcdio curl dbus fontconfig freetype2 fribidi \
libgcrypt gmp libgpg-error gnutls libidn libinotify lzo2 \
libogg sqlite3 tiff tinyxml e2fsprogs-libuuid git libvorbis libxslt libplist \
shairplay avahi-app libcec libbluray samba46 libnfs librtmp libva libvdpau \
jpeg-turbo glew xrandr libedit inputproto giflib m4 encodings \
font-util mysql57-client xf86vidmodeproto python2 p8-platform libbdplus \
libaacs libudev-devd sndio ccache xorg-server binutils libmicrohttpd \
xorg-server xf86-input-mouse xf86-input-keyboard lirc libfmt autoconf automake

-----------------------------------------------------------------------------
4. How to compile
-----------------------------------------------------------------------------
Cmake build instructions V18.0 Leia and higher

Create and change to build directory 
    $ mkdir kodi-build && cd kodi-build

Run CMake
- for X11
    $ cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

Compiling on FreeBSD -DENABLE_ALSA=OFF recommended. Or build would fail due to Linux
specific portion of code in xbmc/cores/AudioEngine/Sinks/AESinkALSA.cpp. After build
KODI would use SNDIO on FreeBSD.

Build
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

    $su
    $gmake install
    $exit

NB! 'gmake' stands for 'GNU Make'. BSD's own make does not work here.

This will install Kodi in the prefix provided in 4.1 as well as a launcher script.

Tip: By adding -j<number> to the make command, you describe how many
     concurrent jobs will be used. So for dualcore the command is:

    $ su
    $ gmake install -j2

Tip: To override the location that Kodi is installed, use PREFIX=<path>.
For example.

    $ gmake install DESTDIR=$HOME/kodi

-----------------------------------------------------------------------------
4.4. Binary addons - compile
-----------------------------------------------------------------------------

From v14 with commit 4090a5f a new API for binary addons is available.
You can compile all addons or only specific addons by specifying e.g. ADDONS="audioencoder.foo pvr.bar audiodecoder.baz"

.0  All addons
    $ gmake -C tools/depends/target/binary-addons PREFIX=/<system prefix added on step 4.1>

.1  Specific addons
    $ gmake -C tools/depends/target/binary-addons PREFIX=/<system prefix added on step 4.1> ADDONS="audioencoder.flac pvr.vdr.vnsi audiodecoder.snesapu"

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

To compile and run Kodi's test suite, type the following:

    $ gmake check

To compile the test suite without running it, type the following.

    $ gmake kodi-test

The test suite program can be run manually as well.
The name of the test suite program is 'kodi-test' and will build in the Kodi source tree.
To bring up the 'help' notes for the program, type the following:

    $ ./kodi-test --gtest_help

The most useful options are,

    --gtest_list_tests
      List the names of all tests instead of running them.
	  The name of TEST(Foo, Bar) is "Foo.Bar".

    --gtest_filter=POSITIVE_PATTERNS[-NEGATIVE_PATTERNS]
      Run only the tests whose name matches one of the positive patterns but
      none of the negative patterns. '?' matches any single character; '*'
      matches any substring; ':' separates two patterns.

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

Prepend "sudo" or "doas" to commands (if installed, if not use just 'su'), if your
user doesn't have write permission to the install directory.

Note: If you have rerun configure with a different prefix,
you will either need to rerun configure with the correct prefix for this step to work correctly.

    $ gmake uninstall
.0  $ sudo make uninstall

If you would like to also remove any settings and 3rd party addons (skins, scripts, etc)
you should also run:

.1  $ rm -rf ~/.kodi

-----------------------------------------------------------------------------
7. Example basic setup of a KODI machine running on FreeBSD
-----------------------------------------------------------------------------
If having installed FreeBSD 'RELEASE' and not 'STABLE' or 'CURRENT, do system 
update first: (have to be 'root'). Also, if you have Radeon graphics card and
plan using HDMI Audio, better install FreeBSD source files during installation.
    
    $ freebsd-update fetch
    $ freebsd-update install

Update binary package repository:
    
    $pkg update

Install convenient text editor ('nano' in this case).
    
    $pkg install nano

Open /boot/loader.conf

    $ nano /boot/loader.conf

Add into file following rows:

    kern.ipc.shmseg=1024
    kern.ipc.shmmni=1024
    kern.maxproc=10000
    hint.acpi_throttle.0.disabled=1
    machdep.disable_mtrrs=1
    kern.cam.scsi_delay=500

Open /etc/rc.conf
   
    $ nano /etc/rc.conf

Add into the beginning of a /etc/rc.conf
   
    kld_list="radeonkms acpi_asus_wmi acpi_asus acpi_video amdtemp tmpfs libiconv msdosfs_iconv snd_driver"

Brief explanation:
   
    radeonkms - Radeon KMS driver.
    acpi_asus_wmi - both ASUS board specific drivers.
    acpi_asus
    amdtemp - temperature sensor module for AMD CPU. For Intel CPU that would be 'coretemp'

-----------------------------------------------------------------------------
7.1 Setting up X server
-----------------------------------------------------------------------------

You may had some X packages during install of KODI's dependency packages but
'xorg' is a metapackage, installing the missing bits as well.

    $ pkg install xorg hal

Add keyboard and mouse support

    $ pkg install xf86-input-mouse xf86-input-keyboard

According to your actual hardware, pick one:

    $ pkg install xf86-video-ati
    $ pkg install xf86-video-nv
    $ pkg install xf86-video intel
    
NB! Nvidia's driver could also be downloaded from https://www.geforce.com/drivers.
Both FreeBSD 32-bit and 64-bit official driver exist there.

Open /etc/rc.conf and add:

    dbus_enable="YES"
    hald_enable="YES" 

Add user running KODI into group 'video'. Necessity for having an working OpenGL
acceleration in KODI.

    $ pw groupmod video -m kodiuser

-----------------------------------------------------------------------------
7.2 Install SLiM login manager
-----------------------------------------------------------------------------

    $ pkg install slim
    $ nano /etc/rc.conf

Add:

    slim_enable="YES"

NB! Has to be below 'dbus_enable="YES"' and 'hald_enable="YES"' entries in the file.

Open SLiM configuration file.

    $ nano /usr/local/etc/slim.conf

Edit until it looks like:

    default_path        /sbin:/bin:/usr/sbin:/usr/bin:/usr/games:/usr/local/sbin:/usr/local/bin
    default_xserver     /usr/local/bin/X 
    xserver_arguments   -nolisten tcp vt09 
    halt_cmd            /sbin/shutdown -p now 
    reboot_cmd          /sbin/shutdown -r now 
    console_cmd         /usr/local/bin/xterm -C -fg white -bg black +sb -T "Console login" -e /bin/sh -c "/bin/cat /etc/motd; exec /usr/bin/login" 
    suspend_cmd        /usr/sbin/acpiconf -s 3 
    xauth_path         /usr/local/bin/xauth 
    authfile           /var/run/slim.auth 
    login_cmd           exec /bin/sh - ~/.xinitrc %session 
    sessiondir              /usr/local/share/xsessions 
    screenshot_cmd      import -window root /slim.png 
    default_user        kodi #or whatever user was chosen 
    auto_login      yes 
    current_theme       default 
    lockfile            /var/run/slim.pid 
    logfile             /var/log/slim.log 


Exit root user and log into user you plan to run KODI with:
create .xinitrc file into it's home directory

    $touch .xinitrc

Add into it:

    exec /usr/local/bin/kodi

Log out of normal user, back to 'root' and add package 'doas' (something like sudo)

    $ pkg install doas

Edit it's config

    $ nano /usr/local/etc/doas.conf

Add into file following rows:

    permit persist :wheel #assuming group 'wheel' has user 
    permit nopass kodi as root cmd reboot 
    permit nopass kodi as root cmd shutdown 

It will allow KODI user basic ability to shut down/reboot machine without needing
privilege escalation. To complete it.

    $ pkg install upower


Create file

    $ nano /usr/local/etc/polkit-1/localauthority/50-local.d/custom-actions.pkla


Add into it:

    [Actions for KODI users] 
    Identity=kodi:kodi 
    Action=org.freedesktop.upower.* 
    ResultAny=yes 
    ResultInactive=yes 
    ResultActive=yes 

On restart (assuming you meanwhile also installed KODI itself), machine should:

-start SLiM
-autologin with user kodi (or whatever was chosen)
-autostart KODI

For the rest: FreeBSD has excellent documentation:
https://www.freebsd.org/doc/handbook/

And active forum you can ask advice from:
https://forums.freebsd.org/

-----------------------------------------------------------------------------
7.3 Solving the troubles with HDMI Audio.
-----------------------------------------------------------------------------

Nvidia graphics.

    $ pkg install nvidia-driver nvidia-xconfig

Add into /boot/loader.conf rows:
    snd_hda_load="YES" 
    nvidia_load="YES" 
    hw.snd.default_unit="0" #ID could be something else besides 0. If it is,
                            #correct it accordingly

You can find it by:

    $ cat /dev/sndstat

After saving the /boot/loader.conf you can force the change in live using

    $ sysctl hw.snd.default_unit=0 (or whatever number your particular machine's
                                   'cat's would make you choose).
    $ kldload nvidia
    $ kldload snd_hda

or just reboot..


Radeon graphics.
First check that HDMI sound output and active sound device in system would be
both the same.

    $ cat /dev/sndstat 
    $ sysctl -a | grep hw.snd.default_unit 

For example:
If former shows:

    pcm0: <ATI R6xx (HDMI)> (play) 
    pcm1: <Realtek ALC269 (Right Analog)> (play/rec) default 
    pcm2: <Realtek ALC269 (Internal Analog)> (play/rec) 
    No devices installed from userspace. 

and latter:

    hw.snd.default_unit: 1


then you should give command:

    $ sysctl hw.snd.default_unit=0


Assuming you did install system sources during installation (you better have!)
Open and find:

    int radeon_audio = 0;

From:

    $ nano /usr/src/sys/dev/drm2/radeon/radeon_drv.c

Change it to:

    int radeon_audio = 1;

Save the file, close the editor. Then

    $ cd /usr/src 
    $ make buildkernel 

Wait until Clang/LLVM finish.

    $make installkernel 

Reboot and now your Radeon's HDMI Audio should function properly.


Intel iGPU. Problem may be  in wrong output ID. Find it, correct
it in a similar manner like demonstrated in examples above.

EOF
