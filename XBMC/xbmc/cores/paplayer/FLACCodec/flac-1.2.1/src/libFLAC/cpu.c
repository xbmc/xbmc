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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "private/cpu.h"
#include <stdlib.h>
#include <stdio.h>

#if defined FLAC__CPU_IA32
# include <signal.h>
#elif defined FLAC__CPU_PPC
# if !defined FLAC__NO_ASM
#  if defined FLAC__SYS_DARWIN
#   include <sys/sysctl.h>
#   include <mach/mach.h>
#   include <mach/mach_host.h>
#   include <mach/host_info.h>
#   include <mach/machine.h>
#   ifndef CPU_SUBTYPE_POWERPC_970
#    define CPU_SUBTYPE_POWERPC_970 ((cpu_subtype_t) 100)
#   endif
#  else /* FLAC__SYS_DARWIN */

#   include <signal.h>
#   include <setjmp.h>

static sigjmp_buf jmpbuf;
static volatile sig_atomic_t canjump = 0;

static void sigill_handler (int sig)
{
	if (!canjump) {
		signal (sig, SIG_DFL);
		raise (sig);
	}
	canjump = 0;
	siglongjmp (jmpbuf, 1);
}
#  endif /* FLAC__SYS_DARWIN */
# endif /* FLAC__NO_ASM */
#endif /* FLAC__CPU_PPC */

#if defined (__NetBSD__) || defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(__APPLE__)
/* how to get sysctlbyname()? */
#endif

/* these are flags in EDX of CPUID AX=00000001 */
static const unsigned FLAC__CPUINFO_IA32_CPUID_CMOV = 0x00008000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_MMX = 0x00800000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_FXSR = 0x01000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE = 0x02000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE2 = 0x04000000;
/* these are flags in ECX of CPUID AX=00000001 */
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE3 = 0x00000001;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSSE3 = 0x00000200;
/* these are flags in EDX of CPUID AX=80000001 */
static const unsigned FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_3DNOW = 0x80000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXT3DNOW = 0x40000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXTMMX = 0x00400000;


/*
 * Extra stuff needed for detection of OS support for SSE on IA-32
 */
#if defined(FLAC__CPU_IA32) && !defined FLAC__NO_ASM && defined FLAC__HAS_NASM && !defined FLAC__NO_SSE_OS && !defined FLAC__SSE_OS
# if defined(__linux__)
/*
 * If the OS doesn't support SSE, we will get here with a SIGILL.  We
 * modify the return address to jump over the offending SSE instruction
 * and also the operation following it that indicates the instruction
 * executed successfully.  In this way we use no global variables and
 * stay thread-safe.
 *
 * 3 + 3 + 6:
 *   3 bytes for "xorps xmm0,xmm0"
 *   3 bytes for estimate of how long the follwing "inc var" instruction is
 *   6 bytes extra in case our estimate is wrong
 * 12 bytes puts us in the NOP "landing zone"
 */
#  undef USE_OBSOLETE_SIGCONTEXT_FLAVOR /* #define this to use the older signal handler method */
#  ifdef USE_OBSOLETE_SIGCONTEXT_FLAVOR
	static void sigill_handler_sse_os(int signal, struct sigcontext sc)
	{
		(void)signal;
		sc.eip += 3 + 3 + 6;
	}
#  else
#   include <sys/ucontext.h>
	static void sigill_handler_sse_os(int signal, siginfo_t *si, void *uc)
	{
		(void)signal, (void)si;
		((ucontext_t*)uc)->uc_mcontext.gregs[14/*REG_EIP*/] += 3 + 3 + 6;
	}
