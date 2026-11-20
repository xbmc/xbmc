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
#include "language/Language.h"
#include "language/LanguageLoader.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

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
  // parse <charsets>
  const CAddonExtensions* charsetsElement =
      Type(AddonType::RESOURCE_LANGUAGE)->GetElement("charsets");
  if (charsetsElement != nullptr)
  {
    m_charsetGui = charsetsElement->GetValue("gui").asString();
    m_charsetSubtitle = charsetsElement->GetValue("subtitle").asString();
  }

  // parse <sorttokens>
  const CAddonExtensions* sorttokensElement =
      Type(AddonType::RESOURCE_LANGUAGE)->GetElement("sorttokens");
  if (sorttokensElement != nullptr)
  {
    // <token separators="'">L</token> is stored as L', one entry per separator
    for (const auto& [_, addonExtensions] : sorttokensElement->GetValues())
    {
      const std::string token = addonExtensions.GetValue("token").asString();
      if (!token.empty())
      {
        const std::string separators = addonExtensions.GetValue("token@separators").asString();
        KODI::LANGUAGE::CLanguage::AddSortToken(
            m_sortTokens, token,
            separators.empty() ? KODI::LANGUAGE::CLanguage::DEFAULT_SORT_TOKEN_SEPARATORS
                               : std::string_view{separators});
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
