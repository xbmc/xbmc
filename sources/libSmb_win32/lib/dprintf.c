/* 
   Unix SMB/CIFS implementation.
   display print functions
   Copyright (C) Andrew Tridgell 2001
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


/*
  this module provides functions for printing internal strings in the "display charset"
  This charset may be quite different from the chosen unix charset

  Eventually these functions will need to take care of column count constraints

  The d_ prefix on print functions in Samba refers to the display character set
  conversion
*/

#include "includes.h"

 int d_vfprintf(FILE *f, const char *format, va_list ap)
{
	char *p, *p2;
	int ret, maxlen, clen;
	const char *msgstr;
	va_list ap2;

	/* do any message translations */
	msgstr = lang_msg(format);
	if (!msgstr) return -1;

	VA_COPY(ap2, ap);

	ret = vasprintf(&p, msgstr, ap2);

	lang_msg_free(msgstr);

	if (ret <= 0) return ret;

	/* now we have the string in unix format, convert it to the display
	   charset, but beware of it growing */
	maxlen = ret*2;
again:
	p2 = SMB_MALLOC(maxlen);
	if (!p2) {
		SAFE_FREE(p);
		return -1;
	}
	clen = convert_string(CH_UNIX, CH_DISPLAY, p, ret, p2, maxlen, True);

	if (clen >= maxlen) {
		/* it didn't fit - try a larger buffer */
		maxlen *= 2;
		SAFE_FREE(p2);
		goto again;
	}

	/* good, its converted OK */
	SAFE_FREE(p);
	ret = fwrite(p2, 1, clen, f);
	SAFE_FREE(p2);

	return ret;
}


 int d_fprintf(FILE *f, const char *format, ...)
{
	int ret;
	va_list ap;

	va_start(ap, format);
	ret = d_vfprintf(f, format, ap);
	va_end(ap);

	return ret;
}

static FILE *outfile;

 int d_printf(const char *format, ...)
{
	int ret;
	va_list ap;

	if (!outfile) outfile = stdout;
	
	va_start(ap, format);
	ret = d_vfprintf(outfile, format, ap);
	va_end(ap);

	return ret;
}

/* interactive programs need a way of tell d_*() to write to stderr instead
   of stdout */
void display_set_stderr(void)
{
	outfile = stderr;
}
