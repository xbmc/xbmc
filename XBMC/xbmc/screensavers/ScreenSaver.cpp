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

using namespace ADDON;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScreenSaver::CScreenSaver(struct ScreenSaver* pScr, DllScreensaver* pDll, const CAddon& addon)
    : m_pScr(pScr)
    , m_pDll(pDll)
    , CAddon(addon)
    , m_ReadyToUse(false)
{}

CScreenSaver::~CScreenSaver()
{
}

void CScreenSaver::Create()
{
  CLog::Log(LOGDEBUG, "Screensaver: %s - Creating Screensaver AddOn", m_strName.c_str());

  /* Allocate the callback table to save all the pointers
     to the helper callback functions */
  m_callbacks = new ScreensaverCallbacks;

  /* Screensaver Helper functions */
  m_callbacks->userData                 = this;

  /* AddOn Helper functions */
  m_callbacks->AddOn.ReportStatus       = AddOnStatusCallback;
  m_callbacks->AddOn.Log                = AddOnLogCallback;
  m_callbacks->AddOn.GetSetting         = AddOnGetSetting;
  m_callbacks->AddOn.OpenSettings       = AddOnOpenSettings;
  m_callbacks->AddOn.OpenOwnSettings    = AddOnOpenOwnSettings;

  /* GUI Dialog Helper functions */
  m_callbacks->Dialog.OpenOK            = CAddon::OpenDialogOK;
  m_callbacks->Dialog.OpenYesNo         = CAddon::OpenDialogYesNo;
  m_callbacks->Dialog.OpenBrowse        = CAddon::OpenDialogBrowse;
  m_callbacks->Dialog.OpenNumeric       = CAddon::OpenDialogNumeric;
  m_callbacks->Dialog.OpenKeyboard      = CAddon::OpenDialogKeyboard;
  m_callbacks->Dialog.OpenSelect        = CAddon::OpenDialogSelect;
  m_callbacks->Dialog.ProgressCreate    = CAddon::ProgressDialogCreate;
  m_callbacks->Dialog.ProgressUpdate    = CAddon::ProgressDialogUpdate;
  m_callbacks->Dialog.ProgressIsCanceled= CAddon::ProgressDialogIsCanceled;
  m_callbacks->Dialog.ProgressClose     = CAddon::ProgressDialogClose;

  /* Utilities Helper functions */
  m_callbacks->Utils.Shutdown           = CAddon::Shutdown;
  m_callbacks->Utils.Restart            = CAddon::Restart;
  m_callbacks->Utils.Dashboard          = CAddon::Dashboard;
  m_callbacks->Utils.ExecuteScript      = CAddon::ExecuteScript;
  m_callbacks->Utils.ExecuteBuiltIn     = CAddon::ExecuteBuiltIn;
  m_callbacks->Utils.ExecuteHttpApi     = CAddon::ExecuteHttpApi;
  m_callbacks->Utils.UnknownToUTF8      = CAddon::UnknownToUTF8;
  m_callbacks->Utils.LocalizedString    = AddOnGetLocalizedString;
  m_callbacks->Utils.GetSkinDir         = CAddon::GetSkinDir;
  m_callbacks->Utils.GetLanguage        = CAddon::GetLanguage;
  m_callbacks->Utils.GetIPAddress       = CAddon::GetIPAddress;
  m_callbacks->Utils.GetDVDState        = CAddon::GetDVDState;
  m_callbacks->Utils.GetInfoLabel       = CAddon::GetInfoLabel;
  m_callbacks->Utils.GetInfoImage       = CAddon::GetInfoImage;
  m_callbacks->Utils.GetFreeMem         = CAddon::GetFreeMem;
  m_callbacks->Utils.GetCondVisibility  = CAddon::GetCondVisibility;
  m_callbacks->Utils.EnableNavSounds    = CAddon::EnableNavSounds;
  m_callbacks->Utils.PlaySFX            = CAddon::PlaySFX;
  m_callbacks->Utils.GetSupportedMedia  = CAddon::GetSupportedMedia;
  m_callbacks->Utils.GetGlobalIdleTime  = CAddon::GetGlobalIdleTime;
  m_callbacks->Utils.GetCacheThumbName  = CAddon::GetCacheThumbName;
  m_callbacks->Utils.MakeLegalFilename  = CAddon::MakeLegalFilename;
  m_callbacks->Utils.TranslatePath      = CAddon::TranslatePath;
  m_callbacks->Utils.GetRegion          = CAddon::GetRegion;
  m_callbacks->Utils.SkinHasImage       = CAddon::SkinHasImage;
  
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

/**********************************************************
 * Addon specific Callbacks
 * Is a must do, to all types of available addons handler
 */

void CScreenSaver::AddOnStatusCallback(void *userData, const ADDON_STATUS status, const char* msg)
{
  CScreenSaver* client = (CScreenSaver*) userData;
  if (!client)
    return;

  CLog::Log(LOGINFO, "Screensaver: %s: Reported bad status: %i", client->m_strName.c_str(), status);

  if (status != STATUS_OK)
  {
    CStdString message;
    if (msg != NULL)
      message = msg;

    /* Delete is performed by the calling class */
    new CAddonStatusHandler(client, status, message, false);
  }
}

void CScreenSaver::AddOnLogCallback(void *userData, const ADDON_LOG loglevel, const char *format, ... )
{
  CScreenSaver* client = (CScreenSaver*) userData;
  if (!client)
    return;
      
  try 
  {
    CStdString clientMsg, xbmcMsg;
    clientMsg.reserve(16384);

    va_list va;
    va_start(va, format);
    clientMsg.FormatV(format, va);
    va_end(va);

    /* insert internal identifiers for brevity */
    xbmcMsg.Format("Screensaver: %s: ", client->m_strName);
    xbmcMsg += clientMsg;

    int xbmclog;
    switch (loglevel)
    {
      case LOG_ERROR:
        xbmclog = LOGERROR;
        break;
      case LOG_INFO:
        xbmclog = LOGINFO;
        break;
      case LOG_DEBUG:
      default:
        xbmclog = LOGDEBUG;
        break;
    }

    /* finally write the logmessage */
    CLog::Log(xbmclog, xbmcMsg);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "Screensaver: %s - exception '%s' during AddOnLogCallback occurred, contact Developer '%s' of this AddOn", client->m_strName.c_str(), e.what(), client->m_strCreator.c_str());
    return;
  }
}

bool CScreenSaver::AddOnGetSetting(void *userData, const char *settingName, void *settingValue)
{
  CScreenSaver* client = (CScreenSaver*) userData;
  if (!client)
    return NULL;

  return CAddon::GetAddonSetting(client, settingName, settingValue);
}

void CScreenSaver::AddOnOpenSettings(const char *url, bool bReload)
{
  CURL cUrl(url);
  CAddon::OpenAddonSettings(cUrl, bReload);
}

void CScreenSaver::AddOnOpenOwnSettings(void *userData, bool bReload)
{
  CScreenSaver* client = (CScreenSaver*) userData;
  if (!client)
    return;

  CAddon::OpenAddonSettings(client, bReload);
}

const char* CScreenSaver::AddOnGetLocalizedString(void *userData, long dwCode)
{
  CScreenSaver* client = (CScreenSaver*) userData;
  if (!client)
    return "";

  return CAddon::GetLocalizedString(client, dwCode);
}
