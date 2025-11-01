/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/SubTagRegistryFile.h"

#include "XBDateTime.h"
#include "utils/log.h"

#include <algorithm>

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

bool IsValidDate(std::string_view d)
{
  // All dates are expected in YYYY-MM-DD RFC3339 full-date format.
  CDateTime dt;
  return dt.SetFromRFC3339FullDate(d);
}
} // namespace

bool CRegistryFile::Parse(CRegistryFile::CRegFile* file)
{
  CLog::LogF(LOGINFO, "IANA Language Subtag Registry file {}", m_filePath);

  // First record is expected to be the file date
  auto recordLines = file->ReadRecord();
  if (!recordLines.has_value())
  {
    CLog::LogF(LOGERROR, "The file is empty.");
    return false;
  }

  RegistryFileRecord record(ProcessRecordLines(recordLines.value()));

  const std::string date = record.m_fileDate;
  if (IsValidDate(date))
  {
    CLog::LogF(LOGINFO, "File date: {}", date);
  }
  else
  {
    CLog::LogF(
        LOGERROR, "Invalid first record, File-Date field was expected.\r\nActual content: [{}] ",
        StringUtils::Join(std::span<std::string>(recordLines->begin(),
                                                 std::min<std::size_t>(recordLines->size(), 5)),
                          "\r\n"));
  }

  // Then loop through the rest of the file
  m_records.clear();

  while (recordLines = file->ReadRecord())
  {
    record = ProcessRecordLines(std::move(recordLines.value()));

    if (record.IsValidSubTag())
      m_records.push_back(std::move(record));
  }

  return !m_records.empty();
}

std::vector<RegistryFileField> CRegistryFile::ProcessRecordLines(std::vector<std::string> lines)
{
  std::string fieldName;
  std::string fieldBody;
  std::vector<RegistryFileField> fields;

  for (const auto& line : lines)
  {
    if (line.empty())
    {
      CLog::LogF(LOGWARNING, "blank lines are invalid - ignoring");
      continue;
    }
    else if (line.front() != ' ')
    {
      // line doesn't start with a space: not a field body continuation.
      size_t sepIndex = line.find(':');
      if (sepIndex != std::string::npos)
      {
        // Start of a new field: process data accumulated for the previous field.
        if (!fieldName.empty() && !fieldBody.empty())
          fields.emplace_back(std::move(fieldName), std::move(fieldBody));

        fieldName = line.substr(0, sepIndex);
        StringUtils::TrimRight(fieldName);
        fieldBody = line.substr(sepIndex + 1, std::string::npos);
        StringUtils::TrimLeft(fieldBody);
      }
      else
      {
        // abnormal: a line that's not a field body continuation must be a new field but the separator
        // was not found
        CLog::LogF(LOGERROR, "invalid field declaration line - no separator ({})", line);
      }
    }
    else
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

std::optional<std::vector<std::string>> CRegistryFile::CRegFile::ReadRecord()
{
  std::string line;
  std::vector<std::string> recordLines;
  bool isFirstRecord{true};

  if (!ReadLine(line))
    return std::nullopt;

  do
  {
    // Remove the line terminators included by ReadLine
    RemoveLineTerminators(line);

    // Registry records are separated by lines containing only the sequence %%
    if (line == "%%")
      return recordLines;

    recordLines.push_back(std::move(line));
  } while (ReadLine(line));

  // Final record not terminated by %%
  return recordLines;
}

void RegistryFileRecord::UpdateRecord(RegistryFileField field)
{
  if (field.m_name == "Type")
  {
    // May be overkill to translate to enum here - likely has to be done again in a similar way later
    if (field.m_body == "language")
      m_type = SubTagType::Language;
    else if (field.m_body == "extlang")
      m_type = SubTagType::ExtLang;
    else if (field.m_body == "script")
      m_type = SubTagType::Script;
    else if (field.m_body == "region")
      m_type = SubTagType::Region;
    else if (field.m_body == "variant")
      m_type = SubTagType::Variant;
    else if (field.m_body == "grandfathered")
      m_type = SubTagType::Grandfathered;
    else if (field.m_body == "redundant")
      m_type = SubTagType::Redundant;
    else
      m_type = SubTagType::Unknown; // new type added by future RFC?
    return;
  }
  else if (field.m_name == "Scope")
  {
    // May be overkill to translate to enum here - likely has to be done again in a similar way later
    if (field.m_body == "macrolanguage")
      m_scope = SubTagScope::MacroLanguage;
    else if (field.m_body == "collection")
      m_scope = SubTagScope::Collection;
    else if (field.m_body == "special")
      m_scope = SubTagScope::Special;
    else if (field.m_body == "private-use")
      m_scope = SubTagScope::PrivateUse;
    else
      m_scope = SubTagScope::Unknown; // new scope added by future RFC?
    return;
  }

  if (field.m_name == "Subtag" || field.m_name == "Tag")
    m_subTag = std::move(field.m_body);
  else if (field.m_name == "Description")
    m_description.emplace_back(std::move(field.m_body));
  else if (field.m_name == "Added")
    m_added = std::move(field.m_body);
  else if (field.m_name == "Deprecated")
    m_deprecated = std::move(field.m_body);
  else if (field.m_name == "Preferred-Value")
    m_preferredValue = std::move(field.m_body);
  else if (field.m_name == "Prefix")
    m_prefix.emplace_back(std::move(field.m_body));
  else if (field.m_name == "Suppress-Script")
    m_suppressScript = std::move(field.m_body);
  else if (field.m_name == "Macrolanguage")
    m_macroLanguage = std::move(field.m_body);
  else if (field.m_name == "File-Date")
    m_fileDate = std::move(field.m_body);

  // Unrecognized field name: do nothing, may have been added by future RFC revision
}

RegistryFileRecord::RegistryFileRecord(std::vector<RegistryFileField> fields)
{
  for (auto&& field : fields)
    UpdateRecord(std::move(field));
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
