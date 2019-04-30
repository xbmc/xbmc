AC_PREREQ(2.59)
AC_INIT([xbmc-depends], [2.00], [http://trac.xbmc.org])
:${CFLAGS=""}
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([target/config.site native/config.site.native Makefile.include target/Toolchain.cmake
                 target/config-binaddons.site target/Toolchain_binaddons.cmake target/cross-file.meson])
AC_CANONICAL_HOST

m4_include([m4/xbmc_arch.m4])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])

# check for not same cpu value
AC_DEFUN([MC_CHECK_NOT_CPU],
[
  AC_MSG_CHECKING([for $2])
  case $1 in
    $2*)
      AC_MSG_ERROR(error in configure of --with-cpu=$1)
      ;;
    *)
      AC_MSG_RESULT([$1 is not $2])
  esac
])

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
  [enable debugging information (default is yes)])],
  [use_debug=$enableval],
  [use_debug=yes])

AC_ARG_ENABLE([ccache],
  [AS_HELP_STRING([--disable-ccache],
  [disable ccache])],
  [use_ccache=no],
  [use_ccache=yes])

AC_ARG_WITH([toolchain],
  [AS_HELP_STRING([--with-toolchain],
  [specify path to toolchain. Required for android. Defaults to xcode root for darwin, /usr for linux])],
  [use_toolchain=$withval])

AC_ARG_WITH([platform],
  [AS_HELP_STRING([--with-platform],
  [target platform [auto]])],
  [use_platform=$withval],
  [use_platform=auto])

AC_ARG_WITH([firmware],
  [AS_HELP_STRING([--with-firmware],
  [platform specific firmware []])],
  [use_firmware=$withval])

AC_ARG_WITH([tarballs],
  [AS_HELP_STRING([--with-tarballs],
  [path where tarballs will be saved [prefix/xbmc-tarballs]])],
  [use_tarballs=$withval])

AC_ARG_WITH([cpu],
  [AS_HELP_STRING([--with-cpu],
  [optional. specify target cpu. guessed if not specified])],
  [use_cpu=$withval],
  [use_cpu=auto])

AC_ARG_WITH([ndk-path],
  [AS_HELP_STRING([--with-ndk-path],
  [specify path to ndk (required for android only)])],
  [use_ndk_path=$withval])

AC_ARG_WITH([sdk-path],
  [AS_HELP_STRING([--with-sdk-path],
  [specify path to sdk (required for android only)])],
  [use_sdk_path=$withval])

AC_ARG_WITH([sdk],
  [AS_HELP_STRING([--with-sdk],
  [specify sdk platform version.])],
  [use_sdk=$withval])

AC_ARG_WITH([ndk-api],
  [AS_HELP_STRING([--with-ndk-api],
  [specify ndk level (optional for android), default is 21.])],
  [use_ndk_api=$withval],
  [use_ndk_api=21])

AC_ARG_ENABLE([gplv3],
  [AS_HELP_STRING([--enable-gplv3],
  [enable gplv3 components. default is yes])],
  [use_gplv3=$enableval],
  [use_gplv3=yes])

AC_ARG_WITH([rendersystem],
  [AS_HELP_STRING([--with-rendersystem],
  [render system to use])],
  [app_rendersystem=$withval])

AC_ARG_WITH([target-cflags],
  [AS_HELP_STRING([--with-target-cflags],
  [C compiler flags (target)])],
  [target_cflags=$withval])

AC_ARG_WITH([target-cxxflags],
  [AS_HELP_STRING([--with-target-cxxflags],
  [C++ compiler flags (target)])],
  [target_cxxflags=$withval],
  [target_cxxflags=$target_cflags])

AC_ARG_WITH([target-ldflags],
  [AS_HELP_STRING([--with-target-ldflags],
  [linker flags. Use e.g. for -l<lib> (target)])],
  [target_ldflags=$withval])

AC_ARG_WITH([ffmpeg-options],
  [AS_HELP_STRING([--with-ffmpeg-options],
  [FFmpeg configure options, e.g. --enable-vaapi (target)])],
  [ffmpeg_options=$withval],
  [ffmpeg_options=default])


OLD_PATH="${PATH}"
if test -n "$use_toolchain"; then
  PATH="${use_toolchain}/bin:${use_toolchain}/usr/bin:$PATH"
  case $host in
    arm*-*linux-android*|aarch64*-*linux-android*)
      COMPILER_LIST="clang++ gcc g++"
    ;;
    *)
      COMPILER_LIST=""
    ;;
  esac
