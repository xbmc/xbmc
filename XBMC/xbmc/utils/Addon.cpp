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
#include "GUIDialogFileBrowser.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogSelect.h"
#include "GUIDialogProgress.h"
#include "GUIDialogKeyboard.h"
#include "GUIAudioManager.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "PVRManager.h"
#include "Util.h"
#include "URL.h"
#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/XBMChttp.h"
#endif
#include "Crc32.h"

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
 
CAddon::CAddon()
{
  m_guid = "";
  m_addonType = ADDON_UNKNOWN;
  m_strPath = "";
  m_disabled = false;
  m_stars = -1;
  m_strVersion = "";
  m_strName = "";
  m_summary = "";
  m_strDesc = "";
  m_disclaimer = "";
  m_strLibName = "";
}

bool CAddon::operator==(const CAddon &rhs) const
{
  return (m_guid == rhs.m_guid);
}

IAddonCallback* CAddon::GetCallbackForType(AddonType type)
{
  switch (type)
  {
    case ADDON_MULTITYPE:
      return AddonDummyCallback;
    case ADDON_VIZ:
      return AddonDummyCallback;
    case ADDON_SKIN:
      return AddonDummyCallback;
    case ADDON_PVRDLL:
      return CPVRManager::GetInstance();
    case ADDON_SCRIPT:
      return AddonDummyCallback;
    case ADDON_SCRAPER:
      return AddonDummyCallback;
    case ADDON_SCREENSAVER:
      return AddonDummyCallback;
    case ADDON_PLUGIN_PVR:
      return AddonDummyCallback;
    case ADDON_PLUGIN_MUSIC:
      return AddonDummyCallback;
    case ADDON_PLUGIN_VIDEO:
      return AddonDummyCallback;
    case ADDON_PLUGIN_PROGRAM:
      return AddonDummyCallback;
    case ADDON_PLUGIN_PICTURES:
      return AddonDummyCallback;
    case ADDON_DSP_AUDIO:
      return AddonDummyCallback;
    case ADDON_UNKNOWN:
    default:
      return AddonDummyCallback;
  }
}

void CAddon::LoadAddonStrings(const CURL &url)
{
  // Path where the addon resides
  CStdString pathToAddon;
  
  //TODO fix all Addon paths
  if (url.GetProtocol() == "plugin")
    pathToAddon = "special://home/plugins/";
  else
    pathToAddon = "special://xbmc/";

  // Build the addon's path
  CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
  CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);

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

void CAddon::OpenAddonSettings(const CURL &url, bool bReload)
{
  try 
  {
    // Path where the addon resides
    CStdString pathToAddon = "addon://";

    // Build the addon's path
    CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
    CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);

    CAddon addon;
    if (g_settings.AddonFromInfoXML(pathToAddon, addon))
    {
      addon.m_strPath = pathToAddon;
      OpenAddonSettings(&addon, bReload);
    }
    else
    {
      CLog::Log(LOGERROR, "Unknown URL %s to open AddOn Settings", pathToAddon.c_str());
    }
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CAddon: Exception '%s' during OpenAddonSettings occurred", e.what());
  }
}

void CAddon::OpenAddonSettings(const CAddon* addon, bool bReload)
{
  if (addon == NULL)
    return;

  try 
  {
    CLog::Log(LOGDEBUG, "Calling OpenAddonSettings for: %s", addon->m_strName.c_str());

    if (!CAddonSettings::SettingsExist(addon->m_strPath))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", addon->m_strName.c_str());
      return;
    }

    CURL cUrl(addon->m_strPath);
    CGUIDialogAddonSettings::ShowAndGetInput(cUrl);

    // reload plugin settings & strings
    if (bReload)
    {
      g_currentPluginSettings.Load(cUrl);
      CAddon::LoadAddonStrings(cUrl);
    }
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s - exception '%s' during OpenAddonSettings occurred, contact Developer '%s' of this AddOn", addon->m_strName.c_str(), e.what(), addon->m_strCreator.c_str());
  }
}

void CAddon::TransferAddonSettings(const CURL &url)
{
  // Path where the addon resides
  CStdString pathToAddon = "addon://";

  // Build the addon's path
  CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
  CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);

  CAddon addon;
  if (g_settings.AddonFromInfoXML(pathToAddon, addon))
  {
    addon.m_strPath = pathToAddon;
    TransferAddonSettings(&addon);
  }
  else
  {
    CLog::Log(LOGERROR, "Unknown URL %s to transfer AddOn Settings", pathToAddon.c_str());
  }
}

