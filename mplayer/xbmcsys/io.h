/* 
 * io.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * System level I/O functions and types.
 *
 */
#ifndef	_IO_H_
#define	_IO_H_

/* All the headers include this file. */
#include <_mingw.h>

/* MSVC's io.h contains the stuff from dir.h, so I will too.
 * NOTE: This also defines off_t, the file offset type, through
 *       an inclusion of sys/types.h */

#include <sys/types.h>	/* To get time_t. */
#include <stdint.h>  /* For intptr_t.  */

/*
 * Attributes of files as returned by _findfirst et al.
 */
#define	_A_NORMAL	0x00000000
#define	_A_RDONLY	0x00000001
#define	_A_HIDDEN	0x00000002
#define	_A_SYSTEM	0x00000004
#define	_A_VOLID	0x00000008
#define	_A_SUBDIR	0x00000010
#define	_A_ARCH		0x00000020


#ifndef RC_INVOKED

#ifndef	_FSIZE_T_DEFINED
typedef	unsigned long	_fsize_t;
#define _FSIZE_T_DEFINED
#endif

/*
 * The maximum length of a file name. You should use GetVolumeInformation
 * instead of this constant. But hey, this works.
 * Also defined in stdio.h. 
 */
#ifndef FILENAME_MAX
#define	FILENAME_MAX	(260)
#endif

/*
 * The following structure is filled in by _findfirst or _findnext when
 * they succeed in finding a match.
 */
struct _finddata_t
{
	unsigned	attrib;		/* Attributes, see constants above. */
	time_t		time_create;
	time_t		time_access;	/* always midnight local time */
	time_t		time_write;
	_fsize_t	size;
	char		name[FILENAME_MAX];	/* may include spaces. */
};

struct _finddatai64_t {
    unsigned    attrib;
    time_t      time_create;
    time_t      time_access;
    time_t      time_write;
    __int64     size;
    char        name[FILENAME_MAX];
};

struct __finddata64_t {
        unsigned    attrib;
        __time64_t  time_create;    
        __time64_t  time_access;    
        __time64_t  time_write;
        _fsize_t    size;
         char       name[FILENAME_MAX];
};

#ifndef _WFINDDATA_T_DEFINED
struct _wfinddata_t {
    	unsigned	attrib;
    	time_t		time_create;	/* -1 for FAT file systems */
    	time_t		time_access;	/* -1 for FAT file systems */
    	time_t		time_write;
    	_fsize_t	size;
    	wchar_t		name[FILENAME_MAX];	/* may include spaces. */
};

struct _wfinddatai64_t {
    unsigned    attrib;
    time_t      time_create;
    time_t      time_access;
    time_t      time_write;
    __int64     size;
    wchar_t     name[FILENAME_MAX];
};

struct __wfinddata64_t {
        unsigned    attrib;
        __time64_t  time_create;    
        __time64_t  time_access;
        __time64_t  time_write;
        _fsize_t    size;
        wchar_t     name[FILENAME_MAX];
};

#define _WFINDDATA_T_DEFINED
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Functions for searching for files. _findfirst returns -1 if no match
 * is found. Otherwise it returns a handle to be used in _findnext and
 * _findclose calls. _findnext also returns -1 if no match could be found,
 * and 0 if a match was found. Call _findclose when you are finished.
 */
/*  FIXME: Should these all use intptr_t, as per recent MSDN docs?  */
_CRTIMP long __cdecl _findfirst (const char*, struct _finddata_t*);
_CRTIMP int __cdecl _findnext (long, struct _finddata_t*);
_CRTIMP int __cdecl _findclose (long);

_CRTIMP int __cdecl _chdir (const char*);
_CRTIMP char* __cdecl _getcwd (char*, int);
_CRTIMP int __cdecl _mkdir (const char*);
_CRTIMP char* __cdecl _mktemp (char*);
_CRTIMP int __cdecl _rmdir (const char*);
_CRTIMP int __cdecl _chmod (const char*, int);

