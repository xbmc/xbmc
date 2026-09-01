/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "LanguageResource.h"

#include "ServiceBroker.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "language/LanguageLoader.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <array>
#include <string_view>

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

namespace
{
constexpr const char* LANGUAGE_ADDON_PREFIX = "resource.language.";
}

namespace ADDON
{

CLanguageResource::CLanguageResource(const AddonInfoPtr& addonInfo)
  : CResource(addonInfo, AddonType::RESOURCE_LANGUAGE),
    // parse <extension> attributes
    m_language(KODI::LANGUAGE::CLanguageTag::Parse(
        Type(AddonType::RESOURCE_LANGUAGE)->GetValue("@locale").asString()))
{
  // The locale is kept as written either way, so an addon naming a language Kodi does not know
  // still loads - it simply cannot be matched against media, which is worth saying out loud
  if (!m_language.IsValid())
  {
    CLog::Log(LOGWARNING,
              "CLanguageResource: addon '{}' states a locale of '{}', which names no language "
              "Kodi recognizes",
              ID(), m_language.ToString());
  }

  // parse <charsets>
  const CAddonExtensions* charsetsElement =
      Type(AddonType::RESOURCE_LANGUAGE)->GetElement("charsets");
  if (charsetsElement != nullptr)
  {
    m_charsetGui = charsetsElement->GetValue("gui").asString();
    m_forceUnicodeFont = charsetsElement->GetValue("gui@unicodefont").asBoolean();
    m_charsetSubtitle = charsetsElement->GetValue("subtitle").asString();
  }

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
    for (const auto& [_, addonExtensions] : sorttokensElement->GetValues())
    {
      /* Second loop goes around the row parts, e.g.
       *   separators = "'"
       *   token = Le
       */
      const std::string token = addonExtensions.GetValue("token").asString();
      if (!token.empty())
      {
        std::string separators = addonExtensions.GetValue("token@separators").asString();
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
  if (!CServiceBroker::GetGUI())
    return;

  if (IsInUse() || (!update && !modal &&
                    (HELPERS::ShowYesNoDialogText(CVariant{Name()}, CVariant{24132}) ==
                     DialogResponse::CHOICE_YES)))
  {
    if (IsInUse())
      KODI::LANGUAGE::CLanguageLoader::GetInstance().Load(ID());
    else
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_LANGUAGE, ID());
  }
}

CResource::Published CLanguageResource::PublishedFiles() const
{
  static constexpr std::array<std::string_view, 2> names{"langinfo.xml", "strings.po"};
  return {.names = names};
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

}
