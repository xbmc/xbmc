/*
 * Copyright (C) 2001 Peter Harris <peter.harris@hummingbird.com>
 * Copyright (C) 2001 Edmund Grimley Evans <edmundo@rano.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Convert a string between UTF-8 and the locale's charset.
 */

#include <stdlib.h>
#include <string.h>

#include "utf8.h"


#ifdef _WIN32

	/* Thanks to Peter Harris <peter.harris@hummingbird.com> for this win32
	 * code.
	 */

#include <stdio.h>
#include <windows.h>

static unsigned char *make_utf8_string(const wchar_t *unicode)
{
    int size = 0, index = 0, out_index = 0;
    unsigned char *out;
    unsigned short c;

    /* first calculate the size of the target string */
    c = unicode[index++];
    while(c) {
        if(c < 0x0080) {
            size += 1;
        } else if(c < 0x0800) {
            size += 2;
        } else {
            size += 3;
        }
        c = unicode[index++];
    }	

    out = malloc(size + 1);
    if (out == NULL)
        return NULL;
    index = 0;

    c = unicode[index++];
    while(c)
    {
        if(c < 0x080) {
            out[out_index++] = (unsigned char)c;
        } else if(c < 0x800) {
            out[out_index++] = 0xc0 | (c >> 6);
            out[out_index++] = 0x80 | (c & 0x3f);
        } else {
            out[out_index++] = 0xe0 | (c >> 12);
            out[out_index++] = 0x80 | ((c >> 6) & 0x3f);
            out[out_index++] = 0x80 | (c & 0x3f);
        }
        c = unicode[index++];
    }
    out[out_index] = 0x00;

    return out;
}

static wchar_t *make_unicode_string(const unsigned char *utf8)
{
    int size = 0, index = 0, out_index = 0;
    wchar_t *out;
    unsigned char c;

    /* first calculate the size of the target string */
    c = utf8[index++];
    while(c) {
        if((c & 0x80) == 0) {
            index += 0;
        } else if((c & 0xe0) == 0xe0) {
            index += 2;
        } else {
            index += 1;
        }
        size += 1;
        c = utf8[index++];
    }	

    out = malloc((size + 1) * sizeof(wchar_t));
    if (out == NULL)
        return NULL;
    index = 0;

    c = utf8[index++];
    while(c)
    {
        if((c & 0x80) == 0) {
            out[out_index++] = c;
        } else if((c & 0xe0) == 0xe0) {
            out[out_index] = (c & 0x1F) << 12;
	        c = utf8[index++];
            out[out_index] |= (c & 0x3F) << 6;
	        c = utf8[index++];
            out[out_index++] |= (c & 0x3F);
        } else {
            out[out_index] = (c & 0x3F) << 6;
	        c = utf8[index++];
            out[out_index++] |= (c & 0x3F);
        }
        c = utf8[index++];
    }
    out[out_index] = 0;

    return out;
}

int utf8_encode(const char *from, char **to)
{
	wchar_t *unicode;
	int wchars, err;

	wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, from,
			strlen(from), NULL, 0);

	if(wchars == 0)
	{
		fprintf(stderr, "Unicode translation error %d\n", GetLastError());
		return -1;
	}

	unicode = calloc(wchars + 1, sizeof(unsigned short));
	if(unicode == NULL) 
	{
		fprintf(stderr, "Out of memory processing string to UTF8\n");
		return -1;
	}

	err = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, from, 
			strlen(from), unicode, wchars);
	if(err != wchars)
	{
		free(unicode);
		fprintf(stderr, "Unicode translation error %d\n", GetLastError());
		return -1;
	}

	/* On NT-based windows systems, we could use WideCharToMultiByte(), but 
	 * MS doesn't actually have a consistent API across win32.
	 */
	*to = make_utf8_string(unicode);

	free(unicode);
	return 0;
}

int utf8_decode(const char *from, char **to)
{
    wchar_t *unicode;
    int chars, err;

    /* On NT-based windows systems, we could use MultiByteToWideChar(CP_UTF8), but 
     * MS doesn't actually have a consistent API across win32.
     */
    unicode = make_unicode_string(from);
    if(unicode == NULL) 
    {
        fprintf(stderr, "Out of memory processing string from UTF8 to UNICODE16\n");
        return -1;
    }

    chars = WideCharToMultiByte(GetConsoleCP(), WC_COMPOSITECHECK, unicode,
            -1, NULL, 0, NULL, NULL);

    if(chars == 0)
    {
        fprintf(stderr, "Unicode translation error %d\n", GetLastError());
        free(unicode);
        return -1;
    }

    *to = calloc(chars + 1, sizeof(unsigned char));
    if(*to == NULL) 
    {
        fprintf(stderr, "Out of memory processing string to local charset\n");
        free(unicode);
        return -1;
    }

    err = WideCharToMultiByte(GetConsoleCP(), WC_COMPOSITECHECK, unicode, 
            -1, *to, chars, NULL, NULL);
    if(err != chars)
    {
        fprintf(stderr, "Unicode translation error %d\n", GetLastError());
        free(unicode);
        free(*to);
        *to = NULL;
        return -1;
    }

    free(unicode);
    return 0;
}

#else /* End win32. Rest is for real operating systems */


#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

int iconvert(const char *fromcode, const char *tocode,
	     const char *from, size_t fromlen,
	     char **to, size_t *tolen);

static char *current_charset = 0; /* means "US-ASCII" */

void convert_set_charset(const char *charset)
{

  if (!charset)
    charset = getenv("CHARSET");

#ifdef HAVE_LANGINFO_CODESET
  if (!charset)
    charset = nl_langinfo(CODESET);
#endif

  free(current_charset);
  current_charset = 0;
  if (charset && *charset)
    current_charset = strdup(charset);
}

static int convert_buffer(const char *fromcode, const char *tocode,
			  const char *from, size_t fromlen,
			  char **to, size_t *tolen)
{
  int ret = -1;

#ifdef HAVE_ICONV
  ret = iconvert(fromcode, tocode, from, fromlen, to, tolen);
  if (ret != -1)
    return ret;
#endif

#ifndef HAVE_ICONV /* should be ifdef USE_CHARSET_CONVERT */
  ret = charset_convert(fromcode, tocode, from, fromlen, to, tolen);
  if (ret != -1)
    return ret;
#endif

  return ret;
}

static int convert_string(const char *fromcode, const char *tocode,
			  const char *from, char **to, char replace)
{
  int ret;
  size_t fromlen;
  char *s;

  fromlen = strlen(from);
  ret = convert_buffer(fromcode, tocode, from, fromlen, to, 0);
  if (ret == -2)
    return -1;
  if (ret != -1)
    return ret;

  s = malloc(fromlen + 1);
  if (!s)
    return -1;
  strcpy(s, from);
  *to = s;
  for (; *s; s++)
    if (*s & ~0x7f)
      *s = replace;
  return 3;
}

int utf8_encode(const char *from, char **to)
{
  char *charset;

  if (!current_charset)
    convert_set_charset(0);
  charset = current_charset ? current_charset : "US-ASCII";
  return convert_string(charset, "UTF-8", from, to, '#');
}

int utf8_decode(const char *from, char **to)
{
  char *charset;

  if(*from == 0) {
      *to = malloc(1);
      **to = 0;
      return 1;
  }

  if (!current_charset)
    convert_set_charset(0);
  charset = current_charset ? current_charset : "US-ASCII";
  return convert_string("UTF-8", charset, from, to, '?');
}

#endif
