#include "..\..\..\stdafx.h"
#include "emu_kernel32.h"
#include "emu_dummy.h"
#include "..\..\..\xbox\iosupport.h"

vector<string> m_vecAtoms;

//#define API_DEBUG

extern "C" HANDLE xboxopendvdrom()
{
  CIoSupport support;
  return support.OpenCDROM();
}

extern "C" UINT WINAPI dllGetAtomNameA( ATOM nAtom, LPTSTR lpBuffer, int nSize)
{
  if (nAtom < 1 || nAtom > m_vecAtoms.size() ) return 0;
  nAtom--;
  string& strAtom = m_vecAtoms[nAtom];
  strcpy(lpBuffer, strAtom.c_str());
  return strAtom.size();
}

extern "C" ATOM WINAPI dllFindAtomA( LPCTSTR lpString)
{
  for (int i = 0; i < (int)m_vecAtoms.size(); ++i)
  {
    string& strAtom = m_vecAtoms[i];
    if (strAtom == lpString) return i + 1;
  }
  return 0;
}

extern "C" ATOM WINAPI dllAddAtomA( LPCTSTR lpString)
{
  m_vecAtoms.push_back(lpString);
  return m_vecAtoms.size();
}

extern "C" BOOL WINAPI dllFindClose(HANDLE hFile)
{
  return FindClose(hFile);
}

// should be moved to CFile! or use CFile::stat
extern "C" DWORD WINAPI dllGetFileAttributesA(LPCSTR lpFileName)
{
  char str[XBMC_MAX_PATH];
  char* p;

  if (!strcmp(lpFileName, "\\Device\\Cdrom0")) return (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY);

  // move to CFile classes
  if (strncmp(lpFileName, "\\Device\\Cdrom0", 14) == 0)
  {
    // replace "\\Device\\Cdrom0" with "D:"
    strcpy(str, "D:");
    strcat(str, lpFileName + 14);
  }
  else strcpy(str, lpFileName);

  // convert '/' to '\\'
  p = str;
  while (p = strchr(p, '/')) * p = '\\';
  return GetFileAttributesA(str);
}

extern "C" BOOL WINAPI dllTerminateThread(HANDLE tHread, DWORD dwExitCode)
{
  not_implement("kernel32.dll fake function TerminateThread called\n");  //warning
  return TRUE;
}

extern "C" void WINAPI dllSleep(DWORD dwTime)
{
  return ::Sleep(dwTime);
}

extern "C" HANDLE WINAPI dllGetCurrentThread(void)
{
  HANDLE retval = GetCurrentThread();
  return retval;
}

extern "C" DWORD WINAPI dllGetCurrentProcessId(void)
{
  //not_implement("kernel32.dll fake function GetCurrentProcessId called\n");  //warning
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetCurrentProcessId(void) => 31337");
#endif
  return 31337;
}

extern "C" BOOL WINAPI dllGetProcessTimes(HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime)
{
  // since the xbox has only one process, we just take the current thread
  HANDLE h = GetCurrentThread();
  BOOL res = GetThreadTimes(h, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime);
  
  return res;
}

extern "C" int WINAPI dllDuplicateHandle(HANDLE hSourceProcessHandle,   // handle to source process
      HANDLE hSourceHandle,          // handle to duplicate
      HANDLE hTargetProcessHandle,   // handle to target process
      HANDLE* lpTargetHandle,       // duplicate handle
      DWORD dwDesiredAccess,         // requested access
      int bInheritHandle,           // handle inheritance option
      DWORD dwOptions               // optional actions
                                          )
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "DuplicateHandle(%p, %p, %p, %p, 0x%x, %d, %d) called\n",
            hSourceProcessHandle, hSourceHandle, hTargetProcessHandle,
            lpTargetHandle, dwDesiredAccess, bInheritHandle, dwOptions);
#endif
  *lpTargetHandle = hSourceHandle;
  return 1;
}

extern "C" BOOL WINAPI dllDisableThreadLibraryCalls(HANDLE h)
{
  not_implement("kernel32.dll fake function DisableThreadLibraryCalls called\n"); //warning
  return TRUE;
}

