/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   
   Copyright (C) Andrew Tridgell 1992-2001
   Copyright (C) Simo Sorce      2001-2002
   Copyright (C) Martin Pool     2003
   
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

/**
 * @file
 * @brief String utilities.
 **/

/**
 * Get the next token from a string, return False if none found.
 * Handles double-quotes.
 * 
 * Based on a routine by GJC@VILLAGE.COM. 
 * Extensively modified by Andrew.Tridgell@anu.edu.au
 **/
BOOL next_token(const char **ptr,char *buff, const char *sep, size_t bufsize)
{
	char *s;
	char *pbuf;
	BOOL quoted;
	size_t len=1;

	if (!ptr)
		return(False);

	s = (char *)*ptr;

	/* default to simple separators */
	if (!sep)
		sep = " \t\n\r";

	/* find the first non sep char */
	while (*s && strchr_m(sep,*s))
		s++;
	
	/* nothing left? */
	if (! *s)
		return(False);
	
	/* copy over the token */
	pbuf = buff;
	for (quoted = False; len < bufsize && *s && (quoted || !strchr_m(sep,*s)); s++) {
		if ( *s == '\"' ) {
			quoted = !quoted;
		} else {
			len++;
			*pbuf++ = *s;
		}
	}
	
	*ptr = (*s) ? s+1 : s;  
	*pbuf = 0;
	
	return(True);
}

/**
This is like next_token but is not re-entrant and "remembers" the first 
parameter so you can pass NULL. This is useful for user interface code
but beware the fact that it is not re-entrant!
**/

static const char *last_ptr=NULL;

BOOL next_token_nr(const char **ptr,char *buff, const char *sep, size_t bufsize)
{
	BOOL ret;
	if (!ptr)
		ptr = &last_ptr;

	ret = next_token(ptr, buff, sep, bufsize);
	last_ptr = *ptr;
	return ret;	
}

static uint16 tmpbuf[sizeof(pstring)];

void set_first_token(char *ptr)
{
	last_ptr = ptr;
}

/**
 Convert list of tokens to array; dependent on above routine.
 Uses last_ptr from above - bit of a hack.
**/

char **toktocliplist(int *ctok, const char *sep)
{
	char *s=(char *)last_ptr;
	int ictok=0;
	char **ret, **iret;

	if (!sep)
		sep = " \t\n\r";

	while(*s && strchr_m(sep,*s))
		s++;

	/* nothing left? */
	if (!*s)
		return(NULL);

	do {
		ictok++;
		while(*s && (!strchr_m(sep,*s)))
			s++;
		while(*s && strchr_m(sep,*s))
			*s++=0;
	} while(*s);
	
	*ctok=ictok;
	s=(char *)last_ptr;
	
	if (!(ret=iret=SMB_MALLOC_ARRAY(char *,ictok+1)))
		return NULL;
	
	while(ictok--) {    
		*iret++=s;
		if (ictok > 0) {
			while(*s++)
				;
			while(!*s)
				s++;
		}
	}

	ret[*ctok] = NULL;
	return ret;
}

/**
 * Case insensitive string compararison.
 *
 * iconv does not directly give us a way to compare strings in
 * arbitrary unix character sets -- all we can is convert and then
 * compare.  This is expensive.
 *
 * As an optimization, we do a first pass that considers only the
 * prefix of the strings that is entirely 7-bit.  Within this, we
 * check whether they have the same value.
 *
 * Hopefully this will often give the answer without needing to copy.
 * In particular it should speed comparisons to literal ascii strings
 * or comparisons of strings that are "obviously" different.
 *
 * If we find a non-ascii character we fall back to converting via
 * iconv.
 *
 * This should never be slower than convering the whole thing, and
 * often faster.
 *
 * A different optimization would be to compare for bitwise equality
 * in the binary encoding.  (It would be possible thought hairy to do
 * both simultaneously.)  But in that case if they turn out to be
 * different, we'd need to restart the whole thing.
 *
 * Even better is to implement strcasecmp for each encoding and use a
 * function pointer. 
 **/
int StrCaseCmp(const char *s, const char *t)
{

	const char *ps, *pt;
	size_t size;
	smb_ucs2_t *buffer_s, *buffer_t;
	int ret;

	for (ps = s, pt = t; ; ps++, pt++) {
		char us, ut;

		if (!*ps && !*pt)
			return 0; /* both ended */
 		else if (!*ps)
			return -1; /* s is a prefix */
		else if (!*pt)
			return +1; /* t is a prefix */
		else if ((*ps & 0x80) || (*pt & 0x80))
			/* not ascii anymore, do it the hard way from here on in */
			break;

		us = toupper_ascii(*ps);
		ut = toupper_ascii(*pt);
		if (us == ut)
			continue;
		else if (us < ut)
			return -1;
		else if (us > ut)
			return +1;
	}

	size = push_ucs2_allocate(&buffer_s, ps);
	if (size == (size_t)-1) {
		return strcmp(ps, pt); 
		/* Not quite the right answer, but finding the right one
		   under this failure case is expensive, and it's pretty close */
	}
	
	size = push_ucs2_allocate(&buffer_t, pt);
	if (size == (size_t)-1) {
		SAFE_FREE(buffer_s);
		return strcmp(ps, pt); 
		/* Not quite the right answer, but finding the right one
		   under this failure case is expensive, and it's pretty close */
	}
	
	ret = strcasecmp_w(buffer_s, buffer_t);
	SAFE_FREE(buffer_s);
	SAFE_FREE(buffer_t);
	return ret;
}


/**
 Case insensitive string compararison, length limited.
**/
int StrnCaseCmp(const char *s, const char *t, size_t n)
{
	pstring buf1, buf2;
	unix_strupper(s, strlen(s)+1, buf1, sizeof(buf1));
	unix_strupper(t, strlen(t)+1, buf2, sizeof(buf2));
	return strncmp(buf1,buf2,n);
}

/**
 * Compare 2 strings.
 *
 * @note The comparison is case-insensitive.
 **/
BOOL strequal(const char *s1, const char *s2)
{
	if (s1 == s2)
		return(True);
	if (!s1 || !s2)
		return(False);
  
	return(StrCaseCmp(s1,s2)==0);
}

/**
 * Compare 2 strings up to and including the nth char.
 *
 * @note The comparison is case-insensitive.
 **/
BOOL strnequal(const char *s1,const char *s2,size_t n)
{
	if (s1 == s2)
		return(True);
	if (!s1 || !s2 || !n)
		return(False);
  
	return(StrnCaseCmp(s1,s2,n)==0);
}

/**
 Compare 2 strings (case sensitive).
**/

BOOL strcsequal(const char *s1,const char *s2)
{
	if (s1 == s2)
		return(True);
	if (!s1 || !s2)
		return(False);
  
	return(strcmp(s1,s2)==0);
}

/**
Do a case-insensitive, whitespace-ignoring string compare.
**/

int strwicmp(const char *psz1, const char *psz2)
{
	/* if BOTH strings are NULL, return TRUE, if ONE is NULL return */
	/* appropriate value. */
	if (psz1 == psz2)
		return (0);
	else if (psz1 == NULL)
		return (-1);
	else if (psz2 == NULL)
		return (1);

	/* sync the strings on first non-whitespace */
	while (1) {
		while (isspace((int)*psz1))
			psz1++;
		while (isspace((int)*psz2))
			psz2++;
		if (toupper_ascii(*psz1) != toupper_ascii(*psz2) || *psz1 == '\0'
		    || *psz2 == '\0')
			break;
		psz1++;
		psz2++;
	}
	return (*psz1 - *psz2);
}


/**
 Convert a string to upper case, but don't modify it.
**/

char *strupper_static(const char *s)
{
	static pstring str;

	pstrcpy(str, s);
	strupper_m(str);

	return str;
}

