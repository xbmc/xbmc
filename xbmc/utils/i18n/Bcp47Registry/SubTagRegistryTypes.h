/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace KODI::UTILS::I18N
{
struct RegistryFileRecord;

/*!
 * \brief Registry record scopes defined in RFC5646 + special value Unknown
 */
enum SubTagScope
{
  Unknown,
  Individual, //!< Scope not defined for the record.
  MacroLanguage,
  Collection,
  Special,
  PrivateUse,
};

SubTagScope FindSubTagScope(std::string_view str);
/*!
 * \brief fmt/std::format formatter for SubTagScope
 */
std::string_view format_as(SubTagScope scope);

/*!
 * \brief Registry record types defined in RFC5646 + special value Unknown
 */
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

SubTagType FindSubTagType(std::string_view str);
/*!
 * \brief fmt/std::format formatter for SubTagType
 */
std::string_view format_as(SubTagType type);

struct BaseSubTag
{
  std::string m_subTag; //!< Tag / Subtag name
  std::vector<std::string> m_descriptions; //!< List of descriptions
  std::string m_added; //!< Added date
  std::string m_deprecated; //!< Deprecated date
  std::string m_preferredValue; //!< Preferred value

  virtual ~BaseSubTag() {}
  bool operator==(const BaseSubTag&) const = default;
  virtual bool Load(const RegistryFileRecord& fileRecord);
};

/*!
 * \brief Language subtag
 */
struct LanguageSubTag : BaseSubTag
{
  std::string m_suppressScript; //!< Script to suppress
  std::string m_macroLanguage; //!< Macro language - primary language subtag
  SubTagScope m_scope{SubTagScope::Individual};

  bool operator==(const LanguageSubTag&) const = default;
  bool Load(const RegistryFileRecord& fileRecord) override;
};

/*!
 * \brief ExtLang subtag
 */
struct ExtLangSubTag : BaseSubTag
{
  std::string m_prefix; //!< Recommended language prefix
  std::string m_suppressScript; //!< Script to suppress
  std::string m_macroLanguage; //!< Macro language - primary language subtag
  SubTagScope m_scope{SubTagScope::Individual};

  bool operator==(const ExtLangSubTag&) const = default;
  bool Load(const RegistryFileRecord& fileRecord) override;
};

/*!
 * \brief Script subtag
 */
struct ScriptSubTag : BaseSubTag
{
};

/*!
 * \brief Region subtag
 */
struct RegionSubTag : BaseSubTag
{
};

/*!
 * \brief Variant subtag
 */
struct VariantSubTag : BaseSubTag
{
  std::vector<std::string> m_prefixes; //!< Recommended language prefixes

  bool operator==(const VariantSubTag&) const = default;
  bool Load(const RegistryFileRecord& fileRecord) override;
};

/*!
 * \brief Grandfathered tag
 */
struct GrandfatheredTag : BaseSubTag
{
};

/*!
 * \brief Redundant tag
 */
struct RedundantTag : BaseSubTag
{
};
} // namespace KODI::UTILS::I18N
