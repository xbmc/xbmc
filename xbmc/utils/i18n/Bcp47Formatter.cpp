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

#include <algorithm>
#include <functional>
#include <string_view>

using namespace KODI::UTILS::I18N;

namespace
{
/*!
 * \brief Return a reference to the shortest string in the provided list. The first match is returned
 *        in case multiple strings have the shortest length.
 * \note The returned reference is valid as long as the list remain in scope of the caller.
 * \param[in] list List of strings
 * \return reference to the shortest string.
 */
std::string_view ShortestDescription(const std::vector<std::string>& list)
{
  if (list.empty())
    return "";

  return *std::ranges::min_element(list, [](const auto& elem1, const auto& elem2)
                                   { return elem1.size() < elem2.size(); });
}

bool AppendRegistryDescSingle(
    const std::optional<TagSubTags>& subTags,
    std::function<std::optional<BaseSubTag>(const TagSubTags& subTag)> member,
    std::string& str)
{
  if (subTags.has_value())
  {
    if (const auto& subTag = member(subTags.value());
        subTag.has_value() && !subTag.value().m_descriptions.empty())
    {
      str.append(ShortestDescription(subTag.value().m_descriptions));
      return true;
    }
  }
  return false;
}

template<class T>
bool AppendRegistryDescVector(const std::optional<TagSubTags>& subTags,
                              std::function<std::vector<T>(const TagSubTags& subTag)> member,
                              const std::string& sep,
                              std::string& str)
{
  if (subTags.has_value())
  {
    if (const auto& tagsVector = member(subTags.value()); !tagsVector.empty())
    {
      // Vector of references OK because subTags has function scope
      std::vector<std::string_view> englishDesc;
      englishDesc.reserve(tagsVector.size());
      std::ranges::for_each(tagsVector,
                            [&englishDesc](const auto& subTag)
                            {
                              auto& newDesc = englishDesc.emplace_back(
                                  ShortestDescription(subTag.m_descriptions));
                              if (newDesc.empty())
                                newDesc = subTag.m_subTag;
                            });
      str.append(StringUtils::Join(englishDesc, sep));

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
    if (tag.m_type == Bcp47TagType::GRANDFATHERED)
    {
      AppendGrandfathered(tag, str);
      return str;
    }

    // Language may be empty only for tags made up only of a private use subtag
    if (tag.m_type == Bcp47TagType::PRIVATE_USE)
    {
      AppendPrivateUse(tag, str);
      return str;
    }
  }

  if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
  {
    // Format the tag in a style similar to locale names, ex. English (United States)
    AppendLanguage(tag, str);
    std::size_t languageSize = str.size();

    if (languageSize > 0)
      str.append(" (");

    std::size_t othersBegin = str.size();
    constexpr std::string_view sep = ", ";

    if (AppendExtLangs(tag, str))
      str.append(sep);
    if (AppendScript(tag, str))
      str.append(sep);
    if (AppendRegion(tag, str))
      str.append(sep);
    if (AppendVariants(tag, str))
      str.append(sep);
    if (AppendExtensions(tag, str))
      str.append(sep);
    if (AppendPrivateUse(tag, str))
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

    if (AppendLanguage(tag, str))
      str.push_back('-');
    if (AppendExtLangs(tag, str))
      str.push_back('-');
    if (AppendScript(tag, str))
      str.push_back('-');
    if (AppendRegion(tag, str))
      str.push_back('-');
    if (AppendVariants(tag, str))
      str.push_back('-');
    if (AppendExtensions(tag, str))
      str.push_back('-');
    if (AppendPrivateUse(tag, str))
      str.push_back('-');

    // remove final -
    if (!str.empty())
      str.pop_back();
  }
  else if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    constexpr std::string_view sep = ", ";

    // Dump as much information of the tag as posible, in raw form
    AppendDebugHeader(tag, str);

    AppendLanguage(tag, str);
    str.append(sep);
    AppendExtLangs(tag, str);
    str.append(sep);
    AppendScript(tag, str);
    str.append(sep);
    AppendRegion(tag, str);
    str.append(sep);
    AppendVariants(tag, str);
    str.append(sep);
    AppendExtensions(tag, str);
    str.append(sep);
    AppendPrivateUse(tag, str);
    str.append(sep);
    AppendGrandfathered(tag, str);

    if (str.back() == ' ')
      str.pop_back();
  }

  return str;
}

bool CBcp47Formatter::AppendLanguage(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("language: ");
    str.append(tag.m_language);
    modified = true;
  }
  else if (const std::string& language = tag.m_language; !language.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
      modified = AppendRegistryDescSingle(tag.m_registrySubTags, &TagSubTags::m_language, str);

    if (!modified)
    {
      str.append(language);
      modified = true;
    }
  }
  return modified;
}

