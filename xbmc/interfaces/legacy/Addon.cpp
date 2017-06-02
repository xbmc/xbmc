/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon.h"
#include "LanguageHook.h"

#include "addons/AddonManager.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
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
      if (!g_windowManager.IsWindowActive(WINDOW_DIALOG_ADDON_SETTINGS))
        return false;

      CGUIDialogAddonSettings* dialog = g_windowManager.GetWindow<CGUIDialogAddonSettings>(WINDOW_DIALOG_ADDON_SETTINGS);
      if (dialog->GetCurrentAddonID() != addon->ID())
        return false;

      CGUIMessage message(GUI_MSG_SETTING_UPDATED, 0, 0);
      std::vector<std::string> params;
      params.push_back(id);
      params.push_back(value);
      message.SetStringParams(params);
      g_windowManager.SendThreadMessage(message, WINDOW_DIALOG_ADDON_SETTINGS);

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
        throw AddonException("No valid addon id could be obtained. None was passed and the script wasn't executed in a normal xbmc manner.");

      if (!ADDON::CAddonMgr::GetInstance().GetAddon(id.c_str(), pAddon))
        throw AddonException("Unknown addon id '%s'.", id.c_str());

      CAddonMgr::GetInstance().AddToUpdateableAddons(pAddon);
    }

    Addon::~Addon()
    {
      CAddonMgr::GetInstance().RemoveFromUpdateableAddons(pAddon);
    }

    String Addon::getLocalizedString(int id)
    {
      return g_localizeStrings.GetAddonString(pAddon->ID(), id);
    }

    String Addon::getSetting(const char* id)
    {
      return pAddon->GetSetting(id);
    }

    bool Addon::getSettingBool(const char* id) throw(XBMCAddon::WrongTypeException)
    {
      bool value = false;
      if (!pAddon->GetSettingBool(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      return value;
    }

    int Addon::getSettingInt(const char* id) throw(XBMCAddon::WrongTypeException)
    {
      int value = 0;
      if (!pAddon->GetSettingInt(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      return value;
    }

    double Addon::getSettingNumber(const char* id) throw(XBMCAddon::WrongTypeException)
    {
      double value = 0.0;
      if (!pAddon->GetSettingNumber(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      return value;
    }

    String Addon::getSettingString(const char* id) throw(XBMCAddon::WrongTypeException)
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

    bool Addon::setSettingBool(const char* id, bool value) throw(XBMCAddon::WrongTypeException)
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

    bool Addon::setSettingInt(const char* id, int value) throw(XBMCAddon::WrongTypeException)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (UpdateSettingInActiveDialog(id, StringUtils::Format("%d", value)))
        return true;

      if (!addon->UpdateSettingInt(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      addon->SaveSettings();

      return true;
    }

    bool Addon::setSettingNumber(const char* id, double value) throw(XBMCAddon::WrongTypeException)
    {
      DelayedCallGuard dcguard(languageHook);
      ADDON::AddonPtr addon(pAddon);
      if (UpdateSettingInActiveDialog(id, StringUtils::Format("%f", value)))
        return true;

      if (!addon->UpdateSettingNumber(id, value))
        throw XBMCAddon::WrongTypeException("Invalid setting type");

      addon->SaveSettings();

      return true;
    }

    bool Addon::setSettingString(const char* id, const String& value) throw(XBMCAddon::WrongTypeException)
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
      if (strcmpi(id, "author") == 0)
        return pAddon->Author();
      else if (strcmpi(id, "changelog") == 0)
        return pAddon->ChangeLog();
      else if (strcmpi(id, "description") == 0)
        return pAddon->Description();
      else if (strcmpi(id, "disclaimer") == 0)
        return pAddon->Disclaimer();
      else if (strcmpi(id, "fanart") == 0)
        return pAddon->FanArt();
      else if (strcmpi(id, "icon") == 0)
        return pAddon->Icon();
      else if (strcmpi(id, "id") == 0)
        return pAddon->ID();
      else if (strcmpi(id, "name") == 0)
        return pAddon->Name();
      else if (strcmpi(id, "path") == 0)
        return pAddon->Path();
      else if (strcmpi(id, "profile") == 0)
        return pAddon->Profile();
      else if (strcmpi(id, "stars") == 0)
        return StringUtils::Format("-1");
      else if (strcmpi(id, "summary") == 0)
        return pAddon->Summary();
      else if (strcmpi(id, "type") == 0)
        return ADDON::TranslateType(pAddon->Type());
      else if (strcmpi(id, "version") == 0)
        return pAddon->Version().asString();
      else
        throw AddonException("'%s' is an invalid Id", id);
    }
  }
}
