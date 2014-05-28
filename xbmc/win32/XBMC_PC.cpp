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

#ifndef WIN32_APPPARAMPARSER_H_INCLUDED
#define WIN32_APPPARAMPARSER_H_INCLUDED
#include "AppParamParser.h"
#endif

#ifndef WIN32_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#define WIN32_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#include "settings/AdvancedSettings.h"
#endif

#ifndef WIN32_UTILS_CHARSETCONVERTER_H_INCLUDED
#define WIN32_UTILS_CHARSETCONVERTER_H_INCLUDED
#include "utils/CharsetConverter.h"
#endif

#ifndef WIN32_UTILS_LOG_H_INCLUDED
#define WIN32_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef WIN32_THREADS_PLATFORM_WIN_WIN32EXCEPTION_H_INCLUDED
#define WIN32_THREADS_PLATFORM_WIN_WIN32EXCEPTION_H_INCLUDED
#include "threads/platform/win/Win32Exception.h"
#endif

#ifndef WIN32_SHELLAPI_H_INCLUDED
#define WIN32_SHELLAPI_H_INCLUDED
#include "shellapi.h"
#endif

#ifndef WIN32_DBGHELP_H_INCLUDED
#define WIN32_DBGHELP_H_INCLUDED
#include "dbghelp.h"
#endif

#ifndef WIN32_XBDATETIME_H_INCLUDED
#define WIN32_XBDATETIME_H_INCLUDED
#include "XBDateTime.h"
#endif

#ifndef WIN32_THREADS_THREAD_H_INCLUDED
#define WIN32_THREADS_THREAD_H_INCLUDED
#include "threads/Thread.h"
#endif

#ifndef WIN32_APPLICATION_H_INCLUDED
#define WIN32_APPLICATION_H_INCLUDED
#include "Application.h"
#endif

#ifndef WIN32_XBMCCONTEXT_H_INCLUDED
#define WIN32_XBMCCONTEXT_H_INCLUDED
#include "XbmcContext.h"
#endif

#ifndef WIN32_GUIINFOMANAGER_H_INCLUDED
#define WIN32_GUIINFOMANAGER_H_INCLUDED
#include "GUIInfoManager.h"
#endif

#ifndef WIN32_UTILS_STRINGUTILS_H_INCLUDED
#define WIN32_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef WIN32_UTILS_CPUINFO_H_INCLUDED
#define WIN32_UTILS_CPUINFO_H_INCLUDED
#include "utils/CPUInfo.h"
#endif

#include <mmdeviceapi.h>
#ifndef WIN32_WIN32_IMMNOTIFICATIONCLIENT_H_INCLUDED
#define WIN32_WIN32_IMMNOTIFICATIONCLIENT_H_INCLUDED
#include "win32/IMMNotificationClient.h"
#endif


#ifndef _DEBUG
#define XBMC_TRACK_EXCEPTIONS
#endif

