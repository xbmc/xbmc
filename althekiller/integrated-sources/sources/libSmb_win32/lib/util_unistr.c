/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Andrew Tridgell 1992-2001
   Copyright (C) Simo Sorce 2001
   Copyright (C) Jeremy Allison 2005
   
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

#ifndef MAXUNI
#define MAXUNI 1024
#endif

/* these 3 tables define the unicode case handling.  They are loaded
   at startup either via mmap() or read() from the lib directory */
static smb_ucs2_t *upcase_table;
static smb_ucs2_t *lowcase_table;
static uint8 *valid_table;
static BOOL upcase_table_use_unmap;
static BOOL lowcase_table_use_unmap;
static BOOL valid_table_use_unmap;

/**
 * This table says which Unicode characters are valid dos
 * characters.
 *
 * Each value is just a single bit.
 **/
static uint8 doschar_table[8192]; /* 65536 characters / 8 bits/byte */

/**
 * Destroy global objects allocated by load_case_tables()
 **/
void gfree_case_tables(void)
{
	if ( upcase_table ) {
		if ( upcase_table_use_unmap )
			unmap_file(upcase_table, 0x20000);
		else
			SAFE_FREE(upcase_table);
	}

	if ( lowcase_table ) {
		if ( lowcase_table_use_unmap )
			unmap_file(lowcase_table, 0x20000);
		else
			SAFE_FREE(lowcase_table);
	}

	if ( valid_table ) {
		if ( valid_table_use_unmap )
			unmap_file(valid_table, 0x10000);
		else
			SAFE_FREE(valid_table);
	}
}

/**
 * Load or generate the case handling tables.
 *
 * The case tables are defined in UCS2 and don't depend on any
 * configured parameters, so they never need to be reloaded.
 **/

void load_case_tables(void)
{
	static int initialised;
	char *old_locale = NULL, *saved_locale = NULL;
	int i;

	if (initialised) {
		return;
	}
	initialised = 1;

	upcase_table = map_file(lib_path("upcase.dat"), 0x20000);
	upcase_table_use_unmap = ( upcase_table != NULL );

	lowcase_table = map_file(lib_path("lowcase.dat"), 0x20000);
	lowcase_table_use_unmap = ( lowcase_table != NULL );

#ifdef HAVE_SETLOCALE
	/* Get the name of the current locale.  */
	old_locale = setlocale(LC_ALL, NULL);

	if (old_locale) {
		/* Save it as it is in static storage. */
		saved_locale = SMB_STRDUP(old_locale);
	}

	/* We set back the locale to C to get ASCII-compatible toupper/lower functions. */
	setlocale(LC_ALL, "C");
#endif

	/* we would like Samba to limp along even if these tables are
	   not available */
	if (!upcase_table) {
		DEBUG(1,("creating lame upcase table\n"));
		upcase_table = SMB_MALLOC(0x20000);
		for (i=0;i<0x10000;i++) {
			smb_ucs2_t v;
			SSVAL(&v, 0, i);
			upcase_table[v] = i;
		}
		for (i=0;i<256;i++) {
			smb_ucs2_t v;
			SSVAL(&v, 0, UCS2_CHAR(i));
			upcase_table[v] = UCS2_CHAR(islower(i)?toupper(i):i);
		}
	}

	if (!lowcase_table) {
		DEBUG(1,("creating lame lowcase table\n"));
		lowcase_table = SMB_MALLOC(0x20000);
		for (i=0;i<0x10000;i++) {
			smb_ucs2_t v;
			SSVAL(&v, 0, i);
			lowcase_table[v] = i;
		}
		for (i=0;i<256;i++) {
			smb_ucs2_t v;
			SSVAL(&v, 0, UCS2_CHAR(i));
			lowcase_table[v] = UCS2_CHAR(isupper(i)?tolower(i):i);
		}
	}

#ifdef HAVE_SETLOCALE
	/* Restore the old locale. */
	if (saved_locale) {
		setlocale (LC_ALL, saved_locale);
		SAFE_FREE(saved_locale);
	}
#endif
}

/*
  see if a ucs2 character can be mapped correctly to a dos character
  and mapped back to the same character in ucs2
*/

