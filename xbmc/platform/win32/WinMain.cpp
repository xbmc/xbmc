/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "application/AppEnvironment.h"
#include "application/AppParamParser.h"
#include "application/AppParams.h"
#include "platform/Environment.h"
#include "platform/xbmc.h"
#include "threads/Thread.h"
#include "utils/CharsetConverter.h" // Required to initialize converters before usage
#include "utils/URIUtils.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "platform/win32/threads/Win32Exception.h"

#include <Objbase.h>
#include <WinSock2.h>
#include <dbghelp.h>
#include <mmsystem.h>
#include <shellapi.h>

namespace
{
// Minidump creation function
LONG WINAPI CreateMiniDump(EXCEPTION_POINTERS* pEp)
{
  win32_exception::write_stacktrace(pEp);
  win32_exception::write_minidump(pEp);
  return pEp->ExceptionRecord->ExceptionCode;
}

/*!
 * \brief Basic error reporting before the log subsystem is initialized
 *
 * The message is formatted using printf and output to debugger and cmd.exe, as applicable.
 * 
 * \param[in] format printf-style format string
 * \param[in] ... optional parameters for the format string.
 */
template<typename... Args>
void LogError(const wchar_t* format, Args&&... args)
{
  const int count = _snwprintf(nullptr, 0, format, args...);
  // terminating null character not included in count
  auto buf = std::make_unique<wchar_t[]>(count + 1);
  swprintf(buf.get(), format, args...);

  OutputDebugString(buf.get());

  static bool isConsoleAttached{false};

  if (!isConsoleAttached && AttachConsole(ATTACH_PARENT_PROCESS))
  {
    (void)freopen("CONOUT$", "w", stdout);
    wprintf(L"\n");
    isConsoleAttached = true;
  }
  wprintf(buf.get());
}

std::shared_ptr<CAppParams> ParseCommandLine()
{
  int argc = 0;
  LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
  char** argv = new char*[argc];

  for (int i = 0; i < argc; ++i)
  {
    int size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
    if (size > 0)
    {
      argv[i] = new char[size];
      WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], size, nullptr, nullptr);
    }
  }

  CAppParamParser appParamParser;
  appParamParser.Parse(argv, argc);

  for (int i = 0; i < argc; ++i)
    delete[] argv[i];
  delete[] argv;

  return appParamParser.GetAppParams();
}

/*!
 * \brief Detect another running instance using the same data directory, using a flag file exclusion
 * mechanism.
 * 
 * Attempt to create the file. If it already exists, another instance using the same data
 * directory is running. New in v22.
 * 
 * \param[in] usePlatformDirectories false for portable mode, true for non-portable mode
 * \param[out] handle Windows handle for the flag file.  Close the handle on program exit, not earlier.
 * \return true if the flag file already exists (ie another instance is running)
 */
bool CheckAndSetFileFlag(bool usePlatformDirectories, HANDLE& handle)
{
  const std::string file =
      URIUtils::AddFileToFolder(CWIN32Util::GetProfilePath(usePlatformDirectories), ".running");

  constexpr DWORD NO_DESIRED_ACCESS = 0;
  constexpr DWORD NO_SHARING = 0;

  using KODI::PLATFORM::WINDOWS::ToW;
  handle = CreateFileW(ToW(file).c_str(), NO_DESIRED_ACCESS, NO_SHARING, nullptr, CREATE_NEW,
                       FILE_FLAG_DELETE_ON_CLOSE, NULL);

  if (handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS)
    return true;

  return false;
}

/*!
 * \brief Detect another running instance, using a global mutex.
 * 
 * Attempt to create the mutex. If it already exists, that means another instance created it and is
 * running. This is the legacy pre-v22 exclusion mechanism.
 * 
 * \param[out] handle Windows handle for the mutex. Close the handle on program exit, not earlier.
 * \return true if the mutex already exists (ie another instance is running)
 */
