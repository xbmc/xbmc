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


#include "WinSystemWin32DX.h"

#ifdef HAS_DX

CWinSystemWin32DX g_Windowing;

CWinSystemWin32DX::CWinSystemWin32DX()
: CRenderSystemDX()
{

}

CWinSystemWin32DX::~CWinSystemWin32DX()
{

}

bool CWinSystemWin32DX::CreateNewWindow(CStdString name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  CWinSystemWin32::CreateNewWindow(name, fullScreen, res, userFunction);

  if(m_hWnd == NULL)
    return false;

  SetFocusWnd(m_hWnd);
  SetDeviceWnd(m_hWnd);

  SetBackBufferSize(m_nWidth, m_nHeight);
  SetDeviceFullScreen(fullScreen);

  return true;
}

bool CWinSystemWin32DX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin32::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemDX::ResetRenderSystem(newWidth, newHeight);  

  return true;
}

bool CWinSystemWin32DX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemWin32::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemDX::ResetRenderSystem(res.iWidth, res.iHeight);  

  return true;
}

#endif