static void DumpSystemInfo(const SYSTEM_INFO* si)
{
  CLog::Log(LOGDEBUG, "  Processor architecture %d\n", si->wProcessorArchitecture);
  CLog::Log(LOGDEBUG, "  Page size: %d\n", si->dwPageSize);
  CLog::Log(LOGDEBUG, "  Minimum app address: %d\n", si->lpMinimumApplicationAddress);
  CLog::Log(LOGDEBUG, "  Maximum app address: %d\n", si->lpMaximumApplicationAddress);
  CLog::Log(LOGDEBUG, "  Active processor mask: 0x%x\n", si->dwActiveProcessorMask);
  CLog::Log(LOGDEBUG, "  Number of processors: %d\n", si->dwNumberOfProcessors);
  CLog::Log(LOGDEBUG, "  Processor type: 0x%x\n", si->dwProcessorType);
  CLog::Log(LOGDEBUG, "  Allocation granularity: 0x%x\n", si->dwAllocationGranularity);
  CLog::Log(LOGDEBUG, "  Processor level: 0x%x\n", si->wProcessorLevel);
  CLog::Log(LOGDEBUG, "  Processor revision: 0x%x\n", si->wProcessorRevision);
}

extern "C" void WINAPI dllGetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetSystemInfo(0x%x) =>", lpSystemInfo);
#endif
  lpSystemInfo->wProcessorArchitecture = 0; //#define PROCESSOR_ARCHITECTURE_INTEL 0
  lpSystemInfo->dwPageSize = 4096;   //Xbox page size
  lpSystemInfo->lpMinimumApplicationAddress = (void *)0x00000000;
  lpSystemInfo->lpMaximumApplicationAddress = (void *)0x7fffffff;
  lpSystemInfo->dwActiveProcessorMask = 1;
  lpSystemInfo->dwNumberOfProcessors = 1;
  lpSystemInfo->dwProcessorType = 586;  //#define PROCESSOR_INTEL_PENTIUM 586
  lpSystemInfo->wProcessorLevel = 6;
  //lpSystemInfo->wProcessorLevel = 5;
  lpSystemInfo->wProcessorRevision = 0x080A;
  lpSystemInfo->dwAllocationGranularity = 0x10000; //virtualalloc reserve block size
#ifdef API_DEBUG
  DumpSystemInfo(lpSystemInfo);
#endif
}  //hardcode for xbox processor type;

extern "C" UINT WINAPI dllGetPrivateProfileIntA(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    INT nDefault,
    LPCSTR lpFileName)
{
  not_implement("kernel32.dll fake function GetPrivateProfileIntA called\n"); //warning
  return nDefault;
}

//globals for memory leak hack, no need for well-behaved dlls call init/del in pair
//We can free the sections if applications does not call deletecriticalsection
//need to initialize the list head NULL at mplayer_open_file, and free memory at close file.
CriticalSection_List * criticalsection_head;

CriticalSection_List::CriticalSection_List()
{
  CriticalSection_List ** curr = &criticalsection_head;
  Next = NULL;

  while ( *curr ) curr = &((*curr)->Next);
  *curr = this;
}

CriticalSection_List::~CriticalSection_List()
{
  CriticalSection_List ** curr = &criticalsection_head;

  while ( *curr && *curr != this ) curr = &((*curr)->Next);
  if ( *curr )  //delete node
  {
    *curr = Next;
    Next = NULL;
  }
}

extern "C" void WINAPI dllDeleteCriticalSection(CriticalSection_List ** fixc)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "DeleteCriticalSection(0x%x)", fixc);
#endif
  DeleteCriticalSection(&((*fixc)->criticalsection));
  delete *fixc;
  //Fix different CRITICAL_SECTION size between win2K and Xbox
  //But need application call  Initialize../Delete.. in pair.
  //For bad behave applications, use criticalsection_head to free memory
}

extern "C" void WINAPI dllInitializeCriticalSection(CriticalSection_List ** fixc)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "InitializeCriticalSection(0x%x)", fixc);
#endif
  *fixc = (CriticalSection_List *) new CriticalSection_List;
  InitializeCriticalSection(&((*fixc)->criticalsection));
}