bool CheckAndSetMutex(HANDLE& handle)
{
  handle = CreateMutexW(nullptr, FALSE, L"Kodi Media Center");
  if (handle != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}
} // namespace

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
_Use_decl_annotations_ INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
  // parse command line parameters
  const auto params = ParseCommandLine();

  // this fixes crash if OPENSSL_CONF is set to existed openssl.cfg
  // need to set it as soon as possible
  CEnvironment::unsetenv("OPENSSL_CONF");

  // Initializes CreateMiniDump to handle exceptions.
  char ver[100];
  if (strlen(CCompileInfo::GetSuffix()) > 0)
    sprintf_s(ver, "%d.%d-%s Git:%s", CCompileInfo::GetMajor(),
    CCompileInfo::GetMinor(), CCompileInfo::GetSuffix(), CCompileInfo::GetSCMID());
  else
    sprintf_s(ver, "%d.%d Git:%s", CCompileInfo::GetMajor(),
    CCompileInfo::GetMinor(), CCompileInfo::GetSCMID());

  if (win32_exception::ShouldHook())
  {
    win32_exception::set_version(std::string(ver));
    win32_exception::set_platformDirectories(params->HasPlatformDirectories());
    SetUnhandledExceptionFilter(CreateMiniDump);
  }

  int status{0};
  HRESULT hrCOM{E_FAIL};
  int rcWinsock{WSANOTINITIALISED};
  WSADATA wd{};

  // Detection of running Kodi instances
  // (avoid conflicts and data corruption caused by two instances using the same data dir)
  //
  // * Non-portable mode. Interference with another Kodi on version < 22 may happen
  // so use the legacy mutex mechanism it understands and abort if it already exists.
  // There may be an older Kodi already running, or one may be started after us.
  //! @todo in a few releases: remove that restriction and assume all Kodi installs understand the
  //! file based protocol.
  //
  // * portable mode: most likely safe to allow multiple instances even if some are < v22
  // because the user has to make special efforts to run two different installs with the same
  // portable data dir.
  //
  // * Regardless of mode, v22 and higher implements a new file-based signal. Create a specific file
  // in the data directory when starting up. If the file already exists, another Kodi >= 22
  // instance is already running with that data directory, abort execution.

  HANDLE appRunningMutex{NULL};
  HANDLE appRunningFile{NULL};

  // File-based method then mutex in non-portable mode for correct interaction with Kodi < v22 versions
  if (CheckAndSetFileFlag(params->HasPlatformDirectories(), appRunningFile) ||
      (params->HasPlatformDirectories() && CheckAndSetMutex(appRunningMutex)))
  {
    using KODI::PLATFORM::WINDOWS::ToW;
    const auto appNameW = ToW(CCompileInfo::GetAppName());
    HWND hwnd = FindWindow(appNameW.c_str(), appNameW.c_str());
    if (hwnd != nullptr)
    {
      // switch to the running instance
      ShowWindow(hwnd, SW_RESTORE);
      SetForegroundWindow(hwnd);
    }
    status = 0;
    goto cleanup;
  }

  //Initialize COM
  if ((hrCOM = CoInitializeEx(nullptr, COINIT_MULTITHREADED)) != S_OK)
  {
    LogError(L"unable to initialize COM, error %ld\n", hrCOM);
    status = -2;
    goto cleanup;
  }

  // Initialise Winsock
  if ((rcWinsock = WSAStartup(MAKEWORD(2, 2), &wd)) != 0)
  {
    LogError(L"unable to initialize Windows Sockets, error %i\n", rcWinsock);
    status = -3;
    goto cleanup;
  }

  // use 1 ms timer precision - like SDL initialization used to do
  timeBeginPeriod(1);

#ifndef _DEBUG
  // we don't want to see the "no disc in drive" windows message box
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#endif

  CAppEnvironment::SetUp(params);

  // Create and run the app
  status = XBMC_Run(true);

  CAppEnvironment::TearDown();

  // clear previously set timer resolution
  timeEndPeriod(1);

cleanup:

  if (rcWinsock == 0)
    WSACleanup();
  if (hrCOM == S_OK)
    CoUninitialize();
  if (appRunningMutex)
    CloseHandle(appRunningMutex);
  if (appRunningFile)
    CloseHandle(appRunningFile);

  return status;
}
