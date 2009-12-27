#ifndef _CYGWIN_INTTYPES_H
#define _CYGWIN_INTTYPES_H
/* /usr/include/inttypes.h for CYGWIN
 * Copyleft 2001-2002 by Felix Buenemann
 * <atmosfear at users.sourceforge.net>
 *
 * Should be installed into /usr/include
 * as inttypes.h*/
typedef char *  caddr_t;
typedef char int8_t;
typedef unsigned char u_int8_t;
typedef short int16_t;
typedef unsigned short u_int16_t;
typedef int int32_t;
typedef unsigned int u_int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 u_int64_t;
typedef int32_t register_t;
typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;
typedef u_int64_t uint64_t;
//typedef u_int64_t off_t;

//typedef void *HANDLE;
//typedef unsigned char UCHAR;
//typedef unsigned int UINT;
//typedef unsigned char BYTE;
//typedef unsigned long ULONG;
//typedef long LONG;
//typedef unsigned short USHORT;
//typedef unsigned short WCHAR;
//typedef unsigned short WORD;
//typedef unsigned long DWORD;
//typedef unsigned int WPARAM;
//typedef __int64 INT64, *PINT64;
//typedef unsigned __int64 UINT64, *PUINT64;
//typedef void *LPVOID;
//typedef const char *LPCSTR;
//typedef LONG HRESULT;
//typedef unsigned short *PWCHAR;
//typedef unsigned short WCHAR;
//typedef WCHAR *         LPWSTR;
//typedef int             WIN_BOOL;
//#define FALSE 0
//
//#define TRUE 1
//#define APIENTRY    WINAPI


//typedef void *HANDLE;
//typedef unsigned char UCHAR;
//typedef unsigned int UINT;
//typedef unsigned long ULONG;
//typedef unsigned short USHORT;
//typedef unsigned short WCHAR;
//typedef unsigned short WORD;
//typedef unsigned int WPARAM;
//typedef __int64 INT64, *PINT64;
//typedef unsigned __int64 UINT64, *PUINT64;
//typedef void *LPVOID;
//typedef const char *LPCSTR;
//
//typedef unsigned char BOOLEAN;
//typedef char CHAR;
//typedef unsigned int UINT;
//typedef long LONG;
//typedef unsigned long       DWORD;
//typedef int                 BOOL;
//typedef unsigned char       BYTE;
//typedef unsigned short      WORD;
//typedef float               FLOAT;
//typedef FLOAT               *PFLOAT;
//typedef BOOL            *PBOOL;
//typedef BOOL             *LPBOOL;
//typedef BYTE            *PBYTE;
//typedef BYTE             *LPBYTE;
//typedef int             *PINT;
//typedef int              *LPINT;
//typedef WORD            *PWORD;
//typedef WORD             *LPWORD;
//typedef long             *LPLONG;
//typedef DWORD           *PDWORD;
//typedef DWORD            *LPDWORD;
//typedef void             *LPVOID;
//typedef short SHORT;
//typedef unsigned short *PWCHAR;
//typedef unsigned short WCHAR;
//typedef void *PVOID;
//typedef char *LPSTR;
//typedef unsigned short  UINT16;
//typedef LONG            LRESULT;
//typedef UINT16          HANDLE16;
//typedef HANDLE16 HGLOBAL16;
//#define FALSE 0
//#define TRUE 1
//#define APIENTRY    WINAPI
//typedef const WCHAR    *LPCWSTR;
//typedef struct _HANDLE_HDR
//{
//  DWORD      type;
//}HANDLE_HDR;

// This structure describes global memory object
// The HGLOBAL is pointer to this structure
//typedef struct _GLOBAL_HANDLE
//{
//  HANDLE_HDR header;      // This is common for all handles
//  void       *pmem;       // pointer to memory
//  DWORD      dwSize;      // Size of allocated memory
//  UINT       uFlags;      // Flags of memory object
//  UINT       uLockCount;  // lock count of memory object
//}GLOBAL_HANDLE;

// this structure describes mapped image. 
// The HINSTANCE is pointer to this structure
//typedef struct _INSTANCE 
//{  
//  HANDLE_HDR  header;
//  BOOL        bElf;
//  LPSTR       lpLibFileName;
//  LPBYTE      lpImage;
//  LPBYTE      lpResource;
//  DWORD       dwResVirt;
//  DWORD       ImageSize;
//  int         TimesUsed;
//  int         LibsInUse;
//  void        **pinstLibs;
//  BOOL        bEntryCalled;
//  char        **exported_names;
//  int         ordinal_base;
//  int         ordinal_max;
//}INSTANCE;
//
//#define PROT_NONE 0
//#define PROT_READ 1
//#define PROT_WRITE 2
//#define PROT_EXEC 4
//
//
//#define MAP_FILE 0
//#define MAP_SHARED 1
//#define MAP_PRIVATE 2
//#define MAP_TYPE 0xF
//#define MAP_FIXED 0x10
//#define MAP_ANONYMOUS 0x20
//#define MAP_ANON MAP_ANONYMOUS
//
//typedef __int32 INT_PTR, *PINT_PTR;
//typedef unsigned int UINT_PTR, *PUINT_PTR;
//typedef LONG            LPARAM;
//typedef	UINT	MMRESULT;

typedef long off_t;
#define ssize_t int
typedef unsigned int   size_t;

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

#endif /* _CYGWIN_INTTYPES_H */

