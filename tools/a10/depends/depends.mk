# I used miniand lubuntu (kernel 3.0.36) on mk802.
#
# On a10 lubuntu run apt-get build-dep xbmc in it to get all dependencies.
# SDKSTAGE: rootfs from lubuntu/armel if cross-compiling. 
# On x86 Ubuntu 12.04: I had to make symlinks for 
# /lib/arm-linux-gnueabi, /usr/lib/arm-linux-gnueabi 
# and /usr/include/arm-linux-gnueabi to their path in ${SDKSTAGE} 
#

#your home dir
HOME=$(shell echo ~)
#where your tarballs go
TARBALLS=$(HOME)/tarballs
#whether to compile for armhf
USEARMHF=0

#
# armhf notes:
#
# I used linaro alip together with wip/linux-sunxi-3.0/next_mali kernel.
#

ifeq ($(USEARMHF), 1)
HF=hf
else
HF=
endif

ifeq ($(shell uname -m),armv7l)
#
#native compile
#

#where is your arm rootfs
SDKSTAGE=/
#where is your xbmc install root 
XBMCPREFIX=/allwinner/xbmc-pvr-bin$(HF)
#where is your toolchain
TOOLCHAIN=/usr

export HOST=arm-linux-gnueabi$(HF)
export BUILD=arm-linux-gnueabi$(HF)
export CROSS_COMPILE=

else
#
#cross compile
#

#where is your arm rootfs
SDKSTAGE=/home/stefan/allwinner/rootfs$(HF)
#where is your xbmc install root 
XBMCPREFIX=/allwinner/xbmc-pvr-bin$(HF)
#where is your toolchain
TOOLCHAIN=/usr/arm-linux-gnueabi$(HF)

export HOST=arm-linux-gnueabi$(HF)
export BUILD=i686-linux
export CROSS_COMPILE=${HOST}-

endif


export PREFIX=${XBMCPREFIX}

GLESDIR=$(shell cd ..;pwd)/opengles

export GLESINCLUDES=-I$(GLESDIR)/include

CEDARDIR=$(shell cd ..;pwd)/cedarv

export CEDARINCLUDES=\
	-I$(CEDARDIR) \
	-I$(CEDARDIR)/adapter \
	-I$(CEDARDIR)/adapter/cdxalloc \
	-I$(CEDARDIR)/adapter/avheap \
	-I$(CEDARDIR)/fbm \
	-I$(CEDARDIR)/libcedarv \
	-I$(CEDARDIR)/libvecore \
	-I$(CEDARDIR)/vbv

#vecore,cedarxalloc taken from $(XBMCPREFIX)/lib
ifeq ($(USEARMHF), 1)
export CEDARLIBS=-L$(CEDARDIR) -lcedarv -lvecore
else
export CEDARLIBS=-L$(CEDARDIR) -lcedarv -lvecore -lcedarxalloc
endif

export RLINK_PATH=-Wl,-rpath,$(XBMCPREFIX)/lib -Wl,-rpath-link,${XBMCPREFIX}/lib:$(SDKSTAGE)/usr/local/lib:${SDKSTAGE}/lib:${SDKSTAGE}/lib/arm-linux-gnueabi$(HF):${SDKSTAGE}/usr/lib:${SDKSTAGE}/usr/lib/arm-linux-gnueabi$(HF)
export LDFLAGS=\
${RLINK_PATH} \
-L${XBMCPREFIX}/lib \
-L$(SDKSTAGE)/usr/local/lib \
-L${SDKSTAGE}/lib \
-L${SDKSTAGE}/lib/arm-linux-gnueabi$(HF) \
-L${SDKSTAGE}/usr/lib \
-L${SDKSTAGE}/usr/lib/arm-linux-gnueabi$(HF)
 
export CFLAGS=-pipe -O3 -mtune=cortex-a8 -D__ARM_NEON__ -DALLWINNERA10
export CFLAGS+=$(CEDARINCLUDES) $(GLESINCLUDES)
export CFLAGS+=\
-isystem${XBMCPREFIX}/include \
-isystem$(SDKSTAGE)/usr/local/include \
-isystem${SDKSTAGE}/usr/include \
-isystem${SDKSTAGE}/usr/include/arm-linux-gnueabi$(HF)
export CFLAGS+=${LDFLAGS}

export CXXFLAGS=${CFLAGS}
export CPPFLAGS=${CFLAGS}
export LD=${CROSS_COMPILE}ld
export AR=${CROSS_COMPILE}ar
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++
export CXXCPP=${CXX} -E
export RANLIB=${CROSS_COMPILE}ranlib
export STRIP=${CROSS_COMPILE}strip
export OBJDUMP=${CROSS_COMPILE}objdump
export PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig:${SDKSTAGE}/lib/pkgconfig:${SDKSTAGE}/usr/lib/pkgconfig:${SDKSTAGE}/usr/lib/arm-linux-gnueabi$(HF)/pkgconfig:${SDKSTAGE}/usr/share/pkgconfig:${SDKSTAGE}/usr/local/lib/pkgconfig
export PKG_CONFIG_PATH=${PREFIX}/bin/pkg-config
export PYTHON_VERSION=2.7
export PATH:=${PREFIX}/bin:${TOOLCHAIN}/bin:$(PATH)
export TEXTUREPACKER_NATIVE_ROOT=/usr
export PYTHON_LDFLAGS=-L${SDKSTAGE}/usr/lib -lpython$(PYTHON_VERSION)
export PYTHON_CPPFLAGS=-I${SDKSTAGE}/usr/include/python$(PYTHON_VERSION)
export PYTHON_SITE_PKG=${SDKSTAGE}/usr/lib/python$(PYTHON_VERSION)/site-packages
export PYTHON_NOVERSIONCHECK=no-check
