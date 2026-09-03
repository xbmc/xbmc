/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/LanguageTag.h"

#include "language/i18n/Bcp47.h"
#include "language/i18n/Bcp47Registry/SubTagRegistryManager.h"
#include "language/i18n/Iso639.h"
#include "language/i18n/Iso639_2.h"
#include "language/i18n/LanguageTable.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

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

//! What a POSIX locale name puts before its codeset and its modifier: sr_RS.UTF-8@latin
constexpr char POSIX_CODESET_SEPARATOR{'.'};
constexpr char POSIX_MODIFIER_SEPARATOR{'@'};

//! The script subtag a POSIX locale modifier names, for the modifiers that name one
constexpr auto POSIX_SCRIPT_MODIFIERS = std::array{
    std::pair<std::string_view, std::string_view>{"cyrillic", "Cyrl"},
    std::pair<std::string_view, std::string_view>{"latin", "Latn"},
};

/*!
 * \brief Take the codeset and modifier off a POSIX locale name.
 * \note Only a name shaped like a POSIX locale - one carrying a territory or a modifier - is
 *       read this way, so dotted text that is not a locale is left for the parse to reject.
 * \param[in,out] locale The name, lower case. Left as the language and territory alone.
 * \return The script subtag the modifier names, or empty where there is none or it names none.
 */
std::string TakePosixModifier(std::string& locale)
{
  std::string modifier;
  if (const std::size_t at = locale.find(POSIX_MODIFIER_SEPARATOR); at != std::string::npos)
  {
    modifier = locale.substr(at + 1);
    locale.erase(at);
  }

  if (modifier.empty() && locale.find(POSIX_SUBTAG_SEPARATOR) == std::string::npos)
    return {};

  if (const std::size_t dot = locale.find(POSIX_CODESET_SEPARATOR); dot != std::string::npos)
    locale.erase(dot);

  const auto script = std::ranges::find(POSIX_SCRIPT_MODIFIERS, modifier,
                                        &std::pair<std::string_view, std::string_view>::first);
  return script == POSIX_SCRIPT_MODIFIERS.end() ? std::string{} : std::string{script->second};
}

/*!
 * \brief A tag as the parse produced it: the canonical form, and the subtags read out of it.
 * \note Read out here because this is the only place a tag is taken apart. Everything a tag is
 *       later asked about is answered from these rather than from the text.
 */
struct Parsed
{
  std::string tag;
  std::string language;
  CTerritory territory;
};

//! What a parsed tag says about itself
Parsed Read(const CBcp47& tag)
{
  return {tag.Format(), tag.GetLanguage(), CTerritory::FromCode(tag.GetRegion())};
}

//! A code Kodi holds but BCP 47 does not describe, which names a language and nothing else
Parsed Undescribed(std::string code)
{
  return {code, std::move(code), {}};
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
std::optional<Parsed> CanonicalBcp47(const std::string& text)
{
  std::string code{StringUtils::ToLower(text)};
  StringUtils::Trim(code);

  // A POSIX locale name spells the subtag separator _, as en_GB, and may qualify the language by
  // a script, as sr_RS@latin. Only the parsing sees the BCP 47 form, so text naming no language
  // is answered as written.
  std::string parseCode{code};
  const std::string script{TakePosixModifier(parseCode)};
  std::ranges::replace(parseCode, POSIX_SUBTAG_SEPARATOR, SUBTAG_SEPARATOR);
  if (!script.empty())
  {
    const std::size_t primaryEnd{parseCode.find(SUBTAG_SEPARATOR)};
    parseCode.insert(primaryEnd == std::string::npos ? parseCode.size() : primaryEnd,
                     SUBTAG_SEPARATOR + script);
  }

  auto tag = CBcp47::ParseTag(parseCode);

  if (tag.has_value() && tag->IsValid())
  {
    tag->Canonicalize();
    return Read(*tag);
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
        return Read(*alpha2Tag);

      // Kodi's table and the subtag registry disagree, which is a data bug rather than bad input
      CLog::LogF(LOGERROR,
                 "'{}' has the ISO 639-1 code '{}', which is not a registered BCP 47 language "
                 "subtag",
                 code, *alpha2);
    }
  }

  // A code advancedsettings.xml declares is a language by declaration, whether or not any
  // standard assigns it, and is answered as declared
  if (CLanguageTable::GetInstance().NameOf(parseCode).has_value())
    return Undescribed(std::move(parseCode));

  // Unknown as a code, the text may be an English language name
  if (const auto named = CodeOfName(code); named.has_value())
  {
    // BCP 47 uses the alpha-2 code where the language has one, and the alpha-3 otherwise
    std::string resolved{CIso639::Alpha3ToAlpha2(*named).value_or(*named)};

    // The table holds a declared code as written, so it is spelled out the way any tag is
    if (auto resolvedTag = CBcp47::ParseTag(resolved);
        resolvedTag.has_value() && resolvedTag->IsValid())
    {
      resolvedTag->Canonicalize();
      return Read(*resolvedTag);
    }

    return Undescribed(std::move(resolved));
  }

  return std::nullopt;
}
} // namespace

CLanguageTag CLanguageTag::Undetermined()
{
  return CLanguageTag(std::string{UNDETERMINED}, std::string{UNDETERMINED}, {});
}

CLanguageTag CLanguageTag::English()
{
  return CLanguageTag(std::string{ENGLISH}, std::string{ENGLISH}, {});
}

bool CLanguageTag::IsEnglish() const
{
  return IsLanguage(ENGLISH);
}

bool CLanguageTag::IsLanguage(std::string_view subtag) const
{
  // The language named, not the text the tag starts with
  return StringUtils::EqualsNoCase(m_language, subtag);
}

bool CLanguageTag::IsUndetermined() const
{
  return m_tag.empty() || m_tag == UNDETERMINED;
}

CLanguageTag CLanguageTag::Parse(const std::string& text)
{
  if (text.empty())
    return {};

  if (auto parsed = CanonicalBcp47(text); parsed.has_value())
  {
    return CLanguageTag(std::move(parsed->tag), std::move(parsed->language),
                        std::move(parsed->territory));
  }

  return CLanguageTag(text);
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

  if (m_language.length() == 2)
    return CIso639::Alpha2ToAlpha3B(m_language).value_or(m_language);

  // An alpha-3 subtag is already an ISO 639-2 code wherever that standard assigns one, so only
  // the languages spelling their two forms differently have a mapping to follow
  if (m_language.length() == 3)
    return CIso639_2::TCodeToBCode(m_language).value_or(m_language);

  return m_language;
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

const CTerritory& CLanguageTag::GetTerritory() const
{
  return m_territory;
}

std::string CLanguageTag::AsIso6391() const
{
  // Canonical BCP 47 already prefers the alpha-2 code wherever a language has one
  if (m_language.length() == 2)
    return m_language;

  return CIso639::Alpha3ToAlpha2(m_language).value_or(std::string{});
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
