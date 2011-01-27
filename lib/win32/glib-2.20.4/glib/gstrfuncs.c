/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * MT safe
 */

#include "config.h"

#define _GNU_SOURCE		/* For stpcpy */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <ctype.h>		/* For tolower() */
#if !defined (HAVE_STRSIGNAL) || !defined(NO_SYS_SIGLIST_DECL)
#include <signal.h>
#endif

#include "glib.h"
#include "gprintf.h"
#include "gprintfint.h"
#include "glibintl.h"

#include "galias.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

/* do not include <unistd.h> in this place since it
 * interferes with g_strsignal() on some OSes
 */

static const guint16 ascii_table_data[256] = {
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x004, 0x104, 0x104, 0x004, 0x104, 0x104, 0x004, 0x004,
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x140, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459,
  0x459, 0x459, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x653, 0x653, 0x653, 0x653, 0x653, 0x653, 0x253,
  0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
  0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
  0x253, 0x253, 0x253, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x473, 0x473, 0x473, 0x473, 0x473, 0x473, 0x073,
  0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
  0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
  0x073, 0x073, 0x073, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x004
  /* the upper 128 are all zeroes */
};

const guint16 * const g_ascii_table = ascii_table_data;

/**
 * g_strdup:
 * @str: the string to duplicate
 *
 * Duplicates a string. If @str is %NULL it returns %NULL.
 * The returned string should be freed with g_free() 
 * when no longer needed.
 * 
 * Returns: a newly-allocated copy of @str
 */
gchar*
g_strdup (const gchar *str)
{
  gchar *new_str;
  gsize length;

  if (str)
    {
      length = strlen (str) + 1;
      new_str = g_new (char, length);
      memcpy (new_str, str, length);
    }
  else
    new_str = NULL;

  return new_str;
}

gpointer
g_memdup (gconstpointer mem,
	  guint         byte_size)
{
  gpointer new_mem;

  if (mem)
    {
      new_mem = g_malloc (byte_size);
      memcpy (new_mem, mem, byte_size);
    }
  else
    new_mem = NULL;

  return new_mem;
}

/**
 * g_strndup:
 * @str: the string to duplicate
 * @n: the maximum number of bytes to copy from @str
 *
 * Duplicates the first @n bytes of a string, returning a newly-allocated
 * buffer @n + 1 bytes long which will always be nul-terminated.
 * If @str is less than @n bytes long the buffer is padded with nuls.
 * If @str is %NULL it returns %NULL.
 * The returned value should be freed when no longer needed.
 * 
 * <note><para>
 * To copy a number of characters from a UTF-8 encoded string, use
 * g_utf8_strncpy() instead.
 * </para></note>
 * 
 * Returns: a newly-allocated buffer containing the first @n bytes 
 *          of @str, nul-terminated 
 */
gchar*
g_strndup (const gchar *str,
	   gsize        n)    
{
  gchar *new_str;

  if (str)
    {
      new_str = g_new (gchar, n + 1);
      strncpy (new_str, str, n);
      new_str[n] = '\0';
    }
  else
    new_str = NULL;

  return new_str;
}

/**
 * g_strnfill:
 * @length: the length of the new string
 * @fill_char: the byte to fill the string with
 *
 * Creates a new string @length bytes long filled with @fill_char.
 * The returned string should be freed when no longer needed.
 * 
 * Returns: a newly-allocated string filled the @fill_char
 */
gchar*
g_strnfill (gsize length,     
	    gchar fill_char)
{
  gchar *str;

  str = g_new (gchar, length + 1);
  memset (str, (guchar)fill_char, length);
  str[length] = '\0';

  return str;
}

/**
 * g_stpcpy:
 * @dest: destination buffer.
 * @src: source string.
 * 
 * Copies a nul-terminated string into the dest buffer, include the
 * trailing nul, and return a pointer to the trailing nul byte.
 * This is useful for concatenating multiple strings together
 * without having to repeatedly scan for the end.
 * 
 * Return value: a pointer to trailing nul byte.
 **/
gchar *
g_stpcpy (gchar       *dest,
          const gchar *src)
{
#ifdef HAVE_STPCPY
  g_return_val_if_fail (dest != NULL, NULL);
  g_return_val_if_fail (src != NULL, NULL);
  return stpcpy (dest, src);
#else
  register gchar *d = dest;
  register const gchar *s = src;

  g_return_val_if_fail (dest != NULL, NULL);
  g_return_val_if_fail (src != NULL, NULL);
  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
#endif
}

/**
 * g_strdup_vprintf:
 * @format: a standard printf() format string, but notice
 *     <link linkend="string-precision">string precision pitfalls</link>
 * @args: the list of parameters to insert into the format string
 * 
 * Similar to the standard C vsprintf() function but safer, since it 
 * calculates the maximum space required and allocates memory to hold 
 * the result. The returned string should be freed with g_free() when 
 * no longer needed.
 *
 * See also g_vasprintf(), which offers the same functionality, but 
 * additionally returns the length of the allocated string.
 *
 * Returns: a newly-allocated string holding the result
 */
gchar*
g_strdup_vprintf (const gchar *format,
		  va_list      args)
{
  gchar *string = NULL;

  g_vasprintf (&string, format, args);

  return string;
}

/**
 * g_strdup_printf:
 * @format: a standard printf() format string, but notice
 *     <link linkend="string-precision">string precision pitfalls</link>
 * @Varargs: the parameters to insert into the format string
 *
 * Similar to the standard C sprintf() function but safer, since it 
 * calculates the maximum space required and allocates memory to hold 
 * the result. The returned string should be freed with g_free() when no 
 * longer needed.
 * 
 * Returns: a newly-allocated string holding the result
 */
gchar*
g_strdup_printf (const gchar *format,
		 ...)
{
  gchar *buffer;
  va_list args;

  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);

  return buffer;
}

/**
 * g_strconcat:
 * @string1: the first string to add, which must not be %NULL
 * @Varargs: a %NULL-terminated list of strings to append to the string
 * 
 * Concatenates all of the given strings into one long string.
 * The returned string should be freed with g_free() when no longer needed.
 *
 *
 * <warning><para>The variable argument list <emphasis>must</emphasis> end 
 * with %NULL. If you forget the %NULL, g_strconcat() will start appending
 * random memory junk to your string.</para></warning>
 *
 * Returns: a newly-allocated string containing all the string arguments
 */
gchar*
g_strconcat (const gchar *string1, ...)
{
  gsize	  l;     
  va_list args;
  gchar	  *s;
  gchar	  *concat;
  gchar   *ptr;

  if (!string1)
    return NULL;

  l = 1 + strlen (string1);
  va_start (args, string1);
  s = va_arg (args, gchar*);
  while (s)
    {
      l += strlen (s);
      s = va_arg (args, gchar*);
    }
  va_end (args);

  concat = g_new (gchar, l);
  ptr = concat;

  ptr = g_stpcpy (ptr, string1);
  va_start (args, string1);
  s = va_arg (args, gchar*);
  while (s)
    {
      ptr = g_stpcpy (ptr, s);
      s = va_arg (args, gchar*);
    }
  va_end (args);

  return concat;
}

/**
 * g_strtod:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  if non-%NULL, it returns the character after
 *           the last character used in the conversion.
 * 
 * Converts a string to a #gdouble value.
 * It calls the standard strtod() function to handle the conversion, but
 * if the string is not completely converted it attempts the conversion
 * again with g_ascii_strtod(), and returns the best match.
 *
 * This function should seldomly be used. The normal situation when reading
 * numbers not for human consumption is to use g_ascii_strtod(). Only when
 * you know that you must expect both locale formatted and C formatted numbers
 * should you use this. Make sure that you don't pass strings such as comma
 * separated lists of values, since the commas may be interpreted as a decimal
 * point in some locales, causing unexpected results.
 * 
 * Return value: the #gdouble value.
 **/
gdouble
g_strtod (const gchar *nptr,
	  gchar      **endptr)
{
  gchar *fail_pos_1;
  gchar *fail_pos_2;
  gdouble val_1;
  gdouble val_2 = 0;

  g_return_val_if_fail (nptr != NULL, 0);

  fail_pos_1 = NULL;
  fail_pos_2 = NULL;

  val_1 = strtod (nptr, &fail_pos_1);

  if (fail_pos_1 && fail_pos_1[0] != 0)
    val_2 = g_ascii_strtod (nptr, &fail_pos_2);

  if (!fail_pos_1 || fail_pos_1[0] == 0 || fail_pos_1 >= fail_pos_2)
    {
      if (endptr)
	*endptr = fail_pos_1;
      return val_1;
    }
  else
    {
      if (endptr)
	*endptr = fail_pos_2;
      return val_2;
    }
}

/**
 * g_ascii_strtod:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  if non-%NULL, it returns the character after
 *           the last character used in the conversion.
 * 
 * Converts a string to a #gdouble value.
 *
 * This function behaves like the standard strtod() function
 * does in the C locale. It does this without actually changing 
 * the current locale, since that would not be thread-safe. 
 * A limitation of the implementation is that this function
 * will still accept localized versions of infinities and NANs. 
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtod() function.
 *
 * To convert from a #gdouble to a string in a locale-insensitive
 * way, use g_ascii_dtostr().
 *
 * If the correct value would cause overflow, plus or minus %HUGE_VAL
 * is returned (according to the sign of the value), and %ERANGE is
 * stored in %errno. If the correct value would cause underflow,
 * zero is returned and %ERANGE is stored in %errno.
 * 
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * Return value: the #gdouble value.
 **/
