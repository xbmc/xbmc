#ifndef __PLATFORM_DEFS_H__
#define __PLATFORM_DEFS_H__

#ifdef _LINUX

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
#ifdef __APPLE__
#include <sys/sysctl.h>
#else
#include <sys/sysinfo.h>
#endif
#include <sys/time.h>
#include <time.h>
#endif

#ifdef HAS_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_mutex.h>
#endif

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64 "I64d"
#else
#define PRId64 "lld"
#endif
#endif

#ifdef _LINUX

#define XXLog(a,b) printf("%s", (b))

#define INSTALL_PATH    "/usr/share/xbmc"

#define CONST		const
#define FALSE		0
#define TRUE		1

#define _fdopen fdopen
#define _vsnprintf vsnprintf
#define _stricmp 	strcasecmp
#define stricmp 	strcasecmp
#define strcmpi	strcasecmp
#define strnicmp  strncasecmp
#define _atoi64(x) atoll(x)
#define CopyMemory(dst,src,size) memmove(dst, src, size)
#define ZeroMemory(dst,size) bzero(dst, size)

#define VOID		void
#define byte		unsigned char
#define __int8 		char
#define __int16		short
#define __int32		int
#define __int64		long long
#define __uint64	unsigned long long

#ifdef __x86_64__
#define __stdcall
#else /* !__x86_64__ */
#define __stdcall   __attribute__((__stdcall__))
#endif /* __x86_64__ */
#define __cdecl
#define WINBASEAPI
#define NTAPI       __stdcall
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define WINAPIV     __cdecl
#ifndef __APPLE__
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

#define __try	try
#define EXCEPTION_EXECUTE_HANDLER ...
//NOTE: dont try to define __except because it breaks g++ (already uses it).

class CXHandle; // foward declaration
typedef CXHandle* HANDLE;

typedef void* HINSTANCE;
typedef void* HMODULE;

#ifdef __APPLE__
typedef int64_t		off64_t;
typedef off_t     __off_t;
typedef off64_t   __off64_t;
#include <sched.h>
#endif

typedef unsigned int 	DWORD;
typedef unsigned short 	WORD;
typedef unsigned char 	BYTE;
typedef char				CHAR;
typedef unsigned char	UCHAR;
typedef wchar_t			WCHAR;
typedef int 				BOOL;
typedef BYTE 				BOOLEAN;
typedef short				SHORT;
typedef unsigned short  USHORT;
typedef int 				INT;
typedef unsigned int 	UINT;
typedef unsigned int 	INT32;
typedef unsigned int 	UINT32;
typedef long long 		INT64;
typedef unsigned long long		UINT64;
typedef long				LONG;
typedef long long			LONGLONG;
typedef unsigned long 	ULONG;
typedef float 				FLOAT;
typedef size_t				SIZE_T;
typedef void* 				PVOID;
typedef void* 				LPVOID;
//typedef PVOID 				HANDLE;
#define INVALID_HANDLE_VALUE     ((HANDLE)~0U)
typedef HANDLE				HDC;
typedef void*				HWND;
typedef LONG				HRESULT;
typedef BYTE*				LPBYTE;
typedef DWORD*				LPDWORD;
typedef CONST CHAR*		LPCSTR;
typedef CONST WCHAR*		LPCWSTR;
typedef CHAR*			LPTSTR;
typedef WCHAR     		*PWSTR,      *LPWSTR,    *NWPSTR;
typedef CHAR           	*PSTR,       *LPSTR,     *NPSTR;
typedef LONG				*PLONG, *LPLONG;
#ifdef UNICODE
typedef LPCWSTR 			LPCTSTR; 
#else
typedef LPCSTR 			LPCTSTR;
#endif
typedef unsigned __int64 ULONGLONG;
typedef long 				LONG_PTR;
typedef unsigned long 	ULONG_PTR;
typedef ULONG_PTR 		DWORD_PTR;
typedef __int64		 	__time64_t;
typedef intptr_t (*FARPROC)(void);

#define MAXWORD		0xffff
#define MAXDWORD	0xffffffff

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

void OutputDebugString(LPCTSTR lpOuputString);

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
#define closesocket(s) 	close(s)
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

class CCriticalSection;
#define CRITICAL_SECTION     XCriticalSection
#define LPCRITICAL_SECTION   XCriticalSection*

// Thread
typedef int (*LPTHREAD_START_ROUTINE)(void *);

// File
#define O_BINARY 0
#define O_TEXT   0
#define _O_TRUNC O_TRUNC
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _off_t off_t

struct __stat64 {
  dev_t 			 st_dev;
  ino_t 			  st_ino;
  mode_t 		  st_mode;
  nlink_t        st_nlink;
  uid_t          st_uid;
  gid_t          st_gid;
  dev_t	 		  st_rdev;
  __int64 st_size;
  __time64_t _st_atime;
  __time64_t _st_mtime;
  __time64_t _st_ctime;
};

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
typedef struct _MEMORYSTATUS 
{  
	DWORD dwLength;  
	DWORD dwMemoryLoad;
	
#ifdef __APPLE__
	uint64_t dwTotalPhys;
	uint64_t dwAvailPhys;
	uint64_t dwTotalPageFile;
	uint64_t dwAvailPageFile;
	uint64_t dwTotalVirtual;
	uint64_t dwAvailVirtual;
#else
	SIZE_T dwTotalPhys;  
	SIZE_T dwAvailPhys;  
	SIZE_T dwTotalPageFile;  
	SIZE_T dwAvailPageFile;  
	SIZE_T dwTotalVirtual;  
	SIZE_T dwAvailVirtual;
#endif
} MEMORYSTATUS, *LPMEMORYSTATUS;