fi
AC_PROG_CXX([$COMPILER_LIST])
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])
c11_flags=$(echo "$CFLAGS" | sed 's/-O@<:@123@:>@//g;s/-g //g;s/ //g')
cxx11_flags=$(echo "$CXXFLAGS" | sed 's/-O@<:@123@:>@//g;s/-g //g;s/ //g')
PATH="${OLD_PATH}"

if test "$use_ccache" = "yes"; then
  AC_CHECK_PROG(HAVE_CCACHE,ccache,"yes","no",)
  if test "x$HAVE_CCACHE" = "xno" ; then
    use_ccache=no
  fi
fi

AC_PATH_PROG(CURL,curl,"no")
if test "x$CURL" = "xno" ; then
  AC_MSG_ERROR("Missing program: curl")
fi

AC_PATH_PROG(TAR,tar,"no")
if test "x$TAR" = "xno" ; then
  AC_MSG_ERROR("Missing program: tar")
fi

if test "$use_debug" = "yes"; then
  build_type="debug"
else
  build_type="release"
fi

use_host=$host_alias

cross_compiling="yes"
if test "x$host" = "x$build"; then
  use_host=$build_cpu-$build_os
  cross_compiling="no"
fi

deps_dir=$use_host-$build_type
tool_dir=$build_cpu-$build_os-native
cross_compiling="yes"

platform_cc=gcc
platform_cxx=g++

case $build in
  arm*-*-linux-gnu*|arm*-*-linux-uclibc*)
    build_os="linux"
  ;;
  *i686*-linux-gnu*|i*86*-*-linux-uclibc*)
    build_os="linux"
  ;;
  x86_64*-linux-gnu*|x86_64-*-linux-uclibc*)
    build_os="linux"
  ;;
  *darwin*)
    build_os="osx"
    native_platform_min_version=-mmacosx-version-min=`sw_vers | grep ProductVersion | sed -E "s/.*:.*(10\..*)\.?.*/\1/"`
  ;;
  *)
    AC_MSG_ERROR(unsupported native build platform: $build)
esac