#ifdef __MSVCRT__
_CRTIMP __int64 __cdecl _filelengthi64(int);
_CRTIMP long __cdecl _findfirsti64(const char*, struct _finddatai64_t*);
_CRTIMP int __cdecl _findnexti64(long, struct _finddatai64_t*);
_CRTIMP __int64 __cdecl _lseeki64(int, __int64, int);
_CRTIMP __int64 __cdecl _telli64(int);
/* These require newer versions of msvcrt.dll (6.1 or higher). */ 
#if __MSVCRT_VERSION__ >= 0x0601
_CRTIMP intptr_t __cdecl _findfirst64(const char*, struct __finddata64_t*);
_CRTIMP intptr_t __cdecl _findnext64(intptr_t, struct __finddata64_t*); 
#endif /* __MSVCRT_VERSION__ >= 0x0601 */

#ifndef __NO_MINGW_LFS
__CRT_INLINE off64_t lseek64 (int fd, off64_t offset, int whence) 
{
  return _lseeki64(fd, (__int64) offset, whence);
}
#endif

#endif /* __MSVCRT__ */

#ifndef _NO_OLDNAMES

#ifndef _UWIN
_CRTIMP int __cdecl chdir (const char*);
_CRTIMP char* __cdecl getcwd (char*, int);
_CRTIMP int __cdecl mkdir (const char*);
_CRTIMP char* __cdecl mktemp (char*);
_CRTIMP int __cdecl rmdir (const char*);
_CRTIMP int __cdecl chmod (const char*, int);
#endif /* _UWIN */

#endif /* Not _NO_OLDNAMES */

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

/* TODO: Maximum number of open handles has not been tested, I just set
 * it the same as FOPEN_MAX. */
#define	HANDLE_MAX	FOPEN_MAX

/* Some defines for _access nAccessMode (MS doesn't define them, but
 * it doesn't seem to hurt to add them). */
#define	F_OK	0	/* Check for file existence */
#define	X_OK	1	/* Check for execute permission. */
#define	W_OK	2	/* Check for write permission */
#define	R_OK	4	/* Check for read permission */

#ifndef RC_INVOKED

