/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Registry/RegistryRecordProvider.h"

#include "XBDateTime.h"
#include "utils/StringUtils.h"

#include <ranges>
#include <utility>

#include <fmt/format.h>

using namespace KODI::UTILS::I18N;

RegistryFileRecord::RegistryFileRecord(std::vector<RegistryFileField>&& fields)
{
  for (auto&& field : fields)
    AssignField(std::move(field));
}

void RegistryFileRecord::AssignField(RegistryFileField&& field)
{
  if (field.m_name == "Type")
    m_type = FindSubTagType(field.m_body);
  else if (field.m_name == "Subtag" || field.m_name == "Tag")
    m_subTag = std::move(field.m_body);
  else if (field.m_name == "Description")
    m_descriptions.emplace_back(std::move(field.m_body));
  else if (field.m_name == "Added")
    m_added = std::move(field.m_body);
  else if (field.m_name == "Deprecated")
    m_deprecated = std::move(field.m_body);
  else if (field.m_name == "Preferred-Value")
    m_preferredValue = std::move(field.m_body);
  else if (field.m_name == "Prefix")
    m_prefixes.emplace_back(std::move(field.m_body));
  else if (field.m_name == "Suppress-Script")
    m_suppressScript = std::move(field.m_body);
  else if (field.m_name == "Macrolanguage")
    m_macroLanguage = std::move(field.m_body);
  else if (field.m_name == "Scope")
    m_scope = FindSubTagScope(field.m_body);
  else if (field.m_name == "File-Date")
    m_fileDate = std::move(field.m_body);

  // Unrecognized field name: do nothing, may have been added by future RFC revision
}

bool RegistryFileRecord::IsValidDate(std::string_view date)
{
  // All dates are expected in YYYY-MM-DD RFC3339 full-date format.
  CDateTime dt;
  return dt.SetFromRFC3339FullDate(date);
}

bool RegistryFileRecord::IsValidSubTag()
{
  if (m_type == SubTagType::Unknown)
    return false;

  if (m_scope == SubTagScope::Unknown)
    return false;

  if (!m_added.empty() && !IsValidDate(m_added))
    return false;

  if (!m_deprecated.empty() && !IsValidDate(m_deprecated))
    return false;

  if (!m_fileDate.empty() && !IsValidDate(m_fileDate))
    return false;

  return true;
}

std::string KODI::UTILS::I18N::format_as(const RegistryFileRecord& record)
{
  const std::string descriptions = StringUtils::Join(record.m_descriptions, ", ");
  const std::string prefixes = StringUtils::Join(record.m_prefixes, ", ");

  return fmt::format(
      "Type: {}, Subtag/Tag: {}, Descriptions: {{{}}}, Added: {}, Deprecated: {}, "
      "Preferred-Value: {}, Prefixes: {{{}}}, Suppress-Script: {}, Macrolanguage: {}, Scope: {}, "
      "File-Date: {}",
      record.m_type, record.m_subTag, descriptions, record.m_added, record.m_deprecated,
      record.m_preferredValue, prefixes, record.m_suppressScript, record.m_macroLanguage,
      record.m_scope, record.m_fileDate);
}