int check_dos_char(smb_ucs2_t c)
{
	lazy_initialize_conv();
	
	/* Find the right byte, and right bit within the byte; return
	 * 1 or 0 */
	return (doschar_table[(c & 0xffff) / 8] & (1 << (c & 7))) != 0;
}


static int check_dos_char_slowly(smb_ucs2_t c)
{
	char buf[10];
	smb_ucs2_t c2 = 0;
	int len1, len2;

	len1 = convert_string(CH_UCS2, CH_DOS, &c, 2, buf, sizeof(buf),False);
	if (len1 == 0) {
		return 0;
	}
	len2 = convert_string(CH_DOS, CH_UCS2, buf, len1, &c2, 2,False);
	if (len2 != 2) {
		return 0;
	}
	return (c == c2);
}


/**
 * Fill out doschar table the hard way, by examining each character
 **/

void init_doschar_table(void)
{
	int i, j, byteval;

	/* For each byte of packed table */
	
	for (i = 0; i <= 0xffff; i += 8) {
		byteval = 0;
		for (j = 0; j <= 7; j++) {
			smb_ucs2_t c;

			c = i + j;
			
			if (check_dos_char_slowly(c)) {
				byteval |= 1 << j;
			}
		}
		doschar_table[i/8] = byteval;
	}
}


/**
 * Load the valid character map table from <tt>valid.dat</tt> or
 * create from the configured codepage.
 *
 * This function is called whenever the configuration is reloaded.
 * However, the valid character table is not changed if it's loaded
 * from a file, because we can't unmap files.
 **/

void init_valid_table(void)
{
	static int mapped_file;
	int i;
	const char *allowed = ".!#$%&'()_-@^`~";
	uint8 *valid_file;

	if (mapped_file) {
		/* Can't unmap files, so stick with what we have */
		return;
	}

	valid_file = map_file(lib_path("valid.dat"), 0x10000);
	if (valid_file) {
		valid_table = valid_file;
		mapped_file = 1;
		valid_table_use_unmap = True;
		return;
	}

	/* Otherwise, we're using a dynamically created valid_table.
	 * It might need to be regenerated if the code page changed.
	 * We know that we're not using a mapped file, so we can
	 * free() the old one. */
	if (valid_table) 
		SAFE_FREE(valid_table);

	/* use free rather than unmap */
	valid_table_use_unmap = False;

	DEBUG(2,("creating default valid table\n"));
	valid_table = SMB_MALLOC(0x10000);
	for (i=0;i<128;i++) {
		valid_table[i] = isalnum(i) || strchr(allowed,i);
	}
	
	for (;i<0x10000;i++) {
		smb_ucs2_t c;
		SSVAL(&c, 0, i);
		valid_table[i] = check_dos_char(c);
	}
}

/*******************************************************************
 Write a string in (little-endian) unicode format. src is in
 the current DOS codepage. len is the length in bytes of the
 string pointed to by dst.

 if null_terminate is True then null terminate the packet (adds 2 bytes)

 the return value is the length in bytes consumed by the string, including the
 null termination if applied
********************************************************************/

size_t dos_PutUniCode(char *dst,const char *src, size_t len, BOOL null_terminate)
{
	int flags = null_terminate ? STR_UNICODE|STR_NOALIGN|STR_TERMINATE
				   : STR_UNICODE|STR_NOALIGN;
	return push_ucs2(NULL, dst, src, len, flags);
}


/*******************************************************************
 Skip past a unicode string, but not more than len. Always move
 past a terminating zero if found.
********************************************************************/

char *skip_unibuf(char *src, size_t len)
{
	char *srcend = src + len;

	while (src < srcend && SVAL(src,0)) {
		src += 2;
	}

	if(!SVAL(src,0)) {
		src += 2;
	}

	return src;
}

/* Copy a string from little-endian or big-endian unicode source (depending
 * on flags) to internal samba format destination
 */ 

int rpcstr_pull(char* dest, void *src, int dest_len, int src_len, int flags)
{
	if (!src) {
		dest[0] = 0;
		return 0;
	}
	if(dest_len==-1) {
		dest_len=MAXUNI-3;
	}
	return pull_ucs2(NULL, dest, src, dest_len, src_len, flags|STR_UNICODE|STR_NOALIGN);
}

