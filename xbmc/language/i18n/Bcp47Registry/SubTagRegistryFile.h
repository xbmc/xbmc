/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/i18n/Bcp47Registry/RegistryRecordProvider.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace KODI::LANGUAGE::I18N
{
class CRegistryFile : public IRegistryRecordProvider
{
public:
  /*!
   * \brief The registry file, read into memory in one go.
   *
   * Reading it line by line costs several file system calls per line, which the
   * wasm build proxies to the browser main thread.
   */
  class CRegFile
  {
  public:
    /*!
     * \brief Load the whole file
     * \param[in] filePath path of the file
     * \return true for success, otherwise false
     */
    bool Open(const std::string& filePath);

    /*!
     * \brief Read the file record by record
     * \param[out] lines the fetched record
     * \return true for success, otherwise false for EOF or error
     */
    bool ReadRecord(std::vector<std::string>& lines);

  private:
    bool ReadLine(std::string& line);

    std::vector<uint8_t> m_data;
    std::size_t m_pos{0};
  };

  CRegistryFile(std::string filePath) : m_filePath(filePath) {}

  bool Load() override;
  const std::vector<RegistryFileRecord>& GetRecords() const override { return m_records; }

protected:
  std::unique_ptr<CRegFile> Open();
  bool Parse(CRegFile* file);

  /*!
   * \brief Convert a series of text lines into registry file record fields.
   *        The function is typically used on the set of lines making up a registry file record.
   * \param[in] lines The text lines to be converted
   * \return List of registry file fields
   */
  std::vector<RegistryFileField> ProcessRecordLines(const std::vector<std::string>& lines);

private:
  std::string m_filePath;
  std::vector<RegistryFileRecord> m_records;
};
} // namespace KODI::LANGUAGE::I18N