#  endif
# elif defined(_MSC_VER)
#  include <windows.h>
#  undef USE_TRY_CATCH_FLAVOR /* #define this to use the try/catch method for catching illegal opcode exception */
#  ifdef USE_TRY_CATCH_FLAVOR
#  else
	LONG CALLBACK sigill_handler_sse_os(EXCEPTION_POINTERS *ep)
	{
		if(ep->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
			ep->ContextRecord->Eip += 3 + 3 + 6;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}
#  endif
# endif
#endif


void FLAC__cpu_info(FLAC__CPUInfo *info)
{
/*
 * IA32-specific
 */
#ifdef FLAC__CPU_IA32
	info->type = FLAC__CPUINFO_TYPE_IA32;
#if !defined FLAC__NO_ASM && defined FLAC__HAS_NASM
	info->use_asm = true; /* we assume a minimum of 80386 with FLAC__CPU_IA32 */
	info->data.ia32.cpuid = FLAC__cpu_have_cpuid_asm_ia32()? true : false;
	info->data.ia32.bswap = info->data.ia32.cpuid; /* CPUID => BSWAP since it came after */
	info->data.ia32.cmov = false;
	info->data.ia32.mmx = false;
	info->data.ia32.fxsr = false;
	info->data.ia32.sse = false;
	info->data.ia32.sse2 = false;
	info->data.ia32.sse3 = false;
	info->data.ia32.ssse3 = false;
	info->data.ia32._3dnow = false;
	info->data.ia32.ext3dnow = false;
	info->data.ia32.extmmx = false;
	if(info->data.ia32.cpuid) {
		/* http://www.sandpile.org/ia32/cpuid.htm */
		FLAC__uint32 flags_edx, flags_ecx;
		FLAC__cpu_info_asm_ia32(&flags_edx, &flags_ecx);
		info->data.ia32.cmov  = (flags_edx & FLAC__CPUINFO_IA32_CPUID_CMOV )? true : false;
		info->data.ia32.mmx   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_MMX  )? true : false;
		info->data.ia32.fxsr  = (flags_edx & FLAC__CPUINFO_IA32_CPUID_FXSR )? true : false;
		info->data.ia32.sse   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_SSE  )? true : false;
		info->data.ia32.sse2  = (flags_edx & FLAC__CPUINFO_IA32_CPUID_SSE2 )? true : false;
		info->data.ia32.sse3  = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSE3 )? true : false;
		info->data.ia32.ssse3 = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSSE3)? true : false;

#ifdef FLAC__USE_3DNOW
		flags_edx = FLAC__cpu_info_extended_amd_asm_ia32();
		info->data.ia32._3dnow   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_3DNOW   )? true : false;
		info->data.ia32.ext3dnow = (flags_edx & FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXT3DNOW)? true : false;
		info->data.ia32.extmmx   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXTMMX  )? true : false;
#else
		info->data.ia32._3dnow = info->data.ia32.ext3dnow = info->data.ia32.extmmx = false;
#endif

#ifdef DEBUG
		fprintf(stderr, "CPU info (IA-32):\n");
		fprintf(stderr, "  CPUID ...... %c\n", info->data.ia32.cpuid   ? 'Y' : 'n');
		fprintf(stderr, "  BSWAP ...... %c\n", info->data.ia32.bswap   ? 'Y' : 'n');
		fprintf(stderr, "  CMOV ....... %c\n", info->data.ia32.cmov    ? 'Y' : 'n');
		fprintf(stderr, "  MMX ........ %c\n", info->data.ia32.mmx     ? 'Y' : 'n');
		fprintf(stderr, "  FXSR ....... %c\n", info->data.ia32.fxsr    ? 'Y' : 'n');
		fprintf(stderr, "  SSE ........ %c\n", info->data.ia32.sse     ? 'Y' : 'n');
		fprintf(stderr, "  SSE2 ....... %c\n", info->data.ia32.sse2    ? 'Y' : 'n');
		fprintf(stderr, "  SSE3 ....... %c\n", info->data.ia32.sse3    ? 'Y' : 'n');
		fprintf(stderr, "  SSSE3 ...... %c\n", info->data.ia32.ssse3   ? 'Y' : 'n');
		fprintf(stderr, "  3DNow! ..... %c\n", info->data.ia32._3dnow  ? 'Y' : 'n');
		fprintf(stderr, "  3DNow!-ext . %c\n", info->data.ia32.ext3dnow? 'Y' : 'n');
		fprintf(stderr, "  3DNow!-MMX . %c\n", info->data.ia32.extmmx  ? 'Y' : 'n');
