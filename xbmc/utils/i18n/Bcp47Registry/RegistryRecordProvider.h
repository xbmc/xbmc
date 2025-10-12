/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Bcp47Registry/SubTagRegistryTypes.h"

#include <string>
#include <vector>

namespace KODI::UTILS::I18N
{
struct RegistryFileField
{
  std::string m_name;
  std::string m_body;

  RegistryFileField(std::string&& name, std::string&& body)
    : m_name(std::move(name)),
      m_body(std::move(body))
  {
  }
};

struct RegistryFileRecord
{
  SubTagType m_type{SubTagType::Unknown};
  std::string m_subTag;
  std::vector<std::string> m_descriptions;
  std::string m_added;
  std::string m_deprecated;
  std::string m_preferredValue;
  std::vector<std::string> m_prefixes;
  std::string m_suppressScript;
  std::string m_macroLanguage;
  SubTagScope m_scope{SubTagScope::Individual};
  std::string m_fileDate;

  RegistryFileRecord(std::vector<RegistryFileField>&& fields);
  void AssignField(RegistryFileField&& field);

  static bool IsValidDate(std::string_view date);
  bool IsValidSubTag();
};

/*!
 * \brief fmt/std::format formatter for RegistryFileRecord
 */
std::string format_as(const RegistryFileRecord& record);

class IRegistryRecordProvider
{
public:
  virtual bool Load() = 0;
  virtual const std::vector<RegistryFileRecord>& GetRecords() const = 0;
  virtual ~IRegistryRecordProvider() {}
};
} // namespace KODI::UTILS::I18N
