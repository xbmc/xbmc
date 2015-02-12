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

#ifndef WIN_SYSTEM_WIN32_DX_H
#define WIN_SYSTEM_WIN32_DX_H

#ifdef HAS_DX

#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <dxdiag.h>
#include "windowing/windows/WinSystemWin32.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/GlobalsHandling.h"


class CWinSystemWin32DX : public CWinSystemWin32, public CRenderSystemDX
{
public:
  CWinSystemWin32DX();
  ~CWinSystemWin32DX();

  virtual bool CreateNewWindow(std::string name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual void OnMove(int x, int y);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual bool WindowedMode() { return CRenderSystemDX::m_useWindowedDX; }

  std::string GetClipboardText(void);

protected:
  virtual void UpdateMonitor();
  bool UseWindowedDX(bool fullScreen);
};

XBMC_GLOBAL_REF(CWinSystemWin32DX,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemWin32DX)

#endif

#endif // WIN_SYSTEM_WIN32_DX_H
