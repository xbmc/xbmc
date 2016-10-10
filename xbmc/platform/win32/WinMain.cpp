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

#include "CompileInfo.h"
#include "threads/Thread.h"
#include "threads/platform/win/Win32Exception.h"
#include "platform/xbmc.h"
#include "utils/CPUInfo.h"
#include "utils/Environment.h"
#include "utils/CharsetConverter.h" // Required to initialize converters before usage

#include <dbghelp.h>
#include <shellapi.h>


extern "C" int main(int argc, char* argv[]);

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
    SetUnhandledExceptionFilter(CreateMiniDump);
  }

  // check if Kodi is already running
  std::string appName = CCompileInfo::GetAppName();
  CreateMutex(nullptr, FALSE, (appName + " Media Center").c_str());
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    HWND hwnd = FindWindow(appName.c_str(), appName.c_str());
    if (hwnd != NULL)
    {
      // switch to the running instance
      ShowWindow(hwnd, SW_RESTORE);
      SetForegroundWindow(hwnd);
    }
    return 0;
  }

  if ((g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_SSE2) == 0)
  {
    MessageBox(NULL, "No SSE2 support detected", (appName + ": Fatal Error").c_str(), MB_OK | MB_ICONERROR);
    return 0;
  }

  //Initialize COM
  CoInitializeEx(nullptr, COINIT_MULTITHREADED);


  int argc;
  LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

  char** argv = new char*[argc];

  for (int i = 0; i < argc; ++i)
  {
    int size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
    if (size > 0)
    {
      argv[i] = new char[size];
      int result = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], size, nullptr, nullptr);
    }
  }

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
  int status = main(argc, argv);

  for (int i = 0; i < argc; ++i)
    delete[] argv[i];
  delete[] argv;

  // clear previously set timer resolution
  timeEndPeriod(1);

  WSACleanup();
  CoUninitialize();

  return status;
}
