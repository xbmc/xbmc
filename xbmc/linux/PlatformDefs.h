#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_POSIX

#define LINE_ENDING "\n"

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#if defined(TARGET_DARWIN)
#include <stdio.h>
#include <sched.h>
#include <AvailabilityMacros.h>
#ifndef __STDC_FORMAT_MACROS
  #define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#if defined(TARGET_DARWIN_OSX)
#include <libkern/OSTypes.h>
#endif
#elif defined(TARGET_FREEBSD)
#include <stdio.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <sys/sysinfo.h>
#endif
#include <sys/time.h>
#include <time.h>
#endif

#if defined(__ppc__) || defined(__powerpc__)
#define PIXEL_ASHIFT 0
#define PIXEL_RSHIFT 8
#define PIXEL_GSHIFT 16
#define PIXEL_BSHIFT 24
#define AMASK 0x000000ff
#define RMASK 0x0000ff00
#define GMASK 0x00ff0000
#define BMASK 0xff000000
#else
#define PIXEL_ASHIFT 24
#define PIXEL_RSHIFT 16
#define PIXEL_GSHIFT 8
#define PIXEL_BSHIFT 0
#define AMASK 0xff000000
#define RMASK 0x00ff0000
#define GMASK 0x0000ff00
#define BMASK 0x000000ff
#endif

#include <stdint.h>

#ifndef PRId64
#ifdef TARGET_WINDOWS
#define PRId64 "I64d"
#else
#if __WORDSIZE == 64
#define PRId64 "ld"
#else
#define PRId64 "lld"
#endif
#endif
#endif

#ifndef PRIu64
#ifdef TARGET_WINDOWS
#define PRIu64 "I64u"
#else
#if __WORDSIZE == 64
#define PRIu64 "lu"
#else
#define PRIu64 "llu"
#endif
#endif
#endif
	
#ifndef PRIx64
#ifdef TARGET_WINDOWS
#define PRIx64 "I64x"
#else
#if __WORDSIZE == 64
#define PRIx64 "lx"
#else
#define PRIx64 "llx"
#endif
#endif
#endif

#ifndef PRIdS
#define PRIdS "zd"
#endif

#ifndef PRIuS
#define PRIuS "zu"
#endif

#ifdef TARGET_POSIX

#ifndef INSTALL_PATH
#define INSTALL_PATH    "/usr/share/xbmc"
#endif

#ifndef BIN_INSTALL_PATH
#define BIN_INSTALL_PATH "/usr/lib/xbmc"
#endif

#define CONST   const
#define FALSE   0
#define TRUE    1

#define _fdopen fdopen
#define _vsnprintf vsnprintf
#define _stricmp  strcasecmp
#define stricmp   strcasecmp
#define strcmpi strcasecmp
#define strnicmp  strncasecmp
#define _atoi64(x) atoll(x)
#define CopyMemory(dst,src,size) memmove(dst, src, size)
#define ZeroMemory(dst,size) memset(dst, 0, size)

#define VOID    void
#define __int8    char
#define __int16   short
#define __int32   int
#define __int64   long long
#define __uint64  unsigned long long

#define __stdcall
#define __cdecl
#define WINBASEAPI
#define NTAPI       __stdcall
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define WINAPIV     __cdecl
#if !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD)
#define APIENTRY    WINAPI
#else
#define APIENTRY
#endif
#define APIPRIVATE  __stdcall
#define IN
#define OUT
#define OPTIONAL
#define _declspec(X)
#define __declspec(X)

struct CXHandle; // forward declaration
typedef CXHandle* HANDLE;

typedef void* HINSTANCE;
typedef void* HMODULE;