/* Copy a string from a unistr2 source to internal samba format
   destination.  Use this instead of direct calls to rpcstr_pull() to avoid
   having to determine whether the source string is null terminated. */

int rpcstr_pull_unistr2_fstring(char *dest, UNISTR2 *src)
{
        return pull_ucs2(NULL, dest, src->buffer, sizeof(fstring),
                         src->uni_str_len * 2, 0);
}

/* Helper function to return a talloc'ed string. I have implemented it with a
 * copy because I don't really know how pull_ucs2 and friends calculate the
 * target size. If this turns out to be a major bottleneck someone with deeper
 * multi-byte knowledge needs to revisit this.
 * My (VL) use is dsr_getdcname, which returns 6 strings, the alternative would
 * have been to manually talloc_strdup them in rpc_client/cli_netlogon.c.
 */

char *rpcstr_pull_unistr2_talloc(TALLOC_CTX *mem_ctx, const UNISTR2 *src)
{
	pstring tmp;
	size_t result;

	result = pull_ucs2(NULL, tmp, src->buffer, sizeof(tmp),
			   src->uni_str_len * 2, 0);
	if (result == (size_t)-1) {
		return NULL;
	}

	return talloc_strdup(mem_ctx, tmp);
}

/* Converts a string from internal samba format to unicode
 */ 

int rpcstr_push(void* dest, const char *src, size_t dest_len, int flags)
{
	return push_ucs2(NULL, dest, src, dest_len, flags|STR_UNICODE|STR_NOALIGN);
}

/*******************************************************************
 Convert a (little-endian) UNISTR2 structure to an ASCII string.
********************************************************************/

void unistr2_to_ascii(char *dest, const UNISTR2 *str, size_t maxlen)
{
	if (str == NULL) {
		*dest='\0';
		return;
	}
	pull_ucs2(NULL, dest, str->buffer, maxlen, str->uni_str_len*2, STR_NOALIGN);
}

/*******************************************************************
 Convert a (little-endian) UNISTR3 structure to an ASCII string.
********************************************************************/

void unistr3_to_ascii(char *dest, const UNISTR3 *str, size_t maxlen)
{
	if (str == NULL) {
		*dest='\0';
		return;
	}
	pull_ucs2(NULL, dest, str->str.buffer, maxlen, str->uni_str_len*2,
	          STR_NOALIGN);
}
	
/*******************************************************************
 Give a static string for displaying a UNISTR2.
********************************************************************/

const char *unistr2_static(const UNISTR2 *str)
{
	static pstring ret;
	unistr2_to_ascii(ret, str, sizeof(ret));
	return ret;
}

/*******************************************************************
 Duplicate a UNISTR2 string into a null terminated char*
 using a talloc context.
********************************************************************/

char *unistr2_tdup(TALLOC_CTX *ctx, const UNISTR2 *str)
{
	char *s;
	int maxlen = (str->uni_str_len+1)*4;
	if (!str->buffer) {
		return NULL;
	}
	s = (char *)TALLOC(ctx, maxlen); /* convervative */
	if (!s) {
		return NULL;
	}
	pull_ucs2(NULL, s, str->buffer, maxlen, str->uni_str_len*2, STR_NOALIGN);
	return s;
}

/*******************************************************************
 Convert a wchar to upper case.
********************************************************************/

smb_ucs2_t toupper_w(smb_ucs2_t val)
{
	return upcase_table[SVAL(&val,0)];
}

/*******************************************************************
 Convert a wchar to lower case.
********************************************************************/

smb_ucs2_t tolower_w( smb_ucs2_t val )
{
	return lowcase_table[SVAL(&val,0)];
}

/*******************************************************************
 Determine if a character is lowercase.
********************************************************************/

BOOL islower_w(smb_ucs2_t c)
{
	return upcase_table[SVAL(&c,0)] != c;
}

/*******************************************************************
 Determine if a character is uppercase.
********************************************************************/

BOOL isupper_w(smb_ucs2_t c)
{
	return lowcase_table[SVAL(&c,0)] != c;
}

/*******************************************************************
 Determine if a character is valid in a 8.3 name.
********************************************************************/

BOOL isvalid83_w(smb_ucs2_t c)
{
	return valid_table[SVAL(&c,0)] != 0;
}

