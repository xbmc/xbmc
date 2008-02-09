/*
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

#include <stdlib.h>

/*
 * These functions are like the C library's mbtowc() and wctomb(),
 * but instead of depending on the locale they always work in UTF-8,
 * and they use int instead of wchar_t.
 */

int utf8_mbtowc(int *pwc, const char *s, size_t n);
int utf8_wctomb(char *s, int wc);

/*
 * This is an object-oriented version of mbtowc() and wctomb().
 * The caller first uses charset_find() to get a pointer to struct
 * charset, then uses the mbtowc() and wctomb() methods on it.
 * The function charset_max() gives the maximum length of a
 * multibyte character in that encoding.
 * This API is only appropriate for stateless encodings like UTF-8
 * or ISO-8859-3, but I have no intention of implementing anything
 * other than UTF-8 and 8-bit encodings.
 *
 * MINOR BUG: If there is no memory charset_find() may return 0 and
 * there is no way to distinguish this case from an unknown encoding.
 */

struct charset;

struct charset *charset_find(const char *code);

int charset_mbtowc(struct charset *charset, int *pwc, const char *s, size_t n);
int charset_wctomb(struct charset *charset, char *s, int wc);
int charset_max(struct charset *charset);

/*
 * Function to convert a buffer from one encoding to another.
 * Invalid bytes are replaced by '#', and characters that are
 * not available in the target encoding are replaced by '?'.
 * Each of TO and TOLEN may be zero if the result is not wanted.
 * The input or output may contain null bytes, but the output
 * buffer is also null-terminated, so it is all right to
 * use charset_convert(fromcode, tocode, s, strlen(s), &t, 0).
 *
 * Return value:
 *
 *  -2 : memory allocation failed
 *  -1 : unknown encoding
 *   0 : data was converted exactly
 *   1 : valid data was converted approximately (using '?')
 *   2 : input was invalid (but still converted, using '#')
 */

int charset_convert(const char *fromcode, const char *tocode,
		    const char *from, size_t fromlen,
		    char **to, size_t *tolen);
