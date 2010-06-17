#
#  rtmpdump build script.
#
#  A quick and dirty Makefile to download/build and install librtmp
#
#  Darwin: librtmp is built as an .so on OSX for the ability to unload it.
#  10.4u.sdk is targeted so we can run on the AppleTV. This might
#  break under OSX ppc and need some build love. Macports is assumed to
#  be used and the required lib depends installed.
#
#  Linux: builds a simple.so without the proper .so version symlinks,
#  maybe someone with good Linux makefile-fu can fix it up.
#  
#  Usage:
#    make
#    sudo make install

RTMPDUMP_VERS = 509
# get OS type from shell
OSTYPE	= $(shell uname)

ifeq ($(OSTYPE),Darwin)
XCFLAGS=-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk \
	 -fPIC -I /opt/local/include
XLDFLAGS=-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk \
	  -bundle -flat_namespace -undefined suppress -arch i386 -L/opt/local/lib \
	  -lpthread -lssl -lcrypto -lz
else
XCFLAGS=-fPIC -DPIC
XLDFLAGS=-shared -fPIC -rdynamic -lpthread -lssl -lcrypto -lz
endif

LIBRTMP = librtmp/librtmp.a

all:: librtmp.so

librtmp.so: $(LIBRTMP)
ifeq ($(OSTYPE),Darwin)
	$(CC) $(XLDFLAGS) -o $@ -Wl,-all_load $<
else
	$(CC) $(XLDFLAGS) -o $@ -Wl,--whole-archive $< -Wl,--no-whole-archive
endif

$(LIBRTMP): librtmp/Makefile
	make SYS=posix XCFLAGS="$(XCFLAGS)" XLDFLAGS="$(XLDFLAGS)" -C librtmp

librtmp/Makefile:
	svn export --revision $(RTMPDUMP_VERS) svn://svn.mplayerhq.hu/rtmpdump/trunk/librtmp librtmp

install:
	mkdir -p /usr/local/lib
	mkdir -p /usr/local/include/librtmp
	cp -f librtmp/amf.h /usr/local/include/librtmp
	cp -f librtmp/log.h /usr/local/include/librtmp
	cp -f librtmp/rtmp.h /usr/local/include/librtmp
	cp -f librtmp.so /usr/local/lib/

clean:
	rm -f librtmp.so
	rm -rf includes
	make -C librtmp clean

distclean::
	rm -rf librtmp.so includes librtmp