void CAddon::TransferAddonSettings(const CAddon* addon)
{
  bool restart = false;
  ADDON_STATUS reportStatus = STATUS_OK;

  if (addon == NULL)
    return;

  CLog::Log(LOGDEBUG, "Calling TransferAddonSettings for: %s", addon->m_strName.c_str());
          
  /* Transmit current unified user settings to the PVR Addon */
  ADDON::IAddonCallback* addonCB = GetCallbackForType(addon->m_addonType);

  CAddonSettings settings;
  if (!settings.Load(addon->m_strPath))
  {
    CLog::Log(LOGERROR, "Could't get Settings for AddOn: %s during transfer", addon->m_strName.c_str());
    return;
  }

  TiXmlElement *setting = settings.GetAddonRoot()->FirstChildElement("setting");
  while (setting)
  {
    ADDON_STATUS status;
    const char *id = setting->Attribute("id");
    const char *type = setting->Attribute("type");
        
    if (type)
    {
      if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
          strcmpi(type, "folder") == 0 || strcmpi(type, "action") == 0 ||
          strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
          strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
          strcmpi(type, "files") == 0 || strcmpi(type, "fileenum") == 0)
      {
        status = addonCB->SetSetting(addon, id, (const char*) settings.Get(id).c_str());
      }
      else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
               strcmpi(type, "labelenum") == 0)
      {
        int tmp = atoi(settings.Get(id));
        status = addonCB->SetSetting(addon, id, (int*) &tmp);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        bool tmp = settings.Get(id) == "true" ? true : false;
        status = addonCB->SetSetting(addon, id, (bool*) &tmp);
      }
      else
      {
        CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type, addon->m_strName.c_str());
      }
      
      if (status == STATUS_NEED_RESTART)
        restart = true; 
      else if (status != STATUS_OK)
        reportStatus = status;
    }
    setting = setting->NextSiblingElement("setting");
  }

  if (restart || reportStatus != STATUS_OK)
    new CAddonStatusHandler(addon, restart ? STATUS_NEED_RESTART : reportStatus, "", true);
}

bool CAddon::GetAddonSetting(const CAddon* addon, const char* settingName, void *settingValue)
{
  if (addon == NULL || settingName == NULL || settingValue == NULL)
    return false;

  try 
  {
    CLog::Log(LOGDEBUG, "CAddon: AddOn %s request Setting %s", addon->m_strName.c_str(), settingName);
    
    /* TODO: Add a caching mechanism to prevent a reloading of settings file on every call */
    CAddonSettings settings;
    if (!settings.Load(addon->m_strPath))
    {
      CLog::Log(LOGERROR, "Could't get Settings for AddOn: %s", addon->m_strName.c_str());
      return false;
    }

    TiXmlElement *setting = settings.GetAddonRoot()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      const char *type = setting->Attribute("type");
        
      if (strcmpi(id, settingName) == 0 && type)
      {
        if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
            strcmpi(type, "folder") == 0 || strcmpi(type, "action") == 0 ||
            strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
            strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
            strcmpi(type, "files") == 0 || strcmpi(type, "fileenum") == 0)
        {
          strcpy((char*) settingValue, settings.Get(id).c_str());
          return true;
        }
        else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
                 strcmpi(type, "labelenum") == 0)
        {
          *(int*) settingValue = (int) atoi(settings.Get(id));
          return true;
        }
        else if (strcmpi(type, "bool") == 0)
        {
          *(bool*) settingValue = (bool) (settings.Get(id) == "true" ? true : false);
          return true;
        }
        else
        {
          CLog::Log(LOGERROR, "Unknown setting type '%s' for id %s in %s", type, id, addon->m_strName.c_str());
        }
      }
      setting = setting->NextSiblingElement("setting");
    }
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s - exception '%s' during GetAddonSetting occurred, contact Developer '%s' of this AddOn", addon->m_strName.c_str(), e.what(), addon->m_strCreator.c_str());
  }
  return false;
}


/**
* XBMC AddOn Dialog callbacks
* Helper functions to access GUI Dialog functions
*/

bool CAddon::OpenDialogOK(const char* heading, const char* line1, const char* line2, const char* line3)
{
  const DWORD dWindow = WINDOW_DIALOG_OK;
  CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(dWindow);
  if (!pDialog) return false;

  if (heading != NULL ) pDialog->SetHeading(heading);
  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);

  //send message and wait for user input
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, m_gWindowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  return pDialog->IsConfirmed();  
}

