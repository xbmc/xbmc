#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install
#
#  credits to:
#  http://randomsplat.com/id5-cross-compiling-python-for-embedded-linux.html
#  http://github.com/cobbal/python-for-iphone

include ../config.mk

# lib name, version
LIBNAME=Python
VERSION=2.6.5
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.bz2
TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export LDFLAGS+=-Lextralibs/
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) --enable-shared \
  CC="$(DEVROOT)/usr/bin/gcc-4.2 -arch armv7" \
  LD="$(DEVROOT)/usr/bin/ld -arch armv7" \
  --disable-toolbox-glue

CONFIGURE_NATIVE= CFLAGS="" CXXFLAGS="" LDFLAGS="" CPPFLAGS="" \
  CPP="/usr/bin/cpp" \
  CC="gcc-4.2" \
  CXX="g++-4.2" \
  LD="ld" \
  ./configure

LIBDYLIB=$(SOURCE)/libpython2.6.dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBDYLIB): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE_NATIVE)
	cd $(SOURCE); make python.exe Parser/pgen
	cd $(SOURCE); mv python.exe hostpython
	cd $(SOURCE); mv Parser/pgen Parser/hostpgen
	cd $(SOURCE); mv libpython2.6.a hostlibpython2.6.a
	cd $(SOURCE); make distclean
	cd $(SOURCE); patch -p1 <../Python-2.6.5-xcompile.patch
	cd $(SOURCE); mkdir -p extralibs; ln -sf "$(SDKROOT)/usr/lib/libgcc_s.1.dylib" extralibs/libgcc_s.10.4.dylib
	cd $(SOURCE); $(CONFIGURE)
	cd $(SOURCE); make HOSTPYTHON=./hostpython HOSTPGEN=./Parser/hostpgen CROSS_COMPILE_TARGET=yes

.installed:
	cd $(SOURCE); make install HOSTPYTHON=./hostpython CROSS_COMPILE_TARGET=yes
	find $(PREFIX)/lib/python2.6 -type f -name *.pyc -exec rm -f {} \;	
	find $(PREFIX)/lib/python2.6 -type f -name *.pyo -exec rm -f {} \;	
	#find $(PREFIX)/lib/python2.6 -type d -name *test* -exec rm -rf {} \;	
	touch $@

clean:
	rm -rf $(SOURCE) .installed

distclean::
	rm -rf $(SOURCE) .installed