/**
 Convert a string to "normal" form.
**/

void strnorm(char *s, int case_default)
{
	if (case_default == CASE_UPPER)
		strupper_m(s);
	else
		strlower_m(s);
}

/**
 Check if a string is in "normal" case.
**/

BOOL strisnormal(const char *s, int case_default)
{
	if (case_default == CASE_UPPER)
		return(!strhaslower(s));
	
	return(!strhasupper(s));
}


/**
 String replace.
 NOTE: oldc and newc must be 7 bit characters
**/

void string_replace( pstring s, char oldc, char newc )
{
	char *p;

	/* this is quite a common operation, so we want it to be
	   fast. We optimise for the ascii case, knowing that all our
	   supported multi-byte character sets are ascii-compatible
	   (ie. they match for the first 128 chars) */

	for (p = s; *p; p++) {
		if (*p & 0x80) /* mb string - slow path. */
			break;
		if (*p == oldc)
			*p = newc;
	}

	if (!*p)
		return;

	/* Slow (mb) path. */
#ifdef BROKEN_UNICODE_COMPOSE_CHARACTERS
	/* With compose characters we must restart from the beginning. JRA. */
	p = s;
#endif
	push_ucs2(NULL, tmpbuf, p, sizeof(tmpbuf), STR_TERMINATE);
	string_replace_w(tmpbuf, UCS2_CHAR(oldc), UCS2_CHAR(newc));
	pull_ucs2(NULL, p, tmpbuf, -1, sizeof(tmpbuf), STR_TERMINATE);
}

/**
 Skip past some strings in a buffer.
**/

char *skip_string(char *buf,size_t n)
{
	while (n--)
		buf += strlen(buf) + 1;
	return(buf);
}

/**
 Count the number of characters in a string. Normally this will
 be the same as the number of bytes in a string for single byte strings,
 but will be different for multibyte.
**/

size_t str_charnum(const char *s)
{
	uint16 tmpbuf2[sizeof(pstring)];
	push_ucs2(NULL, tmpbuf2,s, sizeof(tmpbuf2), STR_TERMINATE);
	return strlen_w(tmpbuf2);
}

/**
 Count the number of characters in a string. Normally this will
 be the same as the number of bytes in a string for single byte strings,
 but will be different for multibyte.
**/

size_t str_ascii_charnum(const char *s)
{
	pstring tmpbuf2;
	push_ascii(tmpbuf2, s, sizeof(tmpbuf2), STR_TERMINATE);
	return strlen(tmpbuf2);
}

BOOL trim_char(char *s,char cfront,char cback)
{
	BOOL ret = False;
	char *ep;
	char *fp = s;

	/* Ignore null or empty strings. */
	if (!s || (s[0] == '\0'))
		return False;

	if (cfront) {
		while (*fp && *fp == cfront)
			fp++;
		if (!*fp) {
			/* We ate the string. */
			s[0] = '\0';
			return True;
		}
		if (fp != s)
			ret = True;
	}

	ep = fp + strlen(fp) - 1;
	if (cback) {
		/* Attempt ascii only. Bail for mb strings. */
		while ((ep >= fp) && (*ep == cback)) {
			ret = True;
			if ((ep > fp) && (((unsigned char)ep[-1]) & 0x80)) {
				/* Could be mb... bail back to tim_string. */
				char fs[2], bs[2];
				if (cfront) {
					fs[0] = cfront;
					fs[1] = '\0';
				}
				bs[0] = cback;
				bs[1] = '\0';
				return trim_string(s, cfront ? fs : NULL, bs);
			} else {
				ep--;
			}
		}
		if (ep < fp) {
			/* We ate the string. */
			s[0] = '\0';
			return True;
		}
	}

	ep[1] = '\0';
	memmove(s, fp, ep-fp+2);
	return ret;
}

/**
 Trim the specified elements off the front and back of a string.
**/

BOOL trim_string(char *s,const char *front,const char *back)
{
	BOOL ret = False;
	size_t front_len;
	size_t back_len;
	size_t len;

	/* Ignore null or empty strings. */
	if (!s || (s[0] == '\0'))
		return False;

	front_len	= front? strlen(front) : 0;
	back_len	= back? strlen(back) : 0;

	len = strlen(s);

	if (front_len) {
		while (len && strncmp(s, front, front_len)==0) {
			/* Must use memmove here as src & dest can
			 * easily overlap. Found by valgrind. JRA. */
			memmove(s, s+front_len, (len-front_len)+1);
			len -= front_len;
			ret=True;
		}
	}
	
	if (back_len) {
		while ((len >= back_len) && strncmp(s+len-back_len,back,back_len)==0) {
			s[len-back_len]='\0';
			len -= back_len;
			ret=True;
		}
	}
	return ret;
}

/**
 Does a string have any uppercase chars in it?
**/

BOOL strhasupper(const char *s)
{
	smb_ucs2_t *ptr;
	push_ucs2(NULL, tmpbuf,s, sizeof(tmpbuf), STR_TERMINATE);
	for(ptr=tmpbuf;*ptr;ptr++)
		if(isupper_w(*ptr))
			return True;
	return(False);
}

/**
 Does a string have any lowercase chars in it?
**/

BOOL strhaslower(const char *s)
{
	smb_ucs2_t *ptr;
	push_ucs2(NULL, tmpbuf,s, sizeof(tmpbuf), STR_TERMINATE);
	for(ptr=tmpbuf;*ptr;ptr++)
		if(islower_w(*ptr))
			return True;
	return(False);
}

/**
 Find the number of 'c' chars in a string
**/

size_t count_chars(const char *s,char c)
{
	smb_ucs2_t *ptr;
	int count;
	smb_ucs2_t *alloc_tmpbuf = NULL;

	if (push_ucs2_allocate(&alloc_tmpbuf, s) == (size_t)-1) {
		return 0;
	}

	for(count=0,ptr=alloc_tmpbuf;*ptr;ptr++)
		if(*ptr==UCS2_CHAR(c))
			count++;

	SAFE_FREE(alloc_tmpbuf);
	return(count);
}

/**
 Safe string copy into a known length string. maxlength does not
 include the terminating zero.
**/

char *safe_strcpy_fn(const char *fn, int line, char *dest,const char *src, size_t maxlength)
{
	size_t len;

	if (!dest) {
		DEBUG(0,("ERROR: NULL dest in safe_strcpy, called from [%s][%d]\n", fn, line));
		return NULL;
	}

#ifdef DEVELOPER
	clobber_region(fn,line,dest, maxlength+1);
#endif

	if (!src) {
		*dest = 0;
		return dest;
	}  

	len = strnlen(src, maxlength+1);

	if (len > maxlength) {
		DEBUG(0,("ERROR: string overflow by %lu (%lu - %lu) in safe_strcpy [%.50s]\n",
			 (unsigned long)(len-maxlength), (unsigned long)len, 
			 (unsigned long)maxlength, src));
		len = maxlength;
	}
      
	memmove(dest, src, len);
	dest[len] = 0;
	return dest;
}  

/**
 Safe string cat into a string. maxlength does not
 include the terminating zero.
**/
char *safe_strcat_fn(const char *fn, int line, char *dest, const char *src, size_t maxlength)
{
	size_t src_len, dest_len;

	if (!dest) {
		DEBUG(0,("ERROR: NULL dest in safe_strcat, called from [%s][%d]\n", fn, line));
		return NULL;
	}

	if (!src)
		return dest;
	
	src_len = strnlen(src, maxlength + 1);
	dest_len = strnlen(dest, maxlength + 1);

#ifdef DEVELOPER
	clobber_region(fn, line, dest + dest_len, maxlength + 1 - dest_len);
#endif

	if (src_len + dest_len > maxlength) {
		DEBUG(0,("ERROR: string overflow by %d in safe_strcat [%.50s]\n",
			 (int)(src_len + dest_len - maxlength), src));
		if (maxlength > dest_len) {
			memcpy(&dest[dest_len], src, maxlength - dest_len);
		}
		dest[maxlength] = 0;
		return NULL;
	}

	memcpy(&dest[dest_len], src, src_len);
	dest[dest_len + src_len] = 0;
	return dest;
}

