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

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/threads/Win32Exception.h"

#include <Objbase.h>
#include <WinSock2.h>
#include <dbghelp.h>
#include <mmsystem.h>
#include <shellapi.h>

// Minidump creation function
LONG WINAPI CreateMiniDump(EXCEPTION_POINTERS* pEp)
{
  win32_exception::write_stacktrace(pEp);
  win32_exception::write_minidump(pEp);
  return pEp->ExceptionRecord->ExceptionCode;
}

static bool isConsoleAttached{false};

/*!
 * \brief Basic error reporting before the log subsystem is initialized
 *
 * The message is formatted using printf and output to debugger and cmd.exe, as applicable.
 * 
 * \param[in] format printf-style format string
 * \param[in] ... optional parameters for the format string.
 */
template<typename... Args>
static void LogError(const wchar_t* format, Args&&... args)
{
  const int count = _snwprintf(nullptr, 0, format, args...);
  // terminating null character not included in count
  auto buf = std::make_unique<wchar_t[]>(count + 1);
  swprintf(buf.get(), format, args...);

  OutputDebugString(buf.get());

  if (!isConsoleAttached && AttachConsole(ATTACH_PARENT_PROCESS))
  {
    (void)freopen("CONOUT$", "w", stdout);
    wprintf(L"\n");
    isConsoleAttached = true;
  }
  wprintf(buf.get());
}

static std::shared_ptr<CAppParams> ParseCommandLine()
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

  // check if Kodi is already running
  using KODI::PLATFORM::WINDOWS::ToW;
  std::string appName = CCompileInfo::GetAppName();
  HANDLE appRunningMutex = CreateMutex(nullptr, FALSE, ToW(appName + " Media Center").c_str());
  if (appRunningMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS)
  {
    auto appNameW = ToW(appName);
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

  return status;
}
