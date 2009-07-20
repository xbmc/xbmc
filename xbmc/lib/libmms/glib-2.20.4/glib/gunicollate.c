/* gunicollate.c - Collation
 *
 *  Copyright 2001,2005 Red Hat, Inc.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <locale.h>
#include <string.h>
#ifdef __STDC_ISO_10646__
#include <wchar.h>
#endif

#ifdef HAVE_CARBON
#include <CoreServices/CoreServices.h>
#endif

#include "glib.h"
#include "gunicodeprivate.h"
#include "galias.h"

#ifdef _MSC_VER
/* Workaround for bug in MSVCR80.DLL */
static gsize
msc_strxfrm_wrapper (char       *string1,
		     const char *string2,
		     gsize       count)
{
  if (!string1 || count <= 0)
    {
      char tmp;

      return strxfrm (&tmp, string2, 1);
    }
  return strxfrm (string1, string2, count);
}
#define strxfrm msc_strxfrm_wrapper
#endif

/**
 * g_utf8_collate:
 * @str1: a UTF-8 encoded string
 * @str2: a UTF-8 encoded string
 * 
 * Compares two strings for ordering using the linguistically
 * correct rules for the <link linkend="setlocale">current locale</link>. 
 * When sorting a large number of strings, it will be significantly 
 * faster to obtain collation keys with g_utf8_collate_key() and 
 * compare the keys with strcmp() when sorting instead of sorting 
 * the original strings.
 * 
 * Return value: &lt; 0 if @str1 compares before @str2, 
 *   0 if they compare equal, &gt; 0 if @str1 compares after @str2.
 **/
gint
g_utf8_collate (const gchar *str1,
		const gchar *str2)
{
  gint result;

#ifdef HAVE_CARBON

  UniChar *str1_utf16;
  UniChar *str2_utf16;
  glong len1;
  glong len2;
  SInt32 retval = 0;

  g_return_val_if_fail (str1 != NULL, 0);
  g_return_val_if_fail (str2 != NULL, 0);

  str1_utf16 = g_utf8_to_utf16 (str1, -1, NULL, &len1, NULL);
  str2_utf16 = g_utf8_to_utf16 (str2, -1, NULL, &len2, NULL);

  UCCompareTextDefault (kUCCollateStandardOptions,
                        str1_utf16, len1, str2_utf16, len2,
                        NULL, &retval);
  result = retval;

  g_free (str2_utf16);
  g_free (str1_utf16);

#elif defined(__STDC_ISO_10646__)

  gunichar *str1_norm;
  gunichar *str2_norm;

  g_return_val_if_fail (str1 != NULL, 0);
  g_return_val_if_fail (str2 != NULL, 0);

  str1_norm = _g_utf8_normalize_wc (str1, -1, G_NORMALIZE_ALL_COMPOSE);
  str2_norm = _g_utf8_normalize_wc (str2, -1, G_NORMALIZE_ALL_COMPOSE);

  result = wcscoll ((wchar_t *)str1_norm, (wchar_t *)str2_norm);

  g_free (str1_norm);
  g_free (str2_norm);

#else /* !__STDC_ISO_10646__ */

  const gchar *charset;
  gchar *str1_norm;
  gchar *str2_norm;

  g_return_val_if_fail (str1 != NULL, 0);
  g_return_val_if_fail (str2 != NULL, 0);

  str1_norm = g_utf8_normalize (str1, -1, G_NORMALIZE_ALL_COMPOSE);
  str2_norm = g_utf8_normalize (str2, -1, G_NORMALIZE_ALL_COMPOSE);

  if (g_get_charset (&charset))
    {
      result = strcoll (str1_norm, str2_norm);
    }
  else
    {
      gchar *str1_locale = g_convert (str1_norm, -1, charset, "UTF-8", NULL, NULL, NULL);
      gchar *str2_locale = g_convert (str2_norm, -1, charset, "UTF-8", NULL, NULL, NULL);

      if (str1_locale && str2_locale)
	result =  strcoll (str1_locale, str2_locale);
      else if (str1_locale)
	result = -1;
      else if (str2_locale)
	result = 1;
      else
	result = strcmp (str1_norm, str2_norm);

      g_free (str1_locale);
      g_free (str2_locale);
    }

  g_free (str1_norm);
  g_free (str2_norm);

#endif /* __STDC_ISO_10646__ */

  return result;
}

#if defined(__STDC_ISO_10646__) || defined(HAVE_CARBON)
/* We need UTF-8 encoding of numbers to encode the weights if
 * we are using wcsxfrm. However, we aren't encoding Unicode
 * characters, so we can't simply use g_unichar_to_utf8.
 *
 * The following routine is taken (with modification) from GNU
 * libc's strxfrm routine:
 *
 * Copyright (C) 1995-1999,2000,2001 Free Software Foundation, Inc.
 * Written by Ulrich Drepper <drepper@cygnus.com>, 1995.
 */
