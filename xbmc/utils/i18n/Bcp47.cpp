/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47.h"

#include "ServiceBroker.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47Formatter.h"
#include "utils/i18n/Bcp47Parser.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryManager.h"

#include <algorithm>

using namespace KODI::UTILS::I18N;
using namespace std::literals;

bool CBcp47::operator==(const ParsedBcp47Tag& other) const
{
  return other.m_type == m_type && other.m_language == m_language &&
         other.m_extLangs == m_extLangs && other.m_script == m_script &&
         other.m_region == m_region && other.m_variants == m_variants &&
         other.m_extensions == m_extensions && other.m_privateUse == m_privateUse &&
         other.m_grandfathered == m_grandfathered;
}

std::optional<CBcp47> CBcp47::ParseTag(std::string str, const CSubTagRegistryManager* registry)
{
  auto p = CBcp47Parser::Parse(str);

  if (!p.has_value())
    return std::nullopt;

  if (registry == nullptr)
    registry = &CServiceBroker::GetSubTagRegistry();

  CBcp47 tag;
  tag.m_registry = registry;
  tag.m_type = p->m_type;
  tag.m_language = std::move(p->m_language);
  tag.m_extLangs = std::move(p->m_extLangs);
  tag.m_script = std::move(p->m_script);
  tag.m_region = std::move(p->m_region);
  tag.m_variants = std::move(p->m_variants);
  tag.m_extensions = std::move(p->m_extensions);
  tag.m_privateUse = std::move(p->m_privateUse);
  tag.m_grandfathered = std::move(p->m_grandfathered);

  tag.m_isValid = tag.Validate(registry);

  return tag;
}

bool CBcp47::Validate(const CSubTagRegistryManager* registry)
{
  // Validity rules per RFC 5646 2.2.9:
  // 1) well-formed: always true as the object can only be created from well-formed text tags.
  //    Exception: irregular grandfathered tags do not have to be well formed in order to be valid
  //    Other special case: A tag may consist entirely of private use subtags (2.2.7.4)

  // There is nothing to lookup in the registry for a private-use only tag
  if (m_type == Bcp47TagType::PRIVATE_USE)
    return true;

  LoadRegistrySubTags(registry);

  if (m_type == Bcp47TagType::GRANDFATHERED)
    return true;

  // 2) either the tag is in the list of grandfathered tags or the primary language, extended
  //    language, script, region and variant subtags appear in the IANA registry as of the
  //    particular date.
  if (!IsValidLanguage() || !IsValidExtLang() || !IsValidScript() || !IsValidRegion() ||
      !IsValidVariants())
    return false;

  // 3) There are no duplicate variant subtags.
  if (HasDuplicateVariants())
    return false;

  // 4) There are no duplicate singleton (extension) subtags.
  if (HasDuplicateExtensions())
    return false;

  // note: validity of the extension subtags is not considered
  // note: validity under older rules (RFC 3066) is not handled

  return true;
}

bool CBcp47::IsValidLanguage() const
{
  // The language subtag is mandatory.
  if (m_language.empty())
    return false;

  // qaa-qtz range reserved for private use by ISO 639-2 - always accept
  //! @todo is it possible to retrieve the range from the registry?
  if (m_language.length() == 3 && m_language >= "qaa" && m_language <= "qtz")
    return true;

  // Conversion probes deliberately submit ISO 639-2/B codes expecting the registry to reject
  // them, so an unregistered subtag is an ordinary answer rather than an error worth logging
  return m_registrySubTags.has_value() && m_registrySubTags.value().m_language.has_value();
}

bool CBcp47::IsValidExtLang() const
{
  // The extended Language subtag is optional.
  if (m_extLangs.empty())
    return true;

  // A tag with multiple extlang subtags cannot be valid per RFC 5646 2.2.2.4.
  if (m_extLangs.size() > 1)
    return false;

  return m_registrySubTags.has_value() && !m_registrySubTags.value().m_extLangs.empty();
}

bool CBcp47::IsValidScript() const
{
  // The region subtag is optional
  if (m_script.empty())
    return true;

  // Values reserved for private use
  //! @todo is it possible to retrieve the range from the registry?
  if (m_script >= "qaaa" && m_script <= "qabx")
    return true;

  return m_registrySubTags.has_value() && m_registrySubTags.value().m_script.has_value();
}

bool CBcp47::IsValidRegion() const
{
  // The region subtag is optional
  if (m_region.empty())
    return true;

  // ISO 3166-1 codes reserved for private use
  // note RFC 5646 doesn't mention the UN M.49 private use codes range 900-999
  //! @todo is it possible to retrieve the range from the registry?
  if (m_region == "aa" || (m_region >= "qm" && m_region <= "qz") ||
      (m_region >= "xa" && m_region <= "xz") || m_region == "zz")
    return true;

  return m_registrySubTags.has_value() && m_registrySubTags.value().m_region.has_value();
}