extern "C" void WINAPI dllLeaveCriticalSection(CriticalSection_List ** fixc)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "LeaveCriticalSection(0x%x) %p\n", fixc, &((*fixc)->criticalsection));
#endif
  LeaveCriticalSection(&((*fixc)->criticalsection));
}

extern "C" void WINAPI dllEnterCriticalSection(CriticalSection_List ** fixc)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "EnterCriticalSection(0x%x) %p\n", fixc, &((*fixc)->criticalsection));
#endif
  if (!&((*fixc)->criticalsection))
  {
#ifdef API_DEBUG
    CLog::Log(LOGDEBUG, "entered uninitialized critisec!\n");
#endif
    dllInitializeCriticalSection(fixc);
  }
  EnterCriticalSection(&((*fixc)->criticalsection));
}

extern "C" DWORD WINAPI dllGetVersion()
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetVersion() => 0xC0000004 (Windows 95)\n");
#endif
  //return 0x0a280105; //Windows XP
  return 0xC0000004; //Windows 95
}

extern "C" BOOL WINAPI dllGetVersionExA(LPOSVERSIONINFO lpVersionInfo)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetVersionExA(0x%x) => 1\n");
#endif
  lpVersionInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  lpVersionInfo->dwMajorVersion = 4;
  lpVersionInfo->dwMinorVersion = 0;
  lpVersionInfo->dwBuildNumber = 0x4000457;
  // leave it here for testing win9x-only codecs
  lpVersionInfo->dwPlatformId = 1; //VER_PLATFORM_WIN32_WINDOWS
  strcpy(lpVersionInfo->szCSDVersion, " B");
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "  Major version: 4\n  Minor version: 0\n  Build number: 0x4000457\n"
            "  Platform Id: VER_PLATFORM_WIN32_NT\n Version string: 'Service Pack 3'\n");
#endif
  return 1;
}

extern "C" UINT WINAPI dllGetProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault)
{
  //  CLog::Log(LOGDEBUG,"GetProfileIntA:%s %s %i", lpAppName,lpKeyName,nDefault);
  not_implement("kernel32.dll fake function GetProfileIntA called\n"); //warning
  return nDefault;
}

extern "C" BOOL WINAPI dllFreeEnvironmentStringsW(LPWSTR lpString)
{
  // we don't have anything to clean up here, just return.
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "FreeEnvironmentStringsA(0x%x) => 1", lpString);
#endif
  return true;
}

extern "C" HMODULE WINAPI dllGetOEMCP()
{
  not_implement("kernel32.dll fake function GetOEMCP called\n"); //warning
  return NULL;
}

extern "C" HMODULE WINAPI dllRtlUnwind(PVOID TargetFrame OPTIONAL, PVOID TargetIp OPTIONAL, PEXCEPTION_RECORD ExceptionRecord OPTIONAL, PVOID ReturnValue)
{
  not_implement("kernel32.dll fake function RtlUnwind called\n"); //warning
  return NULL;
}
extern "C" LPTSTR WINAPI dllGetCommandLineA()
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetCommandLineA() => \"c:\\xbmc.xbe\"\n");
#endif
  return "c:\\xbmc.xbe";
}

extern "C" HMODULE WINAPI dllExitProcess(UINT uExitCode)
{
  not_implement("kernel32.dll fake function ExitProcess called\n"); //warning
  return NULL;
}
extern "C" HMODULE WINAPI dllTerminateProcess(HANDLE hProcess, UINT uExitCode)
{
  not_implement("kernel32.dll fake function TerminateProcess called\n"); //warning
  return NULL;
}
extern "C" HMODULE WINAPI dllGetCurrentProcess()
{
  //not_implement("kernel32.dll fake function GetCurrentProcess called\n"); //warning
  //return NULL;
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetCurrentProcess(void) => 9375");
#endif
  return (HMODULE)9375;
}

extern "C" UINT WINAPI dllGetACP()
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetACP() => 0");
#endif
  return CP_ACP;
}