static inline int
utf8_encode (char *buf, wchar_t val)
{
  int retval;

  if (val < 0x80)
    {
      if (buf)
	*buf++ = (char) val;
      retval = 1;
    }
  else
    {
      int step;

      for (step = 2; step < 6; ++step)
        if ((val & (~(guint32)0 << (5 * step + 1))) == 0)
          break;
      retval = step;

      if (buf)
	{
	  *buf = (unsigned char) (~0xff >> step);
	  --step;
	  do
	    {
	      buf[step] = 0x80 | (val & 0x3f);
	      val >>= 6;
	    }
	  while (--step > 0);
	  *buf |= val;
	}
    }

  return retval;
}
#endif /* __STDC_ISO_10646__ || HAVE_CARBON */

#ifdef HAVE_CARBON

static gchar *
collate_key_to_string (UCCollationValue *key,
                       gsize             key_len)
{
  gchar *result;
  gsize result_len;
  gsize i;

  /* Pretty smart algorithm here: ignore first eight bytes of the
   * collation key. It doesn't produce results equivalent to
   * UCCompareCollationKeys's, but the difference seems to be only
   * that UCCompareCollationKeys in some cases produces 0 where our
   * comparison gets -1 or 1. */

  if (key_len * sizeof (UCCollationValue) <= 8)
    return g_strdup ("");

  result_len = 0;
  for (i = 8; i < key_len * sizeof (UCCollationValue); i++)
    /* there may be nul bytes, encode byteval+1 */
    result_len += utf8_encode (NULL, *((guchar*)key + i) + 1);

  result = g_malloc (result_len + 1);
  result_len = 0;
  for (i = 8; i < key_len * sizeof (UCCollationValue); i++)
    result_len += utf8_encode (result + result_len, *((guchar*)key + i) + 1);

  result[result_len] = 0;
  return result;
}

static gchar *
carbon_collate_key_with_collator (const gchar *str,
                                  gssize       len,
                                  CollatorRef  collator)
{
  UniChar *str_utf16 = NULL;
  glong len_utf16;
  OSStatus ret;
  UCCollationValue staticbuf[512];
  UCCollationValue *freeme = NULL;
  UCCollationValue *buf;
  ItemCount buf_len;
  ItemCount key_len;
  ItemCount try_len;
  gchar *result = NULL;

  str_utf16 = g_utf8_to_utf16 (str, len, NULL, &len_utf16, NULL);
  try_len = len_utf16 * 5 + 2;

  if (try_len <= sizeof staticbuf)
    {
      buf = staticbuf;
      buf_len = sizeof staticbuf;
    }
  else
    {
      freeme = g_new (UCCollationValue, try_len);
      buf = freeme;
      buf_len = try_len;
    }

  ret = UCGetCollationKey (collator, str_utf16, len_utf16,
                           buf_len, &key_len, buf);

  if (ret == kCollateBufferTooSmall)
    {
      freeme = g_renew (UCCollationValue, freeme, try_len * 2);
      buf = freeme;
      buf_len = try_len * 2;
      ret = UCGetCollationKey (collator, str_utf16, len_utf16,
                               buf_len, &key_len, buf);
    }

  if (ret == 0)
    result = collate_key_to_string (buf, key_len);
  else
    result = g_strdup ("");

  g_free (freeme);
  g_free (str_utf16);
  return result;
}

static gchar *
carbon_collate_key (const gchar *str,
                    gssize       len)
{
  static CollatorRef collator;

  if (G_UNLIKELY (!collator))
    {
      UCCreateCollator (NULL, 0, kUCCollateStandardOptions, &collator);

      if (!collator)
        {
          static gboolean been_here;
          if (!been_here)
            g_warning ("%s: UCCreateCollator failed", G_STRLOC);
          been_here = TRUE;
          return g_strdup ("");
        }
    }

  return carbon_collate_key_with_collator (str, len, collator);
}

static gchar *
carbon_collate_key_for_filename (const gchar *str,
                                 gssize       len)
{
  static CollatorRef collator;

  if (G_UNLIKELY (!collator))
    {
      /* http://developer.apple.com/qa/qa2004/qa1159.html */
      UCCreateCollator (NULL, 0,
                        kUCCollateComposeInsensitiveMask
                         | kUCCollateWidthInsensitiveMask
                         | kUCCollateCaseInsensitiveMask
                         | kUCCollateDigitsOverrideMask
                         | kUCCollateDigitsAsNumberMask
                         | kUCCollatePunctuationSignificantMask, 
                        &collator);

      if (!collator)
        {
          static gboolean been_here;
          if (!been_here)
            g_warning ("%s: UCCreateCollator failed", G_STRLOC);
          been_here = TRUE;
          return g_strdup ("");
        }
    }

  return carbon_collate_key_with_collator (str, len, collator);
}

