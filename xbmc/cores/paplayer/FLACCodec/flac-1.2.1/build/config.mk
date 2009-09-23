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
# debug/release selection
#

DEFAULT_BUILD = release

debug    : BUILD = debug
valgrind : BUILD = debug
release  : BUILD = release

# override LINKAGE on OS X until we figure out how to get 'cc -static' to work
ifeq ($(DARWIN_BUILD),yes)
LINKAGE = 
else
debug    : LINKAGE = -static
valgrind : LINKAGE = -dynamic
release  : LINKAGE = -static
endif

all default: $(DEFAULT_BUILD)

#
# GNU makefile fragment for emulating stuff normally done by configure
#

VERSION=\"1.2.1\"

ifeq ($(DARWIN_BUILD),yes)
CONFIG_CFLAGS=-DHAVE_INTTYPES_H -DHAVE_ICONV -DHAVE_LANGINFO_CODESET -DFLAC__HAS_OGG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DFLAC__SYS_DARWIN -DWORDS_BIGENDIAN
else
CONFIG_CFLAGS=-DHAVE_INTTYPES_H -DHAVE_ICONV -DHAVE_LANGINFO_CODESET -DHAVE_SOCKLEN_T -DFLAC__HAS_OGG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
endif

OGG_INCLUDE_DIR=$(HOME)/local/include
OGG_LIB_DIR=$(HOME)/local/lib
