/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Locale.h"
#include "utils/StringUtils.h"

const CLocale CLocale::Empty;

CLocale::CLocale()
  : m_valid(false),
    m_language(),
    m_territory(),
    m_codeset(),
    m_modifier()
{ }

CLocale::CLocale(const std::string& language)
  : m_valid(false),
    m_language(),
    m_territory(),
    m_codeset(),
    m_modifier()
{
  m_valid = ParseLocale(language, m_language, m_territory, m_codeset, m_modifier);
}

CLocale::CLocale(const std::string& language, const std::string& territory)
  : m_valid(false),
    m_language(language),
    m_territory(territory),
    m_codeset(),
    m_modifier()
{
  Initialize();
}

CLocale::CLocale(const std::string& language, const std::string& territory, const std::string& codeset)
  : m_valid(false),
    m_language(language),
    m_territory(territory),
    m_codeset(codeset),
    m_modifier()
{
  Initialize();
}

CLocale::CLocale(const std::string& language, const std::string& territory, const std::string& codeset, const std::string& modifier)
  : m_valid(false),
    m_language(language),
    m_territory(territory),
    m_codeset(codeset),
    m_modifier(modifier)
{
  Initialize();
}

CLocale::~CLocale()
{ }

CLocale CLocale::FromString(const std::string& locale)
{
  return CLocale(locale);
}

bool CLocale::operator==(const CLocale& other) const
{
  if (!m_valid && !other.m_valid)
    return true;

  return m_valid == other.m_valid &&
         StringUtils::EqualsNoCase(m_language, other.m_language) &&
         StringUtils::EqualsNoCase(m_territory, other.m_territory) &&
         StringUtils::EqualsNoCase(m_codeset, other.m_codeset) &&
         StringUtils::EqualsNoCase(m_modifier, other.m_modifier);
}

std::string CLocale::ToString() const
{
  if (!m_valid)
    return "";

  std::string locale = ToShortString();

  if (!m_codeset.empty())
    locale += "." + m_codeset;

  if (!m_modifier.empty())
    locale += "@" + m_modifier;

  return locale;
}

std::string CLocale::ToStringLC() const
{
  if (!m_valid)
    return "";

  std::string locale = ToString();
  StringUtils::ToLower(locale);

  return locale;
}

std::string CLocale::ToShortString() const
{
  if (!m_valid)
    return "";

  std::string locale = m_language;

  if (!m_territory.empty())
    locale += "_" + m_territory;

  return locale;
}

std::string CLocale::ToShortStringLC() const
{
  if (!m_valid)
    return "";

  std::string locale = ToShortString();
  StringUtils::ToLower(locale);

  return locale;
}

bool CLocale::Equals(const std::string& locale) const
{
  CLocale other = FromString(locale);

  return *this == other;
}

bool CLocale::Matches(const std::string& locale) const
{
  CLocale other = FromString(locale);

  if (!m_valid && !other.m_valid)
    return true;
  if (!m_valid || !other.m_valid)
    return false;

  if (!StringUtils::EqualsNoCase(m_language, other.m_language))
    return false;
  if (!m_territory.empty() && !other.m_territory.empty() && !StringUtils::EqualsNoCase(m_territory, other.m_territory))
    return false;
  if (!m_codeset.empty() && !other.m_codeset.empty() && !StringUtils::EqualsNoCase(m_codeset, other.m_codeset))
    return false;
  if (!m_modifier.empty() && !other.m_modifier.empty() && !StringUtils::EqualsNoCase(m_modifier, other.m_modifier))
    return false;

  return true;
}

std::string CLocale::FindBestMatch(const std::set<std::string>& locales) const
{
  std::string bestMatch = "";
  int bestMatchRank = -1;

  for (auto const& locale : locales)
  {
    // check if there is an exact match
    if (Equals(locale))
      return locale;

    int matchRank = GetMatchRank(locale);
    if (matchRank > bestMatchRank)
    {
      bestMatchRank = matchRank;
      bestMatch = locale;
    }
  }

  return bestMatch;
}

bool CLocale::CheckValidity(const std::string& language, const std::string& territory, const std::string& codeset, const std::string& modifier)
{
  static_cast<void>(territory);
  static_cast<void>(codeset);
  static_cast<void>(modifier);

  return !language.empty();
}

bool CLocale::ParseLocale(const std::string &locale, std::string &language, std::string &territory, std::string &codeset, std::string &modifier)
{
  if (locale.empty())
    return false;

  language.clear();
  territory.clear();
  codeset.clear();
  modifier.clear();

  // the format for a locale is [language[_territory][.codeset][@modifier]]
  std::string tmp = locale;

  // look for the modifier after @
  size_t pos = tmp.find("@");
  if (pos != std::string::npos)
  {
    modifier = tmp.substr(pos + 1);
    tmp = tmp.substr(0, pos);
  }

  // look for the codeset after .
  pos = tmp.find(".");
  if (pos != std::string::npos)
  {
    codeset = tmp.substr(pos + 1);
    tmp = tmp.substr(0, pos);
  }

  // look for the codeset after _
  pos = tmp.find("_");
  if (pos != std::string::npos)
  {
    territory = tmp.substr(pos + 1);
    StringUtils::ToUpper(territory);
    tmp = tmp.substr(0, pos);
  }

  // what remains is the language
  language = tmp;
  StringUtils::ToLower(language);

  return CheckValidity(language, territory, codeset, modifier);
}

void CLocale::Initialize()
{
  m_valid = CheckValidity(m_language, m_territory, m_codeset, m_modifier);
  if (m_valid)
  {
    StringUtils::ToLower(m_language);
    StringUtils::ToUpper(m_territory);
  }
}

int CLocale::GetMatchRank(const std::string& locale) const
{
  CLocale other = FromString(locale);

  // both locales must be valid and match in language
  if (!m_valid || !other.m_valid ||
      !StringUtils::EqualsNoCase(m_language, other.m_language))
    return -1;

  int rank = 0;
  // matching in territory is considered more important than matching in
  // codeset and/or modifier
  if (!m_territory.empty() && !other.m_territory.empty() && StringUtils::EqualsNoCase(m_territory, other.m_territory))
    rank += 3;
  if (!m_codeset.empty() && !other.m_codeset.empty() && StringUtils::EqualsNoCase(m_codeset, other.m_codeset))
    rank += 1;
  if (!m_modifier.empty() && !other.m_modifier.empty() && StringUtils::EqualsNoCase(m_modifier, other.m_modifier))
    rank += 1;

  return rank;
}
