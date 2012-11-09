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

#include "settings/AdvancedSettings.h"
#include "settings/AppParamParser.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "threads/platform/win/Win32Exception.h"
#include "shellapi.h"
#include "dbghelp.h"
#include "XBDateTime.h"
#include "threads/Thread.h"
#include "Application.h"
#include "XbmcContext.h"
#include "GUIInfoManager.h"

// Minidump creation function
LONG WINAPI CreateMiniDump( EXCEPTION_POINTERS* pEp )
{
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

    CStdString* strargvA = new CStdString[argc];
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
    delete [] strargvA;
  }

  // Initialise Winsock
  WSADATA wd;
  WSAStartup(MAKEWORD(2,2), &wd);

  // use 1 ms timer precision - like SDL initialization used to do
  timeBeginPeriod(1);

  // Create and run the app
  if(!g_application.Create())
  {
    CStdString errorMsg;
    errorMsg.Format("CApplication::Create() failed - check log file and that it is writable");
    MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR);
    return 0;
  }

#ifndef _DEBUG
  // we don't want to see the "no disc in drive" windows message box
  SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
#endif

  if (!g_application.CreateGUI())
  {
    CStdString errorMsg;
    errorMsg.Format("CApplication::CreateGUI() failed - Check log file for display errors");
    MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR);
    return 0;
  }

  if (!g_application.Initialize())
  {
    CStdString errorMsg;
    errorMsg.Format("CApplication::Initialize() failed - Check log file and that it is writable");
    MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR);
    return 0;
  }

  g_application.Run();

  // clear previously set timer resolution
  timeEndPeriod(1);		

  // the end
  WSACleanup();
  CoUninitialize();

  return 0;
}