bool CBcp47::HasDuplicateVariants() const
{
  if (m_variants.empty() || m_variants.size() < 2)
    return false;

  // Make a copy of pointers to the strings to avoid unnecessary copies of the strings to
  // efficiently find duplicates in a C++ idiomatic way.
  std::vector<const std::string*> variants;
  variants.reserve(m_variants.size());

  auto reference = [](const std::string& s) { return &s; };
  auto dereference = [](const std::string* ptr) { return *ptr; };

  std::ranges::transform(m_variants, std::back_inserter(variants), reference);
  std::ranges::sort(variants, {}, dereference);

  return variants.end() != std::ranges::adjacent_find(variants, {}, dereference);
}

bool CBcp47::IsValidVariants() const
{
  // Variant subtags are optional
  if (m_variants.empty())
    return true;

  // No need to check one by one - they were all found in the registry if the count of parsed
  // variants is equal to the count of variants retrieved from the registry.
  if (m_registrySubTags.has_value() &&
      m_variants.size() == m_registrySubTags.value().m_variants.size())
    return true;

  return false;
}

bool CBcp47::HasDuplicateExtensions() const
{
  if (m_extensions.empty() || m_extensions.size() < 2)
    return false;

  // Copy only the field of the extension relevant to the identification of duplicates
  // char copy is cheap and efficient.
  std::vector<char> extensions;
  extensions.reserve(m_extensions.size());

  std::ranges::transform(m_extensions, std::back_inserter(extensions), &Bcp47Extension::name);
  std::ranges::sort(extensions);

  return extensions.end() != std::ranges::adjacent_find(extensions);
}

std::string CBcp47::Format(Bcp47FormattingStyle style) const
{
  return CBcp47Formatter::Format(*this, style);
}

void CBcp47::Canonicalize()
{
  // RFC 5646 - 4.5, whose steps are applied in order
  //
  // 1. Sort the extensions alphabetically
  std::ranges::sort(m_extensions, {}, &Bcp47Extension::name);

  //! @todo 2. Replace a redundant or grandfathered tag by its preferred value - that value is an
  //! extended language range and can span several subtags, so it has to be parsed rather than
  //! substituted

  // 3. Replace a subtag by its preferred value. Language and region only for now, which the RFC
  //    notes covers renamed countries and clerical corrections to ISO 639-1. A subtag deprecated
  //    without a preferred value is already canonical and stays. The registry spells a preferred
  //    value as its own record does, so a region arrives upper case while the tag holds subtags
  //    in lower case.
  bool replaced{false};

  if (m_registrySubTags.has_value())
  {
    const TagSubTags& subTags = m_registrySubTags.value();

    if (subTags.m_language.has_value() && !subTags.m_language->m_preferredValue.empty())
    {
      m_language = StringUtils::ToLower(subTags.m_language->m_preferredValue);
      replaced = true;
    }

    if (subTags.m_region.has_value() && !subTags.m_region->m_preferredValue.empty())
    {
      m_region = StringUtils::ToLower(subTags.m_region->m_preferredValue);
      replaced = true;
    }
  }

  // The cached records still describe the subtags that were dropped, so formatting by description
  // would name what the replacement retired.
  if (replaced)
  {
    m_registrySubTags.reset();
    LoadRegistrySubTags(m_registry);
  }

  //! @todo the rest of step 3 - extlang and variant preferred values. An extlang also replaces
  //! the primary language subtag, and the canonical form carries no extlang at all, so this is
  //! also where the extlang form would be recreated
  //! @todo what 4.5 leaves optional - reordering variants by the 4.1 recommendations, and
  //! regularizing subtag case - plus suppressing a script that adds no information, which the
  //! RFC does not count as canonicalization at all
}

void CBcp47::LoadRegistrySubTags(const CSubTagRegistryManager* registry)
{
  // Skip when already populated
  if (m_registrySubTags.has_value())
    return;

  TagSubTags subTags;

  if (registry != nullptr)
  {
    if (m_type == Bcp47TagType::GRANDFATHERED)
    {
      subTags.m_grandfathered = registry->GetGrandfatheredTags().Lookup(m_grandfathered);
    }
    else
    {
      subTags.m_language = registry->GetLanguageSubTags().Lookup(m_language);

      std::ranges::for_each(m_extLangs,
                            [&registry, &subTags](const std::string& extLang)
                            {
                              auto subTag = registry->GetExtLangSubTags().Lookup(extLang);
                              if (subTag.has_value())
                                subTags.m_extLangs.push_back(std::move(*subTag));
                            });

      subTags.m_script = registry->GetScriptSubTags().Lookup(m_script);
      subTags.m_region = registry->GetRegionSubTags().Lookup(m_region);

      std::ranges::for_each(m_variants,
                            [&registry, &subTags](const std::string& variant)
                            {
                              auto subTag = registry->GetVariantSubTags().Lookup(variant);
                              if (subTag.has_value())
                                subTags.m_variants.push_back(std::move(*subTag));
                            });
    }
  }
  // need to check errors before assigning or not relevant?
  m_registrySubTags = std::move(subTags);
  //else m_registrySubTags.reset()
}
