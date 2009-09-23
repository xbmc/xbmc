/* mchar.c
 * Codeset and wide character processing
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * REFS:
 *   http://mail.nl.linux.org/linux-utf8/2001-04/msg00083.html
 *   http://www.cl.cam.ac.uk/~mgk25/unicode.html
 *   http://mail.nl.linux.org/linux-utf8/2001-06/msg00020.html
 *   http://mail.nl.linux.org/linux-utf8/2001-04/msg00254.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "srconfig.h"
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined HAVE_WCHAR_SUPPORT
#if defined HAVE_WCHAR_H
#include <wchar.h>
#endif
#if defined HAVE_WCTYPE_H
#include <wctype.h>
#endif
#if defined HAVE_ICONV
#include <iconv.h>
#endif
#endif
#if defined HAVE_LOCALE_CHARSET
#include <localcharset.h>
#elif defined HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif
#include <locale.h>
#include <time.h>
#include <errno.h>
#include "debug.h"
#include "srtypes.h"
#include "mchar.h"

#if WIN32
    #define ICONV_WCHAR "UCS-2-INTERNAL"
    #define vsnprintf _vsnprintf
    #define vswprintf _vsnwprintf
#else
    #define ICONV_WCHAR "WCHAR_T"
    /* This prototype is missing in some systems */
    int vswprintf (wchar_t * ws, size_t n, const wchar_t * format, va_list arg);
#endif


/*****************************************************************************
 * Public functions
 *****************************************************************************/
char	*left_str(char *str, int len);
char	*subnstr_until(const char *str, char *until, char *newstr, int maxlen);
// char	*strip_invalid_chars(char *str);
char	*format_byte_size(char *str, long size);
void	trim(char *str);

/*****************************************************************************
 * Private global variables
 *****************************************************************************/
const char* m_codeset_locale;
const char* m_codeset_filesys;
const char* m_codeset_id3;
const char* m_codeset_metadata;
const char* m_codeset_relay;

/*****************************************************************************
 * These functions are NOT mchar related
 *****************************************************************************/
char*
subnstr_until(const char *str, char *until, char *newstr, int maxlen)
{
    const char *p = str;
    int len = 0;

    for(len = 0; strncmp(p, until, strlen(until)) != 0 && len < maxlen-1; p++)
    {
	newstr[len] = *p;
	len++;
    }
    newstr[len] = '\0';

    return newstr;
}

char *left_str(char *str, int len)
{
    int slen = strlen(str);

    if (slen <= len)
	return str;

    str[len] = '\0';
    return str;
}

char *format_byte_size(char *str, long size)
{
    const long ONE_K = 1024;
    const long ONE_M = ONE_K*ONE_K;

    if (size < ONE_K)
	sprintf(str, "%ldb", size);
    else if (size < ONE_M)
	sprintf(str, "%ldkb", size/ONE_K);
    else 
	sprintf(str, "%.2fM", (float)size/(ONE_M));
	
    return str;
}

void trim(char *str)
{
    int size = strlen(str)-1;
    while(str[size] == 13 || str[size] == 10 || str[size] == ' ')
    {
	str[size] = '\0';
	size--;
    }
    size = strlen(str);
    while(str[0] == ' ')
    {
	size--;
	memmove(str, str+1, size);
    }
    str[size] = '\0';
}

/* This is a little different from standard strncpy, because:
   1) behavior is known when dst & src overlap
   2) only copy n-1 characters max
   3) then add the null char
*/
void
sr_strncpy (char* dst, char* src, int n)
{
    int i = 0;
    for (i = 0; i < n-1; i++) {
	if (!(dst[i] = src[i])) {
	    return;
	}
    }
    dst[i] = 0;
}

/*****************************************************************************
 * These functions ARE mchar related
 *****************************************************************************/
