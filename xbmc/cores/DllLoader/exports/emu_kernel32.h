
#ifndef _EMU_KERNEL32_H_
#define _EMU_KERNEL32_H_

#define MAX_LEADBYTES             12
#define MAX_DEFAULTCHAR           2

typedef struct _STARTUPINFOA
{
  DWORD cb;
  LPSTR lpReserved;
  LPSTR lpDesktop;
  LPSTR lpTitle;
  DWORD dwX;
  DWORD dwY;
  DWORD dwXSize;
  DWORD dwYSize;
  DWORD dwXCountChars;
  DWORD dwYCountChars;
  DWORD dwFillAttribute;
  DWORD dwFlags;
  WORD wShowWindow;
  WORD cbReserved2;
  LPBYTE lpReserved2;
  HANDLE hStdInput;
  HANDLE hStdOutput;
  HANDLE hStdError;
}
STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _cpinfo
{
  UINT MaxCharSize;
  BYTE DefaultChar[MAX_DEFAULTCHAR];
  BYTE LeadByte[MAX_LEADBYTES];
}
CPINFO, *LPCPINFO;

#define STD_INPUT_HANDLE        ((DWORD) -10)
#define STD_OUTPUT_HANDLE       ((DWORD) -11)
#define STD_ERROR_HANDLE        ((DWORD) -12)

//SYSTEM_INFO definition take from Winnt headers.
typedef struct _SYSTEM_INFO
{
  union {
    DWORD dwOemId;          // Obsolete field...do not use
    struct
    {
      WORD wProcessorArchitecture;
      WORD wReserved;
    };
  };
  DWORD dwPageSize;
  LPVOID lpMinimumApplicationAddress;
  LPVOID lpMaximumApplicationAddress;
  DWORD_PTR dwActiveProcessorMask;
  DWORD dwNumberOfProcessors;
  DWORD dwProcessorType;
  DWORD dwAllocationGranularity;
  WORD wProcessorLevel;
  WORD wProcessorRevision;
}
SYSTEM_INFO, *LPSYSTEM_INFO;

typedef DWORD LCTYPE;
typedef BOOL (*PHANDLER_ROUTINE)(DWORD);
 
typedef struct _OSVERSIONINFO
{
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  TCHAR szCSDVersion[128];
}
OSVERSIONINFO, *LPOSVERSIONINFO;

#define ATOM unsigned short

//All kernel32 function should use WINAPI calling convention.
//When doing emulation or interception, the calling convention should
//match exactly the target dlls suppose to use.   Monkeyhappy
extern "C" BOOL WINAPI dllFindClose(HANDLE hFile);
extern "C" UINT WINAPI dllGetAtomNameA( ATOM nAtom, LPTSTR lpBuffer, int nSize);
extern "C" ATOM WINAPI dllFindAtomA( LPCTSTR lpString);
extern "C" ATOM WINAPI dllAddAtomA( LPCTSTR lpString);
extern "C" HANDLE WINAPI dllCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
extern "C" BOOL WINAPI dllTerminateThread(HANDLE tHread, DWORD dwExitCode);
extern "C" HANDLE WINAPI dllGetCurrentThread(void);
extern "C" DWORD WINAPI dllGetCurrentThreadId(VOID);
extern "C" DWORD WINAPI dllGetCurrentProcessId(void);
extern "C" BOOL WINAPI dllDisableThreadLibraryCalls(HANDLE);

//dllLoadLibraryA, dllFreeLibrary, dllGetProcAddress are from dllLoader,
//they are wrapper functions of COFF/PE32 loader.
extern "C" HMODULE WINAPI dllLoadLibraryA(LPCSTR libname);
extern "C" HMODULE WINAPI dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" BOOL WINAPI dllFreeLibrary(HINSTANCE hLibModule);
extern "C" FARPROC WINAPI dllGetProcAddress(HMODULE hModule, LPCSTR function);
extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName);
extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);