typedef unsigned int  DWORD;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef char        CHAR;
typedef wchar_t     WCHAR;
typedef int         BOOL;
typedef int         INT;
typedef unsigned int  UINT;
// typedef int INT32;              // unused; must match Xmd.h but why bother
typedef long long     INT64;
typedef unsigned long long    UINT64;
typedef long        LONG;
typedef long long     LONGLONG;
#if defined(TARGET_DARWIN_OSX)
typedef UInt32          ULONG;
#else
typedef unsigned long   ULONG;
#endif
typedef size_t        SIZE_T;
typedef void*         PVOID;
typedef void*         LPVOID;
//typedef PVOID         HANDLE;
#define INVALID_HANDLE_VALUE     ((HANDLE)~0U)
typedef HANDLE        HDC;
typedef void*       HWND;
typedef BYTE*       LPBYTE;
typedef DWORD*        LPDWORD;
typedef CONST CHAR*   LPCSTR;
typedef CONST WCHAR*    LPCWSTR;
typedef CHAR*     LPTSTR;
typedef WCHAR         *PWSTR,      *LPWSTR,    *NWPSTR;
typedef CHAR            *PSTR,       *LPSTR,     *NPSTR;
typedef LONG        *PLONG, *LPLONG;
#ifdef UNICODE
typedef LPCWSTR       LPCTSTR;
#else
typedef LPCSTR      LPCTSTR;
#endif
typedef unsigned __int64 ULONGLONG;
typedef unsigned long   ULONG_PTR;
typedef ULONG_PTR     DWORD_PTR;
typedef intptr_t (*FARPROC)(void);

#define MAXWORD   0xffff

typedef DWORD LCID;
typedef WORD* LPWORD;
typedef BOOL* LPBOOL;
typedef CHAR* LPCHAR;
typedef CHAR* PCHAR;
typedef const void* LPCVOID;

