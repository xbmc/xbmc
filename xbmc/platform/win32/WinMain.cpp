/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParser.h"
#include "AppParams.h"
#include "CompileInfo.h"
#include "ServiceBroker.h"
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

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR commandLine, INT)
{
  // parse command line parameters
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

  const auto params = appParamParser.GetAppParams();

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

  // check if Kodi is already running
  using KODI::PLATFORM::WINDOWS::ToW;
  std::string appName = CCompileInfo::GetAppName();
  HANDLE appRunningMutex = CreateMutex(nullptr, FALSE, ToW(appName + " Media Center").c_str());
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    auto appNameW = ToW(appName);
    HWND hwnd = FindWindow(appNameW.c_str(), appNameW.c_str());
    if (hwnd != nullptr)
    {
      // switch to the running instance
      ShowWindow(hwnd, SW_RESTORE);
      SetForegroundWindow(hwnd);
    }
    ReleaseMutex(appRunningMutex);
    return 0;
  }

  //Initialize COM
  CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  // Initialise Winsock
  WSADATA wd;
  WSAStartup(MAKEWORD(2, 2), &wd);

  // use 1 ms timer precision - like SDL initialization used to do
  timeBeginPeriod(1);

#ifndef _DEBUG
  // we don't want to see the "no disc in drive" windows message box
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#endif

  // Create and run the app
  int status = XBMC_Run(true, params);

  for (int i = 0; i < argc; ++i)
    delete[] argv[i];
  delete[] argv;

  // clear previously set timer resolution
  timeEndPeriod(1);

  WSACleanup();
  CoUninitialize();
  ReleaseMutex(appRunningMutex);

  return status;
}