//GetSystemInfo are hardcoded for Xbox only.
extern "C" void WINAPI dllGetSystemInfo(LPSYSTEM_INFO lpSystemInfo);

//Current just a dummy function
extern "C" UINT WINAPI dllGetPrivateProfileIntA(LPCSTR lpAppName, LPCSTR lpKeyName,
      INT nDefault, LPCSTR lpFileName);

extern "C" void WINAPI dllDeleteCriticalSection(LPCRITICAL_SECTION cs);
extern "C" void WINAPI dllInitializeCriticalSection(LPCRITICAL_SECTION cs);
extern "C" void WINAPI dllLeaveCriticalSection(LPCRITICAL_SECTION cs);
extern "C" void WINAPI dllEnterCriticalSection(LPCRITICAL_SECTION cs);

extern "C" BOOL WINAPI dllGetVersionExA(LPOSVERSIONINFO lpVersionInfo);
extern "C" DWORD WINAPI dllGetVersion();
extern "C" UINT WINAPI dllGetProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault);

//vp6vfw.dll
extern "C" BOOL WINAPI dllFreeEnvironmentStringsW(LPWSTR lpString);
extern "C" HMODULE WINAPI dllGetOEMCP();
extern "C" HMODULE WINAPI dllRtlUnwind(PVOID TargetFrame OPTIONAL, PVOID TargetIp OPTIONAL, PEXCEPTION_RECORD ExceptionRecord OPTIONAL, PVOID ReturnValue);
extern "C" LPTSTR WINAPI dllGetCommandLineA();
extern "C" HMODULE WINAPI dllExitProcess(UINT uExitCode);
extern "C" HMODULE WINAPI dllTerminateProcess(HANDLE hProcess, UINT uExitCode);
extern "C" HMODULE WINAPI dllGetCurrentProcess();
extern "C" UINT WINAPI dllGetACP();
extern "C" UINT WINAPI dllSetHandleCount(UINT uNumber);
extern "C" HANDLE WINAPI dllGetStdHandle(DWORD nStdHandle);
extern "C" DWORD WINAPI dllGetFileType(HANDLE hFile);
extern "C" int WINAPI dllGetStartupInfoA(LPSTARTUPINFOA lpStartupInfo);
extern "C" BOOL WINAPI dllFreeEnvironmentStringsA(LPSTR lpString);
extern "C" LPVOID WINAPI dllGetEnvironmentStrings();
extern "C" LPVOID WINAPI dllGetEnvironmentStringsW();
extern "C" int WINAPI dllGetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
extern "C" HMODULE WINAPI dllLCMapStringA(LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest);
extern "C" HMODULE WINAPI dllLCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest);
extern "C" HMODULE WINAPI dllSetStdHandle(DWORD nStdHandle, HANDLE hHandle);
extern "C" HMODULE WINAPI dllGetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType);
extern "C" HMODULE WINAPI dllGetStringTypeW(DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType);
extern "C" HMODULE WINAPI dllGetCPInfo(UINT CodePage, LPCPINFO lpCPInfo);

extern "C" LCID WINAPI dllGetThreadLocale(void);
extern "C" BOOL WINAPI dllSetPriorityClass(HANDLE hProcess, DWORD dwPriorityClass);
extern "C" DWORD WINAPI dllFormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPTSTR lpBuffer, DWORD nSize, va_list* Arguments);
extern "C" DWORD WINAPI dllGetFullPathNameA(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart);
extern "C" DWORD WINAPI dllExpandEnvironmentStringsA(LPCTSTR lpSrc, LPTSTR lpDst, DWORD nSize);
extern "C" UINT WINAPI dllGetWindowsDirectoryA(LPTSTR lpBuffer, UINT uSize);
extern "C" UINT WINAPI dllGetSystemDirectoryA(LPTSTR lpBuffer, UINT uSize);

