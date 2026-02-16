/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/i18n/Bcp47Registry/RegistryRecordProvider.h"

#include <ostream>

namespace KODI::UTILS::I18N
{
struct Bcp47Extension;
struct ParsedBcp47Tag;

std::ostream& operator<<(std::ostream& os, const Bcp47Extension& obj);
std::ostream& operator<<(std::ostream& os, const ParsedBcp47Tag& obj);

class CMemoryRecordProvider : public IRegistryRecordProvider
{
public:
  CMemoryRecordProvider(std::vector<RegistryFileRecord>& records) : m_records(records) {}
  CMemoryRecordProvider(std::vector<RegistryFileRecord>&& records) : m_records(std::move(records))
  {
  }

  bool Load() override { return !m_records.empty(); }
  const std::vector<RegistryFileRecord>& GetRecords() const override { return m_records; }

private:
  std::vector<RegistryFileRecord> m_records;
};
} // namespace KODI::UTILS::I18N
