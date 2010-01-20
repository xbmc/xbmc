## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include @OGG_CFLAGS@

noinst_PROGRAMS = decoder_example encoder_example chaining_example\
		vorbisfile_example seeking_example

# uncomment to build static executables from the example code
#LDFLAGS = -all-static 
LDADD = ../lib/libvorbis.la 

decoder_example_SOURCES = decoder_example.c
encoder_example_SOURCES = encoder_example.c
encoder_example_LDADD = ../lib/libvorbisenc.la ../lib/libvorbis.la 
chaining_example_SOURCES = chaining_example.c
chaining_example_LDADD = ../lib/libvorbisfile.la ../lib/libvorbis.la 
vorbisfile_example_SOURCES = vorbisfile_example.c
vorbisfile_example_LDADD = ../lib/libvorbisfile.la ../lib/libvorbis.la 
seeking_example_SOURCES = seeking_example.c
seeking_example_LDADD = ../lib/libvorbisfile.la ../lib/libvorbis.la 

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
