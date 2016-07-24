AC_PREREQ(2.59)
AC_INIT([xbmc-depends], [2.00], [http://trac.xbmc.org])
:${CFLAGS=""}
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([target/config.site native/config.site.native Makefile.include target/Toolchain.cmake
                 target/config-binaddons.site target/Toolchain_binaddons.cmake])
AC_CANONICAL_HOST
m4_include([../../m4/xbmc_arch.m4])
m4_include([../../m4/ax_cxx_compile_stdcxx_11.m4])
AX_CXX_COMPILE_STDCXX_11(,[optional])

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

AC_ARG_WITH([toolchain],
  [AS_HELP_STRING([--with-toolchain],
  [specify path to toolchain. Required for android. Defaults to xcode root for darwin, /usr for linux])],
  [use_toolchain=$withval])

AC_ARG_WITH([platform],
  [AS_HELP_STRING([--with-platform],
  [target platform [auto]])],
  [use_platform=$withval])

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
  [use_cpu=$withval],use_cpu=auto)

AC_ARG_WITH([ndk],
  [AS_HELP_STRING([--with-ndk],
  [specify path to ndk (required for android only)])],
  [use_ndk=$withval])

AC_ARG_WITH([sdk-path],
  [AS_HELP_STRING([--with-sdk-path],
  [specify path to sdk (required for android only)])],
  [use_sdk_path=$withval])

AC_ARG_WITH([sdk],
  [AS_HELP_STRING([--with-sdk],
  [spcify sdk platform version (optional for android). default is android-17])],
  [use_sdk=$withval])

AC_ARG_ENABLE([gplv3],
  [AS_HELP_STRING([--enable-gplv3],
  [enable gplv3 components. default is yes])],
  [use_gplv3=$enableval],
  [use_gplv3=yes])


AC_CHECK_PROG(HAVE_UNZIP,unzip,"yes","no",)
if test "x$HAVE_UNZIP" = "xno" ; then
  AC_MSG_ERROR("Missing program: unzip")
fi

AC_CHECK_PROG(HAVE_ZIP,zip,"yes","no",)
if test "x$HAVE_ZIP" = "xno" ; then
  AC_MSG_ERROR("Missing program: zip")
fi

AC_PATH_PROG(CURL,curl,"no")
if test "x$CURL" = "xno" ; then
  AC_MSG_ERROR("Missing program: curl")
fi

AC_PATH_PROG(TAR,tar,"no")
if test "x$TAR" = "xno" ; then
  AC_MSG_ERROR("Missing program: tar")
fi

use_host=$host_alias

cross_compiling="yes"
if test "x$host" = "x$build"; then
  use_host=$build_cpu-$build_os
  cross_compiling="no"
fi

deps_dir=$use_host
tool_dir=$build_cpu-$build_os-native
cross_compiling="yes"

passed_cflags="$CFLAGS"
passed_ldflags="$LDFLAGS"
passed_cxxflags="$CXXFLAGS"
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
  ;;
  *)
    AC_MSG_ERROR(unsupported native build platform: $build)
esac


