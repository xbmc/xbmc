AM_CFLAGS = $(MPEG2DEC_CFLAGS) $(LIBVO_CFLAGS)

libmpeg2 = $(top_builddir)/libmpeg2/libmpeg2.la
libmpeg2convert = $(top_builddir)/libmpeg2/convert/libmpeg2convert.la
libvo = $(top_builddir)/libvo/libvo.a $(LIBVO_LIBS)

bin_PROGRAMS = mpeg2dec extract_mpeg2 corrupt_mpeg2
mpeg2dec_SOURCES = mpeg2dec.c dump_state.c getopt.c gettimeofday.c
mpeg2dec_LDADD = $(libvo) $(libmpeg2) $(libmpeg2convert)
extract_mpeg2_SOURCES = extract_mpeg2.c getopt.c
corrupt_mpeg2_SOURCES = corrupt_mpeg2.c getopt.c

man_MANS = mpeg2dec.1 extract_mpeg2.1

EXTRA_DIST = getopt.h gettimeofday.h $(man_MANS)