gdouble
g_ascii_strtod (const gchar *nptr,
		gchar      **endptr)
{
  gchar *fail_pos;
  gdouble val;
  struct lconv *locale_data;
  const char *decimal_point;
  int decimal_point_len;
  const char *p, *decimal_point_pos;
  const char *end = NULL; /* Silence gcc */
  int strtod_errno;

  g_return_val_if_fail (nptr != NULL, 0);

  fail_pos = NULL;

  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);

  g_assert (decimal_point_len != 0);
  
  decimal_point_pos = NULL;
  end = NULL;

  if (decimal_point[0] != '.' || 
      decimal_point[1] != 0)
    {
      p = nptr;
      /* Skip leading space */
      while (g_ascii_isspace (*p))
	p++;
      
      /* Skip leading optional sign */
      if (*p == '+' || *p == '-')
	p++;
      
      if (p[0] == '0' && 
	  (p[1] == 'x' || p[1] == 'X'))
	{
	  p += 2;
	  /* HEX - find the (optional) decimal point */
	  
	  while (g_ascii_isxdigit (*p))
	    p++;
	  
	  if (*p == '.')
	    decimal_point_pos = p++;
	      
	  while (g_ascii_isxdigit (*p))
	    p++;
	  
	  if (*p == 'p' || *p == 'P')
	    p++;
	  if (*p == '+' || *p == '-')
	    p++;
	  while (g_ascii_isdigit (*p))
	    p++;

	  end = p;
	}
      else if (g_ascii_isdigit (*p) || *p == '.')
	{
	  while (g_ascii_isdigit (*p))
	    p++;
	  
	  if (*p == '.')
	    decimal_point_pos = p++;
	  
	  while (g_ascii_isdigit (*p))
	    p++;
	  
	  if (*p == 'e' || *p == 'E')
	    p++;
	  if (*p == '+' || *p == '-')
	    p++;
	  while (g_ascii_isdigit (*p))
	    p++;

	  end = p;
	}
      /* For the other cases, we need not convert the decimal point */
    }

  if (decimal_point_pos)
    {
      char *copy, *c;

      /* We need to convert the '.' to the locale specific decimal point */
      copy = g_malloc (end - nptr + 1 + decimal_point_len);
      
      c = copy;
      memcpy (c, nptr, decimal_point_pos - nptr);
      c += decimal_point_pos - nptr;
      memcpy (c, decimal_point, decimal_point_len);
      c += decimal_point_len;
      memcpy (c, decimal_point_pos + 1, end - (decimal_point_pos + 1));
      c += end - (decimal_point_pos + 1);
      *c = 0;

      errno = 0;
      val = strtod (copy, &fail_pos);
      strtod_errno = errno;

      if (fail_pos)
	{
	  if (fail_pos - copy > decimal_point_pos - nptr)
	    fail_pos = (char *)nptr + (fail_pos - copy) - (decimal_point_len - 1);
	  else
	    fail_pos = (char *)nptr + (fail_pos - copy);
	}
      
      g_free (copy);
	  
    }
  else if (end)
    {
      char *copy;
      
      copy = g_malloc (end - (char *)nptr + 1);
      memcpy (copy, nptr, end - nptr);
      *(copy + (end - (char *)nptr)) = 0;
      
      errno = 0;
      val = strtod (copy, &fail_pos);
      strtod_errno = errno;

      if (fail_pos)
	{
	  fail_pos = (char *)nptr + (fail_pos - copy);
	}
      
      g_free (copy);
    }
  else
    {
      errno = 0;
      val = strtod (nptr, &fail_pos);
      strtod_errno = errno;
    }

  if (endptr)
    *endptr = fail_pos;

  errno = strtod_errno;

  return val;
}


/**
 * g_ascii_dtostr:
 * @buffer: A buffer to place the resulting string in
 * @buf_len: The length of the buffer.
 * @d: The #gdouble to convert
 *
 * Converts a #gdouble to a string, using the '.' as
 * decimal point. 
 * 
 * This functions generates enough precision that converting
 * the string back using g_ascii_strtod() gives the same machine-number
 * (on machines with IEEE compatible 64bit doubles). It is
 * guaranteed that the size of the resulting string will never
 * be larger than @G_ASCII_DTOSTR_BUF_SIZE bytes.
 *
 * Return value: The pointer to the buffer with the converted string.
 **/
gchar *
g_ascii_dtostr (gchar       *buffer,
		gint         buf_len,
		gdouble      d)
{
  return g_ascii_formatd (buffer, buf_len, "%.17g", d);
}

/**
 * g_ascii_formatd:
 * @buffer: A buffer to place the resulting string in
 * @buf_len: The length of the buffer.
 * @format: The printf()-style format to use for the
 *          code to use for converting. 
 * @d: The #gdouble to convert
 *
 * Converts a #gdouble to a string, using the '.' as
 * decimal point. To format the number you pass in
 * a printf()-style format string. Allowed conversion
 * specifiers are 'e', 'E', 'f', 'F', 'g' and 'G'. 
 * 
 * If you just want to want to serialize the value into a
 * string, use g_ascii_dtostr().
 *
 * Return value: The pointer to the buffer with the converted string.
 */
gchar *
g_ascii_formatd (gchar       *buffer,
		 gint         buf_len,
		 const gchar *format,
		 gdouble      d)
{
  struct lconv *locale_data;
  const char *decimal_point;
  int decimal_point_len;
  gchar *p;
  int rest_len;
  gchar format_char;

  g_return_val_if_fail (buffer != NULL, NULL);
  g_return_val_if_fail (format[0] == '%', NULL);
  g_return_val_if_fail (strpbrk (format + 1, "'l%") == NULL, NULL);
 
  format_char = format[strlen (format) - 1];
  
  g_return_val_if_fail (format_char == 'e' || format_char == 'E' ||
			format_char == 'f' || format_char == 'F' ||
			format_char == 'g' || format_char == 'G',
			NULL);

  if (format[0] != '%')
    return NULL;

  if (strpbrk (format + 1, "'l%"))
    return NULL;

  if (!(format_char == 'e' || format_char == 'E' ||
	format_char == 'f' || format_char == 'F' ||
	format_char == 'g' || format_char == 'G'))
    return NULL;

      
  _g_snprintf (buffer, buf_len, format, d);

  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);

  g_assert (decimal_point_len != 0);

  if (decimal_point[0] != '.' ||
      decimal_point[1] != 0)
    {
      p = buffer;

      while (g_ascii_isspace (*p))
	p++;

      if (*p == '+' || *p == '-')
	p++;

      while (isdigit ((guchar)*p))
	p++;

      if (strncmp (p, decimal_point, decimal_point_len) == 0)
	{
	  *p = '.';
	  p++;
	  if (decimal_point_len > 1) 
            {
	      rest_len = strlen (p + (decimal_point_len-1));
	      memmove (p, p + (decimal_point_len-1), rest_len);
	      p[rest_len] = 0;
	    }
	}
    }
  
  return buffer;
}

