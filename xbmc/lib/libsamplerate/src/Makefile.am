# This file was automatically generated from the Makefile.am
# DO NOT EDIT!

lib_LTLIBRARIES = libsamplerate.la
include_HEADERS = samplerate.h

EXTRA_DIST = config.h.in Version_script.in check_asm.sh
CLEANFILES = src_sinc.s

COEFF_HDRS = fastest_coeffs.h mid_qual_coeffs.h high_qual_coeffs.h


noinst_HEADERS = common.h float_cast.h $(COEFF_HDRS)

SRC_SOURCES = samplerate.c src_sinc.c $(COEFF_HDRS) src_zoh.c src_linear.c

# MinGW requires -no-undefined if a DLL is to be built.
libsamplerate_la_LDFLAGS = -no-undefined -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@
libsamplerate_la_SOURCES = $(SRC_SOURCES) $(noinst_HEADERS)
libsamplerate_la_LIBADD  = -lm

#-------------------------------------------------------------------------------
# An extra check for bad asm.

check-asm : check_asm.sh src_sinc.s src_linear.s src_zoh.s
	@echo
	@echo
	$(srcdir)/check_asm.sh src_sinc.s
	$(srcdir)/check_asm.sh src_linear.s
	$(srcdir)/check_asm.sh src_zoh.s
	@echo
	@echo

%.s : $(srcdir)/%.c
	$(CC) -S $(CFLAGS) $(DEFAULT_INCLUDES) $(srcdir)/$+ -o $@

# Disable autoheader.
AUTOHEADER=echo