bool CAddon::OpenDialogYesNo(const char* heading, const char* line1, const char* line2, 
                             const char* line3, const char* nolabel, const char* yeslabel)
{
  const DWORD dWindow = WINDOW_DIALOG_YES_NO;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(dWindow);
  if (!pDialog) return false;

  if (heading != NULL ) pDialog->SetHeading(heading);
  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);
  
  if (nolabel != NULL )
    pDialog->SetChoice(0,nolabel);

  if (yeslabel != NULL )
    pDialog->SetChoice(1,yeslabel);

  //send message and wait for user input
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, m_gWindowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  return pDialog->IsConfirmed();
}

const char* CAddon::OpenDialogBrowse(int type, const char* heading, const char* shares, const char* mask, bool useThumbs, bool treatAsFolder, const char* default_folder)
{
  CStdString value;
  CStdString type_mask = mask;

  if (treatAsFolder && !type_mask.size() == 0)
    type_mask += "|.rar|.zip";

  VECSOURCES *shares_type = g_settings.GetSourcesFromType(shares);
  if (!shares_type) return "";
    
  value = default_folder;
  if (type == 1)
    CGUIDialogFileBrowser::ShowAndGetFile(*shares_type, type_mask, heading, value, useThumbs, treatAsFolder);
  else if (type == 2)
    CGUIDialogFileBrowser::ShowAndGetImage(*shares_type, heading, value);
  else
    CGUIDialogFileBrowser::ShowAndGetDirectory(*shares_type, heading, value, type != 0);

  return value.c_str();
}

const char* CAddon::OpenDialogNumeric(int type, const char* heading, const char* default_value)
{
  CStdString value;
  SYSTEMTIME timedate;
  GetLocalTime(&timedate);

  if (heading)
  {
    if (type == 1)
    {
      if (default_value && strlen(default_value) == 10)
      {
        CStdString sDefault = default_value;
        timedate.wDay = atoi(sDefault.Left(2));
        timedate.wMonth = atoi(sDefault.Mid(3,4));
        timedate.wYear = atoi(sDefault.Right(4));
      }
      if (CGUIDialogNumeric::ShowAndGetDate(timedate, heading))
        value.Format("%2d/%2d/%4d", timedate.wDay, timedate.wMonth, timedate.wYear);
      else
        value = default_value;
    }
    else if (type == 2)
    {
      if (default_value && strlen(default_value) == 5)
      {
        CStdString sDefault = default_value;
        timedate.wHour = atoi(sDefault.Left(2));
        timedate.wMinute = atoi(sDefault.Right(2));
      }
      if (CGUIDialogNumeric::ShowAndGetTime(timedate, heading))
        value.Format("%2d:%02d", timedate.wHour, timedate.wMinute);
      else
        value = default_value;
    }
    else if (type == 3)
    {
      value = default_value;
      CGUIDialogNumeric::ShowAndGetIPAddress(value, heading);
    }
    else
    {
      value = default_value;
      CGUIDialogNumeric::ShowAndGetNumber(value, heading);
    }
  }
  return value.c_str();
}

const char* CAddon::OpenDialogKeyboard(const char* heading, const char* default_value, bool hidden)
{
  CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return NULL;

  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->CenterWindow();
  if (heading != NULL)
    pKeyboard->SetHeading(heading);
  else
    pKeyboard->SetHeading("");
  pKeyboard->SetHiddenInput(hidden);
  if (default_value != NULL)
    pKeyboard->SetText(default_value);
  else
    pKeyboard->SetText("");

  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, m_gWindowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    CStdString TextString = pKeyboard->GetText();
    if (TextString.IsEmpty())
      return NULL;

    return TextString.c_str();
  }

  return NULL;
}

int CAddon::OpenDialogSelect(const char* heading, AddOnStringList* list)
{
  const DWORD dWindow = WINDOW_DIALOG_SELECT;
  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)m_gWindowManager.GetWindow(dWindow);
  if (!pDialog) return NULL;

  pDialog->Reset();

  if (heading != NULL)
    pDialog->SetHeading(heading);

  const char *listLine = NULL;
  for(int i = 0; i < list->Items; i++)
  {
    listLine = list->Strings[i];
    if (listLine)
      pDialog->Add(listLine);
  }

  //send message and wait for user input
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, m_gWindowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  return pDialog->GetSelectedLabel();
}