typedef union _LARGE_INTEGER
{
  struct {
    DWORD LowPart;
    int32_t HighPart;
  } u;
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

 typedef union _ULARGE_INTEGER {
  struct {
      DWORD LowPart;
      DWORD HighPart;
  } u;
  ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

#define MAKELONG(low,high)     ((LONG)(((WORD)((DWORD_PTR)(low) & 0xFFFF)) | ((DWORD)((WORD)((DWORD_PTR)(high) & 0xFFFF))) << 16))
LONGLONG Int32x32To64(LONG Multiplier, LONG Multiplicand);

void OutputDebugString(LPCTSTR lpOutputString);

// Date / Time

typedef struct _SYSTEMTIME
{
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _TIME_ZONE_INFORMATION {
  LONG Bias;
  WCHAR StandardName[32];
  SYSTEMTIME StandardDate;
  LONG StandardBias;
  WCHAR DaylightName[32];
  SYSTEMTIME DaylightDate;
  LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#define TIME_ZONE_ID_INVALID    ((DWORD)0xFFFFFFFF)
#define TIME_ZONE_ID_UNKNOWN    0
#define TIME_ZONE_ID_STANDARD   1
#define TIME_ZONE_ID_DAYLIGHT   2

// Thread
#define THREAD_BASE_PRIORITY_LOWRT  15
#define THREAD_BASE_PRIORITY_MAX    2
#define THREAD_BASE_PRIORITY_MIN   -2
#define THREAD_BASE_PRIORITY_IDLE  -15
#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (0x7fffffff)
#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

// Network
#define SOCKADDR_IN struct sockaddr_in
#define IN_ADDR struct in_addr
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (~0)
#define closesocket(s)  close(s)
#define ioctlsocket(s, f, v) ioctl(s, f, v)
#define WSAGetLastError() (errno)
#define WSASetLastError(e) (errno = e)
#define WSAECONNRESET ECONNRESET
#define WSAHOST_NOT_FOUND ENOENT
#define WSAETIMEDOUT  ETIMEDOUT
#define WSAEADDRINUSE EADDRINUSE
#define WSAECANCELLED EINTR
#define WSAECONNREFUSED ECONNREFUSED
#define WSAECONNABORTED ECONNABORTED
#define WSAETIMEDOUT ETIMEDOUT

typedef int SOCKET;

// Thread
typedef int (*LPTHREAD_START_ROUTINE)(void *);

// File
#define O_BINARY 0
#define O_TEXT   0
#define _O_TRUNC O_TRUNC
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
  #define stat64 stat
  #define __stat64 stat
  #define fstat64 fstat
  typedef int64_t off64_t;
  #if defined(TARGET_FREEBSD)
    #define statfs64 statfs
  #endif
#else
  #define __stat64 stat64
#endif

struct _stati64 {
  dev_t st_dev;
  ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  dev_t st_rdev;
  __int64  st_size;
  time_t _st_atime;
  time_t _st_mtime;
  time_t _st_ctime;
};

typedef struct _FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _WIN32_FIND_DATA
{
    DWORD     dwFileAttributes;
    FILETIME  ftCreationTime;
    FILETIME  ftLastAccessTime;
    FILETIME  ftLastWriteTime;
    DWORD     nFileSizeHigh;
    DWORD     nFileSizeLow;
    DWORD     dwReserved0;
    DWORD     dwReserved1;
    CHAR      cFileName[260];
    CHAR      cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

#define LPWIN32_FIND_DATAA LPWIN32_FIND_DATA

#define FILE_ATTRIBUTE_DIRECTORY           0x00000010

typedef struct _SECURITY_ATTRIBUTES {
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#define FILE_BEGIN              0
#define FILE_CURRENT            1
#define FILE_END                2

#define _S_IFREG  S_IFREG
#define _S_IFDIR  S_IFDIR
#define MAX_PATH PATH_MAX

#define _stat stat

// Memory
typedef struct _MEMORYSTATUSEX
{
  DWORD dwLength;
  DWORD dwMemoryLoad;

  uint64_t ullTotalPhys;
  uint64_t ullAvailPhys;
  uint64_t ullTotalPageFile;
  uint64_t ullAvailPageFile;
  uint64_t ullTotalVirtual;
  uint64_t ullAvailVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

// Basic D3D stuff
typedef struct _RECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT;

typedef enum _D3DFORMAT
{
  D3DFMT_A8R8G8B8     = 0x00000006,
  D3DFMT_DXT1         = 0x0000000C,
  D3DFMT_DXT2         = 0x0000000E,
  D3DFMT_DXT4         = 0x0000000F,
  D3DFMT_UNKNOWN      = 0xFFFFFFFF
} D3DFORMAT;

// Misc stuff found in the code, not really important
#define PAGE_READWRITE    0x04
#define MAXULONG_PTR    ((ULONG) 0xffffffff)

// CreateFile defines
#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#define FILE_ATTRIBUTE_DIRECTORY  0x00000010

#define FILE_READ_DATA   ( 0x0001 )
#define FILE_WRITE_DATA  ( 0x0002 )
#define FILE_APPEND_DATA ( 0x0004 )

#define GENERIC_READ  FILE_READ_DATA
#define GENERIC_WRITE FILE_WRITE_DATA
#define FILE_SHARE_READ                  0x00000001
#define FILE_SHARE_WRITE                 0x00000002
#define FILE_SHARE_DELETE                0x00000004

// Audio stuff
typedef struct tWAVEFORMATEX
{
WORD    wFormatTag;
WORD    nChannels;
DWORD   nSamplesPerSec;
DWORD   nAvgBytesPerSec;
WORD    nBlockAlign;
WORD    wBitsPerSample;
WORD    cbSize;
} __attribute__((__packed__)) WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;

#define WAVE_FORMAT_UNKNOWN           0x0000
#define WAVE_FORMAT_PCM               0x0001
#define WAVE_FORMAT_ADPCM             0x0002
#define WAVE_FORMAT_IEEE_FLOAT        0x0003
#define WAVE_FORMAT_EXTENSIBLE        0xFFFE

#endif