case $host in
  *-*linux-android*)
    platform_cc=clang
    platform_cxx=clang++
    deps_dir="$use_host-$use_ndk_api-$build_type"
    platform_cflags="-DANDROID -D__ANDROID_API__=$use_ndk_api -fexceptions -funwind-tables -fstack-protector-strong -no-canonical-prefixes -fPIC -DPIC"
    optimize_flags="-Os"
    platform_ldflags="-Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libatomic.a -L$prefix/$deps_dir/lib/android-$use_ndk_api"

    case $host in
      arm*-*linux-android*)
        if test "x$use_cpu" = "xauto"; then
          use_cpu="armeabi-v7a"
        fi
        if test "x$use_cpu" = "xarmeabi-v7a"; then
          platform_cflags+=" -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon"
        fi
        platform_ldflags+=" -Wl,--exclude-libs,libunwind.a"
      ;;
      aarch64*-*linux-android*)
        if test "x$use_cpu" = "xauto"; then
          use_cpu="arm64-v8a"
        fi
        if test "x$use_cpu" = "xarm64-v8a"; then
          platform_cflags+=" -march=armv8-a -mtune=cortex-a53"
        fi
      ;;
      i*86*-linux-android*)
        if test "x$use_cpu" = "xauto"; then
          use_cpu=$host_cpu
        fi
      ;;
      *)
        AC_MSG_ERROR(unsupported host ($use_host))
      esac

    platform_cxxflags="$platform_cflags -frtti"
    platform_includes="-I$prefix/$deps_dir/include/android-$use_ndk_api"
    platform_os="android"
    #android builds are always cross
    cross_compiling="yes"
  ;;
  arm*-*-linux-gnu*|arm*-*-linux-uclibc*|aarch64*-*-linux-gnu*|aarch*-*-linux-be*)
    if test "x$use_cpu" = "xauto"; then
      use_cpu=$host_cpu
    fi
    use_toolchain="${use_toolchain:-/usr}"
    platform_cflags="-fPIC -DPIC"
    optimize_flags="-Os"
    platform_ldflags="-Wl,-rpath-link=$prefix/$deps_dir/lib"
    platform_cxxflags="$platform_cflags"
    platform_os="linux"
  ;;
  *i686*-linux-gnu*|i*86*-*-linux-uclibc*|x86_64*-linux-gnu*|x86_64-*-linux-uclibc*)
    use_cpu=$host_cpu
    use_toolchain="${use_toolchain:-/usr}"
    platform_cflags="-fPIC -DPIC"
    optimize_flags="-O2"
    platform_cxxflags="$platform_cflags"
    platform_os="linux"
    ffmpeg_options_default="--enable-vaapi --enable-vdpau --cpu=$use_cpu"
  ;;
  *darwin*)
    if test "x$prefix" = "xNONE"; then
      prefix=/Users/Shared/xbmc-depends
    fi

    use_xcodepath=`xcode-select -print-path`
    use_xcodebuild=$use_xcodepath/usr/bin/xcodebuild
    AC_MSG_RESULT(found xcodebuild at $use_xcodebuild)
    use_build_toolchain=$use_xcodepath
    use_toolchain="${use_xcodepath}/Toolchains/XcodeDefault.xctoolchain"

    # darwin builds are always cross
    cross_compiling="yes"

    platform_cc=clang
    platform_cxx=clang++
    platform_cflags="-fheinous-gnu-extensions -no-cpp-precomp"
    platform_ldflags="-Wl,-search_paths_first"
    platform_cxxflags="-no-cpp-precomp"

    case $host in
      *86*-apple-darwin)
        MC_CHECK_NOT_CPU([$use_cpu], "arm")

        # setup which cpu to use
        case $host in
          x86_64-apple-darwin*)
            if test "x$use_cpu" = "xauto"; then
              use_cpu=x86_64
            fi
          ;;
          i*86-apple-darwin*)
            if test "x$use_cpu" = "xauto"; then
              use_cpu=i386
            fi
            platform_ldflags+=" -read_only_relocs suppress"
         ;;
          *)
            AC_MSG_ERROR(error in configure of --with-arch=$use_cpu)
        esac

        # setup which sdk to use
        found_sdk_version=[`$use_xcodebuild -showsdks | sed -E -n 's/.*macosx([0-9]+)\.([0-9]+)/\1.\2/p' | sort  -n -t. -k1,1 -k2,2 | tail -n 1`]
        use_sdk="${use_sdk:-$found_sdk_version}"

        # now that we know which sdk, error check sdk_name
        case $use_sdk in
          10.9);;
          10.10);;
          10.11);;
          10.12);;
          10.13);;
          10.14);;
          *)
            AC_MSG_ERROR(error in configure of --with-sdk=$use_sdk)
        esac
        sdk_name=macosx$use_sdk
        platform_min_version="macosx-version-min=10.9"

        use_sdk_path=[`$use_xcodebuild -version -sdk $sdk_name Path`]
        platform_os="osx"
      ;;

      arm-apple-darwin*)
        MC_CHECK_NOT_CPU([$use_cpu], "*86")

        # setup which sdk to use
        if test "$use_platform" = "tvos"; then
          # setup which cpu to use
          if test "x$use_cpu" = "xauto"; then
            use_cpu=arm64
          fi

          target_platform=appletvos
          platform_cflags+=" -fembed-bitcode"
          platform_cxxflags+=" -fembed-bitcode"
          platform_min_version="$target_platform-version-min=11.0"
        else
          # setup which cpu to use
          if test "x$use_cpu" = "xauto"; then
            use_cpu=armv7
          fi

          target_platform=iphoneos
          platform_min_version="$target_platform-version-min=9.0"
        fi

        found_sdk_version=[`$use_xcodebuild -showsdks | grep $target_platform | sort | tail -n 1 | awk '{ print $2}'`]
        use_sdk="${use_sdk:-$found_sdk_version}"
        sdk_name=$target_platform$use_sdk

        if test "$use_platform" = "tvos"; then
          case $use_sdk in
            11.*);;
            12.*);;
              *)
                AC_MSG_ERROR(error in configure of --with-sdk=$use_sdk)
              ;;
          esac
        else
          case $use_sdk in
            9.*);;
            10.*);;
            11.*);;
            12.*);;
              *)
                AC_MSG_ERROR(error in configure of --with-sdk=$use_sdk)
              ;;
          esac
        fi

        platform_os="darwin_embedded"

        if [ ! test "x$use_cpu" = "xarm64" ]; then
          platform_cflags+=" -mcpu=cortex-a8 -mfpu=neon"
          platform_ldflags+=" -Wl,-segalign,4000"
        fi
        platform_cflags+=" -ftree-vectorize -pipe -Wno-trigraphs -fpascal-strings"
        platform_cflags+=" -Wreturn-type -Wunused-variable -fmessage-length=0 -gdwarf-2"
        platform_cflags+=" -Wno-error=implicit-function-declaration"
        optimize_flags="-O3"
        use_sdk_path=[`$use_xcodebuild -version -sdk $sdk_name | grep ^Path | awk '{ print $2}'`]
        platform_ldflags+=" -L$use_sdk_path/usr/lib/system"
        platform_cxxflags+=" $cpu_flags"
      ;;
    esac
    platform_cflags+=" -arch $use_cpu -m$platform_min_version"
    platform_ldflags+=" -arch $use_cpu -m$platform_min_version -isysroot $use_sdk_path -stdlib=libc++"
    platform_cxxflags+=" -arch $use_cpu -m$platform_min_version -stdlib=libc++"
    platform_includes="-isysroot $use_sdk_path"
    deps_dir="${sdk_name}_${use_cpu}-target-${build_type}"
    AC_CHECK_LIB([z], [main], has_zlib=1, AC_MSG_WARN("No zlib support in toolchain. Will build libz."); has_zlib=0)
  ;;
  *)
    AC_MSG_ERROR(unsupported host ($use_host))
