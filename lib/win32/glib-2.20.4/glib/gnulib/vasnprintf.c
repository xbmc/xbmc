/* vsprintf with automatic memory allocation.
   Copyright (C) 1999, 2002-2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

#ifndef _WIN32
/* Tell glibc's <stdio.h> to provide a prototype for snprintf().
   This must come before <config.h> because <config.h> may include
   <features.h>, and once <features.h> has been included, it's too late.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE    1
#endif
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "glib/galloca.h"

#include "g-gnulib.h"

/* Specification.  */
#include "vasnprintf.h"

#include <stdio.h>	/* snprintf(), sprintf() */
#include <stdlib.h>	/* abort(), malloc(), realloc(), free() */
#include <string.h>	/* memcpy(), strlen() */
#include <errno.h>	/* errno */
#include <limits.h>	/* CHAR_BIT */
#include <float.h>	/* DBL_MAX_EXP, LDBL_MAX_EXP */
#include "printf-parse.h"

#ifdef HAVE_WCHAR_T
# ifdef HAVE_WCSLEN
#  define local_wcslen wcslen
# else
   /* Solaris 2.5.1 has wcslen() in a separate library libw.so. To avoid
      a dependency towards this library, here is a local substitute.
      Define this substitute only once, even if this file is included
      twice in the same compilation unit.  */
#  ifndef local_wcslen_defined
#   define local_wcslen_defined 1
static size_t
local_wcslen (const wchar_t *s)
{
  const wchar_t *ptr;

  for (ptr = s; *ptr != (wchar_t) 0; ptr++)
    ;
  return ptr - s;
}
#  endif
# endif
#endif

/* For those losing systems which don't have 'alloca' we have to add
   some additional code emulating it.  */ 
#ifdef HAVE_ALLOCA 
# define freea(p) /* nothing */
#else
# define alloca(n) malloc (n) 
# define freea(p) free (p) 
#endif

#ifndef HAVE_LONG_LONG_FORMAT
static int
print_long_long (char *buf, 
		 int len, 
		 int width,
		 int precision,
		 unsigned long flags,
		 char conversion,
		 unsigned long long number)
{
  int negative = FALSE;
  char buffer[128];
  char *bufferend;
  char *pointer;
  int base;
  static const char *upper = "0123456789ABCDEFX";
  static const char *lower = "0123456789abcdefx";
  const char *digits;
  int i;
  char *p;
  int count;

#define EMIT(c)           \
  if (p - buf == len - 1) \
    {                     \
      *p++ = '\0';        \
      return len;         \
    }                     \
  else                    \
    *p++ = c;
  
  p = buf;
  
  switch (conversion) 
    {
    case 'o':
      base = 8;
      digits = lower;
      negative = FALSE;
      break;
    case 'x':
      base = 16;
      digits = lower;
      negative = FALSE;
      break;
    case 'X':
      base = 16;
      digits = upper;
      negative = FALSE;
      break;
    default:
      base = 10;
      digits = lower;
      negative = (long long)number < 0;
      if (negative) 
	number = -((long long)number);
      break;
    }

  /* Build number */
  pointer = bufferend = &buffer[sizeof(buffer) - 1];
  *pointer-- = '\0';
  for (i = 1; i < (int)sizeof(buffer); i++)
    {
      *pointer-- = digits[number % base];
      number /= base;
      if (number == 0)
	break;
    }

  /* Adjust width */
  width -= (bufferend - pointer) - 1;

  /* Adjust precision */
  if (precision != -1)
    {
      precision -= (bufferend - pointer) - 1;
      if (precision < 0)
	precision = 0;
      flags |= FLAG_ZERO;
    }

  /* Adjust width further */
  if (negative || (flags & FLAG_SHOWSIGN) || (flags & FLAG_SPACE))
    width--;
  if (flags & FLAG_ALT)
    {
      switch (base)
	{
	case 16:
	  width -= 2;
	  break;
	case 8:
	  width--;
	  break;
        default:
	  break;
	}
    }

  /* Output prefixes spaces if needed */
  if (! ((flags & FLAG_LEFT) ||
	 ((flags & FLAG_ZERO) && (precision == -1))))
    {
      count = (precision == -1) ? 0 : precision;
      while (width-- > count)
	*p++ = ' ';
    }

  /* width has been adjusted for signs and alternatives */
  if (negative) 
    {
      EMIT ('-');
    }
  else if (flags & FLAG_SHOWSIGN) 
    {
      EMIT('+');
    }
  else if (flags & FLAG_SPACE) 
    {
      EMIT(' ');
    }
  
  if (flags & FLAG_ALT)
    {
      switch (base)
	{
	case 8:
	  EMIT('0');
	  break;
	case 16:
	  EMIT('0');
	  EMIT(digits[16]);
	  break;
	default:
	  break;
	} /* switch base */
    }
  
  /* Output prefixed zero padding if needed */
  if (flags & FLAG_ZERO)
    {
      if (precision == -1)
	precision = width;
      while (precision-- > 0)
	{
	  EMIT('0');
	  width--;
	}
    }
  
  /* Output the number itself */
  while (*(++pointer))
    {
      EMIT(*pointer);
    }
  
  /* Output trailing spaces if needed */
  if (flags & FLAG_LEFT)
    {
      while (width-- > 0)
	EMIT(' ');
    }
  
  EMIT('\0');
  
  return p - buf - 1;
}
#endif

