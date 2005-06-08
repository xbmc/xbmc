/*
 * stdio.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Definitions of types and prototypes of functions for standard input and
 * output.
 *
 * NOTE: The file manipulation functions provided by Microsoft seem to
 * work with either slash (/) or backslash (\) as the directory separator.
 *
 */

#ifndef _STDIO_H_
#define	_STDIO_H_

/* All the headers include this file. */
#include <_mingw.h>

#ifndef RC_INVOKED
#define __need_size_t
#define __need_NULL
#define __need_wchar_t
#define	__need_wint_t
#include <stddef.h>
#define __need___va_list
#include <stdarg.h>
#endif	/* Not RC_INVOKED */


/* Flags for the iobuf structure  */
#define	_IOREAD	1 /* currently reading */
#define	_IOWRT	2 /* currently writing */
#define	_IORW	0x0080 /* opened as "r+w" */


/*
 * The three standard file pointers provided by the run time library.
 * NOTE: These will go to the bit-bucket silently in GUI applications!
 */
#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2

/* Returned by various functions on end of file condition or error. */
#define	EOF	(-1)

/*
 * The maximum length of a file name. You should use GetVolumeInformation
 * instead of this constant. But hey, this works.
 * Also defined in io.h.
 */
#ifndef FILENAME_MAX
#define	FILENAME_MAX	(260)
#endif

/*
 * The maximum number of files that may be open at once. I have set this to
 * a conservative number. The actual value may be higher.
 */
#define FOPEN_MAX	(20)

/* After creating this many names, tmpnam and tmpfile return NULL */
#define TMP_MAX	32767
/*
 * Tmpnam, tmpfile and, sometimes, _tempnam try to create
 * temp files in the root directory of the current drive
 * (not in pwd, as suggested by some older MS doc's).
 * Redefining these macros does not effect the CRT functions.
 */
#define _P_tmpdir   "\\"
#ifndef __STRICT_ANSI__
#define P_tmpdir _P_tmpdir
#endif
#define _wP_tmpdir  L"\\"

/*
 * The maximum size of name (including NUL) that will be put in the user
 * supplied buffer caName for tmpnam.
 * Inferred from the size of the static buffer returned by tmpnam
 * when passed a NULL argument. May actually be smaller.
 */
#define L_tmpnam (16)

#define _IOFBF    0x0000  /* full buffered */
#define _IOLBF    0x0040  /* line buffered */
#define _IONBF    0x0004  /* not buffered */

#define _IOMYBUF  0x0008  /* stdio malloc()'d buffer */
#define _IOEOF    0x0010  /* EOF reached on read */
#define _IOERR    0x0020  /* I/O error from system */
#define _IOSTRG   0x0040  /* Strange or no file descriptor */
#ifdef _POSIX_SOURCE
# define _IOAPPEND 0x0200
#endif
/*
 * The buffer size as used by setbuf such that it is equivalent to
 * (void) setvbuf(fileSetBuffer, caBuffer, _IOFBF, BUFSIZ).
 */
#define	BUFSIZ	512

/* Constants for nOrigin indicating the position relative to which fseek
 * sets the file position. Enclosed in ifdefs because io.h could also
 * define them. (Though not anymore since io.h includes this file now.) */
#ifndef	SEEK_SET
#define SEEK_SET	(0)
#endif

#ifndef	SEEK_CUR
#define	SEEK_CUR	(1)
#endif

#ifndef	SEEK_END
#define SEEK_END	(2)
#endif


#ifndef	RC_INVOKED

#ifndef __VALIST
#ifdef __GNUC__
#define __VALIST __gnuc_va_list
#else
#define __VALIST char*
#endif
#endif /* defined __VALIST  */

/*
 * The structure underlying the FILE type.
 *
 * Some believe that nobody in their right mind should make use of the
 * internals of this structure. Provided by Pedro A. Aranda Gutiirrez
 * <paag@tid.es>.
 */
