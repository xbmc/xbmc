/* A GNU-like <string.h>.

   Copyright (C) 1995-1996, 2001-2009 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GL_STRING_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_STRING_H@

#ifndef _GL_STRING_H
#define _GL_STRING_H


#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#  define __attribute__(Spec) /* empty */
# endif
/* The attribute __pure__ was added in gcc 2.96.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#  define __pure__ /* empty */
# endif
#endif


/* The definition of GL_LINK_WARNING is copied here.  */


#ifdef __cplusplus
extern "C" {
#endif


/* Return the first instance of C within N bytes of S, or NULL.  */
#if @GNULIB_MEMCHR@
# if @REPLACE_MEMCHR@
#  define memchr rpl_memchr
extern void *memchr (void const *__s, int __c, size_t __n)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
# undef memchr
# define memchr(s,c,n) \
    (GL_LINK_WARNING ("memchr has platform-specific bugs - " \
                      "use gnulib module memchr for portability" ), \
     memchr (s, c, n))
#endif

/* Return the first occurrence of NEEDLE in HAYSTACK.  */
#if @GNULIB_MEMMEM@
# if @REPLACE_MEMMEM@
#  define memmem rpl_memmem
# endif
# if ! @HAVE_DECL_MEMMEM@ || @REPLACE_MEMMEM@
extern void *memmem (void const *__haystack, size_t __haystack_len,
		     void const *__needle, size_t __needle_len)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
# undef memmem
# define memmem(a,al,b,bl) \
    (GL_LINK_WARNING ("memmem is unportable and often quadratic - " \
                      "use gnulib module memmem-simple for portability, " \
                      "and module memmem for speed" ), \
     memmem (a, al, b, bl))
#endif

/* Copy N bytes of SRC to DEST, return pointer to bytes after the
   last written byte.  */
#if @GNULIB_MEMPCPY@
# if ! @HAVE_MEMPCPY@
extern void *mempcpy (void *restrict __dest, void const *restrict __src,
		      size_t __n);
# endif
#elif defined GNULIB_POSIXCHECK
# undef mempcpy
# define mempcpy(a,b,n) \
    (GL_LINK_WARNING ("mempcpy is unportable - " \
                      "use gnulib module mempcpy for portability"), \
     mempcpy (a, b, n))
#endif

/* Search backwards through a block for a byte (specified as an int).  */
#if @GNULIB_MEMRCHR@
# if ! @HAVE_DECL_MEMRCHR@
extern void *memrchr (void const *, int, size_t)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
# undef memrchr
# define memrchr(a,b,c) \
    (GL_LINK_WARNING ("memrchr is unportable - " \
                      "use gnulib module memrchr for portability"), \
     memrchr (a, b, c))
#endif

/* Find the first occurrence of C in S.  More efficient than
   memchr(S,C,N), at the expense of undefined behavior if C does not
   occur within N bytes.  */
#if @GNULIB_RAWMEMCHR@
# if ! @HAVE_RAWMEMCHR@
extern void *rawmemchr (void const *__s, int __c_in)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
# undef rawmemchr
# define rawmemchr(a,b) \
    (GL_LINK_WARNING ("rawmemchr is unportable - " \
                      "use gnulib module rawmemchr for portability"), \
     rawmemchr (a, b))
#endif

/* Copy SRC to DST, returning the address of the terminating '\0' in DST.  */
#if @GNULIB_STPCPY@
# if ! @HAVE_STPCPY@
extern char *stpcpy (char *restrict __dst, char const *restrict __src);
# endif
#elif defined GNULIB_POSIXCHECK
# undef stpcpy
# define stpcpy(a,b) \
    (GL_LINK_WARNING ("stpcpy is unportable - " \
                      "use gnulib module stpcpy for portability"), \
     stpcpy (a, b))
#endif

/* Copy no more than N bytes of SRC to DST, returning a pointer past the
   last non-NUL byte written into DST.  */
#if @GNULIB_STPNCPY@
# if ! @HAVE_STPNCPY@
#  define stpncpy gnu_stpncpy
extern char *stpncpy (char *restrict __dst, char const *restrict __src,
		      size_t __n);
# endif
#elif defined GNULIB_POSIXCHECK
# undef stpncpy
# define stpncpy(a,b,n) \
    (GL_LINK_WARNING ("stpncpy is unportable - " \
                      "use gnulib module stpncpy for portability"), \
     stpncpy (a, b, n))
#endif

#if defined GNULIB_POSIXCHECK
/* strchr() does not work with multibyte strings if the locale encoding is
   GB18030 and the character to be searched is a digit.  */
# undef strchr
# define strchr(s,c) \
    (GL_LINK_WARNING ("strchr cannot work correctly on character strings " \
                      "in some multibyte locales - " \
                      "use mbschr if you care about internationalization"), \
     strchr (s, c))
#endif

/* Find the first occurrence of C in S or the final NUL byte.  */
#if @GNULIB_STRCHRNUL@
# if ! @HAVE_STRCHRNUL@
extern char *strchrnul (char const *__s, int __c_in)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
# undef strchrnul
# define strchrnul(a,b) \
    (GL_LINK_WARNING ("strchrnul is unportable - " \
                      "use gnulib module strchrnul for portability"), \
     strchrnul (a, b))
#endif

/* Duplicate S, returning an identical malloc'd string.  */
#if @GNULIB_STRDUP@
# if @REPLACE_STRDUP@
#  undef strdup
#  define strdup rpl_strdup
# endif
# if !(@HAVE_DECL_STRDUP@ || defined strdup) || @REPLACE_STRDUP@
extern char *strdup (char const *__s);
# endif
#elif defined GNULIB_POSIXCHECK
# undef strdup
# define strdup(a) \
    (GL_LINK_WARNING ("strdup is unportable - " \
                      "use gnulib module strdup for portability"), \
     strdup (a))
#endif

/* Return a newly allocated copy of at most N bytes of STRING.  */
#if @GNULIB_STRNDUP@
# if ! @HAVE_STRNDUP@
#  undef strndup
#  define strndup rpl_strndup
# endif
# if ! @HAVE_STRNDUP@ || ! @HAVE_DECL_STRNDUP@
extern char *strndup (char const *__string, size_t __n);
# endif
#elif defined GNULIB_POSIXCHECK
# undef strndup
# define strndup(a,n) \
    (GL_LINK_WARNING ("strndup is unportable - " \
                      "use gnulib module strndup for portability"), \
     strndup (a, n))
#endif

/* Find the length (number of bytes) of STRING, but scan at most
   MAXLEN bytes.  If no '\0' terminator is found in that many bytes,
   return MAXLEN.  */
#if @GNULIB_STRNLEN@
# if ! @HAVE_DECL_STRNLEN@
extern size_t strnlen (char const *__string, size_t __maxlen)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
# undef strnlen
# define strnlen(a,n) \
    (GL_LINK_WARNING ("strnlen is unportable - " \
                      "use gnulib module strnlen for portability"), \
     strnlen (a, n))
#endif

#if defined GNULIB_POSIXCHECK
/* strcspn() assumes the second argument is a list of single-byte characters.
   Even in this simple case, it does not work with multibyte strings if the
   locale encoding is GB18030 and one of the characters to be searched is a
   digit.  */
# undef strcspn
# define strcspn(s,a) \
    (GL_LINK_WARNING ("strcspn cannot work correctly on character strings " \
                      "in multibyte locales - " \
                      "use mbscspn if you care about internationalization"), \
     strcspn (s, a))
#endif

/* Find the first occurrence in S of any character in ACCEPT.  */
#if @GNULIB_STRPBRK@
# if ! @HAVE_STRPBRK@
extern char *strpbrk (char const *__s, char const *__accept)
  __attribute__ ((__pure__));
# endif
# if defined GNULIB_POSIXCHECK
/* strpbrk() assumes the second argument is a list of single-byte characters.
   Even in this simple case, it does not work with multibyte strings if the
   locale encoding is GB18030 and one of the characters to be searched is a
   digit.  */
#  undef strpbrk
#  define strpbrk(s,a) \
     (GL_LINK_WARNING ("strpbrk cannot work correctly on character strings " \
                       "in multibyte locales - " \
                       "use mbspbrk if you care about internationalization"), \
      strpbrk (s, a))
# endif
#elif defined GNULIB_POSIXCHECK
# undef strpbrk
# define strpbrk(s,a) \
    (GL_LINK_WARNING ("strpbrk is unportable - " \
                      "use gnulib module strpbrk for portability"), \
     strpbrk (s, a))
#endif

#if defined GNULIB_POSIXCHECK
/* strspn() assumes the second argument is a list of single-byte characters.
   Even in this simple case, it cannot work with multibyte strings.  */
# undef strspn
# define strspn(s,a) \
    (GL_LINK_WARNING ("strspn cannot work correctly on character strings " \
                      "in multibyte locales - " \
                      "use mbsspn if you care about internationalization"), \
     strspn (s, a))
#endif

#if defined GNULIB_POSIXCHECK
/* strrchr() does not work with multibyte strings if the locale encoding is
   GB18030 and the character to be searched is a digit.  */
# undef strrchr
# define strrchr(s,c) \
    (GL_LINK_WARNING ("strrchr cannot work correctly on character strings " \
                      "in some multibyte locales - " \
                      "use mbsrchr if you care about internationalization"), \
     strrchr (s, c))
#endif

/* Search the next delimiter (char listed in DELIM) starting at *STRINGP.
   If one is found, overwrite it with a NUL, and advance *STRINGP
   to point to the next char after it.  Otherwise, set *STRINGP to NULL.
   If *STRINGP was already NULL, nothing happens.
   Return the old value of *STRINGP.

   This is a variant of strtok() that is multithread-safe and supports
   empty fields.

   Caveat: It modifies the original string.
   Caveat: These functions cannot be used on constant strings.
   Caveat: The identity of the delimiting character is lost.
   Caveat: It doesn't work with multibyte strings unless all of the delimiter
           characters are ASCII characters < 0x30.

   See also strtok_r().  */
#if @GNULIB_STRSEP@
# if ! @HAVE_STRSEP@
extern char *strsep (char **restrict __stringp, char const *restrict __delim);
# endif
# if defined GNULIB_POSIXCHECK
#  undef strsep
#  define strsep(s,d) \
     (GL_LINK_WARNING ("strsep cannot work correctly on character strings " \
                       "in multibyte locales - " \
                       "use mbssep if you care about internationalization"), \
      strsep (s, d))
# endif
#elif defined GNULIB_POSIXCHECK
# undef strsep
# define strsep(s,d) \
    (GL_LINK_WARNING ("strsep is unportable - " \
                      "use gnulib module strsep for portability"), \
     strsep (s, d))
#endif

#if @GNULIB_STRSTR@
# if @REPLACE_STRSTR@
#  define strstr rpl_strstr
char *strstr (const char *haystack, const char *needle)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
/* strstr() does not work with multibyte strings if the locale encoding is
   different from UTF-8:
   POSIX says that it operates on "strings", and "string" in POSIX is defined
   as a sequence of bytes, not of characters.  */
# undef strstr
# define strstr(a,b) \
    (GL_LINK_WARNING ("strstr is quadratic on many systems, and cannot " \
                      "work correctly on character strings in most "    \
                      "multibyte locales - " \
                      "use mbsstr if you care about internationalization, " \
                      "or use strstr if you care about speed"), \
     strstr (a, b))
#endif

/* Find the first occurrence of NEEDLE in HAYSTACK, using case-insensitive
   comparison.  */
#if @GNULIB_STRCASESTR@
# if @REPLACE_STRCASESTR@
#  define strcasestr rpl_strcasestr
# endif
# if ! @HAVE_STRCASESTR@ || @REPLACE_STRCASESTR@
extern char *strcasestr (const char *haystack, const char *needle)
  __attribute__ ((__pure__));
# endif
#elif defined GNULIB_POSIXCHECK
/* strcasestr() does not work with multibyte strings:
   It is a glibc extension, and glibc implements it only for unibyte
   locales.  */
# undef strcasestr
# define strcasestr(a,b) \
    (GL_LINK_WARNING ("strcasestr does work correctly on character strings " \
                      "in multibyte locales - " \
                      "use mbscasestr if you care about " \
                      "internationalization, or use c-strcasestr if you want " \
                      "a locale independent function"), \
     strcasestr (a, b))
#endif

/* Parse S into tokens separated by characters in DELIM.
   If S is NULL, the saved pointer in SAVE_PTR is used as
   the next starting point.  For example:
	char s[] = "-abc-=-def";
	char *sp;
	x = strtok_r(s, "-", &sp);	// x = "abc", sp = "=-def"
	x = strtok_r(NULL, "-=", &sp);	// x = "def", sp = NULL
	x = strtok_r(NULL, "=", &sp);	// x = NULL
		// s = "abc\0-def\0"

   This is a variant of strtok() that is multithread-safe.

   For the POSIX documentation for this function, see:
   http://www.opengroup.org/susv3xsh/strtok.html

   Caveat: It modifies the original string.
   Caveat: These functions cannot be used on constant strings.
   Caveat: The identity of the delimiting character is lost.
   Caveat: It doesn't work with multibyte strings unless all of the delimiter
           characters are ASCII characters < 0x30.

   See also strsep().  */
#if @GNULIB_STRTOK_R@
# if ! @HAVE_DECL_STRTOK_R@
extern char *strtok_r (char *restrict s, char const *restrict delim,
		       char **restrict save_ptr);
# endif
# if defined GNULIB_POSIXCHECK
#  undef strtok_r
#  define strtok_r(s,d,p) \
     (GL_LINK_WARNING ("strtok_r cannot work correctly on character strings " \
                       "in multibyte locales - " \
                       "use mbstok_r if you care about internationalization"), \
      strtok_r (s, d, p))
# endif
#elif defined GNULIB_POSIXCHECK
# undef strtok_r
# define strtok_r(s,d,p) \
    (GL_LINK_WARNING ("strtok_r is unportable - " \
                      "use gnulib module strtok_r for portability"), \
     strtok_r (s, d, p))
#endif


/* The following functions are not specified by POSIX.  They are gnulib
   extensions.  */

#if @GNULIB_MBSLEN@
/* Return the number of multibyte characters in the character string STRING.
   This considers multibyte characters, unlike strlen, which counts bytes.  */
extern size_t mbslen (const char *string);
#endif

#if @GNULIB_MBSNLEN@
/* Return the number of multibyte characters in the character string starting
   at STRING and ending at STRING + LEN.  */
extern size_t mbsnlen (const char *string, size_t len);
#endif

#if @GNULIB_MBSCHR@
/* Locate the first single-byte character C in the character string STRING,
   and return a pointer to it.  Return NULL if C is not found in STRING.
   Unlike strchr(), this function works correctly in multibyte locales with
   encodings such as GB18030.  */
# define mbschr rpl_mbschr /* avoid collision with HP-UX function */
extern char * mbschr (const char *string, int c);
#endif

#if @GNULIB_MBSRCHR@
/* Locate the last single-byte character C in the character string STRING,
   and return a pointer to it.  Return NULL if C is not found in STRING.
   Unlike strrchr(), this function works correctly in multibyte locales with
   encodings such as GB18030.  */
# define mbsrchr rpl_mbsrchr /* avoid collision with HP-UX function */
extern char * mbsrchr (const char *string, int c);
#endif

#if @GNULIB_MBSSTR@
/* Find the first occurrence of the character string NEEDLE in the character
   string HAYSTACK.  Return NULL if NEEDLE is not found in HAYSTACK.
   Unlike strstr(), this function works correctly in multibyte locales with
   encodings different from UTF-8.  */
extern char * mbsstr (const char *haystack, const char *needle);
#endif

#if @GNULIB_MBSCASECMP@
/* Compare the character strings S1 and S2, ignoring case, returning less than,
   equal to or greater than zero if S1 is lexicographically less than, equal to
   or greater than S2.
   Note: This function may, in multibyte locales, return 0 for strings of
   different lengths!
   Unlike strcasecmp(), this function works correctly in multibyte locales.  */
extern int mbscasecmp (const char *s1, const char *s2);
#endif

#if @GNULIB_MBSNCASECMP@
/* Compare the initial segment of the character string S1 consisting of at most
   N characters with the initial segment of the character string S2 consisting
   of at most N characters, ignoring case, returning less than, equal to or
   greater than zero if the initial segment of S1 is lexicographically less
   than, equal to or greater than the initial segment of S2.
   Note: This function may, in multibyte locales, return 0 for initial segments
   of different lengths!
   Unlike strncasecmp(), this function works correctly in multibyte locales.
   But beware that N is not a byte count but a character count!  */
extern int mbsncasecmp (const char *s1, const char *s2, size_t n);
#endif

#if @GNULIB_MBSPCASECMP@
/* Compare the initial segment of the character string STRING consisting of
   at most mbslen (PREFIX) characters with the character string PREFIX,
   ignoring case, returning less than, equal to or greater than zero if this
   initial segment is lexicographically less than, equal to or greater than
   PREFIX.
   Note: This function may, in multibyte locales, return 0 if STRING is of
   smaller length than PREFIX!
   Unlike strncasecmp(), this function works correctly in multibyte
   locales.  */
extern char * mbspcasecmp (const char *string, const char *prefix);
#endif

#if @GNULIB_MBSCASESTR@
/* Find the first occurrence of the character string NEEDLE in the character
   string HAYSTACK, using case-insensitive comparison.
   Note: This function may, in multibyte locales, return success even if
   strlen (haystack) < strlen (needle) !
   Unlike strcasestr(), this function works correctly in multibyte locales.  */
extern char * mbscasestr (const char *haystack, const char *needle);
#endif

#if @GNULIB_MBSCSPN@
/* Find the first occurrence in the character string STRING of any character
   in the character string ACCEPT.  Return the number of bytes from the
   beginning of the string to this occurrence, or to the end of the string
   if none exists.
   Unlike strcspn(), this function works correctly in multibyte locales.  */
extern size_t mbscspn (const char *string, const char *accept);
#endif

#if @GNULIB_MBSPBRK@
/* Find the first occurrence in the character string STRING of any character
   in the character string ACCEPT.  Return the pointer to it, or NULL if none
   exists.
   Unlike strpbrk(), this function works correctly in multibyte locales.  */
# define mbspbrk rpl_mbspbrk /* avoid collision with HP-UX function */
extern char * mbspbrk (const char *string, const char *accept);
#endif

#if @GNULIB_MBSSPN@
/* Find the first occurrence in the character string STRING of any character
   not in the character string REJECT.  Return the number of bytes from the
   beginning of the string to this occurrence, or to the end of the string
   if none exists.
   Unlike strspn(), this function works correctly in multibyte locales.  */
extern size_t mbsspn (const char *string, const char *reject);
#endif

#if @GNULIB_MBSSEP@
/* Search the next delimiter (multibyte character listed in the character
   string DELIM) starting at the character string *STRINGP.
   If one is found, overwrite it with a NUL, and advance *STRINGP to point
   to the next multibyte character after it.  Otherwise, set *STRINGP to NULL.
   If *STRINGP was already NULL, nothing happens.
   Return the old value of *STRINGP.

   This is a variant of mbstok_r() that supports empty fields.

   Caveat: It modifies the original string.
   Caveat: These functions cannot be used on constant strings.
   Caveat: The identity of the delimiting character is lost.

   See also mbstok_r().  */
extern char * mbssep (char **stringp, const char *delim);
#endif

#if @GNULIB_MBSTOK_R@
/* Parse the character string STRING into tokens separated by characters in
   the character string DELIM.
   If STRING is NULL, the saved pointer in SAVE_PTR is used as
   the next starting point.  For example:
	char s[] = "-abc-=-def";
	char *sp;
	x = mbstok_r(s, "-", &sp);	// x = "abc", sp = "=-def"
	x = mbstok_r(NULL, "-=", &sp);	// x = "def", sp = NULL
	x = mbstok_r(NULL, "=", &sp);	// x = NULL
		// s = "abc\0-def\0"

   Caveat: It modifies the original string.
   Caveat: These functions cannot be used on constant strings.
   Caveat: The identity of the delimiting character is lost.

   See also mbssep().  */
extern char * mbstok_r (char *string, const char *delim, char **save_ptr);
#endif

/* Map any int, typically from errno, into an error message.  */
#if @GNULIB_STRERROR@
# if @REPLACE_STRERROR@
#  undef strerror
#  define strerror rpl_strerror
extern char *strerror (int);
# endif
#elif defined GNULIB_POSIXCHECK
# undef strerror
# define strerror(e) \
    (GL_LINK_WARNING ("strerror is unportable - " \
                      "use gnulib module strerror to guarantee non-NULL result"), \
     strerror (e))
#endif

#if @GNULIB_STRSIGNAL@
# if @REPLACE_STRSIGNAL@
#  define strsignal rpl_strsignal
# endif
# if ! @HAVE_DECL_STRSIGNAL@ || @REPLACE_STRSIGNAL@
extern char *strsignal (int __sig);
# endif
#elif defined GNULIB_POSIXCHECK
# undef strsignal
# define strsignal(a) \
    (GL_LINK_WARNING ("strsignal is unportable - " \
                      "use gnulib module strsignal for portability"), \
     strsignal (a))
#endif

#if @GNULIB_STRVERSCMP@
# if !@HAVE_STRVERSCMP@
extern int strverscmp (const char *, const char *);
# endif
#elif defined GNULIB_POSIXCHECK
# undef strverscmp
# define strverscmp(a, b) \
    (GL_LINK_WARNING ("strverscmp is unportable - " \
                      "use gnulib module strverscmp for portability"), \
     strverscmp (a, b))
#endif


#ifdef __cplusplus
}
#endif

#endif /* _GL_STRING_H */
#endif /* _GL_STRING_H */
