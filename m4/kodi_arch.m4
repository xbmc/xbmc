AC_DEFUN([KODI_SETUP_ARCH_PROFILE],[

# host detection and setup
unset ARCH_DEFINES
if test -z "${target_platform}" ; then
  case $host in
    *-linux-gnu*|*-*-linux-uclibc*|*-*-linux-gnu*|*-*-linux-uclibc*)
      target_platform='target_linux'
      ;;
    *-*-freebsd*)
      target_platform='target_freebsd'
      ;;
    arm-apple-darwin*)
      target_platform='target_darwin_ios'
      ;;
    *86*-apple-darwin*|powerpc-apple-darwin*)
      target_platform='target_darwin_osx'
      ;;
    *-*linux-android*)
      target_platform='target_android'
      ;;
    *)
      AC_MSG_ERROR([unsupported host ($host)])
      ;;
  esac
fi

case ${target_platform} in
  target_linux )
    ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX'
    case $host in
      powerpc-* )
        ARCH_DEFINES="$ARCH_DEFINES -D_POWERPC"
        ;;
      powerpc64-* )
        ARCH_DEFINES="$ARCH_DEFINES -D_POWERPC64"
        ;;
    esac
    ;;
  target_darwin_osx )
    ARCH_DEFINES='-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX -D_LINUX'
    # TODO: replace with xbmc/darwin
    INCLUDES="-I\$(abs_top_srcdir)/xbmc/osx $INCLUDES"
    ;;
  target_darwin_ios )
    ARCH_DEFINES='-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_IOS -D_LINUX'
    # TODO: replace with xbmc/darwin
    INCLUDES="-I\$(abs_top_srcdir)/xbmc/osx $INCLUDES"
    ;;
  target_android )
    ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX -DTARGET_ANDROID'
    INCLUDES="-I\$(abs_top_srcdir)/xbmc/android $INCLUDES"
    ;;
  target_raspberry_pi )
	ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX -D_ARMEL -DTARGET_RASPBERRY_PI'
    ;;
  target_freebsd )
    ARCH_DEFINES='-DTARGET_POSIX -DTARGET_FREEBSD -D_LINUX'
    INCLUDES="-I\$(abs_top_srcdir)/xbmc/freebsd $INCLUDES"
    ;;
  * )
    AC_MSG_ERROR([unsupported target platform (${target_platform})])
    ;;
esac

# Currently all POSIX platforms use xbmc/linux. TODO: move to Linux only
INCLUDES="-I\$(abs_top_srcdir)/xbmc/linux $INCLUDES" 

# All supported by "configure" platforms are POSIX platforms
INCLUDES="-I\$(abs_top_srcdir)/xbmc/posix $INCLUDES" 

# Add top source directory for all builds so config.h can be used
INCLUDES="-I\$(abs_top_srcdir) $INCLUDES" 

AC_SUBST([ARCH_DEFINES])
])
