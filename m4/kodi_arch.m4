AC_DEFUN([KODI_SETUP_ARCH_PROFILE],[

# host detection and setup
case $host in
  i*86*-linux-gnu*|i*86*-*-linux-uclibc*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX'
     ;;
  x86_64-*-linux-gnu*|x86_64-*-linux-uclibc*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX'
     ;;
  i386-*-freebsd*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_FREEBSD -D_LINUX'
     ;;
  amd64-*-freebsd*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_FREEBSD -D_LINUX'
     ;;
  arm-apple-darwin*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_IOS -D_LINUX'
     ;;
  *86*-apple-darwin*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX -D_LINUX'
     ;;
  powerpc-apple-darwin*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX -D_LINUX'
     ;;
  powerpc-*-linux-gnu*|powerpc-*-linux-uclibc*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX -D_POWERPC'
     ;;
  powerpc64-*-linux-gnu*|powerpc64-*-linux-uclibc*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX -D_POWERPC64'
     ;;
  arm*-*-linux-gnu*|arm*-*-linux-uclibc*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX'
     ;;
  *-*linux-android*)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX -DTARGET_ANDROID'
     ;;
  *)
     AC_MSG_ERROR([unsupported host ($host)])
esac

if test "$target_platform" = "target_android" ; then
  ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX -DTARGET_ANDROID'
fi

case $use_platform in
  raspberry-pi)
     ARCH_DEFINES='-DTARGET_POSIX -DTARGET_LINUX -D_LINUX -D_ARMEL -DTARGET_RASPBERRY_PI'
     ;;
esac

AC_SUBST([ARCH_DEFINES])
])
