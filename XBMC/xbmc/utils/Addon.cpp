/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "Application.h"
#include "Addon.h"
#include "Settings.h"
#include "settings/AddonSettings.h"
#include "GUIWindowManager.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"
#include "Util.h"

using namespace std;
using namespace XFILE;

namespace ADDON
{

/**********************************************************
 * Callback for unknown Add-on types as fallback
 */
CAddonDummyCallback *AddonDummyCallback = new CAddonDummyCallback();


/**********************************************************
 * CAddonStatusHandler - AddOn Status Report Class
 *
 * Used to informate the user about occurred errors and
 * changes inside Add-on's, and ask him what to do.
 *
 */

CCriticalSection CAddonStatusHandler::m_critSection;

CAddonStatusHandler::CAddonStatusHandler(const CAddon* addon, ADDON_STATUS status, CStdString message, bool sameThread)
{
  if (addon == NULL)
    return;

  CLog::Log(LOGINFO, "Called Add-on status handler for '%u' of clientName:%s, clientGUID:%s (same Thread=%s)", status, addon->m_strName.c_str(), addon->m_guid.c_str(), sameThread ? "yes" : "no");

  m_addon   = addon;
  m_status  = status;
  m_message = message;

  if (sameThread)
  {
    Process();
    delete this;
  }
  else
  {
    CStdString ThreadName;
    ThreadName.Format("Addon Status: %s", m_addon->m_strName.c_str());

    Create(false, THREAD_MINSTACKSIZE);
    SetName(ThreadName.c_str());
    SetPriority(-15);
  }
  return;
}

CAddonStatusHandler::~CAddonStatusHandler()
{
  StopThread();
}

void CAddonStatusHandler::OnStartup()
{
}

void CAddonStatusHandler::OnExit()
{
  delete this;
}

void CAddonStatusHandler::Process()
{
  CSingleLock lock(m_critSection);

  /* AddOn want to connect to unknown host (for ones that use Network) */
  if (m_status == STATUS_INVALID_HOST)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23040);
    pDialog->SetLine(2, 23041);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }
  /* Invalid or unknown user */
  else if (m_status == STATUS_INVALID_USER)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23042);
    pDialogYesNo->SetLine(2, 23043);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!CAddonSettings::SettingsExist(m_addon->m_strPath))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->m_strName.c_str());
      return;
    }

    CURL cUrl(m_addon->m_strPath);

    // Load language strings temporarily
    CAddon::LoadAddonStrings(cUrl);

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) m_gWindowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23044]: %s %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());
    pDialog->SetHeading(heading);

    CAddonSettings settings;
    settings.Load(cUrl);
    pDialog->SetSettings(settings);

    pDialog->DoModal();

    settings = pDialog->GetSettings();
    settings.Save();

    // Unload temporary language strings
    CAddon::ClearAddonStrings();

    if (pDialog->IsConfirmed())
    {
      CAddon::GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
    }
  }
  /* Invalid or wrong password */
  else if (m_status == STATUS_WRONG_PASS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23045);
    pDialogYesNo->SetLine(2, 23043);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!CAddonSettings::SettingsExist(m_addon->m_strPath))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->m_strName.c_str());
      return;
    }

    CURL cUrl(m_addon->m_strPath);

    // Load language strings temporarily
    CAddon::LoadAddonStrings(cUrl);

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) m_gWindowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23046]: %s %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());
    pDialog->SetHeading(heading);

    CAddonSettings settings;
    settings.Load(cUrl);
    pDialog->SetSettings(settings);

    pDialog->DoModal();

    settings = pDialog->GetSettings();
    settings.Save();

    // Unload temporary language strings
    CAddon::ClearAddonStrings();

    if (pDialog->IsConfirmed())
    {
      CAddon::GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
    }
  }
  /* AddOn lost connection to his backend (for ones that use Network) */
  else if (m_status == STATUS_LOST_CONNECTION)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23047);
    pDialog->SetLine(2, 23048);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      CAddon::GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, false);
    }
  }
  /* Request to restart the AddOn and data structures need updated */
  else if (m_status == STATUS_NEED_RESTART)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23049);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    CAddon::GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
  }
  /* Request to restart XBMC (hope no AddOn need or do this) */
  else if (m_status == STATUS_NEED_EMER_RESTART)
  {
    /* okey we really don't need to restart, only deinit Add-on, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogYesNo *pDialog = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return ;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine( 0, 23050);
    pDialog->SetLine( 1, 23051);
    pDialog->SetLine( 2, 23052);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      g_application.getApplicationMessenger().RestartApp();
    }
  }
  /* Some required settings are missing */
  else if (m_status == STATUS_MISSING_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23053);
    pDialogYesNo->SetLine(2, 23043);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!CAddonSettings::SettingsExist(m_addon->m_strPath))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->m_strName.c_str());
      return;
    }

    CURL cUrl(m_addon->m_strPath);

    // Load language strings temporarily
    CAddon::LoadAddonStrings(cUrl);

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) m_gWindowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23053]: %s %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());
    pDialog->SetHeading(heading);

    CAddonSettings settings;
    settings.Load(cUrl);
    pDialog->SetSettings(settings);

    pDialog->DoModal();

    settings = pDialog->GetSettings();
    settings.Save();

    // Unload temporary language strings
    CAddon::ClearAddonStrings();

    if (pDialog->IsConfirmed())
    {
      CAddon::GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
    }
  }
  /* A setting value is invalid */
  else if (m_status == STATUS_BAD_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23054);
    pDialogYesNo->SetLine(2, 23043);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!CAddonSettings::SettingsExist(m_addon->m_strPath))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->m_strName.c_str());
      return;
    }

    CURL cUrl(m_addon->m_strPath);

    // Load language strings temporarily
    CAddon::LoadAddonStrings(cUrl);

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) m_gWindowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23054]: %s %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());
    pDialog->SetHeading(heading);

    CAddonSettings settings;
    settings.Load(cUrl);
    pDialog->SetSettings(settings);

    pDialog->DoModal();

    settings = pDialog->GetSettings();
    settings.Save();

    // Unload temporary language strings
    CAddon::ClearAddonStrings();

    if (pDialog->IsConfirmed())
    {
      CAddon::GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
    }
  }
  /* One or more AddOn file(s) missing (check log's for missing data) */
  else if (m_status == STATUS_MISSING_FILE)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23055);
    pDialog->SetLine(2, 23056);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }
  /* A unknown event is occurred */
  else if (m_status == STATUS_UNKNOWN)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23057);
    pDialog->SetLine(2, 23056);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }

  return;
}


