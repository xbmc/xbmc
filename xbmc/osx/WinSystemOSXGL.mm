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

#ifdef __APPLE__

//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL
#include "../guilib/Texture.h"
#include "WinSystemOSXGL.h"
#include "RenderSystemGL.h"
#include "SystemGlobals.h"
#undef BOOL

CWinSystemOSXGL& g_Windowing = g_SystemGlobals.m_Windowing;

CWinSystemOSXGL::CWinSystemOSXGL()
{
}

CWinSystemOSXGL::~CWinSystemOSXGL()
{
}

bool CWinSystemOSXGL::PresentRenderImpl()
{
  return FlushBuffer();
}

void CWinSystemOSXGL::SetVSyncImpl(bool enable)
{
  EnableVSync(false);
  
  if (enable && m_iVSyncMode == 0)
  {
    EnableVSync(true);
    m_iVSyncMode = 10;
  }
}

bool CWinSystemOSXGL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemOSX::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight, false, 0);
  
  if (m_bVSync)
  {
    EnableVSync(m_bVSync);
  } 
  
  return true;
}

bool CWinSystemOSXGL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemOSX::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);
  
  if (m_bVSync)
  {
    EnableVSync(m_bVSync);
  } 
  
  return true;
}

#endif
