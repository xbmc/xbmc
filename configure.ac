#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([kodi], [15.9.701], [http://trac.kodi.tv])
AC_CONFIG_HEADERS([xbmc/config.h])
AH_TOP([#pragma once])
m4_include([m4/ax_prog_cc_for_build.m4])
m4_include([m4/ax_prog_cxx_for_build.m4])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
m4_include([m4/ax_python_devel.m4])
m4_include([m4/xbmc_arch.m4])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign])
AC_CANONICAL_HOST

tolower(){
  echo "$@" | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz
}

# workaround for autotools that don't set this
abs_top_srcdir=${abs_top_srcdir=$(cd $srcdir; pwd)}

# check for enabling additional players
AC_DEFUN([XB_ADD_PLAYER],
[
  AC_MSG_CHECKING([for $2])
  case $add_players in
    *$2*)
      AC_SUBST([USE_$1], 1)
      AC_DEFINE([HAS_$1], 1, [using $2])
      AC_MSG_RESULT([enabling $2])
      ;;
    *)
      AC_MSG_RESULT([$2 is not enabled])
  esac
])

# check for enabling additional codecs
AC_DEFUN([XB_ADD_CODEC],
[
  AC_MSG_CHECKING([for $2])
  case $3 in
    *$2*)
      use_codec_$2="yes"
      AC_SUBST([USE_$1], 1)
      AC_DEFINE([HAS_$1], 1, [using $2])
      AC_MSG_RESULT([enabling $2])
      ;;
    *)
      AC_MSG_RESULT([$2 is not enabled])
  esac
])

# check for library basenames
AC_DEFUN([XB_FIND_SONAME],
[
  if echo "$host" | grep -q freebsd ; then
    AC_MSG_CHECKING([for lib$2 soname])
    $1_SONAME=[`ldconfig -r | sed -n "s;.* \(/.*lib$2\.so.*\)$;\1;p" | head -n 1`]
    if test x$$1_SONAME != x ; then
      $1_SONAME=[`basename $$1_SONAME`]
    fi
  elif [[ "$host_vendor" != "apple" ]]; then
    AC_MSG_CHECKING([for lib$2 soname])
    $1_FILENAME=$($CC -nostdlib -o /dev/null $CFLAGS $LDFLAGS $4 -l$2 -Wl,-M 2>/dev/null | grep "^LOAD.*$2" | awk '{V=2; print $V}')
    if [[ -z $$1_FILENAME ]]; then
      #try gold linker syntax
      $1_FILENAME=$($CC -nostdlib -o /dev/null $CFLAGS $LDFLAGS $4 -l$2 -Wl,-t 3>&1 1>&2 2>&3 | grep "lib$2")
    fi
    if [[ ! -z $$1_FILENAME ]]; then
      $1_SONAME=$($OBJDUMP -p $$1_FILENAME | grep "SONAME.*$2" | awk '{V=2; print $V}')
    fi
  else
    AC_MSG_CHECKING([for lib$2 dylib])
    gcc_lib_path=[`$CC -print-search-dirs 2>/dev/null | fgrep libraries: | sed 's/[^=]*=\(.*\)/\1/' | sed 's/:/ /g'`]
    env_lib_path=[`echo $LDFLAGS | sed 's/-L[ ]*//g'`]
    if test "$cross_compiling" = yes; then
      host_lib_path=""
    else
      host_lib_path="/usr/lib /usr/local/lib"
    fi
    for path in $gcc_lib_path $env_lib_path $host_lib_path; do
      lib=[`ls -- $path/lib$2.dylib 2>/dev/null`]
      if test x$lib != x; then
        # we want the path/name that is embedded in the dylib 
        $1_FILENAME=[`otool -L $lib | grep -v lib$2.dylib | grep lib$2 | awk '{V=1; print $V}'`]
        $1_SONAME=[`basename $$1_FILENAME`]
      fi
    done
  fi
  if [[ -z "$$1_SONAME" ]]; then
    AC_MSG_RESULT([no])
    if test -z "$3" || test "x${$3}" = "xyes"; then
      AC_MSG_ERROR([Unable to determine soname of lib$2 library])
    else
      AC_MSG_WARN([Unable to determine soname of lib$2 library])
      $3=no
      AC_MSG_WARN([lib$2 support disabled])
    fi
  else
    AC_MSG_RESULT([$$1_SONAME])
    AC_SUBST($1_SONAME)
  fi
])

# Function to push and pop libs and includes for a command
AC_DEFUN([XB_PUSH_FLAGS], [
  SAVE_LIBS="$LIBS"
  SAVE_INCLUDES="$INCLUDES"
  LIBS="[$2]"
  INCLUDES="[$1]"
  [$3]
  LIBS="$SAVE_LIBS"
  INCLUDES="$SAVE_INCLUDES"
])

# version can be overridden by setting the following as ENV vars when running configure
APP_NAME=${APP_NAME-$(${AWK} '/APP_NAME/ {print $2}' version.txt)}
APP_NAME_LC=$(echo $APP_NAME | ${AWK} '{print tolower($0)}')
APP_NAME_UC=$(echo $APP_NAME | ${AWK} '{print toupper($0)}')
APP_VERSION_MAJOR=${APP_VERSION_MAJOR-$(${AWK} '/VERSION_MAJOR/ {print $2}' version.txt)}
APP_VERSION_MINOR=${APP_VERSION_MINOR-$(${AWK} '/VERSION_MINOR/ {print $2}' version.txt)}
APP_VERSION_TAG=${APP_VERSION_TAG-$(${AWK} '/VERSION_TAG/ {print $2}' version.txt)}
APP_VERSION_TAG_LC=$(echo $APP_VERSION_TAG | ${AWK} '{print tolower($0)}')
APP_VERSION_CODE=${APP_VERSION_CODE-$(${AWK} '/VERSION_CODE/ {print $2}' version.txt)}
APP_ADDON_API=${APP_ADDON_API-$(${AWK} '/ADDON_API/ {print $2}' version.txt)}
if test "$APP_NAME" != "" && test "$APP_VERSION_MAJOR" != "" && test "$APP_VERSION_MINOR" != "" \
&& test "$APP_VERSION_CODE" != "" && test "$APP_ADDON_API" != ""; then
  APP_VERSION=${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}
  if test "$APP_VERSION_TAG" != ""; then
    APP_VERSION=${APP_VERSION}-${APP_VERSION_TAG}
  fi

  AC_SUBST(APP_NAME)
  AC_SUBST(APP_NAME_LC)
  AC_SUBST(APP_NAME_UC)
  AC_SUBST(APP_VERSION)
  AC_SUBST(APP_VERSION_MAJOR)
  AC_SUBST(APP_VERSION_MINOR)
  AC_SUBST(APP_VERSION_TAG)
  AC_SUBST(APP_VERSION_TAG_LC)
  AC_SUBST(APP_VERSION_CODE)
  AC_SUBST(APP_ADDON_API)

  dashes="------------------------"
  final_message="\n  ${APP_NAME} Configuration:"
  final_message="\n$dashes$final_message\n$dashes"
  final_message="$final_message\n  ${APP_NAME} Version:\t${APP_VERSION}"
else
  AC_MSG_ERROR(could not detect application Version, make sure version.txt is complete)
fi

# check for GIT_REV
AC_CHECK_PROG(HAVE_GIT,git,"yes","no",)
if test "$GIT_REV" = ""; then
  if test -f VERSION ; then
    GIT_REV=$(awk 'END{print substr($1,1,16)}' VERSION)
  elif test "$HAVE_GIT" = "yes" -a -d ${abs_top_srcdir}/.git; then
    GIT_REV=$(git --no-pager log --abbrev=7 -n 1 --pretty=format:"%h %ci" HEAD | awk '{gsub("-", "");print $2"-"$1}')
  else
    GIT_REV="Unknown"
  fi
fi
final_message="$final_message\n  git Rev.:\t${GIT_REV}"

# General message strings
configure_debug="ERROR: this is a configure debug statement"
missing_library="Could not find a required library. Please see the README for your platform."
missing_headers="Could not find some required headers. Please see the README for your platform."
missing_program="Could not find a required program. Please see the README for your platform."
waveform_enabled="== WAVEFORM enabled =="
waveform_disabled="== WAVEFORM disabled =="
spectrum_enabled="== SPECTRUM enabled =="
spectrum_disabled="== SPECTRUM disabled =="
goom_enabled="== GOOM enabled. =="
goom_disabled="== GOOM disabled. =="
alsa_disabled="== ALSA support disabled. =="
dbus_disabled="== DBUS support disabled. =="
rsxs_enabled="== RSXS enabled. =="
rsxs_disabled="== RSXS disabled. =="
fishbmc_enabled="== FishBMC enabled. =="
fishbmc_disabled="== FishBMC disabled. =="
projectm_enabled="== ProjectM enabled. =="
projectm_disabled="== ProjectM disabled. =="
wayland_enabled="== Wayland enabled. =="
wayland_disabled="== Wayland disabled. =="
x11_enabled="== X11 enabled. =="
x11_disabled="== X11 disabled. =="
pulse_not_found="== Could not find libpulse. PulseAudio support disabled. =="
pulse_disabled="== PulseAudio support disabled. =="
dvdcss_enabled="== DVDCSS support enabled. =="
dvdcss_disabled="== DVDCSS support disabled. =="
avahi_not_found="== Could not find libavahi-common or libavahi-client. Avahi support disabled. =="
avahi_disabled="== Avahi support disabled. =="
mdnsembedded_not_found="== Could not find mDNSEmbedded. Might prevent zeroconf support. =="
mdnsembedded_disabled="== mDNSEmbedded support disabled. =="
vdpau_not_found="== Could not find libvdpau. VDPAU support disabled. =="
vdpau_disabled="== VDPAU support manually disabled. =="
vaapi_not_found="== Could not find libva. VAAPI support disabled. =="
vaapi_disabled="== VAAPI support manually disabled. =="
vtbdecoder_enabled="== VTBDecoder support enabled. =="
vtbdecoder_disabled="== VTBDecoder support manually disabled. =="
openmax_disabled="== OpenMax support manually disabled. =="
openmax_not_found="== Could not find OpenMax headers. OpenMax support disabled. =="
ssh_not_found="== Could not find libssh. =="
ssh_disabled="== SSH SFTP disabled. =="
librtmp_not_found="== Could not find libRTMP. RTMP support disabled. =="
librtmp_disabled="== RTMP support disabled. =="
libnfs_not_found="== Could not find libnfs. NFS client support disabled. =="
libnfs_disabled="== NFS support disabled. =="
libshairplay_not_found="== Could not find libshairplay. =="
samba_disabled="== SAMBA support disabled. =="
libplist_not_found="== Could not find libplist. AirPlay support disabled. =="
libplist_disabled="== AirPlay support disabled. =="
alsa_not_found="== Could not find ALSA. ALSA support disabled. =="
dbus_not_found="== Could not find DBUS. DBUS support disabled. =="
sdl_joystick_not_found="== Could not find SDL2. Joystick support disabled. =="
libcap_disabled="== Capabilities detection support disabled. =="
libcap_not_found="== Could not find libcap. Capabilities detection support disabled. =="
gtest_enabled="== Google Test Framework will be configured. =="
gtest_disabled="== Google Test Framework will not be configured. =="
breakpad_enabled="== Google Breakpad will be configured. =="
breakpad_disabled="== Google Breakpad Framework will not be configured. =="

libudev_not_found="== Could not find libudev. Will use polling to check for device changes. =="
libudev_disabled="== udev support disabled. Will use polling to check for device changes. =="
libusb_not_found="== Could not find libusb. Plug and play USB device support will not be available. =="
libusb_disabled="== libusb disabled. Plug and play USB device support will not be available. =="
libusb_disabled_udev_found="== libusb disabled. =="
libcec_enabled="== libcec enabled. =="
libcec_disabled="== libcec disabled. CEC adapter support will not be available. =="

AC_ARG_WITH([ffmpeg],
  [AS_HELP_STRING([--with-ffmpeg],
  [ffmpeg options: auto (search pkg-config or auto build), force (always build ffmpeg), shared (link dynamically), path_to_ffmpeg [default=force]])],
  [ffmpeg_dir=$with_ffmpeg],
  [with_ffmpeg=force])

AC_ARG_ENABLE([shared-lib],
  [AS_HELP_STRING([--enable-shared-lib],
  [build lib${APP_NAME_LC}. helpful for tests (default is no)])],
  [build_shared_lib=$enableval],
  [build_shared_lib=no])

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
  [enable debugging information (default is yes)])],
  [use_debug=$enableval],
  [use_debug=yes])

AC_ARG_WITH([platform],
  [AS_HELP_STRING([--with-platform],
  [use a pre-configured config for common arm boards])],
  [use_platform=$withval],
  [use_platform=none])

AC_ARG_ENABLE([optimizations],
  [AS_HELP_STRING([--enable-optimizations],
  [enable optimization (default is yes)])],
  [use_optimizations=$enableval],
  [use_optimizations=yes])

AC_ARG_ENABLE([gl],
  [AS_HELP_STRING([--enable-gl],
  [enable OpenGL rendering (default is yes)])],
  [use_gl=$enableval],
  [use_gl=yes])

AC_ARG_ENABLE([gles],
  [AS_HELP_STRING([--enable-gles],
  [enable OpenGLES rendering (default is no)])],
  [use_gles=$enableval],
  [use_gles=no])

AC_ARG_ENABLE([vdpau],
  [AS_HELP_STRING([--enable-vdpau],
  [enable VDPAU decoding (default is auto)])],
  [use_vdpau=$enableval],
  [use_vdpau=auto])

AC_ARG_ENABLE([vaapi],
  [AS_HELP_STRING([--enable-vaapi],
  [enable VAAPI decoding (default is auto)])],
  [use_vaapi=$enableval],
  [use_vaapi=auto])

AC_ARG_ENABLE([vtbdecoder],
  [AS_HELP_STRING([--enable-vtbdecoder],
  [enable VTBDecoder decoding (default is auto)])],
  [use_vtbdecoder=$enableval],
  [use_vtbdecoder=auto])

AC_ARG_ENABLE([openmax],
  [AS_HELP_STRING([--enable-openmax],
  [enable OpenMax decoding (default is auto, requires OpenGLES)])],
  [use_openmax=$enableval],
  [use_openmax=auto])

AC_ARG_ENABLE([tegra],
  [AS_HELP_STRING([--enable-tegra],
  [enable Tegra2 arm (default is no)])],
  [use_tegra=$enableval],
  [use_tegra=no])

AC_ARG_ENABLE([profiling],
  [AS_HELP_STRING([--enable-profiling],
  [enable gprof profiling (default is no)])],
  [use_profiling=$enableval],
  [use_profiling=no])

AC_ARG_ENABLE([joystick],
  [AS_HELP_STRING([--enable-joystick],
  [enable SDL joystick support (default is auto)])],
  [use_joystick=$enableval],
  [use_joystick=auto])

AC_ARG_ENABLE([waveform],
  [AS_HELP_STRING([--enable-waveform],
  [enable Waveform visualisation (default is yes)])],
  [use_waveform=$enableval],
  [use_waveform=yes])

AC_ARG_ENABLE([spectrum],
  [AS_HELP_STRING([--enable-spectrum],
  [enable Spectrum visualisation (default is yes)])],
  [use_spectrum=$enableval],
  [use_spectrum=yes])

AC_ARG_ENABLE([goom],
  [AS_HELP_STRING([--enable-goom],
  [enable GOOM visualisation (default is no)])],
  [use_goom=$enableval],
  [use_goom=no])

AC_ARG_ENABLE([rsxs],
  [AS_HELP_STRING([--enable-rsxs],
  [enable really slick X screensavers (default is yes)])],
  [use_rsxs=$enableval],
  [use_rsxs=yes])

AC_ARG_ENABLE([fishbmc],
  [AS_HELP_STRING([--enable-fishbmc],
  [enable FishBMC visualisation (default is yes)])],
  [use_fishbmc=$enableval],
  [use_fishbmc=yes])

