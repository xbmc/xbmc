/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"

#include "utils/i18n/Bcp47Registry/RegistryRecordProvider.h"
#include "utils/i18n/Bcp47Registry/SubTagLoader.h"

#include <array>
#include <ranges>

namespace KODI::UTILS::I18N
{
struct SubTagTypeDetails
{
  SubTagType m_type;
  std::string_view m_code;
};

constexpr auto types = std::array{
    SubTagTypeDetails{SubTagType::Language, "language"},
    SubTagTypeDetails{SubTagType::ExtLang, "extlang"},
    SubTagTypeDetails{SubTagType::Script, "script"},
    SubTagTypeDetails{SubTagType::Region, "region"},
    SubTagTypeDetails{SubTagType::Variant, "variant"},
    SubTagTypeDetails{SubTagType::Grandfathered, "grandfathered"},
    SubTagTypeDetails{SubTagType::Redundant, "redundant"},
};

struct SubTagScopeDetails
{
  SubTagScope m_scope;
  std::string_view m_code;
  std::string_view m_description;
};

constexpr auto scopes = std::array{
    SubTagScopeDetails{SubTagScope::Individual, "", "individual"},
    SubTagScopeDetails{SubTagScope::MacroLanguage, "macrolanguage", "macro language"},
    SubTagScopeDetails{SubTagScope::Collection, "collection", "collection"},
    SubTagScopeDetails{SubTagScope::Special, "special", "special"},
    SubTagScopeDetails{SubTagScope::PrivateUse, "private-use", "private use"},
};

SubTagType FindSubTagType(std::string_view str)
{
  const auto it = std::ranges::find(types, str, &SubTagTypeDetails::m_code);
  if (it != types.end())
    return it->m_type;
  else
    return SubTagType::Unknown;
}

std::string_view format_as(SubTagType type)
{
  const auto it = std::ranges::find(types, type, &SubTagTypeDetails::m_type);
  if (it != types.end())
    return it->m_code;
  else
    return "unknown";
}

SubTagScope FindSubTagScope(std::string_view str)
{
  const auto it = std::ranges::find(scopes, str, &SubTagScopeDetails::m_code);
  if (it != scopes.end())
    return it->m_scope;
  else
    return SubTagScope::Unknown;
}

std::string_view format_as(SubTagScope scope)
{
  const auto it = std::ranges::find(scopes, scope, &SubTagScopeDetails::m_scope);
  if (it != scopes.end())
    return it->m_description;
  else
    return "unknown";
}

bool BaseSubTag::Load(const RegistryFileRecord& fileRecord)
{
  return LoadBaseSubTag(*this, fileRecord);
}

bool LanguageSubTag::Load(const RegistryFileRecord& fileRecord)
{
  return LoadLanguageSubTag(*this, fileRecord);
}

bool ExtLangSubTag::Load(const RegistryFileRecord& fileRecord)
{
  return LoadExtLangSubTag(*this, fileRecord);
}

bool VariantSubTag::Load(const RegistryFileRecord& fileRecord)
{
  return LoadVariantSubTag(*this, fileRecord);
}

} // namespace KODI::UTILS::I18N
