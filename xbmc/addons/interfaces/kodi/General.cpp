/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "General.h"

#include "addons/kodi-addon-dev-kit/include/kodi/General.h"

#include "ServiceBroker.h"
#include "addons/AddonDll.h"
#include "addons/GUIDialogAddonSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace kodi; // addon-dev-kit namespace

namespace ADDON
{

void Interface_General::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi.kodi = (AddonToKodiFuncTable_kodi*)malloc(sizeof(AddonToKodiFuncTable_kodi));
  addonInterface->toKodi.kodi->get_setting = get_setting;
  addonInterface->toKodi.kodi->open_settings_dialog = open_settings_dialog;
}

void Interface_General::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi.kodi)
  {
    free(addonInterface->toKodi.kodi);
    addonInterface->toKodi.kodi = nullptr;
  }
}

bool Interface_General::get_setting(void* kodiBase, const char* settingName, void* settingValue)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || settingName == nullptr || settingValue == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - invalid data (addon='%p', settingName='%p', settingValue='%p')",
                                        __FUNCTION__, addon, settingName, settingValue);

    return false;
  }

  CLog::Log(LOGDEBUG, "kodi::General::%s - add-on '%s' requests setting '%s'",
                        __FUNCTION__, addon->Name().c_str(), settingName);

  if (strcasecmp(settingName, "__addonpath__") == 0)
  {
    strcpy((char*) settingValue, addon->Path().c_str());
    return true;
  }

  if (!addon->ReloadSettings())
  {
    CLog::Log(LOGERROR, "kodi::General::%s - could't get settings for add-on '%s'", __FUNCTION__, addon->Name().c_str());
    return false;
  }

  const TiXmlElement *category = addon->GetSettingsXML()->FirstChildElement("category");
  if (!category) // add a default one...
    category = addon->GetSettingsXML();

  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const std::string   id = XMLUtils::GetAttribute(setting, "id");
      const std::string type = XMLUtils::GetAttribute(setting, "type");

      if (id == settingName && !type.empty())
      {
        if (type == "text"     || type == "ipaddress" ||
            type == "folder"   || type == "action"    ||
            type == "music"    || type == "pictures"  ||
            type == "programs" || type == "fileenum"  ||
            type == "file"     || type == "labelenum")
        {
          strcpy((char*) settingValue, addon->GetSetting(id).c_str());
          return true;
        }
        else if (type == "number" || type == "enum")
        {
          *(int*) settingValue = (int) atoi(addon->GetSetting(id).c_str());
          return true;
        }
        else if (type == "bool")
        {
          *(bool*) settingValue = (bool) (addon->GetSetting(id) == "true" ? true : false);
          return true;
        }
        else if (type == "slider")
        {
          const char *option = setting->Attribute("option");
          if (option && strcmpi(option, "int") == 0)
          {
            *(int*) settingValue = (int) atoi(addon->GetSetting(id).c_str());
            return true;
          }
          else
          {
            *(float*) settingValue = (float) atof(addon->GetSetting(id).c_str());
            return true;
          }
        }
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  CLog::Log(LOGERROR, "kodi::General::%s - can't find setting '%s' in '%s'", __FUNCTION__, settingName, addon->Name().c_str());

  return false;
}

void Interface_General::open_settings_dialog(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "kodi::General::%s - called with a null pointer", __FUNCTION__);
    return;
  }

  // show settings dialog
  CGUIDialogAddonSettings::ShowAndGetInput(ADDON::AddonPtr(addon));
}

} /* namespace ADDON */
