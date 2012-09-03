ifeq ($(USE_BUILDROOT),1)
	export CFLAGS=-pipe -O3 -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mabi=aapcs-linux -Wno-psabi -Wa,-mno-warn-deprecated -Wno-deprecated-declarations
	export HOST=arm-unknown-linux-gnueabi
	export BUILD=i686-linux
	export PREFIX=$(XBMCPREFIX)
	export SYSROOT=$(BUILDROOT)/output/host/usr/arm-unknown-linux-gnueabi/sysroot
	export CFLAGS+=-isystem$(SYSROOT)/usr/include -isystem$(SYSROOT)/opt/vc/include -isystem$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -isystem$(PREFIX)/include -isystem$(PREFIX)/usr/include/mysql --sysroot=$(SYSROOT)
	export CXXFLAGS=$(CFLAGS) --sysroot=$(SYSROOT)
	export CPPFLAGS=$(CFLAGS) --sysroot=$(SYSROOT)
	export LDFLAGS=-L$(SYSROOT)/opt/vc/lib -L$(XBMCPREFIX)/lib
	export LD=$(TOOLCHAIN)/bin/$(HOST)-ld 
	export CC=$(TOOLCHAIN)/bin/$(HOST)-gcc 
	export CXX=$(TOOLCHAIN)/bin/$(HOST)-g++ 
	export OBJDUMP=$(TOOLCHAIN)/bin/$(HOST)-objdump
	export RANLIB=$(TOOLCHAIN)/bin/$(HOST)-ranlib
	export STRIP=$(TOOLCHAIN)/bin/$(HOST)-strip
	export AR=$(TOOLCHAIN)/bin/$(HOST)-ar
	export CXXCPP=$(CXX) -E
	export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig
	export PYTHON_VERSION=2.7
	export PATH:=$(PREFIX)/bin:$(BUILDROOT)/output/host/usr/bin:$(PATH)
	export TEXTUREPACKER_NATIVE_ROOT=/usr
	export PYTHON_LDFLAGS=-L$(SDKSTAGE)/usr/lib -lpython$(PYTHON_VERSION) -lpthread -ldl -lutil -lm
else
	export CFLAGS=-pipe -O3 -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=softfp -mfpu=vfp -mabi=aapcs-linux -Wno-psabi -Wa,-mno-warn-deprecated -Wno-deprecated-declarations
	export HOST=arm-bcm2708-linux-gnueabi
	export BUILD=i686-linux
	export PREFIX=${XBMCPREFIX}
	export TARGETFS
	export SYSROOT=/usr/local/bcm-gcc/arm-bcm2708-linux-gnueabi/sys-root
	export RLINK_PATH=-Wl,-rpath-link,${SYSROOT}/lib -Wl,-rpath-link,${TARGETFS}/lib -Wl,-rpath-link,${TARGETFS}/usr/lib -Wl,-rpath-link,${TARGETFS}/opt/vc/
	export CFLAGS+=-isystem${XBMCPREFIX}/include -isystem${SDKSTAGE}/usr/include -isystem${SDKSTAGE}/opt/vc/include -isystem$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -isystem${SDKSTAGE}/opt/vc 
	export CFLAGS+=-L${XBMCPREFIX}/lib -L${SYSROOT}/lib -L${TARGETFS}/lib -L${TARGETFS}/usr/lib -L${TARGETFS}/opt/vc/lib ${RLINK_PATH}
	export CXXFLAGS=${CFLAGS}
	export CPPFLAGS=${CFLAGS}
	export LDFLAGS=${RLINK_PATH} -L${TARGETFS}/lib -L${TARGETFS}/usr/lib -L${XBMCPREFIX}/lib
	export LD=${TOOLCHAIN}/bin/${HOST}-ld
	export AR=${TOOLCHAIN}/bin/${HOST}-ar
	export CC=${TOOLCHAIN}/bin/${HOST}-gcc
	export CXX=${TOOLCHAIN}/bin/${HOST}-g++
	export CXXCPP=${CXX} -E
	export RANLIB=${TOOLCHAIN}/bin/${HOST}-ranlib
	export STRIP=${TOOLCHAIN}/bin/${HOST}-strip
	export OBJDUMP=${TOOLCHAIN}/bin/${HOST}-objdump
	#export ACLOCAL=aclocal -I ${SDKSTAGE}/usr/share/aclocal -I ${TARGETFS}/usr/share/aclocal-1.11
	export PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig:${SDKSTAGE}/lib/pkgconfig:${SDKSTAGE}/usr/lib/pkgconfig
	export PKG_CONFIG_PATH=$(PREFIX)/bin/pkg-config
	export PYTHON_VERSION=2.6
	export PATH:=${PREFIX}/bin:$(PATH):${TOOLCHAIN}/bin
	export TEXTUREPACKER_NATIVE_ROOT=/usr
	export PYTHON_LDFLAGS=-L$(SDKSTAGE)/usr/lib -lpython$(PYTHON_VERSION)
endif
export PYTHON_CPPFLAGS=-I$(SDKSTAGE)/usr/include/python$(PYTHON_VERSION)
export PYTHON_SITE_PKG=$(SDKSTAGE)/usr/lib/python$(PYTHON_VERSION)/site-packages
export PYTHON_NOVERSIONCHECK=no-check