/*******************************************************************
 Count the number of characters in a smb_ucs2_t string.
********************************************************************/

size_t strlen_w(const smb_ucs2_t *src)
{
	size_t len;
	smb_ucs2_t c;

	for(len = 0; *(COPY_UCS2_CHAR(&c,src)); src++, len++) {
		;
	}

	return len;
}

/*******************************************************************
 Count up to max number of characters in a smb_ucs2_t string.
********************************************************************/

size_t strnlen_w(const smb_ucs2_t *src, size_t max)
{
	size_t len;
	smb_ucs2_t c;

	for(len = 0; *(COPY_UCS2_CHAR(&c,src)) && (len < max); src++, len++) {
		;
	}

	return len;
}

/*******************************************************************
 Wide strchr().
********************************************************************/

smb_ucs2_t *strchr_w(const smb_ucs2_t *s, smb_ucs2_t c)
{
	smb_ucs2_t cp;
	while (*(COPY_UCS2_CHAR(&cp,s))) {
		if (c == cp) {
			return (smb_ucs2_t *)s;
		}
		s++;
	}
	if (c == cp) {
		return (smb_ucs2_t *)s;
	}

	return NULL;
}

smb_ucs2_t *strchr_wa(const smb_ucs2_t *s, char c)
{
	return strchr_w(s, UCS2_CHAR(c));
}

/*******************************************************************
 Wide strrchr().
********************************************************************/

smb_ucs2_t *strrchr_w(const smb_ucs2_t *s, smb_ucs2_t c)
{
	smb_ucs2_t cp;
	const smb_ucs2_t *p = s;
	int len = strlen_w(s);

	if (len == 0) {
		return NULL;
	}
	p += (len - 1);
	do {
		if (c == *(COPY_UCS2_CHAR(&cp,p))) {
			return (smb_ucs2_t *)p;
		}
	} while (p-- != s);
	return NULL;
}

/*******************************************************************
 Wide version of strrchr that returns after doing strrchr 'n' times.
********************************************************************/

smb_ucs2_t *strnrchr_w(const smb_ucs2_t *s, smb_ucs2_t c, unsigned int n)
{
	smb_ucs2_t cp;
	const smb_ucs2_t *p = s;
	int len = strlen_w(s);

	if (len == 0 || !n) {
		return NULL;
	}
	p += (len - 1);
	do {
		if (c == *(COPY_UCS2_CHAR(&cp,p))) {
			n--;
		}

		if (!n) {
			return (smb_ucs2_t *)p;
		}
	} while (p-- != s);
	return NULL;
}

/*******************************************************************
 Wide strstr().
********************************************************************/

smb_ucs2_t *strstr_w(const smb_ucs2_t *s, const smb_ucs2_t *ins)
{
	smb_ucs2_t *r;
	size_t inslen;

	if (!s || !*s || !ins || !*ins) {
		return NULL;
	}

	inslen = strlen_w(ins);
	r = (smb_ucs2_t *)s;

	while ((r = strchr_w(r, *ins))) {
		if (strncmp_w(r, ins, inslen) == 0) {
			return r;
		}
		r++;
	}

	return NULL;
}

/*******************************************************************
 Convert a string to lower case.
 return True if any char is converted
********************************************************************/

BOOL strlower_w(smb_ucs2_t *s)
{
	smb_ucs2_t cp;
	BOOL ret = False;

	while (*(COPY_UCS2_CHAR(&cp,s))) {
		smb_ucs2_t v = tolower_w(cp);
		if (v != cp) {
			COPY_UCS2_CHAR(s,&v);
			ret = True;
		}
		s++;
	}
	return ret;
}

/*******************************************************************
 Convert a string to upper case.
 return True if any char is converted
********************************************************************/

BOOL strupper_w(smb_ucs2_t *s)
{
	smb_ucs2_t cp;
	BOOL ret = False;
	while (*(COPY_UCS2_CHAR(&cp,s))) {
		smb_ucs2_t v = toupper_w(cp);
		if (v != cp) {
			COPY_UCS2_CHAR(s,&v);
			ret = True;
		}
		s++;
	}
	return ret;
}

/*******************************************************************
 Convert a string to "normal" form.
********************************************************************/

