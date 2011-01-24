dnl @synopsis AC_C_FIND_ENDIAN
dnl
dnl Determine endian-ness of target processor.
dnl @version 1.1	Mar 03 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Majority written from scratch to replace the standard autoconf macro 
dnl AC_C_BIGENDIAN. Only part remaining from the original it the invocation
dnl of the AC_TRY_RUN macro.
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.

dnl Find endian-ness in the following way:
dnl    1) Look in <endian.h>.
dnl    2) If 1) fails, look in <sys/types.h> and <sys/param.h>.
dnl    3) If 1) and 2) fails and not cross compiling run a test program.
dnl    4) If 1) and 2) fails and cross compiling then guess based on target.

AC_DEFUN([AC_C_FIND_ENDIAN],
[AC_CACHE_CHECK(processor byte ordering, 
	ac_cv_c_byte_order,

# Initialize to unknown
ac_cv_c_byte_order=unknown

if test x$ac_cv_header_endian_h = xyes ; then

	# First try <endian.h> which should set BYTE_ORDER.

	[AC_TRY_LINK([
		#include <endian.h>
		#if BYTE_ORDER != LITTLE_ENDIAN
			not big endian
		#endif
		], return 0 ;, 
			ac_cv_c_byte_order=little
		)]
				
	[AC_TRY_LINK([
		#include <endian.h>
		#if BYTE_ORDER != BIG_ENDIAN
			not big endian
		#endif
		], return 0 ;, 
			ac_cv_c_byte_order=big
		)]

	fi

if test $ac_cv_c_byte_order = unknown ; then

	[AC_TRY_LINK([
		#include <sys/types.h>
		#include <sys/param.h>
		#if !BYTE_ORDER || !BIG_ENDIAN || !LITTLE_ENDIAN
			bogus endian macros
		#endif
		], return 0 ;, 

		[AC_TRY_LINK([
			#include <sys/types.h>
			#include <sys/param.h>
			#if BYTE_ORDER != LITTLE_ENDIAN
				not big endian
			#endif
			], return 0 ;, 
				ac_cv_c_byte_order=little
			)]
				
		[AC_TRY_LINK([
			#include <sys/types.h>
			#include <sys/param.h>
			#if BYTE_ORDER != LITTLE_ENDIAN
				not big endian
			#endif
			], return 0 ;, 
				ac_cv_c_byte_order=little
			)]

		)]

 	fi

if test $ac_cv_c_byte_order = unknown ; then
	if test $cross_compiling = yes ; then
		# This is the last resort. Try to guess the target processor endian-ness
		# by looking at the target CPU type.	
		[
		case "$target_cpu" in
			alpha* | i?86* | mipsel* | ia64*)
				ac_cv_c_big_endian=0
				ac_cv_c_little_endian=1
				;;
			
			m68* | mips* | powerpc* | hppa* | sparc*)
				ac_cv_c_big_endian=1
				ac_cv_c_little_endian=0
				;;
	
			esac
		]
	else
		AC_TRY_RUN(
		[[
		int main (void) 
		{	/* Are we little or big endian?  From Harbison&Steele.  */
			union
			{	long l ;
				char c [sizeof (long)] ;
			} u ;
			u.l = 1 ;
			return (u.c [sizeof (long) - 1] == 1);
			}
			]], , ac_cv_c_byte_order=big, 
			ac_cv_c_byte_order=unknown
			)

		AC_TRY_RUN(
		[[int main (void) 
		{	/* Are we little or big endian?  From Harbison&Steele.  */
			union
			{	long l ;
				char c [sizeof (long)] ;
			} u ;
			u.l = 1 ;
			return (u.c [0] == 1);
			}]], , ac_cv_c_byte_order=little, 
			ac_cv_c_byte_order=unknown
			)
		fi	
	fi

)
]

if test $ac_cv_c_byte_order = big ; then
	ac_cv_c_big_endian=1
	ac_cv_c_little_endian=0
elif test $ac_cv_c_byte_order = little ; then
	ac_cv_c_big_endian=0
	ac_cv_c_little_endian=1
else
	ac_cv_c_big_endian=0
	ac_cv_c_little_endian=0

	AC_MSG_WARN([[*****************************************************************]])
	AC_MSG_WARN([[*** Not able to determine endian-ness of target processor.       ]])
	AC_MSG_WARN([[*** The constants CPU_IS_BIG_ENDIAN and CPU_IS_LITTLE_ENDIAN in  ]])
	AC_MSG_WARN([[*** src/config.h may need to be hand editied.                    ]])
	AC_MSG_WARN([[*****************************************************************]])
	fi

)# AC_C_FIND_ENDIAN





dnl @synopsis AC_C99_FUNC_LRINT
dnl
dnl Check whether C99's lrint function is available.
dnl @version 1.3	Feb 12 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LRINT],
[AC_CACHE_CHECK(for lrint,
  ac_cv_c99_lrint,
[
lrint_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
], if (!lrint(3.14159)) lrint(2.7183);, ac_cv_c99_lrint=yes, ac_cv_c99_lrint=no)

CFLAGS=$lrint_save_CFLAGS

])

if test "$ac_cv_c99_lrint" = yes; then
  AC_DEFINE(HAVE_LRINT, 1,
            [Define if you have C99's lrint function.])
fi
])# AC_C99_FUNC_LRINT





