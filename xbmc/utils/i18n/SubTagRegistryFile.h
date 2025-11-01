/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/i18n/SubTagRegistry.h"

#include <memory>
#include <string>
#include <vector>

struct RegistryFileField
{
  std::string m_name;
  std::string m_body;
};

struct RegistryFileRecord
{
  SubTagType m_type{SubTagType::Unknown};
  std::string m_subTag;
  std::vector<std::string> m_description;
  std::string m_added;
  std::string m_deprecated;
  std::string m_preferredValue;
  std::vector<std::string> m_prefix;
  std::string m_suppressScript;
  std::string m_macroLanguage;
  SubTagScope m_scope{SubTagScope::Any};
  std::string m_fileDate;

  RegistryFileRecord(std::vector<RegistryFileField> fields);
  void UpdateRecord(RegistryFileField field);

  bool IsValidSubTag();
};

class CRegistryFile
{
public:
  class CRegFile : public XFILE::CFile
  {
  public:
    std::optional<std::vector<std::string>> ReadRecord();
  };

  CRegistryFile(std::string filePath) : m_filePath(filePath) {}

  bool Load();
  const std::vector<RegistryFileRecord>& GetRecords() const { return m_records; }

protected:
  std::unique_ptr<CRegFile> Open();
  bool Parse(CRegFile* file);
  std::vector<RegistryFileField> ProcessRecordLines(std::vector<std::string> lines);

private:
  std::string m_filePath;
  std::vector<RegistryFileRecord> m_records;
};