void strnorm_w(smb_ucs2_t *s, int case_default)
{
	if (case_default == CASE_UPPER) {
		strupper_w(s);
	} else {
		strlower_w(s);
	}
}

int strcmp_w(const smb_ucs2_t *a, const smb_ucs2_t *b)
{
	smb_ucs2_t cpa, cpb;

	while ((*(COPY_UCS2_CHAR(&cpb,b))) && (*(COPY_UCS2_CHAR(&cpa,a)) == cpb)) {
		a++;
		b++;
	}
	return (*(COPY_UCS2_CHAR(&cpa,a)) - *(COPY_UCS2_CHAR(&cpb,b)));
	/* warning: if *a != *b and both are not 0 we return a random
		greater or lesser than 0 number not realted to which
		string is longer */
}

int strncmp_w(const smb_ucs2_t *a, const smb_ucs2_t *b, size_t len)
{
	smb_ucs2_t cpa, cpb;
	size_t n = 0;

	while ((n < len) && (*(COPY_UCS2_CHAR(&cpb,b))) && (*(COPY_UCS2_CHAR(&cpa,a)) == cpb)) {
		a++;
		b++;
		n++;
	}
	return (len - n)?(*(COPY_UCS2_CHAR(&cpa,a)) - *(COPY_UCS2_CHAR(&cpb,b))):0;
}

/*******************************************************************
 Case insensitive string comparison.
********************************************************************/

int strcasecmp_w(const smb_ucs2_t *a, const smb_ucs2_t *b)
{
	smb_ucs2_t cpa, cpb;

	while ((*COPY_UCS2_CHAR(&cpb,b)) && toupper_w(*(COPY_UCS2_CHAR(&cpa,a))) == toupper_w(cpb)) {
		a++;
		b++;
	}
	return (tolower_w(*(COPY_UCS2_CHAR(&cpa,a))) - tolower_w(*(COPY_UCS2_CHAR(&cpb,b))));
}

/*******************************************************************
 Case insensitive string comparison, length limited.
********************************************************************/

int strncasecmp_w(const smb_ucs2_t *a, const smb_ucs2_t *b, size_t len)
{
	smb_ucs2_t cpa, cpb;
	size_t n = 0;

	while ((n < len) && *COPY_UCS2_CHAR(&cpb,b) && (toupper_w(*(COPY_UCS2_CHAR(&cpa,a))) == toupper_w(cpb))) {
		a++;
		b++;
		n++;
	}
	return (len - n)?(tolower_w(*(COPY_UCS2_CHAR(&cpa,a))) - tolower_w(*(COPY_UCS2_CHAR(&cpb,b)))):0;
}

/*******************************************************************
 Compare 2 strings.
********************************************************************/

BOOL strequal_w(const smb_ucs2_t *s1, const smb_ucs2_t *s2)
{
	if (s1 == s2) {
		return(True);
	}
	if (!s1 || !s2) {
		return(False);
	}
  
	return(strcasecmp_w(s1,s2)==0);
}

/*******************************************************************
 Compare 2 strings up to and including the nth char.
******************************************************************/

BOOL strnequal_w(const smb_ucs2_t *s1,const smb_ucs2_t *s2,size_t n)
{
	if (s1 == s2) {
		return(True);
	}
	if (!s1 || !s2 || !n) {
		return(False);
	}
  
	return(strncasecmp_w(s1,s2,n)==0);
}

/*******************************************************************
 Duplicate string.
********************************************************************/

smb_ucs2_t *strdup_w(const smb_ucs2_t *src)
{
	return strndup_w(src, 0);
}

/* if len == 0 then duplicate the whole string */

smb_ucs2_t *strndup_w(const smb_ucs2_t *src, size_t len)
{
	smb_ucs2_t *dest;
	
	if (!len) {
		len = strlen_w(src);
	}
	dest = SMB_MALLOC_ARRAY(smb_ucs2_t, len + 1);
	if (!dest) {
		DEBUG(0,("strdup_w: out of memory!\n"));
		return NULL;
	}

	memcpy(dest, src, len * sizeof(smb_ucs2_t));
	dest[len] = 0;
	return dest;
}

/*******************************************************************
 Copy a string with max len.
********************************************************************/