/**
 Paranoid strcpy into a buffer of given length (includes terminating
 zero. Strips out all but 'a-Z0-9' and the character in other_safe_chars
 and replaces with '_'. Deliberately does *NOT* check for multibyte
 characters. Don't change it !
**/
char *alpha_strcpy_fn(const char *fn, int line, char *dest, const char *src, const char *other_safe_chars, size_t maxlength)
{
	size_t len, i;

#ifdef DEVELOPER
	clobber_region(fn, line, dest, maxlength);
#endif

	if (!dest) {
		DEBUG(0,("ERROR: NULL dest in alpha_strcpy, called from [%s][%d]\n", fn, line));
		return NULL;
	}

	if (!src) {
		*dest = 0;
		return dest;
	}  

	len = strlen(src);
	if (len >= maxlength)
		len = maxlength - 1;

	if (!other_safe_chars)
		other_safe_chars = "";

	for(i = 0; i < len; i++) {
		int val = (src[i] & 0xff);
		if (isupper_ascii(val) || islower_ascii(val) || isdigit(val) || strchr_m(other_safe_chars, val))
			dest[i] = src[i];
		else
			dest[i] = '_';
	}

	dest[i] = '\0';

	return dest;
}

/**
 Like strncpy but always null terminates. Make sure there is room!
 The variable n should always be one less than the available size.
**/
char *StrnCpy_fn(const char *fn, int line,char *dest,const char *src,size_t n)
{
	char *d = dest;

#ifdef DEVELOPER
	clobber_region(fn, line, dest, n+1);
#endif

	if (!dest) {
		DEBUG(0,("ERROR: NULL dest in StrnCpy, called from [%s][%d]\n", fn, line));
		return(NULL);
	}

	if (!src) {
		*dest = 0;
		return(dest);
	}
	
	while (n-- && (*d = *src)) {
		d++;
		src++;
	}

	*d = 0;
	return(dest);
}

#if 0
/**
 Like strncpy but copies up to the character marker.  always null terminates.
 returns a pointer to the character marker in the source string (src).
**/

static char *strncpyn(char *dest, const char *src, size_t n, char c)
{
	char *p;
	size_t str_len;

#ifdef DEVELOPER
	clobber_region(dest, n+1);
#endif
	p = strchr_m(src, c);
	if (p == NULL) {
		DEBUG(5, ("strncpyn: separator character (%c) not found\n", c));
		return NULL;
	}

	str_len = PTR_DIFF(p, src);
	strncpy(dest, src, MIN(n, str_len));
	dest[str_len] = '\0';

	return p;
}
#endif

/**
 Routine to get hex characters and turn them into a 16 byte array.
 the array can be variable length, and any non-hex-numeric
 characters are skipped.  "0xnn" or "0Xnn" is specially catered
 for.

 valid examples: "0A5D15"; "0x15, 0x49, 0xa2"; "59\ta9\te3\n"

**/

size_t strhex_to_str(char *p, size_t len, const char *strhex)
{
	size_t i;
	size_t num_chars = 0;
	unsigned char   lonybble, hinybble;
	const char     *hexchars = "0123456789ABCDEF";
	char           *p1 = NULL, *p2 = NULL;

	for (i = 0; i < len && strhex[i] != 0; i++) {
		if (strnequal(hexchars, "0x", 2)) {
			i++; /* skip two chars */
			continue;
		}

		if (!(p1 = strchr_m(hexchars, toupper_ascii(strhex[i]))))
			break;

		i++; /* next hex digit */

		if (!(p2 = strchr_m(hexchars, toupper_ascii(strhex[i]))))
			break;

		/* get the two nybbles */
		hinybble = PTR_DIFF(p1, hexchars);
		lonybble = PTR_DIFF(p2, hexchars);

		p[num_chars] = (hinybble << 4) | lonybble;
		num_chars++;

		p1 = NULL;
		p2 = NULL;
	}
	return num_chars;
}

DATA_BLOB strhex_to_data_blob(TALLOC_CTX *mem_ctx, const char *strhex) 
{
	DATA_BLOB ret_blob;

	if (mem_ctx != NULL)
		ret_blob = data_blob_talloc(mem_ctx, NULL, strlen(strhex)/2+1);
	else
		ret_blob = data_blob(NULL, strlen(strhex)/2+1);

	ret_blob.length = strhex_to_str((char*)ret_blob.data, 	
					strlen(strhex), 
					strhex);

	return ret_blob;
}

/**
 * Routine to print a buffer as HEX digits, into an allocated string.
 */

char *hex_encode(TALLOC_CTX *mem_ctx, const unsigned char *buff_in, size_t len)
{
	int i;
	char *hex_buffer;

	hex_buffer = TALLOC_ARRAY(mem_ctx, char, (len*2)+1);

	for (i = 0; i < len; i++)
		slprintf(&hex_buffer[i*2], 3, "%02X", buff_in[i]);

	return hex_buffer;
}

/**
 Check if a string is part of a list.
**/

BOOL in_list(const char *s, const char *list, BOOL casesensitive)
{
	pstring tok;
	const char *p=list;

	if (!list)
		return(False);

	while (next_token(&p,tok,LIST_SEP,sizeof(tok))) {
		if (casesensitive) {
			if (strcmp(tok,s) == 0)
				return(True);
		} else {
			if (StrCaseCmp(tok,s) == 0)
				return(True);
		}
	}
	return(False);
}

/* this is used to prevent lots of mallocs of size 1 */
static const char *null_string = "";

/**
 Set a string value, allocing the space for the string
**/

static BOOL string_init(char **dest,const char *src)
{
	size_t l;

	if (!src)     
		src = "";

	l = strlen(src);

	if (l == 0) {
		*dest = CONST_DISCARD(char*, null_string);
	} else {
		(*dest) = SMB_STRDUP(src);
		if ((*dest) == NULL) {
			DEBUG(0,("Out of memory in string_init\n"));
			return False;
		}
	}
	return(True);
}

/**
 Free a string value.
**/

void string_free(char **s)
{
	if (!s || !(*s))
		return;
	if (*s == null_string)
		*s = NULL;
	SAFE_FREE(*s);
}

/**
 Set a string value, deallocating any existing space, and allocing the space
 for the string
**/

BOOL string_set(char **dest,const char *src)
{
	string_free(dest);
	return(string_init(dest,src));
}

/**
 Substitute a string for a pattern in another string. Make sure there is 
 enough room!

 This routine looks for pattern in s and replaces it with 
 insert. It may do multiple replacements or just one.

 Any of " ; ' $ or ` in the insert string are replaced with _
 if len==0 then the string cannot be extended. This is different from the old
 use of len==0 which was for no length checks to be done.
**/

