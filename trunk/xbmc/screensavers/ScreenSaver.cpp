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
// Screensaver.cpp: implementation of the CScreenSaver class.
//
//////////////////////////////////////////////////////////////////////

#include "ScreenSaver.h" 
#include "Settings.h"
#include "WindowingFactory.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenSaver::CScreenSaver(struct ScreenSaver* pScr, DllScreensaver* pDll, const CStdString& strScreenSaverName)
    : m_pScr(pScr)
    , m_pDll(pDll)
    , m_strScreenSaverName(strScreenSaverName)
{}

CScreenSaver::~CScreenSaver()
{
}

void CScreenSaver::Create()
{
  // pass it the screen width,height
  // and the name of the screensaver
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight = g_graphicsContext.GetHeight();

  char szTmp[129];
  sprintf(szTmp, "scr create:%ix%i %s\n", iWidth, iHeight, m_strScreenSaverName.c_str());
  OutputDebugString(szTmp);

  float pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;
#ifdef HAS_DX
  m_pScr->Create(g_Windowing.Get3DDevice(), iWidth, iHeight, m_strScreenSaverName.c_str(), pixelRatio);
#else
  m_pScr->Create(0, iWidth, iHeight, m_strScreenSaverName.c_str(), pixelRatio);
#endif
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  m_pScr->Start();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  m_pScr->Render();
}

void CScreenSaver::Stop()
{
  // ask screensaver to cleanup
  m_pScr->Stop();
}


void CScreenSaver::GetInfo(SCR_INFO *info)
{
  // get info from screensaver
  m_pScr->GetInfo(info);
}
