/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLAC__PRIVATE__CPU_H
#define FLAC__PRIVATE__CPU_H

#include "FLAC/ordinals.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef enum {
	FLAC__CPUINFO_TYPE_IA32,
	FLAC__CPUINFO_TYPE_PPC,
	FLAC__CPUINFO_TYPE_UNKNOWN
} FLAC__CPUInfo_Type;

typedef struct {
	FLAC__bool cpuid;
	FLAC__bool bswap;
	FLAC__bool cmov;
	FLAC__bool mmx;
	FLAC__bool fxsr;
	FLAC__bool sse;
	FLAC__bool sse2;
	FLAC__bool sse3;
	FLAC__bool ssse3;
	FLAC__bool _3dnow;
	FLAC__bool ext3dnow;
	FLAC__bool extmmx;
} FLAC__CPUInfo_IA32;

typedef struct {
	FLAC__bool altivec;
	FLAC__bool ppc64;
} FLAC__CPUInfo_PPC;

typedef struct {
	FLAC__bool use_asm;
	FLAC__CPUInfo_Type type;
	union {
		FLAC__CPUInfo_IA32 ia32;
		FLAC__CPUInfo_PPC ppc;
	} data;
} FLAC__CPUInfo;

void FLAC__cpu_info(FLAC__CPUInfo *info);

#ifndef FLAC__NO_ASM
#ifdef FLAC__CPU_IA32
#ifdef FLAC__HAS_NASM
FLAC__uint32 FLAC__cpu_have_cpuid_asm_ia32(void);
void         FLAC__cpu_info_asm_ia32(FLAC__uint32 *flags_edx, FLAC__uint32 *flags_ecx);
FLAC__uint32 FLAC__cpu_info_extended_amd_asm_ia32(void);
#endif
#endif
#endif

#endif