void string_sub2(char *s,const char *pattern, const char *insert, size_t len, 
		 BOOL remove_unsafe_characters, BOOL replace_once, BOOL allow_trailing_dollar)
{
	char *p;
	ssize_t ls,lp,li, i;

	if (!insert || !pattern || !*pattern || !s)
		return;

	ls = (ssize_t)strlen(s);
	lp = (ssize_t)strlen(pattern);
	li = (ssize_t)strlen(insert);

	if (len == 0)
		len = ls + 1; /* len is number of *bytes* */

	while (lp <= ls && (p = strstr_m(s,pattern))) {
		if (ls + (li-lp) >= len) {
			DEBUG(0,("ERROR: string overflow by %d in string_sub(%.50s, %d)\n", 
				 (int)(ls + (li-lp) - len),
				 pattern, (int)len));
			break;
		}
		if (li != lp) {
			memmove(p+li,p+lp,strlen(p+lp)+1);
		}
		for (i=0;i<li;i++) {
			switch (insert[i]) {
			case '`':
			case '"':
			case '\'':
			case ';':
			case '$':
				/* allow a trailing $ (as in machine accounts) */
				if (allow_trailing_dollar && (i == li - 1 )) {
					p[i] = insert[i];
					break;
				}
			case '%':
			case '\r':
			case '\n':
				if ( remove_unsafe_characters ) {
					p[i] = '_';
					/* yes this break should be here since we want to 
					   fall throw if not replacing unsafe chars */
					break;
				}
			default:
				p[i] = insert[i];
			}
		}
		s = p + li;
		ls += (li-lp);

		if (replace_once)
			break;
	}
}

void string_sub_once(char *s, const char *pattern, const char *insert, size_t len)
{
	string_sub2( s, pattern, insert, len, True, True, False );
}

void string_sub(char *s,const char *pattern, const char *insert, size_t len)
{
	string_sub2( s, pattern, insert, len, True, False, False );
}

void fstring_sub(char *s,const char *pattern,const char *insert)
{
	string_sub(s, pattern, insert, sizeof(fstring));
}

void pstring_sub(char *s,const char *pattern,const char *insert)
{
	string_sub(s, pattern, insert, sizeof(pstring));
}

/**
 Similar to string_sub, but it will accept only allocated strings
 and may realloc them so pay attention at what you pass on no
 pointers inside strings, no pstrings or const may be passed
 as string.
**/

char *realloc_string_sub(char *string, const char *pattern,
			 const char *insert)
{
	char *p, *in;
	char *s;
	ssize_t ls,lp,li,ld, i;

	if (!insert || !pattern || !*pattern || !string || !*string)
		return NULL;

	s = string;

	in = SMB_STRDUP(insert);
	if (!in) {
		DEBUG(0, ("realloc_string_sub: out of memory!\n"));
		return NULL;
	}
	ls = (ssize_t)strlen(s);
	lp = (ssize_t)strlen(pattern);
	li = (ssize_t)strlen(insert);
	ld = li - lp;
	for (i=0;i<li;i++) {
		switch (in[i]) {
			case '`':
			case '"':
			case '\'':
			case ';':
			case '$':
			case '%':
			case '\r':
			case '\n':
				in[i] = '_';
			default:
				/* ok */
				break;
		}
	}
	
	while ((p = strstr_m(s,pattern))) {
		if (ld > 0) {
			int offset = PTR_DIFF(s,string);
			string = SMB_REALLOC(string, ls + ld + 1);
			if (!string) {
				DEBUG(0, ("realloc_string_sub: out of memory!\n"));
				SAFE_FREE(in);
				return NULL;
			}
			p = string + offset + (p - s);
		}
		if (li != lp) {
			memmove(p+li,p+lp,strlen(p+lp)+1);
		}
		memcpy(p, in, li);
		s = p + li;
		ls += ld;
	}
	SAFE_FREE(in);
	return string;
}

/* Same as string_sub, but returns a talloc'ed string */

char *talloc_string_sub(TALLOC_CTX *mem_ctx, const char *src,
			const char *pattern, const char *insert)
{
	char *p, *in;
	char *s;
	char *string;
	ssize_t ls,lp,li,ld, i;

	if (!insert || !pattern || !*pattern || !src || !*src)
		return NULL;

	string = talloc_strdup(mem_ctx, src);
	if (string == NULL) {
		DEBUG(0, ("talloc_strdup failed\n"));
		return NULL;
	}

	s = string;

	in = SMB_STRDUP(insert);
	if (!in) {
		DEBUG(0, ("talloc_string_sub: out of memory!\n"));
		return NULL;
	}
	ls = (ssize_t)strlen(s);
	lp = (ssize_t)strlen(pattern);
	li = (ssize_t)strlen(insert);
	ld = li - lp;
	for (i=0;i<li;i++) {
		switch (in[i]) {
			case '`':
			case '"':
			case '\'':
			case ';':
			case '$':
			case '%':
			case '\r':
			case '\n':
				in[i] = '_';
			default:
				/* ok */
				break;
		}
	}
	
	while ((p = strstr_m(s,pattern))) {
		if (ld > 0) {
			int offset = PTR_DIFF(s,string);
			string = TALLOC_REALLOC(mem_ctx, string, ls + ld + 1);
			if (!string) {
				DEBUG(0, ("talloc_string_sub: out of "
					  "memory!\n"));
				SAFE_FREE(in);
				return NULL;
			}
			p = string + offset + (p - s);
		}
		if (li != lp) {
			memmove(p+li,p+lp,strlen(p+lp)+1);
		}
		memcpy(p, in, li);
		s = p + li;
		ls += ld;
	}
	SAFE_FREE(in);
	return string;
}

/**
 Similar to string_sub() but allows for any character to be substituted. 
 Use with caution!
 if len==0 then the string cannot be extended. This is different from the old
 use of len==0 which was for no length checks to be done.
**/

void all_string_sub(char *s,const char *pattern,const char *insert, size_t len)
{
	char *p;
	ssize_t ls,lp,li;

	if (!insert || !pattern || !s)
		return;

	ls = (ssize_t)strlen(s);
	lp = (ssize_t)strlen(pattern);
	li = (ssize_t)strlen(insert);

	if (!*pattern)
		return;
	
	if (len == 0)
		len = ls + 1; /* len is number of *bytes* */
	
	while (lp <= ls && (p = strstr_m(s,pattern))) {
		if (ls + (li-lp) >= len) {
			DEBUG(0,("ERROR: string overflow by %d in all_string_sub(%.50s, %d)\n", 
				 (int)(ls + (li-lp) - len),
				 pattern, (int)len));
			break;
		}
		if (li != lp) {
			memmove(p+li,p+lp,strlen(p+lp)+1);
		}
		memcpy(p, insert, li);
		s = p + li;
		ls += (li-lp);
	}
}

/**
 Similar to all_string_sub but for unicode strings.
 Return a new allocated unicode string.
 similar to string_sub() but allows for any character to be substituted.
 Use with caution!
**/

static smb_ucs2_t *all_string_sub_w(const smb_ucs2_t *s, const smb_ucs2_t *pattern,
				const smb_ucs2_t *insert)
{
	smb_ucs2_t *r, *rp;
	const smb_ucs2_t *sp;
	size_t	lr, lp, li, lt;

	if (!insert || !pattern || !*pattern || !s)
		return NULL;

	lt = (size_t)strlen_w(s);
	lp = (size_t)strlen_w(pattern);
	li = (size_t)strlen_w(insert);

	if (li > lp) {
		const smb_ucs2_t *st = s;
		int ld = li - lp;
		while ((sp = strstr_w(st, pattern))) {
			st = sp + lp;
			lt += ld;
		}
	}

	r = rp = SMB_MALLOC_ARRAY(smb_ucs2_t, lt + 1);
	if (!r) {
		DEBUG(0, ("all_string_sub_w: out of memory!\n"));
		return NULL;
	}

	while ((sp = strstr_w(s, pattern))) {
		memcpy(rp, s, (sp - s));
		rp += ((sp - s) / sizeof(smb_ucs2_t));
		memcpy(rp, insert, (li * sizeof(smb_ucs2_t)));
		s = sp + lp;
		rp += li;
	}
	lr = ((rp - r) / sizeof(smb_ucs2_t));
	if (lr < lt) {
		memcpy(rp, s, ((lt - lr) * sizeof(smb_ucs2_t)));
		rp += (lt - lr);
	}
	*rp = 0;

	return r;
}