extern "C" UINT WINAPI dllSetHandleCount(UINT uNumber)
{
  //Under Windows NT and Windows 95, this function simply returns the value specified in the uNumber parameter.
  //return uNumber;
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "SetHandleCount(0x%x) => 1\n", uNumber);
#endif
  return uNumber;
}

extern "C" HANDLE WINAPI dllGetStdHandle(DWORD nStdHandle)
{
  switch (nStdHandle)
  {
  case STD_INPUT_HANDLE: return (HANDLE)0;
  case STD_OUTPUT_HANDLE: return (HANDLE)1;
  case STD_ERROR_HANDLE: return (HANDLE)2;
  }
  SetLastError( ERROR_INVALID_PARAMETER );
  return INVALID_HANDLE_VALUE;
}

#define FILE_TYPE_UNKNOWN       0
#define FILE_TYPE_DISK          1
#define FILE_TYPE_CHAR          2

extern "C" DWORD WINAPI dllGetFileType(HANDLE hFile)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetFileType(0x%x) => 0x3 = pipe", hFile);
#endif
  return 3;
}

extern "C" int WINAPI dllGetStartupInfoA(LPSTARTUPINFOA lpStartupInfo)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetStartupInfoA(0x%x) => 1\n");
#endif
  lpStartupInfo->cb = sizeof(_STARTUPINFOA);
  lpStartupInfo->cbReserved2 = 0;
  lpStartupInfo->dwFillAttribute = 0;
  lpStartupInfo->dwFlags = 0;
  lpStartupInfo->dwX = 50; //
  lpStartupInfo->dwXCountChars = 0;
  lpStartupInfo->dwXSize = 0;
  lpStartupInfo->dwY = 50; //
  lpStartupInfo->dwYCountChars = 0;
  lpStartupInfo->dwYSize = 0;
  lpStartupInfo->hStdError = (HANDLE)2;
  lpStartupInfo->hStdInput = (HANDLE)0;
  lpStartupInfo->hStdOutput = (HANDLE)1;
  lpStartupInfo->lpDesktop = NULL;
  lpStartupInfo->lpReserved = NULL;
  lpStartupInfo->lpReserved2 = 0;
  lpStartupInfo->lpTitle = "Xbox Media Center";
  lpStartupInfo->wShowWindow = 0;
  return 1;
}

extern "C" BOOL WINAPI dllFreeEnvironmentStringsA(LPSTR lpString)
{
  // we don't have anything to clean up here, just return.
  return true;
}

static const char ch_envs[] =
  "__MSVCRT_HEAP_SELECT=__GLOBAL_HEAP_SELETED,1\r\n"
  "PATH=C:\\;C:\\windows\\;C:\\windows\\system\r\n";

extern "C" LPVOID WINAPI dllGetEnvironmentStrings()
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetEnvironmentStrings() => 0x%x = %p", ch_envs, ch_envs);
#endif
  return (LPVOID)ch_envs;
}

extern "C" LPVOID WINAPI dllGetEnvironmentStringsW()
{
  return 0;
}

extern "C" int WINAPI dllGetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize)
{
  if (lpBuffer) lpBuffer[0] = 0;

  if (strcmp(lpName, "__MSVCRT_HEAP_SELECT") == 0)
    strcpy(lpBuffer, "__GLOBAL_HEAP_SELECTED,1");
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetEnvironmentVariableA('%s', 0x%x, %d) => %d", lpName, lpBuffer, nSize, strlen(lpBuffer));
#endif
  return strlen(lpBuffer);
}

extern "C" HMODULE WINAPI dllLCMapStringA(LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest)
{
  not_implement("kernel32.dll fake function LCMapStringA called\n"); //warning
  return NULL;
}

extern "C" HMODULE WINAPI dllLCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest)
{
  not_implement("kernel32.dll fake function LCMapStringW called\n"); //warning
  return NULL;
}

extern "C" HMODULE WINAPI dllSetStdHandle(DWORD nStdHandle, HANDLE hHandle)
{
  not_implement("kernel32.dll fake function SetStdHandle called\n"); //warning
  return NULL;
}

extern "C" HMODULE WINAPI dllGetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
  not_implement("kernel32.dll fake function GetStringTypeA called\n"); //warning
  return NULL;
}