static guint64
g_parse_long_long (const gchar  *nptr,
		   const gchar **endptr,
		   guint         base,
		   gboolean     *negative)
{
  /* this code is based on on the strtol(3) code from GNU libc released under
   * the GNU Lesser General Public License.
   *
   * Copyright (C) 1991,92,94,95,96,97,98,99,2000,01,02
   *        Free Software Foundation, Inc.
   */
#define ISSPACE(c)		((c) == ' ' || (c) == '\f' || (c) == '\n' || \
				 (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISUPPER(c)		((c) >= 'A' && (c) <= 'Z')
#define ISLOWER(c)		((c) >= 'a' && (c) <= 'z')
#define ISALPHA(c)		(ISUPPER (c) || ISLOWER (c))
#define	TOUPPER(c)		(ISLOWER (c) ? (c) - 'a' + 'A' : (c))
#define	TOLOWER(c)		(ISUPPER (c) ? (c) - 'A' + 'a' : (c))
  gboolean overflow;
  guint64 cutoff;
  guint64 cutlim;
  guint64 ui64;
  const gchar *s, *save;
  guchar c;
  
  g_return_val_if_fail (nptr != NULL, 0);
  
  *negative = FALSE;
  if (base == 1 || base > 36)
    {
      errno = EINVAL;
      if (endptr)
	*endptr = nptr;
      return 0;
    }
  
  save = s = nptr;
  
  /* Skip white space.  */
  while (ISSPACE (*s))
    ++s;

  if (G_UNLIKELY (!*s))
    goto noconv;
  
  /* Check for a sign.  */
  if (*s == '-')
    {
      *negative = TRUE;
      ++s;
    }
  else if (*s == '+')
    ++s;
  
  /* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
  if (*s == '0')
    {
      if ((base == 0 || base == 16) && TOUPPER (s[1]) == 'X')
	{
	  s += 2;
	  base = 16;
	}
      else if (base == 0)
	base = 8;
    }
  else if (base == 0)
    base = 10;
  
  /* Save the pointer so we can check later if anything happened.  */
  save = s;
  cutoff = G_MAXUINT64 / base;
  cutlim = G_MAXUINT64 % base;
  
  overflow = FALSE;
  ui64 = 0;
  c = *s;
  for (; c; c = *++s)
    {
      if (c >= '0' && c <= '9')
	c -= '0';
      else if (ISALPHA (c))
	c = TOUPPER (c) - 'A' + 10;
      else
	break;
      if (c >= base)
	break;
      /* Check for overflow.  */
      if (ui64 > cutoff || (ui64 == cutoff && c > cutlim))
	overflow = TRUE;
      else
	{
	  ui64 *= base;
	  ui64 += c;
	}
    }
  
  /* Check if anything actually happened.  */
  if (s == save)
    goto noconv;
  
  /* Store in ENDPTR the address of one character
     past the last character we converted.  */
  if (endptr)
    *endptr = s;
  
  if (G_UNLIKELY (overflow))
    {
      errno = ERANGE;
      return G_MAXUINT64;
    }

  return ui64;
  
 noconv:
  /* We must handle a special case here: the base is 0 or 16 and the
     first two characters are '0' and 'x', but the rest are no
     hexadecimal digits.  This is no error case.  We return 0 and
     ENDPTR points to the `x`.  */
  if (endptr)
    {
      if (save - nptr >= 2 && TOUPPER (save[-1]) == 'X'
	  && save[-2] == '0')
	*endptr = &save[-1];
      else
	/*  There was no number to convert.  */
	*endptr = nptr;
    }
  return 0;
}

/**
 * g_ascii_strtoull:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  if non-%NULL, it returns the character after
 *           the last character used in the conversion.
 * @base:    to be used for the conversion, 2..36 or 0
 *
 * Converts a string to a #guint64 value.
 * This function behaves like the standard strtoull() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtoull() function.
 *
 * If the correct value would cause overflow, %G_MAXUINT64
 * is returned, and %ERANGE is stored in %errno.  If the base is
 * outside the valid range, zero is returned, and %EINVAL is stored
 * in %errno.  If the string conversion fails, zero is returned, and
 * @endptr returns @nptr (if @endptr is non-%NULL).
 *
 * Return value: the #guint64 value or zero on error.
 *
 * Since: 2.2
 */
guint64
g_ascii_strtoull (const gchar *nptr,
		  gchar      **endptr,
		  guint        base)
{
  gboolean negative;
  guint64 result;

  result = g_parse_long_long (nptr, (const gchar **) endptr, base, &negative);

  /* Return the result of the appropriate sign.  */
  return negative ? -result : result;
}

/**
 * g_ascii_strtoll:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  if non-%NULL, it returns the character after
 *           the last character used in the conversion.
 * @base:    to be used for the conversion, 2..36 or 0
 *
 * Converts a string to a #gint64 value.
 * This function behaves like the standard strtoll() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtoll() function.
 *
 * If the correct value would cause overflow, %G_MAXINT64 or %G_MININT64
 * is returned, and %ERANGE is stored in %errno.  If the base is
 * outside the valid range, zero is returned, and %EINVAL is stored
 * in %errno.  If the string conversion fails, zero is returned, and
 * @endptr returns @nptr (if @endptr is non-%NULL).
 *
 * Return value: the #gint64 value or zero on error.
 *
 * Since: 2.12
 */
gint64 
g_ascii_strtoll (const gchar *nptr,
		 gchar      **endptr,
		 guint        base)
{
  gboolean negative;
  guint64 result;

  result = g_parse_long_long (nptr, (const gchar **) endptr, base, &negative);

  if (negative && result > (guint64) G_MININT64)
    {
      errno = ERANGE;
      return G_MININT64;
    }
  else if (!negative && result > (guint64) G_MAXINT64)
    {
      errno = ERANGE;
      return G_MAXINT64;
    }
  else if (negative)
    return - (gint64) result;
  else
    return (gint64) result;
}

/**
 * g_strerror:
 * @errnum: the system error number. See the standard C %errno
 *     documentation
 * 
 * Returns a string corresponding to the given error code, e.g. 
 * "no such process". You should use this function in preference to 
 * strerror(), because it returns a string in UTF-8 encoding, and since 
 * not all platforms support the strerror() function.
 *
 * Returns: a UTF-8 string describing the error code. If the error code 
 *     is unknown, it returns "unknown error (&lt;code&gt;)". The string 
 *     can only be used until the next call to g_strerror()
 */
G_CONST_RETURN gchar*
g_strerror (gint errnum)
{
  static GStaticPrivate msg_private = G_STATIC_PRIVATE_INIT;
  char *msg;
  int saved_errno = errno;

#ifdef HAVE_STRERROR
  const char *msg_locale;

  msg_locale = strerror (errnum);
  if (g_get_charset (NULL))
    {
      errno = saved_errno;
      return msg_locale;
    }
  else
    {
      gchar *msg_utf8 = g_locale_to_utf8 (msg_locale, -1, NULL, NULL, NULL);
      if (msg_utf8)
	{
	  /* Stick in the quark table so that we can return a static result
	   */
	  GQuark msg_quark = g_quark_from_string (msg_utf8);
	  g_free (msg_utf8);
	  
	  msg_utf8 = (gchar *) g_quark_to_string (msg_quark);
	  errno = saved_errno;
	  return msg_utf8;
	}
    }
#elif NO_SYS_ERRLIST
  switch (errnum)
    {
#ifdef E2BIG
    case E2BIG: return "argument list too long";
#endif
#ifdef EACCES
    case EACCES: return "permission denied";
#endif
#ifdef EADDRINUSE
    case EADDRINUSE: return "address already in use";
#endif
#ifdef EADDRNOTAVAIL
    case EADDRNOTAVAIL: return "can't assign requested address";
#endif
#ifdef EADV
    case EADV: return "advertise error";
#endif
#ifdef EAFNOSUPPORT
    case EAFNOSUPPORT: return "address family not supported by protocol family";
#endif
#ifdef EAGAIN
    case EAGAIN: return "try again";
#endif
#ifdef EALIGN
    case EALIGN: return "EALIGN";
#endif
#ifdef EALREADY
    case EALREADY: return "operation already in progress";
#endif
#ifdef EBADE
    case EBADE: return "bad exchange descriptor";
#endif
#ifdef EBADF
    case EBADF: return "bad file number";
#endif
#ifdef EBADFD
    case EBADFD: return "file descriptor in bad state";
#endif
#ifdef EBADMSG
    case EBADMSG: return "not a data message";
#endif
#ifdef EBADR
    case EBADR: return "bad request descriptor";
#endif
#ifdef EBADRPC
    case EBADRPC: return "RPC structure is bad";
#endif
#ifdef EBADRQC
    case EBADRQC: return "bad request code";
#endif
#ifdef EBADSLT
    case EBADSLT: return "invalid slot";
#endif
#ifdef EBFONT
    case EBFONT: return "bad font file format";
#endif
#ifdef EBUSY
    case EBUSY: return "mount device busy";
#endif
#ifdef ECHILD
    case ECHILD: return "no children";
#endif
#ifdef ECHRNG
    case ECHRNG: return "channel number out of range";
#endif
#ifdef ECOMM
    case ECOMM: return "communication error on send";
#endif
#ifdef ECONNABORTED
    case ECONNABORTED: return "software caused connection abort";
#endif
#ifdef ECONNREFUSED
    case ECONNREFUSED: return "connection refused";
#endif
#ifdef ECONNRESET
    case ECONNRESET: return "connection reset by peer";
#endif
#if defined(EDEADLK) && (!defined(EWOULDBLOCK) || (EDEADLK != EWOULDBLOCK))
    case EDEADLK: return "resource deadlock avoided";
#endif
#ifdef EDEADLOCK
    case EDEADLOCK: return "resource deadlock avoided";
#endif
#ifdef EDESTADDRREQ
    case EDESTADDRREQ: return "destination address required";
#endif
#ifdef EDIRTY
    case EDIRTY: return "mounting a dirty fs w/o force";
#endif
#ifdef EDOM
    case EDOM: return "math argument out of range";
#endif
#ifdef EDOTDOT
    case EDOTDOT: return "cross mount point";
#endif
#ifdef EDQUOT
    case EDQUOT: return "disk quota exceeded";
#endif
#ifdef EDUPPKG
    case EDUPPKG: return "duplicate package name";
#endif
#ifdef EEXIST
    case EEXIST: return "file already exists";
#endif
#ifdef EFAULT
    case EFAULT: return "bad address in system call argument";
#endif
#ifdef EFBIG
    case EFBIG: return "file too large";
#endif
#ifdef EHOSTDOWN
    case EHOSTDOWN: return "host is down";
#endif
#ifdef EHOSTUNREACH
    case EHOSTUNREACH: return "host is unreachable";
#endif
#ifdef EIDRM
    case EIDRM: return "identifier removed";
#endif
#ifdef EINIT
    case EINIT: return "initialization error";
#endif
#ifdef EINPROGRESS
    case EINPROGRESS: return "operation now in progress";
#endif
#ifdef EINTR
    case EINTR: return "interrupted system call";
#endif
#ifdef EINVAL
    case EINVAL: return "invalid argument";
#endif
#ifdef EIO
    case EIO: return "I/O error";
#endif
#ifdef EISCONN
    case EISCONN: return "socket is already connected";
#endif
#ifdef EISDIR
    case EISDIR: return "is a directory";
#endif
#ifdef EISNAME
    case EISNAM: return "is a name file";
#endif
#ifdef ELBIN
    case ELBIN: return "ELBIN";
#endif
#ifdef EL2HLT
    case EL2HLT: return "level 2 halted";
#endif
#ifdef EL2NSYNC
    case EL2NSYNC: return "level 2 not synchronized";
#endif
#ifdef EL3HLT
    case EL3HLT: return "level 3 halted";
#endif
#ifdef EL3RST
    case EL3RST: return "level 3 reset";
#endif
#ifdef ELIBACC
    case ELIBACC: return "can not access a needed shared library";
#endif
#ifdef ELIBBAD
    case ELIBBAD: return "accessing a corrupted shared library";
#endif
#ifdef ELIBEXEC
    case ELIBEXEC: return "can not exec a shared library directly";
#endif
#ifdef ELIBMAX
    case ELIBMAX: return "attempting to link in more shared libraries than system limit";
#endif
#ifdef ELIBSCN
    case ELIBSCN: return ".lib section in a.out corrupted";
#endif
#ifdef ELNRNG
    case ELNRNG: return "link number out of range";
#endif
#ifdef ELOOP
    case ELOOP: return "too many levels of symbolic links";
#endif
#ifdef EMFILE
    case EMFILE: return "too many open files";
#endif
#ifdef EMLINK
    case EMLINK: return "too many links";
#endif
#ifdef EMSGSIZE
    case EMSGSIZE: return "message too long";
#endif
#ifdef EMULTIHOP
    case EMULTIHOP: return "multihop attempted";
#endif
#ifdef ENAMETOOLONG
    case ENAMETOOLONG: return "file name too long";
#endif
#ifdef ENAVAIL
    case ENAVAIL: return "not available";
#endif
#ifdef ENET
    case ENET: return "ENET";
#endif
#ifdef ENETDOWN
    case ENETDOWN: return "network is down";
#endif
#ifdef ENETRESET
    case ENETRESET: return "network dropped connection on reset";
#endif
#ifdef ENETUNREACH
    case ENETUNREACH: return "network is unreachable";
#endif
#ifdef ENFILE
    case ENFILE: return "file table overflow";
#endif
#ifdef ENOANO
    case ENOANO: return "anode table overflow";
#endif
#if defined(ENOBUFS) && (!defined(ENOSR) || (ENOBUFS != ENOSR))
    case ENOBUFS: return "no buffer space available";
#endif
#ifdef ENOCSI
    case ENOCSI: return "no CSI structure available";
#endif
#ifdef ENODATA
    case ENODATA: return "no data available";
#endif
#ifdef ENODEV
    case ENODEV: return "no such device";
#endif
#ifdef ENOENT
    case ENOENT: return "no such file or directory";
#endif
#ifdef ENOEXEC
    case ENOEXEC: return "exec format error";
#endif
#ifdef ENOLCK
    case ENOLCK: return "no locks available";
#endif
#ifdef ENOLINK
    case ENOLINK: return "link has be severed";
#endif
#ifdef ENOMEM
    case ENOMEM: return "not enough memory";
#endif
#ifdef ENOMSG
    case ENOMSG: return "no message of desired type";
#endif
#ifdef ENONET
    case ENONET: return "machine is not on the network";
#endif
#ifdef ENOPKG
    case ENOPKG: return "package not installed";
#endif
#ifdef ENOPROTOOPT
    case ENOPROTOOPT: return "bad proocol option";
#endif
#ifdef ENOSPC
    case ENOSPC: return "no space left on device";
#endif
#ifdef ENOSR
    case ENOSR: return "out of stream resources";
#endif
#ifdef ENOSTR
    case ENOSTR: return "not a stream device";
#endif
#ifdef ENOSYM
    case ENOSYM: return "unresolved symbol name";
#endif
#ifdef ENOSYS
    case ENOSYS: return "function not implemented";
#endif
#ifdef ENOTBLK
    case ENOTBLK: return "block device required";
#endif
#ifdef ENOTCONN
    case ENOTCONN: return "socket is not connected";
#endif
#ifdef ENOTDIR
    case ENOTDIR: return "not a directory";
#endif
#ifdef ENOTEMPTY
    case ENOTEMPTY: return "directory not empty";
#endif
#ifdef ENOTNAM
    case ENOTNAM: return "not a name file";
#endif
#ifdef ENOTSOCK
    case ENOTSOCK: return "socket operation on non-socket";
#endif
#ifdef ENOTTY
    case ENOTTY: return "inappropriate device for ioctl";
#endif
#ifdef ENOTUNIQ
    case ENOTUNIQ: return "name not unique on network";
#endif
#ifdef ENXIO
    case ENXIO: return "no such device or address";
#endif
#ifdef EOPNOTSUPP
    case EOPNOTSUPP: return "operation not supported on socket";
#endif
#ifdef EPERM
    case EPERM: return "not owner";
#endif
#ifdef EPFNOSUPPORT
    case EPFNOSUPPORT: return "protocol family not supported";
#endif
#ifdef EPIPE
    case EPIPE: return "broken pipe";
#endif
#ifdef EPROCLIM
    case EPROCLIM: return "too many processes";
#endif
#ifdef EPROCUNAVAIL
    case EPROCUNAVAIL: return "bad procedure for program";
#endif
#ifdef EPROGMISMATCH
    case EPROGMISMATCH: return "program version wrong";
#endif
#ifdef EPROGUNAVAIL
    case EPROGUNAVAIL: return "RPC program not available";
#endif
#ifdef EPROTO
    case EPROTO: return "protocol error";
#endif
#ifdef EPROTONOSUPPORT
    case EPROTONOSUPPORT: return "protocol not suppored";
#endif
#ifdef EPROTOTYPE
    case EPROTOTYPE: return "protocol wrong type for socket";
#endif
#ifdef ERANGE
    case ERANGE: return "math result unrepresentable";
#endif
#if defined(EREFUSED) && (!defined(ECONNREFUSED) || (EREFUSED != ECONNREFUSED))
    case EREFUSED: return "EREFUSED";
#endif
#ifdef EREMCHG
    case EREMCHG: return "remote address changed";
#endif
#ifdef EREMDEV
    case EREMDEV: return "remote device";
#endif
#ifdef EREMOTE
    case EREMOTE: return "pathname hit remote file system";
#endif
#ifdef EREMOTEIO
    case EREMOTEIO: return "remote i/o error";
#endif
#ifdef EREMOTERELEASE
    case EREMOTERELEASE: return "EREMOTERELEASE";
#endif
#ifdef EROFS
    case EROFS: return "read-only file system";
#endif
#ifdef ERPCMISMATCH
    case ERPCMISMATCH: return "RPC version is wrong";
#endif
#ifdef ERREMOTE
    case ERREMOTE: return "object is remote";
#endif
#ifdef ESHUTDOWN
    case ESHUTDOWN: return "can't send afer socket shutdown";
#endif
#ifdef ESOCKTNOSUPPORT
    case ESOCKTNOSUPPORT: return "socket type not supported";
#endif
#ifdef ESPIPE
    case ESPIPE: return "invalid seek";
#endif
#ifdef ESRCH
    case ESRCH: return "no such process";
#endif
#ifdef ESRMNT
    case ESRMNT: return "srmount error";
#endif
#ifdef ESTALE
    case ESTALE: return "stale remote file handle";
#endif
#ifdef ESUCCESS
    case ESUCCESS: return "Error 0";
#endif
#ifdef ETIME
    case ETIME: return "timer expired";
#endif
#ifdef ETIMEDOUT
    case ETIMEDOUT: return "connection timed out";
#endif
#ifdef ETOOMANYREFS
    case ETOOMANYREFS: return "too many references: can't splice";
#endif
#ifdef ETXTBSY
    case ETXTBSY: return "text file or pseudo-device busy";
#endif
#ifdef EUCLEAN
    case EUCLEAN: return "structure needs cleaning";
#endif
#ifdef EUNATCH
    case EUNATCH: return "protocol driver not attached";
#endif
#ifdef EUSERS
    case EUSERS: return "too many users";
#endif
#ifdef EVERSION
    case EVERSION: return "version mismatch";
#endif
#if defined(EWOULDBLOCK) && (!defined(EAGAIN) || (EWOULDBLOCK != EAGAIN))
    case EWOULDBLOCK: return "operation would block";
#endif
#ifdef EXDEV
    case EXDEV: return "cross-domain link";
#endif
#ifdef EXFULL
    case EXFULL: return "message tables full";
#endif
    }
#else /* NO_SYS_ERRLIST */
  extern int sys_nerr;
  extern char *sys_errlist[];

  if ((errnum > 0) && (errnum <= sys_nerr))
    return sys_errlist [errnum];
#endif /* NO_SYS_ERRLIST */

  msg = g_static_private_get (&msg_private);
  if (!msg)
    {
      msg = g_new (gchar, 64);
      g_static_private_set (&msg_private, msg, g_free);
    }

  _g_sprintf (msg, "unknown error (%d)", errnum);

  errno = saved_errno;
  return msg;
}

/**
 * g_strsignal:
 * @signum: the signal number. See the <literal>signal</literal>
 *     documentation
 *
 * Returns a string describing the given signal, e.g. "Segmentation fault".
 * You should use this function in preference to strsignal(), because it 
 * returns a string in UTF-8 encoding, and since not all platforms support
 * the strsignal() function.
 *
 * Returns: a UTF-8 string describing the signal. If the signal is unknown,
 *     it returns "unknown signal (&lt;signum&gt;)". The string can only be 
 *     used until the next call to g_strsignal()
 */ 
G_CONST_RETURN gchar*
g_strsignal (gint signum)
{
  static GStaticPrivate msg_private = G_STATIC_PRIVATE_INIT;
  char *msg;

#ifdef HAVE_STRSIGNAL
  const char *msg_locale;
  
#if defined(G_OS_BEOS) || defined(G_WITH_CYGWIN)
extern const char *strsignal(int);
#else
  /* this is declared differently (const) in string.h on BeOS */
  extern char *strsignal (int sig);
#endif /* !G_OS_BEOS && !G_WITH_CYGWIN */
  msg_locale = strsignal (signum);
  if (g_get_charset (NULL))
    return msg_locale;
  else
    {
      gchar *msg_utf8 = g_locale_to_utf8 (msg_locale, -1, NULL, NULL, NULL);
      if (msg_utf8)
	{
	  /* Stick in the quark table so that we can return a static result
	   */
	  GQuark msg_quark = g_quark_from_string (msg_utf8);
	  g_free (msg_utf8);
	  
	  return g_quark_to_string (msg_quark);
	}
    }
#elif NO_SYS_SIGLIST
  switch (signum)
    {
#ifdef SIGHUP
    case SIGHUP: return "Hangup";
#endif
#ifdef SIGINT
    case SIGINT: return "Interrupt";
#endif
#ifdef SIGQUIT
    case SIGQUIT: return "Quit";
#endif
#ifdef SIGILL
    case SIGILL: return "Illegal instruction";
#endif
#ifdef SIGTRAP
    case SIGTRAP: return "Trace/breakpoint trap";
#endif
#ifdef SIGABRT
    case SIGABRT: return "IOT trap/Abort";
#endif
#ifdef SIGBUS
    case SIGBUS: return "Bus error";
#endif
#ifdef SIGFPE
    case SIGFPE: return "Floating point exception";
#endif
#ifdef SIGKILL
    case SIGKILL: return "Killed";
#endif
#ifdef SIGUSR1
    case SIGUSR1: return "User defined signal 1";
#endif
#ifdef SIGSEGV
    case SIGSEGV: return "Segmentation fault";
#endif
#ifdef SIGUSR2
    case SIGUSR2: return "User defined signal 2";
#endif
#ifdef SIGPIPE
    case SIGPIPE: return "Broken pipe";
#endif
#ifdef SIGALRM
    case SIGALRM: return "Alarm clock";
#endif
#ifdef SIGTERM
    case SIGTERM: return "Terminated";
#endif
#ifdef SIGSTKFLT
    case SIGSTKFLT: return "Stack fault";
#endif
#ifdef SIGCHLD
    case SIGCHLD: return "Child exited";
#endif
#ifdef SIGCONT
    case SIGCONT: return "Continued";
#endif
#ifdef SIGSTOP
    case SIGSTOP: return "Stopped (signal)";
#endif
#ifdef SIGTSTP
    case SIGTSTP: return "Stopped";
#endif
#ifdef SIGTTIN
    case SIGTTIN: return "Stopped (tty input)";
#endif
#ifdef SIGTTOU
    case SIGTTOU: return "Stopped (tty output)";
#endif
#ifdef SIGURG
    case SIGURG: return "Urgent condition";
#endif
#ifdef SIGXCPU
    case SIGXCPU: return "CPU time limit exceeded";
#endif
#ifdef SIGXFSZ
    case SIGXFSZ: return "File size limit exceeded";
#endif
#ifdef SIGVTALRM
    case SIGVTALRM: return "Virtual time alarm";
#endif
#ifdef SIGPROF
    case SIGPROF: return "Profile signal";
#endif
#ifdef SIGWINCH
    case SIGWINCH: return "Window size changed";
#endif
#ifdef SIGIO
    case SIGIO: return "Possible I/O";
#endif
#ifdef SIGPWR
    case SIGPWR: return "Power failure";
#endif
#ifdef SIGUNUSED
    case SIGUNUSED: return "Unused signal";
#endif
    }
#else /* NO_SYS_SIGLIST */

#ifdef NO_SYS_SIGLIST_DECL
  extern char *sys_siglist[];	/*(see Tue Jan 19 00:44:24 1999 in changelog)*/
#endif

  return (char*) /* this function should return const --josh */ sys_siglist [signum];
#endif /* NO_SYS_SIGLIST */

  msg = g_static_private_get (&msg_private);
  if (!msg)
    {
      msg = g_new (gchar, 64);
      g_static_private_set (&msg_private, msg, g_free);
    }

  _g_sprintf (msg, "unknown signal (%d)", signum);
  
  return msg;
}

/* Functions g_strlcpy and g_strlcat were originally developed by
 * Todd C. Miller <Todd.Miller@courtesan.com> to simplify writing secure code.
 * See ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/strlcpy.3
 * for more information.
 */

#ifdef HAVE_STRLCPY
/* Use the native ones, if available; they might be implemented in assembly */
gsize
g_strlcpy (gchar       *dest,
	   const gchar *src,
	   gsize        dest_size)
{
  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);
  
  return strlcpy (dest, src, dest_size);
}

