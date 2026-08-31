/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/LanguageTag.h"

#include "utils/StringUtils.h"
#include "language/i18n/Bcp47.h"
#include "language/i18n/Bcp47Registry/SubTagRegistryManager.h"
#include "language/i18n/Iso639.h"
#include "language/i18n/Iso639_2.h"
#include "language/i18n/LanguageTable.h"
#include "utils/log.h"

#include <algorithm>
#include <string_view>
#include <vector>

using namespace KODI::LANGUAGE;
using namespace KODI::LANGUAGE::I18N;

namespace
{
//! The BCP 47 subtag for a language that is present but not known
constexpr std::string_view UNDETERMINED{"und"};

//! The BCP 47 primary subtag for English
constexpr std::string_view ENGLISH{"en"};

//! Past this a composed name is cut short and the tag appended, so a list stays readable
constexpr std::size_t MAX_COMPOSED_NAME_LENGTH = 30;
static_assert(MAX_COMPOSED_NAME_LENGTH > 3);

//! The separator between a tag's subtags
constexpr char SUBTAG_SEPARATOR{'-'};

//! The subtag separator a POSIX locale name uses
constexpr char POSIX_SUBTAG_SEPARATOR{'_'};

//! Every subtag of a tag, in the order BCP 47 gives them
std::vector<std::string> Subtags(const std::string& tag)
{
  return StringUtils::Split(tag, SUBTAG_SEPARATOR);
}

//! The primary language subtag, which is everything a canonical tag has before its first subtag
std::string PrimarySubtag(const std::string& tag)
{
  return tag.substr(0, tag.find(SUBTAG_SEPARATOR));
}

//! The English name of whatever a code names
std::string EnglishNameOf(const std::string& code)
{
  if (const auto known = CLanguageTable::GetInstance().NameOf(code); known.has_value())
    return *known;

  // A tag Kodi has no single name for is described by its own subtags, which composes a name
  // rather than looking one up
  const auto tag = CBcp47::ParseTag(code);
  if (!tag.has_value())
    return {};

  std::string name{tag->Format(Bcp47FormattingStyle::FORMAT_ENGLISH)};
  if (name.size() > MAX_COMPOSED_NAME_LENGTH)
  {
    name.resize(MAX_COMPOSED_NAME_LENGTH - 3);
    name.append("... [");
    name.append(tag->Format(Bcp47FormattingStyle::FORMAT_BCP47));
    name.push_back(']');
  }

  return name;
}

//! The code of a language given by an English name, in whichever notation records that name
std::optional<std::string> CodeOfName(const std::string& name)
{
  if (name.empty())
    return std::nullopt;

  std::string text{name};
  StringUtils::Trim(text);

  if (const auto known = CLanguageTable::GetInstance().CodeOf(text); known.has_value())
    return known;

  const CSubTagRegistryManager& registry{CSubTagRegistryManager::GetInstance()};
  if (const auto subTag = registry.GetLanguageSubTags().LookupByDescription(text);
      subTag.has_value())
    return subTag->m_subTag;

  return std::nullopt;
}

//! The canonical BCP 47 form of a language written in any notation Kodi recognizes
std::optional<std::string> CanonicalBcp47(const std::string& text)
{
  std::string code{StringUtils::ToLower(text)};
  StringUtils::Trim(code);

  // A POSIX locale name spells the subtag separator _, as en_GB. Only the parsing sees it as -,
  // so text naming no language is answered as written.
  std::string parseCode{code};
  std::ranges::replace(parseCode, POSIX_SUBTAG_SEPARATOR, SUBTAG_SEPARATOR);

  auto tag = CBcp47::ParseTag(parseCode);

  if (tag.has_value() && tag->IsValid())
  {
    tag->Canonicalize();
    return tag->Format();
  }

  // Well formed but not registered is how an ISO 639-2/B code parses. Its alpha-2 sibling is what
  // BCP 47 registers where the language has one.
  if (tag.has_value())
  {
    if (const auto alpha2 = CIso639::Alpha3ToAlpha2(parseCode); alpha2.has_value())
    {
      // Alpha-2 codes are likely to be registered but there is no guarantee
      if (const auto alpha2Tag = CBcp47::ParseTag(*alpha2);
          alpha2Tag.has_value() && alpha2Tag->IsValid())
        return alpha2Tag->Format();

      // Kodi's table and the subtag registry disagree, which is a data bug rather than bad input
      CLog::LogF(LOGERROR,
                 "'{}' has the ISO 639-1 code '{}', which is not a registered BCP 47 language "
                 "subtag",
                 code, *alpha2);
    }
  }

  // Unknown as a code, the text may be an English language name
  if (const auto named = CodeOfName(code); named.has_value())
  {
    // BCP 47 uses the alpha-2 code where the language has one, and the alpha-3 otherwise
    return CIso639::Alpha3ToAlpha2(*named).value_or(*named);
  }

  return std::nullopt;
}
} // namespace