case $host in
  arm*-*linux-android*)
    if test "x$use_cpu" = "xauto"; then
      use_cpu="armeabi-v7a"
    fi
    use_sdk="${use_sdk:-android-17}"
    deps_dir="$use_host-$use_sdk"
    platform_cflags="-DANDROID -Os -fexceptions"
    if test "x$use_cpu" = "xarmeabi-v7a"; then
      platform_cflags+=" -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon"
    fi
    platform_ldflags="-L$prefix/$deps_dir/lib/$use_sdk"
    platform_cxxflags="$platform_cflags -frtti"
    platform_includes="-I$prefix/$deps_dir/include/$use_sdk"
    platform_os="android"
    #android builds are always cross
    cross_compiling="yes"
  ;;
  i*86*-linux-android*)
    if test "x$use_cpu" = "xauto"; then
      use_cpu=$host_cpu
    fi
    use_sdk="${use_sdk:-android-17}"
    deps_dir="$use_host-$use_sdk"
    platform_cflags="-DANDROID -Os -fexceptions"
    platform_ldflags="-L$prefix/$deps_dir/lib/$use_sdk"
    platform_cxxflags="$platform_cflags -frtti"
    platform_includes="-I$prefix/$deps_dir/include/$use_sdk"
    platform_os="android"
    #android builds are always cross
    cross_compiling="yes"
  ;;
  arm*-*-linux-gnu*|arm*-*-linux-uclibc*)
    if test "$use_platform" = "auto"; then
      if test "x$use_cpu" = "xauto"; then
        use_cpu=$host_cpu
      fi
      use_toolchain="${use_toolchain:-/usr}"
      platform_cflags="-Os -fPIC -DPIC"
      platform_ldflags="-Wl,-rpath-link=$prefix/$deps_dir/lib"
      platform_cxxflags="$platform_cflags"
      platform_os="linux"
    fi
  ;;
  *i686*-linux-gnu*|i*86*-*-linux-uclibc*)
    use_cpu=$host_cpu
    use_toolchain="${use_toolchain:-/usr}"
    platform_os="linux"
  ;;
  x86_64*-linux-gnu*|x86_64-*-linux-uclibc*)
    use_cpu=$host_cpu
    use_toolchain="${use_toolchain:-/usr}"
    platform_cflags="-fPIC -DPIC"
    platform_cxxflags="$platform_cflags"
    platform_os="linux"
  ;;
  *darwin*)
    if test "x$prefix" = "xNONE"; then
      prefix=/Users/Shared/xbmc-depends
    fi
    use_tarballs="${use_tarballs:-$prefix/tarballs}"
    use_xcodepath=`xcode-select -print-path`
    use_xcodebuild=$use_xcodepath/usr/bin/xcodebuild
    use_xcode=[`$use_xcodebuild -version | grep Xcode | awk '{ print $2}'`]
    AC_MSG_RESULT(found xcodebuild at $use_xcodebuild)
    use_build_toolchain=$use_xcodepath

    # darwin builds are always cross
    cross_compiling="yes"

    platform_cflags="-std=gnu99 -no_compact_linkedit -no-cpp-precomp"
    platform_ldflags="-Wl,-search_paths_first -no_compact_linkedit"
    platform_cxxflags="-no_compact_linkedit -no-cpp-precomp"

    case $use_xcode in
      3.*.*)
        platform_cc=gcc-4.2
        platform_cxx=g++-4.2
        ;;
      4.* | 4.*.*)
        platform_cc=llvm-gcc-4.2
        platform_cxx=llvm-g++-4.2
        ;;
      *)
        platform_cc=clang
        platform_cxx=clang++
        platform_cflags="-fheinous-gnu-extensions -no-cpp-precomp"
        platform_ldflags="-Wl,-search_paths_first"
        platform_cxxflags="-no-cpp-precomp"
        ;;
    esac
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
          10.5);;
          10.6);;
          10.7);;
          10.8);;
          10.9);;
          10.10);;
          10.11);;
          *)
            AC_MSG_ERROR(error in configure of --with-sdk=$use_sdk)
        esac
        sdk_name=macosx$use_sdk
        platform_min_version="macosx-version-min=10.8"

        use_sdk_path=[`$use_xcodebuild -version -sdk $sdk_name Path`]
        platform_os="osx"

        # find the matching toolchain
        case $use_xcode in
          5.* | 5.*.* )
            use_toolchain="${use_xcodepath}/Toolchains/XcodeDefault.xctoolchain"
            ;;
          *)
            use_toolchain="${use_toolchain:-$use_xcodepath}"
            ;;
        esac
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

          found_sdk_version=[`$use_xcodebuild -showsdks | grep $target_platform | sort | tail -n 1 | awk '{ print $2}'`]
          use_sdk="${use_sdk:-$found_sdk_version}"
	  platform_cflags+=" -fembed-bitcode"
	  platform_cxxflags+=" -fembed-bitcode"
          sdk_name=$target_platform$use_sdk
          platform_min_version="$target_platform-version-min=9.0"
        else
          # setup which cpu to use
          if test "x$use_cpu" = "xauto"; then
            use_cpu=armv7
          fi

          target_platform=iphoneos

          found_sdk_version=[`$use_xcodebuild -showsdks | grep $target_platform | sort | tail -n 1 | awk '{ print $2}'`]
          use_sdk="${use_sdk:-$found_sdk_version}"
          sdk_name=$target_platform$use_sdk
          platform_min_version="$target_platform-version-min=5.1"
        fi
	case $use_sdk in
          4.*);;
          5.*);;
          6.*);;
          7.*);;
          8.*);;
          9.*);;
            *)
              AC_MSG_ERROR(error in configure of --with-sdk=$use_sdk)
            ;;
        esac

        # find the matching toolchain
        case $use_xcode in
          3.*.* | 4.* | 4.*.*)
            use_toolchain="${use_toolchain:-`$use_xcodebuild -version -sdk $sdk_name PlatformPath`/Developer}"
            ;;
          *)
            use_toolchain="${use_xcodepath}/Toolchains/XcodeDefault.xctoolchain"
            ;;
        esac

        platform_os="ios"

        if [ ! test "x$use_cpu" = "xarm64" ]; then
          platform_cflags+=" -mcpu=cortex-a8 -mfpu=neon"
          platform_ldflags+=" -Wl,-segalign,4000"
        fi
        platform_cflags+=" -ftree-vectorize -pipe -Wno-trigraphs -fpascal-strings -O3"
        platform_cflags+=" -Wreturn-type -Wunused-variable -fmessage-length=0 -gdwarf-2"
        platform_cflags+=" -Wno-error=implicit-function-declaration"
        platform_ldflags+=" -L$use_sdk_path/usr/lib/system"
        platform_cxxflags+=" $cpu_flags"
        use_sdk_path=[`$use_xcodebuild -version -sdk $sdk_name | grep ^Path | awk '{ print $2}'`]
      ;;
    esac
    platform_cflags+=" -arch $use_cpu -m$platform_min_version"
    platform_ldflags+=" -arch $use_cpu -m$platform_min_version -isysroot $use_sdk_path -stdlib=libc++"
    platform_cxxflags+=" -arch $use_cpu -m$platform_min_version -std=c++11 -stdlib=libc++"
    platform_includes="-isysroot $use_sdk_path"
    deps_dir=$sdk_name"_"$use_cpu-target
    tool_dir=buildtools-native;
  ;;
  *)
    AC_MSG_ERROR(unsupported host ($use_host))