smb_ucs2_t *all_string_sub_wa(smb_ucs2_t *s, const char *pattern,
					     const char *insert)
{
	wpstring p, i;

	if (!insert || !pattern || !s)
		return NULL;
	push_ucs2(NULL, p, pattern, sizeof(wpstring) - 1, STR_TERMINATE);
	push_ucs2(NULL, i, insert, sizeof(wpstring) - 1, STR_TERMINATE);
	return all_string_sub_w(s, p, i);
}

#if 0
/**
 Splits out the front and back at a separator.
**/

static void split_at_last_component(char *path, char *front, char sep, char *back)
{
	char *p = strrchr_m(path, sep);

	if (p != NULL)
		*p = 0;

	if (front != NULL)
		pstrcpy(front, path);

	if (p != NULL) {
		if (back != NULL)
			pstrcpy(back, p+1);
		*p = '\\';
	} else {
		if (back != NULL)
			back[0] = 0;
	}
}
#endif

/**
 Write an octal as a string.
**/

const char *octal_string(int i)
{
	static char ret[64];
	if (i == -1)
		return "-1";
	slprintf(ret, sizeof(ret)-1, "0%o", i);
	return ret;
}


/**
 Truncate a string at a specified length.
**/

char *string_truncate(char *s, unsigned int length)
{
	if (s && strlen(s) > length)
		s[length] = 0;
	return s;
}

/**
 Strchr and strrchr_m are very hard to do on general multi-byte strings. 
 We convert via ucs2 for now.
**/

char *strchr_m(const char *src, char c)
{
	wpstring ws;
	pstring s2;
	smb_ucs2_t *p;
	const char *s;

	/* characters below 0x3F are guaranteed to not appear in
	   non-initial position in multi-byte charsets */
	if ((c & 0xC0) == 0) {
		return strchr(src, c);
	}

	/* this is quite a common operation, so we want it to be
	   fast. We optimise for the ascii case, knowing that all our
	   supported multi-byte character sets are ascii-compatible
	   (ie. they match for the first 128 chars) */

	for (s = src; *s && !(((unsigned char)s[0]) & 0x80); s++) {
		if (*s == c)
			return (char *)s;
	}

	if (!*s)
		return NULL;

#ifdef BROKEN_UNICODE_COMPOSE_CHARACTERS
	/* With compose characters we must restart from the beginning. JRA. */
	s = src;
#endif

	push_ucs2(NULL, ws, s, sizeof(ws), STR_TERMINATE);
	p = strchr_w(ws, UCS2_CHAR(c));
	if (!p)
		return NULL;
	*p = 0;
	pull_ucs2_pstring(s2, ws);
	return (char *)(s+strlen(s2));
}

char *strrchr_m(const char *s, char c)
{
	/* characters below 0x3F are guaranteed to not appear in
	   non-initial position in multi-byte charsets */
	if ((c & 0xC0) == 0) {
		return strrchr(s, c);
	}

	/* this is quite a common operation, so we want it to be
	   fast. We optimise for the ascii case, knowing that all our
	   supported multi-byte character sets are ascii-compatible
	   (ie. they match for the first 128 chars). Also, in Samba
	   we only search for ascii characters in 'c' and that
	   in all mb character sets with a compound character
	   containing c, if 'c' is not a match at position
	   p, then p[-1] > 0x7f. JRA. */

	{
		size_t len = strlen(s);
		const char *cp = s;
		BOOL got_mb = False;

		if (len == 0)
			return NULL;
		cp += (len - 1);
		do {
			if (c == *cp) {
				/* Could be a match. Part of a multibyte ? */
			       	if ((cp > s) && (((unsigned char)cp[-1]) & 0x80)) {
					/* Yep - go slow :-( */
					got_mb = True;
					break;
				}
				/* No - we have a match ! */
			       	return (char *)cp;
			}
		} while (cp-- != s);
		if (!got_mb)
			return NULL;
	}

	/* String contained a non-ascii char. Slow path. */
	{
		wpstring ws;
		pstring s2;
		smb_ucs2_t *p;

		push_ucs2(NULL, ws, s, sizeof(ws), STR_TERMINATE);
		p = strrchr_w(ws, UCS2_CHAR(c));
		if (!p)
			return NULL;
		*p = 0;
		pull_ucs2_pstring(s2, ws);
		return (char *)(s+strlen(s2));
	}
}

/***********************************************************************
 Return the equivalent of doing strrchr 'n' times - always going
 backwards.
***********************************************************************/

char *strnrchr_m(const char *s, char c, unsigned int n)
{
	wpstring ws;
	pstring s2;
	smb_ucs2_t *p;

	push_ucs2(NULL, ws, s, sizeof(ws), STR_TERMINATE);
	p = strnrchr_w(ws, UCS2_CHAR(c), n);
	if (!p)
		return NULL;
	*p = 0;
	pull_ucs2_pstring(s2, ws);
	return (char *)(s+strlen(s2));
}

/***********************************************************************
 strstr_m - We convert via ucs2 for now.
***********************************************************************/

char *strstr_m(const char *src, const char *findstr)
{
	smb_ucs2_t *p;
	smb_ucs2_t *src_w, *find_w;
	const char *s;
	char *s2;
	char *retp;

	size_t findstr_len = 0;

	/* for correctness */
	if (!findstr[0]) {
		return (char*)src;
	}

	/* Samba does single character findstr calls a *lot*. */
	if (findstr[1] == '\0')
		return strchr_m(src, *findstr);

	/* We optimise for the ascii case, knowing that all our
	   supported multi-byte character sets are ascii-compatible
	   (ie. they match for the first 128 chars) */

	for (s = src; *s && !(((unsigned char)s[0]) & 0x80); s++) {
		if (*s == *findstr) {
			if (!findstr_len) 
				findstr_len = strlen(findstr);

			if (strncmp(s, findstr, findstr_len) == 0) {
				return (char *)s;
			}
		}
	}

	if (!*s)
		return NULL;

#if 1 /* def BROKEN_UNICODE_COMPOSE_CHARACTERS */
	/* 'make check' fails unless we do this */

	/* With compose characters we must restart from the beginning. JRA. */
	s = src;
#endif

	if (push_ucs2_allocate(&src_w, src) == (size_t)-1) {
		DEBUG(0,("strstr_m: src malloc fail\n"));
		return NULL;
	}
	
	if (push_ucs2_allocate(&find_w, findstr) == (size_t)-1) {
		SAFE_FREE(src_w);
		DEBUG(0,("strstr_m: find malloc fail\n"));
		return NULL;
	}

	p = strstr_w(src_w, find_w);

	if (!p) {
		SAFE_FREE(src_w);
		SAFE_FREE(find_w);
		return NULL;
	}
	
	*p = 0;
	if (pull_ucs2_allocate(&s2, src_w) == (size_t)-1) {
		SAFE_FREE(src_w);
		SAFE_FREE(find_w);
		DEBUG(0,("strstr_m: dest malloc fail\n"));
		return NULL;
	}
	retp = (char *)(s+strlen(s2));
	SAFE_FREE(src_w);
	SAFE_FREE(find_w);
	SAFE_FREE(s2);
	return retp;
}

/**
 Convert a string to lower case.
**/

void strlower_m(char *s)
{
	size_t len;
	int errno_save;

	/* this is quite a common operation, so we want it to be
	   fast. We optimise for the ascii case, knowing that all our
	   supported multi-byte character sets are ascii-compatible
	   (ie. they match for the first 128 chars) */

	while (*s && !(((unsigned char)s[0]) & 0x80)) {
		*s = tolower_ascii((unsigned char)*s);
		s++;
	}

	if (!*s)
		return;

	/* I assume that lowercased string takes the same number of bytes
	 * as source string even in UTF-8 encoding. (VIV) */
	len = strlen(s) + 1;
	errno_save = errno;
	errno = 0;
	unix_strlower(s,len,s,len);	
	/* Catch mb conversion errors that may not terminate. */
	if (errno)
		s[len-1] = '\0';
	errno = errno_save;
}