esac

case $use_platform in
  gbm|wayland)
    if test "$platform_os" != "linux"; then
      AC_MSG_ERROR([$use_platform is only supported on the Linux platform])
    fi
    target_platform=$use_platform
    ;;
  raspberry-pi)
     target_platform=raspberry-pi
     use_cpu=arm1176jzf-s
     ffmpeg_options_default="--cpu=arm1176jzf-s"
     platform_cflags="-mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp"
     platform_cxxflags="-mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp"
     platform_ldflags=""
     ;;
  raspberry-pi2)
     target_platform=raspberry-pi
     use_cpu=cortex-a7
     ffmpeg_options_default="--cpu=cortex-a7"
     platform_cflags="-fPIC -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad"
     platform_cxxflags="-fPIC -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad"
     platform_ldflags="-lpthread"
     ;;
  raspberry-pi3)
     target_platform=raspberry-pi
     use_cpu=cortex-a53
     ffmpeg_options_default="--cpu=cortex-a53"
     platform_cflags="-fPIC -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mvectorize-with-neon-quad"
     platform_cxxflags="-fPIC -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mvectorize-with-neon-quad"
     platform_ldflags="-lpthread"
     ;;
  tvos)
    if test "$platform_os" != "darwin_embedded"; then
      AC_MSG_ERROR([$use_platform is only supported on the darwin_embedded platform])
    fi
    ;;
  auto)
    ;;
  *)
    AC_MSG_ERROR(unsupported platform ($use_platform))
esac

if test "$target_platform" = "raspberry-pi" ; then
  if test -d "${use_firmware}/opt/vc/include"; then
    :
  else
    AC_MSG_ERROR([Raspberry Pi firmware not found])
  fi
  use_arch="arm"
  use_hardcoded_tables="yes"
  ARCH="arm"
  cross_compiling="yes"
  use_host="arm-linux-gnueabihf"
  deps_dir="$use_platform-$build_type"
  platform_cflags+=" -pipe -mabi=aapcs-linux -Wno-psabi \
   -Wa,-mno-warn-deprecated -Wno-deprecated-declarations \
   -isystem${use_firmware}/opt/vc/include \
   -isystem${use_firmware}/opt/vc/include/interface/vcos/pthreads \
   -isystem${use_firmware}/opt/vc/include/interface/vmcs_host/linux"
  platform_cxxflags+=" -pipe -mabi=aapcs-linux -Wno-psabi \
   -Wa,-mno-warn-deprecated -Wno-deprecated-declarations \
   -isystem${use_firmware}/opt/vc/include \
   -isystem${use_firmware}/opt/vc/include/interface/vcos/pthreads \
   -isystem${use_firmware}/opt/vc/include/interface/vmcs_host/linux"
  platform_ldflags+=" -L${use_firmware}/opt/vc/lib -lEGL -lGLESv2 -lbcm_host -lvcos \
   -lvchiq_arm"
fi

XBMC_SETUP_ARCH_DEFINES()


