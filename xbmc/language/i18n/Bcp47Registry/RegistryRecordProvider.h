/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/i18n/Bcp47Registry/SubTagRegistryTypes.h"

#include <string>
#include <vector>

#include <fmt/format.h> //! \todo remove after upgrade to libfmt >= 10.0

namespace KODI::LANGUAGE::I18N
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
 * \brief RegistryFileRecord formatter for libfmt / future std
 */
std::string format_as(const RegistryFileRecord& record);

class IRegistryRecordProvider
{
public:
  virtual bool Load() = 0;
  virtual const std::vector<RegistryFileRecord>& GetRecords() const = 0;
  virtual ~IRegistryRecordProvider() {}
};
} // namespace KODI::LANGUAGE::I18N

#if FMT_VERSION < 100000
// user-type formatters for libfmt < 10.0
//! \todo remove after libfmt upgrade
template<>
struct fmt::formatter<KODI::LANGUAGE::I18N::RegistryFileRecord> : fmt::formatter<std::string>
{
  auto format(const KODI::LANGUAGE::I18N::RegistryFileRecord& p, format_context& ctx) const
  {
    return fmt::formatter<std::string>::format(KODI::LANGUAGE::I18N::format_as(p), ctx);
  }
};
#endif