AC_ARG_ENABLE([projectm],
  [AS_HELP_STRING([--enable-projectm],
  [enable ProjectM visualisation (default is yes)])],
  [use_projectm=$enableval],
  [use_projectm=yes])

AC_ARG_ENABLE([wayland],
  [AS_HELP_STRING([--enable-wayland],
  [enable wayland (default is no) 'Linux Only'])],
  [use_wayland=$enableval],
  [use_wayland=no])

AC_ARG_ENABLE([x11],
  [AS_HELP_STRING([--enable-x11],
  [enable x11 (default is yes) 'Linux Only'])],
  [use_x11=$enableval],
  [use_x11=yes])

AC_ARG_ENABLE([ccache],
  [AS_HELP_STRING([--enable-ccache],
  [enable building with ccache feature (default is auto)])],
  [use_ccache=$enableval],
  [use_ccache=auto])

AC_ARG_ENABLE([alsa],
  [AS_HELP_STRING([--disable-alsa],
  [disable ALSA support (only for linux/freebsd)])],
  [use_alsa=$enableval],
  [use_alsa=yes])

AC_ARG_ENABLE([dbus],
  [AS_HELP_STRING([--disable-dbus],
  [disable DBUS support])],
  [use_dbus=$enableval],
  [use_dbus=yes])

AC_ARG_ENABLE([pulse],
  [AS_HELP_STRING([--enable-pulse],
  [enable PulseAudio support (default is auto)])],
  [use_pulse=$enableval],
  [use_pulse=auto])

AC_ARG_ENABLE([ssh],
  [AS_HELP_STRING([--disable-ssh],
  [disable SSH SFTP support (default is enabled)])],
  [use_ssh=$enableval],
  [use_ssh=yes])

AC_ARG_ENABLE([rtmp],
  [AS_HELP_STRING([--enable-rtmp],
  [enable RTMP support via librtmp (default is auto)])],
  [use_librtmp=$enableval],
  [use_librtmp=auto])

AC_ARG_ENABLE([samba],
  [AS_HELP_STRING([--disable-samba],
  [disable SAMBA support (default is enabled)])],
  [use_samba=$enableval],
  [use_samba=yes])

AC_ARG_ENABLE([nfs],
  [AS_HELP_STRING([--enable-nfs],
  [enable NFS support via libnfs (default is auto)])],
  [use_libnfs=$enableval],
  [use_libnfs=auto])

AC_ARG_ENABLE([airplay],
  [AS_HELP_STRING([--enable-airplay],
  [enable AirPlay support(default is auto)])],
  [use_airplay=$enableval],
  [use_airplay=auto])

AC_ARG_ENABLE([airtunes],
  [AS_HELP_STRING([--enable-airtunes],
  [enable AirTunes support(default is auto)])],
  [use_airtunes=$enableval],
  [use_airtunes=auto])

AC_ARG_ENABLE([upnp],
  [AS_HELP_STRING([--disable-upnp],
  [disable UPnP support (default is enabled)])],
  [use_upnp=$enableval],
  [use_upnp=yes])

AC_ARG_ENABLE([dvdcss],
  [AS_HELP_STRING([--enable-dvdcss],
  [enable DVDCSS support (default is yes)])],
  [use_dvdcss=$enableval],
  [use_dvdcss=yes])

AC_ARG_ENABLE([mid],
  [AS_HELP_STRING([--enable-mid],
  [enable MID support (default is no)])],
  [use_mid=$enableval],
  [use_mid=no])

AC_ARG_ENABLE([avahi],
  [AS_HELP_STRING([--disable-avahi],
  [disable Avahi support (default is enabled if libavahi-common and libavahi-client is found)])],
  [use_avahi=$enableval],
  [use_avahi=yes])
  
  AC_ARG_ENABLE([mdnsembedded],
  [AS_HELP_STRING([--disable-mdnsembedded],
  [disable mDNSEmbedded support (default is auto)])],
  [use_mdnsembedded=$enableval],
  [use_mdnsembedded=auto])

AC_ARG_ENABLE([non-free],
  [AS_HELP_STRING([--disable-non-free],
  [disable componentents with non-compliant licenses])],
  [use_nonfree=$enableval],
  [use_nonfree=yes])

AC_ARG_ENABLE([asap-codec],
  [AS_HELP_STRING([--enable-asap-codec],
  [enable ASAP ADPCM support])],
  [use_asap=$enableval],
  [use_asap=no])

AC_ARG_ENABLE([mysql],
  [AS_HELP_STRING([--disable-mysql],
  [disable mysql])],
  [use_mysql=$enableval],
  [use_mysql=yes])

AC_ARG_ENABLE([webserver],
  [AS_HELP_STRING([--disable-webserver],
  [disable webserver])],
  [use_webserver=$enableval],
  [use_webserver=yes])

AC_ARG_ENABLE([optical-drive],
  [AS_HELP_STRING([--disable-optical-drive],
  [disable optical drive])],
  [use_optical_drive=$enableval],
  [use_optical_drive=yes])

AC_ARG_ENABLE([libbluray],
  [AS_HELP_STRING([--enable-libbluray],
  [enable libbluray support])],
  [use_libbluray=$enableval],
  [use_libbluray=auto])

AC_ARG_ENABLE([texturepacker],
  [AS_HELP_STRING([--enable-texturepacker],
  [enable texturepacker support (default is auto)])],
  [use_texturepacker=$enableval],
  [use_texturepacker=auto])

AC_ARG_ENABLE([lirc],
  [AS_HELP_STRING([--disable-lirc],
  [disable lirc support (default is enabled)])],
  [AC_MSG_RESULT("Lirc disabled")],
  [AC_DEFINE([HAVE_LIRC], [1], ["Lirc enabled"])])

AC_ARG_WITH([lirc-device],
  [AS_HELP_STRING([--with-lirc-device=file],
  [specify the default LIRC device (default is /dev/lircd)])],
  [lirc_device=$withval],
  [lirc_device=/dev/lircd])
AC_DEFINE_UNQUOTED([LIRC_DEVICE], ["$lirc_device"], [Default LIRC device])

AC_ARG_ENABLE([udev],
  [AS_HELP_STRING([--enable-udev],
  [enable udev support (default is auto)])],
  [use_libudev=$enableval],
  [use_libudev=auto])

AC_ARG_ENABLE([libusb],
  [AS_HELP_STRING([--enable-libusb],
  [enable libusb support (default is auto)])],
  [use_libusb=$enableval],
  [use_libusb=auto])

AC_ARG_ENABLE([libcec],
  [AS_HELP_STRING([--enable-libcec],
  [enable libcec support (default is auto)])],
  [use_libcec=$enableval],
  [use_libcec=auto])

AC_ARG_ENABLE([libcap],
  [AS_HELP_STRING([--enable-libcap],
  [enable libcap support (default is auto)])],
  [use_libcap=$enableval],
  [use_libcap=auto])

AC_ARG_ENABLE([player],
  [AS_HELP_STRING([--enable-player],
  [enable additional players from a list of comma separated names, (default is none, choices are omxplayer)])],
  [add_players=$enableval],
  [add_players=no])

AC_ARG_ENABLE([gtest],
  [AS_HELP_STRING([--enable-gtest],
  [configure Google Test Framework (default is yes)])],
  [configure_gtest=$enableval],
  [configure_gtest=yes])

AC_ARG_ENABLE([breakpad],
  [AS_HELP_STRING([--enable-breakpad],
  [configure Google Breakpad (default is no)])],
  [use_breakpad=$enableval],
  [use_breakpad=no])

AC_ARG_ENABLE([codec],
  [AS_HELP_STRING([--enable-codec],
  [enable additional codecs from a list of comma separated names, (default is none, choices are amcodec, libstagefright and imxvpu)])],
  [add_codecs=$enableval],
  [add_codecs=no])

AC_ARG_ENABLE([libav-compat],
  [AS_HELP_STRING([--enable-libav-compat],
  [build a wrapper around libav to provide the functions needed by ${APP_NAME}. This is
   *not* supported by ${APP_NAME} developers and is provided only for convenience. (default is no) 'Linux only'])],
  [use_libav_hacks=$enableval],
  [use_libav_hacks=no])

### End of external library options

if test "x$host_vendor" != "xapple"; then
  DEFAULT_COMPILE_FLAGS="-fPIC -DPIC -D_REENTRANT"
  DEFAULT_COMPILE_FLAGS="$DEFAULT_COMPILE_FLAGS -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64"
fi

# Checks for programs.
PASSED_CFLAGS=$CFLAGS # Hack to override autoconf default values
AC_PROG_CC
AX_PROG_CC_FOR_BUILD
CFLAGS="$PASSED_CFLAGS $DEFAULT_COMPILE_FLAGS"
PASSED_CXXFLAGS=$CXXFLAGS # Hack to override autoconf default values
AC_PROG_CXX
AX_PROG_CXX_FOR_BUILD
CXXFLAGS="$PASSED_CXXFLAGS $DEFAULT_COMPILE_FLAGS"
AX_CXX_COMPILE_STDCXX_11(,[optional])
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG
MAKE="${MAKE:-make}"
OBJDUMP="${OBJDUMP:-objdump}"
READELF="${READELF:-readelf}"
NM="${NM:-nm}"

