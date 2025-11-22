/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47.h"

#include "utils/LangCodeExpander.h" // circular reference bcp47 needs langcodexpander needs bcp47
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47Parser.h"
#include "utils/i18n/Iso3166_1.h"
#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_1.h"
#include "utils/i18n/Iso639_2.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>

using namespace KODI::UTILS::I18N;
using namespace std::literals;

std::optional<CBcp47> CBcp47::ParseTag(std::string str)
{
  ParsedBcp47Tag p = CBcp47Parser::Parse(str);

  if (p.type == Bcp47TagType::MALFORMED)
    return std::nullopt;

  CBcp47 tag;
  tag.m_language = std::move(p.m_language);
  tag.m_extLangs = std::move(p.m_extLangs);
  tag.m_script = std::move(p.m_script);
  tag.m_region = std::move(p.m_region);
  tag.m_variants = std::move(p.m_variants);
  tag.m_extensions = std::move(p.m_extensions);
  tag.m_privateUse = std::move(p.m_privateUse);
  tag.m_grandfathered = std::move(p.m_grandfathered);

  tag.m_isValid = tag.Validate();

  return tag;
}

bool CBcp47::Validate()
{
  // Validity rules from RFC5646:
  // 1) well-formed: always true as the object can only be created from well-formed text tags.
  //    Exception: irregular grandfathered tags do not have to be well formed in order to be valid
  if (!m_grandfathered.empty())
    return true;

  // 2) either the tag is in the list of grandfathered tags or the primary language, extended
  //    language, script, region and variant subtags appear in the IANA registry
  if (!IsValidLanguage() || !IsValidRegion())
    return false;
  //! @todo validate grandfathered tags
  //! @todo validate extended language - count of extlang may not be enough
  if (HasMultipleExtLang())
    return false;
  //! @todo validate script against ISO 15924 - also reserved Qaaa-Qabx
  //! @todo validate variants

  // 3) There are no duplicate variant subtags
  if (HasDuplicateVariants())
    return false;

  // 4) There are no duplicate singleton (extension) subtags
  if (HasDuplicateExtensions())
    return false;

  // Validity within extensions is not considered
  // Validity under older rules (RFC 3066) is not handled

  return true;
}

bool CBcp47::IsValidLanguage() const
{
  // Language subtag, mandatory, validate against ISO 639
  // except if a private use subtag was used.

  if (m_language.empty())
  {
    if (!m_privateUse.empty())
      return true;

    CLog::LogF(LOGDEBUG, "The language subtag is mandatory and cannot be blank.");
    return false;
  }

  //! @todo separate future PR replace with checks against the IANA subtags registry
  if (m_language.length() == 2 && StringUtils::IsAsciiLetters(m_language))
  {
    // ISO 639-1
    auto ret = CIso639_1::LookupByCode(m_language);
    if (!ret)
    {
      CLog::LogF(LOGDEBUG, "{} is not a valid ISO 639-1 code.", m_language);
      return false;
    }
    return true;
  }
  else if (m_language.length() == 3 && StringUtils::IsAsciiLetters(m_language))
  {
    // qaa-qtz range reserved for private use by ISO 639-2 - always accept
    if (m_language >= "qaa" && m_language <= "qtz")
      return true;

    // Try ISO 639-2. ISO 639-2/T or B
    auto ret = CIso639_2::LookupByCode(m_language);
    if (!ret.has_value() && !CIso639_2::BCodeToTCode(StringToLongCode(m_language)).has_value())
    {
      CLog::LogF(LOGDEBUG, "{} is not a valid ISO 639-2 code.", m_language);
      return false;
    }

    // The alpha-3 of languages that have an alpha-2 is not valid.
    {
      std::string dummy;
      // ISO 639-1 is preferred to ISO 639-2/T, which is preferred to ISO 639-2/B
      if (g_LangCodeExpander.ConvertToISO6391(m_language, dummy))
        return false;
    }

    return true;
  }

  CLog::LogF(LOGDEBUG, "{} is not a valid language subtag.", m_language);
  return false;
}

bool CBcp47::HasMultipleExtLang() const
{
  // RFC 5646 2.2.2.4.
  return m_extLangs.size() > 1;
}

