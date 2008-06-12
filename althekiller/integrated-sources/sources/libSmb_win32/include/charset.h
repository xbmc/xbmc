/* 
   Unix SMB/CIFS implementation.
   charset defines
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

/* this defines the charset types used in samba */
typedef enum {CH_UCS2=0, CH_UNIX=1, CH_DISPLAY=2, CH_DOS=3, CH_UTF8=4} charset_t;

#define NUM_CHARSETS 5

/* 
 *   for each charset we have a function that pushes from that charset to a ucs2
 *   buffer, and a function that pulls from ucs2 buffer to that  charset.
 *     */

struct charset_functions {
	const char *name;
	size_t (*pull)(void *, const char **inbuf, size_t *inbytesleft,
				   char **outbuf, size_t *outbytesleft);
	size_t (*push)(void *, const char **inbuf, size_t *inbytesleft,
				   char **outbuf, size_t *outbytesleft);
	struct charset_functions *prev, *next;
};

/*
 * This is auxiliary struct used by source/script/gen-8-bit-gap.sh script
 * during generation of an encoding table for charset module
 *     */

struct charset_gap_table {
  uint16 start;
  uint16 end;
  int32 idx;
};

/*
 *   Define stub for charset module which implements 8-bit encoding with gaps.
 *   Encoding tables for such module should be produced from glibc's CHARMAPs
 *   using script source/script/gen-8bit-gap.sh
 *   CHARSETNAME is CAPITALIZED charset name
 *
 *     */
#define SMB_GENERATE_CHARSET_MODULE_8_BIT_GAP(CHARSETNAME) 					\
static size_t CHARSETNAME ## _push(void *cd, const char **inbuf, size_t *inbytesleft,			\
			 char **outbuf, size_t *outbytesleft) 					\
{ 												\
	while (*inbytesleft >= 2 && *outbytesleft >= 1) { 					\
		int i; 										\
		int done = 0; 									\
												\
		uint16 ch = SVAL(*inbuf,0); 							\
												\
		for (i=0; from_idx[i].start != 0xffff; i++) {					\
			if ((from_idx[i].start <= ch) && (from_idx[i].end >= ch)) {		\
				((unsigned char*)(*outbuf))[0] = from_ucs2[from_idx[i].idx+ch];	\
				(*inbytesleft) -= 2;						\
				(*outbytesleft) -= 1;						\
				(*inbuf)  += 2;							\
				(*outbuf) += 1;							\
				done = 1;							\
				break;								\
			}									\
		}										\
		if (!done) {									\
			errno = EINVAL;								\
			return -1;								\
		}										\
												\
	}											\
												\
	if (*inbytesleft == 1) {								\
		errno = EINVAL;									\
		return -1;									\
	}											\
												\
	if (*inbytesleft > 1) {									\
		errno = E2BIG;									\
		return -1;									\
	}											\
												\
	return 0;										\
}												\
												\
static size_t CHARSETNAME ## _pull(void *cd, const char **inbuf, size_t *inbytesleft,				\
			 char **outbuf, size_t *outbytesleft)					\
{												\
	while (*inbytesleft >= 1 && *outbytesleft >= 2) {					\
		*(uint16*)(*outbuf) = to_ucs2[((unsigned char*)(*inbuf))[0]];			\
		(*inbytesleft)  -= 1;								\
		(*outbytesleft) -= 2;								\
		(*inbuf)  += 1;									\
		(*outbuf) += 2;									\
	}											\
												\
	if (*inbytesleft > 0) {									\
		errno = E2BIG;									\
		return -1;									\
	}											\
												\
	return 0;										\
}												\
												\
struct charset_functions CHARSETNAME ## _functions = 						\
		{#CHARSETNAME, CHARSETNAME ## _pull, CHARSETNAME ## _push};			\
												\
NTSTATUS charset_ ## CHARSETNAME ## _init(void)							\
{												\
	return smb_register_charset(& CHARSETNAME ## _functions);				\
}												\

