/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "AddonHelpers_Addon.h"
#include "log.h"
#include "LangInfo.h"

namespace ADDON
{

CAddonHelpers_Addon::CAddonHelpers_Addon(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_AddOnLib;

  /* AddOn Helper functions */
  m_callbacks->Log                = AddOnLog;
  m_callbacks->QueueNotification  = QueueNotification;
  m_callbacks->GetSetting         = GetAddonSetting;
  m_callbacks->UnknownToUTF8      = UnknownToUTF8;
  m_callbacks->GetLocalizedString = GetLocalizedString;
  m_callbacks->GetDVDMenuLanguage = GetDVDMenuLanguage;
}

CAddonHelpers_Addon::~CAddonHelpers_Addon()
{
  delete m_callbacks;
}

void CAddonHelpers_Addon::AddOnLog(void *addonData, const addon_log_t loglevel, const char *msg)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (!helper)
    return;

  CAddonHelpers_Addon* addonHelper = helper->GetHelperAddon();

  try
  {
    CStdString xbmcMsg;
    xbmcMsg.Format("AddOnLog: %s/%s: %s", TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), msg);

    int xbmclog;
    switch (loglevel)
    {
      case LOG_ERROR:
        xbmclog = LOGERROR;
        break;
      case LOG_INFO:
        xbmclog = LOGINFO;
        break;
      case LOG_NOTICE:
        xbmclog = LOGNOTICE;
        break;
      case LOG_DEBUG:
      default:
        xbmclog = LOGDEBUG;
        break;
    }

    /* finally write the logmessage */
    CLog::Log(xbmclog, "%s", xbmcMsg.c_str());
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "AddOnLog: %s/%s - exception '%s' during AddOnLogCallback occurred, contact Developer '%s' of this AddOn", TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), e.what(), addonHelper->m_addon->Author().c_str());
    return;
  }
}

void CAddonHelpers_Addon::QueueNotification(void *addonData, const queue_msg_t type, const char *msg)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (!helper)
    return;

  CAddonHelpers_Addon* addonHelper = helper->GetHelperAddon();

  try
  {
    switch (type)
    {
      case QUEUE_WARNING:
        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Warning, addonHelper->m_addon->Name(), msg, 3000, true);
        CLog::Log(LOGDEBUG, "%s: %s-%s - Warning Message : %s", __FUNCTION__, TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), msg);
        break;

      case QUEUE_ERROR:
        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Error, addonHelper->m_addon->Name(), msg, 3000, true);
        CLog::Log(LOGDEBUG, "%s: %s-%s - Error Message : %s", __FUNCTION__, TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), msg);
        break;

      case QUEUE_INFO:
      default:
        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Info, addonHelper->m_addon->Name(), msg, 3000, false);
        CLog::Log(LOGDEBUG, "%s: %s-%s - Info Message : %s", __FUNCTION__, TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), msg);
        break;
    }
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "QueueNotification: %s/%s - exception '%s' during AddOnLogCallback occurred, contact Developer '%s' of this AddOn", TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), e.what(), addonHelper->m_addon->Author().c_str());
    return;
  }
}

bool CAddonHelpers_Addon::GetAddonSetting(void *addonData, const char* settingName, void *settingValue)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (!helper || settingName == NULL || settingValue == NULL)
    return false;

  CAddonHelpers_Addon* addonHelper = helper->GetHelperAddon();

  try
  {
    CLog::Log(LOGDEBUG, "CAddonHelpers_Addon: AddOn %s request Setting %s", addonHelper->m_addon->Name().c_str(), settingName);

    if (!addonHelper->m_addon->LoadSettings())
    {
      CLog::Log(LOGERROR, "Could't get Settings for AddOn: %s", addonHelper->m_addon->Name().c_str());
      return false;
    }

    TiXmlElement *setting = addonHelper->m_addon->GetSettingsXML()->FirstChildElement("setting");
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
          strcpy((char*) settingValue, addonHelper->m_addon->GetSetting(id).c_str());
          return true;
        }
        else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
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
        else
        {
          CLog::Log(LOGERROR, "Unknown setting type '%s' for id %s in %s", type, id, addonHelper->m_addon->Name().c_str());
        }
      }
      setting = setting->NextSiblingElement("setting");
    }
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s - exception '%s' during GetAddonSetting occurred, contact Developer '%s' of this AddOn", addonHelper->m_addon->Name().c_str(), e.what(), addonHelper->m_addon->Author().c_str());
  }
  return false;
}

char* CAddonHelpers_Addon::UnknownToUTF8(const char *sourceDest)
{
  CStdString string;
  if (sourceDest != NULL)
    g_charsetConverter.unknownToUTF8(sourceDest, string);
  else
    string = "";
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

const char* CAddonHelpers_Addon::GetLocalizedString(const void* addonData, long dwCode)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (!helper)
    return NULL;

  CAddonHelpers_Addon* addonHelper = helper->GetHelperAddon();

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

const char* CAddonHelpers_Addon::GetDVDMenuLanguage(const void* addonData)
{
  CAddonHelpers* helper = (CAddonHelpers*) addonData;
  if (!helper)
    return NULL;

  CStdString string = g_langInfo.GetDVDMenuLanguage();

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}



}; /* namespace ADDON */
