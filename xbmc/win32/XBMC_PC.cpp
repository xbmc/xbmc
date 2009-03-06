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

//-----------------------------------------------------------------------------
// File: XBMC_PC.cpp
//
// Desc: This is the first tutorial for using Direct3D. In this tutorial, all
//       we are doing is creating a Direct3D device and using it to clear the
//       window.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "XBMC_PC.h"
#ifndef HAS_SDL
#include <d3d8.h>
#endif
#include "../../xbmc/Application.h"
#include "WIN32Util.h"
#include "shellapi.h"

//-----------------------------------------------------------------------------
// Resource defines
//-----------------------------------------------------------------------------

#include "resource.h"
#define IDI_MAIN_ICON          101 // Application icon
#define IDR_MAIN_ACCEL         113 // Keyboard accelerator

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

CApplication g_application;
CXBMC_PC *g_xbmcPC;

CXBMC_PC::CXBMC_PC()
{
  g_advancedSettings.m_startFullScreen = false;
}

CXBMC_PC::~CXBMC_PC()
{
  // todo: deinitialization code
}

HRESULT CXBMC_PC::Create( HINSTANCE hInstance, LPSTR commandLine )
{
  m_hInstance = hInstance;
  HRESULT hr = S_OK;

  CStdStringW strcl(commandLine);
  LPWSTR *szArglist;
  int nArgs;

  g_application.EnablePlatformDirectories(true);

  szArglist = CommandLineToArgvW(strcl.c_str(), &nArgs);
  if(szArglist != NULL)
  {
    for(int i=0;i<nArgs;i++)
    {
      CStdStringW strArgW(szArglist[i]);
      if(strArgW.Equals(L"-fs"))
        g_advancedSettings.m_startFullScreen = true;
      else if(strArgW.Equals(L"-p"))
        g_application.EnablePlatformDirectories(false);
      else if(strArgW.Equals(L"-d"))
      {
        int iSleep = _wtoi(szArglist[++i]);
        if(iSleep > 0 && iSleep < 360)
          Sleep(iSleep*1000);
      }
    }
    LocalFree(szArglist);
  }

  return S_OK;
}


INT CXBMC_PC::Run()
{
  g_application.Create(NULL);
  g_application.Run();
  return 0;
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR commandLine, INT )
{
  // check if XBMC is already running
  HWND m_hwnd = FindWindow("SDL_app","XBMC Media Center");
  if(m_hwnd != NULL)
  {
    // switch to the running instance
    ShowWindow(m_hwnd,SW_RESTORE);
    SetForegroundWindow(m_hwnd);
    return 0;
  }

  CXBMC_PC myApp;

  g_xbmcPC = &myApp;

  if(CWIN32Util::GetDesktopColorDepth() < 32)
  {
    //FIXME: replace it by a SDL window for all ports
    MessageBox(NULL, "Desktop Color Depth isn't 32Bit", "XBMC: Fatal Error", MB_OK|MB_ICONERROR);
    return 0;
  }

  // we don't want to see the "no disc in drive" windows message box
  SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  // update the current drive mask
  CWIN32Util::UpdateDriveMask();

  if (FAILED(myApp.Create(hInst, commandLine)))
    return 1;

  return myApp.Run();
}