#ifndef _FILE_DEFINED
#define	_FILE_DEFINED
typedef struct _iobuf
{
	char*	_ptr;
	int	_cnt;
	char*	_base;
	int	_flag;
	int	_file;
	int	_charbuf;
	int	_bufsiz;
	char*	_tmpfname;
} FILE;
#endif	/* Not _FILE_DEFINED */


/*
 * The standard file handles
 */
#ifndef __DECLSPEC_SUPPORTED

extern FILE (*_imp___iob)[];	/* A pointer to an array of FILE */

#define _iob	(*_imp___iob)	/* An array of FILE */

#else /* __DECLSPEC_SUPPORTED */

__MINGW_IMPORT FILE _iob[];	/* An array of FILE imported from DLL. */

#endif /* __DECLSPEC_SUPPORTED */

#define stdin	(&_iob[STDIN_FILENO])
#define stdout	(&_iob[STDOUT_FILENO])
#define stderr	(&_iob[STDERR_FILENO])

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File Operations
 */
_CRTIMP FILE* __cdecl fopen (const char*, const char*);
_CRTIMP FILE* __cdecl	freopen (const char*, const char*, FILE*);
_CRTIMP int __cdecl	fflush (FILE*);
_CRTIMP int __cdecl	fclose (FILE*);
/* MS puts remove & rename (but not wide versions) in io.h  also */
_CRTIMP int __cdecl	remove (const char*);
_CRTIMP int __cdecl	rename (const char*, const char*);
_CRTIMP FILE* __cdecl	tmpfile (void);
_CRTIMP char* __cdecl	tmpnam (char*);

#ifndef __STRICT_ANSI__
_CRTIMP char* __cdecl	_tempnam (const char*, const char*);
_CRTIMP int  __cdecl    _rmtmp(void);

#ifndef	NO_OLDNAMES
_CRTIMP char* __cdecl	tempnam (const char*, const char*);
_CRTIMP int __cdecl     rmtmp(void);
#endif
#endif /* __STRICT_ANSI__ */

_CRTIMP int __cdecl	setvbuf (FILE*, char*, int, size_t);

_CRTIMP void __cdecl	setbuf (FILE*, char*);

/*
 * Formatted Output
 */

_CRTIMP int __cdecl	fprintf (FILE*, const char*, ...);
_CRTIMP int __cdecl	printf (const char*, ...);
_CRTIMP int __cdecl	sprintf (char*, const char*, ...);
_CRTIMP int __cdecl	_snprintf (char*, size_t, const char*, ...);
_CRTIMP int __cdecl	vfprintf (FILE*, const char*, __VALIST);
_CRTIMP int __cdecl	vprintf (const char*, __VALIST);
_CRTIMP int __cdecl	vsprintf (char*, const char*, __VALIST);
_CRTIMP int __cdecl	_vsnprintf (char*, size_t, const char*, __VALIST);

#ifndef __NO_ISOCEXT  /* externs in libmingwex.a */
int __cdecl snprintf(char* s, size_t n, const char*  format, ...);
__CRT_INLINE int __cdecl
vsnprintf (char* s, size_t n, const char* format, __VALIST arg)
  { return _vsnprintf ( s, n, format, arg); }
int __cdecl vscanf (const char * __restrict__, __VALIST);
int __cdecl vfscanf (FILE * __restrict__, const char * __restrict__,
		     __VALIST);
int __cdecl vsscanf (const char * __restrict__,
		     const char * __restrict__, __VALIST);
#endif

/*
 * Formatted Input
 */

_CRTIMP int __cdecl	fscanf (FILE*, const char*, ...);
_CRTIMP int __cdecl	scanf (const char*, ...);
_CRTIMP int __cdecl	sscanf (const char*, const char*, ...);
/*
 * Character Input and Output Functions
 */

