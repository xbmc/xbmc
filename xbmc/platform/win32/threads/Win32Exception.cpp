/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32Exception.h"

#include "Util.h"
#include "WIN32Util.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "platform/win32/CharsetConverter.h"

#include <VersionHelpers.h>
#include <dbghelp.h>

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        const PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        const PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        const PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

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

// GetCurrentPackageFullName
typedef LONG (__stdcall *GCPFN)(UINT32*, PWSTR);

std::string win32_exception::mVersion;
bool win32_exception::m_platformDir;

bool win32_exception::write_minidump(EXCEPTION_POINTERS* pEp)
{
  // Create the dump file where the xbmc.exe resides
  bool returncode = false;
  std::string dumpFileName;
  std::wstring dumpFileNameW;
  KODI::TIME::SystemTime stLocalTime;
  KODI::TIME::GetLocalTime(&stLocalTime);

  dumpFileName = StringUtils::Format("kodi_crashlog-{}-{:04}{:02}{:02}-{:02}{:02}{:02}.dmp",
                                     mVersion, stLocalTime.year, stLocalTime.month, stLocalTime.day,
                                     stLocalTime.hour, stLocalTime.minute, stLocalTime.second);

  dumpFileName = CWIN32Util::SmbToUnc(
      URIUtils::AddFileToFolder(CWIN32Util::GetProfilePath(m_platformDir),
                                CUtil::MakeLegalFileName(std::move(dumpFileName))));

  dumpFileNameW = KODI::PLATFORM::WINDOWS::ToW(dumpFileName);
  HANDLE hDumpFile = CreateFileW(dumpFileNameW.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  HMODULE hDbgHelpDll = nullptr;
  BOOL bMiniDumpSuccessful = FALSE;
  MINIDUMPWRITEDUMP pDump = nullptr;

  if (hDumpFile == INVALID_HANDLE_VALUE)
  {
    goto cleanup;
  }

  // Load the DBGHELP DLL
  hDbgHelpDll = ::LoadLibrary(L"DBGHELP.DLL");
  if (!hDbgHelpDll)
  {
    goto cleanup;
  }

  pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDbgHelpDll, "MiniDumpWriteDump");
  if (!pDump)
  {
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
  bMiniDumpSuccessful =
      pDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &mdei, 0, NULL);
  if( !bMiniDumpSuccessful )
  {
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
  KODI::TIME::SystemTime stLocalTime;
  KODI::TIME::GetLocalTime(&stLocalTime);
  bool returncode = false;
  STACKFRAME64 frame = {};
  HANDLE hCurProc = GetCurrentProcess();
  IMAGEHLP_SYMBOL64* pSym = NULL;
  HANDLE hDumpFile = INVALID_HANDLE_VALUE;
  tSC pSC = nullptr;
  IMAGEHLP_LINE64 Line = {};
  Line.SizeOfStruct = sizeof(Line);
  tSI pSI = nullptr;
  tSGO pSGO = nullptr;
  tSSO pSSO = nullptr;
  tSW pSW = nullptr;
  tSGSFA pSGSFA = nullptr;
  tUDSN pUDSN = nullptr;
  tSGLFA pSGLFA = nullptr;
  tSFTA pSFTA = nullptr;
  tSGMB pSGMB = nullptr;
  DWORD symOptions = 0;
  int seq = 0;

  HMODULE hDbgHelpDll = ::LoadLibrary(L"DBGHELP.DLL");
  if (!hDbgHelpDll)
  {
    goto cleanup;
  }

  pSI = (tSI)GetProcAddress(hDbgHelpDll, "SymInitialize");
  pSGO = (tSGO)GetProcAddress(hDbgHelpDll, "SymGetOptions");
  pSSO = (tSSO)GetProcAddress(hDbgHelpDll, "SymSetOptions");
  pSC           = (tSC) GetProcAddress(hDbgHelpDll, "SymCleanup" );
  pSW = (tSW)GetProcAddress(hDbgHelpDll, "StackWalk64");
  pSGSFA = (tSGSFA)GetProcAddress(hDbgHelpDll, "SymGetSymFromAddr64");
  pUDSN = (tUDSN)GetProcAddress(hDbgHelpDll, "UnDecorateSymbolName");
  pSGLFA = (tSGLFA)GetProcAddress(hDbgHelpDll, "SymGetLineFromAddr64");
  pSFTA = (tSFTA)GetProcAddress(hDbgHelpDll, "SymFunctionTableAccess64");
  pSGMB = (tSGMB)GetProcAddress(hDbgHelpDll, "SymGetModuleBase64");

  if(pSI == NULL || pSGO == NULL || pSSO == NULL || pSC == NULL || pSW == NULL || pSGSFA == NULL || pUDSN == NULL || pSGLFA == NULL ||
     pSFTA == NULL || pSGMB == NULL)
    goto cleanup;

  dumpFileName = StringUtils::Format("kodi_stacktrace-{}-{:04}{:02}{:02}-{:02}{:02}{:02}.txt",
                                     mVersion, stLocalTime.year, stLocalTime.month, stLocalTime.day,
                                     stLocalTime.hour, stLocalTime.minute, stLocalTime.second);

  dumpFileName = CWIN32Util::SmbToUnc(
      URIUtils::AddFileToFolder(CWIN32Util::GetProfilePath(m_platformDir),
                                CUtil::MakeLegalFileName(std::move(dumpFileName))));

  dumpFileNameW = KODI::PLATFORM::WINDOWS::ToW(dumpFileName);
  hDumpFile = CreateFileW(dumpFileNameW.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  if (hDumpFile == INVALID_HANDLE_VALUE)
  {
    goto cleanup;
  }

  frame.AddrPC.Mode = AddrModeFlat; // Address mode for this pointer: flat 32 bit addressing
  frame.AddrStack.Mode = AddrModeFlat; // Address mode for this pointer: flat 32 bit addressing
  frame.AddrFrame.Mode = AddrModeFlat; // Address mode for this pointer: flat 32 bit addressing

#if defined(_X86_)
  frame.AddrPC.Offset = pEp->ContextRecord->Eip; // Current location in program
  frame.AddrStack.Offset = pEp->ContextRecord->Esp; // Stack pointers current value
  frame.AddrFrame.Offset = pEp->ContextRecord->Ebp; // Value of register used to access local function variables.
#else
  frame.AddrPC.Offset = pEp->ContextRecord->Rip; // Current location in program
  frame.AddrStack.Offset = pEp->ContextRecord->Rsp; // Stack pointers current value
  frame.AddrFrame.Offset = pEp->ContextRecord->Rbp; // Value of register used to access local function variables.
#endif

  if(pSI(hCurProc, NULL, TRUE) == FALSE)
    goto cleanup;

  symOptions = pSGO();
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


  strOutput = StringUtils::Format("Thread {} (process {})\r\n", GetCurrentThreadId(),
                                  GetCurrentProcessId());
  WriteFile(hDumpFile, strOutput.c_str(), strOutput.size(), &dwBytes, NULL);

  while(pSW(IMAGE_FILE_MACHINE_I386, hCurProc, GetCurrentThread(), &frame, pEp->ContextRecord, NULL, pSFTA, pSGMB, NULL))
  {
    if(frame.AddrPC.Offset != 0)
    {
      DWORD64 symoffset=0;
      DWORD   lineoffset=0;
      strOutput = StringUtils::Format("#{:2}", seq++);

      if(pSGSFA(hCurProc, frame.AddrPC.Offset, &symoffset, pSym))
      {
        if(pUDSN(pSym->Name, cTemp, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE)>0)
          strOutput.append(StringUtils::Format(" {}", cTemp));
      }
      if(pSGLFA(hCurProc, frame.AddrPC.Offset, &lineoffset, &Line))
        strOutput.append(StringUtils::Format(" at {}:{}", Line.FileName, Line.LineNumber));

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

bool win32_exception::ShouldHook()
{
  bool result = true;

  auto module = ::LoadLibrary(L"kernel32.dll");
  if (module)
  {
    auto func = reinterpret_cast<GCPFN>(::GetProcAddress(module, "GetCurrentPackageFullName"));
    if (func)
    {
      UINT32 length = 0;
      auto r = func(&length, nullptr);
      result = r == APPMODEL_ERROR_NO_PACKAGE;
    }

    ::FreeLibrary(module);
  }

  return result;
}