if test -n $use_build_toolchain; then
  PATH_FOR_HOST=$use_toolchain:$use_toolchain/usr/bin:$use_toolchain/bin:$PATH
else
  PATH_FOR_HOST=$PATH
fi
if test -n $use_build_toolchain; then
  PATH_FOR_BUILD=$use_build_toolchain:$use_build_toolchain/usr/bin:$use_toolchain/bin:$PATH
else
  PATH_FOR_BUILD=$PATH
fi

AC_PATH_TOOL([RANLIB], [ranlib],, $PATH_FOR_HOST)
AC_PATH_TOOL([LD], [ld],, $PATH_FOR_HOST)
AC_PATH_TOOL([AR], [ar],, $PATH_FOR_HOST)
AC_PATH_TOOL([READELF], [readelf],, $PATH_FOR_HOST)
AC_PATH_TOOL([STRIP], [strip],, $PATH_FOR_HOST)
AC_PATH_TOOL([AS], [as],, $PATH_FOR_HOST)
AC_PATH_TOOL([NM], [nm],, $PATH_FOR_HOST)
AC_PATH_TOOL([OBJDUMP], [objdump],, $PATH_FOR_HOST)
AC_PATH_TOOL([CC],[$platform_cc],,$PATH_FOR_HOST)
AC_PATH_TOOL([CXX],[$platform_cxx],,$PATH_FOR_HOST)
AC_PROG_CPP

AC_PATH_PROG([RANLIB_FOR_BUILD], [ranlib], ranlib, $PATH_FOR_BUILD)
AC_PATH_PROG([LD_FOR_BUILD], [ld], ld, $PATH_FOR_BUILD)
AC_PATH_PROG([AR_FOR_BUILD], [ar], ar, $PATH_FOR_BUILD)
AC_PATH_PROG([READELF_FOR_BUILD], [readelf], readelf, $PATH_FOR_BUILD)
AC_PATH_PROG([STRIP_FOR_BUILD], [strip], strip, $PATH_FOR_BUILD)
AC_PATH_PROG([AS_FOR_BUILD], [as], as, $PATH_FOR_BUILD)
AC_PATH_PROG([NM_FOR_BUILD], [nm], nm, $PATH_FOR_BUILD)
AC_PATH_PROG([OBJDUMP_FOR_BUILD], [objdump], objdump, $PATH_FOR_BUILD)
AC_PATH_PROG([CC_FOR_BUILD],[gcc llvm-gcc $platform_cc], gcc, $PATH_FOR_BUILD)
AC_PATH_PROG([CXX_FOR_BUILD],[g++ llvm-g++ $platform_cxx], g++, $PATH_FOR_BUILD)