// Minidump creation function
LONG WINAPI CreateMiniDump( EXCEPTION_POINTERS* pEp )
{
  win32_exception::write_stacktrace(pEp);
  win32_exception::write_minidump(pEp);
  return pEp->ExceptionRecord->ExceptionCode;;
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR commandLine, INT )
{
  // set up some xbmc specific relationships
  XBMC::Context context;

  //this can't be set from CAdvancedSettings::Initialize() because it will overwrite
  //the loglevel set with the --debug flag
#ifdef _DEBUG
  g_advancedSettings.m_logLevel     = LOG_LEVEL_DEBUG;
  g_advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
#else
  g_advancedSettings.m_logLevel     = LOG_LEVEL_NORMAL;
  g_advancedSettings.m_logLevelHint = LOG_LEVEL_NORMAL;
#endif
  CLog::SetLogLevel(g_advancedSettings.m_logLevel);

  // Initializes CreateMiniDump to handle exceptions.
  win32_exception::set_version(g_infoManager.GetVersion());
  SetUnhandledExceptionFilter( CreateMiniDump );

  // check if XBMC is already running
  CreateMutex(NULL, FALSE, "XBMC Media Center");
  if(GetLastError() == ERROR_ALREADY_EXISTS)
  {
    HWND m_hwnd = FindWindow("XBMC","XBMC");
    if(m_hwnd != NULL)
    {
      // switch to the running instance
      ShowWindow(m_hwnd,SW_RESTORE);
      SetForegroundWindow(m_hwnd);
    }
    return 0;
  }

#ifndef HAS_DX
  if(CWIN32Util::GetDesktopColorDepth() < 32)
  {
    //FIXME: replace it by a SDL window for all ports
    MessageBox(NULL, "Desktop Color Depth isn't 32Bit", "XBMC: Fatal Error", MB_OK|MB_ICONERROR);
    return 0;
  }
#endif

  if((g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_SSE2) == 0)
  {
    MessageBox(NULL, "No SSE2 support detected", "XBMC: Fatal Error", MB_OK|MB_ICONERROR);
    return 0;
  }

  //Initialize COM
  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  // Handle numeric values using the default/POSIX standard
  setlocale(LC_NUMERIC, "C");

  // If the command line passed to WinMain, commandLine, is not "" we need
  // to process the command line arguments.
  // Note that commandLine does not include the program name and can be
  // equal to "" if no arguments were supplied. By contrast GetCommandLineW()
  // does include the program name and is never equal to "".
  g_advancedSettings.Initialize();
  if (strlen(commandLine) != 0)
  {
    int argc;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::vector<std::string> strargvA;
    strargvA.resize(argc);
    const char** argv = (const char**) LocalAlloc(LMEM_FIXED, argc*sizeof(char*));
    for (int i = 0; i < argc; i++)
    {
      g_charsetConverter.wToUTF8(argvW[i], strargvA[i]);
      argv[i] = strargvA[i].c_str();
    }

    // Parse the arguments
    CAppParamParser appParamParser;
    appParamParser.Parse(argv, argc);

    // Clean up the storage we've used
    LocalFree(argvW);
    LocalFree(argv);
  }

  // Initialise Winsock
  WSADATA wd;
  WSAStartup(MAKEWORD(2,2), &wd);

  // use 1 ms timer precision - like SDL initialization used to do
  timeBeginPeriod(1);

#ifdef XBMC_TRACK_EXCEPTIONS
  try
  {
#endif
    // Create and run the app
    if(!g_application.Create())
    {
      MessageBox(NULL, "ERROR: Unable to create application. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
      return 1;
    }
#ifdef XBMC_TRACK_EXCEPTIONS
  }
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("CApplication::Create()");
    MessageBox(NULL, "ERROR: Unable to create application. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
    return 1;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "exception in CApplication::Create()");
    MessageBox(NULL, "ERROR: Unable to create application. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
    return 1;
  }
#endif

#ifndef _DEBUG
  // we don't want to see the "no disc in drive" windows message box
  SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
#endif

#ifdef XBMC_TRACK_EXCEPTIONS
  try
  {
#endif
    if (!g_application.CreateGUI())
    {
      MessageBox(NULL, "ERROR: Unable to create GUI. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
      return 1;
    }
#ifdef XBMC_TRACK_EXCEPTIONS
  }
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("CApplication::CreateGUI()");
    MessageBox(NULL, "ERROR: Unable to create GUI. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
    return 1;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "exception in CApplication::CreateGUI()");
    MessageBox(NULL, "ERROR: Unable to create GUI. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
    return 1;
  }
#endif

#ifdef XBMC_TRACK_EXCEPTIONS
  try
  {
#endif
    if (!g_application.Initialize())
    {
      MessageBox(NULL, "ERROR: Unable to Initialize. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
      return 1;
    }
#ifdef XBMC_TRACK_EXCEPTIONS
  }
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("CApplication::Initialize()");
    MessageBox(NULL, "ERROR: Unable to Initialize. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
    return 1;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "exception in CApplication::Initialize()");
    MessageBox(NULL, "ERROR: Unable to Initialize. Exiting.", "XBMC: Error", MB_OK|MB_ICONERROR);
    return 1;
  }
#endif

  HRESULT hr = E_FAIL;
  IMMDeviceEnumerator *pEnumerator = NULL;
  CMMNotificationClient cMMNC;
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  if(SUCCEEDED(hr))
  {
    pEnumerator->RegisterEndpointNotificationCallback(&cMMNC);
    SAFE_RELEASE(pEnumerator);
  }

  g_application.Run();

  // clear previously set timer resolution
  timeEndPeriod(1);		

  // the end
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  if(SUCCEEDED(hr))
  {
    pEnumerator->UnregisterEndpointNotificationCallback(&cMMNC);
    SAFE_RELEASE(pEnumerator);
  }
  WSACleanup();
  CoUninitialize();

  return 0;
}