CLanguageTag CLanguageTag::Undetermined()
{
  return CLanguageTag(std::string{UNDETERMINED}, true);
}

CLanguageTag CLanguageTag::English()
{
  return CLanguageTag(std::string{ENGLISH}, true);
}

bool CLanguageTag::IsEnglish() const
{
  return IsLanguage(ENGLISH);
}

bool CLanguageTag::IsLanguage(std::string_view subtag) const
{
  // The language named, not the text the tag starts with
  return StringUtils::EqualsNoCase(PrimarySubtag(m_tag), subtag);
}

bool CLanguageTag::IsUndetermined() const
{
  return m_tag.empty() || m_tag == UNDETERMINED;
}

CLanguageTag CLanguageTag::Parse(const std::string& text)
{
  if (text.empty())
    return {};

  if (auto bcp47 = CanonicalBcp47(text); bcp47.has_value())
    return CLanguageTag(std::move(*bcp47), true);

  return CLanguageTag(text, false);
}

CLanguageTag CLanguageTag::ParseStreamLanguage(const std::string& text)
{
  // A stream that declares nothing is not a stream whose declaration could not be read
  if (text.empty())
    return {};

  CLanguageTag tag{Parse(text)};
  if (!tag.IsValid())
  {
    CLog::LogF(LOGDEBUG, "stream declares a language of '{}', which names none - taking it as {}",
               text, UNDETERMINED);
    return Undetermined();
  }

  return tag;
}

std::optional<CLanguageTag> CLanguageTag::TryParse(const std::string& text)
{
  if (CLanguageTag tag{Parse(text)}; tag.IsValid())
    return tag;

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
  // Text that names no language has no code to narrow to, and is answered as it was given
  if (!m_valid)
    return m_tag;

  const std::string language{PrimarySubtag(m_tag)};

  if (language.length() == 2)
    return CIso639::Alpha2ToAlpha3B(language).value_or(language);

  // An alpha-3 subtag is already an ISO 639-2 code wherever that standard assigns one, so only
  // the languages spelling their two forms differently have a mapping to follow
  if (language.length() == 3)
    return CIso639_2::TCodeToBCode(language).value_or(language);

  return language;
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

CTerritory CLanguageTag::GetTerritory() const
{
  // Text that names no language names no territory either
  if (!m_valid)
    return {};

  const std::vector<std::string> subtags{Subtags(m_tag)};

  // The tag is canonical, so a region subtag is already spelled the way ISO 3166-1 and UN M.49
  // publish theirs, and no other subtag can take either shape: an extlang is three letters, a
  // script is four, and a variant is at least four
  for (std::size_t i{1}; i < subtags.size(); ++i)
  {
    const std::string& subtag{subtags[i]};

    // A singleton opens an extension or the private use sequence, past which nothing is a region
    if (subtag.length() == 1)
      break;

    // Judged when the tag was parsed, so it is handed over rather than judged again
    if (subtag.length() == 2 &&
        std::ranges::all_of(subtag, [](char c) { return c >= 'A' && c <= 'Z'; }))
      return CTerritory(subtag);

    if (subtag.length() == 3 &&
        std::ranges::all_of(subtag, [](char c) { return c >= '0' && c <= '9'; }))
      return CTerritory(subtag);
  }

  return {};
}

std::string CLanguageTag::AsIso6391() const
{
  if (!m_valid)
    return {};

  const std::string language{PrimarySubtag(m_tag)};

  // Canonical BCP 47 already prefers the alpha-2 code wherever a language has one
  if (language.length() == 2)
    return language;

  return CIso639::Alpha3ToAlpha2(language).value_or(std::string{});
}

bool CLanguageTag::Matches(const CLanguageTag& other) const
{
  // Two tags that are already the same name the same language, without any conversion
  if (StringUtils::EqualsNoCase(m_tag, other.m_tag))
    return true;

  // Narrowing drops the subtags, leaving the language each tag names
  return StringUtils::EqualsNoCase(AsIso6392B(), other.AsIso6392B());
}

std::string CLanguageTag::ToEnglishName() const
{
  return EnglishNameOf(m_tag);
}

std::string CLanguageTag::ToEnglishLanguageName() const
{
  return EnglishNameOf(AsIso6392B());
}