/**
 Convert a string to upper case.
**/

void strupper_m(char *s)
{
	size_t len;
	int errno_save;

	/* this is quite a common operation, so we want it to be
	   fast. We optimise for the ascii case, knowing that all our
	   supported multi-byte character sets are ascii-compatible
	   (ie. they match for the first 128 chars) */

	while (*s && !(((unsigned char)s[0]) & 0x80)) {
		*s = toupper_ascii((unsigned char)*s);
		s++;
	}

	if (!*s)
		return;

	/* I assume that lowercased string takes the same number of bytes
	 * as source string even in multibyte encoding. (VIV) */
	len = strlen(s) + 1;
	errno_save = errno;
	errno = 0;
	unix_strupper(s,len,s,len);	
	/* Catch mb conversion errors that may not terminate. */
	if (errno)
		s[len-1] = '\0';
	errno = errno_save;
}

/**
 Return a RFC2254 binary string representation of a buffer.
 Used in LDAP filters.
 Caller must free.
**/

char *binary_string_rfc2254(char *buf, int len)
{
	char *s;
	int i, j;
	const char *hex = "0123456789ABCDEF";
	s = SMB_MALLOC(len * 3 + 1);
	if (!s)
		return NULL;
	for (j=i=0;i<len;i++) {
		s[j] = '\\';
		s[j+1] = hex[((unsigned char)buf[i]) >> 4];
		s[j+2] = hex[((unsigned char)buf[i]) & 0xF];
		j += 3;
	}
	s[j] = 0;
	return s;
}

char *binary_string(char *buf, int len)
{
	char *s;
	int i, j;
	const char *hex = "0123456789ABCDEF";
	s = SMB_MALLOC(len * 2 + 1);
	if (!s)
		return NULL;
	for (j=i=0;i<len;i++) {
		s[j]   = hex[((unsigned char)buf[i]) >> 4];
		s[j+1] = hex[((unsigned char)buf[i]) & 0xF];
		j += 2;
	}
	s[j] = 0;
	return s;
}
/**
 Just a typesafety wrapper for snprintf into a pstring.
**/

 int pstr_sprintf(pstring s, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(s, PSTRING_LEN, fmt, ap);
	va_end(ap);
	return ret;
}


/**
 Just a typesafety wrapper for snprintf into a fstring.
**/

int fstr_sprintf(fstring s, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(s, FSTRING_LEN, fmt, ap);
	va_end(ap);
	return ret;
}


#if !defined(HAVE_STRNDUP) || defined(BROKEN_STRNDUP)
/**
 Some platforms don't have strndup.
**/
#if defined(PARANOID_MALLOC_CHECKER)
#undef strndup
#endif

 char *strndup(const char *s, size_t n)
{
	char *ret;
	
	n = strnlen(s, n);
	ret = SMB_MALLOC(n+1);
	if (!ret)
		return NULL;
	memcpy(ret, s, n);
	ret[n] = 0;

	return ret;
}

#if defined(PARANOID_MALLOC_CHECKER)
#define strndup(s,n) __ERROR_DONT_USE_STRNDUP_DIRECTLY
#endif

#endif

#if !defined(HAVE_STRNLEN) || defined(BROKEN_STRNLEN)
/**
 Some platforms don't have strnlen
**/

 size_t strnlen(const char *s, size_t n)
{
	size_t i;
	for (i=0; i<n && s[i] != '\0'; i++)
		/* noop */ ;
	return i;
}
#endif

/**
 List of Strings manipulation functions
**/

#define S_LIST_ABS 16 /* List Allocation Block Size */

static char **str_list_make_internal(TALLOC_CTX *mem_ctx, const char *string, const char *sep)
{
	char **list, **rlist;
	const char *str;
	char *s;
	int num, lsize;
	pstring tok;
	
	if (!string || !*string)
		return NULL;
	if (mem_ctx) {
		s = talloc_strdup(mem_ctx, string);
	} else {
		s = SMB_STRDUP(string);
	}
	if (!s) {
		DEBUG(0,("str_list_make: Unable to allocate memory"));
		return NULL;
	}
	if (!sep) sep = LIST_SEP;
	
	num = lsize = 0;
	list = NULL;
	
	str = s;
	while (next_token(&str, tok, sep, sizeof(tok))) {		
		if (num == lsize) {
			lsize += S_LIST_ABS;
			if (mem_ctx) {
				rlist = TALLOC_REALLOC_ARRAY(mem_ctx, list, char *, lsize +1);
			} else {
				/* We need to keep the old list on error so we can free the elements
				   if the realloc fails. */
				rlist = SMB_REALLOC_ARRAY_KEEP_OLD_ON_ERROR(list, char *, lsize +1);
			}
			if (!rlist) {
				DEBUG(0,("str_list_make: Unable to allocate memory"));
				str_list_free(&list);
				if (mem_ctx) {
					TALLOC_FREE(s);
				} else {
					SAFE_FREE(s);
				}
				return NULL;
			} else {
				list = rlist;
			}
			memset (&list[num], 0, ((sizeof(char**)) * (S_LIST_ABS +1)));
		}

		if (mem_ctx) {
			list[num] = talloc_strdup(mem_ctx, tok);
		} else {
			list[num] = SMB_STRDUP(tok);
		}
		
		if (!list[num]) {
			DEBUG(0,("str_list_make: Unable to allocate memory"));
			str_list_free(&list);
			if (mem_ctx) {
				TALLOC_FREE(s);
			} else {
				SAFE_FREE(s);
			}
			return NULL;
		}
	
		num++;	
	}

	if (mem_ctx) {
		TALLOC_FREE(s);
	} else {
		SAFE_FREE(s);
	}

	return list;
}

char **str_list_make_talloc(TALLOC_CTX *mem_ctx, const char *string, const char *sep)
{
	return str_list_make_internal(mem_ctx, string, sep);
}

char **str_list_make(const char *string, const char *sep)
{
	return str_list_make_internal(NULL, string, sep);
}

BOOL str_list_copy(char ***dest, const char **src)
{
	char **list, **rlist;
	int num, lsize;
	
	*dest = NULL;
	if (!src)
		return False;
	
	num = lsize = 0;
	list = NULL;
		
	while (src[num]) {
		if (num == lsize) {
			lsize += S_LIST_ABS;
			rlist = SMB_REALLOC_ARRAY_KEEP_OLD_ON_ERROR(list, char *, lsize +1);
			if (!rlist) {
				DEBUG(0,("str_list_copy: Unable to re-allocate memory"));
				str_list_free(&list);
				return False;
			} else {
				list = rlist;
			}
			memset (&list[num], 0, ((sizeof(char **)) * (S_LIST_ABS +1)));
		}
		
		list[num] = SMB_STRDUP(src[num]);
		if (!list[num]) {
			DEBUG(0,("str_list_copy: Unable to allocate memory"));
			str_list_free(&list);
			return False;
		}

		num++;
	}
	
	*dest = list;
	return True;	
}

/**
 * Return true if all the elements of the list match exactly.
 **/
BOOL str_list_compare(char **list1, char **list2)
{
	int num;
	
	if (!list1 || !list2)
		return (list1 == list2); 
	
	for (num = 0; list1[num]; num++) {
		if (!list2[num])
			return False;
		if (!strcsequal(list1[num], list2[num]))
			return False;
	}
	if (list2[num])
		return False; /* if list2 has more elements than list1 fail */
	
	return True;
}