#endif

		/*
		 * now have to check for OS support of SSE/SSE2
		 */
		if(info->data.ia32.fxsr || info->data.ia32.sse || info->data.ia32.sse2) {
#if defined FLAC__NO_SSE_OS
			/* assume user knows better than us; turn it off */
			info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
#elif defined FLAC__SSE_OS
			/* assume user knows better than us; leave as detected above */
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__APPLE__)
			int sse = 0;
			size_t len;
			/* at least one of these must work: */
			len = sizeof(sse); sse = sse || (sysctlbyname("hw.instruction_sse", &sse, &len, NULL, 0) == 0 && sse);
			len = sizeof(sse); sse = sse || (sysctlbyname("hw.optional.sse"   , &sse, &len, NULL, 0) == 0 && sse); /* __APPLE__ ? */
			if(!sse)
				info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
#elif defined(__NetBSD__) || defined (__OpenBSD__)
# if __NetBSD_Version__ >= 105250000 || (defined __OpenBSD__)
			int val = 0, mib[2] = { CTL_MACHDEP, CPU_SSE };
			size_t len = sizeof(val);
			if(sysctl(mib, 2, &val, &len, NULL, 0) < 0 || !val)
				info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
			else { /* double-check SSE2 */
				mib[1] = CPU_SSE2;
				len = sizeof(val);
				if(sysctl(mib, 2, &val, &len, NULL, 0) < 0 || !val)
					info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
			}
# else
			info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
# endif
#elif defined(__linux__)
			int sse = 0;
			struct sigaction sigill_save;
#ifdef USE_OBSOLETE_SIGCONTEXT_FLAVOR
			if(0 == sigaction(SIGILL, NULL, &sigill_save) && signal(SIGILL, (void (*)(int))sigill_handler_sse_os) != SIG_ERR)
#else
			struct sigaction sigill_sse;
			sigill_sse.sa_sigaction = sigill_handler_sse_os;
			__sigemptyset(&sigill_sse.sa_mask);
			sigill_sse.sa_flags = SA_SIGINFO | SA_RESETHAND; /* SA_RESETHAND just in case our SIGILL return jump breaks, so we don't get stuck in a loop */
			if(0 == sigaction(SIGILL, &sigill_sse, &sigill_save))
#endif
			{
				/* http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html */
				/* see sigill_handler_sse_os() for an explanation of the following: */
				asm volatile (
					"xorl %0,%0\n\t"          /* for some reason, still need to do this to clear 'sse' var */
					"xorps %%xmm0,%%xmm0\n\t" /* will cause SIGILL if unsupported by OS */
					"incl %0\n\t"             /* SIGILL handler will jump over this */
					/* landing zone */
					"nop\n\t" /* SIGILL jump lands here if "inc" is 9 bytes */
					"nop\n\t"
					"nop\n\t"
					"nop\n\t"
					"nop\n\t"
					"nop\n\t"
					"nop\n\t" /* SIGILL jump lands here if "inc" is 3 bytes (expected) */
					"nop\n\t"
					"nop"     /* SIGILL jump lands here if "inc" is 1 byte */
					: "=r"(sse)
					: "r"(sse)
				);

				sigaction(SIGILL, &sigill_save, NULL);
			}

			if(!sse)
				info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
#elif defined(_MSC_VER)
# ifdef USE_TRY_CATCH_FLAVOR
			_try {
				__asm {
#  if _MSC_VER <= 1200
					/* VC6 assembler doesn't know SSE, have to emit bytecode instead */
					_emit 0x0F
					_emit 0x57
					_emit 0xC0
#  else
					xorps xmm0,xmm0
#  endif
				}
			}
			_except(EXCEPTION_EXECUTE_HANDLER) {
				if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
					info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
			}
