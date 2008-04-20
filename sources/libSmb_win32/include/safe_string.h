/* 
   Unix SMB/CIFS implementation.
   Safe string handling routines.
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2003
   
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

#ifndef _SAFE_STRING_H
#define _SAFE_STRING_H

#ifndef _SPLINT_ /* http://www.splint.org */

/* Some macros to ensure people don't use buffer overflow vulnerable string
   functions. */

#ifdef bcopy
#undef bcopy
#endif /* bcopy */
#define bcopy(src,dest,size) __ERROR__XX__NEVER_USE_BCOPY___;

#ifdef strcpy
#undef strcpy
#endif /* strcpy */
#define strcpy(dest,src) __ERROR__XX__NEVER_USE_STRCPY___;

#ifdef strcat
#undef strcat
#endif /* strcat */
#define strcat(dest,src) __ERROR__XX__NEVER_USE_STRCAT___;

#ifdef sprintf
#undef sprintf
#endif /* sprintf */
#define sprintf __ERROR__XX__NEVER_USE_SPRINTF__;

/*
 * strcasecmp/strncasecmp aren't an error, but it means you're not thinking about
 * multibyte. Don't use them. JRA.
 */
#ifdef strcasecmp
#undef strcasecmp
#endif
#define strcasecmp __ERROR__XX__NEVER_USE_STRCASECMP__;

#ifdef strncasecmp
#undef strncasecmp
#endif
#define strncasecmp __ERROR__XX__NEVER_USE_STRNCASECMP__;

#endif /* !_SPLINT_ */

#ifdef DEVELOPER
#define SAFE_STRING_FUNCTION_NAME FUNCTION_MACRO
#define SAFE_STRING_LINE __LINE__
#else
#define SAFE_STRING_FUNCTION_NAME ("")
#define SAFE_STRING_LINE (0)
#endif

/* We need a number of different prototypes for our 
   non-existant fuctions */
char * __unsafe_string_function_usage_here__(void);

size_t __unsafe_string_function_usage_here_size_t__(void);

size_t __unsafe_string_function_usage_here_char__(void);

#ifdef HAVE_COMPILER_WILL_OPTIMIZE_OUT_FNS

/* if the compiler will optimize out function calls, then use this to tell if we are 
   have the correct types (this works only where sizeof() returns the size of the buffer, not
   the size of the pointer). */

#define CHECK_STRING_SIZE(d, len) (sizeof(d) != (len) && sizeof(d) != sizeof(char *))

#define fstrterminate(d) (CHECK_STRING_SIZE(d, sizeof(fstring)) \
    ? __unsafe_string_function_usage_here_char__() \
    : (((d)[sizeof(fstring)-1]) = '\0'))
#define pstrterminate(d) (CHECK_STRING_SIZE(d, sizeof(pstring)) \
    ? __unsafe_string_function_usage_here_char__() \
    : (((d)[sizeof(pstring)-1]) = '\0'))

#define wpstrcpy(d,s) ((sizeof(d) != sizeof(wpstring) && sizeof(d) != sizeof(smb_ucs2_t *)) \
    ? __unsafe_string_function_usage_here__() \
    : safe_strcpy_w((d),(s),sizeof(wpstring)))
#define wpstrcat(d,s) ((sizeof(d) != sizeof(wpstring) && sizeof(d) != sizeof(smb_ucs2_t *)) \
    ? __unsafe_string_function_usage_here__() \
    : safe_strcat_w((d),(s),sizeof(wpstring)))
#define wfstrcpy(d,s) ((sizeof(d) != sizeof(wfstring) && sizeof(d) != sizeof(smb_ucs2_t *)) \
    ? __unsafe_string_function_usage_here__() \
    : safe_strcpy_w((d),(s),sizeof(wfstring)))
#define wfstrcat(d,s) ((sizeof(d) != sizeof(wfstring) && sizeof(d) != sizeof(smb_ucs2_t *)) \
    ? __unsafe_string_function_usage_here__() \
    : safe_strcat_w((d),(s),sizeof(wfstring)))

#define push_pstring_base(dest, src, pstring_base) \
    (CHECK_STRING_SIZE(pstring_base, sizeof(pstring)) \
    ? __unsafe_string_function_usage_here_size_t__() \
    : push_ascii(dest, src, sizeof(pstring)-PTR_DIFF(dest,pstring_base)-1, STR_TERMINATE))

#else /* HAVE_COMPILER_WILL_OPTIMIZE_OUT_FNS */

#define fstrterminate(d) (((d)[sizeof(fstring)-1]) = '\0')
#define pstrterminate(d) (((d)[sizeof(pstring)-1]) = '\0')

#define wpstrcpy(d,s) safe_strcpy_w((d),(s),sizeof(wpstring))
#define wpstrcat(d,s) safe_strcat_w((d),(s),sizeof(wpstring))
#define wfstrcpy(d,s) safe_strcpy_w((d),(s),sizeof(wfstring))
#define wfstrcat(d,s) safe_strcat_w((d),(s),sizeof(wfstring))

#define push_pstring_base(dest, src, pstring_base) \
    push_ascii(dest, src, sizeof(pstring)-PTR_DIFF(dest,pstring_base)-1, STR_TERMINATE)

#endif /* HAVE_COMPILER_WILL_OPTIMIZE_OUT_FNS */

#define safe_strcpy_base(dest, src, base, size) \
    safe_strcpy(dest, src, size-PTR_DIFF(dest,base)-1)

/* String copy functions - macro hell below adds 'type checking' (limited,
   but the best we can do in C) and may tag with function name/number to
   record the last 'clobber region' on that string */