// Common HRESULT values
#ifndef NOERROR
#define NOERROR 					(0L)
#endif
#define S_OK						(0L)
#define E_FAIL						(0x80004005L)
#define E_OUTOFMEMORY					(0x8007000EL)
#define FAILED(Status)     				((HRESULT)(Status)<0)

// Basic D3D stuff
typedef struct _RECT { 
  LONG left; 
  LONG top; 
  LONG right; 
  LONG bottom; 
} RECT, *PRECT;

typedef DWORD D3DCOLOR;

typedef enum _D3DFORMAT
{
	D3DFMT_A8R8G8B8					= 0x00000006,
	D3DFMT_DXT1					= 0x0000000C,
	D3DFMT_DXT2					= 0x0000000E,					
	D3DFMT_DXT4					= 0x0000000F,
	D3DFMT_UNKNOWN					= 0xFFFFFFFF
} D3DFORMAT;

typedef enum D3DRESOURCETYPE
{
    D3DRTYPE_SURFACE = 1,
    D3DRTYPE_VOLUME = 2,
    D3DRTYPE_TEXTURE = 3,
    D3DRTYPE_VOLUMETEXTURE = 4,
    D3DRTYPE_CubeTexture = 5,
    D3DRTYPE_VERTEXBUFFER = 6,
    D3DRTYPE_INDEXBUFFER = 7,
    D3DRTYPE_FORCE_DWORD = 0x7fffffff
} D3DRESOURCETYPE, *LPD3DRESOURCETYPE;

typedef enum D3DXIMAGE_FILEFORMAT
{
    D3DXIFF_BMP = 0,
    D3DXIFF_JPG = 1,
    D3DXIFF_TGA = 2,
    D3DXIFF_PNG = 3,
    D3DXIFF_DDS = 4,
    D3DXIFF_PPM = 5,
    D3DXIFF_DIB = 6,
    D3DXIFF_HDR = 7,
    D3DXIFF_PFM = 8,
    D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT, *LPD3DXIMAGE_FILEFORMAT;

typedef struct D3DXIMAGE_INFO {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO, *LPD3DXIMAGE_INFO;

typedef struct _D3DPRESENT_PARAMETERS_
{
    UINT                BackBufferWidth;
    UINT                BackBufferHeight;
    D3DFORMAT           BackBufferFormat;
    UINT                BackBufferCount;
    //D3DMULTISAMPLE_TYPE MultiSampleType;
    //D3DSWAPEFFECT       SwapEffect;
    //HWND                hDeviceWindow;
    BOOL                Windowed;
    BOOL                EnableAutoDepthStencil;
    D3DFORMAT           AutoDepthStencilFormat;
    DWORD               Flags;
    UINT                FullScreen_RefreshRateInHz; 
    UINT                FullScreen_PresentationInterval;
    //D3DSurface         *BufferSurfaces[3];
    //D3DSurface         *DepthStencilSurface;
} D3DPRESENT_PARAMETERS;

typedef enum D3DPRIMITIVETYPE
{
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINESTRIP = 3,
    D3DPT_TRIANGLELIST = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6,
    D3DPT_FORCE_DWORD = 0x7fffffff
} D3DPRIMITIVETYPE, *LPD3DPRIMITIVETYPE;

typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;
        } u;
        float m[4][4];
    };
} D3DMATRIX;

typedef void DIRECT3DTEXTURE8;
typedef void* LPDIRECT3DTEXTURE8;
typedef void* LPDIRECT3DDEVICE8;

// Misc stuff found in the code, not really important
#define PAGE_READONLY 	  0x02
#define PAGE_READWRITE	  0x04
#define MAXULONG_PTR	  ((ULONG) 0xffffffff)

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
#define FILE_ATTRIBUTE_READONLY	0x00000001  
#define FILE_ATTRIBUTE_HIDDEN	0x00000002  
#define FILE_ATTRIBUTE_SYSTEM	0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY	0x00000010  

#define FILE_READ_DATA	 ( 0x0001 )    
#define FILE_WRITE_DATA  ( 0x0002 )    
#define FILE_APPEND_DATA ( 0x0004 )

#define GENERIC_READ	FILE_READ_DATA
#define GENERIC_WRITE	FILE_WRITE_DATA
#define FILE_SHARE_READ                  0x00000001
#define FILE_SHARE_WRITE                 0x00000002
#define FILE_SHARE_DELETE                0x00000004


// String
char *itoa(int i, char *a, int r);
void strlwr(char* string);
void strupr(char* string);

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
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;

#define WAVE_FORMAT_PCM 0x0001

#endif 

#endif //__PLATFORM_DEFS_H__

