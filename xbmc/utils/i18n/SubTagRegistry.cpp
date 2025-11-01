/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/SubTagRegistry.h"

#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/i18n/SubTagRegistryFile.h"

#include <ranges>
#include <utility>

namespace
{
template<typename T>
concept has_description = requires(T a) {
  a.m_description;
  std::ranges::forward_range<decltype(a.m_description)>;
};

template<typename T>
  requires has_description<T>
void BuildLookup(std::unordered_map<std::string, T> subTags,
                 std::unordered_map<std::string, std::string>& reverseMap)
{
  for (const auto& [key, value] : subTags)
    for (auto desc : value.m_description)
    {
      StringUtils::ToLower(desc);
      reverseMap.insert(std::make_pair(desc, key));
    }
}

template<typename T>
std::optional<T> LookupSubTag(std::unordered_map<std::string, T> subTags,
                                           std::string_view subTag)
{
  auto it = subTags.find(std::string{subTag});
  if (it != subTags.end())
    return it->second;
  else
    return std::nullopt;
}

template<typename T>
std::optional<T> LookupDescription(std::unordered_map<std::string, std::string> reverseSubTags,
                                   std::unordered_map<std::string, T> subTags,
                                   std::string_view description)
{
  auto it = reverseSubTags.find(std::string{description});
  if (it == reverseSubTags.end())
    return std::nullopt;

  return LookupSubTag(subTags, it->second);
}
} // namespace

bool CSubTagRegistry::Initialize()
{
  if (LoadFromFile())
  {
    BuildReverseDescriptionLookups();
    return true;
  }
  return false;
}

bool CSubTagRegistry::LoadFromFile()
{
  const std::string filePath =
      CSpecialProtocol::TranslatePath("special://xbmc/system/language-subtag-registry.txt");

  CRegistryFile file(filePath);
  return file.Load();
  //! @todo import all file records
}

void CSubTagRegistry::BuildReverseDescriptionLookups()
{
  BuildLookup(g_languageSubTags, g_languageDescToSubTags);
  BuildLookup(g_extLangSubTags, g_extLangDescToSubTags);
  BuildLookup(g_scriptSubTags, g_scriptDescToSubTags);
  BuildLookup(g_regionSubTags, g_regionDescToSubTags);
  BuildLookup(g_variantSubTags, g_variantDescToSubTags);
  BuildLookup(g_grandfatheredTags, g_grandfatheredDescToTags);
  BuildLookup(g_redundantTags, g_redundantDescToTags);
};

std::optional<LanguageSubTag> CSubTagRegistry::LookupLanguage(std::string_view subTag)
{
  return LookupSubTag(g_languageSubTags, subTag);
}

std::optional<ExtLangSubTag> CSubTagRegistry::LookupExtLang(std::string_view subTag)
{
  return LookupSubTag(g_extLangSubTags, subTag);
}

std::optional<ScriptSubTag> CSubTagRegistry::LookupScript(std::string_view subTag)
{
  return LookupSubTag(g_scriptSubTags, subTag);
}

std::optional<RegionSubTag> CSubTagRegistry::LookupRegsion(std::string_view subTag)
{
  return LookupSubTag(g_regionSubTags, subTag);
}

std::optional<VariantSubTag> CSubTagRegistry::LookupVariant(std::string_view subTag)
{
  return LookupSubTag(g_variantSubTags, subTag);
}

std::optional<GrandfatheredTag> CSubTagRegistry::LookupGrandfathered(std::string_view tag)
{
  return LookupSubTag(g_grandfatheredTags, tag);
}

std::optional<RedundantTag> CSubTagRegistry::LookupRedundant(std::string_view tag)
{
  return LookupSubTag(g_redundantTags, tag);
}

std::optional<LanguageSubTag> CSubTagRegistry::LookupLanguageDescription(
    std::string_view description)
{
  return LookupDescription(g_languageDescToSubTags, g_languageSubTags, description);
}

std::optional<ExtLangSubTag> CSubTagRegistry::LookupExtLangDescription(std::string_view description)
{
  return LookupDescription(g_extLangDescToSubTags, g_extLangSubTags, description);
}

std::optional<ScriptSubTag> CSubTagRegistry::LookupScriptDescription(std::string_view description)
{
  return LookupDescription(g_scriptDescToSubTags, g_scriptSubTags, description);
}

std::optional<RegionSubTag> CSubTagRegistry::LookupRegsionDescription(std::string_view description)
{
  return LookupDescription(g_regionDescToSubTags, g_regionSubTags, description);
}

std::optional<VariantSubTag> CSubTagRegistry::LookupVariantDescription(std::string_view description)
{
  return LookupDescription(g_variantDescToSubTags, g_variantSubTags, description);
}

std::optional<GrandfatheredTag> CSubTagRegistry::LookupGrandfatheredDescription(
    std::string_view description)
{
  return LookupDescription(g_grandfatheredDescToTags, g_grandfatheredTags, description);
}

std::optional<RedundantTag> CSubTagRegistry::LookupRedundantDescription(
    std::string_view description)
{
  return LookupDescription(g_redundantDescToTags, g_redundantTags, description);
}
