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
#include "LanguageResource.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "messaging/helpers/DialogHelper.h"

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

#define LANGUAGE_ADDON_PREFIX   "resource.language."

namespace ADDON
{

CLanguageResource::CLanguageResource(AddonInfoPtr addonInfo)
  : CResource(addonInfo)
{
  // parse <extension> attributes
  m_locale = CLocale::FromString(AddonInfo()->GetValue("@locale").asString());

  // parse <charsets>
  const CAddonExtensions* charsetsElement = AddonInfo()->GetElement("charsets");
  if (charsetsElement != nullptr)
  {
    m_charsetGui = charsetsElement->GetValue("gui").asString();
    m_forceUnicodeFont = charsetsElement->GetValue("gui@unicodefont").asBoolean();
    m_charsetSubtitle = charsetsElement->GetValue("subtitle").asString();
  }

  // parse <dvd>
  const CAddonExtensions* dvdElement = AddonInfo()->GetElement("dvd");
  if (dvdElement != nullptr)
  {
    m_dvdLanguageMenu = dvdElement->GetValue("menu").asString();
    m_dvdLanguageAudio = dvdElement->GetValue("audio").asString();
    m_dvdLanguageSubtitle = dvdElement->GetValue("subtitle").asString();
  }
  // fall back to the language of the addon if a DVD language is not defined
  if (m_dvdLanguageMenu.empty())
    m_dvdLanguageMenu = m_locale.GetLanguageCode();
  if (m_dvdLanguageAudio.empty())
    m_dvdLanguageAudio = m_locale.GetLanguageCode();
  if (m_dvdLanguageSubtitle.empty())
    m_dvdLanguageSubtitle = m_locale.GetLanguageCode();

  // parse <sorttokens>
  const CAddonExtensions* sorttokensElement = AddonInfo()->GetElement("sorttokens");
  if (sorttokensElement != nullptr)
  {
    /* First loop goes around rows e.g.
     *   <token separators="'">L</token>
     *   <token>Le</token>
     *   ...
     */
    for (auto values : sorttokensElement->GetValues())
    {
      std::string token;
      std::string separators;
      /* Second loop goes around the row parts, e.g.
       *   separators = "'"
       *   token = Le
       */
      for (auto value : values.second)
      {
        if (value.first == "token")
          token = value.second.asString();
        else if (value.first == "token@separators")
          separators = value.second.asString();
      }
      if (!token.empty())
      {
        if (separators.empty())
          separators = " ._";

        for (auto separator : separators)
          m_sortTokens.insert(token + separator);
      }
    }
  }
}

bool CLanguageResource::IsInUse() const
{
  return StringUtils::EqualsNoCase(CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOCALE_LANGUAGE), ID());
}

void CLanguageResource::OnPostInstall(bool update, bool modal)
{
  if (IsInUse() ||
     (!update && !modal && 
       (HELPERS::ShowYesNoDialogText(CVariant{Name()}, CVariant{24132}) == DialogResponse::YES)))
  {
    CGUIDialogKaiToast *toast = (CGUIDialogKaiToast *)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
    if (toast)
    {
      toast->ResetTimer();
      toast->Close(true);
    }

    if (IsInUse())
      g_langInfo.SetLanguage(ID());
    else
      CServiceBroker::GetSettings().SetString(CSettings::SETTING_LOCALE_LANGUAGE, ID());
  }
}

bool CLanguageResource::IsAllowed(const std::string &file) const
{
  return file.empty() ||
         StringUtils::EqualsNoCase(file.c_str(), "langinfo.xml") ||
         StringUtils::EqualsNoCase(file.c_str(), "strings.po") ||
         StringUtils::EqualsNoCase(file.c_str(), "strings.xml");
}

std::string CLanguageResource::GetAddonId(const std::string& locale)
{
  if (locale.empty())
    return "";

  std::string addonId = locale;
  if (!StringUtils::StartsWith(addonId, LANGUAGE_ADDON_PREFIX))
    addonId = LANGUAGE_ADDON_PREFIX + locale;

  StringUtils::ToLower(addonId);
  return addonId;
}

bool CLanguageResource::FindLegacyLanguage(const std::string &locale, std::string &legacyLanguage)
{
  if (locale.empty())
    return false;

  std::string addonId = GetAddonId(locale);

  const AddonInfoPtr addon = CAddonMgr::GetInstance().GetInstalledAddonInfo(addonId, ADDON_RESOURCE_LANGUAGE);
  if (addon == nullptr)
    return false;

  legacyLanguage = addon->Name();
  return true;
}

bool CLanguageResource::FindLanguageAddonByName(const std::string &legacyLanguage, std::string &addonId, const AddonInfos &languageAddons /* = AddonInfos() */)
{
  if (legacyLanguage.empty())
    return false;

  AddonInfos addons;
  if (languageAddons.empty())
    addons = CAddonMgr::GetInstance().GetAddonInfos(false, ADDON_RESOURCE_LANGUAGE);
  else
    addons = languageAddons;

  if (addons.empty())
    return false;

  // try to find a language that matches the old language in name or id
  for (auto addon : addons)
  {
    // check if the old language matches the language addon id, the language
    // locale or the language addon name
    CLocale locale = CLocale::FromString(addon->GetValue("@locale").asString());
    if (legacyLanguage.compare(addon->ID()) == 0 ||
        locale.Equals(legacyLanguage) ||
        StringUtils::EqualsNoCase(legacyLanguage, addon->Name()))
    {
      addonId = addon->ID();
      return true;
    }
  }

  return false;
}

}