# host detection and setup
case $host in
  i*86*-linux-android*)
     target_platform=target_android
     ARCH="i486-linux"
     use_arch="x86"
     use_cpu="i686"
     use_joystick=no
     use_gles=yes
     use_optical_drive=no
     use_x11=no
     build_shared_lib=yes    
     ;;
  i*86*-linux-gnu*|i*86*-*-linux-uclibc*)
     ARCH="i486-linux"
     if test "$use_cpu" = "no" -a "$cross_compiling" = "yes";  then
        use_arch="x86"
        use_cpu="i686"
     fi
     USE_STATIC_FFMPEG=1
     # Workaround a build issue on i386 with gcc 4.9:
     # including <algorithm> pulls in SSE intrinsics.
     # possible GCC bug? ideas welcome
     if test "$GCC_CXX" = "yes"; then
       GCC_VERSION=$($CXX -dumpversion)
       GCC_MAJOR_VER=$(echo $GCC_VERSION | awk -F"." '{print $1}')
       GCC_MINOR_VER=$(echo $GCC_VERSION | awk -F"." '{print $2}')

       if (test "$GCC_MAJOR_VER" -eq "4" && test "$GCC_MINOR_VER" -ge "9") || (test "$GCC_MAJOR_VER" -gt "4"); then
         CXXFLAGS="$CXXFLAGS -msse"
         AC_MSG_NOTICE("detected gcc version $GCC_VERSION - enabling SSE")
       fi
     fi
     ;;
  x86_64-*-linux-gnu*|x86_64-*-linux-uclibc*)
     ARCH="x86_64-linux"
     if test "$use_cpu" = "no" -a "$cross_compiling" = "yes";  then
        use_arch="x86_64"
        use_cpu="x86_64"
     fi
     USE_STATIC_FFMPEG=1
     ;;
  i386-*-freebsd*)
     ARCH="x86-freebsd"
     MAKE="gmake"
     ;;
  amd64-*-freebsd*)
     ARCH="x86_64-freebsd"
     MAKE="gmake"
     ;;
  arm-apple-darwin*)
     use_joystick=no
     use_neon=yes
     use_libcec=no
     use_vtbdecoder=yes
     use_optical_drive=no
     use_dvdcss=no
     use_gles=yes
     use_cpu=cortex-a8
     ARCH="arm-osx"
     use_arch="arm"
     PYTHON_VERSION="2.6"
     PYTHON_LDFLAGS="-L${prefix}/lib -lpython2.6"
     PYTHON_CPPFLAGS="-I${prefix}/include/python2.6"
     PYTHON_SITE_PKG="${prefix}/lib/python2.6/site-packages"
     PYTHON_NOVERSIONCHECK="no-check"
     DEPENDS_ROOT_FOR_XCODE=$(echo ${prefix%/*})
     AC_SUBST([DEPENDS_ROOT_FOR_XCODE])
     ;;
  *86*-apple-darwin*)
     use_joystick=no
     use_vtbdecoder=no
     ARCH="x86-osx"
     DEPENDS_ROOT_FOR_XCODE=$(echo ${prefix%/*})
     AC_SUBST([DEPENDS_ROOT_FOR_XCODE])
     ;;
  powerpc-apple-darwin*)
     use_joystick=no
     use_vtbdecoder=no
     ARCH="powerpc-osx"
     use_arch="ppc"
     DEPENDS_ROOT_FOR_XCODE=$(echo ${prefix%/*})
     AC_SUBST([DEPENDS_ROOT_FOR_XCODE])
     ;;
  powerpc-*-linux-gnu*|powerpc-*-linux-uclibc*)
     ARCH="powerpc-linux"
     ;;
  powerpc64-*-linux-gnu*|powerpc64-*-linux-uclibc*)
     ARCH="powerpc64-linux"
     ;;
  arm*-*-linux-gnu*|arm*-*-linux-uclibc*)
     ARCH="arm"
     use_arch="arm"
     use_joystick=no
     use_neon=yes
     use_gles=yes
     use_gl=no
     use_wayland=no
     USE_STATIC_FFMPEG=1
     ;;
  arm*-*linux-android*)
     target_platform=target_android
     use_arch="arm"
     use_cpu=cortex-a9
     ARCH="arm"
     use_joystick=no
     use_neon=yes
     use_gles=yes
     use_optical_drive=no
     use_x11=no
     use_wayland=no
     build_shared_lib=yes
     ;;
  mips*-*-linux-gnu*)
     ARCH="mips"
     use_arch="mips"
     use_joystick=no
     use_gles=no
     use_gl=yes
     use_sdl=yes
     use_wayland=no
     USE_STATIC_FFMPEG=1
     ;;
  *)
     AC_MSG_ERROR(unsupported host ($host))
esac
AC_SUBST([ARCH])

case $host in
  *-apple-darwin*)
     check_sdl_arch=[`file $NATIVE_ROOT/lib/libSDL_image.dylib | awk '{print $NF}'`]
     if test "x$check_sdl_arch" = "xi386" ; then
       DARWIN_NATIVE_ARCH=-m32
     elif test "x$check_sdl_arch" = "xx86_64" ; then
       DARWIN_NATIVE_ARCH=-m64
     fi
     ;;
esac
AC_SUBST([DARWIN_NATIVE_ARCH])

if test "$target_platform" = "target_android" ; then
  USE_ANDROID=1
  webserver_checkdepends=yes
  CFLAGS="$CFLAGS -Wno-psabi"
  DROID_DVDLIB_SEEK64="-D_FILE_OFFSET_BITS=64 -D_OFF_T_DEFINED_ -Doff_t=off64_t -Dlseek=lseek64"
  CXXFLAGS="$CXXFLAGS -Wno-psabi"
  AC_DEFINE(HAS_EGLGLES, [1], [Define if supporting EGL based GLES Framebuffer])
fi

case $use_platform in
  raspberry-pi)
     target_platform=target_raspberry_pi
     use_neon=no
     use_arch="arm"
     use_cpu=arm1176jzf-s
     use_hardcoded_tables="yes"
     use_openmax=no
     ARCH="arm"
     AC_DEFINE(HAS_EGLGLES, [1], [Define if supporting EGL based GLES Framebuffer])
     USE_OMXLIB=1; AC_DEFINE([HAVE_OMXLIB],[1],["Define to 1 if OMX libs is enabled"])
     USE_MMAL=1; AC_DEFINE([HAS_MMAL],[1],["Define to 1 if MMAL libs is enabled"])
     CFLAGS="$CFLAGS"
     CXXFLAGS="$CXXFLAGS"
     ;;
esac

if test "$host_vendor" = "apple"; then
  use_avahi=no
  use_alsa=no
  use_openmax=no
  use_goom=no
  use_libudev=no
  use_libusb=no
  use_x11=no
  use_wayland=no
fi

XBMC_SETUP_ARCH_DEFINES()

# detect soname version

if test "$host_vendor" = "apple"; then
  echo "#define GIT_REV \"$GIT_REV\"" > git_revision.h
else
  SDL_DEFINES="$SDL_DEFINES -D'GIT_REV=\"$GIT_REV\"'"
fi

if test "$build_shared_lib" = "yes"; then
  final_message="$final_message\n Shared lib\tYes"
  AC_SUBST(USE_LIBXBMC,1)
fi


# platform debug flags
if test "$use_debug" = "yes"; then
  final_message="$final_message\n  Debugging:\tYes"
  if test "$use_profiling" = "yes"; then
    final_message="$final_message\n  Profiling:\tYes"
    DEBUG_FLAGS="-g -pg -D_DEBUG -Wall"
  else
    final_message="$final_message\n  Profiling:\tNo"
    DEBUG_FLAGS="-g -D_DEBUG -Wall"
  fi
else
  final_message="$final_message\n  Debugging:\tNo"
  if test "$use_profiling" = "yes"; then
    final_message="$final_message\n  Profiling:\tYes"
    DEBUG_FLAGS="-pg -DNDEBUG=1"
  else
    final_message="$final_message\n  Profiling:\tNo"
    if test "$use_breakpad" = "yes"; then
       DEBUG_FLAGS="-g -DNDEBUG=1"
    else
       DEBUG_FLAGS="-DNDEBUG=1"
    fi
  fi
fi
CFLAGS="$DEBUG_FLAGS $CFLAGS"
CXXFLAGS="$DEBUG_FLAGS $CXXFLAGS"


if test "$use_optimizations" = "yes"; then
  final_message="$final_message\n  Optimization:\tYes"
  CXXFLAGS="-O2 $CXXFLAGS"
  CFLAGS="-O2 $CFLAGS"
else
  final_message="$final_message\n  Optimization:\tNo"
fi


# platform specific flags
if echo "$ARCH" | grep -q "freebsd" ; then
  LOCALBASE="${LOCALBASE:-/usr/local}"
  CFLAGS="$CFLAGS -I$LOCALBASE/include"
  CXXFLAGS="$CXXFLAGS -I$LOCALBASE/include"
  CPPFLAGS="$CPPFLAGS -I$LOCALBASE/include"
  LDFLAGS="$LDFLAGS -L$LOCALBASE/lib"
fi
if test "$host_vendor" = "apple" ; then
  # standard application paths
  INCLUDES="$INCLUDES -I\$(abs_top_srcdir)/xbmc/osx"
  if test "$use_arch" != "arm"; then
    LIBS="$LIBS -framework ApplicationServices"
    LIBS="$LIBS -framework AudioUnit"
    LIBS="$LIBS -framework AudioToolbox"
    LIBS="$LIBS -framework Cocoa"
    LIBS="$LIBS -framework CoreAudio"
    LIBS="$LIBS -framework CoreVideo"
    LIBS="$LIBS -framework CoreServices"
    LIBS="$LIBS -framework CoreFoundation"
    LIBS="$LIBS -framework DiskArbitration"
    LIBS="$LIBS -framework IOKit"
    LIBS="$LIBS -framework IOSurface"
    LIBS="$LIBS -framework QuartzCore"
    LIBS="$LIBS -framework SystemConfiguration"
    LIBS="$LIBS -framework VideoDecodeAcceleration"
  fi
elif test "$target_platform" = "target_raspberry_pi"; then
  ARCH="arm"
  use_arch="arm"
elif test "$use_arch" = "arm"; then
  CFLAGS="$CFLAGS -mno-apcs-stack-check"
  CXXFLAGS="$CXXFLAGS -mno-apcs-stack-check"
  if test "$use_tegra" = "yes"; then
    # Compile for ARMv7a architecture, need to test gcc for vfpv3-d16 support 
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="-mfpu=vfpv3-d16"
    AC_COMPILE_IFELSE(
      [AC_LANG_SOURCE([int foo;])],
      [ CFLAGS="$SAVE_CFLAGS -Wno-psabi -Wa,-march=armv7a -mtune=cortex-a9 -mfpu=vfpv3-d16 -mthumb-interwork"
        CXXFLAGS="$CXXFLAGS -Wno-psabi -Wa,-march=armv7a -mtune=cortex-a9 -mfpu=vfpv3-d16 -mthumb-interwork"
        use_cpu=cortex-a9],
      [ CFLAGS="$SAVE_CFLAGS -Wa,-march=armv6 -mtune=cortex-a8 -mthumb-interwork"
        CXXFLAGS="$CXXFLAGS -Wa,-march=armv6 -mtune=cortex-a8 -mthumb-interwork"    
        use_cpu=cortex-a8])
  else
    if test "$use_neon" = "yes"; then 
      CFLAGS="$CFLAGS -mfpu=neon -mvectorize-with-neon-quad"
      CXXFLAGS="$CXXFLAGS -mfpu=neon -mvectorize-with-neon-quad"
    fi
  fi
fi

use_sse4=no
if test "$ARCH" = "x86_64-linux" || test "$ARCH" = "i486-linux"; then
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="-msse4.1"
  AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE([int foo;])],
    [ use_sse4=yes
      USE_SSE4=1],
    [ use_sse4=no
      USE_SSE4=0])
  CFLAGS="$SAVE_CFLAGS"
fi

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FSEEKO
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_MMAP
# Boxee is apparently having compile problems
# if HAVE_REALLOC is defined.  Sort this later.
#AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRCOLL
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit dup2 fdatasync floor fs_stat_dev ftime ftruncate getcwd gethostbyaddr gethostbyname gethostname getpagesize getpass gettimeofday inet_ntoa lchown localeconv memchr memmove memset mkdir modf munmap pow rmdir select setenv setlocale socket sqrt strcasecmp strchr strcspn strdup strerror strncasecmp strpbrk strrchr strspn strstr strtol strtoul sysinfo tzset utime posix_fadvise localtime_r])

# Check for various sizes
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([size_t])

# Check for intrinsics
AC_MSG_CHECKING([for __sync_add_and_fetch(temp, 1)])
AC_TRY_LINK([],[long* temp=0; long ret=__sync_add_and_fetch(temp, 1);],
                [have_builtin_sync_add_and_fetch=yes],
                [have_builtin_sync_add_and_fetch=no])
AC_MSG_RESULT($have_builtin_sync_add_and_fetch)
if test "x$have_builtin_sync_add_and_fetch" = "xyes"; then
    AC_DEFINE(HAS_BUILTIN_SYNC_ADD_AND_FETCH, 1,
        [Define to 1 if your compiler supports the __sync_add_and_fetch() intrinsic.])
fi

AC_MSG_CHECKING([for __sync_sub_and_fetch(temp, 1)])
AC_TRY_LINK([],[long* temp=0; long ret=__sync_sub_and_fetch(temp, 1);],
                [have_builtin_sync_sub_and_fetch=yes],
                [have_builtin_sync_sub_and_fetch=no])
AC_MSG_RESULT($have_builtin_sync_sub_and_fetch)
if test "x$have_builtin_sync_sub_and_fetch" = "xyes"; then
    AC_DEFINE(HAS_BUILTIN_SYNC_SUB_AND_FETCH, 1,
        [Define to 1 if your compiler supports the __sync_sub_and_fetch() intrinsic.])
fi

AC_MSG_CHECKING([for __sync_val_compare_and_swap(temp, 1, 1)])
AC_TRY_LINK([],[long *temp = 0; long ret=__sync_val_compare_and_swap(temp, 1, 1);],
                [have_builtin_sync_val_compare_and_swap=yes],
                [have_builtin_sync_val_compare_and_swap=no])
AC_MSG_RESULT($have_builtin_sync_val_compare_and_swap)
if test "x$have_builtin_sync_val_compare_and_swap" = "xyes"; then
    AC_DEFINE(HAS_BUILTIN_SYNC_VAL_COMPARE_AND_SWAP, 1,
        [Define to 1 if your compiler supports the __sync_val_compare_and_swap() intrinsic.])
fi

# Check for u16string/u32string declarations
AC_LANG_PUSH([C++])
AC_CHECK_TYPES([std::u16string, std::u32string], [], [], [[#include <string>]])
AC_CHECK_TYPES([char16_t, char32_t])
AC_CHECK_SIZEOF([wchar_t])
AC_LANG_POP([C++])

# Add top source directory for all builds so we can use config.h
INCLUDES="-I\$(abs_top_srcdir) $INCLUDES" 

# Check inotify availability
AC_CHECK_HEADER([sys/inotify.h], AC_DEFINE([HAVE_INOTIFY],[1],[Define if we have inotify]),)

# Checks for boost headers using CXX instead of CC
AC_LANG_PUSH([C++])
AC_CHECK_HEADER([boost/circular_buffer.hpp],, AC_MSG_ERROR($missing_library))
AC_LANG_POP([C++])

# Python
if test -z "$PYTHON_NOVERSIONCHECK"; then
  AX_PYTHON_DEVEL([>= '2.6'])
  AX_PYTHON_DEVEL([< '3.0'])
  PYTHON_VERSION=$ac_python_version
fi

if test -z "$PYTHON_VERSION"; then
  AC_MSG_ERROR([Can't find a Python version.])
else
  LIBS="$LIBS $PYTHON_LDFLAGS"
  AC_MSG_NOTICE([Using Python $PYTHON_VERSION])
fi

# Wayland
if test "$use_wayland" = "yes"; then
  AC_MSG_NOTICE($wayland_enabled)
  PKG_CHECK_MODULES([WAYLAND_CLIENT],    [wayland-client],
    [INCLUDES="$INCLUDES $WAYLAND_CLIENT_CFLAGS";
     WAYLAND_CLIENT_LIBRARY_LINE=`LIBRARY=\`${PKG_CONFIG} --libs-only-l wayland-client\`; echo ${LIBRARY:2}`;
     XB_FIND_SONAME([WAYLAND_CLIENT_LIBRARY], ${WAYLAND_CLIENT_LIBRARY_LINE})],
     AC_MSG_ERROR($missing_library))
  PKG_CHECK_MODULES([WAYLAND_EGL],    [wayland-egl],
    [INCLUDES="$INCLUDES $WAYLAND_EGL_CFLAGS";
     WAYLAND_EGL_LIBRARY_LINE=`LIBRARY=\`${PKG_CONFIG} --libs-only-l wayland-egl\`; echo ${LIBRARY:2:11}`
     XB_FIND_SONAME([WAYLAND_EGL_LIBRARY], ${WAYLAND_EGL_LIBRARY_LINE})],
    AC_MSG_ERROR($missing_library))
  PKG_CHECK_MODULES([XKBCOMMON],    [xkbcommon],
    [INCLUDES="$INCLUDES $XKBCOMMON_CFLAGS";
     XKBCOMMON_LIBRARY_LINE=`LIBRARY=\`${PKG_CONFIG} --libs-only-l xkbcommon\`; echo ${LIBRARY:2}`;
     XB_FIND_SONAME([XKBCOMMON_LIBRARY], $XKBCOMMON_LIBRARY_LINE)],
    AC_MSG_ERROR($missing_library))
  AC_DEFINE([HAVE_WAYLAND], [1], [Define to 1 if you have Wayland libs installed.])
  AC_DEFINE([HAVE_XKBCOMMON], [1], [Define to 1 if you have libxkbcommon installed.])

  # If we are also building with tests then we want to build
  # wayland tests as well
  if test "$configure_gtest" = "yes"; then
    have_weston_sdk=no;
    PKG_CHECK_MODULES([PIXMAN],
                      [pixman-1],have_pixman=yes,
                      [AC_MSG_WARN($missing_library); have_pixman=no])
    PKG_CHECK_MODULES([WESTON],
                      [weston >= 1.1.90],[have_weston_sdk=yes],
                      [have_weston_sdk=no; AC_MSG_WARN($missing_library)])

    AC_CHECK_PROG(WAYLAND_SCANNER, wayland-scanner, "wayland-scanner", "no")
    if test "x$WAYLAND_SCANNER" == "xno"; then
      AC_MSG_WARN($missing_program)
    else
      if test "x$have_weston_sdk" == "xyes" && test "x$have_pixman" = "xyes"; then
        AC_SUBST(WAYLAND_TEST_INCLUDES,"$WAYLAND_CLIENT_CFLAGS $XKBCOMMON_CFLAGS $PIXMAN_CFLAGS $WESTON_CFLAGS")
        AC_SUBST(WAYLAND_TEST_LIBS,"$WAYLAND_CLIENT_LIBS $XKBCOMMON_LIBS $PIXMAN_LIBS $WESTON_LIBS")
        AC_DEFINE([HAVE_WESTON_SDK], [1], [Define to 1 if Weston SDK is installed.])
        AC_SUBST(USE_WAYLAND_TEST_EXTENSION, 1)
      fi
      AC_SUBST(WAYLAND_SCANNER)
      AC_DEFINE([HAVE_WAYLAND_XBMC_PROTO],[1],["Define to 1 if the wayland test-protocol will be built"])
    fi
  fi

  # Disable SDL and X11 builds
  use_joystick=no
  use_x11=no

  # Wayland requires the EGL "window system" which in turn only supports
  # the OpenGL ES API, so enable gles support
  use_gles=yes
else
  AC_MSG_RESULT($wayland_disabled)
fi

# Checks for platforms libraries.
if test "$use_gles" = "yes"; then
  use_gl="no"
  # GLES overwrites GL if both set to yes.
  if test "$host_vendor" = "apple" ; then
    AC_DEFINE([HAVE_LIBEGL],[1],["Define to 1 if you have the `EGL' library (-lEGL)."])
    AC_DEFINE([HAVE_LIBGLESV2],[1],["Define to 1 if you have the `GLESv2' library (-lGLESv2)."])
    AC_MSG_RESULT(== WARNING: OpenGLES support is assumed.)
  else
    if test "$target_platform" = "target_raspberry_pi"; then
      AC_DEFINE([HAVE_LIBEGL],[1],["Define to 1 if you have the `EGL' library (-lEGL)."])
      AC_DEFINE([HAVE_LIBGLESV2],[1],["Define to 1 if you have the `GLESv2' library (-lGLESv2)."])
      AC_MSG_RESULT(== WARNING: OpenGLES support is assumed.)
      LIBS="$LIBS -lEGL -lGLESv2 -lbcm_host -lvcos -lvchiq_arm -lmmal -lmmal_core -lmmal_util"
    else
      AC_CHECK_LIB([EGL],   [main],, AC_MSG_ERROR($missing_library))
      AC_CHECK_LIB([GLESv2],[main],, AC_MSG_ERROR($missing_library))
    fi
  fi
else
  if test "$use_gl" = "yes"; then
    if test "$host_vendor" = "apple" ; then
      # linking to OpenGL.framework instead of libGL, libGLU so AC_CHECK_LIB will fail
      LIBS="$LIBS -framework OpenGL"
      AC_DEFINE([HAVE_LIBGL],[1],["Define to 1 if you have the `GL' library (-lGL)."])
      AC_MSG_RESULT(== WARNING: OpenGL support is assumed.)
      AC_DEFINE([HAVE_LIBGLU],[1],["Define to 1 if you have the `GLU' library (-lGLU)."])
      AC_MSG_RESULT(== WARNING: OpenGLU support is assumed.)
      AC_CHECK_LIB([GLEW],[main],, AC_MSG_ERROR($missing_library))
    else
      AC_CHECK_LIB([GL],  [main],, AC_MSG_ERROR($missing_library))
      AC_CHECK_LIB([GLEW],[main],, AC_MSG_ERROR($missing_library))
      AC_CHECK_LIB([GLU], [main],, AC_MSG_ERROR($missing_library))
    fi
  else
    AC_MSG_RESULT(== WARNING: OpenGL support is disabled. ${APP_NAME} will run VERY slow. ==)
    AC_CHECK_LIB([SDL2_gfx],[main])
  fi
fi

# platform common libraries
if test "$use_mysql" = "yes"; then
  AC_PATH_PROG(MYSQL_CONFIG, mysql_config,"no")
  if test "x$MYSQL_CONFIG" != "xno"; then
    AC_DEFINE([HAVE_MYSQL],[1],["Define to 1 if you have the `mysql' library (-lmysqlclient)."])
    INCLUDES="$INCLUDES `$MYSQL_CONFIG --include`"
    MYSQL_LIBS=`$MYSQL_CONFIG --libs`
    LIBS="$LIBS $MYSQL_LIBS"
    AC_SUBST(MYSQL_LIBS)
  else
    AC_MSG_ERROR($missing_program)
  fi
fi
AC_CHECK_HEADER([ass/ass.h],, AC_MSG_ERROR($missing_library))
AC_CHECK_HEADER([mpeg2dec/mpeg2.h],, AC_MSG_ERROR($missing_library))
AC_CHECK_HEADER([mpeg2dec/mpeg2convert.h],, AC_MSG_ERROR($missing_library),
  AC_INCLUDES_DEFAULT()
  [#include <mpeg2dec/mpeg2.h>])
AC_CHECK_HEADER([jpeglib.h],,        AC_MSG_ERROR($missing_library))
AC_CHECK_HEADER([ogg/ogg.h],,        AC_MSG_ERROR($missing_library))
AC_CHECK_HEADER([vorbis/vorbisfile.h],, AC_MSG_ERROR($missing_library))

PKG_CHECK_MODULES([LIBCURL], [libcurl],, AC_MSG_ERROR([libcurl not found]))
XB_FIND_SONAME([CURL], [curl])
AC_MSG_CHECKING([for CRYPTO_set_locking_callback(0) in $CURL_SONAME])
if test "$host_vendor" = "apple" ; then
  libchecker="$NM"
  searchpattern="T [_]?CRYPTO_set_locking_call"
else
  libchecker="$READELF -s"
  searchpattern="CRYPTO_set_locking_call"
fi
if test $($libchecker $CURL_FILENAME  | grep -Eq "${searchpattern}" ; echo $?) -eq 0 ; then
  AC_MSG_RESULT(yes)
  AC_DEFINE([HAS_CURL_STATIC], [1], [Whether OpenSSL inside libcurl is static.])
else
  AC_MSG_RESULT(no)
fi

AC_CHECK_HEADER([openssl/crypto.h], AC_DEFINE([HAVE_OPENSSL],[1],[Define if we have openssl]),)
AC_CHECK_HEADER([gcrypt.h], gcrypt_headers_available=yes,gcrypt_headers_available=no)
if test "$gcrypt_headers_available" = "yes"; then
   # if we have the headers then we must have the lib
   AC_CHECK_LIB([gpg-error],[main],, AC_MSG_ERROR($missing_library))
   AC_CHECK_LIB([gcrypt],[gcry_control],, AC_MSG_ERROR($missing_library))
   AC_DEFINE([HAVE_GCRYPT],[1],[Define if we have gcrypt])
fi
PKG_CHECK_MODULES([GNUTLS], [gnutls], [have_gnutls=yes];AC_DEFINE([HAVE_GNUTLS], [1], [Define if we have gnutls]), AC_MSG_WARN("gnutls not found"))

AC_CHECK_LIB([bz2],         [main],, AC_MSG_ERROR($missing_library))
AC_CHECK_LIB([jpeg],        [main],, AC_MSG_ERROR($missing_library)) # check for cximage
AC_CHECK_LIB([tiff],        [main],, AC_MSG_ERROR($missing_library))
if echo "$ARCH" | grep -q freebsd; then
AC_CHECK_LIB([pthread],     [main],LIBS="-pthread $LIBS", AC_MSG_ERROR($missing_library))
AC_CHECK_LIB([pthread],     [pthread_set_name_np],
  AC_DEFINE([HAVE_PTHREAD_SET_NAME_NP],[1],["Define to 1 if pthread has pthread_set_name_np"]),
  AC_MSG_RESULT([Could not find pthread_set_name_np in pthread]))
else
if test "$target_platform" != "target_android" ; then
  AC_CHECK_LIB([pthread],     [main],, AC_MSG_ERROR($missing_library))
  AC_CHECK_LIB([pthread],     [pthread_setname_np],
    AC_DEFINE([HAVE_PTHREAD_SETNAME_NP],[1],["Define to 1 if pthread has pthread_setname_np"]),
    AC_MSG_RESULT([Could not find pthread_setname_np in pthread]))
fi
fi
AC_CHECK_LIB([lzo2],        [main],, AC_MSG_ERROR($missing_library))
AC_CHECK_LIB([z],           [main],, AC_MSG_ERROR($missing_library))
AC_CHECK_LIB([crypto],      [main],, AC_MSG_ERROR($missing_library))
AC_CHECK_LIB([ssl],         [main],, AC_MSG_ERROR($missing_library))
if test "$use_mysql" = "yes"; then
  AC_CHECK_LIB([mysqlclient], [main],, AC_MSG_ERROR($missing_library))
fi
AC_CHECK_LIB([bluetooth],   [hci_devid],, AC_MSG_RESULT([Could not find suitable version of libbluetooth]))

AC_LANG_PUSH([C++])
PKG_CHECK_MODULES([TINYXML], [tinyxml >= 2.6.2],
  [INCLUDES="$INCLUDES $TINYXML_CFLAGS"; LIBS="$LIBS $TINYXML_LIBS"],
  [AC_RUN_IFELSE(
   [AC_LANG_SOURCE([[
#include <stdlib.h>
#include <tinyxml.h>

int main() {
  if (TIXML_MAJOR_VERSION < 2) exit(1);
  if (TIXML_MAJOR_VERSION == 2 && ( TIXML_MINOR_VERSION < 6 || ( TIXML_MINOR_VERSION == 6 && TIXML_PATCH_VERSION < 2 ))) exit(1);
}
    ]])],[AC_CHECK_LIB([tinyxml], [main],, AC_MSG_ERROR("tinyxml >= 2.6.2 not found"))], [AC_MSG_ERROR([tinyxml >= 2.6.2 not found])]
  )])
AC_LANG_POP([C++])

PKG_CHECK_MODULES([YAJL], [yajl >= 2],
  [INCLUDES="$INCLUDES $YAJL_CFLAGS"; LIBS="$LIBS $YAJL_LIBS"; YAJL_FOUND="true"],
  [AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE([[
#include <yajl/yajl_version.h>
#if !defined YAJL_MAJOR || YAJL_MAJOR < 2
#error yajl version < 2;
#endif
    ]])],, [AC_MSG_ERROR([libyajl >= 2.0.0 not found])]
  )])

if test "$YAJL_FOUND" != "true"; then
  AC_CHECK_LIB([yajl], [main],, [AC_MSG_ERROR("libyajl >= 2.0.0 not found")])
  AC_CHECK_HEADERS([yajl/yajl_common.h], [AC_MSG_NOTICE(libyajl2 found)], [AC_MSG_ERROR("yajl/yajl_common.h not found")])
fi

if test "$target_platform" = "target_android" ; then
  AC_CHECK_LIB([android],     [main],, AC_MSG_ERROR($missing_library))
  AC_CHECK_LIB([log],         [__android_log_vprint],, AC_MSG_ERROR($missing_library))
  AC_CHECK_LIB([jnigraphics],     [main],, AC_MSG_ERROR($missing_library))
fi
PKG_CHECK_MODULES([LIBXML], [libxml-2.0],
  [INCLUDES="$INCLUDES $LIBXML_CFLAGS"; LIBS="$LIBS $LIBXML_LIBS"],
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([LIBXSLT], [libxslt],
  [INCLUDES="$INCLUDES $LIBXSLT_CFLAGS"; LIBS="$LIBS $LIBXSLT_LIBS"],
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([FRIBIDI],    [fribidi],
  [INCLUDES="$INCLUDES $FRIBIDI_CFLAGS"; LIBS="$LIBS $FRIBIDI_LIBS"],
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([SQLITE3],    [sqlite3],
  [INCLUDES="$INCLUDES $SQLITE3_CFLAGS"; LIBS="$LIBS $SQLITE3_LIBS"],
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([PNG],        [libpng],
  [INCLUDES="$INCLUDES $PNG_CFLAGS"; LIBS="$LIBS $PNG_LIBS"],
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([PCRECPP],    [libpcrecpp],
  [INCLUDES="$INCLUDES $PCRECPP_CFLAGS"; LIBS="$LIBS $PCRECPP_LIBS"]; \
  AC_DEFINE([HAVE_LIBPCRECPP],[1],["Define to 1 if libpcrecpp is installed"]),
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([PCRE],       [libpcre],
  [INCLUDES="$INCLUDES $PCRE_CFLAGS"; LIBS="$LIBS $PCRE_LIBS"]; \
  AC_DEFINE([HAVE_LIBPCRE],[1],["Define to 1 if libpcre is installed"]),
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([FREETYPE2],  [freetype2],
  [INCLUDES="$INCLUDES $FREETYPE2_CFLAGS"; LIBS="$LIBS $FREETYPE2_LIBS"],
  AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([TAGLIB],  [taglib >= 1.8],
  [INCLUDES="$INCLUDES $TAGLIB_CFLAGS"; LIBS="$LIBS $TAGLIB_LIBS"],
  AC_MSG_ERROR($missing_library))

if test "$use_optical_drive" = "yes"; then
  PKG_CHECK_MODULES([CDIO],       [libcdio],
    [INCLUDES="$INCLUDES $CDIO_CFLAGS"; LIBS="$LIBS $CDIO_LIBS"],
    AC_MSG_ERROR($missing_library))
fi

if test "$target_platform" = "target_android" ; then
PKG_CHECK_MODULES([ZIP],       [libzip],
  [INCLUDES="$INCLUDES $ZIP_CFLAGS"; LIBS="$LIBS $ZIP_LIBS"],
  AC_MSG_ERROR($missing_library))
fi

# check for libbluray
AS_CASE([x$use_libbluray],
  [xyes],[
    PKG_CHECK_MODULES([LIBBLURAY],[libbluray >= 0.2.1],[use_libbluray="yes"], AC_MSG_ERROR($missing_library))
  ],
  [xauto],[
    PKG_CHECK_MODULES([LIBBLURAY],[libbluray >= 0.2.1],[use_libbluray="yes"], [use_libbluray="no"])
  ])

AS_CASE([x$use_libbluray],
  [xyes],[
    INCLUDES="$INCLUDES $LIBBLURAY_CFLAGS";
    LIBBLURAY_LIB_BASENAME=`$PKG_CONFIG --libs-only-L "libbluray >= 0.2.1"`;
    XB_FIND_SONAME([BLURAY], [bluray], [use_libbluray], [$LIBBLURAY_LIB_BASENAME])
    AC_DEFINE([HAVE_LIBBLURAY], 1, [System has libbluray library])
    AC_SUBST([HAVE_LIBBLURAY], 1)
    AC_CHECK_LIB([bluray], [bd_register_argb_overlay_proc],
      AC_CHECK_DECL([BD_EVENT_IDLE],
        AC_DEFINE([HAVE_LIBBLURAY_BDJ], 1, [System has libbluray library with bdj support]),,
        [[#include <libbluray/bluray.h>]]))
  ],[
    AC_SUBST([HAVE_LIBBLURAY], 0)
  ]
)

#Check to see if libs are needed for functions that are often built-in to libc
AC_SEARCH_LIBS([iconv_open],iconv,,AC_SEARCH_LIBS([libiconv_open],iconv,,AC_MSG_ERROR($missing_library)))
AC_SEARCH_LIBS([dlopen],dl)
AC_SEARCH_LIBS([clock_gettime],rt)
AC_SEARCH_LIBS([dn_expand],  resolv)
AC_SEARCH_LIBS([_dn_expand], resolv)
AC_SEARCH_LIBS([__dn_expand],resolv)

# platform dependent libraries
if test "$host_vendor" = "apple" ; then
  if test "$use_arch" != "arm"; then
    AC_CHECK_LIB([SDL],      [main],, AC_MSG_ERROR($missing_library))
    AC_DEFINE([HAVE_SDL],[1],["Define to 1 if using sdl"])
    AC_DEFINE([SDL_VERSION],[1],["SDL major version"])
  fi
else
if test "$target_platform" != "target_android" ; then
  AC_CHECK_LIB([jasper],     [main],, AC_MSG_ERROR($missing_library)) # check for cximage
fi
if test "x$use_alsa" != "xno"; then
  PKG_CHECK_MODULES([ALSA],  [alsa],
    [INCLUDES="$INCLUDES $ALSA_CFLAGS"; LIBS="$LIBS $ALSA_LIBS"; use_alsa=yes],
    AC_MSG_NOTICE($alsa_not_found); use_alsa=no)
fi
if test "x$use_dbus" != "xno"; then
  PKG_CHECK_MODULES([DBUS],    [dbus-1],
    [INCLUDES="$INCLUDES $DBUS_CFLAGS"; LIBS="$LIBS $DBUS_LIBS"; use_dbus=yes]; \
    AC_DEFINE([HAVE_DBUS],[1],["Define to 1 if dbus is installed"]),
    AC_MSG_NOTICE($missing_library); use_dbus=no)
else
  AC_MSG_NOTICE($dbus_disabled)
fi
  #case "$host_vendor" != "apple"; determine availability of SDL or SDL2
  # joystick
  if test "$use_joystick" != "no"; then
    PKG_CHECK_MODULES([SDL2], [sdl2],
      [AC_DEFINE([SDL_VERSION],[2],["SDL major version"])
       AC_DEFINE([HAVE_SDL],[1],["Define to 1 if using sdl"])
       INCLUDES="$INCLUDES $SDL2_CFLAGS"; LIBS="$LIBS $SDL2_LIBS"; use_joystick="yes"],
      [if test "$use_joystick" = "yes"; then
        AC_MSG_ERROR($sdl_joystick_not_found)
      elif test "$use_joystick" != "no"; then
        AC_MSG_NOTICE($sdl_joystick_not_found)
        use_joystick="no"
      fi
     ])
  else
    AC_MSG_RESULT($sdl_joystick_not_found)
  fi
fi

XB_FIND_SONAME([OGG],         [ogg])
XB_FIND_SONAME([VORBIS],      [vorbis])
XB_FIND_SONAME([VORBISFILE],  [vorbisfile])
XB_FIND_SONAME([ASS],         [ass])
XB_FIND_SONAME([MPEG2],       [mpeg2])

# WebServer
if test "$use_webserver" = "yes"; then
  AC_CHECK_LIB([microhttpd],  [main],, AC_MSG_ERROR($missing_library))
fi

# Optical
if test "$use_optical_drive" = "yes"; then
  AC_DEFINE([HAS_DVD_DRIVE], [1], [Define to 1 to have optical drive support])
fi

# Alsa
if test "$use_alsa" = "yes"; then
  PKG_CHECK_MODULES([ALSA],  [alsa],
    [INCLUDES="$INCLUDES $ALSA_CFLAGS"; LIBS="$LIBS $ALSA_LIBS"],
    AC_MSG_ERROR($missing_library))
  AC_DEFINE([HAS_ALSA], [1], [Define to 0 to disable ALSA support])
else
  use_alsa="no"
  AC_MSG_RESULT($alsa_disabled)
fi

# PulseAudio
if test "x$use_pulse" != "xno"; then
  if test "$host_vendor" = "apple" ; then
    if test "x$use_pulse" = "xyes"; then
      AC_MSG_ERROR($pulse_disabled)
    else
      use_pulse="no"
      AC_MSG_RESULT($pulse_disabled)
    fi
    USE_PULSE=0
  else
    PKG_CHECK_MODULES([PULSE], [libpulse >= 1.0],
      [INCLUDES="$INCLUDES $PULSE_CFLAGS"; LIBS="$LIBS $PULSE_LIBS"; USE_PULSE=1;
        HAVE_LIBPULSE=1; AC_DEFINE([HAVE_LIBPULSE],[1],[Pulse audio enabled])],
      [pulse_found="no"; USE_PULSE=0; HAVE_LIBPULSE=0])

    if test "$pulse_found" = "no"; then
      if test "x$use_pulse" = "xyes"; then
        AC_MSG_ERROR($pulse_not_found)
      else
        use_pulse="no"
        AC_MSG_RESULT($pulse_disabled)
      fi
    fi
  fi
else
  AC_MSG_RESULT($pulse_disabled)
  USE_PULSE=0
fi

# avahi
if test "$use_avahi" = "yes"; then
  AC_CHECK_LIB([avahi-common], [main],,
    use_avahi=no;AC_MSG_RESULT($avahi_not_found))
  if test "$use_avahi" = "yes"; then
    #either both libs or none
    AC_CHECK_LIB([avahi-client], [main],,
      use_avahi=no;AC_MSG_RESULT($avahi_not_found))
  fi
else
  AC_MSG_RESULT($avahi_disabled)
fi

# mdnsembedded
if test "$use_mdnsembedded" != "no"; then
  AC_CHECK_LIB([mDNSEmbedded], [embedded_mDNSInit],,
    [if test "$use_mdnsembedded" = "yes"; then
      AC_MSG_ERROR($mdnsembedded_not_found)
      USE_MDNSEMBEDDED=0
    elif test "$use_mdnsembedded" != "no"; then
      AC_MSG_NOTICE($mdnsembedded_not_found)
      use_mdnsembedded="no"
      USE_MDNSEMBEDDED=0
    fi
   ])
  if test $use_mdnsembedded != "no"; then
    USE_MDNSEMBEDDED=1
  fi
else
  AC_MSG_RESULT($mdnsembedded_disabled)
  USE_MDNSEMBEDDED=0
fi
AC_SUBST(USE_MDNSEMBEDDED)

# X11
if test "$use_x11" = "yes"; then
  AC_MSG_NOTICE($x11_enabled)
  PKG_CHECK_MODULES([X11],    [x11],
    [INCLUDES="$INCLUDES $X11_CFLAGS"; LIBS="$LIBS $X11_LIBS"],
    AC_MSG_ERROR($missing_library))
  PKG_CHECK_MODULES([XEXT],  [xext],
    [INCLUDES="$INCLUDES $XEXT_CFLAGS"; LIBS="$LIBS $XEXT_LIBS"],
    AC_MSG_ERROR($missing_library))
  PKG_CHECK_MODULES([XRANDR], [xrandr],
    [INCLUDES="$INCLUDES $XRANDR_CFLAGS"; LIBS="$LIBS $XRANDR_LIBS"],
    AC_MSG_ERROR($missing_library))
  PKG_CHECK_MODULES([DRM],  [libdrm],
    [INCLUDES="$INCLUDES $DRM_CFLAGS"; LIBS="$LIBS $DRM_LIBS"],
    AC_MSG_ERROR($missing_library))
  AC_DEFINE([HAVE_X11], [1], [Define to 1 if you have X11 libs installed.])
else
  AC_MSG_RESULT($x11_disabled)
fi

# WAVEFORM
if test "$use_waveform" = "yes"; then
  AC_MSG_NOTICE($waveform_enabled)
    DISABLE_WAVEFORM=0
else
  AC_MSG_NOTICE($waveform_disabled)
  DISABLE_WAVEFORM=1
fi

# SPECTRUM
if test "$use_spectrum" = "yes"; then
  AC_MSG_NOTICE($spectrum_enabled)
    DISABLE_SPECTRUM=0
else
  AC_MSG_NOTICE($spectrum_disabled)
  DISABLE_SPECTRUM=1
fi

# GOOM
if test "$use_goom" = "yes" && test "$use_gl" = "yes"; then
  AC_MSG_NOTICE($goom_enabled)
  DISABLE_GOOM=0
else
  AC_MSG_NOTICE($goom_disabled)
  DISABLE_GOOM=1
fi

# RSXS
if test "$use_rsxs" = "no" || test "$use_gl" = "no"; then
  AC_MSG_NOTICE($rsxs_disabled)
  DISABLE_RSXS=1
else
  AC_MSG_NOTICE($rsxs_enabled)
  DISABLE_RSXS=0
  # darwin osx can do rsxs but does not use x11, so do not pkg-config check for them
  if test "$host_vendor" != "apple" ; then
    PKG_CHECK_MODULES([XT],    [xt],
      [INCLUDES="$INCLUDES $XT_CFLAGS"; LIBS="$LIBS $XT_LIBS"],
      AC_MSG_ERROR($missing_library))
    PKG_CHECK_MODULES([XMU],   [xmu],
      [INCLUDES="$INCLUDES $XMU_CFLAGS"; LIBS="$LIBS $XMU_LIBS"],
      AC_MSG_ERROR($missing_library))
  fi
fi

# FISHBMC
if test "$use_fishbmc" = "no" || test "$use_gl" = "no"; then
  AC_MSG_NOTICE($fishbmc_disabled)   
  DISABLE_FISHBMC=1
  use_fishbmc="no"
else
  AC_MSG_NOTICE($fishbmc_enabled)
  DISABLE_FISHBMC=0
fi

# PROJECTM
if test "$use_projectm" = "no" || test "$use_gl" = "no"; then
  AC_MSG_NOTICE($projectm_disabled)
  DISABLE_PROJECTM=1
  use_projectm="no"
else
  AC_MSG_NOTICE($projectm_enabled)
  DISABLE_PROJECTM=0
fi

# touch skin
use_touch_skin=no
if [[ -f "addons/skin.re-touched/addon.xml" ]]; then 
  use_touch_skin=yes
  USE_TOUCH_SKIN=1
  AC_DEFINE([HAS_TOUCH_SKIN], [1], [Whether to build the touch skin.])
else
  USE_TOUCH_SKIN=0
fi

# libssh
if test "x$use_ssh" = "xno"; then
  AC_MSG_NOTICE($ssh_disabled)
  use_libssh="no"
else
  AC_CHECK_LIB([ssh], [sftp_tell64],, AC_MSG_ERROR($ssh_not_found))
  AC_DEFINE([HAVE_LIBSSH], [1], [Whether to use libSSH library.])
fi

# libRTMP
if test "$use_librtmp" != "no"; then
  AC_CHECK_HEADERS([librtmp/log.h librtmp/amf.h librtmp/rtmp.h],,
   [if test "$use_librtmp" = "yes"; then
      AC_MSG_ERROR($librtmp_not_found)
    elif test "$use_librtmp" != "no"; then
      AC_MSG_NOTICE($librtmp_not_found)
      use_librtmp="no"
    fi
   ])
  if test "$use_librtmp" != "no"; then
    XB_FIND_SONAME([RTMP], [rtmp], [use_librtmp])
  fi
  if test "$use_librtmp" != "no"; then
    AC_DEFINE([HAS_LIBRTMP], [1], [Whether to use libRTMP library.])
  fi
else
  AC_MSG_NOTICE($librtmp_disabled)
fi

# samba
if test "x$use_samba" != "xno"; then
  PKG_CHECK_MODULES([SAMBA], [smbclient],
    [INCLUDES="$INCLUDES $SAMBA_CFLAGS"; LIBS="$LIBS $SAMBA_LIBS"],
    [AC_CHECK_LIB([smbclient], [main],,
      use_samba=no;AC_MSG_ERROR($missing_library))
      USE_LIBSMBCLIENT=0
    ])
else
  AC_MSG_RESULT($samba_disabled)
  USE_LIBSMBCLIENT=0
fi

if test "x$use_samba" != "xno"; then
  AC_DEFINE([HAVE_LIBSMBCLIENT], [1], [Define to 1 if you have Samba installed])
  USE_LIBSMBCLIENT=1
fi

# libnfs
if test "$use_libnfs" != "no"; then
  AC_CHECK_HEADERS([nfsc/libnfs.h],,
   [if test "$use_libnfs" = "yes"; then
      AC_MSG_ERROR($libnfs_not_found)
      USE_LIBNFS=0
    elif test "$use_libnfs" != "no"; then
      AC_MSG_NOTICE($libnfs_not_found)
      use_libnfs="no"
      USE_LIBNFS=0
    fi
   ])
  if test "$use_libnfs" != "no"; then
    XB_FIND_SONAME([NFS], [nfs], [use_libnfs])
  fi
  if test "$use_libnfs" != "no"; then
    AC_DEFINE([HAVE_LIBNFS], [1], [Whether to use libnfs library.])
    USE_LIBNFS=1
  fi
else
  USE_LIBNFS=0
  AC_MSG_NOTICE($libnfs_disabled)
fi


# libplist for airplay feature
USE_AIRPLAY=0
if test "$use_airplay" != "no"; then
  AC_CHECK_HEADER([plist/plist.h],,
   [if test "$use_airplay" = "yes"; then
      AC_MSG_ERROR($libplist_not_found)
    elif test "$use_airplay" != "no"; then
      AC_MSG_NOTICE($libplist_not_found)
      use_airplay="no"
    fi
   ])

  if test "$use_airplay" != "no"; then
    XB_FIND_SONAME([PLIST], [plist], [use_airplay])
    USE_AIRPLAY=1
    AC_DEFINE([HAVE_LIBPLIST],[1],["Define to 1 if you have libplist."])
  fi
fi

# libshairplay for AirTunes
USE_AIRTUNES=0
if test "x$use_airtunes" != "xno"; then
  AC_CHECK_HEADERS([shairplay/raop.h],USE_AIRTUNES=1,
   [AC_MSG_NOTICE($libshairplay_not_found)
   ])

  if test "x$USE_AIRTUNES" != "x0"; then
    XB_FIND_SONAME([SHAIRPLAY], [shairplay], [USE_AIRTUNES])
    AC_CHECK_MEMBERS([struct raop_callbacks_s.cls],,,
                     [[#include <shairplay/raop.h>]])
    AC_DEFINE([HAVE_LIBSHAIRPLAY],[1],["Define to 1 if you have libshairplay."])
  fi

  if test "x$USE_AIRTUNES" == "x0"; then
    if test "x$use_airtunes" == "xyes"; then
      AC_MSG_ERROR("No airtunes library could be found. (libshairplay)")
    fi
    use_airtunes="no"
  fi
fi

# libudev
USE_LIBUDEV=0
if test "$use_libudev" = "auto"; then
  PKG_CHECK_MODULES([UDEV],[libudev],,[use_libudev="no";AC_MSG_RESULT($libudev_not_found)])
elif test "$use_libudev" = "yes" ; then
  PKG_CHECK_MODULES([UDEV],[libudev],,[use_libudev="no";AC_MSG_ERROR($libudev_not_found)])
else
  AC_MSG_NOTICE($libudev_disabled)
fi

if test "x$use_libudev" != "xno"; then
  USE_LIBUDEV=1;INCLUDES="$INCLUDES $UDEV_CFLAGS";LIBS="$LIBS $UDEV_LIBS"
  AC_DEFINE([HAVE_LIBUDEV],[1],["Define to 1 if libudev is installed"])
fi

# libusb
USE_LIBUSB=0

# if libudev is available, we don't need libusb
if test "x$use_libudev" != "xno"; then
  use_libusb="no"
  AC_MSG_NOTICE($libusb_disabled_udev_found)
else
  if echo "$ARCH" | grep -q freebsd ; then
    AC_CHECK_LIB([usb],[main],
	  	[use_libusb="yes";USB_LIBS="-lusb"],
		[use_libusb="no";AC_MSG_RESULT($libusb_not_found)])
  elif test "$use_libusb" = "auto"; then
    PKG_CHECK_MODULES([USB],[libusb],,[use_libusb="no";AC_MSG_RESULT($libusb_not_found)])
  elif test "$use_libusb" = "yes"; then
    PKG_CHECK_MODULES([USB],[libusb],,[use_libusb="no";AC_MSG_ERROR($libusb_not_found)])
  else
    AC_MSG_NOTICE($libusb_disabled)
  fi

  if test "x$use_libusb" != "xno"; then
    USE_LIBUSB=1;INCLUDES="$INCLUDES $USB_CFLAGS";LIBS="$LIBS $USB_LIBS"
    AC_DEFINE([HAVE_LIBUSB],[1],["Define to 1 if libusb is installed"])
  fi
fi

# libcec
USE_LIBCEC=0
if test "x$use_libcec" != "xno"; then
  # libcec is dyloaded, so we need to check for its headers and link any depends.
  if test "x$use_libcec" != "xno"; then
    if test "x$use_libcec" != "xauto"; then
      PKG_CHECK_MODULES([CEC],[libcec >= 3.0.0],,[use_libcec="no";AC_MSG_ERROR($libcec_disabled)])
    else
      PKG_CHECK_MODULES([CEC],[libcec >= 3.0.0],,[use_libcec="no";AC_MSG_RESULT($libcec_disabled)])
    fi

    if test "x$use_libcec" != "xno"; then
      INCLUDES="$INCLUDES $CEC_CFLAGS"
      USE_LIBCEC=1;AC_DEFINE([HAVE_LIBCEC],[1],["Define to 1 if libcec is installed"])
      XB_FIND_SONAME([LIBCEC],[cec],[use_libcec])
      AC_MSG_NOTICE($libcec_enabled)
    else
      use_libcec="no"
      AC_MSG_NOTICE($libcec_disabled)
    fi
  fi
else
  use_libcec="no"
  AC_MSG_NOTICE($libcec_disabled)
fi

# libcap
if test "$use_libcap" != "no"; then
  AC_CHECK_HEADERS([sys/capability.h],,
   [if test "$use_libcap" = "yes"; then
      AC_MSG_ERROR($libcap_not_found)
    elif test "$use_libcap" != "no"; then
      AC_MSG_NOTICE($libcap_not_found)
      use_libcap="no"
    fi
   ])
  if test "$use_libcap" != "no"; then
    AC_CHECK_LIB([cap], main, LIBS="$LIBS -lcap", use_libcap=no)
  fi
  if test "$use_libcap" != "no"; then
    AC_DEFINE([HAVE_LIBCAP], [1], [Whether to use libcap library.])
  fi
else
  AC_MSG_NOTICE($libcap_disabled)
fi

# FFmpeg
FFMPEG_LIBNAMES="libavcodec >= 56.26.100
                 libavfilter >= 5.11.102
                 libavformat >= 56.25.101
                 libavutil >= 54.20.100
                 libpostproc >= 53.3.100
                 libswscale >= 3.1.101
                 libswresample >= 1.1.100"

ffmpeg_build="${abs_top_srcdir}/tools/depends/target/ffmpeg"
FFMPEG_VER_SHA=$(grep "VERSION=" ${ffmpeg_build}/FFMPEG-VERSION | sed 's/VERSION=//g')
AC_DEFINE_UNQUOTED([FFMPEG_VER_SHA], ["$FFMPEG_VER_SHA"], [FFmpeg version hash])

if test "$with_ffmpeg" = "shared"; then
  # allow linking against shared ffmpeg libs
  # a proper version must be installed, we won't build ffmpeg
  USE_STATIC_FFMPEG=0
  AC_DEFINE([USE_STATIC_FFMPEG], [0], [FFmpeg linked dynamically])
fi
if test "${USE_STATIC_FFMPEG}" = "1"; then
  AC_DEFINE([USE_STATIC_FFMPEG], [1], [FFmpeg linked statically])
  ff_libs=$(${ECHO} ${FFMPEG_LIBNAMES} | ${AWK} '/lib/{print $1}' ORS=' ')
  if test -n "${PKG_CONFIG_SYSROOT_DIR}"; then
    # workaround for cross compiling with buildroot on different buildsystem
    # Rpi buildroots pkg-config returns static libs with
    # PKG_CONFIG_SYSROOT_DIR prefixed, OEs does not
    # so lets make sure we always prefix in case PKG_CONFIG_SYSROOT_DIR is defined
    pkg_cfg_prefix="${PKG_CONFIG_SYSROOT_DIR}"
  elif test "${target_platform}" = "target_raspberry_pi" && test "${USE_BUILDROOT}" = "1"; then
      pkg_cfg_prefix=${SYSROOT}
  fi

  if test "$cross_compiling" != "yes"; then
    if test "$use_debug" != "yes"; then
      FFMPEG_OPTS="-r"
    fi
    if test "$use_optimizations" != "yes"; then
      FFMPEG_OPTS="${FFMPEG_OPTS} --disable-optimizations"
    fi

    if test "$with_ffmpeg" = "auto" || test "$with_ffmpeg" = "yes"; then
      SAVE_INCLUDES="$INCLUDES"
      SAVE_LIBS="$LIBS"
      # check for system installed ffmpeg. We require minimum versions.
      PKG_CHECK_MODULES([FFMPEG], [$FFMPEG_LIBNAMES],
        [INCLUDES="$INCLUDES $FFMPEG_CFLAGS"; LIBS="$LIBS $FFMPEG_LIBS"; FFMPEG_FOUND="true"],
        [FFMPEG_FOUND="false"])

      if test "${USE_STATIC_FFMPEG}" = "1" && test "$FFMPEG_FOUND" = "true"; then
        # we need to check if static libs are available
        FFMPEG_LIBDIR=$(${PKG_CONFIG} --static --variable=libdir libavcodec)
        for ff_lib in ${ff_libs}; do
          if test -f ${FFMPEG_LIBDIR}/${ff_lib}.a; then :; else
            AC_MSG_NOTICE("${FFMPEG_LIBDIR}/${ff_lib}.a not found")
            FFMPEG_FOUND="false"
            # restore includes and libs, the ones we found are invalid
            INCLUDES="$SAVE_INCLUDES"
            LIBS="$SAVE_LIBS"
            unset FFMPEG_LIBS FFMPEG_CFLAGS FFMPEG_LIBDIR
          fi
        done
      fi
      if test "$with_ffmpeg" = "yes" || test "$FFMPEG_FOUND" = "false"; then
        # ffmpeg not found with pkg-config, lets install it
        AC_MSG_NOTICE("Installing FFmpeg")
        CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" ${ffmpeg_build}/autobuild.sh ${FFMPEG_OPTS}
        export PKG_CONFIG_PATH="${ffmpeg_build}/ffmpeg-install/lib/pkgconfig:$PKG_CONFIG_PATH"
      fi

    elif test "$with_ffmpeg" = "force"; then
      # always build our ffmpeg
      AC_MSG_NOTICE("FFmpeg installation forced by user - installing our version")
      CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" ${ffmpeg_build}/autobuild.sh ${FFMPEG_OPTS}
      export PKG_CONFIG_PATH="${ffmpeg_build}/ffmpeg-install/lib/pkgconfig:$PKG_CONFIG_PATH"

    elif test "$with_ffmpeg" != "no"; then
      # user passed --with-ffmpeg=/some/path, lets use it
      AC_MSG_NOTICE("using ffmpeg: ${with_ffmpeg}")
      export PKG_CONFIG_PATH="${with_ffmpeg}/lib/pkgconfig:$PKG_CONFIG_PATH"
    fi
  fi
fi

if test "$FFMPEG_FOUND" != "true"; then
  PKG_CHECK_MODULES([FFMPEG], [$FFMPEG_LIBNAMES],
    [INCLUDES="$INCLUDES $FFMPEG_CFLAGS"; LIBS="$LIBS $FFMPEG_LIBS"; FFMPEG_FOUND="true"], 
    [AC_MSG_ERROR("ffmpeg not found")])
fi

if test "${USE_STATIC_FFMPEG}" = "1"; then
  # get the libdir for static linking
  FFMPEG_LIBDIR=${pkg_cfg_prefix}$(PKG_CONFIG_SYSROOT_DIR="" ${PKG_CONFIG} --static --variable=libdir libavcodec)
  GNUTLS_ALL_LIBS=$(${PKG_CONFIG} --static --libs-only-l --silence-errors gnutls)
  VORBISENC_ALL_LIBS=$(${PKG_CONFIG} --static --libs-only-l --silence-errors vorbisenc)

  # check if static libs are available
  for ff_lib in ${ff_libs}; do
    if test -f ${FFMPEG_LIBDIR}/${ff_lib}.a; then :; else
      AC_MSG_ERROR("${FFMPEG_LIBDIR}/${ff_lib}.a not found")
    fi
    # filter out libs we link statically
    filter=$(${ECHO} ${ff_lib} | ${SED} 's/lib/-l/g')
    LIBS=$(${ECHO} ${LIBS} | ${SED} "s/${filter}//g")
  done
  LIBS=$(${ECHO} ${LIBS} | ${SED} ':a;N;$!ba;s/\n/ /g')
fi

echo "Checking for SWIG installation"
AC_PATH_PROG(SWIG_EXE, swig, "none")
if test "$SWIG_EXE" = "none"; then
  AC_PATH_PROG(SWIG20_EXE, swig2.0, "none")
  if test "$SWIG20_EXE" != "none" ; then
    SWIG_EXE=$SWIG20_EXE
  fi
fi
if test "$SWIG_EXE" = "none"; then
  AC_MSG_ERROR($missing_program)
fi
final_message="$final_message\n  SWIG Available:\tYes"

echo "Checking for a jre installation"
AC_PATH_PROG(JAVA_EXE, java, "none")
if test "$JAVA_EXE" = "none"; then
  AC_MSG_ERROR($missing_program)
fi
final_message="$final_message\n  JRE Available:\tYes"

echo "Checking for doxygen installation"
AC_PATH_PROG(DOXYGEN_EXE, doxygen, "none")
if test "$DOXYGEN_EXE" = "none"; then
  AC_MSG_WARN([Unable to find doxygen installation. Will not be able to make docstrings for the python api])
  final_message="$final_message\n  Doxygen Available:\tNo"
  USE_DOXYGEN=0
else
  final_message="$final_message\n  Doxygen Available:\tYes"
  USE_DOXYGEN=1
fi

# VDPAU
if test "x$use_vdpau" != "xno"; then
  if test "$host_vendor" = "apple" ; then
    if test "x$use_vdpau" = "xyes"; then
      AC_MSG_ERROR([VDPAU not supported on this platform])
    else
      use_vdpau="no"
      AC_MSG_NOTICE($vdpau_disabled)
    fi
    USE_VDPAU=0
  else
    USE_VDPAU=1
    AC_CHECK_HEADER([vdpau/vdpau.h],AC_DEFINE([HAVE_LIBVDPAU], [],
      [Define to 1 if you have the 'vdpau' library (-lvdpau).]),
    [if test "x$use_vdpau" = "xyes"; then
      USE_VDPAU=0
      AC_MSG_ERROR([$vdpau_not_found])
    else
      use_vdpau="no"
      USE_VDPAU=0
      AC_MSG_RESULT($vdpau_not_found)
    fi])
  fi
else
  USE_VDPAU=0
  AC_MSG_NOTICE($vdpau_disabled)
fi

# VAAPI
if test "x$use_vaapi" != "xno"; then
  if test "$host_vendor" = "apple" ; then
    if test "x$use_vaapi" = "xyes"; then
      AC_MSG_ERROR([VAAPI not supported on this platform])
    else
      use_vaapi="no"
      AC_MSG_NOTICE($vaapi_disabled)
    fi
    USE_VAAPI=0
  else
    initial_val=$use_vaapi
    if test "x$use_vaapi" != "xno"; then
      PKG_CHECK_MODULES([LIBVA], [libva libva-x11],
        [INCLUDES="$INCLUDES $LIBVA_CFLAGS"; LIBS="$LIBS $LIBVA_LIBS"; USE_VAAPI=1;
         AC_DEFINE([HAVE_LIBVA], [1], [Define to 1 if you have the 'vaapi' libraries])],
        [use_vaapi="no"; USE_VAAPI=0; AC_MSG_RESULT($vaapi_not_found)])
    else
      if test "x$initial_val" = "xyes"; then
        AC_MSG_ERROR($vaapi_not_found)
      fi
    fi
  fi
else
  AC_MSG_NOTICE($vaapi_disabled)
  USE_VAAPI=0
fi

# VTBDecoder
if test "x$use_vtbdecoder" != "xno"; then
  if test "$host_vendor" = "apple" ; then
    HAVE_VIDEOTOOLBOXDECODER=1
    AC_DEFINE([HAVE_VIDEOTOOLBOXDECODER], [1], [Define to 1 if you have the 'VTBDecoder' library.])
    AC_MSG_NOTICE($vtbdecoder_enabled)
  else
    if test "x$use_vtbdecoder" = "xyes"; then
      AC_MSG_ERROR([VTB Decoder not supported on this platform])
    else
      use_vtbdecoder="no"
      AC_MSG_NOTICE($vtbdecoder_disabled)
    fi
  fi
else
  AC_MSG_NOTICE($vtbdecoder_disabled)
fi

# OpenMax
if test "$use_gles" = "yes" && test "$use_openmax" = "auto"; then

  AC_CHECK_HEADERS([OMX_Core.h],
                    USE_OPENMAX=1;[INCLUDES="$INCLUDES $OPENMAX_CFLAGS"; LIBS="$LIBS $OPENMAX_LIBS";AC_DEFINE([HAVE_LIBOPENMAX], [1], [Define to 1 if you have the OpenMax library.])],
                    use_openmax=no;USE_OPENMAX=0;AC_MSG_RESULT($openmax_not_found))
elif test "$use_gles" = "yes" && test "$use_openmax" = "yes"; then
  AC_CHECK_HEADERS([OMX_Core.h],
                    USE_OPENMAX=1;[INCLUDES="$INCLUDES $OPENMAX_CFLAGS"; LIBS="$LIBS $OPENMAX_LIBS";AC_DEFINE([HAVE_LIBOPENMAX], [1], [Define to 1 if you have the OpenMax library.])],
                    AC_MSG_ERROR($openmax_not_found))
else
  AC_MSG_NOTICE($openmax_disabled)
  use_openmax=no
  USE_OPENMAX=0
fi

# additional internal players
case $add_players in
  *omxplayer*)
      XB_ADD_PLAYER([OMXPLAYER], [omxplayer])
      ;;
esac

# additional internal codecs
# remember to convert commas to spaces
for codecs in `echo $add_codecs | sed 's/,/ /g'`; do
  case $codecs in
    *amcodec*)
        AC_CHECK_HEADER([amcodec/codec_error.h],, AC_MSG_ERROR($missing_headers))
        XB_ADD_CODEC([LIBAMCODEC], [amcodec], [$codecs])
        ;;
    *libstagefright*)
        XB_ADD_CODEC([LIBSTAGEFRIGHT], [libstagefright], [$codecs])
        ;;
    *imxvpu*)
      AC_CHECK_HEADER([imx-mm/vpu/vpu_wrapper.h],, AC_MSG_ERROR($missing_headers))
      AC_CHECK_LIB([vpu], main, LIBS="$LIBS -lfslvpuwrap -lvpu -lg2d", AC_MSG_ERROR($missing_library))
      XB_ADD_CODEC([IMXVPU], [imxvpu], [$codecs])
      CXXFLAGS="$CXXFLAGS -Wno-psabi -DLINUX "
      CFLAGS="$CFLAGS -DLINUX"
      if test "$use_x11" = "no"; then
        CXXFLAGS="$CXXFLAGS -DEGL_API_FB"
        CFLAGS="$CFLAGS -DEGL_API_FB"
      fi
      ;;
    *)
  esac
done

# platform specific bin utilities
if test "$build_vendor" != "apple" ; then
  AC_CHECK_PROG(HAVE_GAWK,gawk,"yes","no",)
  if test "$HAVE_GAWK" = "no" ; then
    AC_MSG_ERROR($missing_program)
  fi
fi

if test "$use_arch" != "arm" ; then
  AC_CHECK_PROG(HAVE_CMAKE,cmake,"yes","no",)
  if test "$HAVE_CMAKE" = "no" ; then
    AC_MSG_ERROR($missing_program)
  fi
fi

AC_CHECK_PROG(HAVE_GPERF,gperf,"yes","no",)
if test "$HAVE_GPERF" = "no" ; then
  AC_MSG_ERROR($missing_program)
fi

AC_CHECK_PROG(HAVE_UNZIP,unzip,"yes","no",)
if test "$HAVE_UNZIP" = "no" ; then
  AC_MSG_ERROR($missing_program)
fi

AC_CHECK_PROG(HAVE_ZIP,zip,"yes","no",)
if test "$HAVE_ZIP" = "no" ; then
  AC_MSG_ERROR($missing_program)
fi

if test "$ARCH" = "i486-linux" || test "$ARCH" = "x86-freebsd"; then
  AC_CHECK_PROG(HAVE_NASM,nasm,"yes","no",)
  if test "$HAVE_NASM" = "no" ; then
    AC_MSG_ERROR($missing_program)
  fi
fi

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h \
  malloc.h memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h \
  strings.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h \
  sys/time.h sys/timeb.h sys/vfs.h termios.h unistd.h utime.h wchar.h wctype.h])
AC_CHECK_HEADERS([cdio/iso9660.h],,AC_MSG_ERROR([$missing_headers]))

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_C_BIGENDIAN

if test "$cross_compiling" = "yes"; then
  final_message="$final_message\n  Crosscomp.:\tYes"
  if [[ -d "$NATIVE_ROOT" ]]; then
    CFLAGS_FOR_BUILD="$CFLAGS_FOR_BUILD -I$NATIVE_ROOT/include"
    CXXFLAGS_FOR_BUILD="$CXXFLAGS_FOR_BUILD -I$NATIVE_ROOT/include"
    LDFLAGS_FOR_BUILD="$LDFLAGS_FOR_BUILD -L$NATIVE_ROOT/lib"
    if test "$host_vendor" != "apple" ; then
      LDFLAGS_FOR_BUILD="$LDFLAGS_FOR_BUILD -Wl,-rpath=$NATIVE_ROOT/lib"
    fi
    final_message="$final_message\n  Native Root:\t$NATIVE_ROOT"
  fi
else
  final_message="$final_message\n  Crosscomp.:\tNo"
fi

final_message="$final_message\n  target ARCH:\t$use_arch"
final_message="$final_message\n  target CPU:\t$use_cpu"

if test "$use_gles" = "yes"; then
  final_message="$final_message\n  OpenGLES:\tYes"
  USE_OPENGLES=1
  USE_OPENGL=0
else
  USE_OPENGLES=0
  if test "$use_gl" = "yes"; then
    final_message="$final_message\n  OpenGL:\tYes"
    USE_OPENGL=1
  else
    final_message="$final_message\n  OpenGL:\tNo (Very Slow)"
    SDL_DEFINES="$SDL_DEFINES -DHAS_SDL_2D"
    USE_OPENGL=0
  fi
fi

if test "$use_alsa" = "yes"; then
  USE_ALSA=1
  AC_DEFINE([USE_ALSA],[1],["Define to 1 if alsa is installed"])
  final_message="$final_message\n  ALSA:\t\tYes"
else
  USE_ALSA=0
  final_message="$final_message\n  ALSA:\t\tNo"
fi

if test "$use_dbus" = "yes"; then
  final_message="$final_message\n  DBUS:\t\tYes"
else
  final_message="$final_message\n  DBUS:\t\tNo"
fi

if test "x$use_vdpau" != "xno"; then
  final_message="$final_message\n  VDPAU:\tYes"
else
  final_message="$final_message\n  VDPAU:\tNo"
fi

if test "x$use_vaapi" != "xno"; then
  final_message="$final_message\n  VAAPI:\tYes"
else
  final_message="$final_message\n  VAAPI:\tNo"
fi

if test "x$use_vtbdecoder" != "xno"; then
  final_message="$final_message\n  VTBDecoder:\tYes"
else
  final_message="$final_message\n  VTBDecoder:\tNo"
fi

if test "$use_openmax" != "no"; then
  final_message="$final_message\n  OpenMax:\tYes"
else
  final_message="$final_message\n  OpenMax:\tNo"
fi

if test "$use_joystick" = "yes"; then
  final_message="$final_message\n  Joystick:\tYes"
  AC_DEFINE([HAS_SDL_JOYSTICK],[1],["Define to 1 if using SDL joystick"])
else
  final_message="$final_message\n  Joystick:\tNo"
fi

if test "$use_waveform" = "yes"; then
  final_message="$final_message\n  Waveform:\tYes"
else
  final_message="$final_message\n  Waveform:\tNo"
fi

if test "$use_spectrum" = "yes"; then
  final_message="$final_message\n  Spectrum:\tYes"
else
  final_message="$final_message\n  Spectrum:\tNo"
fi

if test "$use_goom" = "yes"; then
  final_message="$final_message\n  GOOM:\t\tYes"
else
  final_message="$final_message\n  GOOM:\t\tNo"
fi

if test "$use_rsxs" = "yes"; then
  final_message="$final_message\n  RSXS:\t\tYes"
else
  final_message="$final_message\n  RSXS:\t\tNo"
fi

if test "$use_fishbmc" = "yes"; then
  final_message="$final_message\n  FishBMC:\tYes"
else
  final_message="$final_message\n  FishBMC:\tNo"
fi

if test "$use_projectm" = "yes"; then
  final_message="$final_message\n  ProjectM:\tYes"
else
  final_message="$final_message\n  ProjectM:\tNo"
fi

if test "$use_touch_skin" = "yes"; then
  final_message="$final_message\n  Touch skin:\tYes"
else
  final_message="$final_message\n  Touch skin:\tNo"
fi

if test "$use_x11" = "yes"; then
  USE_X11=1
  final_message="$final_message\n  X11:\t\tYes"
else
  USE_X11=0
  final_message="$final_message\n  X11:\t\tNo"
fi

if test "$use_wayland" = "yes"; then
  final_message="$final_message\n  Wayland:\tYes"
  USE_WAYLAND=1
  USE_XKBCOMMON=1
else
  final_message="$final_message\n  Wayland:\tNo"
fi

if test "$use_libbluray" = "yes"; then
  final_message="$final_message\n  Bluray:\tYes"
else
  final_message="$final_message\n  Bluray:\tNo"
fi

# we don't compile with default upstream squish, so we use our own one
# even on non unified deps platforms
if test "$cross_compiling" != "yes"; then
  make -C ${abs_top_srcdir}/tools/depends/native/libsquish-native CXX=${CXX}
  export PKG_CONFIG_PATH="${abs_top_srcdir}/tools/depends/native/libsquish-native/squish-install/lib/pkgconfig:$PKG_CONFIG_PATH"
fi
PKG_CHECK_MODULES([SQUISH], [squish],
 [INCLUDES="$INCLUDES $SQUISH_CFLAGS" LIBS="$LIBS $SQUISH_LIBS"],
 AC_MSG_ERROR("squish not found"))

if test "x$use_texturepacker" != "xno"; then
  AC_PATH_PROG([TEXTUREPACKER], [TexturePacker], ["none"],
    [$PATH$PATH_SEPARATOR${abs_top_srcdir}/tools/depends/native/TexturePacker/bin])
  if test "$TEXTUREPACKER" = "none"; then
    make -C ${abs_top_srcdir}/tools/depends/native/TexturePacker
    TEXTUREPACKER="${abs_top_srcdir}/tools/depends/native/TexturePacker/bin/TexturePacker"
  fi
  if test -x "$TEXTUREPACKER"; then
    final_message="$final_message\n  TexturePacker:Yes"
    USE_TEXTUREPACKER=1
  else
    if test "x$use_texturepacker" = "xyes"; then
      AC_MSG_ERROR("Texturepacker not found")
    else
      AC_MSG_NOTICE("Texturepacker not found")
      USE_TEXTUREPACKER=0
      final_message="$final_message\n  TexturePacker:No"
    fi
  fi
else
  final_message="$final_message\n  TexturePacker:No"
  USE_TEXTUREPACKER=0
fi

if test "$use_mid" = "yes"; then
  final_message="$final_message\n  MID Support:\tYes"
  SDL_DEFINES="$SDL_DEFINES -DMID"
else
  final_message="$final_message\n  MID Support:\tNo"
fi

ORIGCC=$CC
ORIGCXX=$CXX
if test "x$use_ccache" != "xno"; then
  AC_PATH_PROG(CCACHE,ccache,none)
  if test "$ac_cv_path_CCACHE" = "none"; then
    if test "x$use_ccache" = "xyes"; then
      AC_MSG_ERROR([ccache not found.]);
    else
      AC_MSG_NOTICE([ccache not found. Falling back to default CC])
      final_message="$final_message\n  ccache:\tNo"
    fi
  else
    CC="$ac_cv_path_CCACHE $CC"
    CXX="$ac_cv_path_CCACHE $CXX"
    AC_MSG_NOTICE(enabling ccache)
    final_message="$final_message\n  ccache:\tYes"
  fi
else
  final_message="$final_message\n  ccache:\tNo"
fi

if test "$use_alsa" = "yes"; then
  final_message="$final_message\n  ALSA Support:\tYes"
else
  final_message="$final_message\n  ALSA Support:\tNo"
fi

if test "x$use_pulse" != "xno"; then
  XBMC_STANDALONE_SH_PULSE=tools/Linux/${APP_NAME_LC}-standalone.sh.pulse
  final_message="$final_message\n  PulseAudio:\tYes"
else
  XBMC_STANDALONE_SH_PULSE=/dev/null
  final_message="$final_message\n  PulseAudio:\tNo"
fi

# DVDCSS
if test "$use_dvdcss" = "yes"; then
  AC_MSG_NOTICE($dvdcss_enabled)
  final_message="$final_message\n  DVDCSS:\tYes"
  BUILD_DVDCSS=1
  SKIP_CONFIG_DVDCSS=0
  DVDREAD_CFLAGS="-D_XBMC -DHAVE_DVDCSS_DVDCSS_H"
else
  AC_MSG_NOTICE($dvdcss_disabled)
  final_message="$final_message\n  DVDCSS:\tNo"
  BUILD_DVDCSS=0
  SKIP_CONFIG_DVDCSS=1
  DVDREAD_CFLAGS="-D_XBMC -UHAVE_DVDCSS_DVDCSS_H"
fi
if test "$host_vendor" = "apple"; then
 DVDREAD_CFLAGS="$DVDREAD_CFLAGS -D__DARWIN__"
fi

# Google Test Framework
if test "$configure_gtest" = "yes"; then
  AC_MSG_NOTICE($gtest_enabled)
  final_message="$final_message\n  Google Test Framework Configured:\tYes"
  GTEST_CONFIGURED=1
  SKIP_CONFIG_GTEST=0
else
  AC_MSG_NOTICE($gtest_disabled)
  final_message="$final_message\n  Google Test Framework Configured:\tNo"
  GTEST_CONFIGURED=0
  SKIP_CONFIG_GTEST=1
fi

# Google Breakpad
if test "$use_breakpad" = "yes"; then
  USE_BREAKPAD=1
  PKG_CHECK_MODULES([BREAKPAD],    [breakpad-client],
    [INCLUDES="$INCLUDES $BREAKPAD_CFLAGS"; LIBS="$LIBS $BREAKPAD_LIBS"],
    AC_MSG_ERROR("breakpad-client not found"))
  AC_DEFINE([HAVE_BREAKPAD],[1],["Define to 1 if Google Breakpad is installed"])
  AC_MSG_NOTICE($breakpad_enabled)
  final_message="$final_message\n  Google Breakpad Configured:\tYes"
else
  AC_MSG_NOTICE($breakpad_disabled)
  final_message="$final_message\n  Google Breakpad Configured:\tNo"
  USE_BREAKPAD=0
fi

if test "$use_avahi" = "yes"; then
  final_message="$final_message\n  Avahi:\tYes"
else
  final_message="$final_message\n  Avahi:\tNo"
fi

if test "$use_mdnsembedded" != "no"; then
  final_message="$final_message\n  mDNSEmbedded:\tYes"
else
  final_message="$final_message\n  mDNSEmbedded:\tNo"
fi

if test "$use_nonfree" = "yes"; then
  final_message="$final_message\n  Non-free:\tYes"
  HAVE_XBMC_NONFREE=1
  AC_DEFINE([HAVE_XBMC_NONFREE], [1], [Define to 1 to enable non-free components.])
else
  HAVE_XBMC_NONFREE=0
  final_message="$final_message\n  Non-free:\tNo"
fi

if test "$use_asap" = "yes"; then
  AC_CHECK_PROGS(HAVE_GDC,gdc-4.4 gdc-4.3 gdc,"no")
  if test "$HAVE_GDC" = "no"; then
    AC_MSG_ERROR($missing_program);
  fi
  AC_CHECK_PROG(HAVE_FPC,fpc,"yes","no")
  if test "$HAVE_FPC" = "no"; then
    AC_MSG_ERROR($missing_program);
  fi
  USE_ASAP_CODEC=1
  AC_DEFINE([USE_ASAP_CODEC], [1], [Define to 1 to enable ASAP codec.])
  final_message="$final_message\n  ASAP Codec:\tYes"
else
  USE_ASAP_CODEC=0
  final_message="$final_message\n  ASAP Codec:\tNo"
fi

if test "$use_mysql" = "yes"; then
  final_message="$final_message\n  MySQL:\tYes"
  USE_MYSQL=1
else
  final_message="$final_message\n  MySQL:\tNo"
  USE_MYSQL=0
fi
if test "$use_webserver" = "yes"; then
  final_message="$final_message\n  Webserver:\tYes"
  USE_WEB_SERVER=1
else
  final_message="$final_message\n  Webserver:\tNo"
  USE_WEB_SERVER=0
fi

if test "$use_libssh" != "no"; then
  final_message="$final_message\n  libssh support:\tYes"
else
  final_message="$final_message\n  libssh support:\tNo"
fi

if test "$use_librtmp" != "no"; then
  final_message="$final_message\n  libRTMP support:\tYes"
else
  final_message="$final_message\n  libRTMP support:\tNo"
fi

if test "x$use_samba" != "xno"; then
  final_message="$final_message\n  libsmbclient support:\tYes"
else
  final_message="$final_message\n  libsmbclient support:\tNo"
fi

if test "$use_libnfs" != "no"; then
  final_message="$final_message\n  libnfs client support:Yes"
else
  final_message="$final_message\n  libnfs client support:No"
fi

if test "$use_airplay" != "no"; then
  final_message="$final_message\n  AirPlay support:\tYes"
else
  final_message="$final_message\n  AirPLay support:\tNo"
fi

if test "x$use_airtunes" != "xno"; then
  final_message="$final_message\n  AirTunes support (libshairplay):\tYes"  
else
  final_message="$final_message\n  AirTunes support:\tNo"
fi

if test "x$use_upnp" != "xno"; then
  final_message="$final_message\n  UPnP support:\t\tYes"
  USE_UPNP=1
  UPNP_DEFINES='-DNPT_CONFIG_ENABLE_LOGGING -DPLT_HTTP_DEFAULT_USER_AGENT="\"UPnP/1.0 DLNADOC/1.50 Kodi\"" -DPLT_HTTP_DEFAULT_SERVER="\"UPnP/1.0 DLNADOC/1.50 Kodi\""'
  AC_DEFINE([USE_UPNP], [1], [Define to 1 to enable UPnP support.])
else
  USE_UPNP=0
  final_message="$final_message\n  UPnP support:\t\tNo"
fi

if test "$use_optical_drive" = "yes"; then
  final_message="$final_message\n  Optical drive:\tYes"
else
  final_message="$final_message\n  Optical drive:\tNo"
fi

if test "x$use_libudev" != "xno"; then
  final_message="$final_message\n  libudev support:\tYes"
else
  final_message="$final_message\n  libudev support:\tNo"
fi

if test "x$use_libusb" != "xno"; then
  final_message="$final_message\n  libusb support:\tYes"
else
  final_message="$final_message\n  libusb support:\tNo"
fi

if test "x$use_libcec" != "xno"; then
  final_message="$final_message\n  libcec support:\tYes"
else
  final_message="$final_message\n  libcec support:\tNo"
fi

if test "x$use_libcap" != "xno"; then
  final_message="$final_message\n  libcap support:\tYes"
else
  final_message="$final_message\n  libcap support:\tNo"
fi

if test "x$add_players" != "xno"; then
  final_message="$final_message\n  additional players:\tYes, $add_players"
else
  final_message="$final_message\n  additional players:\tNo"
fi

if test "x$add_codecs" != "xno"; then
  final_message="$final_message\n  additional codecs:\tYes, $add_codecs"
else
  final_message="$final_message\n  additional codecs:\tNo"
fi

### External libraries messages

if test "$host_vendor" = "apple" ; then
  # built internal but referenced as external, we link directly to them.
  # this MUST be the last thing before OUTPUT_FILES as they do not
  # exist until after we build FFMpeg.
  LIBS="$LIBS -L\$(abs_top_srcdir)/lib/ffmpeg/libavcodec -lavcodec"
  LIBS="$LIBS -L\$(abs_top_srcdir)/lib/ffmpeg/libavfilter -lavfilter"
  LIBS="$LIBS -L\$(abs_top_srcdir)/lib/ffmpeg/libswresample -lswresample"
  LIBS="$LIBS -L\$(abs_top_srcdir)/lib/ffmpeg/libavformat -lavformat"
  LIBS="$LIBS -L\$(abs_top_srcdir)/lib/ffmpeg/libavutil -lavutil"
  LIBS="$LIBS -L\$(abs_top_srcdir)/lib/ffmpeg/libpostproc -lpostproc"
  LIBS="$LIBS -L\$(abs_top_srcdir)/lib/ffmpeg/libswscale -lswscale"
fi

OUTPUT_FILES="Makefile \
    Makefile.include \
    addons/skin.confluence/media/Makefile \
    xbmc/Makefile \
    xbmc/cdrip/Makefile \
    xbmc/cores/Makefile \
    xbmc/cores/VideoRenderers/Makefile \
    xbmc/cores/dvdplayer/Makefile \
    lib/libdvd/Makefile \
    xbmc/cores/DllLoader/Makefile \
    xbmc/cores/DllLoader/exports/Makefile \
    xbmc/cores/dvdplayer/DVDCodecs/Makefile \
    xbmc/cores/dvdplayer/DVDCodecs/Audio/Makefile \
    xbmc/cores/dvdplayer/DVDCodecs/Overlay/Makefile \
    xbmc/cores/dvdplayer/DVDCodecs/Video/Makefile \
    xbmc/cores/dvdplayer/DVDDemuxers/Makefile \
    xbmc/cores/dvdplayer/DVDSubtitles/Makefile \
    xbmc/cores/AudioEngine/Makefile \
    xbmc/cores/paplayer/Makefile \
    xbmc/cores/omxplayer/Makefile \
    xbmc/cores/playercorefactory/Makefile \
    xbmc/music/karaoke/Makefile \
    xbmc/osx/Makefile \
    xbmc/guilib/Makefile \
    xbmc/input/linux/Makefile \
    xbmc/interfaces/Makefile \
    xbmc/network/Makefile \
    xbmc/network/httprequesthandler/python/Makefile \
    xbmc/network/upnp/Makefile \
    lib/libexif/Makefile \
    lib/libhdhomerun/Makefile \
    lib/cximage-6.0/Makefile \
    lib/libUPnP/Makefile \
    xbmc/DllPaths_generated.h \
    xbmc/DllPaths_generated_android.h \
    xbmc/freebsd/Makefile \
    xbmc/linux/Makefile \
    xbmc/filesystem/Makefile \
    xbmc/screensavers/rsxs-0.9/xbmc/Makefile \
    xbmc/visualizations/XBMCProjectM/Makefile \
    xbmc/visualizations/Goom/Makefile \
    xbmc/visualizations/OpenGLSpectrum/Makefile \
    xbmc/visualizations/fishBMC/Makefile \
    xbmc/visualizations/WaveForm/Makefile \
    xbmc/windowing/Makefile \
    xbmc/windowing/egl/Makefile \
    lib/addons/library.xbmc.addon/Makefile \
    lib/addons/library.xbmc.codec/Makefile \
    lib/addons/library.kodi.guilib/Makefile \
    lib/addons/library.xbmc.pvr/Makefile \
    xbmc/visualizations/EGLHelpers/Makefile \
    tools/Linux/${APP_NAME_LC}.sh \
    tools/Linux/${APP_NAME_LC}-standalone.sh \
    tools/Linux/${APP_NAME_LC}-xsession.desktop \
    tools/EventClients/Makefile \
    tools/EventClients/Clients/OSXRemote/Makefile \
    xbmc/peripherals/bus/Makefile \
    xbmc/peripherals/devices/Makefile \
    xbmc/android/activity/Makefile \
    xbmc/android/loader/Makefile \
    xbmc/android/jni/Makefile \
    xbmc/utils/Makefile \
    xbmc/main/Makefile \
    tools/darwin/Configurations/App.xcconfig \
    tools/darwin/Configurations/Common.xcconfig \
    tools/darwin/packaging/ios/mkdeb-ios.sh \
    tools/darwin/packaging/atv2/mkdeb-atv2.sh \
    tools/darwin/packaging/osx/mkdmg-osx.sh \
    tools/darwin/packaging/migrate_to_kodi_ios.sh \
    tools/darwin/packaging/seatbeltunlock/mkdeb-seatbeltunlock.sh \
    xbmc/osx/Info.plist \
    xbmc/osx/ios/XBMCIOS-Info.plist \
    xbmc/osx/atv2/XBMCATV2-Info.plist \
    project/cmake/kodi-config.cmake \
    project/cmake/xbmc-config.cmake \
    tools/android/packaging/xbmc/AndroidManifest.xml \
    tools/android/packaging/Makefile \
    tools/android/packaging/xbmc/src/org/xbmc/kodi/Splash.java \
    tools/android/packaging/xbmc/src/org/xbmc/kodi/Main.java \
    tools/android/packaging/xbmc/src/org/xbmc/kodi/XBMCBroadcastReceiver.java \
    tools/android/packaging/xbmc/src/org/xbmc/kodi/XBMCOnFrameAvailableListener.java \
    tools/android/packaging/xbmc/src/org/xbmc/kodi/XBMCSettingsContentObserver.java \
    tools/android/packaging/xbmc/src/org/xbmc/kodi/XBMCOnAudioFocusChangeListener.java \
    tools/android/packaging/xbmc/strings.xml \
    addons/xbmc.addon/addon.xml"

if test "$use_wayland" = "yes"; then
OUTPUT_FILES="$OUTPUT_FILES xbmc/windowing/tests/wayland/Makefile"
fi

if test "$use_touch_skin" = "yes"; then
OUTPUT_FILES="$OUTPUT_FILES addons/skin.re-touched/media/Makefile"
fi

if test "$use_codec_libstagefright" = "yes"; then
OUTPUT_FILES="$OUTPUT_FILES xbmc/cores/dvdplayer/DVDCodecs/Video/libstagefrightICS/Makefile"
fi

if test "$use_sse4" = "yes"; then
OUTPUT_FILES="$OUTPUT_FILES xbmc/linux/sse4/Makefile"
fi

OUTPUT_FILES="$OUTPUT_FILES \
  xbmc/interfaces/python/Makefile \
  xbmc/interfaces/python/test/Makefile"

# needed for kodi-config.cmake
if echo "$libdir" | grep -q '${exec_prefix}'; then
  if test "${prefix}" = "NONE" || test "${prefix}" = ""; then
    prefix="/usr/local"
  fi
  if test "${exec_prefix}" = "NONE" || test "${exec_prefix}" = ""; then
    exec_prefix=${prefix}
  fi
fi
eval "APP_PREFIX=${prefix}"
eval "APP_LIB_DIR=${libdir}/${APP_NAME_LC}"
eval "APP_INCLUDE_DIR=${includedir}/${APP_NAME_LC}"
AC_SUBST(APP_PREFIX)
AC_SUBST(APP_LIB_DIR)
AC_SUBST(APP_INCLUDE_DIR)

# Line below is used so we can use AM_INIT_AUTOMAKE. The corresponding
# .dummy.am does nothing.
AC_CONFIG_FILES([.dummy])

AC_CONFIG_FILES([${OUTPUT_FILES}])
OUTPUT_FILES="$OUTPUT_FILES \
  .dummy"
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(INCLUDES)
AC_SUBST(LDFLAGS)
AC_SUBST(CXX_FOR_BUILD)
AC_SUBST(CC_FOR_BUILD)
AC_SUBST(CFLAGS_FOR_BUILD)
AC_SUBST(CXXFLAGS_FOR_BUILD)
AC_SUBST(LDFLAGS_FOR_BUILD)
AC_SUBST(SDL_DEFINES)
AC_SUBST(BUILD_DVDCSS)
AC_SUBST(DISABLE_WAVEFORM)
AC_SUBST(DISABLE_SPECTRUM)
AC_SUBST(DISABLE_GOOM)
AC_SUBST(DISABLE_RSXS)
AC_SUBST(DISABLE_FISHBMC)
AC_SUBST(DISABLE_PROJECTM)
AC_SUBST(FFMPEG_LIBDIR)
AC_SUBST(USE_STATIC_FFMPEG)
AC_SUBST(GNUTLS_ALL_LIBS)
AC_SUBST(VORBISENC_ALL_LIBS)
AC_SUBST(USE_TOUCH_SKIN)
AC_SUBST(USE_LIBAV_HACKS)
AC_SUBST(PYTHON_VERSION)
AC_SUBST(OUTPUT_FILES)
AC_SUBST(HAVE_XBMC_NONFREE)
AC_SUBST(USE_ASAP_CODEC)
AC_SUBST(LIBCURL_BASENAME)
AC_SUBST(LIBVORBISFILE_BASENAME)
AC_SUBST(LIBOGG_BASENAME)
AC_SUBST(LIBVORBIS_BASENAME)
AC_SUBST(LIBASS_BASENAME)
AC_SUBST(LIBMEPG2_BASENAME)
AC_SUBST_FILE(XBMC_STANDALONE_SH_PULSE)
AC_SUBST(USE_OPENGL)
AC_SUBST(USE_OPENGLES)
AC_SUBST(USE_VDPAU)
AC_SUBST(USE_VAAPI)
AC_SUBST(USE_LIBSMBCLIENT)
AC_SUBST(USE_LIBNFS)
AC_SUBST(USE_AIRPLAY)
AC_SUBST(USE_OPENMAX)
AC_SUBST(USE_PULSE)
AC_SUBST(HAVE_LIBPULSE)
AC_SUBST(USE_ALSA)
AC_SUBST(USE_TEXTUREPACKER)
AC_SUBST(TEXTUREPACKER)
AC_SUBST(USE_AIRTUNES)
AC_SUBST(USE_LIBUDEV)
AC_SUBST(USE_LIBUSB)
AC_SUBST(USE_LIBCEC)
AC_SUBST(USE_MYSQL)
AC_SUBST(USE_WAYLAND)
AC_SUBST(USE_WEB_SERVER)
AC_SUBST(USE_UPNP)
AC_SUBST(USE_XKBCOMMON)
AC_SUBST(USE_OMXLIB)
AC_SUBST(USE_ANDROID)
AC_SUBST(GTEST_CONFIGURED)
AC_SUBST(USE_DOXYGEN)
AC_SUBST(UPNP_DEFINES)
AC_SUBST(USE_SSE4)
AC_SUBST(USE_MMAL)
AC_SUBST(USE_X11)
AC_SUBST(USE_BREAKPAD)

# pushd and popd are not available in other shells besides bash, so implement
# our own pushd/popd functions
XB_DIRSTACK="$PWD"
xb_pushd()
{
  local dirname="$1"
  if [[ -d "$dirname" ]] && [[ -x "$dirname" ]]; then
    cd "$dirname"
    XB_DIRSTACK="$dirname ${XB_DIRSTACK:-$PWD}"
    return 0
  else
    AC_MSG_ERROR(xb_pushd: unable to change to $dirname)
  fi
}
xb_popd()
{
  if [[ -n "$XB_DIRSTACK" ]]; then
    XB_DIRSTACK="${XB_DIRSTACK#* }"
    cd "${XB_DIRSTACK%% *}"
    return 0
  else
    AC_MSG_ERROR(xb_popd: unable to go back to previous directory)
  fi
}

# Function to run the configure scripts in our submodules
# Consists of three paramaters, the path to the submodule, the configure command
# with appropriate arguments, and a third parameter set to 1 if we are to skip
# running the script, anything else if not.
AC_DEFUN([XB_CONFIG_MODULE],[
AC_CONFIG_COMMANDS_POST([
if [[ $3 != "1" ]]; then
    if [[ -d $1 ]]; then
      xb_pushd $1
      $2
      if [[ $? -ne 0 ]]; then
        xb_popd
        AC_MSG_ERROR([[Submodule $1 failed to configure]])
      else
        xb_popd
      fi
    else
      AC_MSG_ERROR([[Submodule $1 does not exist]])
    fi
else
    AC_MSG_NOTICE([[Skipping configuration of submodule $1.]])
fi
])
])

XB_CONFIG_MODULE([lib/libdvd/libdvdcss], [
  ./configure \
    CC="$CC" \
    CXX="$CXX" \
    CFLAGS="$CFLAGS $DROID_DVDLIB_SEEK64" \
    --prefix="${prefix}" --includedir="${includedir}" --libdir="${libdir}" --datadir="${datadir}" \
    --host=$host_alias \
    --build=$build_alias \
    --target=$target_alias \      
    --disable-doc \
    --enable-static \
    --with-pic
], [$SKIP_CONFIG_DVDCSS])

XB_CONFIG_MODULE([lib/libdvd/libdvdread], [
  ./configure2 \
    --extra-cflags="$CFLAGS $DVDREAD_CFLAGS -I`pwd`/../libdvdcss/src $DROID_DVDLIB_SEEK64" \
    --prefix="${prefix}" --includedir="${includedir}" --libdir="${libdir}" --datadir="${datadir}" \
    --host=$host_alias \
    --build=$build_alias \
    --target=$target_alias \      
    --enable-static \
    --disable-shared \
    --disable-strip \
    --disable-opts \
    --cc="$CC" &&
  mkdir -p `pwd`/../includes/dvdread
  cp `pwd`/../libdvdread/src/*.h `pwd`/../includes/dvdread
  cp `pwd`/../libdvdread/src/dvdread/*.h `pwd`/../includes/dvdread
], [0])

XB_CONFIG_MODULE([lib/libdvd/libdvdnav], [
  ./configure2 \
    --extra-cflags="$CFLAGS $DVDREAD_CFLAGS -I`pwd`/../includes $DROID_DVDLIB_SEEK64" \
    --extra-ldflags="-L`pwd`/../libdvdread/obj" \
    --with-dvdread-config="`pwd`/../dvdread-config" \
    --prefix="${prefix}" --includedir="${includedir}" --libdir="${libdir}" --datadir="${datadir}" \
    --host=$host_alias \
    --build=$build_alias \
    --target=$target_alias \      
    --enable-static \
    --disable-shared \
    --cc="$CC"
], [0])

XB_CONFIG_MODULE([xbmc/visualizations/XBMCProjectM/libprojectM],[
  set -x
  rm -f CMakeCache.txt &&                              \
  CC="$ORIGCC" CXX="$ORIGCXX" LDFLAGS="$LDFLAGS" cmake \
    -DCMAKE_BUILD_TYPE=None -DUSE_FTGL:BOOL=OFF        \
    -DCMAKE_C_FLAGS:STRING="${CPPFLAGS} ${CFLAGS}"     \
    -DCMAKE_CXX_FLAGS:STRING="${CPPFLAGS} ${CXXFLAGS}" \
    -DCMAKE_INSTALL_PREFIX="${prefix}"                 \
    -DCMAKE_INSTALL_LIBDIR:PATH="${libdir}"            \
    -DINCLUDE_INSTALL_DIR:PATH="${includedir}"         \
    -DLIB_INSTALL_DIR:PATH="${libdir}"                 \
    -DSYSCONF_INSTALL_DIR:PATH="${sysconfdir}"         \
    -DSHARE_INSTALL_PREFIX:PATH="${datadir}" . &&
  if test "$host_vendor" = "apple" ; then
    # cmake has hardcoded paths to macports which bork our darwin depends cross/ppc, remove them
    sed -ie "s|-L/opt/local/lib| |" CMakeFiles/projectM.dir/link.txt
    sed -ie "s|-L/opt/local/lib| |" CMakeFiles/projectM.dir/flags.make
    sed -ie "s|-I/opt/local/include| |" CMakeFiles/projectM.dir/flags.make
  fi
  set +x
], [$DISABLE_PROJECTM])

XB_CONFIG_MODULE([xbmc/visualizations/Goom/goom2k4-0],[
  ./configure  \
    CFLAGS="$CFLAGS" \ 
    CXXFLAGS="$CXXFLAGS" \
    --prefix="${prefix}" --includedir="${includedir}" --libdir="${libdir}" --datadir="${datadir}" \
    --host=$host_alias \
    --build=$build_alias \
    --target=$target_alias \      
    --disable-shared \
    --enable-static \
    --with-pic
], [$DISABLE_GOOM])

XB_CONFIG_MODULE([xbmc/screensavers/rsxs-0.9/], [
  if test "$host_vendor" = "apple"; then
    # clang treats inlines different
    case $CC in
      *clang*)
        TEMPCFLAGS="$CFLAGS";;
      *)
        TEMPCFLAGS="${CFLAGS} -fgnu89-inline";;
    esac
  else
    TEMPCFLAGS="$CFLAGS";
  fi
  ./configure \
    CC="$CC" \
    CXX="$CXX" \
    CFLAGS="$TEMPCFLAGS" \ 
    CXXFLAGS="$CXXFLAGS" \
    `if test "$host_vendor" = "apple"; then echo --with-png=${prefix} --x-includes=/usr/X11/include --x-libraries=/usr/X11/lib; fi` \
    --prefix="${prefix}" --includedir="${includedir}" --libdir="${libdir}" --datadir="${datadir}" \
    --host=$host_alias \
    --build=$build_alias \
    --target=$target_alias \      
    --without-xscreensaver \
    --disable-sound \
    --disable-cyclone \
    --disable-fieldlines \
    --disable-flocks \
    --disable-flux \
    --disable-helios \
    --disable-hyperspace \
    --disable-lattice \
    --disable-skyrocket
  if echo "$ARCH" | grep -q freebsd ; then
    sed -i.back "s;\(STDBOOL_H = \)stdbool.h;\1;" lib/Makefile
  fi
], [$DISABLE_RSXS])

XB_CONFIG_MODULE([lib/cpluff], [
  ./configure --disable-nls \
    --prefix="${prefix}" --includedir="${includedir}" --libdir="${libdir}" --datadir="${datadir}" \
    --host=$host_alias \
    --build=$build_alias \
    --target=$target_alias CFLAGS="$CFLAGS" CC="$CC" CXX="$CXX" LDFLAGS="$LDFLAGS" LIBS=""
    #LDFLAGS="$LDFLAGS -Wl,-read_only_relocs,suppress"    
], [0])

XB_CONFIG_MODULE([lib/gtest], [
  ./configure \
    CC="$CC" \
    CXX="$CXX" \
    CFLAGS="$CFLAGS" \
    CXXFLAGS="$CXXFLAGS" \
    --prefix="${prefix}" --includedir="${includedir}" --libdir="${libdir}" --datadir="${datadir}" \
    --host=$host_alias \
    --build=$build_alias \
    --target=$target_alias \
    --disable-shared \
    --enable-static \
    --with-pthreads
], [$SKIP_CONFIG_GTEST])

AC_OUTPUT

final_message="$final_message\n  prefix:\t$prefix\n$dashes"
echo -e "$final_message\n"
