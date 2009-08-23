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

#include "stdafx.h"
#include "WinSystemWin32.h"
#include "WinEventsWin32.h"

#ifdef _WIN32

CWinSystemWin32::CWinSystemWin32()
: CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_WIN32;
  m_hWnd = NULL;
  m_hInstance = NULL;
  m_hIcon = NULL;
  m_hDC = NULL;
}

CWinSystemWin32::~CWinSystemWin32()
{
  Destroy();
};

bool CWinSystemWin32::InitWindowSystem()
{
  if(!CWinSystemBase::InitWindowSystem())
    return false;

  return true;
}

bool CWinSystemWin32::DestroyWindowSystem()
{
  return true;
}

bool CWinSystemWin32::CreateNewWindow(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  m_hInstance = ( HINSTANCE )GetModuleHandle( NULL );

  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;
  
  // Register the windows class
  WNDCLASS wndClass;
  wndClass.style = CS_DBLCLKS;
  wndClass.lpfnWndProc = CWinEvents::WndProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.style = CS_OWNDC;
  wndClass.hInstance = m_hInstance;
  wndClass.hIcon = m_hIcon;
  wndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
  wndClass.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
  wndClass.lpszMenuName = NULL;
  wndClass.lpszClassName = name.c_str();

  // center the window on the screen
  int left = 0;
  int top = 0;
  if(!fullScreen && m_DesktopRes.iHeight != 0 && m_DesktopRes.iWidth != 0)
  {
    left = (m_DesktopRes.iWidth / 2) - (width / 2);
    top = (m_DesktopRes.iHeight / 2) - (height / 2);
  }

  if( !RegisterClass( &wndClass ) )
  {
    return false;
  }

  RECT rc;
  SetRect( &rc, 0, 0, m_nWidth, m_nHeight );
  AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );

  DWORD dwStyle = WS_VISIBLE | WS_CLIPCHILDREN;

  if(!m_bFullScreen)
    dwStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION |
    WS_SYSMENU | WS_MINIMIZEBOX;

  HWND hWnd = CreateWindow( name.c_str(), name.c_str(), dwStyle,
    left, top, ( rc.right - rc.left ), ( rc.bottom - rc.top ), 0,
    NULL, m_hInstance, userFunction );
  if( hWnd == NULL )
  {
    return false;
  }

  m_hWnd = hWnd;
  m_hDC = GetDC(m_hWnd);

  // Show the window
  ShowWindow( m_hWnd, SW_SHOWDEFAULT );
  UpdateWindow( m_hWnd );

  return true;
}

bool CWinSystemWin32::Destroy()
{
  return true;
}

bool CWinSystemWin32::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  return true;
}

bool CWinSystemWin32::SetFullScreen(bool fullScreen, int width, int height)
{
  return true;
}

bool CWinSystemWin32::Resize()
{
  return true;
}

void CWinSystemWin32::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  DWORD		iDevNum	= 0 ;
  DWORD		iModeNum = 0 ;
  DISPLAY_DEVICE	ddi ;
  DEVMODE		dmi ;

  ZeroMemory (&ddi, sizeof(ddi)) ;
  ddi.cb = sizeof(ddi) ;
  ZeroMemory (&dmi, sizeof(dmi)) ;
  dmi.dmSize = sizeof(dmi) ;

  while (EnumDisplayDevices (NULL, iDevNum++, &ddi, 0))
  {
    while (EnumDisplaySettings (ddi.DeviceName, iModeNum++, &dmi))
    {
      RESOLUTION_INFO newRes;
      ZeroMemory(&newRes, sizeof(RESOLUTION_INFO));

      if(dmi.dmDisplayFrequency == 59 || dmi.dmDisplayFrequency == 29 || dmi.dmDisplayFrequency == 23)
        newRes.fRefreshRate = (float)(dmi.dmDisplayFrequency + 1) / 1.001f;
      else
        newRes.fRefreshRate = (float)dmi.dmDisplayFrequency;
      
      newRes.iWidth = dmi.dmPelsWidth;
      newRes.iHeight = dmi.dmPelsHeight;
      newRes.fRefreshRate = (float)dmi.dmDisplayFrequency;

      AddNewResolution(newRes);
      
      ZeroMemory (&dmi, sizeof(dmi)) ;
      dmi.dmSize = sizeof(dmi) ;
    }
    ZeroMemory (&ddi, sizeof(ddi)) ;
    ddi.cb = sizeof(ddi) ;
    iModeNum = 0 ;
  }
}

void CWinSystemWin32::GetDesktopRes(RESOLUTION_INFO& desktopRes)
{
  // get current screen refresh rate
  DEVMODE mode;

  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);
  m_DesktopRes.iWidth = mode.dmPelsWidth;
  m_DesktopRes.iHeight = mode.dmPelsHeight;
  if(mode.dmDisplayFrequency == 59 || mode.dmDisplayFrequency == 29 || mode.dmDisplayFrequency == 23)
    m_DesktopRes.fRefreshRate = (float)(mode.dmDisplayFrequency + 1) / 1.001f;
  else
    m_DesktopRes.fRefreshRate = (float)mode.dmDisplayFrequency;
}

#endif