static void str_list_free_internal(TALLOC_CTX *mem_ctx, char ***list)
{
	char **tlist;
	
	if (!list || !*list)
		return;
	tlist = *list;
	for(; *tlist; tlist++) {
		if (mem_ctx) {
			TALLOC_FREE(*tlist);
		} else {
			SAFE_FREE(*tlist);
		}
	}
	if (mem_ctx) {
		TALLOC_FREE(*tlist);
	} else {
		SAFE_FREE(*list);
	}
}

void str_list_free_talloc(TALLOC_CTX *mem_ctx, char ***list)
{
	str_list_free_internal(mem_ctx, list);
}

void str_list_free(char ***list)
{
	str_list_free_internal(NULL, list);
}

/******************************************************************************
 *****************************************************************************/

int str_list_count( const char **list )
{
	int i = 0;

	if ( ! list )
		return 0;

	/* count the number of list members */
	
	for ( i=0; *list; i++, list++ );
	
	return i;
}

/******************************************************************************
 version of standard_sub_basic() for string lists; uses alloc_sub_basic() 
 for the work
 *****************************************************************************/
 
BOOL str_list_sub_basic( char **list, const char *smb_name )
{
	char *s, *tmpstr;
	
	while ( *list ) {
		s = *list;
		tmpstr = alloc_sub_basic(smb_name, s);
		if ( !tmpstr ) {
			DEBUG(0,("str_list_sub_basic: alloc_sub_basic() return NULL!\n"));
			return False;
		}

		SAFE_FREE(*list);
		*list = tmpstr;
			
		list++;
	}

	return True;
}

/******************************************************************************
 substritute a specific pattern in a string list
 *****************************************************************************/
 
BOOL str_list_substitute(char **list, const char *pattern, const char *insert)
{
	char *p, *s, *t;
	ssize_t ls, lp, li, ld, i, d;

	if (!list)
		return False;
	if (!pattern)
		return False;
	if (!insert)
		return False;

	lp = (ssize_t)strlen(pattern);
	li = (ssize_t)strlen(insert);
	ld = li -lp;
			
	while (*list) {
		s = *list;
		ls = (ssize_t)strlen(s);

		while ((p = strstr_m(s, pattern))) {
			t = *list;
			d = p -t;
			if (ld) {
				t = (char *) SMB_MALLOC(ls +ld +1);
				if (!t) {
					DEBUG(0,("str_list_substitute: Unable to allocate memory"));
					return False;
				}
				memcpy(t, *list, d);
				memcpy(t +d +li, p +lp, ls -d -lp +1);
				SAFE_FREE(*list);
				*list = t;
				ls += ld;
				s = t +d +li;
			}
			
			for (i = 0; i < li; i++) {
				switch (insert[i]) {
					case '`':
					case '"':
					case '\'':
					case ';':
					case '$':
					case '%':
					case '\r':
					case '\n':
						t[d +i] = '_';
						break;
					default:
						t[d +i] = insert[i];
				}
			}	
		}
		
		
		list++;
	}
	
	return True;
}


#define IPSTR_LIST_SEP	","
#define IPSTR_LIST_CHAR	','

/**
 * Add ip string representation to ipstr list. Used also
 * as part of @function ipstr_list_make
 *
 * @param ipstr_list pointer to string containing ip list;
 *        MUST BE already allocated and IS reallocated if necessary
 * @param ipstr_size pointer to current size of ipstr_list (might be changed
 *        as a result of reallocation)
 * @param ip IP address which is to be added to list
 * @return pointer to string appended with new ip and possibly
 *         reallocated to new length
 **/

char* ipstr_list_add(char** ipstr_list, const struct ip_service *service)
{
	char* new_ipstr = NULL;
	
	/* arguments checking */
	if (!ipstr_list || !service) return NULL;

	/* attempt to convert ip to a string and append colon separator to it */
	if (*ipstr_list) {
		asprintf(&new_ipstr, "%s%s%s:%d", *ipstr_list, IPSTR_LIST_SEP,
			inet_ntoa(service->ip), service->port);
		SAFE_FREE(*ipstr_list);
	} else {
		asprintf(&new_ipstr, "%s:%d", inet_ntoa(service->ip), service->port);
	}
	*ipstr_list = new_ipstr;
	return *ipstr_list;
}


/**
 * Allocate and initialise an ipstr list using ip adresses
 * passed as arguments.
 *
 * @param ipstr_list pointer to string meant to be allocated and set
 * @param ip_list array of ip addresses to place in the list
 * @param ip_count number of addresses stored in ip_list
 * @return pointer to allocated ip string
 **/
 
char* ipstr_list_make(char** ipstr_list, const struct ip_service* ip_list, int ip_count)
{
	int i;
	
	/* arguments checking */
	if (!ip_list || !ipstr_list) return 0;

	*ipstr_list = NULL;
	
	/* process ip addresses given as arguments */
	for (i = 0; i < ip_count; i++)
		*ipstr_list = ipstr_list_add(ipstr_list, &ip_list[i]);
	
	return (*ipstr_list);
}


/**
 * Parse given ip string list into array of ip addresses
 * (as ip_service structures)  
 *    e.g. 192.168.1.100:389,192.168.1.78, ...
 *
 * @param ipstr ip string list to be parsed 
 * @param ip_list pointer to array of ip addresses which is
 *        allocated by this function and must be freed by caller
 * @return number of succesfully parsed addresses
 **/
 
int ipstr_list_parse(const char* ipstr_list, struct ip_service **ip_list)
{
	fstring token_str;
	size_t count;
	int i;

	if (!ipstr_list || !ip_list) 
		return 0;
	
	count = count_chars(ipstr_list, IPSTR_LIST_CHAR) + 1;
	if ( (*ip_list = SMB_MALLOC_ARRAY(struct ip_service, count)) == NULL ) {
		DEBUG(0,("ipstr_list_parse: malloc failed for %lu entries\n", (unsigned long)count));
		return 0;
	}
	
	for ( i=0; 
		next_token(&ipstr_list, token_str, IPSTR_LIST_SEP, FSTRING_LEN) && i<count; 
		i++ ) 
	{
		struct in_addr addr;
		unsigned port = 0;	
		char *p = strchr(token_str, ':');
		
		if (p) {
			*p = 0;
			port = atoi(p+1);
		}

		/* convert single token to ip address */
		if ( (addr.s_addr = inet_addr(token_str)) == INADDR_NONE )
			break;
				
		(*ip_list)[i].ip = addr;
		(*ip_list)[i].port = port;
	}
	
	return count;
}


/**
 * Safely free ip string list
 *
 * @param ipstr_list ip string list to be freed
 **/

void ipstr_list_free(char* ipstr_list)
{
	SAFE_FREE(ipstr_list);
}


/**
 Unescape a URL encoded string, in place.
**/

void rfc1738_unescape(char *buf)
{
	char *p=buf;

	while (p && *p && (p=strchr_m(p,'%'))) {
		int c1 = p[1];
		int c2 = p[2];

		if (c1 >= '0' && c1 <= '9')
			c1 = c1 - '0';
		else if (c1 >= 'A' && c1 <= 'F')
			c1 = 10 + c1 - 'A';
		else if (c1 >= 'a' && c1 <= 'f')
			c1 = 10 + c1 - 'a';
		else {p++; continue;}

		if (c2 >= '0' && c2 <= '9')
			c2 = c2 - '0';
		else if (c2 >= 'A' && c2 <= 'F')
			c2 = 10 + c2 - 'A';
		else if (c2 >= 'a' && c2 <= 'f')
			c2 = 10 + c2 - 'a';
		else {p++; continue;}
			
		*p = (c1<<4) | c2;

		memmove(p+1, p+3, strlen(p+3)+1);
		p++;
	}
}

static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Decode a base64 string into a DATA_BLOB - simple and slow algorithm
 **/
