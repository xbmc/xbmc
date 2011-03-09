# Find where this Makefile is located
TOP := $(dir $(lastword $(MAKEFILE_LIST)))

platform_os=iPhoneOS
platform_sdk_version=4.2
platform_min_version=iphoneos-version-min=4.1
platform_gcc_version=4.2.1
platform_host=arm-apple-darwin10
platform_path=/Developer/Platforms/iPhoneOS.platform/Developer
platform_os_cflags=-march=armv7 -mcpu=cortex-a8 -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -pipe -Wno-trigraphs -fpascal-strings -Os -Wreturn-type -Wunused-variable -fmessage-length=0 -gdwarf-2
platform_os_ldflags=-march=armv7 -mcpu=cortex-a8
prefix_path=/Users/Shared/xbmc-depends/ios-4.2_arm7
platform_sdk_path=${platform_path}/SDKs/${platform_os}${platform_sdk_version}.sdk
platform_bin_path=${platform_path}/usr/bin

export platform_sdk_version
export NM=/usr/bin/nm
export CPP=/usr/bin/cpp-4.2
export CXXCPP=${CPP} -I${platform_sdk_path}/usr/include/c++/${platform_gcc_version}/${platform_host}
export CPPFLAGS=-I${platform_sdk_path}/usr/include  -I${prefix_path}/include
export CC=${platform_bin_path}/${platform_host}-gcc-${platform_gcc_version}
export CFLAGS=-std=gnu99 -no-cpp-precomp -m${platform_min_version} -isysroot ${platform_sdk_path} -I${platform_sdk_path}/usr/include ${platform_os_cflags}
export LD=${platform_bin_path}/ld
export LDFLAGS=-m${platform_min_version} -isysroot ${platform_sdk_path} -L${platform_sdk_path}/usr/lib ${platform_os_ldflags} -L${prefix_path}/lib
export CXX=${platform_bin_path}/${platform_host}-g++-${platform_gcc_version} -I${platform_sdk_path}/usr/include/c++/${platform_gcc_version}/${platform_host}
export CXXFLAGS=-m${platform_min_version} -isysroot ${platform_sdk_path} ${platform_os_cflags}
export AR=${platform_bin_path}/ar
export AS=${prefix_path}/bin/gas-preprocessor.pl ${CC}
export CCAS=--tag CC ${prefix_path}/bin/gas-preprocessor.pl ${CC}
export STRIP=${platform_bin_path}/strip
export RANLIB=${platform_bin_path}/ranlib
export ACLOCAL=aclocal -I /Developer/usr/share/aclocal -I ${prefix_path}/share/aclocal
export LIBTOOL=/Developer/usr/bin/glibtool
export LIBTOOLIZE=/Developer/usr/bin/glibtoolize
export HOST=${platform_host}
export PREFIX=${prefix_path}
export DEVROOT=${platform_path}
export SDKROOT=${platform_sdk_path}
export PKG_CONFIG_PATH=${prefix_path}:${platform_sdk_path}/usr/lib/pkgconfig
export PATH:=${prefix_path}/bin:${platform_bin_path}:/Developer/usr/bin:$(PATH)

