/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "LanguageResource.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

#define LANGUAGE_ADDON_PREFIX   "resource.language."

namespace ADDON
{

CLanguageResource::CLanguageResource(const AddonInfoPtr& addonInfo)
  : CResource(addonInfo, AddonType::RESOURCE_LANGUAGE)
{
  // parse <extension> attributes
  m_locale =
      CLocale::FromString(Type(AddonType::RESOURCE_LANGUAGE)->GetValue("@locale").asString());

  // parse <charsets>
  const CAddonExtensions* charsetsElement =
      Type(AddonType::RESOURCE_LANGUAGE)->GetElement("charsets");
  if (charsetsElement != nullptr)
  {
    m_charsetGui = charsetsElement->GetValue("gui").asString();
    m_forceUnicodeFont = charsetsElement->GetValue("gui@unicodefont").asBoolean();
    m_charsetSubtitle = charsetsElement->GetValue("subtitle").asString();
  }

  // parse <dvd>
  const CAddonExtensions* dvdElement = Type(AddonType::RESOURCE_LANGUAGE)->GetElement("dvd");
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
  const CAddonExtensions* sorttokensElement =
      Type(AddonType::RESOURCE_LANGUAGE)->GetElement("sorttokens");
  if (sorttokensElement != nullptr)
  {
    /* First loop goes around rows e.g.
     *   <token separators="'">L</token>
     *   <token>Le</token>
     *   ...
     */
    for (const auto& values : sorttokensElement->GetValues())
    {
      /* Second loop goes around the row parts, e.g.
       *   separators = "'"
       *   token = Le
       */
      std::string token = values.second.GetValue("token").asString();
      std::string separators = values.second.GetValue("token@separators").asString();
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
  return StringUtils::EqualsNoCase(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE), ID());
}

void CLanguageResource::OnPostInstall(bool update, bool modal)
{
  if (!g_SkinInfo)
    return;

  if (IsInUse() || (!update && !modal &&
                    (HELPERS::ShowYesNoDialogText(CVariant{Name()}, CVariant{24132}) ==
                     DialogResponse::CHOICE_YES)))
  {
    if (IsInUse())
      g_langInfo.SetLanguage(ID());
    else
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_LANGUAGE, ID());
  }
}

bool CLanguageResource::IsAllowed(const std::string &file) const
{
  return file.empty() ||
         StringUtils::EqualsNoCase(file.c_str(), "langinfo.xml") ||
         StringUtils::EqualsNoCase(file.c_str(), "strings.po");
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

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, AddonType::RESOURCE_LANGUAGE,
                                              OnlyEnabled::CHOICE_YES))
    return false;

  legacyLanguage = addon->Name();
  return true;
}

}