AC_SEARCH_LIBS([iconv_open],iconv, link_iconv=$ac_cv_search_iconv_open, link_iconv=-liconv; AC_MSG_WARN("No iconv support in toolchain. Will build libiconv."); need_libiconv=1)
AC_TRY_LINK([#include <locale.h>],[struct lconv* test=localeconv();], has_localeconv=yes, AC_MSG_WARN("No localeconv support in toolchain. Using replacement."); has_localeconv=no)

if test "$link_iconv" = "none required"; then
  link_iconv=
fi

if test "x$prefix" = "xNONE"; then
  AC_MSG_ERROR([No prefix path defined. Use for ex: --prefix=/opt/xbmc-depends]);
fi

if test "$use_ccache" = "yes"; then
  AC_PATH_TOOL([CCACHE], [ccache],, $PATH_FOR_HOST)
fi

if test -z $use_tarballs; then
  use_tarballs=$prefix/xbmc-tarballs
fi

if test -n "$app_rendersystem"; then
  if test "$app_rendersystem" != "gl" && test "$app_rendersystem" != "gles"; then
    AC_MSG_ERROR(Rendersystem must be gl or gles)
  fi
fi

if test "$platform_os" == "android"; then
  if test -z $use_ndk_path; then
    AC_MSG_ERROR("NDK path is required for android")
  fi

  if ([! test -f "$use_ndk_path/source.properties"] && [! test -f "$use_ndk_path/RELEASE.TXT"]) ; then
    AC_MSG_ERROR("$use_ndk_path is not an NDK directory")
  fi

  if test -z $use_sdk_path; then
    AC_MSG_ERROR("SDK path is required for android")
  fi

  if [ ! test -f $use_sdk_path/tools/android ]; then
    AC_MSG_ERROR(verify sdk path)
  fi

  if [ ! test -f $use_ndk_path/sources/android/native_app_glue/android_native_app_glue.h ]; then
    AC_MSG_ERROR(verify ndk path)
  fi

  #not all sort versions support -V - probe it...
  SORT_PARAMS=""
  sort -V /dev/null > /dev/null 2>&1 && SORT_PARAMS="-V"
  build_tools_path=$use_sdk_path/tools:$use_sdk_path/platform-tools:$use_sdk_path/build-tools/`ls $use_sdk_path/build-tools | sort $SORT_PARAMS | tail -n 1`

  AC_PATH_PROG(AAPT,aapt,"no",$build_tools_path)
  if test "x$AAPT" = "xno" ; then
    AC_MSG_ERROR("Missing program: aapt")
  fi

  AC_PATH_PROG(DX,dx,"no",$build_tools_path)
  if test "x$DX" = "xno" ; then
    AC_MSG_ERROR("Missing program: dx")
  fi

  AC_PATH_PROG(ZIPALIGN,zipalign,"no",$build_tools_path)
  if test "x$ZIPALIGN" = "xno" ; then
    AC_MSG_ERROR("Missing program: zipalign")
  fi
fi

# darwin needs unzip/zip in Codesign.command
if test "$platform_os" = "android" || test "$platform_os" = "osx" || test "$platform_os" = "darwin_embedded"; then
  AC_CHECK_PROG(HAVE_UNZIP,unzip,"yes","no",)
  if test "x$HAVE_UNZIP" = "xno"; then
    AC_MSG_ERROR("Missing program: unzip")
  fi
fi
if test "$platform_os" = "osx" || test "$platform_os" = "darwin_embedded"; then
  AC_CHECK_PROG(HAVE_ZIP,zip,"yes","no",)
  if test "x$HAVE_ZIP" = "xno"; then
    AC_MSG_ERROR("Missing program: zip")
  fi
fi

# Some dumb checks to see if paths might be correct.
if [ ! `mkdir -p $prefix/$deps_dir/include` ]; then
  AC_MSG_ERROR(unable to create $prefix/$deps_dir/include. verify that the path and permissions are correct.)
fi

if [ ! `mkdir -p $prefix/$deps_dir/lib` ]; then
  AC_MSG_ERROR(unable to create $prefix/$deps_dir/lib. verify that the path and permissions are correct.)
fi

if [ ! `mkdir -p $prefix/$deps_dir/share` ]; then
  AC_MSG_ERROR(unable to create $prefix/$deps_dir/share. verify that the path and permissions are correct.)
fi

if [ ! `mkdir -p $prefix/$tool_dir/include` ]; then
  AC_MSG_ERROR(unable to create $prefix/$tool_dir/include. verify that the path and permissions are correct.)
fi

if [ ! `mkdir -p $prefix/$tool_dir/lib` ]; then
  AC_MSG_ERROR(unable to create $prefix/$tool_dir/lib. verify that the path and permissions are correct.)
fi

if [ ! `mkdir -p $prefix/$tool_dir/share` ]; then
  AC_MSG_ERROR(unable to create $prefix/$tool_dir/share. verify that the path and permissions are correct.)
fi

if [ ! `mkdir -p $prefix/$tool_dir/bin` ]; then
  AC_MSG_ERROR(unable to create $prefix/$tool_dir/share. verify that the path and permissions are correct.)
fi

if [ ! `mkdir -p $use_tarballs` ]; then
  AC_MSG_ERROR(unable to create tarballs dir. verify that the path and permissions are correct.)
fi

# remove unwanted optimization flags
tmp_cflags=$(echo $c11_flags $platform_cflags | sed 's/-O@<:@123@:>@//g;s/-g //g;s/ \{2,\}//g')
tmp_cxxflags=$(echo $cxx11_flags $platform_cxxflags | sed 's/-O@<:@123@:>@//g;s/-g //g;s/ \{2,\}//g')

release_cflags="-DNDEBUG=1"

# newer xcode version accept -Og flag, but debugging doesn't work with it
if test "$platform_os" = "osx" || test "$platform_os" = "darwin_embedded"; then
  debug_cflags="-g -D_DEBUG"
else
  CFLAGS="$tmp_cflags -Og -g -D_DEBUG"
  AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE([int foo;])],
    [debug_cflags="-Og -g -D_DEBUG"],
    [debug_cflags="-g -D_DEBUG"])
