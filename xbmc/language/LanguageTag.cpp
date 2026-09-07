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
#include <optional>
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

//! The length of an ISO 639-2 code
constexpr std::size_t ISO6392_CODE_LENGTH{3};

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
 * \brief The BCP 47 spelling of text that may be a POSIX locale name.
 *
 * A POSIX locale name spells the subtag separator _, as en_GB, and may qualify the language by
 * a script, as sr_RS@latin. Only the parse sees this form, so text naming no language is still
 * answered as it was written.
 *
 * \param[in] code The text, lower case and trimmed.
 * \return The text with BCP 47 separators, and the script the modifier named in its place.
 */
std::string AsBcp47Spelling(const std::string& code)
{
  std::string spelling{code};
  const std::string script{TakePosixModifier(spelling)};
  std::ranges::replace(spelling, POSIX_SUBTAG_SEPARATOR, SUBTAG_SEPARATOR);
  if (!script.empty())
  {
    const std::size_t primaryEnd{spelling.find(SUBTAG_SEPARATOR)};
    spelling.insert(primaryEnd == std::string::npos ? spelling.size() : primaryEnd,
                    SUBTAG_SEPARATOR + script);
  }

  return spelling;
}

/*!
 * \brief A tag as the parse produced it: the canonical form, and where its parts are in it.
 * \note Read out here because this is the only place a tag is taken apart. Everything a tag is
 *       later asked about is answered from these rather than from the text.
 */
struct Parsed
{
  std::string tag;
  std::size_t languageLength{0};
  std::size_t regionOffset{0};
  std::size_t regionLength{0};
};

/*!
 * \brief Where a subtag sits in a canonical tag.
 * \param[in] tag The canonical tag.
 * \param[in] subtag The subtag, in whichever case the parse holds it.
 * \return The offset of the subtag past the primary language, or npos where it is not there.
 */
std::size_t FindSubtag(const std::string& tag, const std::string& subtag)
{
  std::size_t begin{tag.find(SUBTAG_SEPARATOR)};
  while (begin != std::string::npos)
  {
    ++begin;
    const std::size_t end{tag.find(SUBTAG_SEPARATOR, begin)};
    const std::size_t length{(end == std::string::npos ? tag.size() : end) - begin};
    if (StringUtils::EqualsNoCase(std::string_view{tag}.substr(begin, length), subtag))
      return begin;

    begin = end;
  }

  return std::string::npos;
}

//! What a parsed tag says about itself
Parsed Read(const CBcp47& tag)
{
  Parsed parsed{.tag = tag.Format()};

  // Canonical BCP 47 leads with the primary language subtag, so its length places it
  if (StringUtils::StartsWith(parsed.tag, tag.GetLanguage()))
    parsed.languageLength = tag.GetLanguage().size();

  if (const std::string& region = tag.GetRegion(); !region.empty())
  {
    if (const std::size_t at = FindSubtag(parsed.tag, region); at != std::string::npos)
    {
      parsed.regionOffset = at;
      parsed.regionLength = region.size();
    }
  }

  return parsed;
}