/**********************************************************
 * CAddon - AddOn Info and Helper Class
 *
 */

IAddonCallback *CAddon::m_cbMultitye        = NULL;
IAddonCallback *CAddon::m_cbViz             = NULL;
IAddonCallback *CAddon::m_cbSkin            = NULL;
IAddonCallback *CAddon::m_cbPVR             = NULL;
IAddonCallback *CAddon::m_cbScript          = NULL;
IAddonCallback *CAddon::m_cbScraperPVR      = NULL;
IAddonCallback *CAddon::m_cbScraperVideo    = NULL;
IAddonCallback *CAddon::m_cbScraperMusic    = NULL;
IAddonCallback *CAddon::m_cbScraperProgram  = NULL;
IAddonCallback *CAddon::m_cbScreensaver     = NULL;
IAddonCallback *CAddon::m_cbPluginPVR       = NULL;
IAddonCallback *CAddon::m_cbPluginVideo     = NULL;
IAddonCallback *CAddon::m_cbPluginMusic     = NULL;
IAddonCallback *CAddon::m_cbPluginProgram   = NULL;
IAddonCallback *CAddon::m_cbPluginPictures  = NULL;
IAddonCallback *CAddon::m_cbDSPAudio        = NULL;

CAddon::CAddon()
{
  Reset();
}

