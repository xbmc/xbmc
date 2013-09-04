/* A substitute for ISO C 1x <stdalign.h>.

   Copyright 2011 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Paul Eggert and Bruno Haible.  */

#ifndef _GL_STDALIGN_H
#define _GL_STDALIGN_H

/* ISO C1X <stdalign.h> for platforms that lack it.

   References:
   ISO C1X <http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf>
   sections 6.5.3.4, 6.7.5, 7.15.
   C++0X <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3242.pdf>
   section 18.10. */

/* alignof (TYPE), also known as _Alignof (TYPE), yields the alignment
   requirement of a structure member (i.e., slot or field) that is of
   type TYPE, as an integer constant expression.

   This differs from GCC's __alignof__ operator, which can yield a
   better-performing alignment for an object of that type.  For
   example, on x86 with GCC, __alignof__ (double) and __alignof__
   (long long) are 8, whereas alignof (double) and alignof (long long)
   are 4 unless the option '-malign-double' is used.

   The result cannot be used as a value for an 'enum' constant, if you
   want to be portable to HP-UX 10.20 cc and AIX 3.2.5 xlc.  */
#include <stddef.h>
#if defined __cplusplus
   template <class __t> struct __alignof_helper { char __a; __t __b; };
# define _Alignof(type) offsetof (__alignof_helper<type>, __b)
#else
# define _Alignof(type) offsetof (struct { char __a; type __b; }, __b)
#endif
#define alignof _Alignof
#define __alignof_is_defined 1

/* alignas (A), also known as _Alignas (A), aligns a variable or type
   to the alignment A, where A is an integer constant expression.  For
   example:

      int alignas (8) foo;
      struct s { int a; int alignas (8) bar; };

   aligns the address of FOO and the offset of BAR to be multiples of 8.

   A should be a power of two that is at least the type's alignment
   and at most the implementation's alignment limit.  This limit is
   2**28 on typical GNUish hosts, and 2**13 on MSVC.  To be portable
   to MSVC through at least version 10.0, A should be an integer
   constant, as MSVC does not support expressions such as 1 << 3.
   To be portable to Sun C 5.11, do not align auto variables to
   anything stricter than their default alignment.

   The following draft C1X requirements are not supported here:

     - If A is zero, alignas has no effect.
     - alignas can be used multiple times; the strictest one wins.
     - alignas (TYPE) is equivalent to alignas (alignof (TYPE)).

   */

#if __GNUC__ || __IBMC__ || __IBMCPP__ || 0x5110 <= __SUNPRO_C
# define _Alignas(a) __attribute__ ((__aligned__ (a)))
#elif 1300 <= _MSC_VER
# define _Alignas(a) __declspec (align (a))
#endif
#ifdef _Alignas
# define alignas _Alignas
# define __alignas_is_defined 1
#endif

#endif /* _GL_STDALIGN_H */
