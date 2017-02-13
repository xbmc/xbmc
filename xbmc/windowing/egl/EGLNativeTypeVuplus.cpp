/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#include "EGLNativeTypeVuplus.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#include "linux/DllBCM.h"

#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gles_init.h"

using namespace std;

bool CEGLNativeTypeVuplus::CheckCompatibility()
{
  return true;
}

void CEGLNativeTypeVuplus::Initialize()
{
}

void CEGLNativeTypeVuplus::Destroy()
{
}

bool CEGLNativeTypeVuplus::CreateNativeDisplay()
{
  GLES_Native_Init();
  return GLES_Native_CreateNativeDisplay(&m_nativeDisplay);
}

bool CEGLNativeTypeVuplus::CreateNativeWindow()
{
  m_nativeWindow = GLES_Native_CreateNativeWindow();
  if (!m_nativeWindow) {
	  return false;
  }
  return true;
}

bool CEGLNativeTypeVuplus::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeVuplus::GetNativeWindow(XBNativeDisplayType **nativeWindow) const
{
  *nativeWindow = (XBNativeWindowType*) &m_nativeWindow;
  return true;
}  

bool CEGLNativeTypeVuplus::DestroyNativeDisplay()
{
  GLES_Native_DestroyNativeDisplay();
  return true;
}

bool CEGLNativeTypeVuplus::DestroyNativeWindow()
{
  GLES_Native_DestroyNativeWindow();
  return true;
}

bool CEGLNativeTypeVuplus::GetNativeResolution(RESOLUTION_INFO *res) const
{
  *res = m_desktopRes;
  return true;
}

bool CEGLNativeTypeVuplus::SetNativeResolution(const RESOLUTION_INFO &res)
{
  m_desktopRes = res;
  return true;
}

bool CEGLNativeTypeVuplus::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  resolutions.clear();

  m_desktopResAll[0].iScreen      = 0;
  m_desktopResAll[0].bFullScreen  = true;
  m_desktopResAll[0].iWidth       = 1280;
  m_desktopResAll[0].iHeight      = 720;
  m_desktopResAll[0].iScreenWidth = 1280;
  m_desktopResAll[0].iScreenHeight= 720;
  m_desktopResAll[0].dwFlags      =  D3DPRESENTFLAG_PROGRESSIVE;
  m_desktopResAll[0].fRefreshRate = 50;
  m_desktopResAll[0].strMode = StringUtils::Format("%dx%d", 1280, 720);
  m_desktopResAll[0].strMode = StringUtils::Format("%s @ %.2f%s - Full Screen", m_desktopRes.strMode.c_str(), (float)50,m_desktopRes.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  m_desktopResAll[0].iSubtitles   = (int)(0.965 * m_desktopResAll[0].iHeight);

  CLog::Log(LOGDEBUG, "EGL initial desktop resolution %s\n", m_desktopResAll[0].strMode.c_str());

  resolutions.push_back(m_desktopResAll[0]);

  m_desktopResAll[1].iScreen      = 0;
  m_desktopResAll[1].bFullScreen  = true;
  m_desktopResAll[1].iWidth       = 1280;
  m_desktopResAll[1].iHeight      = 720;
  m_desktopResAll[1].iScreenWidth = 1280;
  m_desktopResAll[1].iScreenHeight= 720;
  m_desktopResAll[1].dwFlags      =  D3DPRESENTFLAG_PROGRESSIVE;

  m_desktopResAll[1].dwFlags      |=  D3DPRESENTFLAG_MODE3DSBS;
  m_desktopResAll[1].fRefreshRate = 50;
  m_desktopResAll[1].strMode = StringUtils::Format("%dx%d", 1280, 720);
  m_desktopResAll[1].strMode = StringUtils::Format("%s @ %.2f%s - Full Screen 3DSBS", m_desktopResAll[1].strMode.c_str(), (float)50,m_desktopResAll[1].dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  m_desktopResAll[1].iSubtitles   = (int)(0.965 * m_desktopResAll[1].iHeight);

  CLog::Log(LOGDEBUG, "EGL initial desktop resolution %s\n", m_desktopResAll[1].strMode.c_str());

  resolutions.push_back(m_desktopResAll[1]);

  m_desktopResAll[2].iScreen      = 0;
  m_desktopResAll[2].bFullScreen  = true;
  m_desktopResAll[2].iWidth       = 1280;
  m_desktopResAll[2].iHeight      = 720;
  m_desktopResAll[2].iScreenWidth = 1280;
  m_desktopResAll[2].iScreenHeight= 720;
  m_desktopResAll[2].dwFlags      =  D3DPRESENTFLAG_PROGRESSIVE;

  m_desktopResAll[2].dwFlags      |=  D3DPRESENTFLAG_MODE3DTB;
  m_desktopResAll[2].fRefreshRate = 50;
  m_desktopResAll[2].strMode = StringUtils::Format("%dx%d", 1280, 720);
  m_desktopResAll[2].strMode = StringUtils::Format("%s @ %.2f%s - Full Screen 3DTB", m_desktopResAll[2].strMode.c_str(), (float)50,m_desktopResAll[2].dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  m_desktopResAll[2].iSubtitles   = (int)(0.965 * m_desktopResAll[2].iHeight);

  CLog::Log(LOGDEBUG, "EGL initial desktop resolution %s\n", m_desktopResAll[2].strMode.c_str());

  resolutions.push_back(m_desktopResAll[2]);

  m_desktopRes = m_desktopResAll[0];

  return true;
}

bool CEGLNativeTypeVuplus::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  *res = m_desktopResAll[0];
  return true;
}

bool CEGLNativeTypeVuplus::ShowWindow(bool show)
{
  return false;
}


