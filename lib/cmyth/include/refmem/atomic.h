/*
 *  Copyright (C) 2004-2008, Eric Lund, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MVP_ATOMIC_H
#define __MVP_ATOMIC_H

#ifdef __APPLE__
#pragma GCC optimization_level 0
#endif

#if defined(_MSC_VER)
#include <windows.h>
#define inline __inline
#endif

#if defined __mips__
#include <atomic.h>
#endif

#if defined(__APPLE__)
#include <libkern/OSAtomic.h>

typedef	volatile int32_t mvp_atomic_t;

#define __mvp_atomic_increment(x)	OSAtomicIncrement32(x)
#define __mvp_atomic_decrement(x)	OSAtomicDecrement32(x)
#else
typedef	volatile unsigned int mvp_atomic_t;

/**
 * Atomically incremente a reference count variable.
 * \param valp address of atomic variable
 * \return incremented reference count
 */
static inline unsigned
__mvp_atomic_increment(mvp_atomic_t *valp)
{
	mvp_atomic_t __val;
#if defined __i486__ || defined __i586__ || defined __i686__
	__asm__ __volatile__(
		"lock xaddl %0, (%1);"
		"     inc   %0;"
		: "=r" (__val)
		: "r" (valp), "0" (0x1)
		: "cc", "memory"
		);
#elif defined __i386__ || defined __x86_64__
	__asm__ volatile (".byte 0xf0, 0x0f, 0xc1, 0x02" /*lock; xaddl %eax, (%edx) */
		      : "=a" (__val)
		      : "0" (1), "m" (*valp), "d" (valp)
		      : "memory");
	/* on the x86 __val is the pre-increment value, so normalize it. */
	++__val;
#elif defined __powerpc__ || defined __ppc__
	asm volatile ("1:	lwarx   %0,0,%1\n"
		      "	addic.   %0,%0,1\n"
		      "	dcbt    %0,%1\n"
		      "	stwcx.  %0,0,%1\n"
		      "	bne-    1b\n"
		      "	isync\n"
		      : "=&r" (__val)
		      : "r" (valp)
		      : "cc", "memory");
#elif defined _MSC_VER
	__val = InterlockedIncrement(valp);
#elif defined ANDROID
	__val = __atomic_inc(valp) + 1;
#elif defined __arm__ && !defined __thumb__
	int tmp1, tmp2;
	int inc = 1;
	__asm__ __volatile__ (
		"\n"
		"0:"
		"ldr     %0, [%3]\n"
		"add     %1, %0, %4\n"
		"swp     %2, %1, [%3]\n"
		"cmp     %0, %2\n"
		"swpne   %0, %2, [%3]\n"
		"bne     0b\n"
		: "=&r"(tmp1), "=&r"(__val), "=&r"(tmp2) 
		: "r" (valp), "r"(inc) 
		: "cc", "memory");
#elif defined __mips__
	__val = atomic_increment_val(valp);
#elif defined __GNUC__
	/*
	 * Don't know how to atomic increment for a generic architecture
	 * so try to use GCC builtin
	 */
	__val = __sync_add_and_fetch(valp,1);
#else
#if !defined(_MSC_VER)
#warning unknown architecture, atomic increment is not...
#endif
	__val = ++(*valp);
#endif
	return __val;
}

/**
 * Atomically decrement a reference count variable.
 * \param valp address of atomic variable
 * \return decremented reference count
 */
static inline unsigned
__mvp_atomic_decrement(mvp_atomic_t *valp)
{
	mvp_atomic_t __val;
#if defined __i486__ || defined __i586__ || defined __i686__
	__asm__ __volatile__(
		"lock xaddl %0, (%1);"
		"     dec   %0;"
		: "=r" (__val)
		: "r" (valp), "0" (0x1)
		: "cc", "memory"
		);
#elif defined __i386__ || defined __x86_64__
	__asm__ volatile (".byte 0xf0, 0x0f, 0xc1, 0x02" /*lock; xaddl %eax, (%edx) */
		      : "=a" (__val)
		      : "0" (-1), "m" (*valp), "d" (valp)
		      : "memory");
	/* __val is the pre-decrement value, so normalize it */
	--__val;
#elif defined __powerpc__ || defined __ppc__
	asm volatile ("1:	lwarx   %0,0,%1\n"
		      "	addic.   %0,%0,-1\n"
		      "	dcbt    %0,%1\n"
		      "	stwcx.  %0,0,%1\n"
		      "	bne-    1b\n"
		      "	isync\n"
		      : "=&r" (__val)
		      : "r" (valp)
		      : "cc", "memory");
#elif defined ANDROID
	__val = __atomic_dec(valp) - 1;
#elif defined __arm__ && !defined __thumb__
	int tmp1, tmp2;
	int inc = -1;
	__asm__ __volatile__ (
		"\n"
		"0:"
		"ldr     %0, [%3]\n"
		"add     %1, %0, %4\n"
		"swp     %2, %1, [%3]\n"
		"cmp     %0, %2\n"
		"swpne   %0, %2, [%3]\n"
		"bne     0b\n"
		: "=&r"(tmp1), "=&r"(__val), "=&r"(tmp2) 
		: "r" (valp), "r"(inc) 
		: "cc", "memory");
#elif defined __mips__
	__val = atomic_decrement_val(valp);
#elif defined __sparcv9__
	mvp_atomic_t __newval, __oldval = (*valp);
	do
		{
			__newval = __oldval - 1;
			__asm__ ("cas	[%4], %2, %0"
				 : "=r" (__oldval), "=m" (*valp)
				 : "r" (__oldval), "m" (*valp), "r"((valp)), "0" (__newval));
		}
	while (__newval != __oldval);
	/*  The value for __val is in '__oldval' */
	__val = __oldval;
#elif defined _MSC_VER
	__val = InterlockedDecrement(valp);
#elif defined __GNUC__
	/*
	 * Don't know how to atomic decrement for a generic architecture
	 * so use GCC builtin
	 */
	__val = __sync_sub_and_fetch(valp,1);
#else
#if !defined(_MSC_VER)
#warning unknown architecture, atomic deccrement is not...
#endif
	__val = --(*valp);
#endif
	return __val;
}
#endif

#define mvp_atomic_inc __mvp_atomic_inc
static inline int mvp_atomic_inc(mvp_atomic_t *a) {
	return __mvp_atomic_increment(a);
};

#define mvp_atomic_dec __mvp_atomic_dec
static inline int mvp_atomic_dec(mvp_atomic_t *a) {
	return __mvp_atomic_decrement(a);
};

#define mvp_atomic_dec_and_test __mvp_atomic_dec_and_test
static inline int mvp_atomic_dec_and_test(mvp_atomic_t *a) {
	return (__mvp_atomic_decrement(a) == 0);
};

#define mvp_atomic_set __mvp_atomic_set
static inline void mvp_atomic_set(mvp_atomic_t *a, unsigned val) {
	*a = val;
};

#define mvp_atomic_val __mvp_atomic_val
static inline int mvp_atomic_val(mvp_atomic_t *a) {
	return *a;
};

#ifdef __APPLE__
#pragma GCC optimization_level reset
#endif

#endif  /* __MVP_ATOMIC_H */
