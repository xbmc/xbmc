SDKSTAGE=/home/stefan/allwinner/rootfs
XBMCPREFIX=/allwinner/xbmc-pvr-bin
TARGETFS=/home/stefan/allwinner/rootfs
TOOLCHAIN=/usr/arm-linux-gnueabi
BUILDROOT=/home/stefan/allwinner/rootfs
USE_BUILDROOT=0
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
TARBALLS_LOCATION=/opt/xbmc-tarballs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=/bin/tar
ARCHIVE_TOOL_FLAGS=xf
JOBS=4

CEDARDIR=$(shell pwd)/../cedarv

export CEDARINCLUDES=\
	-I$(CEDARDIR) \
	-I$(CEDARDIR)/adapter \
	-I$(CEDARDIR)/adapter/cdxalloc \
	-I$(CEDARDIR)/fbm \
	-I$(CEDARDIR)/libcedarv \
	-I$(CEDARDIR)/libvecore \
	-I$(CEDARDIR)/vbv
	
export CEDARLIBS=\
	-L$(CEDARDIR) -lcedarv \
	-L$(CEDARDIR)/adapter/cdxalloc -lcedarxalloc \
	-L$(CEDARDIR)/libvecore -lvecore

export CFLAGS=-pipe -O3 -mtune=cortex-a8 -D__ARM_NEON__ -DALLWINNERA10
export HOST=arm-linux-gnueabi
export BUILD=i686-linux
export PREFIX=${XBMCPREFIX}
export TARGETFS
export SYSROOT=/home/stefan/allwinner/rootfs/usr
export RLINK_PATH=-Wl,-rpath-link,${SYSROOT}/lib -Wl,-rpath-link,${TARGETFS}/lib/arm-linux-gnueabi:${TARGETFS}/usr/lib/arm-linux-gnueabi:${TARGETFS}/lib:${TARGETFS}/usr/lib:${TARGETFS}/usr/lib/pulseaudio 
export CFLAGS+=-isystem${XBMCPREFIX}/include -isystem${SDKSTAGE}/usr/include/arm-linux-gnueabi -isystem${SDKSTAGE}/usr/include -isystem${SDKSTAGE}/usr/local/include
export CFLAGS+=$(CEDARINCLUDES)
export CFLAGS+=-L${XBMCPREFIX}/lib -L${SYSROOT}/lib -L${SYSROOT}/usr/lib -L${TARGETFS}/lib -L${TARGETFS}/usr/lib -L${TARGETFS}/usr/local/lib ${RLINK_PATH} -L${TARGETFS}/usr/lib/arm-linux-gnueabi -L${TARGETFS}/lib/arm-linux-gnueabi
export CXXFLAGS=${CFLAGS}
export CPPFLAGS=${CFLAGS}
export LDFLAGS=${RLINK_PATH} -L${SYSROOT}/usr/lib -L${TARGETFS}/lib -L${TARGETFS}/usr/lib -L${TARGETFS}/usr/local/lib -L${XBMCPREFIX}/lib -L${TARGETFS}/usr/lib/arm-linux-gnueabi 
export LD=${HOST}-ld
export AR=${HOST}-ar
export CC=${HOST}-gcc
export CXX=${HOST}-g++
export CXXCPP=${CXX} -E
export RANLIB=${HOST}-ranlib
export STRIP=${HOST}-strip
export OBJDUMP=${HOST}-objdump
#export ACLOCAL=aclocal -I ${SDKSTAGE}/usr/share/aclocal -I ${TARGETFS}/usr/share/aclocal-1.11
export PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig:${SDKSTAGE}/lib/pkgconfig:${SDKSTAGE}/usr/lib/pkgconfig:${SDKSTAGE}/usr/lib/arm-linux-gnueabi/pkgconfig:${STKSTAGE}/usr/share/pkgconfig:${SDKSTAGE}/usr/local/lib/pkgconfig
export PKG_CONFIG_PATH=$(PREFIX)/bin/pkg-config
export PYTHON_VERSION=2.7
export PATH:=${PREFIX}/bin:$(PATH):${TOOLCHAIN}/bin
export TEXTUREPACKER_NATIVE_ROOT=/usr
export PYTHON_LDFLAGS=-L$(SDKSTAGE)/usr/lib -lpython$(PYTHON_VERSION)
export PYTHON_CPPFLAGS=-I$(SDKSTAGE)/usr/include/python$(PYTHON_VERSION)
export PYTHON_SITE_PKG=$(SDKSTAGE)/usr/lib/python$(PYTHON_VERSION)/site-packages
export PYTHON_NOVERSIONCHECK=no-check

