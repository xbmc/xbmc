/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Win32Exception.h"
#include <eh.h>
#include <dbghelp.h>
#include "Util.h"
#include "WIN32Util.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"

#define LOG if(logger) logger->Log

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

// StackWalk64()
typedef BOOL (__stdcall *tSW)(
  DWORD MachineType,
  HANDLE hProcess,
  HANDLE hThread,
  LPSTACKFRAME64 StackFrame,
  PVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress );

// SymInitialize()
typedef BOOL (__stdcall *tSI)( IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess );
// SymCleanup()
typedef BOOL (__stdcall *tSC)( IN HANDLE hProcess );
// SymGetSymFromAddr64()
typedef BOOL (__stdcall *tSGSFA)( IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD64 pdwDisplacement, OUT PIMAGEHLP_SYMBOL64 Symbol );
// UnDecorateSymbolName()
typedef DWORD (__stdcall WINAPI *tUDSN)( PCSTR DecoratedName, PSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags );
// SymGetLineFromAddr64()
typedef BOOL (__stdcall *tSGLFA)( IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Line );
// SymGetModuleBase64()
typedef DWORD64 (__stdcall *tSGMB)( IN HANDLE hProcess, IN DWORD64 dwAddr );
// SymFunctionTableAccess64()
typedef PVOID (__stdcall *tSFTA)( HANDLE hProcess, DWORD64 AddrBase );
// SymGetOptions()
typedef DWORD (__stdcall *tSGO)( VOID );
// SymSetOptions()
typedef DWORD (__stdcall *tSSO)( IN DWORD SymOptions );

std::string win32_exception::mVersion;

void win32_exception::install_handler()
{
  _set_se_translator(win32_exception::translate);
}

void win32_exception::translate(unsigned code, EXCEPTION_POINTERS* info)
{
  switch (code)
  {
    case EXCEPTION_ACCESS_VIOLATION:
      throw access_violation(info);
      break;
    default:
      throw win32_exception(info);
  }
}

win32_exception::win32_exception(EXCEPTION_POINTERS* info, const char* classname) :
  XbmcCommons::UncheckedException(classname ? classname : "win32_exception"),
  mWhat("Win32 exception"), mWhere(info->ExceptionRecord->ExceptionAddress), mCode(info->ExceptionRecord->ExceptionCode), mExceptionPointers(info)
{
  // Windows guarantees that *(info->ExceptionRecord) is valid
  switch (info->ExceptionRecord->ExceptionCode)
  {
  case EXCEPTION_ACCESS_VIOLATION:
    mWhat = "Access violation";
    break;
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    mWhat = "Division by zero";
    break;
  }
}

void win32_exception::LogThrowMessage(const char *prefix)  const
{
  if( prefix )
    LOG(LOGERROR, "Unhandled exception in %s : %s (code:0x%08x) at 0x%08x", prefix, (unsigned int) what(), code(), where());
  else
    LOG(LOGERROR, "Unhandled exception in %s (code:0x%08x) at 0x%08x", what(), code(), where());

  write_stacktrace();
  write_minidump();
}

bool win32_exception::write_minidump(EXCEPTION_POINTERS* pEp)
{
  // Create the dump file where the xbmc.exe resides
  bool returncode = false;
  std::string dumpFileName;
  std::wstring dumpFileNameW;
  SYSTEMTIME stLocalTime;
  GetLocalTime(&stLocalTime);

  dumpFileName = StringUtils::Format("xbmc_crashlog-%s-%04d%02d%02d-%02d%02d%02d.dmp",
                      mVersion.c_str(),
                      stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                      stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

  dumpFileName = CWIN32Util::SmbToUnc(URIUtils::AddFileToFolder(CWIN32Util::GetProfilePath(), CUtil::MakeLegalFileName(dumpFileName)));

  g_charsetConverter.utf8ToW(dumpFileName, dumpFileNameW, false);
  HANDLE hDumpFile = CreateFileW(dumpFileNameW.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  if (hDumpFile == INVALID_HANDLE_VALUE)
  {
    LOG(LOGERROR, "CreateFile '%s' failed with error id %d", dumpFileName.c_str(), GetLastError());
    goto cleanup;
  }

  // Load the DBGHELP DLL
  HMODULE hDbgHelpDll = ::LoadLibrary("DBGHELP.DLL");
  if (!hDbgHelpDll)
  {
    LOG(LOGERROR, "LoadLibrary 'DBGHELP.DLL' failed with error id %d", GetLastError());
    goto cleanup;
  }

  MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDbgHelpDll, "MiniDumpWriteDump");
  if (!pDump)
  {
    LOG(LOGERROR, "Failed to locate MiniDumpWriteDump with error id %d", GetLastError());
    goto cleanup;
  }

  // Initialize minidump structure
  MINIDUMP_EXCEPTION_INFORMATION mdei;
  mdei.ThreadId = GetCurrentThreadId();
  mdei.ExceptionPointers = pEp;
  mdei.ClientPointers = FALSE;

  // Call the minidump api with normal dumping
  // We can get more detail information by using other minidump types but the dump file will be
  // extremely large.
  BOOL bMiniDumpSuccessful = pDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &mdei, 0, NULL);
  if( !bMiniDumpSuccessful )
  {
    LOG(LOGERROR, "MiniDumpWriteDump failed with error id %d", GetLastError());
    goto cleanup;
  }

  returncode = true;

cleanup:

  if (hDumpFile != INVALID_HANDLE_VALUE)
    CloseHandle(hDumpFile);

  if (hDbgHelpDll)
    FreeLibrary(hDbgHelpDll);

  return returncode;
}