char *
vasnprintf (char *resultbuf, size_t *lengthp, const char *format, va_list args)
{
  char_directives d;
  arguments a;

  if (printf_parse (format, &d, &a) < 0)
    {
      errno = EINVAL;
      return NULL;
    }

#define CLEANUP() \
  free (d.dir);								\
  if (a.arg)								\
    free (a.arg);

  if (printf_fetchargs (args, &a) < 0)
    {
      CLEANUP ();
      errno = EINVAL;
      return NULL;
    }

  {
    char *buf =
      (char *) alloca (7 + d.max_width_length + d.max_precision_length + 6);
    const char *cp;
    unsigned int i;
    char_directive *dp;
    /* Output string accumulator.  */
    char *result;
    size_t allocated;
    size_t length;

    if (resultbuf != NULL)
      {
	result = resultbuf;
	allocated = *lengthp;
      }
    else
      {
	result = NULL;
	allocated = 0;
      }
    length = 0;
    /* Invariants:
       result is either == resultbuf or == NULL or malloc-allocated.
       If length > 0, then result != NULL.  */

#define ENSURE_ALLOCATION(needed) \
    if ((needed) > allocated)						\
      {									\
	char *memory;							\
									\
	allocated = (allocated > 0 ? 2 * allocated : 12);		\
	if ((needed) > allocated)					\
	  allocated = (needed);						\
	if (result == resultbuf || result == NULL)			\
	  memory = (char *) malloc (allocated);				\
	else								\
	  memory = (char *) realloc (result, allocated);		\
									\
	if (memory == NULL)						\
	  {								\
	    if (!(result == resultbuf || result == NULL))		\
	      free (result);						\
	    freea (buf);						\
	    CLEANUP ();							\
	    errno = ENOMEM;						\
	    return NULL;						\
	  }								\
	if (result == resultbuf && length > 0)				\
	  memcpy (memory, result, length);				\
	result = memory;						\
      }

    for (cp = format, i = 0, dp = &d.dir[0]; ; cp = dp->dir_end, i++, dp++)
      {
	if (cp != dp->dir_start)
	  {
	    size_t n = dp->dir_start - cp;

	    ENSURE_ALLOCATION (length + n);
	    memcpy (result + length, cp, n);
	    length += n;
	  }
	if (i == d.count)
	  break;

	/* Execute a single directive.  */
	if (dp->conversion == '%')
	  {
	    if (!(dp->arg_index < 0))
	      abort ();
	    ENSURE_ALLOCATION (length + 1);
	    result[length] = '%';
	    length += 1;
	  }
	else
	  {
	    if (!(dp->arg_index >= 0))
	      abort ();

	    if (dp->conversion == 'n')
	      {
		switch (a.arg[dp->arg_index].type)
		  {
		  case TYPE_COUNT_SCHAR_POINTER:
		    *a.arg[dp->arg_index].a.a_count_schar_pointer = length;
		    break;
		  case TYPE_COUNT_SHORT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_short_pointer = length;
		    break;
		  case TYPE_COUNT_INT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_int_pointer = length;
		    break;
		  case TYPE_COUNT_LONGINT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_longint_pointer = length;
		    break;
#ifdef HAVE_LONG_LONG
		  case TYPE_COUNT_LONGLONGINT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_longlongint_pointer = length;
		    break;
#endif
		  default:
		    abort ();
		  }
	      }
	    else
	      {
		arg_type type = a.arg[dp->arg_index].type;
		char *p;
		unsigned int prefix_count;
		int prefixes[2];
#if !HAVE_SNPRINTF
		unsigned int tmp_length;
		char tmpbuf[700];
		char *tmp;

		/* Allocate a temporary buffer of sufficient size for calling
		   sprintf.  */
		{
		  unsigned int width;
		  unsigned int precision;

		  width = 0;
		  if (dp->width_start != dp->width_end)
		    {
		      if (dp->width_arg_index >= 0)
			{
			  int arg;

			  if (!(a.arg[dp->width_arg_index].type == TYPE_INT))
			    abort ();
			  arg = a.arg[dp->width_arg_index].a.a_int;
			  width = (arg < 0 ? -arg : arg);
			}
		      else
			{
			  const char *digitp = dp->width_start;

			  do
			    width = width * 10 + (*digitp++ - '0');
			  while (digitp != dp->width_end);
			}
		    }

		  precision = 6;
		  if (dp->precision_start != dp->precision_end)
		    {
		      if (dp->precision_arg_index >= 0)
			{
			  int arg;

			  if (!(a.arg[dp->precision_arg_index].type == TYPE_INT))
			    abort ();
			  arg = a.arg[dp->precision_arg_index].a.a_int;
			  precision = (arg < 0 ? 0 : arg);
			}
		      else
			{
			  const char *digitp = dp->precision_start + 1;

			  precision = 0;
			  while (digitp != dp->precision_end)
			    precision = precision * 10 + (*digitp++ - '0');
			}
		    }

		  switch (dp->conversion)
		    {
		    case 'd': case 'i': case 'u':
# ifdef HAVE_LONG_LONG
		      if (type == TYPE_LONGLONGINT || type == TYPE_ULONGLONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long long) * CHAR_BIT
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
# endif
		      if (type == TYPE_LONGINT || type == TYPE_ULONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long) * CHAR_BIT
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
			tmp_length =
			  (unsigned int) (sizeof (unsigned int) * CHAR_BIT
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      break;

		    case 'o':
# ifdef HAVE_LONG_LONG
		      if (type == TYPE_LONGLONGINT || type == TYPE_ULONGLONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long long) * CHAR_BIT
					  * 0.333334 /* binary -> octal */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
# endif
		      if (type == TYPE_LONGINT || type == TYPE_ULONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long) * CHAR_BIT
					  * 0.333334 /* binary -> octal */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
			tmp_length =
			  (unsigned int) (sizeof (unsigned int) * CHAR_BIT
					  * 0.333334 /* binary -> octal */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      break;

		    case 'x': case 'X':
# ifdef HAVE_LONG_LONG
		      if (type == TYPE_LONGLONGINT || type == TYPE_ULONGLONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long long) * CHAR_BIT
					  * 0.25 /* binary -> hexadecimal */
					 )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading sign or alternate form */
		      else
# endif
# ifdef HAVE_INT64_AND_I64
		      if (type == TYPE_INT64 || type == TYPE_UINT64)
			tmp_length =
			  (unsigned int) (sizeof (unsigned __int64) * CHAR_BIT
					  * 0.25 /* binary -> hexadecimal */
					  )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading sign or alternate form */
		      else
# endif
		      if (type == TYPE_LONGINT || type == TYPE_ULONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long) * CHAR_BIT
					  * 0.25 /* binary -> hexadecimal */
					 )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading sign or alternate form */
		      else
			tmp_length =
			  (unsigned int) (sizeof (unsigned int) * CHAR_BIT
					  * 0.25 /* binary -> hexadecimal */
					 )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading sign or alternate form */
		      break;

		    case 'f': case 'F':
# ifdef HAVE_LONG_DOUBLE
		      if (type == TYPE_LONGDOUBLE)
			tmp_length =
			  (unsigned int) (LDBL_MAX_EXP
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + precision
			  + 10; /* sign, decimal point etc. */
		      else
# endif
			tmp_length =
			  (unsigned int) (DBL_MAX_EXP
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + precision
			  + 10; /* sign, decimal point etc. */
		      break;

		    case 'e': case 'E': case 'g': case 'G':
		    case 'a': case 'A':
		      tmp_length =
			precision
			+ 12; /* sign, decimal point, exponent etc. */
		      break;

		    case 'c':
# ifdef HAVE_WINT_T
		      if (type == TYPE_WIDE_CHAR)
			tmp_length = MB_CUR_MAX;
		      else
# endif
			tmp_length = 1;
		      break;

		    case 's':
# ifdef HAVE_WCHAR_T
		      if (type == TYPE_WIDE_STRING)
			tmp_length =
			  (a.arg[dp->arg_index].a.a_wide_string == NULL
			  ? 6 /* wcslen(L"(null)") */
			   : local_wcslen (a.arg[dp->arg_index].a.a_wide_string)) 
			  * MB_CUR_MAX;
		      else
# endif
			tmp_length = a.arg[dp->arg_index].a.a_string == NULL
			  ? 6 /* strlen("(null)") */
			  : strlen (a.arg[dp->arg_index].a.a_string);
		      break;

		    case 'p':
		      tmp_length =
			(unsigned int) (sizeof (void *) * CHAR_BIT
					* 0.25 /* binary -> hexadecimal */
				       )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading 0x */
		      break;

		    default:
		      abort ();
		    }

		  if (tmp_length < width)
		    tmp_length = width;

		  tmp_length++; /* account for trailing NUL */
		}

		if (tmp_length <= sizeof (tmpbuf))
		  tmp = tmpbuf;
		else
		  {
		    tmp = (char *) malloc (tmp_length);
		    if (tmp == NULL)
		      {
			/* Out of memory.  */
			if (!(result == resultbuf || result == NULL))
			  free (result);
			freea (buf);
			CLEANUP ();
			errno = ENOMEM;
			return NULL;
		      }
		  }
#endif

		/* Construct the format string for calling snprintf or
		   sprintf.  */
		p = buf;
		*p++ = '%';
		if (dp->flags & FLAG_GROUP)
		  *p++ = '\'';
		if (dp->flags & FLAG_LEFT)
		  *p++ = '-';
		if (dp->flags & FLAG_SHOWSIGN)
		  *p++ = '+';
		if (dp->flags & FLAG_SPACE)
		  *p++ = ' ';
		if (dp->flags & FLAG_ALT)
		  *p++ = '#';
		if (dp->flags & FLAG_ZERO)
		  *p++ = '0';
		if (dp->width_start != dp->width_end)
		  {
		    size_t n = dp->width_end - dp->width_start;
		    memcpy (p, dp->width_start, n);
		    p += n;
		  }
		if (dp->precision_start != dp->precision_end)
		  {
		    size_t n = dp->precision_end - dp->precision_start;
		    memcpy (p, dp->precision_start, n);
		    p += n;
		  }

		switch (type)
		  {
#ifdef HAVE_INT64_AND_I64
		  case TYPE_INT64:
		  case TYPE_UINT64:
		    *p++ = 'I';
		    *p++ = '6';
		    *p++ = '4';
		    break;
#endif
#ifdef HAVE_LONG_LONG
		  case TYPE_LONGLONGINT:
		  case TYPE_ULONGLONGINT:
#ifdef HAVE_INT64_AND_I64	/* The system (sn)printf uses %I64. Also assume
				 * that long long == __int64.
				 */
		    *p++ = 'I';
		    *p++ = '6';
		    *p++ = '4';
		    break;
#else
		    *p++ = 'l';
		    /*FALLTHROUGH*/
#endif
#endif
		  case TYPE_LONGINT:
		  case TYPE_ULONGINT:
#ifdef HAVE_WINT_T
		  case TYPE_WIDE_CHAR:
#endif
#ifdef HAVE_WCHAR_T
		  case TYPE_WIDE_STRING:
#endif
		    *p++ = 'l';
		    break;
#ifdef HAVE_LONG_DOUBLE
		  case TYPE_LONGDOUBLE:
		    *p++ = 'L';
		    break;
#endif
		  default:
		    break;
		  }
		*p = dp->conversion;
#if HAVE_SNPRINTF
		p[1] = '%';
		p[2] = 'n';
		p[3] = '\0';
#else
		p[1] = '\0';
#endif

		/* Construct the arguments for calling snprintf or sprintf.  */
		prefix_count = 0;
		if (dp->width_arg_index >= 0)
		  {
		    if (!(a.arg[dp->width_arg_index].type == TYPE_INT))
		      abort ();
		    prefixes[prefix_count++] = a.arg[dp->width_arg_index].a.a_int;
		  }
		if (dp->precision_arg_index >= 0)
		  {
		    if (!(a.arg[dp->precision_arg_index].type == TYPE_INT))
		      abort ();
		    prefixes[prefix_count++] = a.arg[dp->precision_arg_index].a.a_int;
		  }

#if HAVE_SNPRINTF
		/* Prepare checking whether snprintf returns the count
		   via %n.  */
		ENSURE_ALLOCATION (length + 1);
		result[length] = '\0';
#endif

		for (;;)
		  {
		    size_t maxlen;
		    int count;
		    int retcount;

		    maxlen = allocated - length;
		    count = -1;
		    retcount = 0;

#if HAVE_SNPRINTF
#define SNPRINTF_BUF(arg) \
		    switch (prefix_count)				    \
		      {							    \
		      case 0:						    \
			retcount = snprintf (result + length, maxlen, buf,  \
					     arg, &count);		    \
			break;						    \
		      case 1:						    \
			retcount = snprintf (result + length, maxlen, buf,  \
					     prefixes[0], arg, &count);	    \
			break;						    \
		      case 2:						    \
			retcount = snprintf (result + length, maxlen, buf,  \
					     prefixes[0], prefixes[1], arg, \
					     &count);			    \
			break;						    \
		      default:						    \
			abort ();					    \
		      }
#else
#define SNPRINTF_BUF(arg) \
		    switch (prefix_count)				    \
		      {							    \
		      case 0:						    \
			count = sprintf (tmp, buf, arg);		    \
			break;						    \
		      case 1:						    \
			count = sprintf (tmp, buf, prefixes[0], arg);	    \
			break;						    \
		      case 2:						    \
			count = sprintf (tmp, buf, prefixes[0], prefixes[1],\
					 arg);				    \
			break;						    \
		      default:						    \
			abort ();					    \
		      }
#endif

		    switch (type)
		      {
		      case TYPE_SCHAR:
			{
			  int arg = a.arg[dp->arg_index].a.a_schar;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_UCHAR:
			{
			  unsigned int arg = a.arg[dp->arg_index].a.a_uchar;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_SHORT:
			{
			  int arg = a.arg[dp->arg_index].a.a_short;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_USHORT:
			{
			  unsigned int arg = a.arg[dp->arg_index].a.a_ushort;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_INT:
			{
			  int arg = a.arg[dp->arg_index].a.a_int;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_UINT:
			{
			  unsigned int arg = a.arg[dp->arg_index].a.a_uint;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_LONGINT:
			{
			  long int arg = a.arg[dp->arg_index].a.a_longint;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_ULONGINT:
			{
			  unsigned long int arg = a.arg[dp->arg_index].a.a_ulongint;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_INT64_AND_I64
		      case TYPE_INT64:
			{
			  __int64 arg = a.arg[dp->arg_index].a.a_int64;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_UINT64:
			{
			  unsigned __int64 arg = a.arg[dp->arg_index].a.a_uint64;
			  SNPRINTF_BUF (arg);
			}
			break;			
#endif
#ifdef HAVE_LONG_LONG
#ifndef HAVE_LONG_LONG_FORMAT
		      case TYPE_LONGLONGINT:
		      case TYPE_ULONGLONGINT:
			{
			  unsigned long long int arg = a.arg[dp->arg_index].a.a_ulonglongint;
			  int width;
			  int precision;

			  width = 0;
			  if (dp->width_start != dp->width_end)
			    {
			      if (dp->width_arg_index >= 0)
				{
				  int arg;
				  
				  if (!(a.arg[dp->width_arg_index].type == TYPE_INT))
				    abort ();
				  arg = a.arg[dp->width_arg_index].a.a_int;
				  width = (arg < 0 ? -arg : arg);
				}
			      else
				{
				  const char *digitp = dp->width_start;
				  
				  do
				    width = width * 10 + (*digitp++ - '0');
				  while (digitp != dp->width_end);
				}
			    }

			  precision = -1;
			  if (dp->precision_start != dp->precision_end)
			    {
			      if (dp->precision_arg_index >= 0)
				{
				  int arg;
				  
				  if (!(a.arg[dp->precision_arg_index].type == TYPE_INT))
				    abort ();
				  arg = a.arg[dp->precision_arg_index].a.a_int;
				  precision = (arg < 0 ? 0 : arg);
				}
			      else
				{
				  const char *digitp = dp->precision_start + 1;
				  
				  precision = 0;
				  do
				    precision = precision * 10 + (*digitp++ - '0');
				  while (digitp != dp->precision_end);
				}
			    }
			  
#if HAVE_SNPRINTF
 			  count = print_long_long (result + length, maxlen,
 						   width, precision,
 						   dp->flags,
 						   dp->conversion,
 						   arg);
#else
			  count = print_long_long (tmp, tmp_length,
						   width, precision,
						   dp->flags,
						   dp->conversion,
						   arg);
#endif
			}
			break;
#else
		      case TYPE_LONGLONGINT:
			{
			  long long int arg = a.arg[dp->arg_index].a.a_longlongint;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_ULONGLONGINT:
			{
			  unsigned long long int arg = a.arg[dp->arg_index].a.a_ulonglongint;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
#endif
		      case TYPE_DOUBLE:
			{
			  double arg = a.arg[dp->arg_index].a.a_double;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_LONG_DOUBLE
		      case TYPE_LONGDOUBLE:
			{
			  long double arg = a.arg[dp->arg_index].a.a_longdouble;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
		      case TYPE_CHAR:
			{
			  int arg = a.arg[dp->arg_index].a.a_char;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_WINT_T
		      case TYPE_WIDE_CHAR:
			{
			  wint_t arg = a.arg[dp->arg_index].a.a_wide_char;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
		      case TYPE_STRING:
			{
			  const char *arg = a.arg[dp->arg_index].a.a_string == NULL
			    ? "(null)"
			    : a.arg[dp->arg_index].a.a_string;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_WCHAR_T
		      case TYPE_WIDE_STRING:
			{
			  const wchar_t *arg = a.arg[dp->arg_index].a.a_wide_string == NULL
			    ? L"(null)"
			    : a.arg[dp->arg_index].a.a_wide_string;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
		      case TYPE_POINTER:
			{
			  void *arg = a.arg[dp->arg_index].a.a_pointer;
			  SNPRINTF_BUF (arg);
			}
			break;
		      default:
			abort ();
		      }

#if HAVE_SNPRINTF
		    /* Portability: Not all implementations of snprintf()
		       are ISO C 99 compliant.  Determine the number of
		       bytes that snprintf() has produced or would have
		       produced.  */
		    if (count >= 0)
		      {
			/* Verify that snprintf() has NUL-terminated its
			   result.  */
			if (count < maxlen && result[length + count] != '\0')
			  abort ();
			/* Portability hack.  */
			if (retcount > count)
			  count = retcount;
		      }
		    else
		      {
			/* snprintf() doesn't understand the '%n'
			   directive.  */
			if (p[1] != '\0')
			  {
			    /* Don't use the '%n' directive; instead, look
			       at the snprintf() return value.  */
			    p[1] = '\0';
			    continue;
			  }
			count = retcount;
		      }
#endif

		    /* Attempt to handle failure.  */
		    if (count < 0)
		      {
			if (!(result == resultbuf || result == NULL))
			  free (result);
			freea (buf);
			CLEANUP ();
			errno = EINVAL;
			return NULL;
		      }

#if !HAVE_SNPRINTF
		    if (count >= tmp_length)
		      /* tmp_length was incorrectly calculated - fix the
			 code above!  */
		      abort ();
#endif

		    /* Make room for the result.  */
		    if (count >= maxlen)
		      {
			/* Need at least count bytes.  But allocate
			   proportionally, to avoid looping eternally if
			   snprintf() reports a too small count.  */
			size_t n = length + count;

			if (n < 2 * allocated)
			  n = 2 * allocated;

			ENSURE_ALLOCATION (n);
#if HAVE_SNPRINTF
			continue;
#endif
		      }

#if HAVE_SNPRINTF
		    /* The snprintf() result did fit.  */
#else
		    /* Append the sprintf() result.  */
		    memcpy (result + length, tmp, count);
		    if (tmp != tmpbuf)
		      free (tmp);
#endif

		    length += count;
		    break;
		  }
	      }
	  }
      }

    /* Add the final NUL.  */
    ENSURE_ALLOCATION (length + 1);
    result[length] = '\0';

    if (result != resultbuf && length + 1 < allocated)
      {
	/* Shrink the allocated memory if possible.  */
	char *memory;

	memory = (char *) realloc (result, length + 1);
	if (memory != NULL)
	  result = memory;
      }

    freea (buf);
    CLEANUP ();
    *lengthp = length;
    return result;
  }
}
