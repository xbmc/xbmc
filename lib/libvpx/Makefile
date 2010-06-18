#
#  libvpx build script.
#
#  A quick and dirty Makefile to download and build libvpx
#
#  Darwin: libvpx is built as an static lib for linkage with FFmpeg.
#  x86-darwin8-gcc is targeted so we can run on the AppleTV.
#  prefix is /opt/local so XBMC's build system will pick it up.
#  
#  Usage:
#    make

LIBVPX_VERS= 0.9.1
LIBVPX_SRC = libvpx-$(LIBVPX_VERS)
LIBVPX_LIB = $(LIBVPX_SRC)/libvpx.a

# get OS type from shell
OSTYPE	= $(shell uname)
ifeq ($(OSTYPE),Darwin)
PREFIX = --prefix=/opt/local
TARGET = --target=x86-darwin8-gcc
DOWNLOAD_SRC = curl --output $(LIBVPX_SRC).tar.bz2 http://webm.googlecode.com/files/$(LIBVPX_SRC).tar.bz2
else
DOWNLOAD_SRC = wget http://webm.googlecode.com/files/$(LIBVPX_SRC).tar.bz2
endif

all:: $(LIBVPX_LIB)

$(LIBVPX_LIB): $(LIBVPX_SRC)/Makefile
	make -C $(LIBVPX_SRC)

$(LIBVPX_SRC)/Makefile:
	$(DOWNLOAD_SRC)
	tar -xjf $(LIBVPX_SRC).tar.bz2
	cd $(LIBVPX_SRC); ./configure $(PREFIX) $(TARGET) --disable-examples --enable-vp8

install:
	make -C $(LIBVPX_SRC) install
clean:
	make -C $(LIBVPX_SRC) clean

distclean::
	rm $(LIBVPX_SRC).tar.bz2
	rm -rf $(LIBVPX_SRC)