gsize
g_strlcat (gchar       *dest,
	   const gchar *src,
	   gsize        dest_size)
{
  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);
  
  return strlcat (dest, src, dest_size);
}

#else /* ! HAVE_STRLCPY */
/**
 * g_strlcpy:
 * @dest: destination buffer
 * @src: source buffer
 * @dest_size: length of @dest in bytes
 * 
 * Portability wrapper that calls strlcpy() on systems which have it, 
 * and emulates strlcpy() otherwise. Copies @src to @dest; @dest is 
 * guaranteed to be nul-terminated; @src must be nul-terminated; 
 * @dest_size is the buffer size, not the number of chars to copy. 
 *
 * At most dest_size - 1 characters will be copied. Always nul-terminates
 * (unless dest_size == 0). This function does <emphasis>not</emphasis> 
 * allocate memory. Unlike strncpy(), this function doesn't pad dest (so 
 * it's often faster). It returns the size of the attempted result, 
 * strlen (src), so if @retval >= @dest_size, truncation occurred.
 * 
 * <note><para>Caveat: strlcpy() is supposedly more secure than
 * strcpy() or strncpy(), but if you really want to avoid screwups, 
 * g_strdup() is an even better idea.</para></note>
 *
 * Returns: length of @src
 */
gsize
g_strlcpy (gchar       *dest,
           const gchar *src,
           gsize        dest_size)
{
  register gchar *d = dest;
  register const gchar *s = src;
  register gsize n = dest_size;
  
  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);
  
  /* Copy as many bytes as will fit */
  if (n != 0 && --n != 0)
    do
      {
	register gchar c = *s++;
	
	*d++ = c;
	if (c == 0)
	  break;
      }
    while (--n != 0);
  
  /* If not enough room in dest, add NUL and traverse rest of src */
  if (n == 0)
    {
      if (dest_size != 0)
	*d = 0;
      while (*s++)
	;
    }
  
  return s - src - 1;  /* count does not include NUL */
}