bool CAddon::ProgressDialogCreate(const char* heading, const char* line1, const char* line2, const char* line3)
{
  CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return true;

  if (heading != NULL ) pDialog->SetHeading(heading);
  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);

  pDialog->StartModal();

  return false;
}

void CAddon::ProgressDialogUpdate(int percent, const char* line1, const char* line2, const char* line3)
{
  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return;

  if (percent >= 0 && percent <= 100)
  {
    pDialog->SetPercentage(percent);
    pDialog->ShowProgressBar(true);
  }
  else
  {
    pDialog->ShowProgressBar(false);
  }

  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);

  return;
}

bool CAddon::ProgressDialogIsCanceled()
{
  bool canceled = false;
  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return canceled;

  canceled = pDialog->IsCanceled();

  return canceled;
}

void CAddon::ProgressDialogClose()
{
  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return;

  pDialog->Close();

  return;
}
  

/**
 * XBMC AddOn GUI callbacks
 * Helper to access different types of GUI functions
 */

static int iAddOnGUILockRef = 0;

void CAddon::GUILock()
{
  if (iAddOnGUILockRef == 0) g_graphicsContext.Lock();
    iAddOnGUILockRef++;
}

void CAddon::GUIUnlock()
{
  if (iAddOnGUILockRef > 0)
  {
    iAddOnGUILockRef--;
    if (iAddOnGUILockRef == 0) g_graphicsContext.Unlock();
  }
}

int CAddon::GUIGetCurrentWindowId()
{
  GUILock();
  DWORD dwId = m_gWindowManager.GetActiveWindow();
  GUIUnlock();
  return dwId;
}

int CAddon::GUIGetCurrentWindowDialogId()
{
  GUILock();
  DWORD dwId = m_gWindowManager.GetTopMostModalDialogID();
  GUIUnlock();
  return dwId;
}


/**
* XBMC AddOn Utils callbacks
* Helper to access XBMC Utilities
*/

void CAddon::Shutdown()
{
  ThreadMessage tMsg = {TMSG_SHUTDOWN};
  g_application.getApplicationMessenger().SendMessage(tMsg);
  return;
}

void CAddon::Restart()
{
  ThreadMessage tMsg = {TMSG_RESTART};
  g_application.getApplicationMessenger().SendMessage(tMsg);
  return;
}

void CAddon::Dashboard()
{
  ThreadMessage tMsg = {TMSG_DASHBOARD};
  g_application.getApplicationMessenger().SendMessage(tMsg);
}

void CAddon::ExecuteScript(const char *script)
{
  if (script == NULL)
    return;

  ThreadMessage tMsg = {TMSG_EXECUTE_SCRIPT};
  tMsg.strParam = script;
  g_application.getApplicationMessenger().SendMessage(tMsg);
}

void CAddon::ExecuteBuiltIn(const char *function)
{
  if (function == NULL)
    return;

  g_application.getApplicationMessenger().ExecBuiltIn(function);
}

const char* CAddon::ExecuteHttpApi(char *httpcommand)
{
  if (httpcommand == NULL)
    return "";

#ifdef HAS_WEB_SERVER
  CStdString ret;

  if (!m_pXbmcHttp)
  {
    CSectionLoader::Load("LIBHTTP");
    m_pXbmcHttp = new CXbmcHttp();
  }
  if (!pXbmcHttpShim)
  {
    pXbmcHttpShim = new CXbmcHttpShim();
    if (!pXbmcHttpShim)
      return "";
  }
  ret = pXbmcHttpShim->xbmcExternalCall(httpcommand);

  return ret.c_str();
#else
  return "";
#endif
}

const char* CAddon::GetLocalizedString(const CAddon* addon, long dwCode)
{
  if (addon == NULL)
    return "";

  CURL cUrl(addon->m_strPath);

  // Load language strings temporarily
  CAddon::LoadAddonStrings(cUrl);

  if (dwCode >= 30000 && dwCode <= 30999)
    return g_localizeStringsTemp.Get(dwCode).c_str();
  else if (dwCode >= 32000 && dwCode <= 32999)
    return g_localizeStringsTemp.Get(dwCode).c_str();
  else
    return g_localizeStrings.Get(dwCode).c_str();

  // Unload temporary language strings
  CAddon::ClearAddonStrings();

  return "";
}

