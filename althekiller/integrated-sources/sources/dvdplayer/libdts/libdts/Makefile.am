AM_CFLAGS = $(OPT_CFLAGS) $(LIBDTS_CFLAGS)

#libtool# lib_LTLIBRARIES = libdts.la
#libtool# 
#libtool# libdts_la_SOURCES = bitstream.c parse.c downmix.c
#libtool# libdts_la_LIBADD = $(LIBDTS_LIBS)
#libtool# libdts_la_LDFLAGS = -no-undefined

lib_LIBRARIES = libdts.a $(libdts_pic_a)
libdts_a_SOURCES = bitstream.c parse.c downmix.c

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdts.pc

EXTRA_DIST = configure.incl dts_internal.h bitstream.h tables.h tables_quantization.h tables_huffman.h tables_fir.h tables_adpcm.h tables_vq.h

if NEED_PIC
libdts_pic_a = libdts_pic.a
endif
libdts_pic_a_SOURCES = $(libdts_a_SOURCES)
libdts_pic_a_CPPFLAGS = -fPIC -DPIC