#define pstrcpy(d,s) safe_strcpy((d), (s),sizeof(pstring)-1)
#define pstrcat(d,s) safe_strcat((d), (s),sizeof(pstring)-1)
#define fstrcpy(d,s) safe_strcpy((d),(s),sizeof(fstring)-1)
#define fstrcat(d,s) safe_strcat((d),(s),sizeof(fstring)-1)
#define nstrcpy(d,s) safe_strcpy((d), (s),sizeof(nstring)-1)
#define unstrcpy(d,s) safe_strcpy((d), (s),sizeof(unstring)-1)

/* the addition of the DEVELOPER checks in safe_strcpy means we must
 * update a lot of code. To make this a little easier here are some
 * functions that provide the lengths with less pain */
#define pstrcpy_base(dest, src, pstring_base) \
    safe_strcpy(dest, src, sizeof(pstring)-PTR_DIFF(dest,pstring_base)-1)


/* Inside the _fn variants of these is a call to clobber_region(), -
 * which might destroy the stack on a buggy function.  We help the
 * debugging process by putting the function and line who last caused
 * a clobbering into a static buffer.  If the program crashes at
 * address 0xf1f1f1f1 then this function is probably, but not
 * necessarily, to blame. */

/* overmalloc_safe_strcpy: DEPRECATED!  Used when you know the
 * destination buffer is longer than maxlength, but you don't know how
 * long.  This is not a good situation, because we can't do the normal
 * sanity checks. Don't use in new code! */

#define overmalloc_safe_strcpy(dest,src,maxlength)	safe_strcpy_fn(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE,dest,src,maxlength)
#define safe_strcpy(dest,src,maxlength)	safe_strcpy_fn2(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE,dest,src,maxlength)
#define safe_strcat(dest,src,maxlength)	safe_strcat_fn2(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE,dest,src,maxlength)
#define push_string(base_ptr, dest, src, dest_len, flags) push_string_fn2(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, base_ptr, dest, src, dest_len, flags)
#define pull_string(base_ptr, dest, src, dest_len, src_len, flags) pull_string_fn2(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, base_ptr, dest, src, dest_len, src_len, flags)
#define clistr_push(cli, dest, src, dest_len, flags) clistr_push_fn2(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, cli, dest, src, dest_len, flags)
#define clistr_pull(cli, dest, src, dest_len, src_len, flags) clistr_pull_fn2(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, cli, dest, src, dest_len, src_len, flags)
#define srvstr_push(base_ptr, dest, src, dest_len, flags) srvstr_push_fn2(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, base_ptr, dest, src, dest_len, flags)

#define alpha_strcpy(dest,src,other_safe_chars,maxlength) alpha_strcpy_fn(SAFE_STRING_FUNCTION_NAME,SAFE_STRING_LINE,dest,src,other_safe_chars,maxlength)
#define StrnCpy(dest,src,n)		StrnCpy_fn(SAFE_STRING_FUNCTION_NAME,SAFE_STRING_LINE,dest,src,n)

#ifdef HAVE_COMPILER_WILL_OPTIMIZE_OUT_FNS

/* if the compiler will optimize out function calls, then use this to tell if we are 
   have the correct types (this works only where sizeof() returns the size of the buffer, not
   the size of the pointer). */

#define safe_strcpy_fn2(fn_name, fn_line, d, s, max_len) \
    (CHECK_STRING_SIZE(d, max_len+1) \
    ? __unsafe_string_function_usage_here__() \
    : safe_strcpy_fn(fn_name, fn_line, (d), (s), (max_len)))

#define safe_strcat_fn2(fn_name, fn_line, d, s, max_len) \
    (CHECK_STRING_SIZE(d, max_len+1) \
    ? __unsafe_string_function_usage_here__() \
    : safe_strcat_fn(fn_name, fn_line, (d), (s), (max_len)))

#define push_string_fn2(fn_name, fn_line, base_ptr, dest, src, dest_len, flags) \
    (CHECK_STRING_SIZE(dest, dest_len) \
    ? __unsafe_string_function_usage_here_size_t__() \
    : push_string_fn(fn_name, fn_line, base_ptr, dest, src, dest_len, flags))

#define pull_string_fn2(fn_name, fn_line, base_ptr, dest, src, dest_len, src_len, flags) \
    (CHECK_STRING_SIZE(dest, dest_len) \
    ? __unsafe_string_function_usage_here_size_t__() \
    : pull_string_fn(fn_name, fn_line, base_ptr, dest, src, dest_len, src_len, flags))

#define clistr_push_fn2(fn_name, fn_line, cli, dest, src, dest_len, flags) \
    (CHECK_STRING_SIZE(dest, dest_len) \
    ? __unsafe_string_function_usage_here_size_t__() \
    : clistr_push_fn(fn_name, fn_line, cli, dest, src, dest_len, flags))

#define clistr_pull_fn2(fn_name, fn_line, cli, dest, src, dest_len, srclen, flags) \
    (CHECK_STRING_SIZE(dest, dest_len) \
    ? __unsafe_string_function_usage_here_size_t__() \
    : clistr_pull_fn(fn_name, fn_line, cli, dest, src, dest_len, srclen, flags))

#define srvstr_push_fn2(fn_name, fn_line, base_ptr, dest, src, dest_len, flags) \
    (CHECK_STRING_SIZE(dest, dest_len) \
    ? __unsafe_string_function_usage_here_size_t__() \
    : srvstr_push_fn(fn_name, fn_line, base_ptr, dest, src, dest_len, flags))

#else

#define safe_strcpy_fn2 safe_strcpy_fn
#define safe_strcat_fn2 safe_strcat_fn
#define push_string_fn2 push_string_fn
#define pull_string_fn2 pull_string_fn
#define clistr_push_fn2 clistr_push_fn
#define clistr_pull_fn2 clistr_pull_fn
#define srvstr_push_fn2 srvstr_push_fn

#endif

#endif
