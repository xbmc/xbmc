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
#include "WinEvents.h"

CWinSystem g_WinSystem;

CWinSystemWin32::CWinSystemWin32()
: CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_WIN32;
  m_Hwnd = NULL;
  m_hInstance = NULL;
  m_hIcon = NULL;
}

CWinSystemWin32::~CWinSystemWin32()
{
  Destroy();
};

bool CWinSystemWin32::Create(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
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
    0, 0, ( rc.right - rc.left ), ( rc.bottom - rc.top ), 0,
    NULL, m_hInstance, userFunction );
  if( hWnd == NULL )
  {
    return false;
  }

  m_Hwnd = hWnd;

  // Show the window
  ShowWindow( m_Hwnd, SW_SHOWDEFAULT );
  UpdateWindow( m_Hwnd );

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