_CRTIMP int __cdecl	fgetc (FILE*);
_CRTIMP char* __cdecl	fgets (char*, int, FILE*);
_CRTIMP int __cdecl	fputc (int, FILE*);
_CRTIMP int __cdecl	fputs (const char*, FILE*);
_CRTIMP char* __cdecl	gets (char*);
_CRTIMP int __cdecl	puts (const char*);
_CRTIMP int __cdecl	ungetc (int, FILE*);

/* Traditionally, getc and putc are defined as macros. but the
   standard doesn't say that they must be macros.
   We use inline functions here to allow the fast versions
   to be used in C++ with namespace qualification, eg., ::getc.

   _filbuf and _flsbuf  are not thread-safe. */
_CRTIMP int __cdecl	_filbuf (FILE*);
_CRTIMP int __cdecl	_flsbuf (int, FILE*);

#if !defined _MT && !defined _XBOX

__CRT_INLINE int __cdecl getc (FILE* __F)
{
  return (--__F->_cnt >= 0)
    ?  (int) (unsigned char) *__F->_ptr++
    : _filbuf (__F);
}

__CRT_INLINE int __cdecl putc (int __c, FILE* __F)
{
  return (--__F->_cnt >= 0)
    ?  (int) (unsigned char) (*__F->_ptr++ = (char)__c)
    :  _flsbuf (__c, __F);
}

__CRT_INLINE int __cdecl getchar (void)
{
  return (--stdin->_cnt >= 0)
    ?  (int) (unsigned char) *stdin->_ptr++
    : _filbuf (stdin);
}

__CRT_INLINE int __cdecl putchar(int __c)
{
  return (--stdout->_cnt >= 0)
    ?  (int) (unsigned char) (*stdout->_ptr++ = (char)__c)
    :  _flsbuf (__c, stdout);}

#else  /* Use library functions.  */

_CRTIMP int __cdecl	getc (FILE*);
_CRTIMP int __cdecl	putc (int, FILE*);
_CRTIMP int __cdecl	getchar (void);
_CRTIMP int __cdecl	putchar (int);

#endif

/*
 * Direct Input and Output Functions
 */

_CRTIMP size_t __cdecl	fread (void*, size_t, size_t, FILE*);
_CRTIMP size_t __cdecl	fwrite (const void*, size_t, size_t, FILE*);

/*
 * File Positioning Functions
 */

_CRTIMP int __cdecl	fseek (FILE*, long, int);
_CRTIMP long __cdecl	ftell (FILE*);
_CRTIMP void __cdecl	rewind (FILE*);

#ifdef __USE_MINGW_FSEEK  /* These are in libmingwex.a */
/*
 * Workaround for limitations on win9x where a file contents are
 * not zero'd out if you seek past the end and then write.
 */

int __cdecl __mingw_fseek (FILE *, long, int);
int __cdecl __mingw_fwrite (const void*, size_t, size_t, FILE*);
#define fseek(fp, offset, whence)  __mingw_fseek(fp, offset, whence)
#define fwrite(buffer, size, count, fp)  __mingw_fwrite(buffer, size, count, fp)
#endif /* __USE_MINGW_FSEEK */

/*
 * An opaque data type used for storing file positions... The contents of
 * this type are unknown, but we (the compiler) need to know the size
 * because the programmer using fgetpos and fsetpos will be setting aside
 * storage for fpos_t structres. Actually I tested using a byte array and
 * it is fairly evident that the fpos_t type is a long (in CRTDLL.DLL).
 * Perhaps an unsigned long? TODO? It's definitely a 64-bit number in
 * MSVCRT however, and for now `long long' will do.
 */
#ifdef __MSVCRT__
typedef long long fpos_t;
#else
typedef long fpos_t;
#endif

_CRTIMP int __cdecl	fgetpos	(FILE*, fpos_t*);
_CRTIMP int __cdecl	fsetpos (FILE*, const fpos_t*);

/*
 * Error Functions
 */

_CRTIMP int __cdecl	feof (FILE*);
_CRTIMP int __cdecl	ferror (FILE*);

