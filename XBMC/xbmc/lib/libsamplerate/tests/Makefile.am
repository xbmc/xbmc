noinst_PROGRAMS = misc_test termination_test simple_test callback_test \
					reset_test multi_channel_test snr_bw_test \
					float_short_test varispeed_test callback_hang_test \
					src-evaluate throughput_test multichan_throughput_test \
					downsample_test

SAMPLRATEDIR =../src
INCLUDES = -I$(srcdir)/$(SAMPLRATEDIR)
noinst_HEADERS = util.h

#===============================================================================

misc_test_SOURCES = misc_test.c util.c util.h
misc_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

termination_test_SOURCES = termination_test.c util.c util.h
termination_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

callback_hang_test_SOURCES = callback_hang_test.c util.c util.h
callback_hang_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

simple_test_SOURCES = simple_test.c util.c util.h
simple_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

reset_test_SOURCES = reset_test.c util.c util.h
reset_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

multi_channel_test_SOURCES = multi_channel_test.c util.c calc_snr.c
multi_channel_test_CFLAGS = @FFTW3_CFLAGS@
multi_channel_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la $(FFTW3_LIBS)

snr_bw_test_SOURCES = snr_bw_test.c calc_snr.c util.c util.h
snr_bw_test_CFLAGS = @FFTW3_CFLAGS@
snr_bw_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la $(FFTW3_LIBS)

callback_test_SOURCES = callback_test.c util.c util.h
callback_test_CFLAGS = @FFTW3_CFLAGS@
callback_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

float_short_test_SOURCES = float_short_test.c util.c util.h
float_short_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

downsample_test_SOURCES = downsample_test.c util.c util.h
downsample_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la

varispeed_test_SOURCES = varispeed_test.c util.c util.h calc_snr.c
varispeed_test_CFLAGS = @FFTW3_CFLAGS@
varispeed_test_LDADD = $(SAMPLRATEDIR)/libsamplerate.la $(FFTW3_LIBS)

# This program is for evaluating other sample rate converters.

throughput_test_CFLAGS = $(AM_CFLAGS) @FFTW3_CFLAGS@
throughput_test_SOURCES = throughput_test.c  util.c calc_snr.c
throughput_test_LDADD  = $(SAMPLRATEDIR)/libsamplerate.la $(FFTW3_LIBS)

multichan_throughput_test_CFLAGS = $(AM_CFLAGS) @FFTW3_CFLAGS@
multichan_throughput_test_SOURCES = multichan_throughput_test.c  util.c calc_snr.c
multichan_throughput_test_LDADD  = $(SAMPLRATEDIR)/libsamplerate.la $(FFTW3_LIBS)

src_evaluate_SOURCES = src-evaluate.c calc_snr.c util.c
src_evaluate_CFLAGS = @SNDFILE_CFLAGS@ @FFTW3_CFLAGS@
src_evaluate_LDADD = $(SNDFILE_LIBS) $(FFTW3_LIBS)

#===============================================================================

check: $(noinst_PROGRAMS)
	date
	uname -a
	./misc_test
	./termination_test
	./callback_hang_test
	./downsample_test
	./simple_test
	./callback_test
	./reset_test
	./multi_channel_test
	./varispeed_test
	./float_short_test
	./snr_bw_test
	./throughput_test
	@echo "-----------------------------------------------------------------"
	@echo "  ${PACKAGE}-${VERSION} passed all tests."
	@echo "-----------------------------------------------------------------"

