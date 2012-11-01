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

#ifdef _MSC_VER
#include <windows.h>
#endif
/**
 * Atomically incremente a reference count variable.
 * \param valp address of atomic variable
 * \return incremented reference count
 */
typedef	unsigned mvp_atomic_t;
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
#elif defined __i386__
	asm volatile (".byte 0xf0, 0x0f, 0xc1, 0x02" /*lock; xaddl %eax, (%edx) */
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
#else
	/*
	 * Don't know how to atomic increment for a generic architecture
	 * so punt and just increment the value.
	 */
#ifdef _WIN32
  #pragma message("unknown architecture, atomic increment is not...");
#else
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
#elif defined __i386__
	asm volatile (".byte 0xf0, 0x0f, 0xc1, 0x02" /*lock; xaddl %eax, (%edx) */
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
#else
	/*
	 * Don't know how to atomic decrement for a generic architecture
	 * so punt and just decrement the value.
	 */
//#warning unknown architecture, atomic decrement is not...
	__val = --(*valp);
#endif
	return __val;
}
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

#ifdef __APPLE__
#pragma GCC optimization_level reset
#endif

#endif  /* __MVP_ATOMIC_H */
