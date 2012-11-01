#ifndef _RAR_OS_
#define _RAR_OS_

#ifdef WIN32
#include <windows.h>
#endif

#define FALSE 0
#define TRUE  1

#ifdef __EMX__
  #define INCL_BASE
#endif

#if defined(_WIN_32) || defined(_EMX)
#define ENABLE_BAD_ALLOC
#endif


#if defined(_WIN_32) || defined(_EMX)

#define LITTLE_ENDIAN
#define NM  1024

#ifdef _WIN_32

//#define STRICT
//#define WINVER 0x0400
#if _WIN32_WINNT < 0x0300
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0300
#endif

#ifndef XBMC
#define WIN32_LEAN_AND_MEAN
#endif


#include "system.h"
#include <prsht.h>
#if defined(_LINUX)
#include "PlatformDefs.h"
#endif

#endif

#ifndef _WIN_CE
#include <sys/types.h>
#include <sys/stat.h>
#include <dos.h>
#endif

#if !defined(_EMX) && !defined(_MSC_VER) && !defined(_WIN_CE)
  #define ENABLE_MKTEMP
  #include <dir.h>
#endif
#ifdef _MSC_VER
//  #define for if (0) ; else for
#ifndef _WIN_CE
  #include <direct.h>
#endif
#else
  #include <dirent.h>
#endif

#ifndef _WIN_CE
#include <share.h>
#endif

#if defined(ENABLE_BAD_ALLOC) && !defined(_WIN_CE)
  #include <new.h>
#endif

#ifdef _EMX
  #include <unistd.h>
  #include <pwd.h>
  #include <grp.h>
  #include <errno.h>
  #ifdef _DJGPP
    #include <utime.h>
  #else
    #include <os2.h>
    #include <sys/utime.h>
    #include <emx/syscalls.h>
  #endif
#else
  #if defined(_MSC_VER) || defined(__MINGW32__)
      #include <exception>
  #else
    #include <except.h>
  #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN_CE
  #include <fcntl.h>
  #include <dos.h>
  #include <io.h>
  #include <time.h>
  #include <signal.h>
#endif

/*
#ifdef _WIN_32
#pragma hdrstop
#endif
*/

#define ENABLE_ACCESS

#define DefConfigName  "rar.ini"
#define DefLogName     "rar.log"


#define PATHDIVIDER  "\\"
#define PATHDIVIDERW L"\\"
#define CPATHDIVIDER '\\'
#define MASKALL      "*"
#define MASKALLW     L"*"

#define READBINARY   "rb"
#define READTEXT     "rt"
#define UPDATEBINARY "r+b"
#define CREATEBINARY "w+b"
#define APPENDTEXT   "at"

#if defined(_WIN_32)
  #ifdef _MSC_VER
    #define _stdfunction __cdecl
  #else
    #define _stdfunction _USERENTRY
  #endif
#else
  #define _stdfunction
#endif

#endif

#ifdef _UNIX

#define  NM  1024

#ifdef _BEOS
#include <be/kernel/fs_info.h>
#include <be/kernel/fs_attr.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#if defined(__QNXNTO__)
  #include <sys/param.h>
#endif
#if defined(__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || defined(__APPLE__)
  #include <sys/param.h>
  #include <sys/mount.h>
#else
#endif
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <utime.h>
#include <locale.h>

#ifdef  S_IFLNK
#define SAVE_LINKS
#endif

#define ENABLE_ACCESS

#define DefConfigName  ".rarrc"
#define DefLogName     ".rarlog"


#define PATHDIVIDER  "/"
#define PATHDIVIDERW L"/"
#define CPATHDIVIDER '/'
#define MASKALL      "*"
#define MASKALLW     L"*"

#define READBINARY   "r"
#define READTEXT     "r"
#define UPDATEBINARY "r+"
#define CREATEBINARY "w+"
#define APPENDTEXT   "a"

#define _stdfunction 

#if defined(__sparc) || defined(sparc) || defined(__hpux)
  #ifndef BIG_ENDIAN
     #define BIG_ENDIAN
  #endif
#endif

#endif

typedef const char* MSGID;

#define safebuf static

#if defined(LITTLE_ENDIAN) && defined(BIG_ENDIAN)
  #if defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN
    #undef LITTLE_ENDIAN
  #elif defined(BYTE_ORDER) && BYTE_ORDER == LITTLE_ENDIAN
    #undef BIG_ENDIAN
  #else
    #error "Both LITTLE_ENDIAN and BIG_ENDIAN are defined. Undef something one"
  #endif
#endif

#if !defined(BIG_ENDIAN) && !defined(_WIN_CE) && defined(_WIN_32)
#define ALLOW_NOT_ALIGNED_INT
#endif

#endif // _RAR_OS_
