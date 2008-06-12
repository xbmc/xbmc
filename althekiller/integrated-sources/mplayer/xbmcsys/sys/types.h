/*
 * types.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * The definition of constants, data types and global variables.
 *
 */

#ifndef	_TYPES_H_
#define	_TYPES_H_

/* All the headers include this file. */
#include <_mingw.h>

#define __need_wchar_t
#define __need_size_t
#define __need_ptrdiff_t
#ifndef RC_INVOKED
#include <stddef.h>
#endif	/* Not RC_INVOKED */

#ifndef RC_INVOKED

#ifndef _TIME_T_DEFINED
typedef	long	time_t;
#define	_TIME_T_DEFINED
#endif

#ifndef _TIME64_T_DEFINED
typedef __int64 __time64_t;
#define _TIME64_T_DEFINED
#endif

#ifndef	_OFF_T_
#define	_OFF_T_
typedef long _off_t;

#ifndef	_NO_OLDNAMES
typedef _off_t	off_t;
#endif
#endif	/* Not _OFF_T_ */


#ifndef _DEV_T_
#define	_DEV_T_
#ifdef __MSVCRT__
typedef unsigned int _dev_t;
#else
typedef short _dev_t;
#endif

#ifndef	_NO_OLDNAMES
typedef _dev_t	dev_t;
#endif
#endif	/* Not _DEV_T_ */


#ifndef _INO_T_
#define	_INO_T_
typedef short _ino_t;

#ifndef	_NO_OLDNAMES
typedef _ino_t	ino_t;
#endif
#endif	/* Not _INO_T_ */


#ifndef _PID_T_
#define	_PID_T_
typedef int	_pid_t;

#ifndef	_NO_OLDNAMES
typedef _pid_t	pid_t;
#endif
#endif	/* Not _PID_T_ */


#ifndef _MODE_T_
#define	_MODE_T_
typedef unsigned short _mode_t;

#ifndef	_NO_OLDNAMES
typedef _mode_t	mode_t;
#endif
#endif	/* Not _MODE_T_ */


#ifndef _SIGSET_T_
#define	_SIGSET_T_
typedef int	_sigset_t;

#ifndef	_NO_OLDNAMES
typedef _sigset_t	sigset_t;
#endif
#endif	/* Not _SIGSET_T_ */

#ifndef _SSIZE_T_
#define _SSIZE_T_
typedef long _ssize_t;

#ifndef	_NO_OLDNAMES
typedef _ssize_t ssize_t;
#endif
#endif /* Not _SSIZE_T_ */ 

#ifndef _FPOS64_T_
#define _FPOS64_T_
typedef long long fpos64_t;
#endif

#ifndef _OFF64_T_
#define _OFF64_T_
typedef long long off64_t;
#endif

#ifdef _XBOX
#ifndef _XBOXTYPES_
#define _XBOXTYPES_

typedef int _xbssize_t;
#define _ssize_t _xbssize_t
#define ssize_t _xbssize_t

#define _off_t off64_t
#define off_t off64_t

#endif
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _TYPES_H_ */
