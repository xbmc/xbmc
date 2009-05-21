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
// Screensaver.cpp: implementation of the CScreenSaver class.
//
//////////////////////////////////////////////////////////////////////

#include "ScreenSaver.h"
#include "URL.h"
#include "Settings.h"
#include "../utils/AddonHelpers.h"

using namespace ADDON;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenSaver::CScreenSaver(struct ScreenSaver* pScr, DllScreensaver* pDll, const CAddon& addon)
    : m_pScr(pScr)
    , m_pDll(pDll)
    , CAddon(addon)
    , m_ReadyToUse(false)
    , m_callbacks(NULL)
{}

CScreenSaver::~CScreenSaver()
{
  Destroy();
}

void CScreenSaver::Create()
{
  CLog::Log(LOGDEBUG, "Screensaver: %s - Creating Screensaver AddOn", m_strName.c_str());

  /* Allocate the callback table to save all the pointers
     to the helper callback functions */
  m_callbacks = new AddonCB;

  /* PVR Helper functions */
  m_callbacks->userData                 = this;
  m_callbacks->addonData                = (CAddon*) this;

  /* Write XBMC Global Add-on function addresses to callback table */
  CAddonUtils::CreateAddOnCallbacks(m_callbacks);

  // pass it the screen width,height
  // and the name of the screensaver
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight = g_graphicsContext.GetHeight();

  char szTmp[129];
  sprintf(szTmp, "scr create:%ix%i %s\n", iWidth, iHeight, m_strName.c_str());
  OutputDebugString(szTmp);

  float pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  try
  {
#ifndef HAS_SDL
    ADDON_STATUS status = m_pScr->Create(m_callbacks, g_graphicsContext.Get3DDevice(), iWidth, iHeight, m_strName.c_str(), pixelRatio);
#else
    ADDON_STATUS status = m_pScr->Create(m_callbacks, 0, iWidth, iHeight, m_strName.c_str(), pixelRatio);
#endif
    if (status != STATUS_OK)
      throw status;
    m_ReadyToUse = true;
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "Screensaver: %s - exception '%s' during Create occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), e.what(), m_strCreator.c_str());
    m_ReadyToUse = false;
  }
  catch (ADDON_STATUS status)
  {
    CLog::Log(LOGERROR, "Screensaver: %s - Client returns bad status (%i) after Create and is not usable", m_strName.c_str(), status);
    m_ReadyToUse = false;

    /* Delete is performed by the calling class */
    new CAddonStatusHandler(this, status, "", false);
  }
}

void CScreenSaver::Destroy()
{
  /* tell the AddOn to disconnect and prepare for destruction */
  try
  {
    CLog::Log(LOGDEBUG, "Screensaver: %s - Destroying Screensaver AddOn", m_strName.c_str());
    Stop();
    m_ReadyToUse = false;

    /* Release Callback table in memory */
    delete m_callbacks;
    m_callbacks = NULL;
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "Screensaver: %s - exception '%s' during destruction of AddOn occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), e.what(), m_strCreator.c_str());
  }
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  if (m_ReadyToUse) m_pScr->Start();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  if (m_ReadyToUse) m_pScr->Render();
}

void CScreenSaver::Stop()
{
  // ask screensaver to cleanup
  if (m_ReadyToUse) m_pScr->Stop();
}


void CScreenSaver::GetInfo(SCR_INFO *info)
{
  // get info from screensaver
  if (m_ReadyToUse) m_pScr->GetInfo(info);
}
