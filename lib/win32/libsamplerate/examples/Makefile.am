# This file was automatically generated
# DO NOT EDIT!

bin_PROGRAMS = sndfile-resample

noinst_PROGRAMS = varispeed-play timewarp-file

SAMPLERATEDIR =../src
INCLUDES = -I$(srcdir)/$(SAMPLERATEDIR) @OS_SPECIFIC_INCLUDES@

OS_SPECIFIC_LINKS = @OS_SPECIFIC_LINKS@

SNDFILE_LIBS = @SNDFILE_LIBS@
AM_CFLAGS = @SNDFILE_CFLAGS@

#===============================================================================

sndfile_resample_SOURCES = sndfile-resample.c
sndfile_resample_LDADD = $(SAMPLERATEDIR)/libsamplerate.la $(SNDFILE_LIBS)

varispeed_play_SOURCES = varispeed-play.c audio_out.c audio_out.h
varispeed_play_LDADD = $(SAMPLERATEDIR)/libsamplerate.la $(SNDFILE_LIBS) $(OS_SPECIFIC_LINKS)

timewarp_file_SOURCES = timewarp-file.c
timewarp_file_LDADD = $(SAMPLERATEDIR)/libsamplerate.la $(SNDFILE_LIBS)