bool CBcp47::IsValidRegion() const
{
  // Region subtag is optional
  if (m_region.empty())
    return true;

  // Values reserved for private use
  if (m_region == "aa" || (m_region >= "qm" && m_region <= "qz") ||
      (m_region >= "xa" && m_region <= "xz") || m_region == "zz")
    return true;

  //! @todo separate future PR replace with checks against the IANA subtags registry
  if (m_region.length() == 2 && StringUtils::IsAsciiLetters(m_region))
  {
    // ISO 3166-1
    auto ret = CIso3166_1::ContainsAlpha2(m_region);
    if (!ret)
    {
      CLog::LogF(LOGDEBUG, "{} is not a valid ISO 3166-1 alpha-2 code.", m_region);
      return false;
    }
  }
  else if (!m_region.empty())
  {
    CLog::LogF(LOGDEBUG, "{} is not a valid region.", m_region);
    return false;
  }

  // Region subtag is optional
  return true;
}

namespace
{
template<class T>
static auto reference = [](const T& ptr) { return &ptr; };
template<class T>
static auto dereference = [](const T* ptr) { return *ptr; };
} // namespace

bool CBcp47::HasDuplicateVariants() const
{
  if (m_variants.empty() || m_variants.size() < 2)
    return false;

  std::vector<const std::string*> variants;
  variants.reserve(m_variants.size());

  std::ranges::transform(m_variants, std::back_inserter(variants), reference<std::string>);
  std::ranges::sort(variants, {}, dereference<std::string>);

  return variants.end() != std::ranges::adjacent_find(variants, {}, dereference<std::string>);
}

bool CBcp47::HasDuplicateExtensions() const
{
  if (m_extensions.empty() || m_extensions.size() < 2)
    return false;

  std::vector<char> extensions;
  extensions.reserve(m_extensions.size());

  std::ranges::transform(m_extensions, std::back_inserter(extensions), &Bcp47Extension::name);
  std::ranges::sort(extensions, {}, {});

  return extensions.end() != std::ranges::adjacent_find(extensions, {}, {});
}

std::string CBcp47::Format() const
{
  std::string tag;

  if (!m_grandfathered.empty())
    return m_grandfathered;

  if (!m_language.empty())
  {
    tag = m_language;

    if (!m_extLangs.empty())
    {
      tag.push_back('-');
      tag.append(StringUtils::Join(m_extLangs, "-"));
    }

    if (!m_script.empty())
    {
      tag.push_back('-');
      std::string s = m_script;
      StringUtils::ToCapitalize(s);
      tag.append(s);
    }

    if (!m_region.empty())
    {
      tag.push_back('-');
      tag.append(StringUtils::ToUpper(m_region));
    }

    if (!m_variants.empty())
    {
      tag.push_back('-');
      tag.append(StringUtils::Join(m_variants, "-"));
    }

    if (!m_extensions.empty())
    {
      for (const auto& ext : m_extensions)
      {
        tag.push_back('-');
        tag.push_back(ext.name);
        tag.push_back('-');
        tag.append(StringUtils::Join(ext.segments, "-"));
      }
    }
  }

  if (!m_privateUse.empty())
  {
    if (!tag.empty())
      tag.push_back('-');
    tag.append("x-");
    tag.append(StringUtils::Join(m_privateUse, "-"));
  }
  return tag;
}

void CBcp47::Canonicalize()
{
  //! @todo future PR revisit once subtag registry support is implemented - cannot be done correctly without.
  /*
  // Not a part of the ietf bcp47 canonicalization: the tag may have been created from an ISO 639
  // alpha-3 code, convert to alpha-2 when available.
  if (m_language.length() == 3)
  {
    // ISO 639-1 is preferred to ISO 639-2/T, which is preferred to ISO 639-2/B
    if (!g_LangCodeExpander.ConvertToISO6391(m_language, m_language))
      m_language = g_LangCodeExpander.ConvertToISO6392T(m_language);
  }
  */
  // RFC 5646 - 4.5
  //
  // 1. Sort the extensions alphabetically
  std::ranges::sort(m_extensions, {}, &Bcp47Extension::name);

  //! @todo once registry support is available:
  //! @todo grandfathered tags preferred replacements
  //! @todo subtags replacement with preferred values
  //! @todo extlang replacement for canonical form - recreate extlang for extlang form
  //! @todo reordering of variants using prefixes
  //! @todo replacement of deprecated with preferred
  //! @todo suppress script - not part of official canonicalization, but tags should not use
  //! a script unless it adds information
}

CBcp47 CBcp47::ToAudioLanguageTag()
{
  CBcp47 tag{*this};
  tag.m_script = "";

  return tag;
}
