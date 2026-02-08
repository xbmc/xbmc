/*
 *  Copyright (C) 2025-2026 Team Kodi
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

  if (m_style != Bcp47FormattingStyle::FORMAT_DEBUG)
  {
    // Shortened formats for grandfathered and private use
    // except for debug format, which always prints everything
    if (tag.GetType() == Bcp47TagType::GRANDFATHERED)
    {
      FormatGrandfathered(tag, str);
      return str;
    }

    // Language may be empty only for tags made up only of a private use subtag
    if (tag.GetType() == Bcp47TagType::PRIVATE_USE)
    {
      FormatPrivateUse(tag, str);
      return str;
    }
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
  else if (m_style == Bcp47FormattingStyle::FORMAT_BCP47)
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
  else if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    constexpr std::string_view sep = ", ";

    // Dump as much information of the tag as posible, in raw form
    FormatDebugHeader(tag, str);

    FormatLanguage(tag, str);
    str.append(sep);
    FormatExtLangs(tag, str);
    str.append(sep);
    FormatScript(tag, str);
    str.append(sep);
    FormatRegion(tag, str);
    str.append(sep);
    FormatVariants(tag, str);
    str.append(sep);
    FormatExtensions(tag, str);
    str.append(sep);
    FormatPrivateUse(tag, str);
    str.append(sep);
    FormatGrandfathered(tag, str);

    if (str.back() == ' ')
      str.pop_back();
  }

  return str;
}

bool CBcp47Formatter::FormatLanguage(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("language: ");
    str.append(tag.GetLanguage());
    modified = true;
  }
  else if (const std::string& language = tag.GetLanguage(); !language.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
    {
      // Language from ISO 639-1 or ISO 639-2
      if (std::string lang; LookupInISO639Tables(language, lang))
        str.append(lang);
      else
        str.append(language); // was likely ISO 639-3 or 639-5
    }
    else
    {
      str.append(language);
    }
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::FormatExtLangs(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("ext langs: {");
    str.append(StringUtils::Join(tag.GetExtLangs(), ", "));
    str.append("}");
    modified = true;
  }
  else if (const std::vector<std::string>& extLangs = tag.GetExtLangs(); !extLangs.empty())
  {
    str.append(StringUtils::Join(extLangs, "-"));
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::FormatScript(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("script: ");
    str.append(tag.GetScript());
    modified = true;
  }
  else if (const std::string& script = tag.GetScript(); !script.empty())
  {
    std::string s = script;
    StringUtils::ToCapitalize(s);
    str.append(s);
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::FormatRegion(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("region: ");
    str.append(tag.GetRegion());
    modified = true;
  }
  else if (const std::string& region = tag.GetRegion(); !region.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
    {
      // Region from ISO 3166-1. UN M.49 is not supported.
      const auto reg = CIso3166_1::LookupByCode(region);
      str.append(reg.value_or(StringUtils::ToUpper(region)));
    }
    else
    {
      str.append(StringUtils::ToUpper(region));
    }
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::FormatVariants(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("variants: {");
    str.append(StringUtils::Join(tag.GetVariants(), ", "));
    str.append("}");
    modified = true;
  }
  else if (const std::vector<std::string>& variants = tag.GetVariants(); !variants.empty())
  {
    str.append(StringUtils::Join(variants, "-"));
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::FormatExtensions(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("extensions: {");

    for (const auto& ext : tag.GetExtensions())
    {
      str.append("name: ");
      str.push_back(ext.name);
      str.append(" values: {");
      str.append(StringUtils::Join(ext.segments, ", "));
      str.append("} ");
    }
    // remove final space
    if (!tag.GetExtensions().empty())
      str.pop_back();

    str.append("}");
    modified = true;
  }
  else if (const std::vector<Bcp47Extension>& extensions = tag.GetExtensions(); !extensions.empty())
  {
    for (const auto& ext : extensions)
    {
      str.push_back(ext.name);
      str.push_back('-');
      str.append(StringUtils::Join(ext.segments, "-"));
      str.push_back('-');
    }
    // remove final -
    str.pop_back();
    modified = true;
  }

  return modified;
}

bool CBcp47Formatter::FormatPrivateUse(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("private use: {");
    str.append(StringUtils::Join(tag.GetPrivateUse(), ", "));
    str.append("}");
    modified = true;
  }
  else if (const std::vector<std::string>& privateUse = tag.GetPrivateUse(); !privateUse.empty())
  {
    str.append("x-");
    str.append(StringUtils::Join(privateUse, "-"));
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::FormatGrandfathered(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("grandfathered: ");
    str.append(tag.GetGrandfathered());
    modified = true;
  }
  else if (const std::string& grandfathered = tag.GetGrandfathered(); !grandfathered.empty())
  {
    str.append(grandfathered);
    modified = true;
  }

  return modified;
}

void CBcp47Formatter::FormatDebugHeader(const CBcp47& tag, std::string& str) const
{
  str.append("BCP47 (");

  if (tag.GetType() == Bcp47TagType::WELL_FORMED)
    str.append("well formed, ");
  else if (tag.GetType() == Bcp47TagType::GRANDFATHERED)
    str.append("grandfathered, ");
  else if (tag.GetType() == Bcp47TagType::PRIVATE_USE)
    str.append("private use, ");

  str.append(tag.IsValid() ? "valid) " : "invalid) ");
}