#endif /* HAVE_CARBON */

/**
 * g_utf8_collate_key:
 * @str: a UTF-8 encoded string.
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 *
 * Converts a string into a collation key that can be compared
 * with other collation keys produced by the same function using 
 * strcmp(). 
 *
 * The results of comparing the collation keys of two strings 
 * with strcmp() will always be the same as comparing the two 
 * original keys with g_utf8_collate().
 * 
 * Note that this function depends on the 
 * <link linkend="setlocale">current locale</link>.
 * 
 * Return value: a newly allocated string. This string should
 *   be freed with g_free() when you are done with it.
 **/
gchar *
g_utf8_collate_key (const gchar *str,
		    gssize       len)
{
  gchar *result;

#ifdef HAVE_CARBON

  g_return_val_if_fail (str != NULL, NULL);
  result = carbon_collate_key (str, len);

#elif defined(__STDC_ISO_10646__)

  gsize xfrm_len;
  gunichar *str_norm;
  wchar_t *result_wc;
  gsize i;
  gsize result_len = 0;

  g_return_val_if_fail (str != NULL, NULL);

  str_norm = _g_utf8_normalize_wc (str, len, G_NORMALIZE_ALL_COMPOSE);

  xfrm_len = wcsxfrm (NULL, (wchar_t *)str_norm, 0);
  result_wc = g_new (wchar_t, xfrm_len + 1);
  wcsxfrm (result_wc, (wchar_t *)str_norm, xfrm_len + 1);

  for (i=0; i < xfrm_len; i++)
    result_len += utf8_encode (NULL, result_wc[i]);

  result = g_malloc (result_len + 1);
  result_len = 0;
  for (i=0; i < xfrm_len; i++)
    result_len += utf8_encode (result + result_len, result_wc[i]);

  result[result_len] = '\0';

  g_free (result_wc);
  g_free (str_norm);

  return result;
#else /* !__STDC_ISO_10646__ */

  gsize xfrm_len;
  const gchar *charset;
  gchar *str_norm;

  g_return_val_if_fail (str != NULL, NULL);

  str_norm = g_utf8_normalize (str, len, G_NORMALIZE_ALL_COMPOSE);

  result = NULL;

  if (g_get_charset (&charset))
    {
      xfrm_len = strxfrm (NULL, str_norm, 0);
      if (xfrm_len >= 0 && xfrm_len < G_MAXINT - 2)
        {
          result = g_malloc (xfrm_len + 1);
          strxfrm (result, str_norm, xfrm_len + 1);
        }
    }
  else
    {
      gchar *str_locale = g_convert (str_norm, -1, charset, "UTF-8", NULL, NULL, NULL);

      if (str_locale)
	{
	  xfrm_len = strxfrm (NULL, str_locale, 0);
	  if (xfrm_len < 0 || xfrm_len >= G_MAXINT - 2)
	    {
	      g_free (str_locale);
	      str_locale = NULL;
	    }
	}
      if (str_locale)
	{
	  result = g_malloc (xfrm_len + 2);
	  result[0] = 'A';
	  strxfrm (result + 1, str_locale, xfrm_len + 1);
	  
	  g_free (str_locale);
	}
    }
    
  if (!result) 
    {
      xfrm_len = strlen (str_norm);
      result = g_malloc (xfrm_len + 2);
      result[0] = 'B';
      memcpy (result + 1, str_norm, xfrm_len);
      result[xfrm_len+1] = '\0';
    }

  g_free (str_norm);
#endif /* __STDC_ISO_10646__ */

  return result;
}

/* This is a collation key that is very very likely to sort before any
   collation key that libc strxfrm generates. We use this before any
   special case (dot or number) to make sure that its sorted before
   anything else.
 */
#define COLLATION_SENTINEL "\1\1\1"

/**
 * g_utf8_collate_key_for_filename:
 * @str: a UTF-8 encoded string.
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 *
 * Converts a string into a collation key that can be compared
 * with other collation keys produced by the same function using strcmp(). 
 * 
 * In order to sort filenames correctly, this function treats the dot '.' 
 * as a special case. Most dictionary orderings seem to consider it
 * insignificant, thus producing the ordering "event.c" "eventgenerator.c"
 * "event.h" instead of "event.c" "event.h" "eventgenerator.c". Also, we
 * would like to treat numbers intelligently so that "file1" "file10" "file5"
 * is sorted as "file1" "file5" "file10".
 * 
 * Note that this function depends on the 
 * <link linkend="setlocale">current locale</link>.
 *
 * Return value: a newly allocated string. This string should
 *   be freed with g_free() when you are done with it.
 *
 * Since: 2.8
 */