DATA_BLOB base64_decode_data_blob(const char *s)
{
	int bit_offset, byte_offset, idx, i, n;
	DATA_BLOB decoded = data_blob(s, strlen(s)+1);
	unsigned char *d = decoded.data;
	char *p;

	n=i=0;

	while (*s && (p=strchr_m(b64,*s))) {
		idx = (int)(p - b64);
		byte_offset = (i*6)/8;
		bit_offset = (i*6)%8;
		d[byte_offset] &= ~((1<<(8-bit_offset))-1);
		if (bit_offset < 3) {
			d[byte_offset] |= (idx << (2-bit_offset));
			n = byte_offset+1;
		} else {
			d[byte_offset] |= (idx >> (bit_offset-2));
			d[byte_offset+1] = 0;
			d[byte_offset+1] |= (idx << (8-(bit_offset-2))) & 0xFF;
			n = byte_offset+2;
		}
		s++; i++;
	}

	if ((n > 0) && (*s == '=')) {
		n -= 1;
	}

	/* fix up length */
	decoded.length = n;
	return decoded;
}

/**
 * Decode a base64 string in-place - wrapper for the above
 **/
void base64_decode_inplace(char *s)
{
	DATA_BLOB decoded = base64_decode_data_blob(s);

	if ( decoded.length != 0 ) {
		memcpy(s, decoded.data, decoded.length);

		/* null terminate */
		s[decoded.length] = '\0';
	} else {
		*s = '\0';
	}

	data_blob_free(&decoded);
}

/**
 * Encode a base64 string into a malloc()ed string caller to free.
 *
 *From SQUID: adopted from http://ftp.sunet.se/pub2/gnu/vm/base64-encode.c with adjustments
 **/
char * base64_encode_data_blob(DATA_BLOB data)
{
	int bits = 0;
	int char_count = 0;
	size_t out_cnt, len, output_len;
	char *result;

        if (!data.length || !data.data)
		return NULL;

	out_cnt = 0;
	len = data.length;
	output_len = data.length * 2;
	result = SMB_MALLOC(output_len); /* get us plenty of space */

	while (len-- && out_cnt < (data.length * 2) - 5) {
		int c = (unsigned char) *(data.data++);
		bits += c;
		char_count++;
		if (char_count == 3) {
			result[out_cnt++] = b64[bits >> 18];
			result[out_cnt++] = b64[(bits >> 12) & 0x3f];
			result[out_cnt++] = b64[(bits >> 6) & 0x3f];
	    result[out_cnt++] = b64[bits & 0x3f];
	    bits = 0;
	    char_count = 0;
	} else {
	    bits <<= 8;
	}
    }
    if (char_count != 0) {
	bits <<= 16 - (8 * char_count);
	result[out_cnt++] = b64[bits >> 18];
	result[out_cnt++] = b64[(bits >> 12) & 0x3f];
	if (char_count == 1) {
	    result[out_cnt++] = '=';
	    result[out_cnt++] = '=';
	} else {
	    result[out_cnt++] = b64[(bits >> 6) & 0x3f];
	    result[out_cnt++] = '=';
	}
    }
    result[out_cnt] = '\0';	/* terminate */
    return result;
}

/* read a SMB_BIG_UINT from a string */
SMB_BIG_UINT STR_TO_SMB_BIG_UINT(const char *nptr, const char **entptr)
{

	SMB_BIG_UINT val = -1;
	const char *p = nptr;
	
	if (!p) {
		if (entptr) {
			*entptr = p;
		}
		return val;
	}

	while (*p && isspace(*p))
		p++;

#ifdef LARGE_SMB_OFF_T
	sscanf(p,"%llu",&val);	
#else /* LARGE_SMB_OFF_T */
	sscanf(p,"%lu",&val);
#endif /* LARGE_SMB_OFF_T */
	if (entptr) {
		while (*p && isdigit(*p))
			p++;
		*entptr = p;
	}

	return val;
}

void string_append(char **left, const char *right)
{
	int new_len = strlen(right) + 1;

	if (*left == NULL) {
		*left = SMB_MALLOC(new_len);
		*left[0] = '\0';
	} else {
		new_len += strlen(*left);
		*left = SMB_REALLOC(*left, new_len);
	}

	if (*left == NULL) {
		return;
	}

	safe_strcat(*left, right, new_len-1);
}

BOOL add_string_to_array(TALLOC_CTX *mem_ctx,
			 const char *str, const char ***strings,
			 int *num)
{
	char *dup_str = talloc_strdup(mem_ctx, str);

	*strings = TALLOC_REALLOC_ARRAY(mem_ctx, *strings, const char *, (*num)+1);

	if ((*strings == NULL) || (dup_str == NULL))
		return False;

	(*strings)[*num] = dup_str;
	*num += 1;
	return True;
}

/* Append an sprintf'ed string. Double buffer size on demand. Usable without
 * error checking in between. The indiation that something weird happened is
 * string==NULL */

void sprintf_append(TALLOC_CTX *mem_ctx, char **string, ssize_t *len,
		    size_t *bufsize, const char *fmt, ...)
{
	va_list ap;
	char *newstr;
	int ret;
	BOOL increased;

	/* len<0 is an internal marker that something failed */
	if (*len < 0)
		goto error;

	if (*string == NULL) {
		if (*bufsize == 0)
			*bufsize = 128;

		if (mem_ctx != NULL)
			*string = TALLOC_ARRAY(mem_ctx, char, *bufsize);
		else
			*string = SMB_MALLOC_ARRAY(char, *bufsize);

		if (*string == NULL)
			goto error;
	}

	va_start(ap, fmt);
	ret = vasprintf(&newstr, fmt, ap);
	va_end(ap);

	if (ret < 0)
		goto error;

	increased = False;

	while ((*len)+ret >= *bufsize) {
		increased = True;
		*bufsize *= 2;
		if (*bufsize >= (1024*1024*256))
			goto error;
	}

	if (increased) {
		if (mem_ctx != NULL) {
			*string = TALLOC_REALLOC_ARRAY(mem_ctx, *string, char,
						       *bufsize);
		} else {
			*string = SMB_REALLOC_ARRAY(*string, char, *bufsize);
		}

		if (*string == NULL) {
			goto error;
		}
	}

	StrnCpy((*string)+(*len), newstr, ret);
	(*len) += ret;
	free(newstr);
	return;

 error:
	*len = -1;
	if (mem_ctx == NULL) {
		SAFE_FREE(*string);
	}
	*string = NULL;
}

/*
   Returns the substring from src between the first occurrence of
   the char "front" and the first occurence of the char "back".
   Mallocs the return string which must be freed.  Not for use
   with wide character strings.
*/
char *sstring_sub(const char *src, char front, char back)
{
	char *temp1, *temp2, *temp3;
	ptrdiff_t len;

	temp1 = strchr(src, front);
	if (temp1 == NULL) return NULL;
	temp2 = strchr(src, back);
	if (temp2 == NULL) return NULL;
	len = temp2 - temp1;
	if (len <= 0) return NULL;
	temp3 = (char*)SMB_MALLOC(len);
	if (temp3 == NULL) {
		DEBUG(1,("Malloc failure in sstring_sub\n"));
		return NULL;
	}
	memcpy(temp3, temp1+1, len-1);
	temp3[len-1] = '\0';
	return temp3;
}

/********************************************************************
 Check a string for any occurrences of a specified list of invalid
 characters.
********************************************************************/

BOOL validate_net_name( const char *name, const char *invalid_chars, int max_len )
{
	int i;

	for ( i=0; i<max_len && name[i]; i++ ) {
		/* fail if strchr_m() finds one of the invalid characters */
		if ( name[i] && strchr_m( invalid_chars, name[i] ) ) {
			return False;
		}
	}

	return True;
}

