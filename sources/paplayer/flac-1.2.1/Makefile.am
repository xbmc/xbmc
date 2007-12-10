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

#
# automake provides the following useful targets:
#
# all: build all programs and libraries using the current
# configuration (set by configure)
#
# check: build and run all self-tests
#
# clean: remove everything except what's required to build everything
#
# distclean: remove everything except what goes in the distribution
#

AUTOMAKE_OPTIONS = foreign 1.7

SUBDIRS = doc include m4 man src examples test build obj

DISTCLEANFILES = libtool-disable-static

EXTRA_DIST = \
	COPYING.FDL \
	COPYING.GPL \
	COPYING.LGPL \
	COPYING.Xiph \
	FLAC.dsw \
	FLAC.sln \
	Makefile.lite \
	all.dsp \
	all_dynamic.dsp \
	all_static.dsp \
	autogen.sh \
	config.rpath \
	depcomp \
	ltmain.sh \
	strip_non_asm_libtool_args.sh
