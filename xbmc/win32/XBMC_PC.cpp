/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "WIN32Util.h"
#include "shellapi.h"
#include "dbghelp.h"
#include "XBDateTime.h"
#include "threads/Thread.h"
#include "Application.h"

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);


// Minidump creation function 
LONG WINAPI CreateMiniDump( EXCEPTION_POINTERS* pEp ) 
{
  // Create the dump file where the xbmc.exe resides
  CStdString errorMsg;
  CStdString dumpFile;
  CDateTime now(CDateTime::GetCurrentDateTime());
  dumpFile.Format("%s\\XBMC\\xbmc_crashlog-%04i%02i%02i-%02i%02i%02i.dmp", CWIN32Util::GetProfilePath().c_str(), now.GetYear(), now.GetMonth(), now.GetDay(), now.GetHour(), now.GetMinute(), now.GetSecond());
  HANDLE hFile = CreateFile(dumpFile.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ); 

  // Call MiniDumpWriteDump api with the dump file
  if ( hFile && ( hFile != INVALID_HANDLE_VALUE ) ) 
  {
    // Load the DBGHELP DLL
    HMODULE hDbgHelpDll = ::LoadLibrary("DBGHELP.DLL");       
    if (hDbgHelpDll)
    {
      MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDbgHelpDll, "MiniDumpWriteDump");
      if (pDump)
      {
        // Initialize minidump structure
        MINIDUMP_EXCEPTION_INFORMATION mdei; 
        mdei.ThreadId           = CThread::GetCurrentThreadId();
        mdei.ExceptionPointers  = pEp; 
        mdei.ClientPointers     = FALSE; 

        // Call the minidump api with normal dumping
        // We can get more detail information by using other minidump types but the dump file will be
        // extermely large.
        BOOL rv = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, 0, NULL); 
        if( !rv ) 
        {
          errorMsg.Format("MiniDumpWriteDump failed with error id %d", GetLastError());
          MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR); 
        } 
      }
      else
      {
        errorMsg.Format("MiniDumpWriteDump failed to load with error id %d", GetLastError());
        MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR);
      }
      
      // Close the DLL
      FreeLibrary(hDbgHelpDll);
    }
    else
    {
      errorMsg.Format("LoadLibrary 'DBGHELP.DLL' failed with error id %d", GetLastError());
      MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR);
    }

    // Close the file 
    CloseHandle( hFile ); 
  }
  else 
  {
    errorMsg.Format("CreateFile '%s' failed with error id %d", dumpFile.c_str(), GetLastError());
    MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR); 
  }

  return pEp->ExceptionRecord->ExceptionCode;;
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR commandLine, INT )
{
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

  // parse the command line
  CStdStringW strcl(commandLine);
  LPWSTR *szArglist;
  int nArgs;

  setlocale(LC_NUMERIC, "C");
  g_advancedSettings.Initialize();
  szArglist = CommandLineToArgvW(strcl.c_str(), &nArgs);
  if(szArglist != NULL)
  {
    for(int i=0;i<nArgs;i++)
    {
      CStdStringW strArgW(szArglist[i]);
      if(strArgW.Equals(L"-fs"))
        g_advancedSettings.SetStartFullScreen(true);
      else if(strArgW.Equals(L"-p") || strArgW.Equals(L"--portable"))
        g_application.EnablePlatformDirectories(false);
      else if(strArgW.Equals(L"-d"))
      {
        if(++i < nArgs)
        {
          int iSleep = _wtoi(szArglist[i]);
          if(iSleep > 0 && iSleep < 360)
            Sleep(iSleep*1000);
          else
            --i;
        }
      }
      else if(strArgW.Equals(L"--debug"))
      {
        g_advancedSettings.m_logLevel     = LOG_LEVEL_DEBUG;
        g_advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
        CLog::SetLogLevel(g_advancedSettings.m_logLevel);
      }
    }
    LocalFree(szArglist);
  }

  WSADATA wd;
  WSAStartup(MAKEWORD(2,2), &wd);

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

  g_application.Run();
  
  // put everything in CApplication::Cleanup() since this point is never reached

  return 0;
}
