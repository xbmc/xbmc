/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef OPTCODE_H_INCLUDED
#define OPTCODE_H_INCLUDED 1

/* optimizing mode */
/* 0: none         */
/* 1: x86 asm      */
#define OPT_MODE 1

/* PowerPC's AltiVec enhancement */
/* 0: none                       */
/* 1: use altivec                */
/*    (need -faltivec option)    */
#ifndef USE_ALTIVEC
#define USE_ALTIVEC 0
#endif

/*****************************************************************************/

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif/* <sys/param.h> */
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif/* <sys/sysctl.h> */
#ifdef STDC_HEADERS
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif/* <string.h> */

#if __STDC_VERSION__ == 199901L
# include <stdbool.h>
#endif

#ifdef LITTLE_ENDIAN
#define iman_ 0
#else
#define iman_ 1
#endif
#define _double2fixmagic 68719476736.0 * 1.5

#define imuldiv8(a, b) \
    (int32)(((int64)(a) * (int64)(b)) >> 8)

#define imuldiv16(a, b) \
    (int32)(((int64)(a) * (int64)(b)) >> 16)

#define imuldiv24(a, b) \
    (int32)(((int64)(a) * (int64)(b)) >> 24)

#define imuldiv28(a, b) \
    (int32)(((int64)(a) * (int64)(b)) >> 28)

static inline int32 signlong(int32 a)
{
	return ((a | 0x7fffffff) >> 30);
}

/*****************************************************************************/
#if USE_ALTIVEC

#ifndef __bool_true_false_are_defined
#define bool _Bool
typedef enum { false = 0, true = 1 } bool;
#endif /* C99 Hack */

/* typedefs */
typedef vector signed int  vint32;
typedef vector signed char vint8;

/* prototypes */
void v_memset(void* dest, int c, size_t len);
void v_memzero(void* dest, size_t len);
void v_set_dry_signal(void* dest, const int32* buf, int32 n);

/* inline functions */
extern inline bool is_altivec_available(void)
{
  int sel[2] = { CTL_HW, HW_VECTORUNIT };
  int has_altivec = false;
  size_t len = sizeof(has_altivec);
  int error = sysctl(sel, 2, &has_altivec, &len, NULL, 0);
  if(!error) {
    return (bool)!!has_altivec;
  } else {
    return false;
  }
}

extern inline void libc_memset(void* destp, int c, size_t len)
{
    memset(destp,c,len);
}

static inline void* switch_memset(void* destp, int c, size_t len)
{
    void* keepdestp = destp;
    if(!is_altivec_available()) {
        libc_memset(destp,c,len);
    } else if (c) {
        v_memset(destp,c,len);
    } else {
        v_memzero(destp,len);
    }
    return keepdestp;
}

#define memset switch_memset
#endif /* altivec */

#endif /* OPTCODE_H_INCLUDED */