gchar*
g_utf8_collate_key_for_filename (const gchar *str,
				 gssize       len)
{
#ifndef HAVE_CARBON
  GString *result;
  GString *append;
  const gchar *p;
  const gchar *prev;
  const gchar *end;
  gchar *collate_key;
  gint digits;
  gint leading_zeros;

  /*
   * How it works:
   *
   * Split the filename into collatable substrings which do
   * not contain [.0-9] and special-cased substrings. The collatable 
   * substrings are run through the normal g_utf8_collate_key() and the 
   * resulting keys are concatenated with keys generated from the 
   * special-cased substrings.
   *
   * Special cases: Dots are handled by replacing them with '\1' which 
   * implies that short dot-delimited substrings are before long ones, 
   * e.g.
   * 
   *   a\1a   (a.a)
   *   a-\1a  (a-.a)
   *   aa\1a  (aa.a)
   * 
   * Numbers are handled by prepending to each number d-1 superdigits 
   * where d = number of digits in the number and SUPERDIGIT is a 
   * character with an integer value higher than any digit (for instance 
   * ':'). This ensures that single-digit numbers are sorted before 
   * double-digit numbers which in turn are sorted separately from 
   * triple-digit numbers, etc. To avoid strange side-effects when 
   * sorting strings that already contain SUPERDIGITs, a '\2'
   * is also prepended, like this
   *
   *   file\21      (file1)
   *   file\25      (file5)
   *   file\2:10    (file10)
   *   file\2:26    (file26)
   *   file\2::100  (file100)
   *   file:foo     (file:foo)
   * 
   * This has the side-effect of sorting numbers before everything else (except
   * dots), but this is probably OK.
   *
   * Leading digits are ignored when doing the above. To discriminate
   * numbers which differ only in the number of leading digits, we append
   * the number of leading digits as a byte at the very end of the collation
   * key.
   *
   * To try avoid conflict with any collation key sequence generated by libc we
   * start each switch to a special cased part with a sentinel that hopefully
   * will sort before anything libc will generate.
   */

  if (len < 0)
    len = strlen (str);

  result = g_string_sized_new (len * 2);
  append = g_string_sized_new (0);

  end = str + len;

  /* No need to use utf8 functions, since we're only looking for ascii chars */
  for (prev = p = str; p < end; p++)
    {
      switch (*p)
	{
	case '.':
	  if (prev != p) 
	    {
	      collate_key = g_utf8_collate_key (prev, p - prev);
	      g_string_append (result, collate_key);
	      g_free (collate_key);
	    }
	  
	  g_string_append (result, COLLATION_SENTINEL "\1");
	  
	  /* skip the dot */
	  prev = p + 1;
	  break;
	  
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (prev != p) 
	    {
	      collate_key = g_utf8_collate_key (prev, p - prev);
	      g_string_append (result, collate_key);
	      g_free (collate_key);
	    }
	  
	  g_string_append (result, COLLATION_SENTINEL "\2");
	  
	  prev = p;
	  
	  /* write d-1 colons */
	  if (*p == '0')
	    {
	      leading_zeros = 1;
	      digits = 0;
	    }
	  else
	    {
	      leading_zeros = 0;
	      digits = 1;
	    }
	  
	  while (++p < end)
	    {
	      if (*p == '0' && !digits)
		++leading_zeros;
	      else if (g_ascii_isdigit(*p))
		++digits;
	      else
                {
 		  /* count an all-zero sequence as
                   * one digit plus leading zeros
                   */
          	  if (!digits)
                    {
                      ++digits;
                      --leading_zeros;
                    }        
		  break;
                }
	    }

	  while (digits > 1)
	    {
	      g_string_append_c (result, ':');
	      --digits;
	    }

	  if (leading_zeros > 0)
	    {
	      g_string_append_c (append, (char)leading_zeros);
	      prev += leading_zeros;
	    }
	  
	  /* write the number itself */
	  g_string_append_len (result, prev, p - prev);
	  
	  prev = p;
	  --p;	  /* go one step back to avoid disturbing outer loop */
	  break;
	  
	default:
	  /* other characters just accumulate */
	  break;
	}
    }
  
  if (prev != p) 
    {
      collate_key = g_utf8_collate_key (prev, p - prev);
      g_string_append (result, collate_key);
      g_free (collate_key);
    }
  
  g_string_append (result, append->str);
  g_string_free (append, TRUE);

  return g_string_free (result, FALSE);
#else /* HAVE_CARBON */
  return carbon_collate_key_for_filename (str, len);
#endif
}


#define __G_UNICOLLATE_C__
#include "galiasdef.c"
