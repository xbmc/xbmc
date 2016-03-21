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
  return (CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN) || !fullScreen);
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
  const MONITOR_DETAILS* monitor = GetMonitor(res.iScreen);
  if (!monitor)
    return false;

  SetMonitor(monitor->hMonitor);

  return true;
}

void CWinSystemWin32DX::UpdateMonitor()
{
  const MONITOR_DETAILS* monitor = GetMonitor(m_nScreen);
  if (monitor)
    SetMonitor(monitor->hMonitor);
}

bool CWinSystemWin32DX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin32::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemDX::OnResize(newWidth, newHeight);

  return true;
}

void CWinSystemWin32DX::OnMove(int x, int y)
{
  CRenderSystemDX::OnMove();
}

bool CWinSystemWin32DX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  // When going DX fullscreen -> windowed, we must switch DXGI device to windowed mode first to
  // get it out of fullscreen mode because it restores a former resolution.
  // We then change to the mode we want.
  // In other cases, set the window/mode then swith DXGI mode.
  bool FS2Windowed = !m_useWindowedDX && UseWindowedDX(fullScreen);

  const MONITOR_DETAILS* monitor = GetMonitor(res.iScreen);
  if (!monitor)
    return false;

  SetMonitor(monitor->hMonitor);
  CRenderSystemDX::m_interlaced = ((res.dwFlags & D3DPRESENTFLAG_INTERLACED) != 0);
  CRenderSystemDX::m_useWindowedDX = UseWindowedDX(fullScreen);

  // this needed to prevent resize/move events from DXGI during changing mode
  CWinSystemWin32::m_IsAlteringWindow = true;
  if (FS2Windowed)
    CRenderSystemDX::SetFullScreenInternal();

  if (!m_useWindowedDX)
  {
    // if the window isn't focused, bring it to front or SetFullScreen will fail
    BYTE keyState[256] = { 0 };
    // to unlock SetForegroundWindow we need to imitate Alt pressing
    if (GetKeyboardState((LPBYTE)&keyState) && !(keyState[VK_MENU] & 0x80))
      keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);

    BringWindowToTop(m_hWnd);

    if (GetKeyboardState((LPBYTE)&keyState) && !(keyState[VK_MENU] & 0x80))
      keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
  }

  // to disable stereo mode in windowed mode we must recreate swapchain and then change display mode
  // so this flags delays call SetFullScreen _after_ resetting render system
  bool delaySetFS = CRenderSystemDX::m_bHWStereoEnabled && UseWindowedDX(fullScreen);
  if (!delaySetFS)
    CWinSystemWin32::SetFullScreen(fullScreen, res, blankOtherDisplays);

  // this needed to prevent resize/move events from DXGI during changing mode
  CWinSystemWin32::m_IsAlteringWindow = true;
  CRenderSystemDX::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);
  CWinSystemWin32::m_IsAlteringWindow = false;

  if (delaySetFS)
    // now resize window and force changing resolution if stereo mode disabled
    CWinSystemWin32::SetFullScreenEx(fullScreen, res, blankOtherDisplays, !CRenderSystemDX::m_bHWStereoEnabled);

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

void CWinSystemWin32DX::NotifyAppFocusChange(bool bGaining)
{
  CWinSystemWin32::NotifyAppFocusChange(bGaining);

  // if true fullscreen we need switch render system to/from ff manually like dx9 does
  if (!UseWindowedDX(m_bFullScreen) && CRenderSystemDX::m_bRenderCreated)
  {
    CRenderSystemDX::m_useWindowedDX = !bGaining;
    CRenderSystemDX::SetFullScreenInternal();
    if (bGaining)
      CRenderSystemDX::CreateWindowSizeDependentResources();

    // minimize window on lost focus
    if (!bGaining)
      ShowWindow(m_hWnd, SW_FORCEMINIMIZE);
  }
}

#endif
