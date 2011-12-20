/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "Application.h"
#include "Addon.h"
#include "AddonCallbacksAddon.h"
#include "utils/log.h"
#include "LangInfo.h"
#include "dialogs/GUIDialogKaiToast.h"

namespace ADDON
{

CAddonCallbacksAddon::CAddonCallbacksAddon(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_AddOnLib;

  /* write XBMC addon-on specific add-on function addresses to the callback table */
  m_callbacks->Log                = AddOnLog;
  m_callbacks->QueueNotification  = QueueNotification;
  m_callbacks->GetSetting         = GetAddonSetting;
  m_callbacks->UnknownToUTF8      = UnknownToUTF8;
  m_callbacks->GetLocalizedString = GetLocalizedString;
  m_callbacks->GetDVDMenuLanguage = GetDVDMenuLanguage;
}

CAddonCallbacksAddon::~CAddonCallbacksAddon()
{
  /* delete the callback table */
  delete m_callbacks;
}

void CAddonCallbacksAddon::AddOnLog(void *addonData, const addon_log_t addonLogLevel, const char *strMessage)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || strMessage == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CAddonCallbacksAddon* addonHelper = addon->GetHelperAddon();

  try
  {
    int xbmcLogLevel = LOGNONE;
    switch (addonLogLevel)
    {
      case LOG_ERROR:
        xbmcLogLevel = LOGERROR;
        break;
      case LOG_INFO:
        xbmcLogLevel = LOGINFO;
        break;
      case LOG_NOTICE:
        xbmcLogLevel = LOGNOTICE;
        break;
      case LOG_DEBUG:
      default:
        xbmcLogLevel = LOGDEBUG;
        break;
    }

    CStdString strXbmcMessage;
    strXbmcMessage.Format("AddOnLog: %s: %s", addonHelper->m_addon->Name().c_str(), strMessage);
    CLog::Log(xbmcLogLevel, "%s", strXbmcMessage.c_str());
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - exception '%s' caught in call in add-on '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), addonHelper->m_addon->Name().c_str(), addonHelper->m_addon->Author().c_str());
  }
}

void CAddonCallbacksAddon::QueueNotification(void *addonData, const queue_msg_t type, const char *strMessage)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || strMessage == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - called with a null pointer", __FUNCTION__);
    return;
  }

  CAddonCallbacksAddon* addonHelper = addon->GetHelperAddon();

  try
  {
    switch (type)
    {
      case QUEUE_WARNING:
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, addonHelper->m_addon->Name(), strMessage, 3000, true);
        CLog::Log(LOGDEBUG, "CAddonCallbacksAddon - %s - %s - Warning Message: '%s'", __FUNCTION__, addonHelper->m_addon->Name().c_str(), strMessage);
        break;

      case QUEUE_ERROR:
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, addonHelper->m_addon->Name(), strMessage, 3000, true);
        CLog::Log(LOGDEBUG, "CAddonCallbacksAddon - %s - %s - Error Message : '%s'", __FUNCTION__, addonHelper->m_addon->Name().c_str(), strMessage);
        break;

      case QUEUE_INFO:
      default:
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, addonHelper->m_addon->Name(), strMessage, 3000, false);
        CLog::Log(LOGDEBUG, "CAddonCallbacksAddon - %s - %s - Info Message : '%s'", __FUNCTION__, addonHelper->m_addon->Name().c_str(), strMessage);
        break;
    }
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - exception '%s' caught in call in add-on '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), addonHelper->m_addon->Name().c_str(), addonHelper->m_addon->Author().c_str());
  }
}

bool CAddonCallbacksAddon::GetAddonSetting(void *addonData, const char *strSettingName, void *settingValue)
{
  CAddonCallbacks* addon = (CAddonCallbacks*) addonData;
  if (addon == NULL || strSettingName == NULL || settingValue == NULL)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - called with a null pointer", __FUNCTION__);
    return false;
  }

  CAddonCallbacksAddon* addonHelper = addon->GetHelperAddon();

  try
  {
    CLog::Log(LOGDEBUG, "CAddonCallbacksAddon - %s - add-on '%s' requests setting '%s'", __FUNCTION__, addonHelper->m_addon->Name().c_str(), strSettingName);

    if (!addonHelper->m_addon->ReloadSettings())
    {
      CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - could't get settings for add-on '%s'", __FUNCTION__, addonHelper->m_addon->Name().c_str());
      return false;
    }

    const TiXmlElement *category = addonHelper->m_addon->GetSettingsXML()->FirstChildElement("category");
    if (!category) // add a default one...
      category = addonHelper->m_addon->GetSettingsXML();

    while (category)
    {
      const TiXmlElement *setting = category->FirstChildElement("setting");
      while (setting)
      {
        const char *id = setting->Attribute("id");
        const char *type = setting->Attribute("type");

        if (strcmpi(id, strSettingName) == 0 && type)
        {
          if (strcmpi(type, "text")   == 0 || strcmpi(type, "ipaddress") == 0 ||
              strcmpi(type, "folder") == 0 || strcmpi(type, "action")    == 0 ||
              strcmpi(type, "music")  == 0 || strcmpi(type, "pictures")  == 0 ||
              strcmpi(type, "folder") == 0 || strcmpi(type, "programs")  == 0 ||
              strcmpi(type, "files")  == 0 || strcmpi(type, "fileenum")  == 0)
          {
            strcpy((char*) settingValue, addonHelper->m_addon->GetSetting(id).c_str());
            return true;
          }
          else if (strcmpi(type, "number") == 0 || strcmpi(type, "enum") == 0 ||
                   strcmpi(type, "labelenum") == 0)
          {
            *(int*) settingValue = (int) atoi(addonHelper->m_addon->GetSetting(id));
            return true;
          }
          else if (strcmpi(type, "bool") == 0)
          {
            *(bool*) settingValue = (bool) (addonHelper->m_addon->GetSetting(id) == "true" ? true : false);
            return true;
          }
        }
        setting = setting->NextSiblingElement("setting");
      }
      category = category->NextSiblingElement("category");
    }
    CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - can't find setting '%s' in '%s'", __FUNCTION__, strSettingName, addonHelper->m_addon->Name().c_str());
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CAddonCallbacksAddon - %s - exception '%s' caught in call in add-on '%s'. please contact the developer of this addon: %s",
        __FUNCTION__, e.what(), addonHelper->m_addon->Name().c_str(), addonHelper->m_addon->Author().c_str());
  }

  return false;
}

char* CAddonCallbacksAddon::UnknownToUTF8(const char *strSource)
{
  CStdString string;
  if (strSource != NULL)
    g_charsetConverter.unknownToUTF8(strSource, string);
  else
    string = "";
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

const char* CAddonCallbacksAddon::GetLocalizedString(const void* addonData, long dwCode)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper || g_application.m_bStop)
    return NULL;

  CAddonCallbacksAddon* addonHelper = helper->GetHelperAddon();

  CStdString string;
  if (dwCode >= 30000 && dwCode <= 30999)
    string = addonHelper->m_addon->GetString(dwCode).c_str();
  else if (dwCode >= 32000 && dwCode <= 32999)
    string = addonHelper->m_addon->GetString(dwCode).c_str();
  else
    string = g_localizeStrings.Get(dwCode).c_str();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

const char* CAddonCallbacksAddon::GetDVDMenuLanguage(const void* addonData)
{
  CAddonCallbacks* helper = (CAddonCallbacks*) addonData;
  if (!helper)
    return NULL;

  CStdString string = g_langInfo.GetDVDMenuLanguage();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

}; /* namespace ADDON */
