/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Registry/SubTagRegistryFile.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <utility>

using namespace KODI::UTILS::I18N;

bool CRegistryFile::Load()
{
  std::unique_ptr<CRegFile> file = Open();
  if (!file)
    return false;

  bool result = Parse(file.get());

  file->Close();

  return result;
}

std::unique_ptr<CRegistryFile::CRegFile> CRegistryFile::Open()
{
  std::unique_ptr<CRegFile> file = std::make_unique<CRegFile>();

  if (!file->Open(m_filePath))
  {
    CLog::LogF(LOGERROR, "Unable to open the language subtag registry file ({})", m_filePath);
    return nullptr;
  }
  return file;
}

namespace
{
void RemoveLineTerminators(std::string& s)
{
  if (s.ends_with('\n'))
    s.pop_back();
  if (s.ends_with('\r'))
    s.pop_back();
}
} // namespace

bool CRegistryFile::Parse(CRegistryFile::CRegFile* file)
{
  CLog::Log(LOGINFO, "IANA Language Subtag Registry: file {}", m_filePath);

  std::vector<std::string> recordLines;
  recordLines.reserve(20); // max lines per record for file dated 2025-08-25

  // First record is expected to be the file date
  if (!file->ReadRecord(recordLines))
  {
    CLog::LogF(LOGERROR, "The file is empty.");
    return false;
  }

  RegistryFileRecord record(ProcessRecordLines(recordLines));

  const std::string date = record.m_fileDate;
  if (RegistryFileRecord::IsValidDate(date)) [[likely]]
  {
    CLog::Log(LOGINFO, "IANA Language Subtag Registry: registry date {}", date);
  }
  else [[unlikely]]
  {
    CLog::LogF(
        LOGERROR, "Invalid first record, File-Date field was expected.\r\nActual content: [{}] ",
        StringUtils::Join(std::span<std::string>(recordLines.begin(),
                                                 std::min<std::size_t>(recordLines.size(), 5)),
                          "\r\n"));
  }

  // Then loop through the rest of the file
  m_records.clear();

  while (file->ReadRecord(recordLines)) [[likely]]
  {
    record = ProcessRecordLines(recordLines);

    if (record.IsValidSubTag()) [[likely]]
      m_records.push_back(std::move(record));
  }

  CLog::LogF(LOGDEBUG, "IANA Language Subtag Registry: file read.");

  return !m_records.empty();
}

std::vector<RegistryFileField> CRegistryFile::ProcessRecordLines(
    const std::vector<std::string>& lines)
{
  std::string fieldName;
  std::string fieldBody;
  std::vector<RegistryFileField> fields;

  for (const auto& line : lines) [[likely]]
  {
    if (line.empty()) [[unlikely]]
    {
      CLog::LogF(LOGWARNING, "blank lines are invalid - ignoring");
      continue;
    }
    else if (line.front() != ' ') [[likely]]
    {
      // line doesn't start with a space: not a field body continuation.
      size_t sepIndex = line.find(':');
      if (sepIndex != std::string::npos) [[likely]]
      {
        // Start of a new field: process data accumulated for the previous field.
        if (!fieldName.empty() && !fieldBody.empty())
          fields.emplace_back(std::move(fieldName), std::move(fieldBody));

        fieldName = line.substr(0, sepIndex);
        StringUtils::TrimRight(fieldName);
        fieldBody = line.substr(sepIndex + 1, std::string::npos);
        StringUtils::TrimLeft(fieldBody);
      }
      else [[unlikely]]
      {
        // abnormal: a line that's not a field body continuation must be a new field but the separator
        // was not found
        CLog::LogF(LOGERROR, "invalid field declaration line - no separator ({})", line);
      }
    }
    else [[unlikely]]
    {
      if (fieldName.empty())
      {
        // abnormal: record starts with a field body continuation line
        CLog::LogF(LOGERROR, "invalid field body continuation line - no current field ({})", line);
      }
      // Valid field body continuation
      //
      // ABNF of registry records in RFC5646 doesn't allow unambiguous recovery of the original
      // string since one or multiple spaces are allowed at the beginning of a continuation line,
      // it's not possible to know if some of the spaces are part of the field body.
      //
      // However at this time in actual registry files, body continuation lines start with 2 spaces,
      // one of which belongs to the field body text > remove a single space character
      fieldBody.append(line.substr(1, std::string::npos));
    }
  }
  // End of the data : process data accumulated for the last field.
  if (!fieldName.empty() && !fieldBody.empty())
    fields.emplace_back(std::move(fieldName), std::move(fieldBody));

  return fields;
}

bool CRegistryFile::CRegFile::ReadRecord(std::vector<std::string>& lines)
{
  std::string line;

  if (!ReadLine(line)) [[unlikely]]
    return false;

  lines.clear();

  do
  {
    // Remove the line terminators included by ReadLine
    RemoveLineTerminators(line);

    // Registry records are separated by lines containing only the sequence %%
    if (line == "%%")
      return true;

    lines.push_back(std::move(line));
  } while (ReadLine(line));

  // Final record not terminated by %%
  return true;
}