dnl @synopsis AC_C99_FUNC_LRINTF
dnl
dnl Check whether C99's lrintf function is available.
dnl @version 1.3	Feb 12 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl

AC_DEFUN([AC_C99_FUNC_LRINTF],
[AC_CACHE_CHECK(for lrintf,
  ac_cv_c99_lrintf,
[
lrintf_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
], if (!lrintf(3.14159)) lrintf(2.7183);, ac_cv_c99_lrintf=yes, ac_cv_c99_lrintf=no)

CFLAGS=$lrintf_save_CFLAGS

])

if test "$ac_cv_c99_lrintf" = yes; then
  AC_DEFINE(HAVE_LRINTF, 1,
            [Define if you have C99's lrintf function.])
fi
])# AC_C99_FUNC_LRINTF





dnl @synopsis AC_C99_FUNC_LLRINT
dnl
dnl Check whether C99's llrint function is available.
dnl @version 1.1	Sep 30 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LLRINT],
[AC_CACHE_CHECK(for llrint,
  ac_cv_c99_llrint,
[
llrint_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
#include <stdint.h>
], int64_t	x ; x = llrint(3.14159) ;, ac_cv_c99_llrint=yes, ac_cv_c99_llrint=no)

CFLAGS=$llrint_save_CFLAGS

])

if test "$ac_cv_c99_llrint" = yes; then
  AC_DEFINE(HAVE_LLRINT, 1,
            [Define if you have C99's llrint function.])
fi
])# AC_C99_FUNC_LLRINT





dnl @synopsis AC_C_CLIP_MODE
dnl
dnl Determine the clipping mode when converting float to int.
dnl @version 1.0	May 17 2003
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.

dnl Find the clipping mode in the following way:
dnl    1) If we are not cross compiling test it.
dnl    2) IF we are cross compiling, assume that clipping isn't done correctly.

AC_DEFUN([AC_C_CLIP_MODE],
[AC_CACHE_CHECK(processor clipping capabilities, 
	ac_cv_c_clip_type,

# Initialize to unknown
ac_cv_c_clip_positive=unknown
ac_cv_c_clip_negative=unknown

if test $ac_cv_c_clip_positive = unknown ; then
	AC_TRY_RUN(
	[[
	#define	_ISOC9X_SOURCE	1
	#define _ISOC99_SOURCE	1
	#define	__USE_ISOC99	1
	#define __USE_ISOC9X	1
	#include <math.h>
	int main (void)
	{	double	fval ;
		int k, ival ;

		fval = 1.0 * 0x7FFFFFFF ;
		for (k = 0 ; k < 100 ; k++)
		{	ival = (lrint (fval)) >> 24 ;
			if (ival != 127)
				return 1 ;
		
			fval *= 1.2499999 ;
			} ;
		
			return 0 ;
		}
		]],
		ac_cv_c_clip_positive=yes,
		ac_cv_c_clip_positive=no,
		ac_cv_c_clip_positive=unknown
		)

	AC_TRY_RUN(
	[[
	#define	_ISOC9X_SOURCE	1
	#define _ISOC99_SOURCE	1
	#define	__USE_ISOC99	1
	#define __USE_ISOC9X	1
	#include <math.h>
	int main (void)
	{	double	fval ;
		int k, ival ;

		fval = -8.0 * 0x10000000 ;
		for (k = 0 ; k < 100 ; k++)
		{	ival = (lrint (fval)) >> 24 ;
			if (ival != -128)
				return 1 ;
		
			fval *= 1.2499999 ;
			} ;
		
			return 0 ;
		}
		]],
		ac_cv_c_clip_negative=yes,
		ac_cv_c_clip_negative=no,
		ac_cv_c_clip_negative=unknown
		)

	fi

if test $ac_cv_c_clip_positive = yes ; then
	ac_cv_c_clip_positive=1
else
	ac_cv_c_clip_positive=0
	fi

if test $ac_cv_c_clip_negative = yes ; then
	ac_cv_c_clip_negative=1
else
	ac_cv_c_clip_negative=0
	fi

[[
case "$ac_cv_c_clip_positive$ac_cv_c_clip_negative" in
	"00")
		ac_cv_c_clip_type="none"
		;;
	"10")
		ac_cv_c_clip_type="positive"
		;;
	"01")
		ac_cv_c_clip_type="negative"
		;;
	"11")
		ac_cv_c_clip_type="both"
		;;
	esac
	]]

)
]

)# AC_C_CLIP_MODE





dnl @synopsis AC_CHECK_SIGNAL(SIGNAME)
dnl
dnl
dnl @category C
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl @version 1.0	Jul 07 2007

AC_DEFUN([AC_CHECK_SIGNAL],
[AC_CACHE_CHECK(for $1,
  ac_cv_signal_$1,
[
AC_TRY_LINK([
#include <signal.h>

], signal($1, SIG_DFL) ;, ac_cv_signal_$1=yes, ac_cv_signal_$1=no)

])

if test "$ac_cv_signal_$1" = yes; then
  AC_DEFINE(HAVE_$1, 1,
            [Define if you have signal $1.])
fi
])# AC_CHECK_SIGNAL


## AC_CHECK_SIGNALS
