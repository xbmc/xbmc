/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Addon.h"

#include "GUIUserMessages.h"
#include "LanguageHook.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/settings/AddonSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

using namespace ADDON;

namespace XBMCAddon
{
  namespace xbmcaddon
  {
    String Addon::getDefaultId() { return languageHook == NULL ? emptyString : languageHook->GetAddonId(); }

    String Addon::getAddonVersion() { return languageHook == NULL ? emptyString : languageHook->GetAddonVersion(); }

    bool Addon::UpdateSettingInActiveDialog(const char* id, const String& value)
    {
      ADDON::AddonPtr addon(pAddon);
      if (!CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
        return false;

      CGUIDialogAddonSettings* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogAddonSettings>(WINDOW_DIALOG_ADDON_SETTINGS);
      if (dialog->GetCurrentAddonID() != addon->ID())
        return false;

      CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
      std::vector<std::string> params;
      params.emplace_back(id);
      params.push_back(value);
      message.SetStringParams(params);
      message.SetParam1(ADDON_SETTINGS_ID);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message, WINDOW_DIALOG_ADDON_SETTINGS);

      return true;
    }

    Addon::Addon(const char* cid)
    {
      String id(cid ? cid : emptyString);

      // if the id wasn't passed then get the id from
      //   the global dictionary
      if (id.empty())
        id = getDefaultId();

      // if we still don't have an id then bail
      if (id.empty())
        throw AddonException("No valid addon id could be obtained. None was passed and the script "
                             "wasn't executed in a normal Kodi manner.");

      if (!CServiceBroker::GetAddonMgr().GetAddon(id, pAddon, OnlyEnabled::CHOICE_YES))
        throw AddonException("Unknown addon id '%s'.", id.c_str());

      CServiceBroker::GetAddonMgr().AddToUpdateableAddons(pAddon);
    }

    Addon::~Addon()
    {
      CServiceBroker::GetAddonMgr().RemoveFromUpdateableAddons(pAddon);
    }

    String Addon::getLocalizedString(int id)
    {
      return g_localizeStrings.GetAddonString(pAddon->ID(), id);
    }

    Settings* Addon::getSettings()
    {
      return new Settings(pAddon->GetSettings());
    }

    String Addon::getSetting(const char* id)
    {
      return pAddon->GetSetting(id);
    }

    bool Addon::getSettingBool(const char* id)
    {
      bool value = false;
      if (!pAddon->GetSettingBool(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      return value;
    }

    int Addon::getSettingInt(const char* id)
    {
      int value = 0;
      if (!pAddon->GetSettingInt(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      return value;
    }

    double Addon::getSettingNumber(const char* id)
    {
      double value = 0.0;
      if (!pAddon->GetSettingNumber(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      return value;
    }

    String Addon::getSettingString(const char* id)
    {
      std::string value;
      if (!pAddon->GetSettingString(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      return value;
    }

    void Addon::setSetting(const char* id, const String& value)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (!UpdateSettingInActiveDialog(id, value))
      {
        addon->UpdateSetting(id, value);
        addon->SaveSettings();
      }
    }

    bool Addon::setSettingBool(const char* id, bool value)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (UpdateSettingInActiveDialog(id, value ? "true" : "false"))
        return true;

      if (!addon->UpdateSettingBool(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      addon->SaveSettings();

      return true;
    }

    bool Addon::setSettingInt(const char* id, int value)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (UpdateSettingInActiveDialog(id, std::to_string(value)))
        return true;

      if (!addon->UpdateSettingInt(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      addon->SaveSettings();

      return true;
    }

    bool Addon::setSettingNumber(const char* id, double value)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (UpdateSettingInActiveDialog(id, StringUtils::Format("{:f}", value)))
        return true;

      if (!addon->UpdateSettingNumber(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      addon->SaveSettings();

      return true;
    }

    bool Addon::setSettingString(const char* id, const String& value)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (UpdateSettingInActiveDialog(id, value))
        return true;

      if (!addon->UpdateSettingString(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      addon->SaveSettings();

      return true;
    }

    void Addon::openSettings()
    {
      DelayedCallGuard dcguard(languageHook);
      // show settings dialog
      ADDON::AddonPtr addon(pAddon);
      CGUIDialogAddonSettings::ShowForAddon(addon);
    }

    String Addon::getAddonInfo(const char* id)
    {
      if (StringUtils::CompareNoCase(id, "author") == 0)
        return pAddon->Author();
      else if (StringUtils::CompareNoCase(id, "changelog") == 0)
        return pAddon->ChangeLog();
      else if (StringUtils::CompareNoCase(id, "description") == 0)
        return pAddon->Description();
      else if (StringUtils::CompareNoCase(id, "disclaimer") == 0)
        return pAddon->Disclaimer();
      else if (StringUtils::CompareNoCase(id, "fanart") == 0)
        return pAddon->FanArt();
      else if (StringUtils::CompareNoCase(id, "icon") == 0)
        return pAddon->Icon();
      else if (StringUtils::CompareNoCase(id, "id") == 0)
        return pAddon->ID();
      else if (StringUtils::CompareNoCase(id, "name") == 0)
        return pAddon->Name();
      else if (StringUtils::CompareNoCase(id, "path") == 0)
        return pAddon->Path();
      else if (StringUtils::CompareNoCase(id, "profile") == 0)
        return pAddon->Profile();
      else if (StringUtils::CompareNoCase(id, "stars") == 0)
        return StringUtils::Format("-1");
      else if (StringUtils::CompareNoCase(id, "summary") == 0)
        return pAddon->Summary();
      else if (StringUtils::CompareNoCase(id, "type") == 0)
        return ADDON::CAddonInfo::TranslateType(pAddon->Type());
      else if (StringUtils::CompareNoCase(id, "version") == 0)
        return pAddon->Version().asString();
      else
        throw AddonException("'%s' is an invalid Id", id);
    }
  }
}