#ifndef _XBOX //Cant use inlines or macros here, they have to be overridden in our code.
#ifdef __cplusplus
inline int __cdecl feof (FILE* __F)
  { return __F->_flag & _IOEOF; }
inline int __cdecl ferror (FILE* __F)
  { return __F->_flag & _IOERR; }
#else
#define feof(__F)     ((__F)->_flag & _IOEOF)
#define ferror(__F)   ((__F)->_flag & _IOERR)
#endif
#endif

_CRTIMP void __cdecl	clearerr (FILE*);
_CRTIMP void __cdecl	perror (const char*);


#ifndef __STRICT_ANSI__
/*
 * Pipes
 */
_CRTIMP FILE* __cdecl	_popen (const char*, const char*);
_CRTIMP int __cdecl	_pclose (FILE*);

#ifndef NO_OLDNAMES
_CRTIMP FILE* __cdecl	popen (const char*, const char*);
_CRTIMP int __cdecl	pclose (FILE*);
#endif

/*
 * Other Non ANSI functions
 */
_CRTIMP int __cdecl	_flushall (void);
_CRTIMP int __cdecl	_fgetchar (void);
_CRTIMP int __cdecl	_fputchar (int);
_CRTIMP FILE* __cdecl	_fdopen (int, const char*);
_CRTIMP int __cdecl	_fileno (FILE*);
_CRTIMP int __cdecl	_fcloseall(void);
_CRTIMP FILE* __cdecl	_fsopen(const char*, const char*, int);
#ifdef __MSVCRT__
_CRTIMP int __cdecl	_getmaxstdio(void);
_CRTIMP int __cdecl	_setmaxstdio(int);
#endif

#ifndef _NO_OLDNAMES
_CRTIMP int __cdecl	fgetchar (void);
_CRTIMP int __cdecl	fputchar (int);
_CRTIMP FILE* __cdecl	fdopen (int, const char*);
_CRTIMP int __cdecl	fileno (FILE*);
#endif	/* Not _NO_OLDNAMES */

#define _fileno(__F) ((__F)->_file)
#ifndef _NO_OLDNAMES
#define fileno(__F) ((__F)->_file)
#endif

#if defined (__MSVCRT__) && !defined (__NO_MINGW_LFS)
#include <sys/types.h>
__CRT_INLINE FILE* __cdecl fopen64 (const char* filename, const char* mode)
{
  return fopen (filename, mode); 
}

int __cdecl fseeko64 (FILE*, off64_t, int);

#ifdef __USE_MINGW_FSEEK
int __cdecl __mingw_fseeko64 (FILE *, off64_t, int);
#define fseeko64(fp, offset, whence)  __mingw_fseeko64(fp, offset, whence)
#endif

__CRT_INLINE off64_t __cdecl ftello64 (FILE * stream)
{
  fpos_t pos;
  if (fgetpos(stream, &pos))
    return  -1LL;
  else
   return ((off64_t) pos);
}
#endif /* __NO_MINGW_LFS */

#endif	/* Not __STRICT_ANSI__ */

/* Wide  versions */

#ifndef _WSTDIO_DEFINED
/*  also in wchar.h - keep in sync */
_CRTIMP int __cdecl	fwprintf (FILE*, const wchar_t*, ...);
_CRTIMP int __cdecl	wprintf (const wchar_t*, ...);
_CRTIMP int __cdecl	swprintf (wchar_t*, const wchar_t*, ...);
_CRTIMP int __cdecl	_snwprintf (wchar_t*, size_t, const wchar_t*, ...);
_CRTIMP int __cdecl	vfwprintf (FILE*, const wchar_t*, __VALIST);
_CRTIMP int __cdecl	vwprintf (const wchar_t*, __VALIST);
_CRTIMP int __cdecl	vswprintf (wchar_t*, const wchar_t*, __VALIST);
_CRTIMP int __cdecl	_vsnwprintf (wchar_t*, size_t, const wchar_t*, __VALIST);
_CRTIMP int __cdecl	fwscanf (FILE*, const wchar_t*, ...);
_CRTIMP int __cdecl	wscanf (const wchar_t*, ...);
_CRTIMP int __cdecl	swscanf (const wchar_t*, const wchar_t*, ...);
_CRTIMP wint_t __cdecl	fgetwc (FILE*);
_CRTIMP wint_t __cdecl	fputwc (wchar_t, FILE*);
_CRTIMP wint_t __cdecl	ungetwc (wchar_t, FILE*);