/* \brief Writes a simple stack trace to the XBMC user directory.
   It needs a valid .pdb file to show the method, filename and
   line number where the exception took place.
*/
bool win32_exception::write_stacktrace(EXCEPTION_POINTERS* pEp)
{
  #define STACKWALK_MAX_NAMELEN 1024

  std::string dumpFileName, strOutput;
  std::wstring dumpFileNameW;
  CHAR cTemp[STACKWALK_MAX_NAMELEN];
  DWORD dwBytes;
  SYSTEMTIME stLocalTime;
  GetLocalTime(&stLocalTime);
  bool returncode = false;
  STACKFRAME64 frame = { 0 };
  HANDLE hCurProc = GetCurrentProcess();
  IMAGEHLP_SYMBOL64* pSym = NULL;
  HANDLE hDumpFile = INVALID_HANDLE_VALUE;
  tSC pSC = NULL;

  HMODULE hDbgHelpDll = ::LoadLibrary("DBGHELP.DLL");
  if (!hDbgHelpDll)
  {
    LOG(LOGERROR, "LoadLibrary 'DBGHELP.DLL' failed with error id %d", GetLastError());
    goto cleanup;
  }

  tSI pSI       = (tSI) GetProcAddress(hDbgHelpDll, "SymInitialize" );
  tSGO pSGO     = (tSGO) GetProcAddress(hDbgHelpDll, "SymGetOptions" );
  tSSO pSSO     = (tSSO) GetProcAddress(hDbgHelpDll, "SymSetOptions" );
  pSC           = (tSC) GetProcAddress(hDbgHelpDll, "SymCleanup" );
  tSW pSW       = (tSW) GetProcAddress(hDbgHelpDll, "StackWalk64" );
  tSGSFA pSGSFA = (tSGSFA) GetProcAddress(hDbgHelpDll, "SymGetSymFromAddr64" );
  tUDSN pUDSN   = (tUDSN) GetProcAddress(hDbgHelpDll, "UnDecorateSymbolName" );
  tSGLFA pSGLFA = (tSGLFA) GetProcAddress(hDbgHelpDll, "SymGetLineFromAddr64" );
  tSFTA pSFTA   = (tSFTA) GetProcAddress(hDbgHelpDll, "SymFunctionTableAccess64" );
  tSGMB pSGMB   = (tSGMB) GetProcAddress(hDbgHelpDll, "SymGetModuleBase64" );

  if(pSI == NULL || pSGO == NULL || pSSO == NULL || pSC == NULL || pSW == NULL || pSGSFA == NULL || pUDSN == NULL || pSGLFA == NULL ||
     pSFTA == NULL || pSGMB == NULL)
    goto cleanup;

  dumpFileName = StringUtils::Format("xbmc_stacktrace-%s-%04d%02d%02d-%02d%02d%02d.txt",
                                      mVersion.c_str(),
                                      stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                                      stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

  dumpFileName = CWIN32Util::SmbToUnc(URIUtils::AddFileToFolder(CWIN32Util::GetProfilePath(), CUtil::MakeLegalFileName(dumpFileName)));

  g_charsetConverter.utf8ToW(dumpFileName, dumpFileNameW, false);
  hDumpFile = CreateFileW(dumpFileNameW.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  if (hDumpFile == INVALID_HANDLE_VALUE)
  {
    LOG(LOGERROR, "CreateFile '%s' failed with error id %d", dumpFileName.c_str(), GetLastError());
    goto cleanup;
  }

  frame.AddrPC.Offset         = pEp->ContextRecord->Eip;      // Current location in program
  frame.AddrPC.Mode           = AddrModeFlat;                 // Address mode for this pointer: flat 32 bit addressing
  frame.AddrStack.Offset      = pEp->ContextRecord->Esp;      // Stack pointers current value
  frame.AddrStack.Mode        = AddrModeFlat;                 // Address mode for this pointer: flat 32 bit addressing
  frame.AddrFrame.Offset      = pEp->ContextRecord->Ebp;      // Value of register used to access local function variables.
  frame.AddrFrame.Mode        = AddrModeFlat;                 // Address mode for this pointer: flat 32 bit addressing

  if(pSI(hCurProc, NULL, TRUE) == FALSE)
    goto cleanup;

  DWORD symOptions = pSGO();
  symOptions |= SYMOPT_LOAD_LINES;
  symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
  symOptions &= ~SYMOPT_UNDNAME;
  symOptions &= ~SYMOPT_DEFERRED_LOADS;
  symOptions = pSSO(symOptions);

  pSym = (IMAGEHLP_SYMBOL64 *) malloc(sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
  if (!pSym)
    goto cleanup;
  memset(pSym, 0, sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
  pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
  pSym->MaxNameLength = STACKWALK_MAX_NAMELEN;

  IMAGEHLP_LINE64 Line;
  memset(&Line, 0, sizeof(Line));
  Line.SizeOfStruct = sizeof(Line);

  IMAGEHLP_MODULE64 Module;
  memset(&Module, 0, sizeof(Module));
  Module.SizeOfStruct = sizeof(Module);
  int seq=0;

  strOutput = StringUtils::Format("Thread %d (process %d)\r\n", GetCurrentThreadId(), GetCurrentProcessId());
  WriteFile(hDumpFile, strOutput.c_str(), strOutput.size(), &dwBytes, NULL);

  while(pSW(IMAGE_FILE_MACHINE_I386, hCurProc, GetCurrentThread(), &frame, pEp->ContextRecord, NULL, pSFTA, pSGMB, NULL))
  {
    if(frame.AddrPC.Offset != 0)
    {
      DWORD64 symoffset=0;
      DWORD   lineoffset=0;
      strOutput = StringUtils::Format("#%2d", seq++);

      if(pSGSFA(hCurProc, frame.AddrPC.Offset, &symoffset, pSym))
      {
        if(pUDSN(pSym->Name, cTemp, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE)>0)
          strOutput.append(StringUtils::Format(" %s", cTemp));
      }
      if(pSGLFA(hCurProc, frame.AddrPC.Offset, &lineoffset, &Line))
        strOutput.append(StringUtils::Format(" at %s:%d", Line.FileName, Line.LineNumber));

      strOutput.append("\r\n");
      WriteFile(hDumpFile, strOutput.c_str(), strOutput.size(), &dwBytes, NULL);
    }
  }
  returncode = true;

cleanup:
  if (pSym)
    free( pSym );

  if (hDumpFile != INVALID_HANDLE_VALUE)
    CloseHandle(hDumpFile);

  if(pSC)
    pSC(hCurProc);

  if (hDbgHelpDll)
    FreeLibrary(hDbgHelpDll);

  return returncode;
}

access_violation::access_violation(EXCEPTION_POINTERS* info) :
  win32_exception(info,"access_violation"), mAccessType(Invalid), mBadAddress(0)
{
  switch(info->ExceptionRecord->ExceptionInformation[0])
  {
  case 0:
    mAccessType = Read;
    break;
  case 1:
    mAccessType = Write;
    break;
  case 8:
    mAccessType = DEP;
    break;
  }
  mBadAddress = reinterpret_cast<win32_exception ::Address>(info->ExceptionRecord->ExceptionInformation[1]);
}

void access_violation::LogThrowMessage(const char *prefix) const
{
  if( prefix )
    if( mAccessType == Write)
      LOG(LOGERROR, "Unhandled exception in %s : %s at 0x%08x: Writing location 0x%08x", prefix, what(), where(), address());
    else if( mAccessType == Read)
      LOG(LOGERROR, "Unhandled exception in %s : %s at 0x%08x: Reading location 0x%08x", prefix, what(), where(), address());
    else if( mAccessType == DEP)
      LOG(LOGERROR, "Unhandled exception in %s : %s at 0x%08x: DEP violation, location 0x%08x", prefix, what(), where(), address());
    else
      LOG(LOGERROR, "Unhandled exception in %s : %s at 0x%08x: unknown access type, location 0x%08x", prefix, what(), where(), address());
  else
    if( mAccessType == Write)
      LOG(LOGERROR, "Unhandled exception in %s at 0x%08x: Writing location 0x%08x", what(), where(), address());
    else if( mAccessType == Read)
      LOG(LOGERROR, "Unhandled exception in %s at 0x%08x: Reading location 0x%08x", what(), where(), address());
    else if( mAccessType == DEP)
      LOG(LOGERROR, "Unhandled exception in %s at 0x%08x: DEP violation, location 0x%08x", what(), where(), address());
    else
      LOG(LOGERROR, "Unhandled exception in %s at 0x%08x: unknown access type, location 0x%08x", what(), where(), address());

  write_stacktrace();
  write_minidump();
}
