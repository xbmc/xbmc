/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/i18n/LanguageTable.h"

#include "language/i18n/Iso639.h"
#include "language/i18n/Iso639_2_Table.h"
#include "language/i18n/TableLanguageCodes.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <mutex>
#include <shared_mutex>

using namespace KODI::LANGUAGE::I18N;

namespace
{
std::string Key(std::string_view text)
{
  std::string key{StringUtils::ToLower(text)};
  StringUtils::Trim(key);

  // A declaration may spell a code the POSIX way, pt_BR, and a tag spells it pt-BR
  std::ranges::replace(key, '_', '-');
  return key;
}

//! The ISO 639-1 codes in use, to their names. Withdrawn codes are left out.
void ListIso6391Languages(std::map<std::string, std::string>& languages)
{
  for (const ISO639_1& entry : TableISO639_1)
  {
    if (!entry.alpha2.empty() && !entry.withdrawn)
      languages.emplace(entry.alpha2, entry.name);
  }
}

//! The ISO 639-2 codes in either form, to their names
void ListIso6392Languages(std::map<std::string, std::string>& languages)
{
  for (const LCENTRY& entry : TableISO639_2ByCode)
    languages.emplace(LongCodeToString(entry.code), entry.name);

  for (const ISO639_2_TB& tb : ISO639_2_TB_Mappings)
  {
    const auto it =
        std::ranges::lower_bound(TableISO639_2ByCode, tb.terminological, {}, &LCENTRY::code);
    if (it != TableISO639_2ByCode.end() && it->code == tb.terminological)
      languages[LongCodeToString(tb.bibliographic)] = it->name;
  }
}

//! Every name a language is known by, to its code. A name already present is kept.
void ListLanguageNames(std::map<std::string, std::string>& names)
{
  // ISO 639-1 first, so that a language having codes in both standards is named by its alpha-2
  // one, which is what the rest of the application prefers
  for (const bool withdrawn : {false, true})
  {
    for (const ISO639_1& entry : TableISO639_1)
    {
      if (!entry.alpha2.empty() && entry.withdrawn == withdrawn)
        names.emplace(entry.name, entry.alpha2);
    }
  }

  // ISO 639-2 names are mapped to the ISO 639-2/T code
  for (const LCENTRY& entry : TableISO639_2ByCode)
    names.emplace(entry.name, LongCodeToString(entry.code));
  for (const LCENTRY& entry : TableISO639_2_Names)
    names.emplace(entry.name, LongCodeToString(entry.code));
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
  m_names.clear();
  m_codes.clear();
  m_declared.clear();

  ListIso6391Languages(m_names);
  ListIso6392Languages(m_names);

  std::map<std::string, std::string> names;
  ListLanguageNames(names);

  for (const auto& [name, code] : names)
    m_codes.emplace(Key(name), code);
}

void CLanguageTable::Declare(const std::map<std::string, std::string>& languages)
{
  std::unique_lock lock(m_section);
  Seed();

  for (const auto& [code, name] : languages)
  {
    const std::string key{Key(code)};
    const std::string nameKey{Key(name)};
    if (key.empty() || nameKey.empty())
      continue;

    m_declared[key] = name;
    m_names[key] = name;
    m_codes[nameKey] = key;
  }
}

void CLanguageTable::DeclareNames(const std::map<std::string, std::string>& languages)
{
  std::unique_lock lock(m_section);

  for (const auto& [code, name] : languages)
  {
    const std::string key{Key(code)};
    const std::string nameKey{Key(name)};
    if (key.empty() || nameKey.empty())
      continue;

    // try_emplace, not assignment: whatever already names this language outranks an addon
    m_names.try_emplace(key, name);
    m_codes.try_emplace(nameKey, key);
  }
}

void CLanguageTable::Reset()
{
  std::unique_lock lock(m_section);
  Seed();
}

std::optional<std::string> CLanguageTable::NameOf(std::string_view code) const
{
  std::shared_lock lock(m_section);
  if (const auto it = m_names.find(Key(code)); it != m_names.end())
    return it->second;

  return std::nullopt;
}

std::optional<std::string> CLanguageTable::CodeOf(std::string_view name) const
{
  std::shared_lock lock(m_section);
  if (const auto it = m_codes.find(Key(name)); it != m_codes.end())
    return it->second;

  return std::nullopt;
}

void CLanguageTable::List(std::map<std::string, std::string>& languages) const
{
  ListIso6391Languages(languages);

  std::shared_lock lock(m_section);
  for (const auto& [code, name] : m_declared)
    languages.insert_or_assign(code, name);
}