/**
 * g_strlcat:
 * @dest: destination buffer, already containing one nul-terminated string
 * @src: source buffer
 * @dest_size: length of @dest buffer in bytes (not length of existing string 
 *     inside @dest)
 *
 * Portability wrapper that calls strlcat() on systems which have it, 
 * and emulates it otherwise. Appends nul-terminated @src string to @dest, 
 * guaranteeing nul-termination for @dest. The total size of @dest won't 
 * exceed @dest_size. 
 *
 * At most dest_size - 1 characters will be copied.
 * Unlike strncat, dest_size is the full size of dest, not the space left over.
 * This function does NOT allocate memory.
 * This always NUL terminates (unless siz == 0 or there were no NUL characters
 * in the dest_size characters of dest to start with).
 * Returns size of attempted result, which is
 * MIN (dest_size, strlen (original dest)) + strlen (src),
 * so if retval >= dest_size, truncation occurred.
 *
 * <note><para>Caveat: this is supposedly a more secure alternative to 
 * strcat() or strncat(), but for real security g_strconcat() is harder 
 * to mess up.</para></note>
 *
 */
gsize
g_strlcat (gchar       *dest,
           const gchar *src,
           gsize        dest_size)
{
  register gchar *d = dest;
  register const gchar *s = src;
  register gsize bytes_left = dest_size;
  gsize dlength;  /* Logically, MIN (strlen (d), dest_size) */
  
  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (src  != NULL, 0);
  
  /* Find the end of dst and adjust bytes left but don't go past end */
  while (*d != 0 && bytes_left-- != 0)
    d++;
  dlength = d - dest;
  bytes_left = dest_size - dlength;
  
  if (bytes_left == 0)
    return dlength + strlen (s);
  
  while (*s != 0)
    {
      if (bytes_left != 1)
	{
	  *d++ = *s;
	  bytes_left--;
	}
      s++;
    }
  *d = 0;
  
  return dlength + (s - src);  /* count does not include NUL */
}
#endif /* ! HAVE_STRLCPY */

/**
 * g_ascii_strdown:
 * @str: a string.
 * @len: length of @str in bytes, or -1 if @str is nul-terminated.
 * 
 * Converts all upper case ASCII letters to lower case ASCII letters.
 * 
 * Return value: a newly-allocated string, with all the upper case
 *               characters in @str converted to lower case, with
 *               semantics that exactly match g_ascii_tolower(). (Note
 *               that this is unlike the old g_strdown(), which modified
 *               the string in place.)
 **/
gchar*
g_ascii_strdown (const gchar *str,
		 gssize       len)
{
  gchar *result, *s;
  
  g_return_val_if_fail (str != NULL, NULL);

  if (len < 0)
    len = strlen (str);

  result = g_strndup (str, len);
  for (s = result; *s; s++)
    *s = g_ascii_tolower (*s);
  
  return result;
}

/**
 * g_ascii_strup:
 * @str: a string.
 * @len: length of @str in bytes, or -1 if @str is nul-terminated.
 * 
 * Converts all lower case ASCII letters to upper case ASCII letters.
 * 
 * Return value: a newly allocated string, with all the lower case
 *               characters in @str converted to upper case, with
 *               semantics that exactly match g_ascii_toupper(). (Note
 *               that this is unlike the old g_strup(), which modified
 *               the string in place.)
 **/
gchar*
g_ascii_strup (const gchar *str,
	       gssize       len)
{
  gchar *result, *s;

  g_return_val_if_fail (str != NULL, NULL);

  if (len < 0)
    len = strlen (str);

  result = g_strndup (str, len);
  for (s = result; *s; s++)
    *s = g_ascii_toupper (*s);

  return result;
}

/**
 * g_strdown:
 * @string: the string to convert.
 * 
 * Converts a string to lower case.  
 * 
 * Return value: the string 
 *
 * Deprecated:2.2: This function is totally broken for the reasons discussed 
 * in the g_strncasecmp() docs - use g_ascii_strdown() or g_utf8_strdown() 
 * instead.
 **/
gchar*
g_strdown (gchar *string)
{
  register guchar *s;
  
  g_return_val_if_fail (string != NULL, NULL);
  
  s = (guchar *) string;
  
  while (*s)
    {
      if (isupper (*s))
	*s = tolower (*s);
      s++;
    }
  
  return (gchar *) string;
}

/**
 * g_strup:
 * @string: the string to convert.
 * 
 * Converts a string to upper case. 
 * 
 * Return value: the string
 *
 * Deprecated:2.2: This function is totally broken for the reasons discussed 
 * in the g_strncasecmp() docs - use g_ascii_strup() or g_utf8_strup() instead.
 **/
gchar*
g_strup (gchar *string)
{
  register guchar *s;

  g_return_val_if_fail (string != NULL, NULL);

  s = (guchar *) string;

  while (*s)
    {
      if (islower (*s))
	*s = toupper (*s);
      s++;
    }

  return (gchar *) string;
}

/**
 * g_strreverse:
 * @string: the string to reverse 
 *
 * Reverses all of the bytes in a string. For example, 
 * <literal>g_strreverse ("abcdef")</literal> will result 
 * in "fedcba".
 * 
 * Note that g_strreverse() doesn't work on UTF-8 strings 
 * containing multibyte characters. For that purpose, use 
 * g_utf8_strreverse().
 * 
 * Returns: the same pointer passed in as @string
 */
gchar*
g_strreverse (gchar *string)
{
  g_return_val_if_fail (string != NULL, NULL);

  if (*string)
    {
      register gchar *h, *t;

      h = string;
      t = string + strlen (string) - 1;

      while (h < t)
	{
	  register gchar c;

	  c = *h;
	  *h = *t;
	  h++;
	  *t = c;
	  t--;
	}
    }

  return string;
}

/**
 * g_ascii_tolower:
 * @c: any character.
 * 
 * Convert a character to ASCII lower case.
 *
 * Unlike the standard C library tolower() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are lower case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #guchar
 * before passing a possibly non-ASCII character in.
 * 
 * Return value: the result of converting @c to lower case.
 *               If @c is not an ASCII upper case letter,
 *               @c is returned unchanged.
 **/
gchar
g_ascii_tolower (gchar c)
{
  return g_ascii_isupper (c) ? c - 'A' + 'a' : c;
}

/**
 * g_ascii_toupper:
 * @c: any character.
 * 
 * Convert a character to ASCII upper case.
 *
 * Unlike the standard C library toupper() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are upper case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #guchar
 * before passing a possibly non-ASCII character in.
 * 
 * Return value: the result of converting @c to upper case.
 *               If @c is not an ASCII lower case letter,
 *               @c is returned unchanged.
 **/
gchar
g_ascii_toupper (gchar c)
{
  return g_ascii_islower (c) ? c - 'a' + 'A' : c;
}

/**
 * g_ascii_digit_value:
 * @c: an ASCII character.
 *
 * Determines the numeric value of a character as a decimal
 * digit. Differs from g_unichar_digit_value() because it takes
 * a char, so there's no worry about sign extension if characters
 * are signed.
 *
 * Return value: If @c is a decimal digit (according to
 * g_ascii_isdigit()), its numeric value. Otherwise, -1.
 **/
int
g_ascii_digit_value (gchar c)
{
  if (g_ascii_isdigit (c))
    return c - '0';
  return -1;
}

/**
 * g_ascii_xdigit_value:
 * @c: an ASCII character.
 *
 * Determines the numeric value of a character as a hexidecimal
 * digit. Differs from g_unichar_xdigit_value() because it takes
 * a char, so there's no worry about sign extension if characters
 * are signed.
 *
 * Return value: If @c is a hex digit (according to
 * g_ascii_isxdigit()), its numeric value. Otherwise, -1.
 **/
int
g_ascii_xdigit_value (gchar c)
{
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return g_ascii_digit_value (c);
}

/**
 * g_ascii_strcasecmp:
 * @s1: string to compare with @s2.
 * @s2: string to compare with @s1.
 * 
 * Compare two strings, ignoring the case of ASCII characters.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * bytes as if they are not letters.
 *
 * This function should be used only on strings that are known to be
 * in encodings where the bytes corresponding to ASCII letters always
 * represent themselves. This includes UTF-8 and the ISO-8859-*
 * charsets, but not for instance double-byte encodings like the
 * Windows Codepage 932, where the trailing bytes of double-byte
 * characters include all ASCII letters. If you compare two CP932
 * strings using this function, you will get false matches.
 *
 * Return value: 0 if the strings match, a negative value if @s1 &lt; @s2, 
 *   or a positive value if @s1 &gt; @s2.
 **/
