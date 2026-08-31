/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/LanguageTable.h"

#include "utils/StringUtils.h"
#include "language/i18n/Iso639_1.h"
#include "language/i18n/Iso639_2.h"

using namespace KODI::LANGUAGE::I18N;

namespace
{
std::string Key(std::string_view text)
{
  std::string key{StringUtils::ToLower(text)};
  StringUtils::Trim(key);
  return key;
}
} // namespace

CLanguageTable::CLanguageTable()
{
  Seed();
}

CLanguageTable& CLanguageTable::GetInstance()
{
  static CLanguageTable table;
  return table;
}

void CLanguageTable::Seed()
{
  CIso639_1::ListLanguages(m_names);
  CIso639_2::ListLanguages(m_names);

  // ISO 639-1 is enumerated first so that a language having codes in both standards is named by
  // its alpha-2 one, which is what the rest of the application prefers
  std::map<std::string, std::string> names;
  CIso639_1::ListLanguageNames(names);
  CIso639_2::ListLanguageNames(names);

  for (const auto& [name, code] : names)
    m_codes.emplace(Key(name), code);
}

void CLanguageTable::Declare(const std::map<std::string, std::string>& languages)
{
  for (const auto& [code, name] : languages)
  {
    const std::string key{Key(code)};

    m_declared[key] = name;
    m_names[key] = name;
    m_codes[Key(name)] = key;
  }
}

void CLanguageTable::DeclareNames(const std::map<std::string, std::string>& languages)
{
  for (const auto& [code, name] : languages)
  {
    if (code.empty() || name.empty())
      continue;

    // try_emplace, not assignment: whatever already names this language outranks an addon
    const std::string key{Key(code)};
    m_names.try_emplace(key, name);
    m_codes.try_emplace(Key(name), key);
  }
}

void CLanguageTable::Reset()
{
  m_names.clear();
  m_codes.clear();
  m_declared.clear();

  Seed();
}

std::optional<std::string> CLanguageTable::NameOf(std::string_view code) const
{
  if (code.empty())
    return std::nullopt;

  if (const auto it = m_names.find(Key(code)); it != m_names.end())
    return it->second;

  return std::nullopt;
}

std::optional<std::string> CLanguageTable::CodeOf(std::string_view name) const
{
  if (name.empty())
    return std::nullopt;

  if (const auto it = m_codes.find(Key(name)); it != m_codes.end())
    return it->second;

  return std::nullopt;
}

void CLanguageTable::List(std::map<std::string, std::string>& languages) const
{
  std::map<std::string, std::string> iso;
  CIso639_1::ListLanguages(iso);

  for (const auto& [code, name] : iso)
    languages.insert_or_assign(code, name);

  for (const auto& [code, name] : m_declared)
    languages.insert_or_assign(code, name);
}