const char* CAddon::GetSkinDir()
{
  return g_guiSettings.GetString("lookandfeel.skin");
}

const char* CAddon::UnknownToUTF8(const char *sourceDest)
{
  if (sourceDest == NULL)
    return "";

  CStdString string = sourceDest;
  g_charsetConverter.unknownToUTF8(string);
  return string.c_str();
}

const char* CAddon::GetLanguage()
{
  return g_guiSettings.GetString("locale.language");
}

const char* CAddon::GetIPAddress()
{
  return g_infoManager.GetLabel(NETWORK_IP_ADDRESS).c_str();
}

int CAddon::GetDVDState()
{
  return CIoSupport::GetTrayState();
}

int CAddon::GetFreeMem()
{
  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  return stat.dwAvailPhys  / ( 1024 * 1024 );
}

const char* CAddon::GetInfoLabel(const char *infotag)
{
  if (infotag == NULL)
    return "";

  int ret = g_infoManager.TranslateString(infotag);
  return g_infoManager.GetLabel(ret).c_str();
}

const char* CAddon::GetInfoImage(const char *infotag)
{
  if (infotag == NULL)
    return "";

  int ret = g_infoManager.TranslateString(infotag);
  return g_infoManager.GetImage(ret, WINDOW_INVALID).c_str();
}

bool CAddon::GetCondVisibility(const char *condition)
{
  if (condition == NULL)
    return false;

  DWORD dwId = m_gWindowManager.GetTopMostModalDialogID();
  if (dwId == WINDOW_INVALID) dwId = m_gWindowManager.GetActiveWindow();

  int ret = g_infoManager.TranslateString(condition);
  return g_infoManager.GetBool(ret,dwId);
}

void CAddon::EnableNavSounds(bool yesNo)
{
  g_audioManager.Enable(yesNo);
}

void CAddon::PlaySFX(const char *filename)
{
  if (filename == NULL)
    return;

  if (CFile::Exists(filename))
  {
    g_audioManager.PlayPythonSound(filename);
  }
}

int CAddon::GetGlobalIdleTime()
{
  return g_application.GlobalIdleTime();
}

const char* CAddon::GetCacheThumbName(const char *path)
{
  if (path == NULL)
    return "";

  string strText = path;

  Crc32 crc;
  CStdString strPath;
  crc.ComputeFromLowerCase(strText);
  strPath.Format("%08x.tbn", (unsigned __int32)crc);
  return strPath.c_str();
}

const char* CAddon::MakeLegalFilename(const char *filename)
{
  if (filename == NULL)
    return "";

  CStdString strText = filename;
  CStdString strFilename = CUtil::MakeLegalPath(strText);
  return strFilename.c_str();
}

const char* CAddon::TranslatePath(const char *path)
{
  if (path == NULL)
    return "";

  CStdString strText = path;

  if (CUtil::IsDOSPath(strText))
    strText = CSpecialProtocol::ReplaceOldPath(strText, 0);

  CStdString strPath = CSpecialProtocol::TranslatePath(strText);
  return strPath.c_str();
}

const char* CAddon::GetRegion(int id)
{
  CStdString result;

  if (id == 0)      // datelong
    result = g_langInfo.GetDateFormat(true);
  else if (id == 1) // dateshort
    result = g_langInfo.GetDateFormat(false);
  else if (id == 2) // tempunit
    result = g_langInfo.GetTempUnitString();
  else if (id == 3) // speedunit
    result = g_langInfo.GetSpeedUnitString();
  else if (id == 4) // time
    result = g_langInfo.GetTimeFormat();
  else if (id == 5) // meridiem
    result.Format("%s/%s", g_langInfo.GetMeridiemSymbol(CLangInfo::MERIDIEM_SYMBOL_AM), g_langInfo.GetMeridiemSymbol(CLangInfo::MERIDIEM_SYMBOL_PM));

  return result.c_str();
}

const char* CAddon::GetSupportedMedia(int media)
{
  CStdString result;
  if (media == 0)
    result = g_stSettings.m_videoExtensions;
  else if (media == 1)
    result = g_stSettings.m_musicExtensions;
  else if (media == 2)
    result = g_stSettings.m_pictureExtensions;

  return result.c_str();
}

bool CAddon::SkinHasImage(const char *filename)
{
  return g_TextureManager.HasTexture(filename);
}

} /* namespace ADDON */

