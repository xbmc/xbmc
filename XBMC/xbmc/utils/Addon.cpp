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
#include "PVRManager.h"
#include "Util.h"
#include "URL.h"

namespace ADDON
{

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

bool CAddon::operator==(const CAddon &rhs) const {
  return (m_guid == rhs.m_guid);
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

void CAddon::OpenAddonSettings(const CAddon* addon, bool bReload)
{
  if (addon == NULL)
    return;

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

  return;
}

/**
* XBMC AddOn Dialog callbacks
* Helper functions to access GUI Dialog related functions
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
* XBMC AddOn Utils callbacks
* Helper' to access XBMC Utilities
*/

const char* CAddon::GetLocalizedString(const CAddon* addon, long dwCode)
{
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

const char* CAddon::UnknownToUTF8(const char *sourceDest)
{
  CStdString string = sourceDest;
  g_charsetConverter.unknownToUTF8(string);
  return string.c_str();
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
  if (addon == NULL)
    return;

  CLog::Log(LOGDEBUG, "Calling TransferAddonSettings for: %s", addon->m_strName.c_str());
        
  /* Transmit current unified user settings to the PVR Addon */
  ADDON::IAddonCallback* addonCB = GetCallbackForType(addon->m_addonType);

  CAddonSettings settings;
  settings.Load(addon->m_strPath);

  TiXmlElement *setting = settings.GetAddonRoot()->FirstChildElement("setting");
  while (setting)
  {
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
        addonCB->SetSetting(addon, id, (const char*) settings.Get(id).c_str());
      }
      else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
               strcmpi(type, "labelenum") == 0)
      {
        int tmp = atoi(settings.Get(id));
        addonCB->SetSetting(addon, id, (int*) &tmp);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        bool tmp = settings.Get(id) == "true" ? true : false;
        addonCB->SetSetting(addon, id, (bool*) &tmp);
      }
      else
      {
        CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type, addon->m_strName.c_str());
      }
    }
    setting = setting->NextSiblingElement("setting");
  }
}

IAddonCallback* CAddon::GetCallbackForType(AddonType type)
{
  switch (type)
  {
    case ADDON_MULTITYPE:
      return NULL;
    case ADDON_VIZ:
      return NULL;
    case ADDON_SKIN:
      return NULL;
    case ADDON_PVRDLL:
      return CPVRManager::GetInstance();
    case ADDON_SCRIPT:
      return NULL;
    case ADDON_SCRAPER:
      return NULL;
    case ADDON_SCREENSAVER:
      return NULL;
    case ADDON_PLUGIN_PVR:
      return NULL;
    case ADDON_PLUGIN_MUSIC:
      return NULL;
    case ADDON_PLUGIN_VIDEO:
      return NULL;
    case ADDON_PLUGIN_PROGRAM:
      return NULL;
    case ADDON_PLUGIN_PICTURES:
      return NULL;
    case ADDON_UNKNOWN:
    default:
      return NULL;
  }
}

} /* namespace ADDON */