gint
g_ascii_strcasecmp (const gchar *s1,
		    const gchar *s2)
{
  gint c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (*s1 && *s2)
    {
      c1 = (gint)(guchar) TOLOWER (*s1);
      c2 = (gint)(guchar) TOLOWER (*s2);
      if (c1 != c2)
	return (c1 - c2);
      s1++; s2++;
    }

  return (((gint)(guchar) *s1) - ((gint)(guchar) *s2));
}

/**
 * g_ascii_strncasecmp:
 * @s1: string to compare with @s2.
 * @s2: string to compare with @s1.
 * @n:  number of characters to compare.
 * 
 * Compare @s1 and @s2, ignoring the case of ASCII characters and any
 * characters after the first @n in each string.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * characters as if they are not letters.
 * 
 * The same warning as in g_ascii_strcasecmp() applies: Use this
 * function only on strings known to be in encodings where bytes
 * corresponding to ASCII letters always represent themselves.
 *
 * Return value: 0 if the strings match, a negative value if @s1 &lt; @s2, 
 *   or a positive value if @s1 &gt; @s2.
 **/
gint
g_ascii_strncasecmp (const gchar *s1,
		     const gchar *s2,
		     gsize n)
{
  gint c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (n && *s1 && *s2)
    {
      n -= 1;
      c1 = (gint)(guchar) TOLOWER (*s1);
      c2 = (gint)(guchar) TOLOWER (*s2);
      if (c1 != c2)
	return (c1 - c2);
      s1++; s2++;
    }

  if (n)
    return (((gint) (guchar) *s1) - ((gint) (guchar) *s2));
  else
    return 0;
}

/**
 * g_strcasecmp:
 * @s1: a string.
 * @s2: a string to compare with @s1.
 * 
 * A case-insensitive string comparison, corresponding to the standard
 * strcasecmp() function on platforms which support it.
 *
 * Return value: 0 if the strings match, a negative value if @s1 &lt; @s2, 
 *   or a positive value if @s1 &gt; @s2.
 *
 * Deprecated:2.2: See g_strncasecmp() for a discussion of why this function 
 *   is deprecated and how to replace it.
 **/
gint
g_strcasecmp (const gchar *s1,
	      const gchar *s2)
{
#ifdef HAVE_STRCASECMP
  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  return strcasecmp (s1, s2);
#else
  gint c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (*s1 && *s2)
    {
      /* According to A. Cox, some platforms have islower's that
       * don't work right on non-uppercase
       */
      c1 = isupper ((guchar)*s1) ? tolower ((guchar)*s1) : *s1;
      c2 = isupper ((guchar)*s2) ? tolower ((guchar)*s2) : *s2;
      if (c1 != c2)
	return (c1 - c2);
      s1++; s2++;
    }

  return (((gint)(guchar) *s1) - ((gint)(guchar) *s2));
#endif
}

/**
 * g_strncasecmp:
 * @s1: a string.
 * @s2: a string to compare with @s1.
 * @n: the maximum number of characters to compare.
 * 
 * A case-insensitive string comparison, corresponding to the standard
 * strncasecmp() function on platforms which support it.
 * It is similar to g_strcasecmp() except it only compares the first @n 
 * characters of the strings.
 * 
 * Return value: 0 if the strings match, a negative value if @s1 &lt; @s2, 
 *   or a positive value if @s1 &gt; @s2.
 *
 * Deprecated:2.2: The problem with g_strncasecmp() is that it does the 
 * comparison by calling toupper()/tolower(). These functions are
 * locale-specific and operate on single bytes. However, it is impossible
 * to handle things correctly from an I18N standpoint by operating on
 * bytes, since characters may be multibyte. Thus g_strncasecmp() is
 * broken if your string is guaranteed to be ASCII, since it's
 * locale-sensitive, and it's broken if your string is localized, since
 * it doesn't work on many encodings at all, including UTF-8, EUC-JP,
 * etc.
 *
 * There are therefore two replacement functions: g_ascii_strncasecmp(),
 * which only works on ASCII and is not locale-sensitive, and
 * g_utf8_casefold(), which is good for case-insensitive sorting of UTF-8.
 **/
gint
g_strncasecmp (const gchar *s1,
	       const gchar *s2,
	       guint n)     
{
#ifdef HAVE_STRNCASECMP
  return strncasecmp (s1, s2, n);
#else
  gint c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (n && *s1 && *s2)
    {
      n -= 1;
      /* According to A. Cox, some platforms have islower's that
       * don't work right on non-uppercase
       */
      c1 = isupper ((guchar)*s1) ? tolower ((guchar)*s1) : *s1;
      c2 = isupper ((guchar)*s2) ? tolower ((guchar)*s2) : *s2;
      if (c1 != c2)
	return (c1 - c2);
      s1++; s2++;
    }

  if (n)
    return (((gint) (guchar) *s1) - ((gint) (guchar) *s2));
  else
    return 0;
#endif
}

gchar*
g_strdelimit (gchar	  *string,
	      const gchar *delimiters,
	      gchar	   new_delim)
{
  register gchar *c;

  g_return_val_if_fail (string != NULL, NULL);

  if (!delimiters)
    delimiters = G_STR_DELIMITERS;

  for (c = string; *c; c++)
    {
      if (strchr (delimiters, *c))
	*c = new_delim;
    }

  return string;
}

gchar*
g_strcanon (gchar       *string,
	    const gchar *valid_chars,
	    gchar        substitutor)
{
  register gchar *c;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (valid_chars != NULL, NULL);

  for (c = string; *c; c++)
    {
      if (!strchr (valid_chars, *c))
	*c = substitutor;
    }

  return string;
}

gchar*
g_strcompress (const gchar *source)
{
  const gchar *p = source, *octal;
  gchar *dest = g_malloc (strlen (source) + 1);
  gchar *q = dest;
  
  while (*p)
    {
      if (*p == '\\')
	{
	  p++;
	  switch (*p)
	    {
	    case '\0':
	      g_warning ("g_strcompress: trailing \\");
	      goto out;
	    case '0':  case '1':  case '2':  case '3':  case '4':
	    case '5':  case '6':  case '7':
	      *q = 0;
	      octal = p;
	      while ((p < octal + 3) && (*p >= '0') && (*p <= '7'))
		{
		  *q = (*q * 8) + (*p - '0');
		  p++;
		}
	      q++;
	      p--;
	      break;
	    case 'b':
	      *q++ = '\b';
	      break;
	    case 'f':
	      *q++ = '\f';
	      break;
	    case 'n':
	      *q++ = '\n';
	      break;
	    case 'r':
	      *q++ = '\r';
	      break;
	    case 't':
	      *q++ = '\t';
	      break;
	    default:		/* Also handles \" and \\ */
	      *q++ = *p;
	      break;
	    }
	}
      else
	*q++ = *p;
      p++;
    }
out:
  *q = 0;
  
  return dest;
}

gchar *
g_strescape (const gchar *source,
	     const gchar *exceptions)
{
  const guchar *p;
  gchar *dest;
  gchar *q;
  guchar excmap[256];
  
  g_return_val_if_fail (source != NULL, NULL);

  p = (guchar *) source;
  /* Each source byte needs maximally four destination chars (\777) */
  q = dest = g_malloc (strlen (source) * 4 + 1);

  memset (excmap, 0, 256);
  if (exceptions)
    {
      guchar *e = (guchar *) exceptions;

      while (*e)
	{
	  excmap[*e] = 1;
	  e++;
	}
    }

  while (*p)
    {
      if (excmap[*p])
	*q++ = *p;
      else
	{
	  switch (*p)
	    {
	    case '\b':
	      *q++ = '\\';
	      *q++ = 'b';
	      break;
	    case '\f':
	      *q++ = '\\';
	      *q++ = 'f';
	      break;
	    case '\n':
	      *q++ = '\\';
	      *q++ = 'n';
	      break;
	    case '\r':
	      *q++ = '\\';
	      *q++ = 'r';
	      break;
	    case '\t':
	      *q++ = '\\';
	      *q++ = 't';
	      break;
	    case '\\':
	      *q++ = '\\';
	      *q++ = '\\';
	      break;
	    case '"':
	      *q++ = '\\';
	      *q++ = '"';
	      break;
	    default:
	      if ((*p < ' ') || (*p >= 0177))
		{
		  *q++ = '\\';
		  *q++ = '0' + (((*p) >> 6) & 07);
		  *q++ = '0' + (((*p) >> 3) & 07);
		  *q++ = '0' + ((*p) & 07);
		}
	      else
		*q++ = *p;
	      break;
	    }
	}
      p++;
    }
  *q = 0;
  return dest;
}

gchar*
g_strchug (gchar *string)
{
  guchar *start;

  g_return_val_if_fail (string != NULL, NULL);

  for (start = (guchar*) string; *start && g_ascii_isspace (*start); start++)
    ;

  g_memmove (string, start, strlen ((gchar *) start) + 1);

  return string;
}

gchar*
g_strchomp (gchar *string)
{
  gsize len;

  g_return_val_if_fail (string != NULL, NULL);

  len = strlen (string);
  while (len--)
    {
      if (g_ascii_isspace ((guchar) string[len]))
	string[len] = '\0';
      else
	break;
    }

  return string;
}

/**
 * g_strsplit:
 * @string: a string to split.
 * @delimiter: a string which specifies the places at which to split the string.
 *     The delimiter is not included in any of the resulting strings, unless
 *     @max_tokens is reached.
 * @max_tokens: the maximum number of pieces to split @string into. If this is
 *              less than 1, the string is split completely.
 * 
 * Splits a string into a maximum of @max_tokens pieces, using the given
 * @delimiter. If @max_tokens is reached, the remainder of @string is appended
 * to the last token. 
 *
 * As a special case, the result of splitting the empty string "" is an empty
 * vector, not a vector containing a single string. The reason for this
 * special case is that being able to represent a empty vector is typically
 * more useful than consistent handling of empty elements. If you do need
 * to represent empty elements, you'll need to check for the empty string
 * before calling g_strsplit().
 * 
 * Return value: a newly-allocated %NULL-terminated array of strings. Use 
 *    g_strfreev() to free it.
 **/