#ifdef	__cplusplus
extern "C" {
#endif

_CRTIMP int __cdecl _access (const char*, int);
_CRTIMP int __cdecl _chsize (int, long);
_CRTIMP int __cdecl _close (int);
_CRTIMP int __cdecl _commit(int);

/* NOTE: The only significant bit in unPermissions appears to be bit 7 (0x80),
 *       the "owner write permission" bit (on FAT). */
_CRTIMP int __cdecl _creat (const char*, int);

_CRTIMP int __cdecl _dup (int);
_CRTIMP int __cdecl _dup2 (int, int);
_CRTIMP long __cdecl _filelength (int);
_CRTIMP long __cdecl _get_osfhandle (int);
_CRTIMP int __cdecl _isatty (int);

/* In a very odd turn of events this function is excluded from those
 * files which define _STREAM_COMPAT. This is required in order to
 * build GNU libio because of a conflict with _eof in streambuf.h
 * line 107. Actually I might just be able to change the name of
 * the enum member in streambuf.h... we'll see. TODO */
#ifndef	_STREAM_COMPAT
_CRTIMP int __cdecl _eof (int);
#endif

/* LK_... locking commands defined in sys/locking.h. */
_CRTIMP int __cdecl _locking (int, int, long);
_CRTIMP long __cdecl _lseek (int, long, int);

/* Optional third argument is unsigned unPermissions. */
_CRTIMP int __cdecl _open (const char*, int, ...);

_CRTIMP int __cdecl _open_osfhandle (long, int);
_CRTIMP int __cdecl _pipe (int *, unsigned int, int);
_CRTIMP int __cdecl _read (int, void*, unsigned int);
_CRTIMP int __cdecl _setmode (int, int);

/* SH_... flags for nShFlags defined in share.h
 * Optional fourth argument is unsigned unPermissions */
_CRTIMP int __cdecl _sopen (const char*, int, int, ...);
_CRTIMP long __cdecl _tell (int);

/* Should umask be in sys/stat.h and/or sys/types.h instead? */
_CRTIMP int __cdecl _umask (int);
_CRTIMP int __cdecl _unlink (const char*);
_CRTIMP int __cdecl _write (int, const void*, unsigned int);

/* Wide character versions. Also declared in wchar.h. */
/* Not in crtdll.dll */
#if !defined (_WIO_DEFINED)
#if defined (__MSVCRT__)
_CRTIMP int __cdecl _waccess(const wchar_t*, int);
_CRTIMP int __cdecl _wchmod(const wchar_t*, int);
_CRTIMP int __cdecl _wcreat(const wchar_t*, int);
_CRTIMP long __cdecl _wfindfirst(const wchar_t*, struct _wfinddata_t*);
_CRTIMP int __cdecl _wfindnext(long, struct _wfinddata_t *);
_CRTIMP int __cdecl _wunlink(const wchar_t*);
_CRTIMP int __cdecl _wopen(const wchar_t*, int, ...);
_CRTIMP int __cdecl _wsopen(const wchar_t*, int, int, ...);
_CRTIMP wchar_t * __cdecl _wmktemp(wchar_t*);
_CRTIMP long __cdecl _wfindfirsti64(const wchar_t*, struct _wfinddatai64_t*);
_CRTIMP int __cdecl _wfindnexti64(long, struct _wfinddatai64_t*);
#if __MSVCRT_VERSION__ >= 0x0601
_CRTIMP intptr_t __cdecl _wfindfirst64(const wchar_t*, struct __wfinddata64_t*); 
_CRTIMP intptr_t __cdecl _wfindnext64(intptr_t, struct __wfinddata64_t*);
#endif
#endif /* defined (__MSVCRT__) */
#define _WIO_DEFINED
#endif /* _WIO_DEFINED */

#ifndef	_NO_OLDNAMES
/*
 * Non-underscored versions of non-ANSI functions to improve portability.
 * These functions live in libmoldname.a.
 */

#ifndef _UWIN
_CRTIMP int __cdecl access (const char*, int);
_CRTIMP int __cdecl chsize (int, long );
_CRTIMP int __cdecl close (int);
_CRTIMP int __cdecl creat (const char*, int);
_CRTIMP int __cdecl dup (int);
_CRTIMP int __cdecl dup2 (int, int);
_CRTIMP int __cdecl eof (int);
_CRTIMP long __cdecl filelength (int);
_CRTIMP int __cdecl isatty (int);
_CRTIMP long __cdecl lseek (int, long, int);
_CRTIMP int __cdecl open (const char*, int, ...);
_CRTIMP int __cdecl read (int, void*, unsigned int);
_CRTIMP int __cdecl setmode (int, int);
_CRTIMP int __cdecl sopen (const char*, int, int, ...);
_CRTIMP long __cdecl tell (int);
_CRTIMP int __cdecl umask (int);
_CRTIMP int __cdecl unlink (const char*);
_CRTIMP int __cdecl write (int, const void*, unsigned int);
#endif /* _UWIN */

/* Wide character versions. Also declared in wchar.h. */
/* Where do these live? Not in libmoldname.a nor in libmsvcrt.a */
#if 0
int 		waccess(const wchar_t *, int);
int 		wchmod(const wchar_t *, int);
int 		wcreat(const wchar_t *, int);
long 		wfindfirst(wchar_t *, struct _wfinddata_t *);
int 		wfindnext(long, struct _wfinddata_t *);
int 		wunlink(const wchar_t *);
int 		wrename(const wchar_t *, const wchar_t *);
int 		wopen(const wchar_t *, int, ...);
int 		wsopen(const wchar_t *, int, int, ...);
wchar_t * 	wmktemp(wchar_t *);
#endif

#endif	/* Not _NO_OLDNAMES */

/* XBMC-Largefiles */
#ifdef _XBOX
#ifndef _XBOXSEEK_
#define _XBOXSEEK_

#define _lseek(a, b, c) _lseeki64(a, b, c)
#define lseek(a, b, c) _lseeki64(a, b, c)
#define _tell(a) _telli64(a)
#define tell(a) _telli64(a)

#endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* _IO_H_ not defined */