bool CBcp47Formatter::AppendExtLangs(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("ext langs: {");
    str.append(StringUtils::Join(tag.m_extLangs, ", "));
    str.append("}");
    modified = true;
  }
  else if (const std::vector<std::string>& extLangs = tag.m_extLangs; !extLangs.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
    {
      const auto& tags{tag.m_registrySubTags};
      modified = AppendRegistryDescVector<ExtLangSubTag>(tags, &TagSubTags::m_extLangs, " ", str);
    }
    if (!modified)
    {
      str.append(StringUtils::Join(extLangs, "-"));
      modified = true;
    }
  }
  return modified;
}

bool CBcp47Formatter::AppendScript(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("script: ");
    str.append(tag.m_script);
    modified = true;
  }
  else if (const std::string& script = tag.m_script; !script.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
      modified = AppendRegistryDescSingle(tag.m_registrySubTags, &TagSubTags::m_script, str);

    if (!modified)
    {
      std::string s = script;
      StringUtils::ToCapitalize(s);
      str.append(s);
      modified = true;
    }
  }
  return modified;
}

bool CBcp47Formatter::AppendRegion(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("region: ");
    str.append(tag.m_region);
    modified = true;
  }
  else if (const std::string& region = tag.m_region; !region.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
      modified = AppendRegistryDescSingle(tag.m_registrySubTags, &TagSubTags::m_region, str);

    if (!modified)
    {
      str.append(StringUtils::ToUpper(region));
    }
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::AppendVariants(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("variants: {");
    str.append(StringUtils::Join(tag.m_variants, ", "));
    str.append("}");
    modified = true;
  }
  else if (const std::vector<std::string>& variants = tag.m_variants; !variants.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
    {
      const auto& tags{tag.m_registrySubTags};
      modified = AppendRegistryDescVector<VariantSubTag>(tags, &TagSubTags::m_variants, " ", str);
    }
    if (!modified)
    {
      str.append(StringUtils::Join(variants, "-"));
      modified = true;
    }
  }
  return modified;
}

bool CBcp47Formatter::AppendExtensions(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("extensions: {");

    for (const auto& ext : tag.m_extensions)
    {
      str.append("name: ");
      str.push_back(ext.name);
      str.append(" values: {");
      str.append(StringUtils::Join(ext.segments, ", "));
      str.append("} ");
    }
    // remove final space
    if (!tag.m_extensions.empty())
      str.pop_back();

    str.append("}");
    modified = true;
  }
  else if (const std::vector<Bcp47Extension>& extensions = tag.m_extensions; !extensions.empty())
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

bool CBcp47Formatter::AppendPrivateUse(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("private use: {");
    str.append(StringUtils::Join(tag.m_privateUse, ", "));
    str.append("}");
    modified = true;
  }
  else if (const std::vector<std::string>& privateUse = tag.m_privateUse; !privateUse.empty())
  {
    str.append("x-");
    str.append(StringUtils::Join(privateUse, "-"));
    modified = true;
  }
  return modified;
}

bool CBcp47Formatter::AppendGrandfathered(const CBcp47& tag, std::string& str) const
{
  bool modified{false};

  if (m_style == Bcp47FormattingStyle::FORMAT_DEBUG) [[unlikely]]
  {
    str.append("grandfathered: ");
    str.append(tag.m_grandfathered);
    modified = true;
  }
  else if (const std::string& grandfathered = tag.m_grandfathered; !grandfathered.empty())
  {
    if (m_style == Bcp47FormattingStyle::FORMAT_ENGLISH)
      modified = AppendRegistryDescSingle(tag.m_registrySubTags, &TagSubTags::m_grandfathered, str);

    if (!modified)
      str.append(grandfathered);

    modified = true;
  }

  return modified;
}

void CBcp47Formatter::AppendDebugHeader(const CBcp47& tag, std::string& str) const
{
  str.append("BCP47 (");

  if (tag.m_type == Bcp47TagType::WELL_FORMED)
    str.append("well formed, ");
  else if (tag.m_type == Bcp47TagType::GRANDFATHERED)
    str.append("grandfathered, ");
  else if (tag.m_type == Bcp47TagType::PRIVATE_USE)
    str.append("private use, ");

  str.append(tag.IsValid() ? "valid) " : "invalid) ");
}