gchar**
g_strsplit (const gchar *string,
	    const gchar *delimiter,
	    gint         max_tokens)
{
  GSList *string_list = NULL, *slist;
  gchar **str_array, *s;
  guint n = 0;
  const gchar *remainder;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (delimiter != NULL, NULL);
  g_return_val_if_fail (delimiter[0] != '\0', NULL);

  if (max_tokens < 1)
    max_tokens = G_MAXINT;

  remainder = string;
  s = strstr (remainder, delimiter);
  if (s)
    {
      gsize delimiter_len = strlen (delimiter);   

      while (--max_tokens && s)
	{
	  gsize len;     

	  len = s - remainder;
	  string_list = g_slist_prepend (string_list,
					 g_strndup (remainder, len));
	  n++;
	  remainder = s + delimiter_len;
	  s = strstr (remainder, delimiter);
	}
    }
  if (*string)
    {
      n++;
      string_list = g_slist_prepend (string_list, g_strdup (remainder));
    }

  str_array = g_new (gchar*, n + 1);

  str_array[n--] = NULL;
  for (slist = string_list; slist; slist = slist->next)
    str_array[n--] = slist->data;

  g_slist_free (string_list);

  return str_array;
}

/**
 * g_strsplit_set:
 * @string: The string to be tokenized
 * @delimiters: A nul-terminated string containing bytes that are used
 *              to split the string.
 * @max_tokens: The maximum number of tokens to split @string into. 
 *              If this is less than 1, the string is split completely
 * 
 * Splits @string into a number of tokens not containing any of the characters
 * in @delimiter. A token is the (possibly empty) longest string that does not
 * contain any of the characters in @delimiters. If @max_tokens is reached, the
 * remainder is appended to the last token.
 *
 * For example the result of g_strsplit_set ("abc:def/ghi", ":/", -1) is a
 * %NULL-terminated vector containing the three strings "abc", "def", 
 * and "ghi".
 *
 * The result if g_strsplit_set (":def/ghi:", ":/", -1) is a %NULL-terminated
 * vector containing the four strings "", "def", "ghi", and "".
 * 
 * As a special case, the result of splitting the empty string "" is an empty
 * vector, not a vector containing a single string. The reason for this
 * special case is that being able to represent a empty vector is typically
 * more useful than consistent handling of empty elements. If you do need
 * to represent empty elements, you'll need to check for the empty string
 * before calling g_strsplit_set().
 *
 * Note that this function works on bytes not characters, so it can't be used 
 * to delimit UTF-8 strings for anything but ASCII characters.
 * 
 * Return value: a newly-allocated %NULL-terminated array of strings. Use 
 *    g_strfreev() to free it.
 * 
 * Since: 2.4
 **/
gchar **
g_strsplit_set (const gchar *string,
	        const gchar *delimiters,
	        gint         max_tokens)
{
  gboolean delim_table[256];
  GSList *tokens, *list;
  gint n_tokens;
  const gchar *s;
  const gchar *current;
  gchar *token;
  gchar **result;
  
  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (delimiters != NULL, NULL);

  if (max_tokens < 1)
    max_tokens = G_MAXINT;

  if (*string == '\0')
    {
      result = g_new (char *, 1);
      result[0] = NULL;
      return result;
    }
  
  memset (delim_table, FALSE, sizeof (delim_table));
  for (s = delimiters; *s != '\0'; ++s)
    delim_table[*(guchar *)s] = TRUE;

  tokens = NULL;
  n_tokens = 0;

  s = current = string;
  while (*s != '\0')
    {
      if (delim_table[*(guchar *)s] && n_tokens + 1 < max_tokens)
	{
	  token = g_strndup (current, s - current);
	  tokens = g_slist_prepend (tokens, token);
	  ++n_tokens;

	  current = s + 1;
	}
      
      ++s;
    }

  token = g_strndup (current, s - current);
  tokens = g_slist_prepend (tokens, token);
  ++n_tokens;

  result = g_new (gchar *, n_tokens + 1);

  result[n_tokens] = NULL;
  for (list = tokens; list != NULL; list = list->next)
    result[--n_tokens] = list->data;

  g_slist_free (tokens);
  
  return result;
}

/**
 * g_strfreev:
 * @str_array: a %NULL-terminated array of strings to free.

 * Frees a %NULL-terminated array of strings, and the array itself.
 * If called on a %NULL value, g_strfreev() simply returns. 
 **/
void
g_strfreev (gchar **str_array)
{
  if (str_array)
    {
      int i;

      for (i = 0; str_array[i] != NULL; i++)
	g_free (str_array[i]);

      g_free (str_array);
    }
}

/**
 * g_strdupv:
 * @str_array: %NULL-terminated array of strings.
 * 
 * Copies %NULL-terminated array of strings. The copy is a deep copy;
 * the new array should be freed by first freeing each string, then
 * the array itself. g_strfreev() does this for you. If called
 * on a %NULL value, g_strdupv() simply returns %NULL.
 * 
 * Return value: a new %NULL-terminated array of strings.
 **/
gchar**
g_strdupv (gchar **str_array)
{
  if (str_array)
    {
      gint i;
      gchar **retval;

      i = 0;
      while (str_array[i])
        ++i;
          
      retval = g_new (gchar*, i + 1);

      i = 0;
      while (str_array[i])
        {
          retval[i] = g_strdup (str_array[i]);
          ++i;
        }
      retval[i] = NULL;

      return retval;
    }
  else
    return NULL;
}

/**
 * g_strjoinv:
 * @separator: a string to insert between each of the strings, or %NULL
 * @str_array: a %NULL-terminated array of strings to join
 * 
 * Joins a number of strings together to form one long string, with the 
 * optional @separator inserted between each of them. The returned string
 * should be freed with g_free().
 *
 * Returns: a newly-allocated string containing all of the strings joined
 *     together, with @separator between them
 */
gchar*
g_strjoinv (const gchar  *separator,
	    gchar       **str_array)
{
  gchar *string;
  gchar *ptr;

  g_return_val_if_fail (str_array != NULL, NULL);

  if (separator == NULL)
    separator = "";

  if (*str_array)
    {
      gint i;
      gsize len;
      gsize separator_len;     

      separator_len = strlen (separator);
      /* First part, getting length */
      len = 1 + strlen (str_array[0]);
      for (i = 1; str_array[i] != NULL; i++)
        len += strlen (str_array[i]);
      len += separator_len * (i - 1);

      /* Second part, building string */
      string = g_new (gchar, len);
      ptr = g_stpcpy (string, *str_array);
      for (i = 1; str_array[i] != NULL; i++)
	{
          ptr = g_stpcpy (ptr, separator);
          ptr = g_stpcpy (ptr, str_array[i]);
	}
      }
  else
    string = g_strdup ("");

  return string;
}

/**
 * g_strjoin:
 * @separator: a string to insert between each of the strings, or %NULL
 * @Varargs: a %NULL-terminated list of strings to join
 *
 * Joins a number of strings together to form one long string, with the 
 * optional @separator inserted between each of them. The returned string
 * should be freed with g_free().
 *
 * Returns: a newly-allocated string containing all of the strings joined
 *     together, with @separator between them
 */
gchar*
g_strjoin (const gchar  *separator,
	   ...)
{
  gchar *string, *s;
  va_list args;
  gsize len;               
  gsize separator_len;     
  gchar *ptr;

  if (separator == NULL)
    separator = "";

  separator_len = strlen (separator);

  va_start (args, separator);

  s = va_arg (args, gchar*);

  if (s)
    {
      /* First part, getting length */
      len = 1 + strlen (s);

      s = va_arg (args, gchar*);
      while (s)
	{
	  len += separator_len + strlen (s);
	  s = va_arg (args, gchar*);
	}
      va_end (args);

      /* Second part, building string */
      string = g_new (gchar, len);

      va_start (args, separator);

      s = va_arg (args, gchar*);
      ptr = g_stpcpy (string, s);

      s = va_arg (args, gchar*);
      while (s)
	{
	  ptr = g_stpcpy (ptr, separator);
          ptr = g_stpcpy (ptr, s);
	  s = va_arg (args, gchar*);
	}
    }
  else
    string = g_strdup ("");

  va_end (args);

  return string;
}


/**
 * g_strstr_len:
 * @haystack: a string.
 * @haystack_len: the maximum length of @haystack. Note that -1 is
 * a valid length, if @haystack is nul-terminated, meaning it will
 * search through the whole string.
 * @needle: the string to search for.
 *
 * Searches the string @haystack for the first occurrence
 * of the string @needle, limiting the length of the search
 * to @haystack_len. 
 *
 * Return value: a pointer to the found occurrence, or
 *    %NULL if not found.
 **/
gchar *
g_strstr_len (const gchar *haystack,
	      gssize       haystack_len,
	      const gchar *needle)
{
  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);

  if (haystack_len < 0)
    return strstr (haystack, needle);
  else
    {
      const gchar *p = haystack;
      gsize needle_len = strlen (needle);
      const gchar *end;
      gsize i;

      if (needle_len == 0)
	return (gchar *)haystack;

      if (haystack_len < needle_len)
	return NULL;
      
      end = haystack + haystack_len - needle_len;
      
      while (p <= end && *p)
	{
	  for (i = 0; i < needle_len; i++)
	    if (p[i] != needle[i])
	      goto next;
	  
	  return (gchar *)p;
	  
	next:
	  p++;
	}
      
      return NULL;
    }
}

/**
 * g_strrstr:
 * @haystack: a nul-terminated string.
 * @needle: the nul-terminated string to search for.
 *
 * Searches the string @haystack for the last occurrence
 * of the string @needle.
 *
 * Return value: a pointer to the found occurrence, or
 *    %NULL if not found.
 **/
gchar *
g_strrstr (const gchar *haystack,
	   const gchar *needle)
{
  gsize i;
  gsize needle_len;
  gsize haystack_len;
  const gchar *p;
      
  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);

  needle_len = strlen (needle);
  haystack_len = strlen (haystack);

  if (needle_len == 0)
    return (gchar *)haystack;

  if (haystack_len < needle_len)
    return NULL;
  
  p = haystack + haystack_len - needle_len;

  while (p >= haystack)
    {
      for (i = 0; i < needle_len; i++)
	if (p[i] != needle[i])
	  goto next;
      
      return (gchar *)p;
      
    next:
      p--;
    }
  
  return NULL;
}

/**
 * g_strrstr_len:
 * @haystack: a nul-terminated string.
 * @haystack_len: the maximum length of @haystack.
 * @needle: the nul-terminated string to search for.
 *
 * Searches the string @haystack for the last occurrence
 * of the string @needle, limiting the length of the search
 * to @haystack_len. 
 *
 * Return value: a pointer to the found occurrence, or
 *    %NULL if not found.
 **/
gchar *
g_strrstr_len (const gchar *haystack,
	       gssize        haystack_len,
	       const gchar *needle)
{
  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);
  
  if (haystack_len < 0)
    return g_strrstr (haystack, needle);
  else
    {
      gsize needle_len = strlen (needle);
      const gchar *haystack_max = haystack + haystack_len;
      const gchar *p = haystack;
      gsize i;

      while (p < haystack_max && *p)
	p++;

      if (p < haystack + needle_len)
	return NULL;
	
      p -= needle_len;

      while (p >= haystack)
	{
	  for (i = 0; i < needle_len; i++)
	    if (p[i] != needle[i])
	      goto next;
	  
	  return (gchar *)p;
	  
	next:
	  p--;
	}

      return NULL;
    }
}