extern "C" HMODULE WINAPI dllGetStringTypeW(DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
  not_implement("kernel32.dll fake function GetStringTypeW called\n"); //warning
  return NULL;
}

extern "C" HMODULE WINAPI dllGetCPInfo(UINT CodePage, LPCPINFO lpCPInfo)
{
  not_implement("kernel32.dll fake function GetCPInfo called\n"); //warning
  return NULL;
}

extern "C" LCID WINAPI dllGetThreadLocale(void)
{
  // primary language identifier, sublanguage identifier, sorting identifier
  return MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
}

extern "C" BOOL WINAPI dllSetPriorityClass(HANDLE hProcess, DWORD dwPriorityClass)
{
  not_implement("kernel32.dll fake function SetPriorityClass called\n"); //warning
  return NULL;
}

extern "C" DWORD WINAPI dllFormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPTSTR lpBuffer, DWORD nSize, va_list* Arguments)
{
  not_implement("kernel32.dll fake function FormatMessage called\n"); //warning
  return NULL;
}

extern "C" DWORD WINAPI dllGetFullPathNameA(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart)
{
  if (!lpFileName) return 0;
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetFullPathNameA('%s',%d,%p,%p)\n", lpFileName, nBufferLength, lpBuffer, lpFilePart);
#endif
  if (strrchr(lpFileName, '\\'))
    lpFilePart = (LPSTR*)strrchr((const char *)lpFileName, '\\');
  else
    lpFilePart = (LPTSTR *)lpFileName;

  strcpy(lpBuffer, lpFileName);
  return strlen(lpBuffer);
}

extern "C" DWORD WINAPI dllExpandEnvironmentStringsA(LPCTSTR lpSrc, LPTSTR lpDst, DWORD nSize)
{
  not_implement("kernel32.dll fake function GetFullPathNameA called\n"); //warning
  return NULL;
}

extern "C" UINT WINAPI dllGetWindowsDirectoryA(LPTSTR lpBuffer, UINT uSize)
{
  not_implement("kernel32.dll fake function dllGetWindowsDirectory called\n"); //warning
  return NULL;
}

extern "C" UINT WINAPI dllGetSystemDirectoryA(LPTSTR lpBuffer, UINT uSize)
{
  //char* systemdir = "q:\\mplayer\\codecs";
  //unsigned int len = strlen(systemdir);
  //if (len > uSize) return 0;
  //strcpy(lpBuffer, systemdir);
  //not_implement("kernel32.dll incompete function dllGetSystemDirectory called\n"); //warning
  //CLog::Log(LOGDEBUG,"KERNEL32!GetSystemDirectoryA(0x%x, %d) => %s", lpBuffer, uSize, systemdir);
  //return len;
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetSystemDirectoryA(%p,%d)\n", lpBuffer, uSize);
#endif
  if (!lpBuffer) strcpy(lpBuffer, ".");
  return 1;
}


extern "C" UINT WINAPI dllGetShortPathName(LPTSTR lpszLongPath, LPTSTR lpszShortPath, UINT cchBuffer)
{
  if (!lpszLongPath) return 0;
  if (strlen(lpszLongPath) == 0)
  {
    //strcpy(lpszLongPath, "Q:\\mplayer\\codecs\\QuickTime.qts");
  }
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "KERNEL32!GetShortPathNameA('%s',%p,%d)\n", lpszLongPath, lpszShortPath, cchBuffer);
#endif
  strcpy(lpszShortPath, lpszLongPath);
  return strlen(lpszShortPath);
}

extern "C" HANDLE WINAPI dllGetProcessHeap()
{
  HANDLE hHeap;
  hHeap = GetProcessHeap();
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "KERNEL32!GetProcessHeap() => 0x%x", hHeap);
#endif
  return hHeap;
}

extern "C" UINT WINAPI dllSetErrorMode(UINT i)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "SetErrorMode(%d) => 0\n", i);
#endif
  return 0;
}

