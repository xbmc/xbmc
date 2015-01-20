/*
 *      Copyright (C) 2014 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 06, 07 by Ralf Baechle (ralf@linux-mips.org)
 *
 * Most of this file was borrowed from the linux kernel.
 */

#pragma once

#include <inttypes.h>
#include <pthread.h>

extern pthread_mutex_t cmpxchg_mutex;

static inline long cmpxchg32(volatile long *m, long oldval, long newval)
{
	long retval;
	__asm__ __volatile__(						\
		"	.set	push				\n"	\
		"	.set	noat				\n"	\
		"	.set	mips3				\n"	\
		"1:	ll	%0, %2		# __cmpxchg_asm	\n"	\
		"	bne	%0, %z3, 2f			\n"	\
		"	.set	mips0				\n"	\
		"	move	$1, %z4				\n"	\
		"	.set	mips3				\n"	\
		"	sc	$1, %1				\n"	\
		"	beqz	$1, 3f				\n"	\
		"2:						\n"	\
		"	.subsection 2				\n"	\
		"3:	b	1b				\n"	\
		"	.previous				\n"	\
		"	.set	pop				\n"	\
		: "=&r" (retval), "=R" (*m)				\
		: "R" (*m), "Jr" (oldval), "Jr" (newval)			\
		: "memory");						\

	return retval;
}


static inline long long cmpxchg64(volatile long long *ptr,
				      long long oldval, long long newval)
{
	long long prev;

	pthread_mutex_lock(&cmpxchg_mutex);
	prev = *(long long *)ptr;
	if (prev == oldval)
		*(long long *)ptr = newval;
	pthread_mutex_unlock(&cmpxchg_mutex);
	return prev;
}


static __inline__ long atomic_add(int i, volatile long* v)
{
	long temp;

	__asm__ __volatile__(
		"	.set	mips3					\n"
		"1:	ll	%0, %1		# atomic_add		\n"
		"	addu	%0, %2					\n"
		"	sc	%0, %1					\n"
		"	beqz	%0, 2f					\n"
		"	.subsection 2					\n"
		"2:	b	1b					\n"
		"	.previous					\n"
		"	.set	mips0					\n"
		: "=&r" (temp), "=m" (*v)
		: "Ir" (i), "m" (*v));

	return temp;
}

static __inline__ long atomic_sub(int i, volatile long* v)
{
	long temp;

	__asm__ __volatile__(
		"	.set	mips3					\n"
		"1:	ll	%0, %1		# atomic_sub		\n"
		"	subu	%0, %2					\n"
		"	sc	%0, %1					\n"
		"	beqz	%0, 2f					\n"
		"	.subsection 2					\n"
		"2:	b	1b					\n"
		"	.previous					\n"
		"	.set	mips0					\n"
		: "=&r" (temp), "=m" (*v)
		: "Ir" (i), "m" (*v));

	return temp;
}