/**
 * g_str_has_suffix:
 * @str: a nul-terminated string.
 * @suffix: the nul-terminated suffix to look for.
 *
 * Looks whether the string @str ends with @suffix.
 *
 * Return value: %TRUE if @str end with @suffix, %FALSE otherwise.
 *
 * Since: 2.2
 **/
gboolean
g_str_has_suffix (const gchar  *str,
		  const gchar  *suffix)
{
  int str_len;
  int suffix_len;
  
  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (suffix != NULL, FALSE);

  str_len = strlen (str);
  suffix_len = strlen (suffix);

  if (str_len < suffix_len)
    return FALSE;

  return strcmp (str + str_len - suffix_len, suffix) == 0;
}

/**
 * g_str_has_prefix:
 * @str: a nul-terminated string.
 * @prefix: the nul-terminated prefix to look for.
 *
 * Looks whether the string @str begins with @prefix.
 *
 * Return value: %TRUE if @str begins with @prefix, %FALSE otherwise.
 *
 * Since: 2.2
 **/
gboolean
g_str_has_prefix (const gchar  *str,
		  const gchar  *prefix)
{
  int str_len;
  int prefix_len;
  
  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (prefix != NULL, FALSE);

  str_len = strlen (str);
  prefix_len = strlen (prefix);

  if (str_len < prefix_len)
    return FALSE;
  
  return strncmp (str, prefix, prefix_len) == 0;
}


/**
 * g_strip_context:
 * @msgid: a string
 * @msgval: another string
 * 
 * An auxiliary function for gettext() support (see Q_()).
 * 
 * Return value: @msgval, unless @msgval is identical to @msgid and contains
 *   a '|' character, in which case a pointer to the substring of msgid after
 *   the first '|' character is returned. 
 *
 * Since: 2.4
 **/
G_CONST_RETURN gchar *
g_strip_context  (const gchar *msgid, 
		  const gchar *msgval)
{
  if (msgval == msgid)
    {
      const char *c = strchr (msgid, '|');
      if (c != NULL)
	return c + 1;
    }
  
  return msgval;
}


/**
 * g_strv_length:
 * @str_array: a %NULL-terminated array of strings.
 * 
 * Returns the length of the given %NULL-terminated 
 * string array @str_array.
 * 
 * Return value: length of @str_array.
 *
 * Since: 2.6
 **/
guint
g_strv_length (gchar **str_array)
{
  guint i = 0;

  g_return_val_if_fail (str_array != NULL, 0);

  while (str_array[i])
    ++i;

  return i;
}


/**
 * g_dpgettext:
 * @domain: the translation domain to use, or %NULL to use
 *   the domain set with textdomain()
 * @msgctxtid: a combined message context and message id, separated
 *   by a \004 character
 * @msgidoffset: the offset of the message id in @msgctxid
 *
 * This function is a variant of g_dgettext() which supports
 * a disambiguating message context. GNU gettext uses the
 * '\004' character to separate the message context and
 * message id in @msgctxtid.
 * If 0 is passed as @msgidoffset, this function will fall back to
 * trying to use the deprecated convention of using "|" as a separation
 * character.
 *
 * This uses g_dgettext() internally.  See that functions for differences
 * with dgettext() proper.
 *
 * Applications should normally not use this function directly,
 * but use the C_() macro for translations with context.
 *
 * Returns: The translated string
 *
 * Since: 2.16
 */
G_CONST_RETURN gchar *
g_dpgettext (const gchar *domain, 
             const gchar *msgctxtid, 
             gsize        msgidoffset)
{
  const gchar *translation;
  gchar *sep;

  translation = g_dgettext (domain, msgctxtid);

  if (translation == msgctxtid)
    {
      if (msgidoffset > 0)
        return msgctxtid + msgidoffset;

      sep = strchr (msgctxtid, '|');
 
      if (sep)
        {
          /* try with '\004' instead of '|', in case
           * xgettext -kQ_:1g was used
           */
          gchar *tmp = g_alloca (strlen (msgctxtid) + 1);
          strcpy (tmp, msgctxtid);
          tmp[sep - msgctxtid] = '\004';

          translation = g_dgettext (domain, tmp);
   
          if (translation == tmp)
            return sep + 1; 
        }
    }

  return translation;
}

/* This function is taken from gettext.h 
 * GNU gettext uses '\004' to separate context and msgid in .mo files.
 */
/**
 * g_dpgettext2:
 * @domain: the translation domain to use, or %NULL to use
 *   the domain set with textdomain()
 * @context: the message context
 * @msgid: the message
 *
 * This function is a variant of g_dgettext() which supports
 * a disambiguating message context. GNU gettext uses the
 * '\004' character to separate the message context and
 * message id in @msgctxtid.
 *
 * This uses g_dgettext() internally.  See that functions for differences
 * with dgettext() proper.
 *
 * This function differs from C_() in that it is not a macro and 
 * thus you may use non-string-literals as context and msgid arguments.
 *
 * Returns: The translated string
 *
 * Since: 2.18
 */
G_CONST_RETURN char *
g_dpgettext2 (const char *domain,
              const char *msgctxt,
              const char *msgid)
{
  size_t msgctxt_len = strlen (msgctxt) + 1;
  size_t msgid_len = strlen (msgid) + 1;
  const char *translation;
  char* msg_ctxt_id;

  msg_ctxt_id = g_alloca (msgctxt_len + msgid_len);

  memcpy (msg_ctxt_id, msgctxt, msgctxt_len - 1);
  msg_ctxt_id[msgctxt_len - 1] = '\004';
  memcpy (msg_ctxt_id + msgctxt_len, msgid, msgid_len);

  translation = g_dgettext (domain, msg_ctxt_id);

  if (translation == msg_ctxt_id) 
    {
      /* try the old way of doing message contexts, too */
      msg_ctxt_id[msgctxt_len - 1] = '|';
      translation = g_dgettext (domain, msg_ctxt_id);

      if (translation == msg_ctxt_id)
        return msgid;
    }

  return translation;
}

static gboolean
_g_dgettext_should_translate (void)
{
  static gsize translate = 0;
  enum {
    SHOULD_TRANSLATE = 1,
    SHOULD_NOT_TRANSLATE = 2
  };

  if (G_UNLIKELY (g_once_init_enter (&translate)))
    {
      gboolean should_translate = TRUE;

      const char *default_domain     = textdomain (NULL);
      const char *translator_comment = gettext ("");
#ifndef G_OS_WIN32
      const char *translate_locale   = setlocale (LC_MESSAGES, NULL);
#else
      const char *translate_locale   = g_win32_getlocale ();
#endif
      /* We should NOT translate only if all the following hold:
       *   - user has called textdomain() and set textdomain to non-default
       *   - default domain has no translations
       *   - locale does not start with "en_" and is not "C"
       *
       * Rationale:
       *   - If text domain is still the default domain, maybe user calls
       *     it later. Continue with old behavior of translating.
       *   - If locale starts with "en_", we can continue using the
       *     translations even if the app doesn't have translations for
       *     this locale.  That is, en_UK and en_CA for example.
       *   - If locale is "C", maybe user calls setlocale(LC_ALL,"") later.
       *     Continue with old behavior of translating.
       */
      if (0 != strcmp (default_domain, "messages") &&
	  '\0' == *translator_comment &&
	  0 != strncmp (translate_locale, "en_", 3) &&
	  0 != strcmp (translate_locale, "C"))
        should_translate = FALSE;
      
      g_once_init_leave (&translate,
			 should_translate ?
			 SHOULD_TRANSLATE :
			 SHOULD_NOT_TRANSLATE);
    }

  return translate == SHOULD_TRANSLATE;
}

/**
 * g_dgettext:
 * @domain: the translation domain to use, or %NULL to use
 *   the domain set with textdomain()
 * @msgid: message to translate
 *
 * This function is a wrapper of dgettext() which does not translate
 * the message if the default domain as set with textdomain() has no
 * translations for the current locale.
 *
 * The advantage of using this function over dgettext() proper is that
 * libraries using this function (like GTK+) will not use translations
 * if the application using the library does not have translations for
 * the current locale.  This results in a consistent English-only
 * interface instead of one having partial translations.  For this
 * feature to work, the call to textdomain() and setlocale() should
 * precede any g_dgettext() invocations.  For GTK+, it means calling
 * textdomain() before gtk_init or its variants.
 *
 * This function disables translations if and only if upon its first
 * call all the following conditions hold:
 * <itemizedlist>
 * <listitem>@domain is not %NULL</listitem>
 * <listitem>textdomain() has been called to set a default text domain</listitem>
 * <listitem>there is no translations available for the default text domain
 *           and the current locale</listitem>
 * <listitem>current locale is not "C" or any English locales (those
 *           starting with "en_")</listitem>
 * </itemizedlist>
 *
 * Note that this behavior may not be desired for example if an application
 * has its untranslated messages in a language other than English.  In those
 * cases the application should call textdomain() after initializing GTK+.
 *
 * Applications should normally not use this function directly,
 * but use the _() macro for translations.
 *
 * Returns: The translated string
 *
 * Since: 2.18
 */
G_CONST_RETURN gchar *
g_dgettext (const gchar *domain,
            const gchar *msgid)
{
  if (domain && G_UNLIKELY (!_g_dgettext_should_translate ()))
    return msgid;

  return dgettext (domain, msgid);
}

/**
 * g_dngettext:
 * @domain: the translation domain to use, or %NULL to use
 *   the domain set with textdomain()
 * @msgid: message to translate
 * @msgid_plural: plural form of the message
 * @n: the quantity for which translation is needed
 *
 * This function is a wrapper of dngettext() which does not translate
 * the message if the default domain as set with textdomain() has no
 * translations for the current locale.
 *
 * See g_dgettext() for details of how this differs from dngettext()
 * proper.
 *
 * Returns: The translated string
 *
 * Since: 2.18
 */
G_CONST_RETURN gchar *
g_dngettext (const gchar *domain,
             const gchar *msgid,
             const gchar *msgid_plural,
	     gulong       n)
{
  if (domain && G_UNLIKELY (!_g_dgettext_should_translate ()))
    return n == 1 ? msgid : msgid_plural;

  return dngettext (domain, msgid, msgid_plural, n);
}


#define __G_STRFUNCS_C__
#include "galiasdef.c"