void CAddon::Reset()
{
  m_guid        = "";
  m_guid_parent = "";
  m_addonType   = ADDON_UNKNOWN;
  m_strPath     = "";
  m_disabled    = false;
  m_stars       = -1;
  m_strVersion  = "";
  m_strName     = "";
  m_summary     = "";
  m_strDesc     = "";
  m_disclaimer  = "";
  m_strLibName  = "";
  m_childs      = 0;
}

bool CAddon::operator==(const CAddon &rhs) const
{
  return (m_guid == rhs.m_guid);
}

IAddonCallback* CAddon::GetCallbackForType(AddonType type)
{
  IAddonCallback *cb_tmp;

  switch (type)
  {
    case ADDON_MULTITYPE:
      cb_tmp = m_cbMultitye;
      break;
    case ADDON_VIZ:
      cb_tmp = m_cbViz;
      break;
    case ADDON_SKIN:
      cb_tmp = m_cbSkin;
      break;
    case ADDON_PVRDLL:
      cb_tmp = m_cbPVR;
      break;
    case ADDON_SCRIPT:
      cb_tmp = m_cbScript;
      break;
    case ADDON_SCRAPER_PVR:
      cb_tmp = m_cbScraperPVR;
      break;
    case ADDON_SCRAPER_VIDEO:
      cb_tmp = m_cbScraperVideo;
      break;
    case ADDON_SCRAPER_MUSIC:
      cb_tmp = m_cbScraperMusic;
      break;
    case ADDON_SCRAPER_PROGRAM:
      cb_tmp = m_cbScraperProgram;
      break;
    case ADDON_SCREENSAVER:
      cb_tmp = m_cbScreensaver;
      break;
    case ADDON_PLUGIN_PVR:
      cb_tmp = m_cbPluginPVR;
      break;
    case ADDON_PLUGIN_MUSIC:
      cb_tmp = m_cbPluginMusic;
      break;
    case ADDON_PLUGIN_VIDEO:
      cb_tmp = m_cbPluginVideo;
      break;
    case ADDON_PLUGIN_PROGRAM:
      cb_tmp = m_cbPluginProgram;
      break;
    case ADDON_PLUGIN_PICTURES:
      cb_tmp = m_cbPluginPictures;
      break;
    case ADDON_DSP_AUDIO:
      cb_tmp = m_cbDSPAudio;
      break;
    case ADDON_UNKNOWN:
    default:
      cb_tmp = NULL;
      break;
  }

  if (cb_tmp == NULL)
    cb_tmp = AddonDummyCallback;

  return cb_tmp;
}

bool CAddon::RegisterAddonCallback(AddonType type, IAddonCallback* cb)
{
  if (cb == NULL)
    return false;

  if (type == ADDON_MULTITYPE && m_cbMultitye == NULL)
    m_cbMultitye = cb;
  else if (type == ADDON_VIZ && m_cbViz == NULL)
    m_cbViz = cb;
  else if (type == ADDON_SKIN && m_cbSkin == NULL)
    m_cbSkin = cb;
  else if (type == ADDON_PVRDLL && m_cbPVR == NULL)
    m_cbPVR = cb;
  else if (type == ADDON_SCRIPT && m_cbScript == NULL)
    m_cbScript = cb;
  else if (type == ADDON_SCRAPER_PVR && m_cbScraperPVR == NULL)
    m_cbScraperPVR = cb;
  else if (type == ADDON_SCRAPER_VIDEO && m_cbScraperVideo == NULL)
    m_cbScraperVideo = cb;
  else if (type == ADDON_SCRAPER_MUSIC && m_cbScraperMusic == NULL)
    m_cbScraperMusic = cb;
  else if (type == ADDON_SCRAPER_PROGRAM && m_cbScraperProgram == NULL)
    m_cbScraperProgram = cb;
  else if (type == ADDON_SCREENSAVER && m_cbScreensaver == NULL)
    m_cbScreensaver = cb;
  else if (type == ADDON_PLUGIN_PVR && m_cbPluginPVR == NULL)
    m_cbPluginPVR = cb;
  else if (type == ADDON_PLUGIN_MUSIC && m_cbPluginMusic == NULL)
    m_cbPluginMusic = cb;
  else if (type == ADDON_PLUGIN_VIDEO && m_cbPluginVideo == NULL)
    m_cbPluginVideo = cb;
  else if (type == ADDON_PLUGIN_PROGRAM && m_cbPluginProgram == NULL)
    m_cbPluginProgram = cb;
  else if (type == ADDON_PLUGIN_PICTURES && m_cbPluginPictures == NULL)
    m_cbPluginPictures = cb;
  else if (type == ADDON_DSP_AUDIO && m_cbDSPAudio == NULL)
    m_cbDSPAudio = cb;
  else
    return false;

  return true;
}