smb_ucs2_t *strncpy_w(smb_ucs2_t *dest, const smb_ucs2_t *src, const size_t max)
{
	smb_ucs2_t cp;
	size_t len;
	
	if (!dest || !src) {
		return NULL;
	}
	
	for (len = 0; (*COPY_UCS2_CHAR(&cp,(src+len))) && (len < max); len++) {
		cp = *COPY_UCS2_CHAR(dest+len,src+len);
	}
	cp = 0;
	for ( /*nothing*/ ; len < max; len++ ) {
		cp = *COPY_UCS2_CHAR(dest+len,&cp);
	}
	
	return dest;
}

/*******************************************************************
 Append a string of len bytes and add a terminator.
********************************************************************/

smb_ucs2_t *strncat_w(smb_ucs2_t *dest, const smb_ucs2_t *src, const size_t max)
{	
	size_t start;
	size_t len;	
	smb_ucs2_t z = 0;

	if (!dest || !src) {
		return NULL;
	}
	
	start = strlen_w(dest);
	len = strnlen_w(src, max);

	memcpy(&dest[start], src, len*sizeof(smb_ucs2_t));			
	z = *COPY_UCS2_CHAR(dest+start+len,&z);

	return dest;
}

smb_ucs2_t *strcat_w(smb_ucs2_t *dest, const smb_ucs2_t *src)
{	
	size_t start;
	size_t len;	
	smb_ucs2_t z = 0;
	
	if (!dest || !src) {
		return NULL;
	}
	
	start = strlen_w(dest);
	len = strlen_w(src);

	memcpy(&dest[start], src, len*sizeof(smb_ucs2_t));			
	z = *COPY_UCS2_CHAR(dest+start+len,&z);
	
	return dest;
}


/*******************************************************************
 Replace any occurence of oldc with newc in unicode string.
********************************************************************/

void string_replace_w(smb_ucs2_t *s, smb_ucs2_t oldc, smb_ucs2_t newc)
{
	smb_ucs2_t cp;

	for(;*(COPY_UCS2_CHAR(&cp,s));s++) {
		if(cp==oldc) {
			COPY_UCS2_CHAR(s,&newc);
		}
	}
}

/*******************************************************************
 Trim unicode string.
********************************************************************/

BOOL trim_string_w(smb_ucs2_t *s, const smb_ucs2_t *front,
				  const smb_ucs2_t *back)
{
	BOOL ret = False;
	size_t len, front_len, back_len;

	if (!s) {
		return False;
	}

	len = strlen_w(s);

	if (front && *front) {
		front_len = strlen_w(front);
		while (len && strncmp_w(s, front, front_len) == 0) {
			memmove(s, (s + front_len), (len - front_len + 1) * sizeof(smb_ucs2_t));
			len -= front_len;
			ret = True;
		}
	}
	
	if (back && *back) {
		back_len = strlen_w(back);
		while (len && strncmp_w((s + (len - back_len)), back, back_len) == 0) {
			s[len - back_len] = 0;
			len -= back_len;
			ret = True;
		}
	}

	return ret;
}

/*
  The *_wa() functions take a combination of 7 bit ascii
  and wide characters They are used so that you can use string
  functions combining C string constants with ucs2 strings

  The char* arguments must NOT be multibyte - to be completely sure
  of this only pass string constants */

int strcmp_wa(const smb_ucs2_t *a, const char *b)
{
	smb_ucs2_t cp = 0;

	while (*b && *(COPY_UCS2_CHAR(&cp,a)) == UCS2_CHAR(*b)) {
		a++;
		b++;
	}
	return (*(COPY_UCS2_CHAR(&cp,a)) - UCS2_CHAR(*b));
}

int strncmp_wa(const smb_ucs2_t *a, const char *b, size_t len)
{
	smb_ucs2_t cp = 0;
	size_t n = 0;

	while ((n < len) && *b && *(COPY_UCS2_CHAR(&cp,a)) == UCS2_CHAR(*b)) {
		a++;
		b++;
		n++;
	}
	return (len - n)?(*(COPY_UCS2_CHAR(&cp,a)) - UCS2_CHAR(*b)):0;
}

