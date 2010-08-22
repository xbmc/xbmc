#
#  rtmpdump build script for OSX darwin.
#
#  A quick and dirty Makefile to download/build and install librtmp
#
#  Darwin: librtmp is built as an .so on OSX for the ability to unload it.
#  10.4u.sdk is targeted so we can run on the AppleTV. Macports is assumed to
#  be used and the required lib depends installed.
#
#  Linux: builds using the existing librtmp make system
#  
#  Usage:
#    make
#    sudo make install

# get OS type from shell
OSTYPE	= $(shell uname)

ifeq ($(OSTYPE),Darwin)
  SYS=darwin
  prefix=/opt/local
  XCFLAGS=-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -I /opt/local/include
  XLDFLAGS=-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -L/opt/local/lib
  MACHINE = $(shell uname -m)
  ifeq ($(findstring Power,$(MACHINE)), Power)
    arch = ppc
  else
    arch = i386
  endif
else
  SYS=posix
  prefix=/usr
endif

LIBRTMP = librtmp/librtmp/librtmp.so

all:: librtmp/librtmp/librtmp.so

$(LIBRTMP): librtmp/Makefile
	make SYS=$(SYS) prefix=$(prefix) XCFLAGS="$(XCFLAGS)" XLDFLAGS="$(XLDFLAGS)" -C librtmp/librtmp

librtmp/Makefile:
	svn export svn://svn.mplayerhq.hu/rtmpdump/tags/rel-2.3 librtmp
ifeq ($(OSTYPE),Darwin)
	cd librtmp; patch -p1 < ../make_shared_lib_for_darwin-tag2.3.patch
endif

install:
	make SYS=$(SYS) prefix=$(prefix) -C librtmp/librtmp install

clean:
	make SYS=$(SYS) prefix=$(prefix) -C librtmp/librtmp clean

distclean::
	rm -rf librtmp