extern "C" BOOL WINAPI dllIsProcessorFeaturePresent(DWORD ProcessorFeature)
{
  BOOL result = 0;
  switch (ProcessorFeature)
  {
  case PF_3DNOW_INSTRUCTIONS_AVAILABLE:
    result = false;
    break;
  case PF_COMPARE_EXCHANGE_DOUBLE:
    result = true;
    break;
  case PF_FLOATING_POINT_EMULATED:
    result = true;
    break;
  case PF_FLOATING_POINT_PRECISION_ERRATA:
    result = false;
    break;
  case PF_MMX_INSTRUCTIONS_AVAILABLE:
    result = true;
    break;
  case PF_PAE_ENABLED:
    result = true;
    break;
  case PF_RDTSC_INSTRUCTION_AVAILABLE:
    result = true;
    break;
  case PF_XMMI_INSTRUCTIONS_AVAILABLE:
    result = true;
    break;
  case 10: //PF_XMMI64_INSTRUCTIONS_AVAILABLE
    result = true;
    break;
  }

#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "IsProcessorFeaturePresent(0x%x) => 0x%x\n", ProcessorFeature, result);
#endif
  return result;
}

extern "C" DWORD WINAPI dllTlsAlloc()
{
  DWORD retval = TlsAlloc();
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "TlsAlloc() => %d\n", retval);
#endif
  return retval;
}

extern "C" BOOL WINAPI dllTlsFree(DWORD dwTlsIndex)
{
  BOOL retval = TlsFree(dwTlsIndex);
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "KERNEL32!TlsFree(%d) => %d", dwTlsIndex, retval);
#endif
  return retval;
}

extern "C" BOOL WINAPI dllTlsSetValue(int dwTlsIndex, LPVOID lpTlsValue)
{
  //if ((int)lpTlsValue == 0x4d6f6f56) {
  // __asm {
  //  int 3;
  // }
  //}

  BOOL retval = TlsSetValue(dwTlsIndex, lpTlsValue);
  if (retval)
  {
    __asm {
      mov ecx, dwTlsIndex;
      mov ebx, lpTlsValue
      mov eax, fs: [0x18]
      mov [eax + 0x88 + ecx*4], ebx
    }
  }
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "KERNEL32!TlsSetValue(%d, 0x%x) => %d", dwTlsIndex, lpTlsValue, retval);
#endif
  return retval;
}

extern "C" LPVOID WINAPI dllTlsGetValue(DWORD dwTlsIndex)
{
  LPVOID retval = TlsGetValue(dwTlsIndex);
  if (retval)
  {
    __asm {
      mov ecx, dwTlsIndex;
  mov eax, fs: [0x18]
      mov eax, [eax + 0x88 + ecx*4]
      mov retval, eax
    }
  }

#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "KERNEL32!TlsGetValue(%d) => 0x%x", dwTlsIndex, retval);
#endif
  return retval;
}

extern "C" UINT WINAPI dllGetCurrentDirectoryA(UINT c, LPSTR s)
{
  char curdir[] = "Q:\\";
  int result;
  strncpy(s, curdir, c);
  result = 1 + ((c < strlen(curdir)) ? c : strlen(curdir));
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "GetCurrentDirectoryA(0x%x, %d) => %d\n", s, c, result);
#endif
  return result;
}

extern "C" UINT WINAPI dllSetCurrentDirectoryA(const char *pathname)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "SetCurrentDirectoryA(0x%x = %s) => 1\n", pathname, pathname);
#endif
  return 1;
}

extern "C" int WINAPI dllSetUnhandledExceptionFilter(void* filter)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "SetUnhandledExceptionFilter(0x%x) => 1\n", filter);
#endif
  return 1; //unsupported and probably won't ever be supported
}

extern "C" int WINAPI dllSetEnvironmentVariableA(const char *name, const char *value)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "SetEnvironmentVariableA(%s, %s)\n", name, value);
#endif
  return 0;
}

extern "C" int WINAPI dllCreateDirectoryA(const char *pathname, void *sa)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "CreateDirectory(0x%x = %s, 0x%x) => 1\n", pathname, pathname, sa);
#endif
  return 1;
}

