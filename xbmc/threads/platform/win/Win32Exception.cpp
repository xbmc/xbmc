/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#define LOG if(logger) logger->Log

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

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
  write_minidump();
}

bool win32_exception::write_minidump(EXCEPTION_POINTERS* pEp)
{
  // Create the dump file where the xbmc.exe resides
  bool returncode = false;
  CStdString dumpFileName;
  SYSTEMTIME stLocalTime;
  GetLocalTime(&stLocalTime);

  dumpFileName.Format("xbmc_crashlog-%s-%04d%02d%02d-%02d%02d%02d.dmp",
                      mVersion.c_str(),
                      stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
                      stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

  dumpFileName.Format("%s\\%s", CWIN32Util::GetProfilePath().c_str(), CUtil::MakeLegalFileName(dumpFileName));

  HANDLE hDumpFile = CreateFile(dumpFileName.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

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

access_violation::access_violation(EXCEPTION_POINTERS* info) : 
  win32_exception(info,"access_voilation"), mAccessType(Invalid), mBadAddress(0)
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

  write_minidump();
}
