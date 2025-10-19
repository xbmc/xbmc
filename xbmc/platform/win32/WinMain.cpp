/*
 *  Copyright (C) 2005-2025 Team Kodi
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
#include "utils/Digest.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "platform/win32/threads/Win32Exception.h"

#include <Objbase.h>
#include <WinSock2.h>
#include <dbghelp.h>
#include <mmsystem.h>
#include <shellapi.h>

using KODI::PLATFORM::WINDOWS::ToW;

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

constexpr wchar_t MUTEXBASENAME[] = L"Kodi Media Center";

/*!
 * \brief Detect another running instance, using a global mutex.
 * 
 * Attempt to create the mutex. If it already exists, that means another instance created it and is
 * running. This is the legacy pre-v22 exclusion mechanism with a constant mutex name that doesn't
 * depend on the portable/non-portable or data directory used by the instance.
 * 
 * \param[out] handle Windows handle for the mutex. Close the handle on program exit, not earlier.
 * \return true if the mutex already exists (ie another instance is running)
 */
bool CheckAndSetGlobalMutex(HANDLE& handle)
{
  handle = CreateMutexW(nullptr, FALSE, MUTEXBASENAME);
  if (handle != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}

/*!
 * \brief Detect another running instance, using a mutex with a name derived from the data directory.
 * 
 * Attempt to create the mutex. If it already exists, that means another instance created it and is
 * running.
 * 
 * \param[out] handle Windows handle for the mutex. Close the handle on program exit, not earlier.
 * \return true if the mutex already exists (ie another instance is running with the same data directory)
 */
bool CheckAndSetProfileMutex(bool usePlatformDirectories, HANDLE& handle)
{
  // Prepare a mutex name using a digest instead of the raw path to respect the mutex name
  // MAX_PATH max length
  std::wstring mutexName = MUTEXBASENAME;
  mutexName.push_back(L' ');

  const std::string path = CWIN32Util::GetProfilePath(usePlatformDirectories);

  try
  {
    using KODI::UTILITY::CDigest;
    CDigest digest{CDigest::Type::MD5};
    digest.Update(path);

    mutexName.append(ToW(digest.Finalize()));
  }
  catch (const std::exception& e)
  {
    LogError(L"Error creating the digest of the data directory %s, error %s\n", ToW(path).c_str(),
             ToW(e.what()).c_str());
  }

  handle = CreateMutexW(nullptr, FALSE, mutexName.c_str());
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
  // so use the legacy global mutex it supports and abort if it already exists.
  // There may be an older Kodi already running, or one may be started after us.
  //! @todo in a few releases: remove that restriction and assume all Kodi installs understand the
  //! file based protocol.
  //
  // * portable mode: most likely safe to allow multiple instances even if some are < v22
  // because the user has to make special efforts to run two different installs with the same
  // portable data dir.
  //
  // * Regardless of mode, v22 and higher implements a new mutex with a name derived from the data
  // directory. If the mutex already exists, another Kodi >= 22 instance is already running with
  // that data directory, abort execution.

  HANDLE appGlobalMutex{NULL};
  HANDLE appProfileMutex{NULL};

  // Profile-specific mutex then global mutex in non-portable mode for correct interaction with
  // Kodi < v22
  if (CheckAndSetProfileMutex(params->HasPlatformDirectories(), appProfileMutex) ||
      (params->HasPlatformDirectories() && CheckAndSetGlobalMutex(appGlobalMutex)))
  {
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
  if (appGlobalMutex)
    CloseHandle(appGlobalMutex);
  if (appProfileMutex)
    CloseHandle(appProfileMutex);

  return status;
}