#if HAVE_WCHAR_SUPPORT
# if HAVE_ICONV
int 
iconv_convert_string (char* dst, int dst_len, char* src, int src_len, 
		      const char* dst_codeset, const char* src_codeset)
{
    size_t rc;
    iconv_t ict;
    size_t src_left, dst_left;
    char *src_ptr, *dst_ptr;

    /* First try to convert using iconv. */
    ict = iconv_open (dst_codeset, src_codeset);
    if (ict == (iconv_t)(-1)) {
	printf ("Error on iconv_open(\"%s\",\"%s\")\n",
		      dst_codeset, src_codeset);
	perror ("Error string is: ");
	return -1;
    }
    src_left = src_len;
    dst_left = dst_len;
    src_ptr = src;
    dst_ptr = dst;
    rc = iconv (ict,&src_ptr,&src_left,&dst_ptr,&dst_left);
    if (rc == -1) {
	if (errno == EINVAL) {
	    /* EINVAL means the last character was truncated 
	       Declare success and try to continue... */
	    debug_printf ("ICONV: EINVAL\n");
	    printf ("ICONV: EINVAL\n\n");
	} else if (errno == E2BIG) {
	    /* E2BIG means the output buffer was too small.  This can 
	       happen, for example, when converting for id3v1 tags */
	    debug_printf ("ICONV: E2BIG\n");
	} else if (errno == EILSEQ) {
	    /* Here I should advance cptr and try to continue, right? */
	    debug_printf ("ICONV: EILSEQ\n");
	    printf ("ICONV: EILSEQ\n\n");
	} else {
	    debug_printf ("ICONV: ERROR %d\n", errno);
	    printf ("ICONV:ERROR %d\n\n", errno);
	}
    }
    iconv_close (ict);
    return dst_len - dst_left;
}
# endif

/* Return value is the number of char occupied by the converted string, 
   not including the null character. */
int 
string_from_wstring (char* c, int clen, wchar_t* w, const char* codeset)
{
    int rc;

# if HAVE_ICONV
    int wlen, clen_out;
    wlen = wcslen (w) * sizeof(wchar_t);
    debug_printf ("ICONV: c <- w (len=%d,tgt=%s)\n", wlen, codeset);
    rc = iconv_convert_string (c, clen, (char*) w, wlen, codeset, ICONV_WCHAR);
    debug_printf ("rc = %d\n", rc);
    clen_out = rc;
    if (clen_out == clen) clen_out--;
    c[clen_out] = 0;
    return clen_out;
# endif

    rc = wcstombs(c,w,clen);
    if (rc == -1) {
	/* Do something smart here */
    }
    return rc;
}

/* Return value is the number of wchar_t occupied by the converted string, 
   not including the null character. */
int 
wstring_from_string (wchar_t* w, int wlen, char* c, const char* codeset)
{
    int rc;

# if HAVE_ICONV
    int clen, wlen_out;
    clen = strlen (c);  // <----<<<<  GCS FIX. String is arbitrarily encoded.
    debug_printf ("ICONV: w <- c (%s)\n", c);
    rc = iconv_convert_string ((char*) w, wlen * sizeof(wchar_t), 
			       c, clen, ICONV_WCHAR, codeset);
    debug_printf ("rc = %d\n", rc);
    //    debug_mprintf (m("val = ") mS m("\n"), w);
    wlen_out = rc / sizeof(wchar_t);
    if (wlen_out == wlen) wlen_out--;
    w[wlen_out] = 0;
    return wlen_out;
# endif

    /* Currently this never happens, because now iconv is required. */
    rc = mbstowcs(w,c,wlen);
    if (rc == -1) {
	/* Do something smart here */
    }
    return 0;
}

int 
wchar_from_char (char c, const char* codeset)
{
    wchar_t w[1];
    int rc;

# if HAVE_ICONV
    rc = iconv_convert_string ((char*) w, sizeof(wchar_t), &c, 1, 
			       ICONV_WCHAR, codeset);
    if (rc == 1) return w[0];
    /* Otherwise, fall through to mbstowcs method */
# endif

    /* Do something smart here */
    return 0;
}
#endif /* HAVE_WCHAR_SUPPORT */

/* Input value mlen is measured in mchar, not bytes.
   Return value is the number of mchar occupied by the converted string, 
   not including the null character. */
int
mstring_from_string (mchar* m, int mlen, char* c, int codeset_type)
{
    if (mlen < 0) return 0;
    *m = 0;
    if (!c) return 0;
#if defined (HAVE_WCHAR_SUPPORT)
    switch (codeset_type) {
    case CODESET_UTF8:
	return wstring_from_string (m, mlen, c, "UTF-8");
	break;
    case CODESET_LOCALE:
	return wstring_from_string (m, mlen, c, m_codeset_locale);
	break;
    case CODESET_FILESYS:
	return wstring_from_string (m, mlen, c, m_codeset_filesys);
	break;
    case CODESET_ID3:
	return wstring_from_string (m, mlen, c, m_codeset_id3);
	break;
    case CODESET_METADATA:
	return wstring_from_string (m, mlen, c, m_codeset_metadata);
	break;
    case CODESET_RELAY:
	return wstring_from_string (m, mlen, c, m_codeset_relay);
	break;
    default:
	printf ("Program error.  Bad codeset m->c (%d)\n", codeset_type);
	exit (-1);
    }
#else
    sr_strncpy (m, c, mlen);
    return strlen (m);
#endif
}