esac

case $use_platform in
  raspberry-pi)
     target_platform=raspberry-pi
     use_neon=no
     use_cpu=arm1176jzf-s
     platform_cflags="-mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp"
     platform_cxxflags="-mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp"
     platform_ldflags=""
     ;;
  raspberry-pi2)
     target_platform=raspberry-pi
     use_neon=yes
     use_cpu=cortex-a7
     platform_cflags="-fPIC -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4"
     platform_cxxflags="-fPIC -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4"
     platform_ldflags="-lpthread"
     ;;
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
  platform_os="linux"
  cross_compiling="yes"
  use_host="arm-linux-gnueabihf"
  deps_dir="$use_host"
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

platform_cflags+=" $passed_cflags"
platform_ldflags+=" $passed_ldflags $LIBS"
platform_cxxflags+=" $passed_cxxflags"

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

AC_CHECK_LIB([z],   [main], has_zlib=1, AC_MSG_WARN("No zlib support in toolchain. Will build libz."); has_zlib=0)
AC_SEARCH_LIBS([iconv_open],iconv, link_iconv=$ac_cv_search_iconv_open, link_iconv=-liconv; AC_MSG_WARN("No iconv support in toolchain. Will build libiconv."); need_libiconv=1)
AC_TRY_LINK([#include <locale.h>],[struct lconv* test=localeconv();], has_localeconv=yes, AC_MSG_WARN("No localeconv support in toolchain. Using replacement."); has_localeconv=no)
AC_CHECK_LIB([crystax],   [main], has_libcrystax=1, has_libcrystax=0)

if test "$link_iconv" = "none required"; then
  link_iconv=
fi

if test "x$prefix" = "xNONE"; then
  AC_MSG_ERROR([No prefix path defined. Use for ex: --prefix=/opt/xbmc-depends]);
fi

if test -z $use_tarballs; then
  use_tarballs=$prefix/xbmc-tarballs
fi

if test "$platform_os" == "android"; then
  if test -z $use_ndk; then
    AC_MSG_ERROR("NDK path is required for android")
  fi

  if ([! test -f "$use_ndk/source.properties"] && [! test -f "$use_ndk/RELEASE.TXT"]) ; then
    AC_MSG_ERROR("$use_ndk is not an NDK directory")
  fi

  if test -z $use_sdk_path; then
    AC_MSG_ERROR("SDK path is required for android")
  fi

  if [ ! test -f $use_sdk_path/tools/android ]; then
    AC_MSG_ERROR(verify sdk path)
  fi

  if [ ! test -f $use_ndk/sources/android/native_app_glue/android_native_app_glue.h ]; then
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

if test "$platform_os" == "android"; then
  echo

  if [ ! `mkdir -p $prefix/$deps_dir/include/$use_sdk` ]; then
    AC_MSG_ERROR(unable to create $prefix/$deps_dir/include/$use_sdk. verify that the path and permissions are correct.)
  fi

  if [ ! `mkdir -p $prefix/$deps_dir/lib/$use_sdk` ]; then
    AC_MSG_ERROR(unable to create $prefix/$deps_dir/lib/$use_sdk. verify that the path and permissions are correct.)
  fi
fi

if test "$platform_os" == "android"; then
echo -e
  AC_SUBST(use_sdk_path)
  AC_SUBST(use_ndk)
  AC_SUBST(use_sdk)
  AC_SUBST(build_tools_path)
fi

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
AC_SUBST(platform_ldflags)
AC_SUBST(platform_includes)
AC_SUBST(platform_os)
AC_SUBST(build_os)
AC_SUBST(has_zlib)
AC_SUBST(link_iconv)
AC_SUBST(need_libiconv)
AC_SUBST(use_gplv3)
AC_SUBST(has_libcrystax)
AC_SUBST(use_xcode)

AC_OUTPUT
echo -e "toolchain:\t $use_toolchain"
echo -e "cpu:\t\t $use_cpu"
echo -e "host:\t\t $use_host"
if test "$platform_os" == "android"; then
  echo -e "sdk-platform:\t $use_sdk"
  echo -e "build-tools:\t $build_tools_path"
fi

if test "$platform_os" == "ios"; then
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
cp -vf native/config.site.native $prefix/$tool_dir/share/config.site