# else
			int sse = 0;
			LPTOP_LEVEL_EXCEPTION_FILTER save = SetUnhandledExceptionFilter(sigill_handler_sse_os);
			/* see GCC version above for explanation */
			/*  http://msdn2.microsoft.com/en-us/library/4ks26t93.aspx */
			/*  http://www.codeproject.com/cpp/gccasm.asp */
			/*  http://www.hick.org/~mmiller/msvc_inline_asm.html */
			__asm {
#  if _MSC_VER <= 1200
				/* VC6 assembler doesn't know SSE, have to emit bytecode instead */
				_emit 0x0F
				_emit 0x57
				_emit 0xC0
#  else
				xorps xmm0,xmm0
#  endif
				inc sse
				nop
				nop
				nop
				nop
				nop
				nop
				nop
				nop
				nop
			}
			SetUnhandledExceptionFilter(save);
			if(!sse)
				info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
# endif
#else
			/* no way to test, disable to be safe */
			info->data.ia32.fxsr = info->data.ia32.sse = info->data.ia32.sse2 = info->data.ia32.sse3 = info->data.ia32.ssse3 = false;
#endif
#ifdef DEBUG
		fprintf(stderr, "  SSE OS sup . %c\n", info->data.ia32.sse     ? 'Y' : 'n');
#endif

		}
	}
#else
	info->use_asm = false;
#endif

/*
 * PPC-specific
 */
#elif defined FLAC__CPU_PPC
	info->type = FLAC__CPUINFO_TYPE_PPC;
# if !defined FLAC__NO_ASM
	info->use_asm = true;
#  ifdef FLAC__USE_ALTIVEC
#   if defined FLAC__SYS_DARWIN
	{
		int val = 0, mib[2] = { CTL_HW, HW_VECTORUNIT };
		size_t len = sizeof(val);
		info->data.ppc.altivec = !(sysctl(mib, 2, &val, &len, NULL, 0) || !val);
	}
	{
		host_basic_info_data_t hostInfo;
		mach_msg_type_number_t infoCount;

		infoCount = HOST_BASIC_INFO_COUNT;
		host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostInfo, &infoCount);

		info->data.ppc.ppc64 = (hostInfo.cpu_type == CPU_TYPE_POWERPC) && (hostInfo.cpu_subtype == CPU_SUBTYPE_POWERPC_970);
	}
#   else /* FLAC__USE_ALTIVEC && !FLAC__SYS_DARWIN */
	{
		/* no Darwin, do it the brute-force way */
		/* @@@@@@ this is not thread-safe; replace with SSE OS method above or remove */
		info->data.ppc.altivec = 0;
		info->data.ppc.ppc64 = 0;

		signal (SIGILL, sigill_handler);
		canjump = 0;
		if (!sigsetjmp (jmpbuf, 1)) {
			canjump = 1;

			asm volatile (
				"mtspr 256, %0\n\t"
				"vand %%v0, %%v0, %%v0"
				:
				: "r" (-1)
			);

			info->data.ppc.altivec = 1;
		}
		canjump = 0;
		if (!sigsetjmp (jmpbuf, 1)) {
			int x = 0;
			canjump = 1;

			/* PPC64 hardware implements the cntlzd instruction */
			asm volatile ("cntlzd %0, %1" : "=r" (x) : "r" (x) );

			info->data.ppc.ppc64 = 1;
		}
		signal (SIGILL, SIG_DFL); /*@@@@@@ should save and restore old signal */
	}
#   endif
#  else /* !FLAC__USE_ALTIVEC */
	info->data.ppc.altivec = 0;
	info->data.ppc.ppc64 = 0;
#  endif
# else
	info->use_asm = false;
# endif

/*
 * unknown CPI
 */
#else
	info->type = FLAC__CPUINFO_TYPE_UNKNOWN;
	info->use_asm = false;
#endif
}
