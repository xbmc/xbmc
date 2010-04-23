/***
*intrin.h = declerations/definitions for some platform specific intrinsic stuff.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*Purpose:
*   This include file contains the declerations for platform specific intrinsic
*   functions, or will include other files that have decleration of intrinsic
*   functions. Also there will be some platform specific macros to be used with
*   intrinsic functions.
*
****/

/*
	NOTE : header from Vista SDK, with functions _interlockedbittestandset, _interlockedbittestandreset,
		   _interlockedbittestandset64 and _interlockedbittestandreset64 commented to fix compilation errors
*/

#pragma once
#define __INTRIN_H_
#ifndef RC_INVOKED
#ifndef __midl

#include <crtdefs.h>
#include <setjmp.h>
#include <stddef.h>

#if !defined(_M_CEE_PURE)

#include <emmintrin.h>
#include <xmmintrin.h>
#include <mmintrin.h>

#if defined(_M_IX86)
#include <mm3dnow.h>
#if defined(__cplusplus)
#include <dvec.h>
#include <fvec.h>
#include <ivec.h>
#endif /* defined(__cplusplus) */
#endif

#endif /* !defined(_M_CEE_PURE) */

#if defined(__cplusplus)
extern "C" {
#endif

/*
**	__MACHINE		: all compilers
**	__MACHINEX64: X64 compiler only
**	__MACHINEARMX : ARM XSCALE intrinsics
**      __MACHINECC   : Intel XSCALE Concan
**      __MACHINESA   : ARM (StrongARM) only
**      __MACHINECE   : common intrinsic functions for Windows CE
**	__MACHINEI	: Intel (32 bit x86) and X64
**      __MACHINEIA32 : 32 bit x86 arch only
**	__MACHINEX86X	: X86 Extended intrinsics
**	__MACHINEX86X_NOX64	: X86 Extended intrinsics not supported on X64
**	__MACHINEX86X_NOIA64	: X86 Extended intrinsics not supported on IA64
**	__MACHINEX86X_NOWIN64	: X86 Extended intrinsics not supported on Win64 targets
**	__MACHINEIA64	: IA64 compiler only
**      __MACHINEIW64 : IA32 + Win64 compilers only (__MACHINEI + __MACHINEW64)
**	__MACHINEW64	: WIN64(tm), 64 bit compilers only
**	__MACHINEZ	: nothing
*/

#define __MACHINEX64	__MACHINE
#define __MACHINEARMX	__MACHINE
#define __MACHINECC   __MACHINE
#define __MACHINECE   __MACHINE
#define	__MACHINEI	__MACHINE
#define __MACHINEIA32 __MACHINE
#define	__MACHINEX86X	__MACHINE
#define	__MACHINEX86X_NOX64	__MACHINE
#define	__MACHINEX86X_NOIA64	__MACHINE
#define	__MACHINEX86X_NOWIN64	__MACHINE
#define	__MACHINEIA64	__MACHINE
#define __MACHINESA   __MACHINE
#define	__MACHINEIW64	__MACHINE
#define	__MACHINEW64	__MACHINE
#if defined(_M_CEE_PURE)
/* No intrinsics available to pure managed code */
#define	__MACHINE(X)	__MACHINEZ(X)
#else
#define	__MACHINE(X)	X;
#endif
#define __MACHINEZ(X)	/* NOTHING */

#if !(_M_IX86)
#undef	__MACHINEIA32
#define	__MACHINEIA32	__MACHINEZ
#endif

#if !(_M_IX86 || _M_AMD64 || _M_IA64)
#undef __MACHINEIW64
#define __MACHINEIW64 __MACHINEZ
#endif

#if !(_M_IA64)
#undef	__MACHINEIA64
#define	__MACHINEIA64	__MACHINEZ
#endif

#if !(_M_IA64 || _M_AMD64)
#undef	__MACHINEW64
#define	__MACHINEW64	__MACHINEZ
#endif

#if !(_M_IX86 || _M_AMD64) 
#undef __MACHINEX86X
#define __MACHINEX86X	__MACHINEZ
#endif

#if !(_M_IX86) || _M_AMD64
#undef __MACHINEX86X_NOX64
#define __MACHINEX86X_NOX64 __MACHINEZ
#endif

#if !(_M_IX86) || _M_IA64
#undef __MACHINEX86X_NOIA64
#define __MACHINEX86X_NOIA64 __MACHINEZ
#endif

#if !(_M_IX86) || _M_AMD64 || _M_IA64
#undef __MACHINEX86X_NOWIN64
#define __MACHINEX86X_NOWIN64 __MACHINEZ
#endif

#if !(_M_ARM)
#undef  __MACHINESA
#undef  __MACHINEARMX
#undef  __MACHINECC
#define __MACHINESA  __MACHINEZ
#define __MACHINEARMX  __MACHINEZ
#define __MACHINECC  __MACHINEZ
#endif

#if !(_M_AMD64)
#undef __MACHINEX64
#define __MACHINEX64 __MACHINEZ
#endif

#if !defined(_WIN32_WCE)
#undef __MACHINECE
#define __MACHINECE __MACHINEZ
#endif

__MACHINEIA64(void _AcquireSpinLock(unsigned __int64 *))
__MACHINE(void * __cdecl _alloca(size_t))
__MACHINE(int __cdecl abs(int))
__MACHINEIA64(void __break(int))
__MACHINECE(__int64 __cdecl _abs64(__int64))
__MACHINE(unsigned short __cdecl _byteswap_ushort(unsigned short value))
__MACHINE(unsigned long __cdecl _byteswap_ulong(unsigned long value))
__MACHINE(unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64 value))
__MACHINECE(void __CacheRelease(void *))
__MACHINECE(void __CacheWriteback(void *))
__MACHINECE(_CRTIMP double ceil(double))
__MACHINEX64(_CRTIMP double ceil(double))
__MACHINECE(_CRTIMP double ceil(double))
__MACHINECE(double _CopyDoubleFromInt64(__int64))
__MACHINECE(float _CopyFloatFromInt32(__int32))
__MACHINECE(__int64 _CopyInt64FromDouble(double))
__MACHINECE(__int32 _CopyInt32FromFloat(float))
__MACHINECE(unsigned _CountLeadingOnes(long))
__MACHINECE(unsigned _CountLeadingOnes64(__int64))
__MACHINECE(unsigned _CountLeadingSigns(long))
__MACHINECE(unsigned _CountLeadingSigns64(__int64))
__MACHINECE(unsigned _CountLeadingZeros(long))
__MACHINECE(unsigned _CountLeadingZeros64(__int64))
__MACHINECE(unsigned _CountOneBits(long))
__MACHINECE(unsigned _CountOneBits64(__int64))
__MACHINE(void __cdecl __debugbreak(void))
__MACHINEI(void __cdecl _disable(void))
__MACHINEIA64(void __cdecl _disable(void))
__MACHINEIA64(void __dsrlz(void))
__MACHINEI(__int64 __emul(int,int))
__MACHINEI(unsigned __int64 __emulu(unsigned int,unsigned int))
__MACHINEI(void __cdecl _enable(void))
__MACHINEIA64(void __cdecl _enable(void))
__MACHINEIA64(void __fc(__int64))
__MACHINEIA64(void __fclrf(void))
__MACHINEIA64(void __fsetc(int, int))
__MACHINEIA64(void __fwb(void))
__MACHINEIA64(unsigned __int64 __getReg(int))
__MACHINEIA64(unsigned __int64 __getPSP(void))
__MACHINEIA64(unsigned __int64 __getCFS(void))
__MACHINECE(void __ICacheRefresh(void *))
__MACHINEIA64(long _InterlockedAdd(long volatile *, long))
__MACHINEIA64(long _InterlockedAdd_acq(long volatile *, long))
__MACHINEIA64(long _InterlockedAdd_rel(long volatile *, long))
__MACHINEIA64(__int64 _InterlockedAdd64(__int64 volatile *, __int64))
__MACHINEIA64(__int64 _InterlockedAdd64_acq(__int64 volatile *, __int64))
__MACHINEIA64(__int64 _InterlockedAdd64_rel(__int64 volatile *, __int64))
__MACHINEI(long __cdecl _InterlockedDecrement(long volatile *))
__MACHINEIA64(long _InterlockedDecrement(long volatile *))
__MACHINEIA64(long _InterlockedDecrement_acq(long volatile *))
__MACHINEIA64(long _InterlockedDecrement_rel(long volatile *))
__MACHINEIA64(__int64 _InterlockedDecrement64(__int64 volatile *))
__MACHINEIA64(__int64 _InterlockedDecrement64_acq(__int64 volatile *))
__MACHINEIA64(__int64 _InterlockedDecrement64_rel(__int64 volatile *))
__MACHINEX64(__int64 _InterlockedDecrement64(__int64 volatile *))
__MACHINEI(long _InterlockedExchange(long volatile *, long))
__MACHINEIA64(long _InterlockedExchange(long volatile *, long))
__MACHINEIA64(long _InterlockedExchange_acq(long volatile *, long))
__MACHINESA(long __stdcall _InterlockedExchange(long volatile *, long))
__MACHINEIA64(__int64 _InterlockedExchange64(__int64 volatile *, __int64))
__MACHINEIA64(__int64 _InterlockedExchange64_acq(__int64 volatile *, __int64))
__MACHINEX64(__int64 _InterlockedExchange64(__int64 volatile *, __int64))
__MACHINEIA64(void * _InterlockedExchangePointer(void * volatile *, void *))
__MACHINEIA64(void * _InterlockedExchangePointer_acq(void * volatile *, void volatile *))
__MACHINEX64(void * _InterlockedExchangePointer(void * volatile *, void *))
__MACHINEI(long _InterlockedExchangeAdd(long volatile *, long))
__MACHINEIA64(long _InterlockedExchangeAdd(long volatile *, long))
__MACHINEIA64(long _InterlockedExchangeAdd_acq(long volatile *, long))
__MACHINEIA64(long _InterlockedExchangeAdd_rel(long volatile *, long))
__MACHINEIA64(__int64 _InterlockedExchangeAdd64(__int64 volatile *, __int64))
__MACHINEIA64(__int64 _InterlockedExchangeAdd64_acq(__int64 volatile *, __int64))
__MACHINEIA64(__int64 _InterlockedExchangeAdd64_rel(__int64 volatile *, __int64))
__MACHINEX64(__int64 _InterlockedExchangeAdd64(__int64 volatile *, __int64))
__MACHINEI(long _InterlockedCompareExchange (long volatile *, long, long))
__MACHINEIA64(long _InterlockedCompareExchange (long volatile *, long, long))
__MACHINEIA64(long _InterlockedCompareExchange_acq (long volatile *, long, long))
__MACHINEIA64(long _InterlockedCompareExchange_rel (long volatile *, long, long))
__MACHINEIA64(__int64 _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64))
__MACHINEIA64(__int64 _InterlockedCompareExchange64_acq(__int64 volatile *, __int64, __int64))
__MACHINEIA64(__int64 _InterlockedCompareExchange64_rel(__int64 volatile *, __int64, __int64))
__MACHINEIA64(__int64 _InterlockedCompare64Exchange128(__int64 volatile *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEIA64(__int64 _InterlockedCompare64Exchange128_acq(__int64 volatile *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEIA64(__int64 _InterlockedCompare64Exchange128_rel(__int64 volatile *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEX64(__int64 _InterlockedCompare64Exchange128(__int64 volatile *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEX64(__int64 _InterlockedCompare64Exchange128_acq(__int64 volatile *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEX64(__int64 _InterlockedCompare64Exchange128_rel(__int64 volatile *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEIA64(void *_InterlockedCompareExchangePointer (void * volatile *, void *, void *))
__MACHINEIA64(void *_InterlockedCompareExchangePointer_acq (void * volatile *, void *, void *))
__MACHINEIA64(void *_InterlockedCompareExchangePointer_rel (void * volatile *, void *, void *))
__MACHINEI(__int64 _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64))
__MACHINEX64(void *_InterlockedCompareExchangePointer (void * volatile *, void *, void *))
__MACHINEI(long __cdecl _InterlockedIncrement(long volatile *))
__MACHINEIA64(long _InterlockedIncrement(long volatile *))
__MACHINEIA64(long _InterlockedIncrement_acq(long volatile *))
__MACHINEIA64(long _InterlockedIncrement_rel(long volatile *))
__MACHINEIA64(__int64 _InterlockedIncrement64(__int64 volatile *))
__MACHINEIA64(__int64 _InterlockedIncrement64_acq(__int64 volatile *))
__MACHINEIA64(__int64 _InterlockedIncrement64_rel(__int64 volatile *))
__MACHINEX64(__int64 _InterlockedIncrement64(__int64 volatile *))
__MACHINEIW64(long _InterlockedOr(long volatile *, long))
__MACHINEIW64(char _InterlockedOr8(char volatile *, char))
__MACHINEIW64(short _InterlockedOr16(short volatile *, short))
__MACHINEW64(__int64 _InterlockedOr64(__int64 volatile *, __int64))
__MACHINEIA64(long _InterlockedOr_acq(long volatile *, long))
__MACHINEIA64(char _InterlockedOr8_acq(char volatile *, char))
__MACHINEIA64(short _InterlockedOr16_acq(short volatile *, short))
__MACHINEIA64(__int64 _InterlockedOr64_acq(__int64 volatile *, __int64))
__MACHINEIA64(long _InterlockedOr_rel(long volatile *, long))
__MACHINEIA64(char _InterlockedOr8_rel(char volatile *, char))
__MACHINEIA64(short _InterlockedOr16_rel(short volatile *, short))
__MACHINEIA64(__int64 _InterlockedOr64_rel(__int64 volatile *, __int64))
__MACHINEIW64(long _InterlockedXor(long volatile *, long))
__MACHINEIW64(char _InterlockedXor8(char volatile *, char))
__MACHINEIW64(short _InterlockedXor16(short volatile *, short))
__MACHINEW64(__int64 _InterlockedXor64(__int64 volatile *, __int64))
__MACHINEIA64(long _InterlockedXor_acq(long volatile *, long))
__MACHINEIA64(char _InterlockedXor8_acq(char volatile *, char))
__MACHINEIA64(short _InterlockedXor16_acq(short volatile *, short))
__MACHINEIA64(__int64 _InterlockedXor64_acq(__int64 volatile *, __int64))
__MACHINEIA64(long _InterlockedXor_rel(long volatile *, long))
__MACHINEIA64(char _InterlockedXor8_rel(char volatile *, char))
__MACHINEIA64(short _InterlockedXor16_rel(short volatile *, short))
__MACHINEIA64(__int64 _InterlockedXor64_rel(__int64 volatile *, __int64))
__MACHINEIW64(long _InterlockedAnd(long volatile *, long))
__MACHINEIW64(char _InterlockedAnd8(char volatile *, char))
__MACHINEIW64(short _InterlockedAnd16(short volatile *, short))
__MACHINEW64(__int64 _InterlockedAnd64(__int64 volatile *, __int64))
__MACHINEIA64(long _InterlockedAnd_acq(long volatile *, long))
__MACHINEIA64(char _InterlockedAnd8_acq(char volatile *, char))
__MACHINEIA64(short _InterlockedAnd16_acq(short volatile *, short))
__MACHINEIA64(__int64 _InterlockedAnd64_acq(__int64 volatile *, __int64))
__MACHINEIA64(long _InterlockedAnd_rel(long volatile *, long))
__MACHINEIA64(char _InterlockedAnd8_rel(char volatile *, char))
__MACHINEIA64(short _InterlockedAnd16_rel(short volatile *, short))
__MACHINEIA64(__int64 _InterlockedAnd64_rel(__int64 volatile *, __int64))
__MACHINEIA32(long _InterlockedAddLargeStatistic(__int64 volatile *, long))
__MACHINEI(int __cdecl _inp(unsigned short))
__MACHINEI(int __cdecl inp(unsigned short))
__MACHINEI(unsigned long __cdecl _inpd(unsigned short))
__MACHINEI(unsigned long __cdecl inpd(unsigned short))
__MACHINEI(unsigned short __cdecl _inpw(unsigned short))
__MACHINEI(unsigned short __cdecl inpw(unsigned short))
__MACHINEIA64(int __isNat(int))
__MACHINEIA64(void __isrlz(void))
__MACHINEIA64(void __invalat(void))
__MACHINECE(int _isnan(double))
__MACHINECE(int _isnanf(float))
__MACHINECE(int _isunordered(double,double))
__MACHINECE(int _isunorderedf(float,float))
__MACHINE(long __cdecl labs(long))
__MACHINEIA64(void __lfetch(int, void const *))
__MACHINEIA64(void __lfetchfault(int, void const *))
__MACHINEIA64(void __lfetch_excl(int, void const *))
__MACHINEIA64(void __lfetchfault_excl(int, void const *))
__MACHINEIA64(__int64 __load128(void *, __int64 *))
__MACHINEIA64(__int64 __load128_acq(void *, __int64 *))
__MACHINEZ(void __cdecl longjmp(jmp_buf, int))
__MACHINE(unsigned long __cdecl _lrotl(unsigned long,int))
__MACHINE(unsigned long __cdecl _lrotr(unsigned long,int))
__MACHINEI(unsigned __int64  __ll_lshift(unsigned __int64,int))
__MACHINEI(__int64  __ll_rshift(__int64,int))
__MACHINEIA64(__m64 __m64_czx1l(__m64))
__MACHINEIA64(__m64 __m64_czx1r(__m64))
__MACHINEIA64(__m64 __m64_czx2l(__m64))
__MACHINEIA64(__m64 __m64_czx2r(__m64))
__MACHINEIA64(__m64 __m64_dep_mi(const int,__m64,const int,const int))
__MACHINEIA64(__m64 __m64_dep_mr(__m64,__m64,const int,const int))
__MACHINEIA64(__m64 __m64_dep_zi(const int,const int,const int))
__MACHINEIA64(__m64 __m64_dep_zr(__m64,const int,const int))
__MACHINEIA64(__m64 __m64_extr(__m64,const int,const int))
__MACHINEIA64(__m64 __m64_extru(__m64,const int,const int))
__MACHINEIA64(__m64 __m64_mix1l(__m64,__m64))
__MACHINEIA64(__m64 __m64_mix1r(__m64,__m64))
__MACHINEIA64(__m64 __m64_mix2l(__m64,__m64))
__MACHINEIA64(__m64 __m64_mix2r(__m64,__m64))
__MACHINEIA64(__m64 __m64_mix4l(__m64,__m64))
__MACHINEIA64(__m64 __m64_mix4r(__m64,__m64))
__MACHINEIA64(__m64 __m64_mux1(__m64,const int))
__MACHINEIA64(__m64 __m64_mux2(__m64,const int))
__MACHINEIA64(__m64 __m64_muladd64hi(__m64,__m64,__m64))
__MACHINEIA64(__m64 __m64_muladd64hi_u(__m64,__m64,__m64))
__MACHINEIA64(__m64 __m64_muladd64lo(__m64,__m64,__m64))
__MACHINEIA64(__m64 __m64_padd1uus(__m64,__m64))
__MACHINEIA64(__m64 __m64_padd2uus(__m64,__m64))
__MACHINEIA64(__m64 __m64_pavg1_nraz(__m64,__m64))
__MACHINEIA64(__m64 __m64_pavg2_nraz(__m64,__m64))
__MACHINEIA64(__m64 __m64_pavgsub1(__m64,__m64))
__MACHINEIA64(__m64 __m64_pavgsub2(__m64,__m64))
__MACHINEIA64(__m64 __m64_pmpy2l(__m64,__m64))
__MACHINEIA64(__m64 __m64_pmpy2r(__m64,__m64))
__MACHINEIA64(__m64 __m64_pmpyshr2(__m64,__m64,const int))
__MACHINEIA64(__m64 __m64_pmpyshr2u(__m64,__m64,const int))
__MACHINEIA64(__m64 __m64_popcnt(__m64))
__MACHINEIA64(__m64 __m64_pshladd2(__m64,const int,__m64))
__MACHINEIA64(__m64 __m64_pshradd2(__m64,const int,__m64))
__MACHINEIA64(__m64 __m64_psub1uus(__m64,__m64))
__MACHINEIA64(__m64 __m64_psub2uus(__m64,__m64))
__MACHINEIA64(__m64 __m64_shladd(__m64,const int,__m64))
__MACHINEIA64(__m64 __m64_shrp(__m64,__m64,const int))
__MACHINECE(_CONST_RETURN void * __cdecl memchr(const void *,int,size_t))
__MACHINE(int __cdecl memcmp(const void *,const void *,size_t))
__MACHINE(void * __cdecl memcpy(void *,const void *,size_t))
__MACHINE(void * __cdecl memset(void *,int,size_t))
__MACHINEIA64(void __mf(void))
__MACHINEIA64(void __mfa(void))
__MACHINECE(long _MulHigh(long,long))
__MACHINECE(unsigned long _MulUnsignedHigh (unsigned long,unsigned long))
__MACHINEI(int __cdecl _outp(unsigned short,int))
__MACHINEI(int __cdecl outp(unsigned short,int))
__MACHINEI(unsigned long __cdecl _outpd(unsigned short,unsigned long))
__MACHINEI(unsigned long __cdecl outpd(unsigned short,unsigned long))
__MACHINEI(unsigned short __cdecl _outpw(unsigned short,unsigned short))
__MACHINEI(unsigned short __cdecl outpw(unsigned short,unsigned short))
__MACHINECE(void __cdecl __prefetch(unsigned long *addr))
__MACHINEIA64(void __ptcl(__int64, __int64))
__MACHINEIA64(void __ptcg(__int64, __int64))
__MACHINEIA64(void __ptcga(__int64, __int64))
__MACHINEIA64(void __ptrd(__int64, __int64))
__MACHINEIA64(void __ptri(__int64, __int64))
__MACHINEIA64(void *_rdteb(void))
__MACHINEIA64(void *_rdtebex(void))
__MACHINESA(int _ReadStatusReg(int))
__MACHINECE(void _ReadWriteBarrier(void))
__MACHINEIA64(void _ReleaseSpinLock(unsigned __int64 * ))
__MACHINEI(void * _ReturnAddress(void))
__MACHINEIA64(void * _ReturnAddress(void))
__MACHINESA(void * _ReturnAddress(void))
__MACHINECE(void * _ReturnAddress(void))
__MACHINE(unsigned int __cdecl _rotl(unsigned int,int))
__MACHINECE(unsigned __int64 __cdecl _rotl64(unsigned __int64,int))
__MACHINE(unsigned int __cdecl _rotr(unsigned int,int))
__MACHINECE(unsigned __int64 __cdecl _rotr64(unsigned __int64,int))
__MACHINEIA64(void __rsm(int))
__MACHINEIA64(void __rum(int))
__MACHINE(int __cdecl _setjmp(jmp_buf))
__MACHINEIA64(int __cdecl _setjmpex(jmp_buf))
__MACHINEX64(int __cdecl _setjmpex(jmp_buf))
__MACHINEIA64(void __setReg(int,unsigned __int64))
__MACHINEARMX(void _SmulAdd_SL_ACC(int, int))
__MACHINEARMX(void _SmulAddPack_2SW_ACC(int, int))
__MACHINEARMX(void _SmulAddLo_SW_ACC(int, int))
__MACHINEARMX(void _SmulAddHi_SW_ACC(int, int))
__MACHINEARMX(void _SmulAddHiLo_SW_ACC(int,int))
__MACHINEARMX(void _SmulAddLoHi_SW_ACC(int,int))
__MACHINEIA64(void __store128(void *, __int64, __int64))
__MACHINEIA64(void __store128_rel(void *, __int64, __int64))
__MACHINE(char * __cdecl strcat(char *,const char *))
__MACHINE(int __cdecl strcmp(const char *,const char *))
__MACHINE(char * __cdecl strcpy(char *,const char *))
__MACHINE(size_t __cdecl strlen(const char *))
__MACHINECE(int __cdecl strncmp(const char *,const char *,size_t))
__MACHINECE(char * __cdecl strncpy(char *,const char *,size_t))
__MACHINE(char * __cdecl _strset(char *,int))
__MACHINE(char * __cdecl strset(char *,int))
__MACHINEIA64(void __ssm(int))
__MACHINEIA64(void __sum(int))
__MACHINESA(int __swi(unsigned, ...))
__MACHINEIA64(void __synci(void))
__MACHINEIA64(__int64 __thash(__int64))
__MACHINEIA64(__int64 __ttag(__int64))
__MACHINECE(int __trap(int,...))
__MACHINEI(unsigned __int64 __ull_rshift(unsigned __int64,int))
__MACHINEIA64(unsigned __int64 __UMULH(unsigned __int64 a, unsigned __int64 b))
__MACHINECE(wchar_t * __cdecl wcscat(wchar_t *,const wchar_t *))
__MACHINECE(int __cdecl wcscmp(const wchar_t *,const wchar_t *))
__MACHINECE(wchar_t * __cdecl wcscpy(wchar_t *,const wchar_t *))
__MACHINECE(size_t __cdecl wcslen(const wchar_t *))
__MACHINECE(int __cdecl wcsncmp(const wchar_t *, const wchar_t *,size_t))
__MACHINECE(wchar_t * __cdecl wcsncpy(wchar_t *, const wchar_t *,size_t))
__MACHINECE(wchar_t * __cdecl _wcsset(wchar_t *,wchar_t))
__MACHINECE(void _WriteBarrier(void))
__MACHINESA(void _WriteStatusReg(int,int, int))
__MACHINEI(void * _AddressOfReturnAddress(void))
__MACHINEIA64(void __yield(void))
__MACHINEIA64(void __fci(void*))

__MACHINEX86X_NOX64(void  _m_empty(void))
__MACHINEX86X_NOX64(__m64 _m_from_int(int))
__MACHINEX86X_NOX64(int   _m_to_int(__m64))
__MACHINEX86X_NOX64(__m64 _m_packsswb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_packssdw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_packuswb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_punpckhbw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_punpckhwd(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_punpckhdq(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_punpcklbw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_punpcklwd(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_punpckldq(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_paddb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_paddw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_paddd(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_paddsb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_paddsw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_paddusb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_paddusw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psubb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psubw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psubd(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psubsb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psubsw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psubusb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psubusw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pmaddwd(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pmulhw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pmullw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psllw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psllwi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_pslld(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pslldi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_psllq(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psllqi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_psraw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psrawi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_psrad(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psradi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_psrlw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psrlwi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_psrld(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psrldi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_psrlq(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psrlqi(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_pand(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pandn(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_por(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pxor(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pcmpeqb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pcmpeqw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pcmpeqd(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pcmpgtb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pcmpgtw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pcmpgtd(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _mm_setzero_si64(void))
__MACHINEX86X_NOX64(__m64 _mm_set_pi32(int,int))
__MACHINEX86X_NOX64(__m64 _mm_set_pi16(short,short,short,short))
__MACHINEX86X_NOX64(__m64 _mm_set_pi8(char,char,char,char,char,char,char,char))
__MACHINEX86X_NOX64(__m64 _mm_set1_pi32(int))
__MACHINEX86X_NOX64(__m64 _mm_set1_pi16(short))
__MACHINEX86X_NOX64(__m64 _mm_set1_pi8(char))
__MACHINEX86X_NOX64(__m64 _mm_setr_pi32(int,int))
__MACHINEX86X_NOX64(__m64 _mm_setr_pi16(short,short,short,short))
__MACHINEX86X_NOX64(__m64 _mm_setr_pi8(char,char,char,char,char,char,char,char))
__MACHINEX86X_NOX64(int _m_pextrw(__m64,int))
__MACHINEX86X_NOX64(__m64 _m_pinsrw(__m64,int,int))
__MACHINEX86X_NOX64(__m64 _m_pmaxsw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pmaxub(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pminsw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pminub(__m64,__m64))
__MACHINEX86X_NOX64(int _m_pmovmskb(__m64))
__MACHINEX86X_NOX64(__m64 _m_pmulhuw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pshufw(__m64,int))
__MACHINEX86X_NOX64(void _m_maskmovq(__m64,__m64,char*))
__MACHINEX86X_NOX64(__m64 _m_pavgb(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_pavgw(__m64,__m64))
__MACHINEX86X_NOX64(__m64 _m_psadbw(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_from_float(float))
__MACHINEX86X_NOWIN64(float _m_to_float(__m64))
__MACHINEX86X_NOIA64(__m128 _mm_add_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_add_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_sub_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_sub_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_mul_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_mul_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_div_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_div_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_sqrt_ss(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_sqrt_ps(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_rcp_ss(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_rcp_ps(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_rsqrt_ss(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_rsqrt_ps(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_min_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_min_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_max_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_max_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_and_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_andnot_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_or_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_xor_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpeq_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpeq_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmplt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmplt_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmple_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmple_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpgt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpgt_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpge_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpge_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpneq_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpneq_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpnlt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpnlt_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpnle_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpnle_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpngt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpngt_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpnge_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpnge_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpord_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpord_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpunord_ss(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cmpunord_ps(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_comieq_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_comilt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_comile_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_comigt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_comige_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_comineq_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_ucomieq_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_ucomilt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_ucomile_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_ucomigt_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_ucomige_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_ucomineq_ss(__m128,__m128))
__MACHINEX86X_NOIA64(int _mm_cvt_ss2si(__m128))
__MACHINEX86X_NOWIN64(__m64 _mm_cvt_ps2pi(__m128))
__MACHINEX86X_NOIA64(int _mm_cvtt_ss2si(__m128))
__MACHINEX86X_NOWIN64(__m64 _mm_cvtt_ps2pi(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_cvt_si2ss(__m128,int))
__MACHINEX86X_NOWIN64(__m128 _mm_cvt_pi2ps(__m128,__m64))
__MACHINEX86X_NOIA64(__m128 _mm_shuffle_ps(__m128,__m128,unsigned int))
__MACHINEX86X_NOIA64(__m128 _mm_unpackhi_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_unpacklo_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_loadh_pi(__m128,__m64 const*))
__MACHINEX86X_NOIA64(void _mm_storeh_pi(__m64*,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_loadl_pi(__m128,__m64 const*))
__MACHINEX86X_NOIA64(void _mm_storel_pi(__m64*,__m128))
__MACHINEX86X_NOIA64(int _mm_movemask_ps(__m128))
__MACHINEX86X_NOIA64(__m128 _mm_set_ss(float))
__MACHINEX86X_NOIA64(__m128 _mm_set_ps1(float))
__MACHINEX86X_NOIA64(__m128 _mm_set_ps(float,float,float,float))
__MACHINEX86X_NOIA64(__m128 _mm_setr_ps(float,float,float,float))
__MACHINEX86X_NOIA64(__m128 _mm_setzero_ps(void))
__MACHINEX86X_NOIA64(__m128 _mm_load_ss(float const*))
__MACHINEX86X_NOIA64(__m128 _mm_load_ps1(float const*))
__MACHINEX86X_NOIA64(__m128 _mm_load_ps(float const*))
__MACHINEX86X_NOIA64(__m128 _mm_loadr_ps(float const*))
__MACHINEX86X_NOIA64(__m128 _mm_loadu_ps(float const*))
__MACHINEX86X_NOIA64(__m128 _mm_move_ss(__m128,__m128))
__MACHINEX86X_NOIA64(void _mm_store_ss(float*,__m128))
__MACHINEX86X_NOIA64(void _mm_store_ps1(float*,__m128))
__MACHINEX86X_NOIA64(void _mm_store_ps(float*,__m128))
__MACHINEX86X_NOIA64(void _mm_storer_ps(float*,__m128))
__MACHINEX86X_NOIA64(void _mm_storeu_ps(float*,__m128))
__MACHINEX86X_NOIA64(void _mm_prefetch(char const*,int))
__MACHINEX86X_NOWIN64(void _mm_stream_pi(__m64*,__m64))
__MACHINEX86X_NOIA64(void _mm_stream_ps(float*,__m128))
__MACHINEX86X_NOIA64(void _mm_sfence(void))
__MACHINEX86X_NOIA64(unsigned int _mm_getcsr(void))
__MACHINEX86X_NOIA64(void _mm_setcsr(unsigned int))
__MACHINEX86X_NOIA64(__m128 _mm_movelh_ps(__m128,__m128))
__MACHINEX86X_NOIA64(__m128 _mm_movehl_ps(__m128,__m128))
__MACHINEX86X_NOIA64(void  _m_prefetch(void*))
__MACHINEX86X_NOIA64(void  _m_prefetchw(volatile const void*_Source))
__MACHINEX86X_NOWIN64(void  _m_femms(void))
__MACHINEX86X_NOWIN64(__m64 _m_pavgusb(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pf2id(__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfacc(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfadd(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfcmpeq(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfcmpge(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfcmpgt(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfmax(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfmin(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfmul(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfrcp(__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfrcpit1(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfrcpit2(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfrsqrt(__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfrsqit1(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfsub(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfsubr(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pi2fd(__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pmulhrw(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pf2iw(__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfnacc(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pfpnacc(__m64,__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pi2fw(__m64))
__MACHINEX86X_NOWIN64(__m64 _m_pswapd(__m64))
__MACHINEX86X(__m128d _mm_add_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_add_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_div_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_div_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_max_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_max_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_min_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_min_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_mul_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_mul_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_sqrt_sd(__m128d, __m128d))
__MACHINEX86X(__m128d _mm_sqrt_pd(__m128d))
__MACHINEX86X(__m128d _mm_sub_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_sub_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_and_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_andnot_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_or_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_xor_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpeq_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpeq_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmplt_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmplt_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmple_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmple_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpgt_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpgt_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpge_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpge_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpneq_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpneq_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpnlt_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpnlt_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpnle_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpnle_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpngt_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpngt_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpnge_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpnge_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpord_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpord_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpunord_sd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_cmpunord_pd(__m128d,__m128d))
__MACHINEX86X(int _mm_comieq_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_comilt_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_comile_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_comigt_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_comige_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_comineq_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_ucomieq_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_ucomilt_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_ucomile_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_ucomigt_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_ucomige_sd(__m128d,__m128d))
__MACHINEX86X(int _mm_ucomineq_sd(__m128d,__m128d))
__MACHINEX86X(__m128 _mm_cvtpd_ps(__m128d))
__MACHINEX86X(__m128d _mm_cvtps_pd(__m128))
__MACHINEX86X(__m128d _mm_cvtepi32_pd(__m128i))
__MACHINEX86X(__m128i _mm_cvtpd_epi32(__m128d))
__MACHINEX86X(int _mm_cvtsd_si32(__m128d))
__MACHINEX86X(__m128 _mm_cvtsd_ss(__m128,__m128d))
__MACHINEX86X(__m128d _mm_cvtsi32_sd(__m128d,int))
__MACHINEX86X(__m128d _mm_cvtss_sd(__m128d,__m128))
__MACHINEX86X(__m128i _mm_cvttpd_epi32(__m128d))
__MACHINEX86X(int _mm_cvttsd_si32(__m128d))
__MACHINEX86X(__m128 _mm_cvtepi32_ps(__m128i))
__MACHINEX86X(__m128i _mm_cvtps_epi32(__m128))
__MACHINEX86X(__m128i _mm_cvttps_epi32(__m128))
__MACHINEX86X_NOX64(__m64 _mm_cvtpd_pi32(__m128d))
__MACHINEX86X_NOX64(__m64 _mm_cvttpd_pi32(__m128d))
__MACHINEX86X_NOX64(__m128d _mm_cvtpi32_pd(__m64))
__MACHINEX86X(__m128d _mm_unpackhi_pd(__m128d,__m128d))
__MACHINEX86X(__m128d _mm_unpacklo_pd(__m128d,__m128d))
__MACHINEX86X(int _mm_movemask_pd(__m128d))
__MACHINEX86X(__m128d _mm_shuffle_pd(__m128d,__m128d,int))
__MACHINEX86X(__m128d _mm_load_pd(double const*))
__MACHINEX86X(__m128d _mm_load1_pd(double const*))
__MACHINEX86X(__m128d _mm_loadr_pd(double const*))
__MACHINEX86X(__m128d _mm_loadu_pd(double const*))
__MACHINEX86X(__m128d _mm_load_sd(double const*))
__MACHINEX86X(__m128d _mm_loadh_pd(__m128d,double const*))
__MACHINEX86X(__m128d _mm_loadl_pd(__m128d,double const*))
__MACHINEX86X(__m128d _mm_set_sd(double))
__MACHINEX86X(__m128d _mm_set1_pd(double))
__MACHINEX86X(__m128d _mm_set_pd(double,double))
__MACHINEX86X(__m128d _mm_setr_pd(double,double))
__MACHINEX86X(__m128d _mm_setzero_pd(void))
__MACHINEX86X(__m128d _mm_move_sd(__m128d,__m128d))
__MACHINEX86X(void _mm_store_sd(double*,__m128d))
__MACHINEX86X(void _mm_store1_pd(double*,__m128d))
__MACHINEX86X(void _mm_store_pd(double*,__m128d))
__MACHINEX86X(void _mm_storeu_pd(double*,__m128d))
__MACHINEX86X(void _mm_storer_pd(double*,__m128d))
__MACHINEX86X(void _mm_storeh_pd(double*,__m128d))
__MACHINEX86X(void _mm_storel_pd(double*,__m128d))
__MACHINEX86X(__m128i _mm_add_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_add_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_add_epi32(__m128i,__m128i))
__MACHINEX86X_NOX64(__m64 _mm_add_si64(__m64,__m64))
__MACHINEX86X(__m128i _mm_add_epi64(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_adds_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_adds_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_adds_epu8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_adds_epu16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_avg_epu8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_avg_epu16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_madd_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_max_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_max_epu8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_min_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_min_epu8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_mulhi_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_mulhi_epu16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_mullo_epi16(__m128i,__m128i))
__MACHINEX86X_NOX64(__m64 _mm_mul_su32(__m64,__m64))
__MACHINEX86X(__m128i _mm_mul_epu32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_sad_epu8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_sub_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_sub_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_sub_epi32(__m128i,__m128i))
__MACHINEX86X_NOX64(__m64 _mm_sub_si64(__m64,__m64))
__MACHINEX86X(__m128i _mm_sub_epi64(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_subs_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_subs_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_subs_epu8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_subs_epu16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_andnot_si128(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_and_si128(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_or_si128(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_xor_si128(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_slli_si128(__m128i,int))
__MACHINEX86X(__m128i _mm_slli_epi16(__m128i,int))
__MACHINEX86X(__m128i _mm_sll_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_slli_epi32(__m128i,int))
__MACHINEX86X(__m128i _mm_sll_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_slli_epi64(__m128i,int))
__MACHINEX86X(__m128i _mm_sll_epi64(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_srai_epi16(__m128i,int))
__MACHINEX86X(__m128i _mm_sra_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_srai_epi32(__m128i,int))
__MACHINEX86X(__m128i _mm_sra_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_srli_si128(__m128i,int))
__MACHINEX86X(__m128i _mm_srli_epi16(__m128i,int))
__MACHINEX86X(__m128i _mm_srl_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_srli_epi32(__m128i,int))
__MACHINEX86X(__m128i _mm_srl_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_srli_epi64(__m128i,int))
__MACHINEX86X(__m128i _mm_srl_epi64(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmpeq_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmpeq_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmpeq_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmpgt_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmpgt_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmpgt_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmplt_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmplt_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cmplt_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_cvtsi32_si128(int))
__MACHINEX86X(int _mm_cvtsi128_si32(__m128i))
__MACHINEX86X(__m128i _mm_packs_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_packs_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_packus_epi16(__m128i,__m128i))
__MACHINEX86X(int _mm_extract_epi16(__m128i,int))
__MACHINEX86X(__m128i _mm_insert_epi16(__m128i,int,int))
__MACHINEX86X(int _mm_movemask_epi8(__m128i))
__MACHINEX86X(__m128i _mm_shuffle_epi32(__m128i,int))
__MACHINEX86X(__m128i _mm_shufflehi_epi16(__m128i,int))
__MACHINEX86X(__m128i _mm_shufflelo_epi16(__m128i,int))
__MACHINEX86X(__m128i _mm_unpackhi_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_unpackhi_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_unpackhi_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_unpackhi_epi64(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_unpacklo_epi8(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_unpacklo_epi16(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_unpacklo_epi32(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_unpacklo_epi64(__m128i,__m128i))
__MACHINEX86X(__m128i _mm_load_si128(__m128i const*))
__MACHINEX86X(__m128i _mm_loadu_si128(__m128i const*))
__MACHINEX86X(__m128i _mm_loadl_epi64(__m128i const*))
__MACHINEX86X_NOX64(__m128i _mm_set_epi64(__m64,__m64))
__MACHINEX86X(__m128i _mm_set_epi32(int,int,int,int))
__MACHINEX86X(__m128i _mm_set_epi16(short,short,short,short,short,short,short,short))
__MACHINEX86X(__m128i _mm_set_epi8(char,char,char,char,char,char,char,char,char,char,char,char,char,char,char,char))
__MACHINEX86X_NOX64(__m128i _mm_set1_epi64(__m64))
__MACHINEX86X(__m128i _mm_set1_epi32(int))
__MACHINEX86X(__m128i _mm_set1_epi16(short))
__MACHINEX86X(__m128i _mm_set1_epi8(char))
__MACHINEX86X(__m128i _mm_setl_epi64(__m128i))
__MACHINEX86X_NOX64(__m128i _mm_setr_epi64(__m64,__m64))
__MACHINEX86X(__m128i _mm_setr_epi32(int,int,int,int))
__MACHINEX86X(__m128i _mm_setr_epi16(short,short,short,short,short,short,short,short))
__MACHINEX86X(__m128i _mm_setr_epi8(char,char,char,char,char,char,char,char,char,char,char,char,char,char,char,char))
__MACHINEX86X(__m128i _mm_setzero_si128(void))
__MACHINEX86X(void _mm_store_si128(__m128i*,__m128i))
__MACHINEX86X(void _mm_storeu_si128(__m128i*,__m128i))
__MACHINEX86X(void _mm_storel_epi64(__m128i*,__m128i))
__MACHINEX86X(void _mm_maskmoveu_si128(__m128i,__m128i,char*))
__MACHINEX86X(__m128i _mm_move_epi64(__m128i))
__MACHINEX86X_NOX64(__m128i _mm_movpi64_epi64(__m64))
__MACHINEX86X_NOX64(__m64 _mm_movepi64_pi64(__m128i))
__MACHINEX86X(void _mm_stream_pd(double*,__m128d))
__MACHINEX86X(void _mm_stream_si128(__m128i*,__m128i))
__MACHINEX86X(void _mm_clflush(void const *))
__MACHINEX86X(void _mm_lfence(void))
__MACHINEX86X(void _mm_mfence(void))
__MACHINEX86X(void _mm_stream_si32(int*,int))
__MACHINEX86X(void _mm_pause(void))
__MACHINEX86X(__m128 _mm_addsub_ps(__m128,__m128))
__MACHINEX86X(__m128d _mm_addsub_pd(__m128d,__m128d))
__MACHINEX86X(__m128 _mm_hadd_ps(__m128,__m128))
__MACHINEX86X(__m128d _mm_hadd_pd(__m128d,__m128d))
__MACHINEX86X(__m128 _mm_hsub_ps(__m128,__m128))
__MACHINEX86X(__m128d _mm_hsub_pd(__m128d,__m128d))
__MACHINEX86X(__m128i _mm_lddqu_si128(__m128i const*))
__MACHINEX86X(void _mm_monitor(void const*,unsigned int,unsigned int))
__MACHINEX86X(__m128d _mm_movedup_pd(__m128d))
__MACHINEX86X(__m128d _mm_loaddup_pd(double const*))
__MACHINEX86X(__m128 _mm_movehdup_ps(__m128))
__MACHINEX86X(__m128 _mm_moveldup_ps(__m128))
__MACHINEX86X(void _mm_mwait(unsigned int,unsigned int))
__MACHINEI(void _WriteBarrier(void))
__MACHINEI(void _ReadWriteBarrier(void))
__MACHINEIA64(void _WriteBarrier(void))
__MACHINEIA64(void _ReadWriteBarrier(void))
__MACHINEX64(void __faststorefence(void))
__MACHINEX64(__int64 __mulh(__int64,__int64))
__MACHINEX64(unsigned __int64 __umulh(unsigned __int64,unsigned __int64))
__MACHINEX64(unsigned __int64 __readcr0(void))
__MACHINEX64(unsigned __int64 __readcr2(void))
__MACHINEX64(unsigned __int64 __readcr3(void))
__MACHINEX64(unsigned __int64 __readcr4(void))
__MACHINEX64(unsigned __int64 __readcr8(void))
__MACHINEIA32(unsigned long __readcr0(void))
__MACHINEIA32(unsigned long __readcr2(void))
__MACHINEIA32(unsigned long __readcr3(void))
__MACHINEIA32(unsigned long __readcr4(void))
__MACHINEIA32(unsigned long __readcr8(void))
__MACHINEX64(void __writecr0(unsigned __int64))
__MACHINEX64(void __writecr3(unsigned __int64))
__MACHINEX64(void __writecr4(unsigned __int64))
__MACHINEX64(void __writecr8(unsigned __int64))
__MACHINEIA32(void __writecr0(unsigned))
__MACHINEIA32(void __writecr3(unsigned))
__MACHINEIA32(void __writecr4(unsigned))
__MACHINEIA32(void __writecr8(unsigned))
__MACHINEX64(unsigned __int64 __readdr(unsigned int))
__MACHINEIA32(unsigned __readdr(unsigned int))
__MACHINEX64(void __writedr(unsigned int, unsigned __int64))
__MACHINEIA32(void __writedr(unsigned int, unsigned))
__MACHINEX64(unsigned __int64 __readeflags(void))
__MACHINEIA32(unsigned __readeflags(void))
__MACHINEX64(void __writeeflags(unsigned __int64))
__MACHINEIA32(void __writeeflags(unsigned))
__MACHINEI(void __wbinvd(void))
__MACHINEI(void __invlpg(void*))
__MACHINEI(unsigned __int64 __readmsr(unsigned long))
__MACHINEI(void __writemsr(unsigned long, unsigned __int64))
__MACHINEI(unsigned __int64 __rdtsc(void))
__MACHINEI(void __movsb(unsigned char *, unsigned char const *, size_t))
__MACHINEI(void __movsw(unsigned short *, unsigned short const *, size_t))
__MACHINEI(void __movsd(unsigned long *, unsigned long const *, size_t))
__MACHINEX64(void __movsq(unsigned long long *, unsigned long long const *, size_t))
__MACHINEX64(unsigned char __readgsbyte(unsigned long Offset))
__MACHINEX64(unsigned short __readgsword(unsigned long Offset))
__MACHINEX64(unsigned long __readgsdword(unsigned long Offset))
__MACHINEX64(unsigned __int64 __readgsqword(unsigned long Offset))
__MACHINEX64(void __writegsbyte(unsigned long Offset, unsigned char Data))
__MACHINEX64(void __writegsword(unsigned long Offset, unsigned short Data))
__MACHINEX64(void __writegsdword(unsigned long Offset, unsigned long Data))
__MACHINEX64(void __writegsqword(unsigned long Offset, unsigned __int64 Data))
__MACHINEI(unsigned char __inbyte(unsigned short Port))
__MACHINEI(unsigned short __inword(unsigned short Port))
__MACHINEI(unsigned long __indword(unsigned short Port))
__MACHINEI(void __outbyte(unsigned short Port, unsigned char Data))
__MACHINEI(void __outword(unsigned short Port, unsigned short Data))
__MACHINEI(void __outdword(unsigned short Port, unsigned long Data))
__MACHINEI(void __inbytestring(unsigned short Port, unsigned char *Buffer, unsigned long Count))
__MACHINEI(void __inwordstring(unsigned short Port, unsigned short *Buffer, unsigned long Count))
__MACHINEI(void __indwordstring(unsigned short Port, unsigned long *Buffer, unsigned long Count))
__MACHINEI(void __outbytestring(unsigned short Port, unsigned char *Buffer, unsigned long Count))
__MACHINEI(void __outwordstring(unsigned short Port, unsigned short *Buffer, unsigned long Count))
__MACHINEI(void __outdwordstring(unsigned short Port, unsigned long *Buffer, unsigned long Count))
__MACHINEI(unsigned int __getcallerseflags())
__MACHINEX64(unsigned char __vmx_vmclear(unsigned __int64*))
__MACHINEX64(unsigned char __vmx_vmlaunch(void))
__MACHINEX64(unsigned char __vmx_vmptrld(unsigned __int64*))
__MACHINEI(void __vmx_vmptrst(unsigned __int64 *))
__MACHINEX64(unsigned char __vmx_vmread(size_t, size_t*))
__MACHINEX64(unsigned char __vmx_vmresume(void))
__MACHINEX64(unsigned char __vmx_vmwrite(size_t, size_t))
__MACHINEI(void __vmx_off(void))
__MACHINEX64(unsigned char __vmx_on(unsigned __int64*))
__MACHINEI(void __svm_clgi(void))
__MACHINEI(void __svm_invlpga(void*, int))
__MACHINEI(void __svm_skinit(int))
__MACHINEI(void __svm_stgi(void))
__MACHINEI(void __svm_vmload(size_t))
__MACHINEI(void __svm_vmrun(size_t))
__MACHINEI(void __svm_vmsave(size_t))
__MACHINEI(void __halt(void))
__MACHINEI(void __sidt(void*))
__MACHINEI(void __lidt(void*))
__MACHINEI(void __ud2(void))
__MACHINEI(void __nop(void))
__MACHINEX64(__m128i _mm_set_epi64x(__int64 i1, __int64 i0))
__MACHINEX64(__m128i _mm_set1_epi64x(__int64 i))
__MACHINEX64(__int64 _mm_cvtsd_si64x(__m128d a))
__MACHINEX64(__m128d _mm_cvtsi64x_sd(__m128d a, __int64 b))
__MACHINEX64(__m128 _mm_cvtsi64x_ss(__m128 a, __int64 b))
__MACHINEX64(__int64 _mm_cvtss_si64x(__m128 a))
__MACHINEX64(__int64 _mm_cvttsd_si64x(__m128d a))
__MACHINEX64(__int64 _mm_cvttss_si64x(__m128 a))
__MACHINEX64(__m128i _mm_cvtsi64x_si128(__int64 a))
__MACHINEX64(__int64 _mm_cvtsi128_si64x(__m128i a))
__MACHINEX64(void _mm_stream_si64x(__int64 *,__int64))
__MACHINEI(void __stosb(unsigned char *, unsigned char, size_t))
__MACHINEI(void __stosw(unsigned short *,  unsigned short, size_t))
__MACHINEI(void __stosd(unsigned long *,  unsigned long, size_t))
__MACHINEX64(void __stosq(unsigned __int64 *,  unsigned __int64, size_t))
__MACHINEIW64(unsigned char _bittest(long const *a, long b))
__MACHINEIW64(unsigned char _bittestandset(long *a, long b))
__MACHINEIW64(unsigned char _bittestandreset(long *a, long b))
__MACHINEIW64(unsigned char _bittestandcomplement(long *a, long b))
//__MACHINEI(unsigned char _interlockedbittestandset(long *a, long b))
//__MACHINEI(unsigned char _interlockedbittestandreset(long *a, long b))
__MACHINEW64(unsigned char _bittest64(__int64 const *a, __int64 b))
__MACHINEW64(unsigned char _bittestandset64(__int64 *a, __int64 b))
__MACHINEW64(unsigned char _bittestandreset64(__int64 *a, __int64 b))
__MACHINEW64(unsigned char _bittestandcomplement64(__int64 *a, __int64 b))
//__MACHINEX64(unsigned char _interlockedbittestandset64(__int64 *a, __int64 b))
//__MACHINEX64(unsigned char _interlockedbittestandreset64(__int64 *a, __int64 b))
__MACHINEI(void __cpuid(int a[4], int b))
__MACHINEI(unsigned __int64 __readpmc(unsigned long a))
__MACHINEI(unsigned long __segmentlimit(unsigned long a))

__MACHINEIA32(unsigned char __readfsbyte(unsigned long Offset))
__MACHINEIA32(unsigned short __readfsword(unsigned long Offset))
__MACHINEIA32(unsigned long __readfsdword(unsigned long Offset))
__MACHINEIA32(unsigned __int64 __readfsqword(unsigned long Offset))
__MACHINEIA32(void __writefsbyte(unsigned long Offset, unsigned char Data))
__MACHINEIA32(void __writefsword(unsigned long Offset, unsigned short Data))
__MACHINEIA32(void __writefsdword(unsigned long Offset, unsigned long Data))
__MACHINEIA32(void __writefsqword(unsigned long Offset, unsigned __int64 Data))


__MACHINE(unsigned __int64 __cdecl _rotl64(unsigned __int64,int))
__MACHINE(unsigned __int64 __cdecl _rotr64(unsigned __int64,int))
__MACHINE(__int64 __cdecl _abs64(__int64))

__MACHINEIW64(unsigned char _BitScanForward(unsigned long* Index, unsigned long Mask))
__MACHINEIW64(unsigned char _BitScanReverse(unsigned long* Index, unsigned long Mask))
__MACHINEW64(unsigned char _BitScanForward64(unsigned long* Index, unsigned __int64 Mask))
__MACHINEW64(unsigned char _BitScanReverse64(unsigned long* Index, unsigned __int64 Mask))
__MACHINEIW64(_CRTIMP wchar_t * __cdecl wcscat(wchar_t *, const wchar_t *))
__MACHINEIW64(_CRTIMP int __cdecl wcscmp(const wchar_t *, const wchar_t *))
__MACHINEIW64(_CRTIMP wchar_t * __cdecl wcscpy(wchar_t *, const wchar_t *))
__MACHINEIW64(_CRTIMP size_t __cdecl wcslen(const wchar_t *))
__MACHINEIW64(_CRTIMP wchar_t * __cdecl _wcsset(wchar_t *, wchar_t))
__MACHINEW64(unsigned __int64 __shiftleft128(unsigned __int64 LowPart, unsigned __int64 HighPart, unsigned char Shift))
__MACHINEW64(unsigned __int64 __shiftright128(unsigned __int64 LowPart, unsigned __int64 HighPart, unsigned char Shift))
__MACHINEW64(unsigned __int64 _umul128(unsigned __int64 multiplier, unsigned __int64 multiplicand, unsigned __int64 *highproduct))
__MACHINEW64(__int64 _mul128(__int64 multiplier, __int64 multiplicand, __int64 *highproduct))
__MACHINEI(void __int2c(void))
__MACHINEIW64(void _ReadBarrier(void))
__MACHINEIW64(unsigned char _rotr8(unsigned char value, unsigned char shift))
__MACHINEIW64(unsigned short _rotr16(unsigned short value, unsigned char shift))
__MACHINEIW64(unsigned char _rotl8(unsigned char value, unsigned char shift))
__MACHINEIW64(unsigned short _rotl16(unsigned short value, unsigned char shift))
__MACHINEIW64(short _InterlockedIncrement16(short volatile *Addend))
__MACHINEIW64(short _InterlockedDecrement16(short volatile *Addend))
__MACHINEIW64(short _InterlockedCompareExchange16(short volatile *Destination, short Exchange, short Comparand))
__MACHINEIA64(short _InterlockedIncrement16_acq(short volatile *Addend))
__MACHINEIA64(short _InterlockedIncrement16_rel(short volatile *Addend))
__MACHINEIA64(short _InterlockedDecrement16_acq(short volatile *Addend))
__MACHINEIA64(short _InterlockedDecrement16_rel(short volatile *Addend))
__MACHINEIA64(short _InterlockedCompareExchange16_acq(short volatile *Destination, short Exchange, short Comparand))
__MACHINEIA64(short _InterlockedCompareExchange16_rel(short volatile *Destination, short Exchange, short Comparand))
__MACHINECC(unsigned __int64 _m_paddb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddsb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddsw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddsd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddusb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddusw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paddusd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubsb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubsw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubsd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubusb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubusw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psubusd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmaddwd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmadduwd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmulhw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmulhuw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmullw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmullw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmacsw(unsigned __int64 m1, unsigned __int64 m2, unsigned __int64 m3))
__MACHINECC(unsigned __int64 _m_pmacuw(unsigned __int64 m1, unsigned __int64 m2, unsigned __int64 m3))
__MACHINECC(unsigned __int64 _m_pmacszw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_padduzw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paccb(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_paccw(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_paccd(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_pmia(unsigned __int64 m1, int i1, int i0))
__MACHINECC(unsigned __int64 _m_pmiaph(unsigned __int64 m1, int i1, int i0))
__MACHINECC(unsigned __int64 _m_pmiabb(unsigned __int64 m1, int i1, int i0))
__MACHINECC(unsigned __int64 _m_pmiabt(unsigned __int64 m1, int i1, int i0))
__MACHINECC(unsigned __int64 _m_pmiatb(unsigned __int64 m1, int i1, int i0))
__MACHINECC(unsigned __int64 _m_pmiatt(unsigned __int64 m1, int i1, int i0))
__MACHINECC(unsigned __int64 _m_psllw(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psllwi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_pslld(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_pslldi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_psllq(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psllqi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_psraw(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psrawi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_psrad(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psradi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_psraq(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psraqi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_psrlw(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psrlwi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_psrld(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psrldi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_psrlq(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_psrlqi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_prorw(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_prorwi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_prord(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_prordi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_prorq(unsigned __int64 m1, unsigned __int64 count))
__MACHINECC(unsigned __int64 _m_prorqi(unsigned __int64 m1, int count))
__MACHINECC(unsigned __int64 _m_pand(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pandn(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_por(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pxor(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpeqb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpeqw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpeqd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpgtb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpgtub(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpgtw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpgtuw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpgtd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pcmpgtud(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_packsswb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_packssdw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_packssqd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_packuswb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_packusdw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_packusqd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_punpckhbw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_punpckhwd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_punpckhdq(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_punpcklbw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_punpcklwd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_punpckldq(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_punpckehsbw(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckehswd(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckehsdq(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckehubw(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckehuwd(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckehudq(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckelsbw(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckelswd(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckelsdq(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckelubw(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckeluwd(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_punpckeludq(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _mm_setzero_si64())
__MACHINECC(unsigned __int64 _mm_set_pi32(int i1, int i0))
__MACHINECC(unsigned __int64 _mm_set_pi16(short s3, short s2, short s1, short s0))
__MACHINECC(unsigned __int64 _mm_set_pi8(char b7, char b6, char b5, char b4, char b3, char b2, char b1, char b0))
__MACHINECC(unsigned __int64 _mm_set1_pi32(int i))
__MACHINECC(unsigned __int64 _mm_set1_pi16(short s))
__MACHINECC(unsigned __int64 _mm_set1_pi8(char b))
__MACHINECC(unsigned __int64 _mm_setr_pi32(int i1, int i0))
__MACHINECC(unsigned __int64 _mm_setr_pi16(short s3, short s2, short s1, short s0))
__MACHINECC(unsigned __int64 _mm_setr_pi8(char b7, char b6, char b5, char b4, char b3, char b2, char b1, char b0))
__MACHINECC(void  _mm_setwcx(int i1, int i0))
__MACHINECC(int   _mm_getwcx(int i))
__MACHINECC(int _m_pextrb(unsigned __int64 m1, const int c))
__MACHINECC(int _m_pextrw(unsigned __int64 m1, const int c))
__MACHINECC(int _m_pextrd(unsigned __int64 m1, const int c))
__MACHINECC(unsigned int _m_pextrub(unsigned __int64 m1, const int c))
__MACHINECC(unsigned int _m_pextruw(unsigned __int64 m1, const int c))
__MACHINECC(unsigned int _m_pextrud(unsigned __int64 m1, const int c))
__MACHINECC(unsigned __int64 _m_pinsrb(unsigned __int64 m1, int i, const int c))
__MACHINECC(unsigned __int64 _m_pinsrw(unsigned __int64 m1, int i, const int c))
__MACHINECC(unsigned __int64 _m_pinsrd(unsigned __int64 m1, int i, const int c))
__MACHINECC(unsigned __int64 _m_pmaxsb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmaxsw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmaxsd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmaxub(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmaxuw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pmaxud(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pminsb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pminsw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pminsd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pminub(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pminuw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pminud(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(int   _m_pmovmskb(unsigned __int64 m1))
__MACHINECC(int   _m_pmovmskw(unsigned __int64 m1))
__MACHINECC(int   _m_pmovmskd(unsigned __int64 m1))
__MACHINECC(unsigned __int64 _m_pshufw(unsigned __int64 m1, int i))
__MACHINECC(unsigned __int64 _m_pavgb(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pavgw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pavg2b(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_pavg2w(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psadbw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psadwd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psadzbw(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_psadzwd(unsigned __int64 m1, unsigned __int64 m2))
__MACHINECC(unsigned __int64 _m_paligniq(unsigned __int64 m1, unsigned __int64 m2, int i))
__MACHINECC(unsigned __int64 _m_cvt_si2pi(__int64 i))
__MACHINECC(__int64 _m_cvt_pi2si(unsigned __int64 m1))
__MACHINEIW64(void __nvreg_save_fence(void))
__MACHINEIW64(void __nvreg_restore_fence(void))

__MACHINEX64(short _InterlockedCompareExchange16_np(short volatile *Destination, short Exchange, short Comparand))
__MACHINEX64(long _InterlockedCompareExchange_np (long *, long, long))
__MACHINEX64(__int64 _InterlockedCompareExchange64_np(__int64 *, __int64, __int64))
__MACHINEX64(void *_InterlockedCompareExchangePointer_np (void **, void *, void *))
__MACHINEX64(__int64 _InterlockedCompare64Exchange128_np(__int64 *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEX64(__int64 _InterlockedCompare64Exchange128_acq_np(__int64 *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEX64(__int64 _InterlockedCompare64Exchange128_rel_np(__int64 *Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 Comparand))
__MACHINEX64(long _InterlockedAnd_np(long *, long))
__MACHINEX64(char _InterlockedAnd8_np(char *, char))
__MACHINEX64(short _InterlockedAnd16_np(short *, short))
__MACHINEX64(__int64 _InterlockedAnd64_np(__int64 *, __int64))
__MACHINEX64(long _InterlockedOr_np(long *, long))
__MACHINEX64(char _InterlockedOr8_np(char *, char))
__MACHINEX64(short _InterlockedOr16_np(short *, short))
__MACHINEX64(__int64 _InterlockedOr64_np(__int64 *, __int64))
__MACHINEX64(long _InterlockedXor_np(long *, long))
__MACHINEX64(char _InterlockedXor8_np(char *, char))
__MACHINEX64(short _InterlockedXor16_np(short *, short))
__MACHINEX64(__int64 _InterlockedXor64_np(__int64 *, __int64))

#if defined(_M_IA64)

    /* Special Registers */

#define     __REG_IA64_Ip       	1016

    /* General-Purpose Registers */

    /* Integer registers */
#define     __REG_IA64_IntR0    	1024
#define     __REG_IA64_IntR1    	1025
#define     __REG_IA64_IntR2    	1026
#define     __REG_IA64_IntR3    	1027
#define     __REG_IA64_IntR4    	1028
#define     __REG_IA64_IntR5    	1029
#define     __REG_IA64_IntR6    	1030
#define     __REG_IA64_IntR7    	1031
#define     __REG_IA64_IntR8    	1032
#define     __REG_IA64_IntR9    	1033
#define     __REG_IA64_IntR10   	1034
#define     __REG_IA64_IntR11   	1035
#define     __REG_IA64_IntR12   	1036
#define     __REG_IA64_IntR13   	1037
#define     __REG_IA64_IntR14   	1038
#define     __REG_IA64_IntR15   	1039
#define     __REG_IA64_IntR16   	1040
#define     __REG_IA64_IntR17   	1041
#define     __REG_IA64_IntR18   	1042
#define     __REG_IA64_IntR19   	1043
#define     __REG_IA64_IntR20   	1044
#define     __REG_IA64_IntR21   	1045
#define     __REG_IA64_IntR22   	1046
#define     __REG_IA64_IntR23   	1047
#define     __REG_IA64_IntR24   	1048
#define     __REG_IA64_IntR25   	1049
#define     __REG_IA64_IntR26   	1050
#define     __REG_IA64_IntR27   	1051
#define     __REG_IA64_IntR28   	1052
#define     __REG_IA64_IntR29   	1053
#define     __REG_IA64_IntR30   	1054
#define     __REG_IA64_IntR31   	1055

    /* Register Stack */
#define     __REG_IA64_IntR32   	1056
#define     __REG_IA64_IntR33   	1057
#define     __REG_IA64_IntR34   	1058
#define     __REG_IA64_IntR35   	1059
#define     __REG_IA64_IntR36   	1060
#define     __REG_IA64_IntR37   	1061
#define     __REG_IA64_IntR38   	1062
#define     __REG_IA64_IntR39   	1063
#define     __REG_IA64_IntR40   	1064
#define     __REG_IA64_IntR41   	1065
#define     __REG_IA64_IntR42   	1066
#define     __REG_IA64_IntR43   	1067
#define     __REG_IA64_IntR44   	1068
#define     __REG_IA64_IntR45   	1069
#define     __REG_IA64_IntR46   	1070
#define     __REG_IA64_IntR47   	1071
#define     __REG_IA64_IntR48   	1072
#define     __REG_IA64_IntR49   	1073
#define     __REG_IA64_IntR50   	1074
#define     __REG_IA64_IntR51   	1075
#define     __REG_IA64_IntR52   	1076
#define     __REG_IA64_IntR53   	1077
#define     __REG_IA64_IntR54   	1078
#define     __REG_IA64_IntR55   	1079
#define     __REG_IA64_IntR56   	1080
#define     __REG_IA64_IntR57   	1081
#define     __REG_IA64_IntR58   	1082
#define     __REG_IA64_IntR59   	1083
#define     __REG_IA64_IntR60   	1084
#define     __REG_IA64_IntR61   	1085
#define     __REG_IA64_IntR62   	1086
#define     __REG_IA64_IntR63   	1087
#define     __REG_IA64_IntR64   	1088
#define     __REG_IA64_IntR65   	1089
#define     __REG_IA64_IntR66   	1090
#define     __REG_IA64_IntR67   	1091
#define     __REG_IA64_IntR68   	1092
#define     __REG_IA64_IntR69   	1093
#define     __REG_IA64_IntR70   	1094
#define     __REG_IA64_IntR71   	1095
#define     __REG_IA64_IntR72   	1096
#define     __REG_IA64_IntR73   	1097
#define     __REG_IA64_IntR74   	1098
#define     __REG_IA64_IntR75   	1099
#define     __REG_IA64_IntR76   	1100
#define     __REG_IA64_IntR77   	1101
#define     __REG_IA64_IntR78   	1102
#define     __REG_IA64_IntR79   	1103
#define     __REG_IA64_IntR80   	1104
#define     __REG_IA64_IntR81   	1105
#define     __REG_IA64_IntR82   	1106
#define     __REG_IA64_IntR83   	1107
#define     __REG_IA64_IntR84   	1108
#define     __REG_IA64_IntR85   	1109
#define     __REG_IA64_IntR86   	1110
#define     __REG_IA64_IntR87   	1111
#define     __REG_IA64_IntR88   	1112
#define     __REG_IA64_IntR89   	1113
#define     __REG_IA64_IntR90   	1114
#define     __REG_IA64_IntR91   	1115
#define     __REG_IA64_IntR92   	1116
#define     __REG_IA64_IntR93   	1117
#define     __REG_IA64_IntR94   	1118
#define     __REG_IA64_IntR95   	1119
#define     __REG_IA64_IntR96   	1120
#define     __REG_IA64_IntR97   	1121
#define     __REG_IA64_IntR98   	1122
#define     __REG_IA64_IntR99   	1123
#define     __REG_IA64_IntR100  	1124
#define     __REG_IA64_IntR101  	1125
#define     __REG_IA64_IntR102  	1126
#define     __REG_IA64_IntR103  	1127
#define     __REG_IA64_IntR104  	1128
#define     __REG_IA64_IntR105  	1129
#define     __REG_IA64_IntR106  	1130
#define     __REG_IA64_IntR107  	1131
#define     __REG_IA64_IntR108  	1132
#define     __REG_IA64_IntR109  	1133
#define     __REG_IA64_IntR110  	1134
#define     __REG_IA64_IntR111  	1135
#define     __REG_IA64_IntR112  	1136
#define     __REG_IA64_IntR113  	1137
#define     __REG_IA64_IntR114  	1138
#define     __REG_IA64_IntR115  	1139
#define     __REG_IA64_IntR116  	1140
#define     __REG_IA64_IntR117  	1141
#define     __REG_IA64_IntR118  	1142
#define     __REG_IA64_IntR119  	1143
#define     __REG_IA64_IntR120  	1144
#define     __REG_IA64_IntR121  	1145
#define     __REG_IA64_IntR122  	1146
#define     __REG_IA64_IntR123  	1147
#define     __REG_IA64_IntR124  	1148
#define     __REG_IA64_IntR125  	1149
#define     __REG_IA64_IntR126  	1150
#define     __REG_IA64_IntR127  	1151

    /* Application Registers */

#define     __REG_IA64_ApKR0    	3072
#define     __REG_IA64_ApKR1    	3073
#define     __REG_IA64_ApKR2    	3074
#define     __REG_IA64_ApKR3    	3075
#define     __REG_IA64_ApKR4    	3076
#define     __REG_IA64_ApKR5    	3077
#define     __REG_IA64_ApKR6    	3078
#define     __REG_IA64_ApKR7    	3079
#define     __REG_IA64_AR8      	3080
#define     __REG_IA64_AR9      	3081
#define     __REG_IA64_AR10     	3082
#define     __REG_IA64_AR11     	3083
#define     __REG_IA64_AR12     	3084
#define     __REG_IA64_AR13     	3085
#define     __REG_IA64_AR14     	3086
#define     __REG_IA64_AR15     	3087
#define     __REG_IA64_RsRSC    	3088
#define     __REG_IA64_RsBSP    	3089
#define     __REG_IA64_RsBSPSTORE   	3090
#define     __REG_IA64_RsRNAT   	3091
#define     __REG_IA64_AR20     	3092
#define     __REG_IA64_StFCR    	3093
#define     __REG_IA64_AR22     	3094
#define     __REG_IA64_AR23     	3095
#define     __REG_IA64_EFLAG    	3096
#define     __REG_IA64_CSD      	3097
#define     __REG_IA64_SSD      	3098
#define     __REG_IA64_CFLG     	3099
#define     __REG_IA64_StFSR    	3100
#define     __REG_IA64_StFIR    	3101
#define     __REG_IA64_StFDR    	3102
#define     __REG_IA64_AR31     	3103
#define     __REG_IA64_ApCCV    	3104
#define     __REG_IA64_AR33     	3105
#define     __REG_IA64_AR34     	3106
#define     __REG_IA64_AR35     	3107
#define     __REG_IA64_ApUNAT   	3108
#define     __REG_IA64_AR37     	3109
#define     __REG_IA64_AR38     	3110
#define     __REG_IA64_AR39     	3111
#define     __REG_IA64_StFPSR   	3112
#define     __REG_IA64_AR41     	3113
#define     __REG_IA64_AR42     	3114
#define     __REG_IA64_AR43     	3115
#define     __REG_IA64_ApITC    	3116
#define     __REG_IA64_AR45     	3117
#define     __REG_IA64_AR46     	3118
#define     __REG_IA64_AR47     	3119
#define     __REG_IA64_AR48     	3120
#define     __REG_IA64_AR49     	3121
#define     __REG_IA64_AR50     	3122
#define     __REG_IA64_AR51     	3123
#define     __REG_IA64_AR52     	3124
#define     __REG_IA64_AR53     	3125
#define     __REG_IA64_AR54     	3126
#define     __REG_IA64_AR55     	3127
#define     __REG_IA64_AR56     	3128
#define     __REG_IA64_AR57     	3129
#define     __REG_IA64_AR58     	3130
#define     __REG_IA64_AR59     	3131
#define     __REG_IA64_AR60     	3132
#define     __REG_IA64_AR61     	3133
#define     __REG_IA64_AR62     	3134
#define     __REG_IA64_AR63     	3135
#define     __REG_IA64_RsPFS    	3136
#define     __REG_IA64_ApLC     	3137
#define     __REG_IA64_ApEC     	3138
#define     __REG_IA64_AR67     	3139
#define     __REG_IA64_AR68     	3140
#define     __REG_IA64_AR69     	3141
#define     __REG_IA64_AR70     	3142
#define     __REG_IA64_AR71     	3143
#define     __REG_IA64_AR72     	3144
#define     __REG_IA64_AR73     	3145
#define     __REG_IA64_AR74     	3146
#define     __REG_IA64_AR75     	3147
#define     __REG_IA64_AR76     	3148
#define     __REG_IA64_AR77     	3149
#define     __REG_IA64_AR78     	3150
#define     __REG_IA64_AR79     	3151
#define     __REG_IA64_AR80     	3152
#define     __REG_IA64_AR81     	3153
#define     __REG_IA64_AR82     	3154
#define     __REG_IA64_AR83     	3155
#define     __REG_IA64_AR84     	3156
#define     __REG_IA64_AR85     	3157
#define     __REG_IA64_AR86     	3158
#define     __REG_IA64_AR87     	3159
#define     __REG_IA64_AR88     	3160
#define     __REG_IA64_AR89     	3161
#define     __REG_IA64_AR90     	3162
#define     __REG_IA64_AR91     	3163
#define     __REG_IA64_AR92     	3164
#define     __REG_IA64_AR93     	3165
#define     __REG_IA64_AR94     	3166
#define     __REG_IA64_AR95     	3167
#define     __REG_IA64_AR96     	3168
#define     __REG_IA64_AR97     	3169
#define     __REG_IA64_AR98     	3170
#define     __REG_IA64_AR99     	3171
#define     __REG_IA64_AR100    	3172
#define     __REG_IA64_AR101    	3173
#define     __REG_IA64_AR102    	3174
#define     __REG_IA64_AR103    	3175
#define     __REG_IA64_AR104    	3176
#define     __REG_IA64_AR105    	3177
#define     __REG_IA64_AR106    	3178
#define     __REG_IA64_AR107    	3179
#define     __REG_IA64_AR108    	3180
#define     __REG_IA64_AR109    	3181
#define     __REG_IA64_AR110    	3182
#define     __REG_IA64_AR111    	3183
#define     __REG_IA64_AR112    	3184
#define     __REG_IA64_AR113    	3185
#define     __REG_IA64_AR114    	3186
#define     __REG_IA64_AR115    	3187
#define     __REG_IA64_AR116    	3188
#define     __REG_IA64_AR117    	3189
#define     __REG_IA64_AR118    	3190
#define     __REG_IA64_AR119    	3191
#define     __REG_IA64_AR120    	3192
#define     __REG_IA64_AR121    	3193
#define     __REG_IA64_AR122    	3194
#define     __REG_IA64_AR123    	3195
#define     __REG_IA64_AR124    	3196
#define     __REG_IA64_AR125    	3197
#define     __REG_IA64_AR126    	3198
#define     __REG_IA64_AR127    	3199

    /* CPUID Registers */

#define     __REG_IA64_CPUID0   	3328
#define     __REG_IA64_CPUID1   	3329
#define     __REG_IA64_CPUID2   	3330
#define     __REG_IA64_CPUID3   	3331
#define     __REG_IA64_CPUID4   	3332

    /* Control Registers */

#define     __REG_IA64_ApDCR    	4096
#define     __REG_IA64_ApITM    	4097
#define     __REG_IA64_ApIVA    	4098
#define     __REG_IA64_ApPTA    	4104
#define     __REG_IA64_ApGPTA   	4105
#define     __REG_IA64_StIPSR   	4112
#define     __REG_IA64_StISR    	4113
#define     __REG_IA64_StIIP    	4115
#define     __REG_IA64_StIFA    	4116
#define     __REG_IA64_StITIR   	4117
#define     __REG_IA64_StIIPA   	4118
#define     __REG_IA64_StIFS    	4119
#define     __REG_IA64_StIIM    	4120
#define     __REG_IA64_StIHA    	4121
#define     __REG_IA64_SaLID    	4160
#define     __REG_IA64_SaIVR    	4161
#define     __REG_IA64_SaTPR    	4162
#define     __REG_IA64_SaEOI    	4163
#define     __REG_IA64_SaIRR0   	4164
#define     __REG_IA64_SaIRR1   	4165
#define     __REG_IA64_SaIRR2   	4166
#define     __REG_IA64_SaIRR3   	4167
#define     __REG_IA64_SaITV    	4168
#define     __REG_IA64_SaPMV    	4169
#define     __REG_IA64_SaCMCV   	4170
#define     __REG_IA64_SaLRR0   	4176
#define     __REG_IA64_SaLRR1   	4177

    /* Performance Monitor Data Registers */

#define     __REG_IA64_PFD0      7168
#define     __REG_IA64_PFD1      7169
#define     __REG_IA64_PFD2      7170
#define     __REG_IA64_PFD3      7171
#define     __REG_IA64_PFD4      7172
#define     __REG_IA64_PFD5      7173
#define     __REG_IA64_PFD6      7174
#define     __REG_IA64_PFD7      7175
#define     __REG_IA64_PFD8      7176
#define     __REG_IA64_PFD9      7177
#define     __REG_IA64_PFD10     7178
#define     __REG_IA64_PFD11     7179
#define     __REG_IA64_PFD12     7180
#define     __REG_IA64_PFD13     7181
#define     __REG_IA64_PFD14     7182
#define     __REG_IA64_PFD15     7183
#define     __REG_IA64_PFD16     7184
#define     __REG_IA64_PFD17     7185

    /* Performance Monitor Config Registers */

#define     __REG_IA64_PFC0      7424
#define     __REG_IA64_PFC1      7425
#define     __REG_IA64_PFC2      7426
#define     __REG_IA64_PFC3      7427
#define     __REG_IA64_PFC4      7428
#define     __REG_IA64_PFC5      7429
#define     __REG_IA64_PFC6      7430
#define     __REG_IA64_PFC7      7431
#define     __REG_IA64_PFC8      7432
#define     __REG_IA64_PFC9      7433
#define     __REG_IA64_PFC10     7434
#define     __REG_IA64_PFC11     7435
#define     __REG_IA64_PFC12     7436
#define     __REG_IA64_PFC13     7437
#define     __REG_IA64_PFC14     7438
#define     __REG_IA64_PFC15     7439

    /* Instruction Breakpoint Registers */

#define     __REG_IA64_DbI0     	8448
#define     __REG_IA64_DbI1     	8449
#define     __REG_IA64_DbI2     	8450
#define     __REG_IA64_DbI3     	8451
#define     __REG_IA64_DbI4     	8452
#define     __REG_IA64_DbI5     	8453
#define     __REG_IA64_DbI6     	8454
#define     __REG_IA64_DbI7     	8455

    /* Data Breakpoint Registers */

#define     __REG_IA64_DbD0     	8576
#define     __REG_IA64_DbD1     	8577
#define     __REG_IA64_DbD2     	8578
#define     __REG_IA64_DbD3     	8579
#define     __REG_IA64_DbD4     	8580
#define     __REG_IA64_DbD5     	8581
#define     __REG_IA64_DbD6     	8582
#define     __REG_IA64_DbD7     	8583

#endif  /* defined(_M_IA64) */


#if defined(_NO_PREFETCHW)
#if defined(_M_AMD64)

#define _InterlockedCompareExchange16 _InterlockedCompareExchange16_np
#define _InterlockedCompareExchange _InterlockedCompareExchange_np
#define _InterlockedCompareExchange64 _InterlockedCompareExchange64_np
#define _InterlockedCompareExchangePointer _InterlockedCompareExchangePointer_np
#define _InterlockedCompare64Exchange128 _InterlockedCompare64Exchange128_np
#define _InterlockedCompare64Exchange128_acq _InterlockedCompare64Exchange128_acq_np
#define _InterlockedCompare64Exchange128_rel _InterlockedCompare64Exchange128_rel_np
#define _InterlockedAnd _InterlockedAnd_np
#define _InterlockedAnd8 _InterlockedAnd8_np
#define _InterlockedAnd16 _InterlockedAnd16_np
#define _InterlockedAnd64 _InterlockedAnd64_np
#define _InterlockedOr _InterlockedOr_np
#define _InterlockedOr8 _InterlockedOr8_np
#define _InterlockedOr16 _InterlockedOr16_np
#define _InterlockedOr64 _InterlockedOr64_np
#define _InterlockedXor _InterlockedXor_np
#define _InterlockedXor8 _InterlockedXor8_np
#define _InterlockedXor16 _InterlockedXor16_np
#define _InterlockedXor64 _InterlockedXor64_np

#endif /* defined(_M_AMD64) */
#endif /* defined(_NO_PREFETCHW) */

#if defined(__cplusplus)
}
#endif
#endif
#endif
