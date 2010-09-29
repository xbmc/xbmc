AM_CFLAGS = @CFLAGS@

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libapetag.la
include_HEADERS = apetaglib.h

libapetag_la_SOURCES = \
    apetaglib.c \
    is_tag.c \
    info_mac.c \
    info_mpc.c \
    file_io.c

if COMPILE_WITH_ID3
    libapetag_la_SOURCES += id3v2_read.c
endif

libapetag_la_LDFLAGS = -version-info @APETAG_VERSION@

EXTRA_DIST = \
    COPYING.LGPL \
    README.apetag