extern "C" DWORD WINAPI dllWaitForSingleObject(HANDLE hHandle, DWORD dwMiliseconds)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "WaitForSingleObject(0x%x, %d)", hHandle, dwMiliseconds);
#endif
  return WaitForSingleObject(hHandle, dwMiliseconds);
}

extern "C" DWORD WINAPI dllWaitForMultipleObjects(DWORD nCount, CONST HANDLE *lpHandles, BOOL fWaitAll, DWORD dwMilliseconds)
{
#ifdef API_DEBUG
  CLog::Log(LOGDEBUG, "WaitForMultipleObjects(..)");
#endif
  return WaitForMultipleObjects(nCount, lpHandles, fWaitAll, dwMilliseconds);
}

extern "C" BOOL WINAPI dllGetProcessAffinityMask(HANDLE hProcess, LPDWORD lpProcessAffinityMask, LPDWORD lpSystemAffinityMask)
{
  CLog::Log(LOGDEBUG, "GetProcessAffinityMask(0x%x, 0x%x, 0x%x) => 1\n",
            hProcess, lpProcessAffinityMask, lpSystemAffinityMask);
  if (lpProcessAffinityMask)*lpProcessAffinityMask = 1;
  if (lpSystemAffinityMask)*lpSystemAffinityMask = 1;
  return 1;
}
extern "C" int WINAPI dllGetLocaleInfoA(LCID Locale, LCTYPE LCType, LPTSTR lpLCData, int cchData)
{
  not_implement("kernel32.dll fake function GetLocaleInfoA called\n");  //warning
  SetLastError(ERROR_INVALID_FUNCTION);
  return 0;
}

extern "C" UINT WINAPI dllGetConsoleCP()
{
  return 437; // OEM - United States 
}

extern "C" UINT WINAPI dllGetConsoleOutputCP()
{
  return 437; // OEM - United States 
}

extern "C" UINT WINAPI dllSetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add)
{
  // no consoles exists on the xbox, do nothing
  not_implement("kernel32.dll fake function SetConsoleCtrlHandler called\n");  //warning
  SetLastError(ERROR_INVALID_FUNCTION);
  return 0;
}

/*

The following routine was hacked up by JM while looking at why the DVD player was failing
in the middle of the movie.  The symptoms were:

1. DVD player returned error about expecting a NAV packet but none found.
2. Resulted in DVD player closing.
3. Always occured in the same place.
4. Occured on every DVD I tried (originals)
5. Approximately where I would expect the layer change to be (ie just over half way
   through the movie)
6. Resulted in the last chunk of the requested data to be NULL'd out completely.  ReadFile()
   returns correctly, but the last chunk is completely zero'd out.

This routine checks the last chunk for zeros, and re-reads if necessary.
*/
#define DVD_CHUNK_SIZE 2048

extern "C" BOOL WINAPI dllDVDReadFileLayerChangeHack(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
  BOOL ret = ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
  if (!ret || !lpNumberOfBytesRead || *lpNumberOfBytesRead < DVD_CHUNK_SIZE) return ret;
  DWORD numChecked = *lpNumberOfBytesRead;
  while (numChecked >= DVD_CHUNK_SIZE)
  {
    int p = *(int *)((BYTE *)lpBuffer + numChecked - DVD_CHUNK_SIZE);
    if (p == 0)
    { // reread this block
      LONG low = 0;
      LONG high = 0;
      low = SetFilePointer(hFile, low, &high, FILE_CURRENT);
      CLog::Log(LOGWARNING, "DVDReadFile() warning - invalid data read from block at %d (%d) - rereading", low, high);
      DWORD numRead;
      SetFilePointer(hFile, (int)numChecked - (int)*lpNumberOfBytesRead - DVD_CHUNK_SIZE, NULL, FILE_CURRENT);
      ret = ReadFile(hFile, (BYTE *)lpBuffer + numChecked - DVD_CHUNK_SIZE, DVD_CHUNK_SIZE, &numRead, lpOverlapped);
      if (!ret) return FALSE;
      SetFilePointer(hFile, low, &high, FILE_BEGIN);
    }
    numChecked -= DVD_CHUNK_SIZE;
  }
  return ret;
}
