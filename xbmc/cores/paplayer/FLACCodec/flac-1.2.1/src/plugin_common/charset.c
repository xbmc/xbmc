/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * Only slightly modified charset.c from:
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 1999-2001  Håvard Kvålen <havardk@xmms.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "charset.h"


/*************
 * Functions *
 *************/

char* FLAC_plugin__charset_get_current (void)
{
	char *charset = getenv("CHARSET");

#ifdef HAVE_LANGINFO_CODESET
	if (!charset)
		charset = nl_langinfo(CODESET);
#endif
	if (!charset)
		charset = "ISO-8859-1";

	return charset;
}


#ifdef HAVE_ICONV
char* FLAC_plugin__charset_convert_string (const char *string, char *from, char *to)
{
	size_t outleft, outsize, length;
	iconv_t cd;
	char *out, *outptr;
	const char *input = string;

	if (!string)
		return NULL;

	length = strlen(string);

	if ((cd = iconv_open(to, from)) == (iconv_t)-1)
	{
#ifdef DEBUG
		fprintf(stderr, "convert_string(): Conversion not supported. Charsets: %s -> %s", from, to);
#endif
		return strdup(string);
	}

	/* Due to a GLIBC bug, round outbuf_size up to a multiple of 4 */
	/* + 1 for nul in case len == 1 */
	outsize = ((length + 3) & ~3) + 1;
	if(outsize < length) /* overflow check */
		return NULL;
	out = (char*)malloc(outsize);
	outleft = outsize - 1;
	outptr = out;

retry:
	if (iconv(cd, (char**)&input, &length, &outptr, &outleft) == (size_t)(-1))
	{
		int used;
		switch (errno)
		{
			case E2BIG:
				used = outptr - out;
				if((outsize - 1) * 2 + 1 <= outsize) { /* overflow check */
					free(out);
					return NULL;
				}
				outsize = (outsize - 1) * 2 + 1;
				out = realloc(out, outsize);
				outptr = out + used;
				outleft = outsize - 1 - used;
				goto retry;
			case EINVAL:
				break;
			case EILSEQ:
				/* Invalid sequence, try to get the rest of the string */
				input++;
				length = strlen(input);
				goto retry;
			default:
#ifdef DEBUG
				fprintf(stderr, "convert_string(): Conversion failed. Inputstring: %s; Error: %s", string, strerror(errno));
#endif
				break;
		}
	}
	*outptr = '\0';

	iconv_close(cd);
	return out;
}
#else
char* FLAC_plugin__charset_convert_string (const char *string, char *from, char *to)
{
	(void)from, (void)to;
	if (!string)
		return NULL;
	return strdup(string);
}
#endif

#ifdef HAVE_ICONV
int FLAC_plugin__charset_test_conversion (char *from, char *to)
{
	iconv_t cd;

	if ((cd=iconv_open(to,from)) == (iconv_t)-1)
	{
		/* Conversion not supported */
		return 0;
	}
	iconv_close(cd);
	return 1;
}
#else
int FLAC_plugin__charset_test_conversion (char *from, char *to)
{
	(void)from, (void)to;
	return 1;
}
#endif
