/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GlobalsHandling.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class SubTagScope
{
  Unknown,
  Any,
  MacroLanguage,
  Collection,
  Special,
  PrivateUse
};

struct GenericSubTag
{
  std::string m_subTag;
  std::vector<std::string> m_description;
  std::string m_added;
  std::string m_deprecated;
  std::string m_preferredValue;
};

struct LanguageSubTag : GenericSubTag
{
  std::string m_suppressScript;
  std::string m_macroLanguage;
  SubTagScope m_scope;
};

struct ExtLangSubTag : GenericSubTag
{
  std::string m_prefix;
  std::string m_suppressScript;
  std::string m_macroLanguage;
  SubTagScope m_scope;
};

struct ScriptSubTag : GenericSubTag
{
};

struct RegionSubTag : GenericSubTag
{
};

struct VariantSubTag : GenericSubTag
{
  std::vector<std::string> m_prefix;
};

struct GrandfatheredTag : GenericSubTag
{
};

struct RedundantTag : GenericSubTag
{
};

class CSubTagRegistry
{
public:
  bool Initialize();
  void Deinitialize() {}

  std::optional<LanguageSubTag> LookupLanguage(std::string_view subTag);
  std::optional<ExtLangSubTag> LookupExtLang(std::string_view subTag);
  std::optional<ScriptSubTag> LookupScript(std::string_view subTag);
  std::optional<RegionSubTag> LookupRegsion(std::string_view subTag);
  std::optional<VariantSubTag> LookupVariant(std::string_view subTag);
  std::optional<GrandfatheredTag> LookupGrandfathered(std::string_view tag);
  std::optional<RedundantTag> LookupRedundant(std::string_view tag);

  std::optional<LanguageSubTag> LookupLanguageDescription(std::string_view description);
  std::optional<ExtLangSubTag> LookupExtLangDescription(std::string_view description);
  std::optional<ScriptSubTag> LookupScriptDescription(std::string_view description);
  std::optional<RegionSubTag> LookupRegsionDescription(std::string_view description);
  std::optional<VariantSubTag> LookupVariantDescription(std::string_view description);
  std::optional<GrandfatheredTag> LookupGrandfatheredDescription(std::string_view description);
  std::optional<RedundantTag> LookupRedundantDescription(std::string_view subTagdescription);

private:

  bool LoadFromFile();
  void BuildReverseDescriptionLookups();
  
  std::unordered_map<std::string, LanguageSubTag> g_languageSubTags;
  std::unordered_map<std::string, ExtLangSubTag> g_extLangSubTags;
  std::unordered_map<std::string, ScriptSubTag> g_scriptSubTags;
  std::unordered_map<std::string, RegionSubTag> g_regionSubTags;
  std::unordered_map<std::string, VariantSubTag> g_variantSubTags;
  std::unordered_map<std::string, GrandfatheredTag> g_grandfatheredTags;
  std::unordered_map<std::string, RedundantTag> g_redundantTags;

  /*!
   * \brief Description to tag/subtag lookup for efficiency
   *        key = description, value = tag/subtag value
   */
  std::unordered_map<std::string, std::string> g_languageDescToSubTags;
  std::unordered_map<std::string, std::string> g_extLangDescToSubTags;
  std::unordered_map<std::string, std::string> g_scriptDescToSubTags;
  std::unordered_map<std::string, std::string> g_regionDescToSubTags;
  std::unordered_map<std::string, std::string> g_variantDescToSubTags;
  std::unordered_map<std::string, std::string> g_grandfatheredDescToTags;
  std::unordered_map<std::string, std::string> g_redundantDescToTags;
};


enum class SubTagType
{
Unknown,
Language,
ExtLang,
Script,
Region,
Variant,
Grandfathered,
Redundant,
};

XBMC_GLOBAL_REF(CSubTagRegistry, g_subTagRegistry);
#define g_subTagRegistry XBMC_GLOBAL_USE(CSubTagRegistry)
