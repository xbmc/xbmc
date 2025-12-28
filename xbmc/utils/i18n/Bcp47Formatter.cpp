/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Formatter.h"

#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47.h"
#include "utils/i18n/Iso3166_1.h"
#include "utils/i18n/Iso639.h"
#include "utils/i18n/Iso639_1.h"
#include "utils/i18n/Iso639_2.h"

#include <string_view>

using namespace KODI::UTILS::I18N;

namespace
{
bool LookupInISO639Tables(const std::string& code, std::string& desc)
{
  if (code.empty())
    return false;

  std::string sCode(code);
  StringUtils::ToLower(sCode);
  StringUtils::Trim(sCode);

  if (sCode.length() == 2)
  {
    const auto ret = CIso639_1::LookupByCode(StringToLongCode(sCode));
    if (ret.has_value())
    {
      desc = ret.value();
      return true;
    }
  }
  else if (sCode.length() == 3)
  {
    uint32_t longCode = StringToLongCode(sCode);

    // Map B to T for the few codes that have differences
    const auto tCode = CIso639_2::BCodeToTCode(longCode);
    if (tCode.has_value())
      longCode = tCode.value();

    // Lookup the T code
    const auto ret = CIso639_2::LookupByCode(longCode);
    if (ret.has_value())
    {
      desc = ret.value();
      return true;
    }
  }
  return false;
}
} // namespace

std::string CBcp47Formatter::Format(const CBcp47& tag) const
{
  std::string str;

  if (tag.m_type == Bcp47TagType::GRANDFATHERED)
  {
    FormatGrandfathered(tag, str);
    return str;
  }

  // Language may be empty only for tags made up only of a private use subtag
  if (tag.m_type == Bcp47TagType::PRIVATE_USE)
  {
    FormatPrivateUse(tag, str);
    return str;
  }

  if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
  {
    // Format the tag in a style similar to locale names, ex. English (United States)
    FormatLanguage(tag, str);
    std::size_t languageSize = str.size();

    if (languageSize > 0)
      str.append(" (");

    std::size_t othersBegin = str.size();
    constexpr std::string_view sep = ", ";

    if (FormatExtLangs(tag, str))
      str.append(sep);
    if (FormatScript(tag, str))
      str.append(sep);
    if (FormatRegion(tag, str))
      str.append(sep);
    if (FormatVariants(tag, str))
      str.append(sep);
    if (FormatExtensions(tag, str))
      str.append(sep);
    if (FormatPrivateUse(tag, str))
      str.append(sep);

    if (str.size() > othersBegin)
    {
      // remove final space
      str.erase(str.size() - sep.length());
      str.append(")");
    }
    else
    {
      // language was the only subtag - remove additional formatting text
      str.erase(languageSize);
    }
  }
  else
  {
    // Format the tag as a BCP 47 tag with the recommended casing

    str.reserve(35); // size recommended by RFC5646

    if (FormatLanguage(tag, str))
      str.push_back('-');
    if (FormatExtLangs(tag, str))
      str.push_back('-');
    if (FormatScript(tag, str))
      str.push_back('-');
    if (FormatRegion(tag, str))
      str.push_back('-');
    if (FormatVariants(tag, str))
      str.push_back('-');
    if (FormatExtensions(tag, str))
      str.push_back('-');
    if (FormatPrivateUse(tag, str))
      str.push_back('-');

    // remove final -
    if (!str.empty())
      str.pop_back();
  }

  return str;
}

bool CBcp47Formatter::FormatLanguage(const CBcp47& tag, std::string& str) const
{
  if (tag.m_language.empty())
    return false;

  if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
  {
    // Language from ISO 639-1 or ISO 639-2
    if (std::string lang; LookupInISO639Tables(tag.m_language, lang))
      str.append(lang);
    else
      str.append(tag.m_language); // was likely ISO 639-3 or 639-5
    return true;
  }

  str.append(tag.m_language);
  return true;
}

bool CBcp47Formatter::FormatExtLangs(const CBcp47& tag, std::string& str) const
{
  if (tag.m_extLangs.empty())
    return false;

  str.append(StringUtils::Join(tag.m_extLangs, "-"));
  return true;
}

bool CBcp47Formatter::FormatScript(const CBcp47& tag, std::string& str) const
{
  if (tag.m_script.empty())
    return false;

  std::string s = tag.m_script;
  StringUtils::ToCapitalize(s);
  str.append(s);
  return true;
}

bool CBcp47Formatter::FormatRegion(const CBcp47& tag, std::string& str) const
{
  if (tag.m_region.empty())
    return false;

  if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
  {
    // Region from ISO 3166-1. UN M.49 is not supported.
    const auto reg = CIso3166_1::LookupByCode(tag.m_region);
    str.append(reg.value_or(StringUtils::ToUpper(tag.m_region)));
  }
  else
  {
    str.append(StringUtils::ToUpper(tag.m_region));
  }
  return true;
}

bool CBcp47Formatter::FormatVariants(const CBcp47& tag, std::string& str) const
{
  if (tag.m_variants.empty())
    return false;

  str.append(StringUtils::Join(tag.m_variants, "-"));
  return true;
}

bool CBcp47Formatter::FormatExtensions(const CBcp47& tag, std::string& str) const
{
  if (tag.m_extensions.empty())
    return false;

  for (const auto& ext : tag.m_extensions)
  {
    str.push_back(ext.name);
    str.push_back('-');
    str.append(StringUtils::Join(ext.segments, "-"));
    str.push_back('-');
  }
  // remove final -
  str.pop_back();

  return true;
}

bool CBcp47Formatter::FormatPrivateUse(const CBcp47& tag, std::string& str) const
{
  if (tag.m_privateUse.empty())
    return false;

  str.append("x-");
  str.append(StringUtils::Join(tag.m_privateUse, "-"));

  return true;
}

bool CBcp47Formatter::FormatGrandfathered(const CBcp47& tag, std::string& str) const
{
  if (tag.m_grandfathered.empty())
    return false;

  str.append(tag.m_grandfathered);
  return true;
}