/* Return value is the number of char occupied by the converted string, 
   not including the null character. */
int
string_from_mstring (char* c, int clen, mchar* m, int codeset_type)
{
    if (clen < 0) return 0;
    *c = 0;
    if (!m) return 0;
#if defined (HAVE_WCHAR_SUPPORT)
    switch (codeset_type) {
    case CODESET_UTF8:
	return string_from_wstring (c, clen, m, "UTF-8");
	break;
    case CODESET_LOCALE:
	return string_from_wstring (c, clen, m, m_codeset_locale);
	break;
    case CODESET_FILESYS:
	return string_from_wstring (c, clen, m, m_codeset_filesys);
	break;
    case CODESET_ID3:
	return string_from_wstring (c, clen, m, m_codeset_id3);
	break;
    case CODESET_METADATA:
	return string_from_wstring (c, clen, m, m_codeset_metadata);
	break;
    case CODESET_RELAY:
	return string_from_wstring (c, clen, m, m_codeset_relay);
	break;
    default:
	printf ("Program error.  Bad codeset c->m.\n");
	exit (-1);
    }
#else
    sr_strncpy (c, m, clen);
    return strlen (c);
#endif
}

mchar
mchar_from_char (char c, int codeset_type)
{
#if defined (HAVE_WCHAR_SUPPORT)
    switch (codeset_type) {
    case CODESET_UTF8:
	return wchar_from_char (c, "UTF-8");
	break;
    case CODESET_LOCALE:
	return wchar_from_char (c, m_codeset_locale);
	break;
    case CODESET_FILESYS:
	return wchar_from_char (c, m_codeset_filesys);
	break;
    case CODESET_ID3:
	return wchar_from_char (c, m_codeset_id3);
	break;
    case CODESET_METADATA:
	return wchar_from_char (c, m_codeset_metadata);
	break;
    case CODESET_RELAY:
	return wchar_from_char (c, m_codeset_relay);
	break;
    default:
	printf ("Program error.  Bad codeset c->m.\n");
	exit (-1);
    }
#else
    return c;
#endif
}

const char*
default_codeset (void)
{
    const char* fromcode = 0;

#if defined HAVE_LOCALE_CHARSET
    debug_printf ("Using locale_charset() to get system codeset.\n");
    fromcode = locale_charset ();
#elif defined HAVE_LANGINFO_CODESET
    debug_printf ("Using nl_langinfo() to get system codeset.\n");
    fromcode = nl_langinfo (CODESET);
#else
    debug_printf ("No way to get system codeset.\n");
#endif
    if (!fromcode || !fromcode[0]) {
	debug_printf ("No default codeset, using ISO-8859-1.\n");
	fromcode = "ISO-8859-1";
    } else {
	debug_printf ("Found default codeset %s\n", fromcode);
    }

#if defined (WIN32)
    {
	/* This is just for debugging */
	LCID lcid;
	lcid = GetSystemDefaultLCID ();
	debug_printf ("SystemDefaultLCID: %04x\n", lcid);
	lcid = GetUserDefaultLCID ();
	debug_printf ("UserDefaultLCID: %04x\n", lcid);
    }
#endif

#if defined HAVE_ICONV
    debug_printf ("Have iconv.\n");
#else
    debug_printf ("No iconv.\n");
#endif

    return fromcode;
}

void
set_codesets_default (CODESET_OPTIONS* cs_opt)
{
    const char* fromcode = 0;
    setlocale (LC_ALL, "");
    setlocale (LC_CTYPE, "");
    debug_printf ("LOCALE is %s\n",setlocale(LC_ALL,NULL));

    /* Set default codesets */
    fromcode = default_codeset ();
    if (fromcode) {
	strncpy (cs_opt->codeset_locale, fromcode, MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_filesys, fromcode, MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_id3, fromcode, MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_metadata, fromcode, MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_relay, fromcode, MAX_CODESET_STRING);
    }

    /* I could potentially add stuff like forcing filesys to be utf8 
       (or whatever) for osx here */
    
}