smb_ucs2_t *strpbrk_wa(const smb_ucs2_t *s, const char *p)
{
	smb_ucs2_t cp;

	while (*(COPY_UCS2_CHAR(&cp,s))) {
		int i;
		for (i=0; p[i] && cp != UCS2_CHAR(p[i]); i++) 
			;
		if (p[i]) {
			return (smb_ucs2_t *)s;
		}
		s++;
	}
	return NULL;
}

smb_ucs2_t *strstr_wa(const smb_ucs2_t *s, const char *ins)
{
	smb_ucs2_t *r;
	size_t inslen;

	if (!s || !ins) { 
		return NULL;
	}

	inslen = strlen(ins);
	r = (smb_ucs2_t *)s;

	while ((r = strchr_w(r, UCS2_CHAR(*ins)))) {
		if (strncmp_wa(r, ins, inslen) == 0) 
			return r;
		r++;
	}

	return NULL;
}

BOOL trim_string_wa(smb_ucs2_t *s, const char *front,
				  const char *back)
{
	wpstring f, b;

	if (front) {
		push_ucs2(NULL, f, front, sizeof(wpstring) - 1, STR_TERMINATE);
	} else {
		*f = 0;
	}
	if (back) {
		push_ucs2(NULL, b, back, sizeof(wpstring) - 1, STR_TERMINATE);
	} else {
		*b = 0;
	}
	return trim_string_w(s, f, b);
}

/*******************************************************************
 Returns the length in number of wide characters.
******************************************************************/

int unistrlen(uint16 *s)
{
	int len;

	if (!s) {
		return -1;
	}

	for (len=0; SVAL(s,0); s++,len++) {
		;
	}

	return len;
}

/*******************************************************************
 Strcpy for unicode strings. Returns length (in num of wide chars).
 Not odd align safe.
********************************************************************/

int unistrcpy(uint16 *dst, uint16 *src)
{
	int num_wchars = 0;

	while (SVAL(src,0)) {
		*dst++ = *src++;
		num_wchars++;
	}
	*dst = 0;

	return num_wchars;
}

/**
 * Samba ucs2 type to UNISTR2 conversion
 *
 * @param ctx Talloc context to create the dst strcture (if null) and the 
 *            contents of the unicode string.
 * @param dst UNISTR2 destination. If equals null, then it's allocated.
 * @param src smb_ucs2_t source.
 * @param max_len maximum number of unicode characters to copy. If equals
 *        null, then null-termination of src is taken
 *
 * @return copied UNISTR2 destination
 **/

UNISTR2* ucs2_to_unistr2(TALLOC_CTX *ctx, UNISTR2* dst, smb_ucs2_t* src)
{
	size_t len;

	if (!src) {
		return NULL;
	}

	len = strlen_w(src);
	
	/* allocate UNISTR2 destination if not given */
	if (!dst) {
		dst = TALLOC_P(ctx, UNISTR2);
		if (!dst)
			return NULL;
	}
	if (!dst->buffer) {
		dst->buffer = TALLOC_ARRAY(ctx, uint16, len + 1);
		if (!dst->buffer)
			return NULL;
	}
	
	/* set UNISTR2 parameters */
	dst->uni_max_len = len + 1;
	dst->offset = 0;
	dst->uni_str_len = len;
	
	/* copy the actual unicode string */
	strncpy_w(dst->buffer, src, dst->uni_max_len);
	
	return dst;
}

/*************************************************************
 ascii only toupper - saves the need for smbd to be in C locale.
*************************************************************/

int toupper_ascii(int c)
{
	smb_ucs2_t uc = toupper_w(UCS2_CHAR(c));
	return UCS2_TO_CHAR(uc);
}

/*************************************************************
 ascii only tolower - saves the need for smbd to be in C locale.
*************************************************************/

int tolower_ascii(int c)
{
	smb_ucs2_t uc = tolower_w(UCS2_CHAR(c));
	return UCS2_TO_CHAR(uc);
}

/*************************************************************
 ascii only isupper - saves the need for smbd to be in C locale.
*************************************************************/

int isupper_ascii(int c)
{
	return isupper_w(UCS2_CHAR(c));
}

/*************************************************************
 ascii only islower - saves the need for smbd to be in C locale.
*************************************************************/

int islower_ascii(int c)
{
	return islower_w(UCS2_CHAR(c));
}
