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
#include "WinSystem.h"
#include "GraphicContext.h"
#include "Settings.h"

CWinSystemBase::CWinSystemBase()
{
  m_nWidth = 0;
  m_nHeight = 0;
  m_nTop = 0;
  m_nLeft = 0;
  m_bWindowCreated = false;
  m_bFullScreen = false;
  m_nScreen = 0;
  m_bBlankOtherDisplay = false;
}

CWinSystemBase::~CWinSystemBase()
{

}

bool CWinSystemBase::InitWindowSystem()
{
  UpdateResolutions();

  return true;
}

void CWinSystemBase::UpdateDesktopResolution(RESOLUTION_INFO& newRes, int screen, int width, int height, float refreshRate)
{
  newRes.Overscan.left = 0;
  newRes.Overscan.top = 0;
  newRes.Overscan.right = width;
  newRes.Overscan.bottom = height;
  newRes.iScreen = screen;
  newRes.bFullScreen = true;
  newRes.iSubtitles = (int)(0.965 * height);
  newRes.fRefreshRate = refreshRate;
  newRes.fPixelRatio = 1.0f;  
  newRes.iWidth = width;
  newRes.iHeight = height;
  
  bool stdRes = false;
  if (width == 1280 && height == 720)
  {
    newRes.strMode = "720";
    stdRes = true;
  }
  if (width == 1366 && height == 768)
  {
    newRes.strMode = "720";
    stdRes = true;
  }
  else if (width == 720 && height == 480)
  {
    newRes.strMode = "480";
    stdRes = true;
  }
  else if (width == 1920 && height == 1080)
  {
    newRes.strMode = "1080";
    stdRes = true;
  }

  if (stdRes)
  {
    if (refreshRate > 23 && refreshRate < 31)
    {
      newRes.strMode += "i";
    }
    else
    {
      newRes.strMode += "p";
    }
      
    if (refreshRate > 1)
    {
      newRes.strMode.Format("%s%2.f (Full Screen)",  newRes.strMode, refreshRate);
    }
  }  
  else
  {
    newRes.strMode.Format("%dx%d", width, height);
    if (refreshRate > 1)
    {
	    newRes.strMode.Format("%s @ %.2f - Full Screen", newRes.strMode, refreshRate);
    }
  }
  
  if (screen > 0)
  {
    newRes.strMode.Format("%s #%d", newRes.strMode, screen + 1);    
  }  
}

void CWinSystemBase::UpdateResolutions()
{
  // Set the resolution info for a window
  RESOLUTION_INFO& window = g_settings.m_ResInfo[RES_WINDOW];
  window.iSubtitles = (int)(0.965 * 480);
  window.iWidth = 720;
  window.iHeight = 480;
  window.iScreen = 0;
  window.fPixelRatio = 1.0f;
  window.strMode = "Windowed";
  window.fRefreshRate = 0.0f;
  window.Overscan.left = 0;
  window.Overscan.top = 0;
  window.Overscan.right = window.iWidth;
  window.Overscan.bottom = window.iHeight;  
}
