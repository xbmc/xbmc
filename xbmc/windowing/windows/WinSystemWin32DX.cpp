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


#include "WinSystemWin32DX.h"
#include "settings/Settings.h"
#include "guilib/gui3d.h"
#include "utils/CharsetConverter.h"

#ifdef HAS_DX

CWinSystemWin32DX::CWinSystemWin32DX()
: CRenderSystemDX()
{

}

CWinSystemWin32DX::~CWinSystemWin32DX()
{

}

bool CWinSystemWin32DX::UseWindowedDX(bool fullScreen)
{
  return (CSettings::Get().GetBool("videoscreen.fakefullscreen") || !fullScreen);
}

bool CWinSystemWin32DX::CreateNewWindow(std::string name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  if(!CWinSystemWin32::CreateNewWindow(name, fullScreen, res, userFunction))
    return false;

  SetFocusWnd(m_hWnd);
  SetDeviceWnd(m_hWnd);
  CRenderSystemDX::m_interlaced = ((res.dwFlags & D3DPRESENTFLAG_INTERLACED) != 0);
  CRenderSystemDX::m_useWindowedDX = UseWindowedDX(fullScreen);
  SetRenderParams(m_nWidth, m_nHeight, fullScreen, res.fRefreshRate);
  SetMonitor(GetMonitor(res.iScreen).hMonitor);

  return true;
}

void CWinSystemWin32DX::UpdateMonitor()
{
  SetMonitor(GetMonitor(m_nScreen).hMonitor);
}

bool CWinSystemWin32DX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin32::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemDX::ResetRenderSystem(newWidth, newHeight, false, 0);

  return true;
}

void CWinSystemWin32DX::OnMove(int x, int y)
{
  CRenderSystemDX::OnMove();
}

bool CWinSystemWin32DX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  // When going DX fullscreen -> windowed, we must reset the D3D device first to
  // get it out of fullscreen mode because it restores a former resolution.
  // We then change to the mode we want.
  // In other cases, set the window/mode then reset the D3D device.

  bool FS2Windowed = !m_useWindowedDX && UseWindowedDX(fullScreen);

  SetMonitor(GetMonitor(res.iScreen).hMonitor);
  CRenderSystemDX::m_interlaced = ((res.dwFlags & D3DPRESENTFLAG_INTERLACED) != 0);
  CRenderSystemDX::m_useWindowedDX = UseWindowedDX(fullScreen);

  if (FS2Windowed)
    CRenderSystemDX::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);

  CWinSystemWin32::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemDX::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);

  return true;
}

std::string CWinSystemWin32DX::GetClipboardText(void)
{
  std::wstring unicode_text;
  std::string utf8_text;

  if (OpenClipboard(NULL))
  {
    HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
    if (hglb != NULL)
    {
      LPWSTR lpwstr = (LPWSTR) GlobalLock(hglb);
      if (lpwstr != NULL)
      {
        unicode_text = lpwstr;
        GlobalUnlock(hglb);
      }
    }
    CloseClipboard();
  }

  g_charsetConverter.wToUTF8(unicode_text, utf8_text);

  return utf8_text;
}

#endif
