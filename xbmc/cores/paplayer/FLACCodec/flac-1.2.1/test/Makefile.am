#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
#
#  This file is part the FLAC project.  FLAC is comprised of several
#  components distributed under difference licenses.  The codec libraries
#  are distributed under Xiph.Org's BSD-like license (see the file
#  COPYING.Xiph in this distribution).  All other programs, libraries, and
#  plugins are distributed under the GPL (see COPYING.GPL).  The documentation
#  is distributed under the Gnu FDL (see COPYING.FDL).  Each file in the
#  FLAC distribution contains at the top the terms under which it may be
#  distributed.
#
#  Since this particular file is relevant to all components of FLAC,
#  it may be distributed under the Xiph.Org license, which is the least
#  restrictive of those mentioned above.  See the file COPYING.Xiph in this
#  distribution.

if FLaC__WITH_CPPLIBS
CPPLIBS_TESTS = test_libFLAC++.sh
endif

TESTS_ENVIRONMENT = FLAC__TEST_LEVEL=@FLAC__TEST_LEVEL@ FLAC__TEST_WITH_VALRGIND=@FLAC__TEST_WITH_VALGRIND@

SUBDIRS = cuesheets flac-to-flac-metadata-test-files metaflac-test-files pictures

TESTS = \
	./test_libFLAC.sh \
	$(CPPLIBS_TESTS) \
	./test_grabbag.sh \
	./test_flac.sh \
	./test_metaflac.sh \
	./test_seeking.sh \
	./test_streams.sh

EXTRA_DIST = \
	Makefile.lite \
	cuesheet.ok \
	metaflac.flac.in \
	metaflac.flac.ok \
	picture.ok \
	test_libFLAC.sh \
	$(CPPLIBS_TESTS) \
	test_flac.sh \
	test_metaflac.sh \
	test_grabbag.sh \
	test_seeking.sh \
	test_streams.sh \
	test_bins.sh

clean-local:
	-rm -f *.raw *.flac *.oga *.ogg *.cmp *.aiff *.wav *.diff *.log *.cue core