//
extern "C" HANDLE WINAPI dllHeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
extern "C" LPVOID WINAPI dllHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
extern "C" LPVOID WINAPI dllHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
extern "C" BOOL WINAPI dllHeapFree(HANDLE heap, DWORD flags, LPVOID mem);
extern "C" HANDLE WINAPI dllGetProcessHeap(void);

extern "C" UINT WINAPI dllGetShortPathName(LPTSTR lpszLongPath, LPTSTR lpszShortPath, UINT cchBuffer);
extern "C" UINT WINAPI dllSetErrorMode(UINT i);
extern "C" BOOL WINAPI dllIsProcessorFeaturePresent(DWORD ProcessorFeature);

extern "C" LPVOID WINAPI dllTlsGetValue(DWORD dwTlsIndex);
extern "C" BOOL WINAPI dllTlsSetValue(int dwTlsIndex, LPVOID lpTlsValue);
extern "C" BOOL WINAPI dllTlsFree(DWORD dwTlsIndex);
extern "C" DWORD WINAPI dllTlsAlloc();

extern "C" HANDLE WINAPI dllFindFirstFileA(LPCSTR s, LPWIN32_FIND_DATAA lpfd);
extern "C" BOOL WINAPI dllFindNextFileA(HANDLE h, LPWIN32_FIND_DATAA lpfd);

extern "C" BOOL WINAPI dllFileTimeToLocalFileTime(CONST FILETIME *lpFileTime, LPFILETIME lpLocalFileTime);
extern "C" BOOL WINAPI dllFileTimeToSystemTime(CONST FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);
extern "C" DWORD WINAPI dllGetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation);

extern "C" DWORD WINAPI dllGetFileAttributesA(LPCSTR lpFileName);

extern "C" UINT WINAPI dllGetCurrentDirectoryA(UINT c, LPSTR s);
extern "C" UINT WINAPI dllSetCurrentDirectoryA(const char *pathname);

extern "C" int WINAPI dllDuplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, HANDLE* lpTargetHandle, DWORD dwDesiredAccess, int bInheritHandle, DWORD dwOptions);

extern "C" int WINAPI dllSetUnhandledExceptionFilter(void* filter);

extern "C" int WINAPI dllSetEnvironmentVariableA(const char *name, const char *value);
extern "C" int WINAPI dllCreateDirectoryA(const char *pathname, void *sa);

extern "C" DWORD WINAPI dllWaitForSingleObject(HANDLE hHandle, DWORD dwMiliseconds);
extern "C" DWORD WINAPI dllWaitForMultipleObjects(DWORD nCount, CONST HANDLE *lpHandles, BOOL fWaitAll, DWORD dwMilliseconds);
extern "C" BOOL WINAPI dllGetProcessAffinityMask(HANDLE hProcess, LPDWORD lpProcessAffinityMask, LPDWORD lpSystemAffinityMask);

extern "C" void* WINAPI dllLocalLock(void* z);
extern "C" void* WINAPI dllLocalHandle(void* v);
extern "C" int WINAPI dllLocalUnlock(void* v);
extern "C" void* WINAPI dllGlobalHandle(void* v);
extern "C" int WINAPI dllGlobalUnlock(void* v);
extern "C" HGLOBAL WINAPI dllLoadResource(HMODULE module, HRSRC res);
extern "C" HRSRC WINAPI dllFindResourceA(HMODULE module, char* name, char* type);
extern "C" BOOL WINAPI dllGetProcessTimes(HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);
extern "C" int WINAPI dllGetLocaleInfoA(LCID Locale, LCTYPE LCType, LPTSTR lpLCData, int cchData);
extern "C" UINT WINAPI dllGetConsoleCP();
extern "C" UINT WINAPI dllGetConsoleOutputCP();
extern "C" UINT WINAPI dllSetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add);

extern "C" HANDLE xboxopendvdrom();
extern "C" void WINAPI dllSleep(DWORD dwTime);

extern "C" BOOL WINAPI dllDVDReadFileLayerChangeHack(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);

#endif // _EMU_KERNEL32_H_
