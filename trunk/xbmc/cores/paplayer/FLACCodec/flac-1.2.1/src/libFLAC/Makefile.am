#  libFLAC - Free Lossless Audio Codec library
#  Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  - Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  - Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  - Neither the name of the Xiph.org Foundation nor the names of its
#  contributors may be used to endorse or promote products derived from
#  this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

lib_LTLIBRARIES = libFLAC.la
if DEBUG
DEBUGCFLAGS = -DFLAC__OVERFLOW_DETECT
endif
if FLaC__CPU_PPC
# The -force_cpusubtype_ALL is needed to insert a ppc64 instruction
# into cpu.c with an asm().
if FLaC__SYS_DARWIN
#@@@ PPC optimizations temporarily disabled
CPUCFLAGS = -faltivec -force_cpusubtype_ALL -DFLAC__NO_ASM
else
# Linux-gcc for PPC does not have -force_cpusubtype_ALL, it is Darwin-specific
#@@@ PPC optimizations temporarily disabled
CPUCFLAGS = -maltivec -mabi=altivec -DFLAC__NO_ASM
endif
endif

AM_CFLAGS = $(DEBUGCFLAGS) $(CPUCFLAGS) @OGG_CFLAGS@

if FLaC__NO_ASM
else
if FLaC__CPU_IA32
if FLaC__HAS_NASM
ARCH_SUBDIRS = ia32
LOCAL_EXTRA_LIBADD = ia32/libFLAC-asm.la
endif
endif
if FLaC__CPU_PPC
ARCH_SUBDIRS = ppc
if FLaC__HAS_AS__TEMPORARILY_DISABLED
LOCAL_EXTRA_LIBADD = ppc/as/libFLAC-asm.la
LOCAL_EXTRA_LDFLAGS = "-Wl,-read_only_relocs,warning"
else
if FLaC__HAS_GAS__TEMPORARILY_DISABLED
LOCAL_EXTRA_LIBADD = ppc/gas/libFLAC-asm.la
LOCAL_EXTRA_LDFLAGS = ""
endif
endif
endif
endif

libFLAC_la_LIBADD = $(LOCAL_EXTRA_LIBADD) @OGG_LIBS@

SUBDIRS = $(ARCH_SUBDIRS) include .

m4datadir = $(datadir)/aclocal
m4data_DATA = libFLAC.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = flac.pc

EXTRA_DIST = \
	Makefile.lite \
	flac.pc.in \
	libFLAC_dynamic.dsp \
	libFLAC_dynamic.vcproj \
	libFLAC_static.dsp \
	libFLAC_static.vcproj \
	libFLAC.m4

if FLaC__HAS_OGG
extra_ogg_sources = \
	ogg_decoder_aspect.c \
	ogg_encoder_aspect.c \
	ogg_helper.c \
	ogg_mapping.c
endif
# see 'http://www.gnu.org/software/libtool/manual.html#Libtool-versioning' for numbering convention
libFLAC_la_LDFLAGS = -version-info 10:0:2 -lm $(LOCAL_EXTRA_LDFLAGS)
libFLAC_la_SOURCES = \
	bitmath.c \
	bitreader.c \
	bitwriter.c \
	cpu.c \
	crc.c \
	fixed.c \
	float.c \
	format.c \
	lpc.c \
	md5.c \
	memory.c \
	metadata_iterators.c \
	metadata_object.c \
	stream_decoder.c \
	stream_encoder.c \
	stream_encoder_framing.c \
	window.c \
	$(extra_ogg_sources)