void CAddon::UnregisterAddonCallback(AddonType type)
{
  switch (type)
  {
    case ADDON_MULTITYPE:
      m_cbMultitye = NULL;
      return;
    case ADDON_VIZ:
      m_cbViz = NULL;
      return;
    case ADDON_SKIN:
      m_cbSkin = NULL;
      return;
    case ADDON_PVRDLL:
      m_cbPVR = NULL;
      return;
    case ADDON_SCRIPT:
      m_cbScript = NULL;
      return;
    case ADDON_SCRAPER_PVR:
      m_cbScraperPVR = NULL;
      return;
    case ADDON_SCRAPER_VIDEO:
      m_cbScraperVideo = NULL;
      return;
    case ADDON_SCRAPER_MUSIC:
      m_cbScraperMusic = NULL;
      return;
    case ADDON_SCRAPER_PROGRAM:
      m_cbScraperProgram = NULL;
      return;
    case ADDON_SCREENSAVER:
      m_cbScreensaver = NULL;
      return;
    case ADDON_PLUGIN_PVR:
      m_cbPluginPVR = NULL;
      return;
    case ADDON_PLUGIN_MUSIC:
      m_cbPluginMusic = NULL;
      return;
    case ADDON_PLUGIN_VIDEO:
      m_cbPluginVideo = NULL;
      return;
    case ADDON_PLUGIN_PROGRAM:
      m_cbPluginProgram = NULL;
      return;
    case ADDON_PLUGIN_PICTURES:
      m_cbPluginPictures = NULL;
      return;
    case ADDON_DSP_AUDIO:
      m_cbDSPAudio = NULL;
      return;
    case ADDON_UNKNOWN:
    default:
      return;
  }
}

void CAddon::LoadAddonStrings(const CURL &url)
{
  // Path where the addon resides
  CStdString pathToAddon;
  if (url.GetProtocol() == "plugin")
  {
    pathToAddon = "special://home/addons/plugins/";
    CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
    CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);
  }
  else
    url.GetURL(pathToAddon);

  // Path where the language strings reside
  CStdString pathToLanguageFile = pathToAddon;
  CStdString pathToFallbackLanguageFile = pathToAddon;
  CUtil::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "resources", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);

  // Load language strings temporarily
  g_localizeStringsTemp.Load(pathToLanguageFile, pathToFallbackLanguageFile);
}

void CAddon::ClearAddonStrings()
{
  // Unload temporary language strings
  g_localizeStringsTemp.Clear();
}

bool CAddon::CreateChildAddon(const CAddon &parent, CAddon &child)
{
  if (parent.m_addonType != ADDON_PVRDLL)
  {
    CLog::Log(LOGERROR, "Can't create a child add-on for '%s' and type '%i', is not allowed for this type!", parent.m_strName.c_str(), parent.m_addonType);
    return false;
  }

  child = parent;
  child.m_guid_parent = parent.m_guid;
  child.m_guid = CUtil::CreateUUID();

  VECADDONS *addons = g_settings.GetAddonsFromType(parent.m_addonType);
  if (!addons) return false;

  for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
  {
    if ((*it).m_guid == parent.m_guid)
    {
      (*it).m_childs++;
      child.m_strName.Format("%s #%i", child.m_strName.c_str(), (*it).m_childs);
      break;
    }
  }

  return true;
}

} /* namespace ADDON */