fi

# add user supplied flags to the end, so they override our defaults
platform_cflags_release="$tmp_cflags $release_cflags $optimize_flags $target_cflags"
platform_cxxflags_release="$tmp_cxxflags $release_cflags $optimize_flags $target_cxxflags"
platform_cflags_debug="$tmp_cflags $debug_cflags $target_cflags"
platform_cxxflags_debug="$tmp_cxxflags $debug_cflags $target_cxxflags"
platform_ldflags+=" $target_ldflags $LIBS"

if test "$use_debug" = "yes"; then
  platform_cflags="$platform_cflags_debug"
  platform_cxxflags="$platform_cxxflags_debug"
else
  platform_cflags="$platform_cflags_release"
  platform_cxxflags="$platform_cxxflags_release"
fi

if test "$ffmpeg_options" == "default"; then
  ffmpeg_options="$ffmpeg_options_default"
fi

if test "$platform_os" == "android"; then
echo -e
  AC_SUBST(use_sdk_path)
  AC_SUBST(use_ndk_path)
  AC_SUBST(use_ndk_api)
  AC_SUBST(build_tools_path)
fi

AC_SUBST(use_debug)
AC_SUBST(use_host)
AC_SUBST(deps_dir)
AC_SUBST(tool_dir)
AC_SUBST(use_cpu)
AC_SUBST(use_toolchain)
AC_SUBST(use_build_toolchain)
AC_SUBST(use_tarballs)
AC_SUBST(target_platform)
AC_SUBST(use_firmware)
AC_SUBST(cross_compiling)
AC_SUBST(platform_cflags)
AC_SUBST(platform_cxxflags)
AC_SUBST(platform_cflags_release)
AC_SUBST(platform_cxxflags_release)
AC_SUBST(platform_cflags_debug)
AC_SUBST(platform_cxxflags_debug)
AC_SUBST(platform_ldflags)
AC_SUBST(platform_includes)
AC_SUBST(platform_os)
AC_SUBST(build_os)
AC_SUBST(has_zlib)
AC_SUBST(link_iconv)
AC_SUBST(need_libiconv)
AC_SUBST(use_gplv3)
AC_SUBST(use_ccache)
AC_SUBST(native_platform_min_version)
AC_SUBST(app_rendersystem)
AC_SUBST(ffmpeg_options)

AC_OUTPUT

if test "$platform_os" == "darwin_embedded"; then
  if test "$use_platform" = "tvos"; then
    simulator_sdk_path=[`$use_xcodebuild -version -sdk appletvsimulator$use_sdk  | grep ^Path | awk '{ print $2}'`]
  else
    simulator_sdk_path=[`$use_xcodebuild -version -sdk iphonesimulator$use_sdk  | grep ^Path | awk '{ print $2}'`]
  fi
  cp -vf $simulator_sdk_path/usr/include/crt_externs.h $prefix/$deps_dir/include
fi

echo -e "use simulator:\t $use_simulator"

if test "x$has_localeconv" == "xno" && test "$platform_os" == "android"; then
  cp -vf target/android-libc-replacements/locale.h $prefix/$deps_dir/include/
fi

cp -vf target/config.site $prefix/$deps_dir/share
cp -vf target/config-binaddons.site $prefix/$tool_dir/share
cp -vf target/Toolchain.cmake $prefix/$deps_dir/share
cp -vf target/Toolchain_binaddons.cmake $prefix/$deps_dir/share
cp -vf target/cross-file.meson $prefix/$deps_dir/share
cp -vf native/config.site.native $prefix/$tool_dir/share/config.site


echo -e "\n\n#------- configuration -------#"
echo -e "ccache:\t\t $use_ccache"
echo -e "build type:\t $build_type"
echo -e "toolchain:\t $use_toolchain"
echo -e "cpu:\t\t $use_cpu"
echo -e "host:\t\t $use_host"
echo -e "cflags:\t\t $platform_cflags"
echo -e "cxxflags:\t $platform_cxxflags"
echo -e "ldflags:\t $platform_ldflags"
echo -e "ffmpeg options:\t $ffmpeg_options"
echo -e "prefix:\t\t $prefix"
echo -e "depends:\t $prefix/$deps_dir"
if test "$platform_os" == "android"; then
  echo -e "ndk-api-level:\t $use_ndk_api"
  echo -e "build-tools:\t $build_tools_path"
fi