/* This sets the global variables (ugh) */
void
register_codesets (CODESET_OPTIONS* cs_opt)
{
    /* For ID3, force UCS-2, UCS-2LE, UCS-2BE, UTF-16LE, and UTF-16BE 
       to be UTF-16.  This way, we get the BOM like we need.
       This might change if we upgrade to id3v2.4, which allows 
       UTF-8 and UTF-16 without BOM. */
    if (!strncmp (cs_opt->codeset_id3, "UCS-2", strlen("UCS-2")) ||
	!strncmp (cs_opt->codeset_id3, "UTF-16", strlen("UTF-16"))) {
	strcpy (cs_opt->codeset_id3, "UTF-16");
    }

    m_codeset_locale = cs_opt->codeset_locale;
    m_codeset_filesys = cs_opt->codeset_filesys;
    m_codeset_id3 = cs_opt->codeset_id3;
    m_codeset_metadata = cs_opt->codeset_metadata;
    m_codeset_relay = cs_opt->codeset_relay;
    debug_printf ("Locale codeset: %s\n", m_codeset_locale);
    debug_printf ("Filesys codeset: %s\n", m_codeset_filesys);
    debug_printf ("ID3 codeset: %s\n", m_codeset_id3);
    debug_printf ("Metadata codeset: %s\n", m_codeset_metadata);
    debug_printf ("Relay codeset: %s\n", m_codeset_relay);


}

/* This is used to set the codeset byte for id3v2 frames */
int
is_id3_unicode (void)
{
#if HAVE_WCHAR_SUPPORT
    if (!strcmp ("UTF-16", m_codeset_id3)) {
	return 1;
    }
#endif
    return 0;
}

void
mstrncpy (mchar* dst, mchar* src, int n)
{
    int i = 0;
    for (i = 0; i < n-1; i++) {
	if (!(dst[i] = src[i])) {
	    return;
	}
    }
    dst[i] = 0;
}

mchar* 
mstrdup (mchar* src)
{
#if defined HAVE_WCHAR_SUPPORT
    /* wstrdup/wcsdup is non-standard */
    mchar* new_string = (mchar*) malloc (sizeof(mchar)*(wcslen(src) + 1));
    wcscpy (new_string, src);
    return new_string;
#else
    return strdup (src);
#endif
}

mchar* 
mstrcpy (mchar* dest, const mchar* src)
{
#if defined HAVE_WCHAR_SUPPORT
    return wcscpy (dest, src);
#else
    return strcpy (dest, src);
#endif
}

size_t
mstrlen (mchar* s)
{
#if defined HAVE_WCHAR_SUPPORT
    return wcslen (s);
#else
    return strlen (s);
#endif
}

/* GCS FIX: gcc can give a warning about vswprintf.  This may require 
   setting gcc -std=c99, or gcc -lang-c99 */
int
msnprintf (mchar* dest, size_t n, const mchar* fmt, ...)
{
    int rc;
    va_list ap;
    va_start (ap, fmt);
#if defined HAVE_WCHAR_SUPPORT
    rc = vswprintf (dest, n, fmt, ap);
    debug_printf ("vswprintf got %d\n", rc);
#else
    rc = vsnprintf (dest, n, fmt, ap);
#endif
    va_end (ap);
    return rc;
}

mchar*
mstrchr (const mchar* ws, mchar wc)
{
#if defined HAVE_WCHAR_SUPPORT
    return wcschr (ws, wc);
#else
    return strchr (ws, wc);
#endif
}

mchar*
mstrncat (mchar* ws1, const mchar* ws2, size_t n)
{
#if defined HAVE_WCHAR_SUPPORT
    return wcsncat (ws1, ws2, n);
#else
    return strncat (ws1, ws2, n);
#endif
}

int
mstrcmp (const mchar* ws1, const mchar* ws2)
{
#if defined HAVE_WCHAR_SUPPORT
    return wcscmp (ws1, ws2);
#else
    return strcmp (ws1, ws2);
#endif
}

long int
mtol (const mchar* string)
{
#if defined HAVE_WCHAR_SUPPORT
    return wcstol (string, 0, 0);
#else
    return strtol (string, 0, 0);
#endif
}
