/* 
   Unix SMB/CIFS implementation.
   Samba module with developer tools
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Jelmer Vernooij 2002
   
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

#include "includes.h"

static struct {
	char from;
	const char *to;
	int len;
} weird_table[] = {
	{'q', "^q^", 3},
	{'Q', "^Q^", 3},
	{0, NULL}
};

static size_t weird_pull(void *cd, const char **inbuf, size_t *inbytesleft,
			 char **outbuf, size_t *outbytesleft)
{
	while (*inbytesleft >= 1 && *outbytesleft >= 2) {
		int i;
		int done = 0;
		for (i=0;weird_table[i].from;i++) {
			if (strncmp((*inbuf), 
				    weird_table[i].to, 
				    weird_table[i].len) == 0) {
				if (*inbytesleft < weird_table[i].len) {
					DEBUG(0,("ERROR: truncated weird string\n"));
					/* smb_panic("weird_pull"); */

				} else {
					(*outbuf)[0] = weird_table[i].from;
					(*outbuf)[1] = 0;
					(*inbytesleft)  -= weird_table[i].len;
					(*outbytesleft) -= 2;
					(*inbuf)  += weird_table[i].len;
					(*outbuf) += 2;
					done = 1;
					break;
				}
			}
		}
		if (done) continue;
		(*outbuf)[0] = (*inbuf)[0];
		(*outbuf)[1] = 0;
		(*inbytesleft)  -= 1;
		(*outbytesleft) -= 2;
		(*inbuf)  += 1;
		(*outbuf) += 2;
	}

	if (*inbytesleft > 0) {
		errno = E2BIG;
		return -1;
	}
	
	return 0;
}

static size_t weird_push(void *cd, const char **inbuf, size_t *inbytesleft,
			 char **outbuf, size_t *outbytesleft)
{
	int ir_count=0;

	while (*inbytesleft >= 2 && *outbytesleft >= 1) {
		int i;
		int done=0;
		for (i=0;weird_table[i].from;i++) {
			if ((*inbuf)[0] == weird_table[i].from &&
			    (*inbuf)[1] == 0) {
				if (*outbytesleft < weird_table[i].len) {
					DEBUG(0,("No room for weird character\n"));
					/* smb_panic("weird_push"); */
				} else {
					memcpy(*outbuf, weird_table[i].to, 
					       weird_table[i].len);
					(*inbytesleft)  -= 2;
					(*outbytesleft) -= weird_table[i].len;
					(*inbuf)  += 2;
					(*outbuf) += weird_table[i].len;
					done = 1;
					break;
				}
			}
		}
		if (done) continue;

		(*outbuf)[0] = (*inbuf)[0];
		if ((*inbuf)[1]) ir_count++;
		(*inbytesleft)  -= 2;
		(*outbytesleft) -= 1;
		(*inbuf)  += 2;
		(*outbuf) += 1;
	}

	if (*inbytesleft == 1) {
		errno = EINVAL;
		return -1;
	}

	if (*inbytesleft > 1) {
		errno = E2BIG;
		return -1;
	}
	
	return ir_count;
}

struct charset_functions weird_functions = {"WEIRD", weird_pull, weird_push};

NTSTATUS charset_weird_init(void)
{
	return smb_register_charset(&weird_functions);
}