//! A code Kodi holds but BCP 47 does not describe, which names a language and nothing else
Parsed Undescribed(std::string code)
{
  const std::size_t length{code.size()};
  return {.tag = std::move(code), .languageLength = length};
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

/*
 * The attempts a parse makes, in the order CanonicalBcp47 tries them. Each answers for one
 * notation and nothing for any other.
 */

//! A registered BCP 47 tag, in its canonical form
std::optional<Parsed> AsRegisteredTag(const std::string& code)
{
  auto tag = CBcp47::ParseTag(code);
  if (!tag.has_value() || !tag->IsValid())
    return std::nullopt;

  tag->Canonicalize();
  return Read(*tag);
}

/*!
 * \brief An ISO 639-2 code, answered by the tag of its alpha-2 sibling.
 * \note BCP 47 registers only the shortest code a language has, so an alpha-3 code for a
 *       language that has an alpha-2 one is never a registered subtag. The table says which
 *       alpha-2 code that is, and the tag is parsed once from it. A language with no alpha-2
 *       code is registered under its alpha-3 one and is left for AsRegisteredTag.
 */
std::optional<Parsed> AsIso6392Code(const std::string& code)
{
  if (code.size() != ISO6392_CODE_LENGTH)
    return std::nullopt;

  const auto alpha2 = CIso639::Alpha3ToAlpha2(code);
  if (!alpha2.has_value())
    return std::nullopt;

  if (const auto parsed = AsRegisteredTag(*alpha2); parsed.has_value())
    return parsed;

  // Kodi's table and the subtag registry disagree, which is a data bug rather than bad input
  CLog::LogF(LOGERROR,
             "'{}' has the ISO 639-1 code '{}', which is not a registered BCP 47 language subtag",
             code, *alpha2);
  return std::nullopt;
}

//! A code advancedsettings.xml declares, which is a language by declaration whether or not any
//! standard assigns it
std::optional<Parsed> AsDeclaredCode(const std::string& code)
{
  if (!CLanguageTable::GetInstance().NameOf(code).has_value())
    return std::nullopt;

  return Undescribed(code);
}

//! A language written as a code in any notation Kodi recognizes
std::optional<Parsed> FromCode(const std::string& code)
{
  if (auto parsed = AsIso6392Code(code); parsed.has_value())
    return parsed;

  if (auto parsed = AsRegisteredTag(code); parsed.has_value())
    return parsed;

  return AsDeclaredCode(code);
}

//! A language written as an English name
std::optional<Parsed> AsEnglishName(const std::string& text)
{
  const auto named = CodeOfName(text);
  if (!named.has_value())
    return std::nullopt;

  if (auto parsed = FromCode(*named); parsed.has_value())
    return parsed;

  // The table holds a declared code as written
  return Undescribed(*named);
}

//! The canonical BCP 47 form of a language written in any notation Kodi recognizes
std::optional<Parsed> CanonicalBcp47(const std::string& text)
{
  std::string code{StringUtils::ToLower(text)};
  StringUtils::Trim(code);

  if (auto parsed = FromCode(AsBcp47Spelling(code)); parsed.has_value())
    return parsed;

  return AsEnglishName(code);
}
} // namespace

CLanguageTag CLanguageTag::Undetermined()
{
  return CLanguageTag(std::string{UNDETERMINED}, UNDETERMINED.size(), 0, 0);
}

CLanguageTag CLanguageTag::English()
{
  return CLanguageTag(std::string{ENGLISH}, ENGLISH.size(), 0, 0);
}

bool CLanguageTag::IsEnglish() const
{
  return IsLanguage(ENGLISH);
}

bool CLanguageTag::IsLanguage(std::string_view subtag) const
{
  // The language named, not the text the tag starts with
  return StringUtils::EqualsNoCase(Language(), subtag);
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
    return CLanguageTag(std::move(parsed->tag), parsed->languageLength, parsed->regionOffset,
                        parsed->regionLength);
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

  const std::string_view language{Language()};

  if (language.length() == 2)
    return CIso639::Alpha2ToAlpha3B(language).value_or(std::string{language});

  // An alpha-3 subtag is already an ISO 639-2 code wherever that standard assigns one, so only
  // the languages spelling their two forms differently have a mapping to follow
  if (language.length() == ISO6392_CODE_LENGTH)
    return CIso639_2::TCodeToBCode(language).value_or(std::string{language});

  return std::string{language};
}

std::string CLanguageTag::AsIso6392T() const
{
  const std::string iso6392B{AsIso6392B()};

  // A language with no ISO 639-2 code narrows to the tag itself, which is not a code to map
  if (iso6392B.length() != ISO6392_CODE_LENGTH)
    return iso6392B;

  // Only the languages whose two forms are spelled differently have a mapping to follow
  if (const auto tCode = CIso639_2::BCodeToTCode(StringToLongCode(iso6392B)); tCode.has_value())
    return LongCodeToString(*tCode);

  return iso6392B;
}

CTerritory CLanguageTag::GetTerritory() const
{
  // The region subtag in a canonical tag is spelled the way CTerritory holds it
  return m_regionLength == 0 ? CTerritory{} : CTerritory{std::string{Region()}};
}

std::string CLanguageTag::AsIso6391() const
{
  const std::string_view language{Language()};

  // Canonical BCP 47 already prefers the alpha-2 code wherever a language has one
  if (language.length() == 2)
    return std::string{language};

  return CIso639::Alpha3ToAlpha2(language).value_or(std::string{});
}

bool CLanguageTag::Matches(const CLanguageTag& other) const
{
  // Two tags that are already the same name the same language, which is the only way text that
  // named none can match
  if (StringUtils::EqualsNoCase(m_tag, other.m_tag))
    return true;

  // The primary subtag is the language each tag names, with everything qualifying it dropped
  return m_valid && other.m_valid && Language() == other.Language();
}

std::string CLanguageTag::ToEnglishName() const
{
  return EnglishNameOf(m_tag);
}

std::string CLanguageTag::ToEnglishLanguageName() const
{
  return EnglishNameOf(AsIso6392B());
}