#ifdef __MSVCRT__ 
_CRTIMP wchar_t* __cdecl fgetws (wchar_t*, int, FILE*);
_CRTIMP int __cdecl	fputws (const wchar_t*, FILE*);
_CRTIMP wint_t __cdecl	getwc (FILE*);
_CRTIMP wint_t __cdecl	getwchar (void);
_CRTIMP wchar_t* __cdecl _getws (wchar_t*);
_CRTIMP wint_t __cdecl	putwc (wint_t, FILE*);
_CRTIMP int __cdecl	_putws (const wchar_t*);
_CRTIMP wint_t __cdecl	putwchar (wint_t);
_CRTIMP FILE* __cdecl	_wfdopen(int, wchar_t *);
_CRTIMP FILE* __cdecl	_wfopen (const wchar_t*, const wchar_t*);
_CRTIMP FILE* __cdecl	_wfreopen (const wchar_t*, const wchar_t*, FILE*);
_CRTIMP FILE* __cdecl	_wfsopen (const wchar_t*, const wchar_t*, int);
_CRTIMP wchar_t* __cdecl _wtmpnam (wchar_t*);
_CRTIMP wchar_t* __cdecl _wtempnam (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl	_wrename (const wchar_t*, const wchar_t*);
_CRTIMP int __cdecl	_wremove (const wchar_t*);
_CRTIMP void __cdecl	_wperror (const wchar_t*);
_CRTIMP FILE* __cdecl	_wpopen (const wchar_t*, const wchar_t*);
#endif	/* __MSVCRT__ */

#ifndef __NO_ISOCEXT  /* externs in libmingwex.a */
int __cdecl snwprintf (wchar_t* s, size_t n, const wchar_t*  format, ...);
__CRT_INLINE int __cdecl
vsnwprintf (wchar_t* s, size_t n, const wchar_t* format, __VALIST arg)
  { return _vsnwprintf ( s, n, format, arg);}
int __cdecl vwscanf (const wchar_t * __restrict__, __VALIST);
int __cdecl vfwscanf (FILE * __restrict__,
		       const wchar_t * __restrict__, __VALIST);
int __cdecl vswscanf (const wchar_t * __restrict__,
		       const wchar_t * __restrict__, __VALIST);
#endif

#define _WSTDIO_DEFINED
#endif /* _WSTDIO_DEFINED */

#ifndef __STRICT_ANSI__
#ifdef __MSVCRT__
#ifndef NO_OLDNAMES
_CRTIMP FILE* __cdecl	wpopen (const wchar_t*, const wchar_t*);
#endif /* not NO_OLDNAMES */
#endif /* MSVCRT runtime */

/*
 * Other Non ANSI wide functions
 */
_CRTIMP wint_t __cdecl	_fgetwchar (void);
_CRTIMP wint_t __cdecl	_fputwchar (wint_t);
_CRTIMP int __cdecl	_getw (FILE*);
_CRTIMP int __cdecl	_putw (int, FILE*);

#ifndef _NO_OLDNAMES
_CRTIMP wint_t __cdecl	fgetwchar (void);
_CRTIMP wint_t __cdecl	fputwchar (wint_t);
_CRTIMP int __cdecl	getw (FILE*);
_CRTIMP int __cdecl	putw (int, FILE*);
#endif	/* Not _NO_OLDNAMES */

#endif /* __STRICT_ANSI */

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif /* _STDIO_H_ */
