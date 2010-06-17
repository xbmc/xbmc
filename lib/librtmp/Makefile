#
#  rtmpdump build script.
#  Usage:
#    make
#

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
	mkdir -p includes
	cp -f librtmp/amf.h includes/
	cp -f librtmp/log.h includes/
	cp -f librtmp/rtmp.h includes/
ifeq ($(OSTYPE),Darwin)
	$(CC) $(XLDFLAGS) -o $@ -Wl,-all_load $<
else
	$(CC) $(XLDFLAGS) -o $@ -Wl,--whole-archive $< -Wl,--no-whole-archive
endif

$(LIBRTMP): librtmp/Makefile
	make SYS=posix XCFLAGS="$(XCFLAGS)" XLDFLAGS="$(XLDFLAGS)" -C librtmp

librtmp/Makefile:
	svn export --revision $(RTMPDUMP_VERS) svn://svn.mplayerhq.hu/rtmpdump/trunk/librtmp librtmp

clean:
	rm -f librtmp.so
	rm -rf includes
	make -C librtmp clean

distclean::
	rm -rf librtmp.so includes librtmp
