/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LanguageTag.h"

#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_2.h"

#include <string_view>

using namespace KODI::UTILS;
using namespace KODI::UTILS::I18N;

namespace
{
//! The BCP 47 subtag for a language that is present but not known
constexpr std::string_view UNDETERMINED{"und"};
} // namespace

CLanguageTag CLanguageTag::Undetermined()
{
  return CLanguageTag(std::string{UNDETERMINED});
}

bool CLanguageTag::IsUndetermined() const
{
  return m_tag.empty() || m_tag == UNDETERMINED;
}

CLanguageTag CLanguageTag::Parse(const std::string& text)
{
  if (text.empty())
    return {};

  return CLanguageTag(CLangCodeExpander::AsBcp47(text));
}

std::optional<CLanguageTag> CLanguageTag::TryParse(const std::string& text)
{
  if (std::string bcp47; !text.empty() && CLangCodeExpander::ConvertToBcp47(text, bcp47))
    return CLanguageTag(std::move(bcp47));

  return std::nullopt;
}

std::optional<CLanguageTag> CLanguageTag::FindInText(const std::string& text)
{
  const std::size_t begin = text.find('{');
  if (begin == std::string::npos)
    return std::nullopt;

  const std::size_t end = text.find('}', begin + 1);
  if (end == std::string::npos)
    return std::nullopt;

  return TryParse(text.substr(begin + 1, end - begin - 1));
}

std::string CLanguageTag::AsIso6392B() const
{
  if (m_tag.empty())
    return m_tag;

  return CLangCodeExpander::AsISO6392B(m_tag);
}

std::string CLanguageTag::AsIso6392T() const
{
  const std::string iso6392B{AsIso6392B()};

  // A language with no ISO 639-2 code narrows to the tag itself, which is not a code to map
  if (iso6392B.length() != 3)
    return iso6392B;

  // Only the languages whose two forms are spelled differently have a mapping to follow
  if (const auto tCode = CIso639_2::BCodeToTCode(StringToLongCode(iso6392B)); tCode.has_value())
    return LongCodeToString(*tCode);

  return iso6392B;
}

std::string CLanguageTag::AsIso6391() const
{
  // Reduced to the primary language subtag first, as the conversion does not accept a tag
  if (std::string code; CLangCodeExpander::ConvertToISO6391(AsIso6392B(), code))
    return code;

  return {};
}

bool CLanguageTag::Matches(const CLanguageTag& other) const
{
  // Two tags that are already the same name the same language, without any conversion
  if (StringUtils::EqualsNoCase(m_tag, other.m_tag))
    return true;

  // Narrowing drops the subtags, leaving the language each tag names
  return CLangCodeExpander::CompareISO639Codes(AsIso6392B(), other.AsIso6392B());
}

std::string CLanguageTag::GetEnglishName() const
{
  if (std::string name; CLangCodeExpander::Lookup(m_tag, name))
    return name;

  return {};
}

std::string CLanguageTag::GetEnglishLanguageName() const
{
  if (std::string name; CLangCodeExpander::Lookup(AsIso6392B(), name))
    return name;

  return {};
}
