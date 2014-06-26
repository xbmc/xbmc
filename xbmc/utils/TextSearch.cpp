/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "TextSearch.h"
#include "StringUtils.h"

using namespace std;

CTextSearch::CTextSearch(const std::string &strSearchTerms, bool bCaseSensitive /* = false */, TextSearchDefault defaultSearchMode /* = SEARCH_DEFAULT_OR */)
{
  m_bCaseSensitive = bCaseSensitive;
  ExtractSearchTerms(strSearchTerms, defaultSearchMode);
}

CTextSearch::~CTextSearch(void)
{
  m_AND.clear();
  m_OR.clear();
  m_NOT.clear();
}

bool CTextSearch::IsValid(void) const
{
  return m_AND.size() > 0 || m_OR.size() > 0 || m_NOT.size() > 0;
}

bool CTextSearch::Search(const std::string &strHaystack) const
{
  if (strHaystack.empty() || !IsValid())
    return false;

  std::string strSearch(strHaystack);
  if (!m_bCaseSensitive)
    StringUtils::ToLower(strSearch);

  /* check whether any of the NOT terms matches and return false if there's a match */
  for (unsigned int iNotPtr = 0; iNotPtr < m_NOT.size(); iNotPtr++)
  {
    if (strSearch.find(m_NOT.at(iNotPtr)) != std::string::npos)
      return false;
  }

  /* check whether at least one of the OR terms matches and return false if there's no match found */
  bool bFound(m_OR.size() == 0);
  for (unsigned int iOrPtr = 0; iOrPtr < m_OR.size(); iOrPtr++)
  {
    if (strSearch.find(m_OR.at(iOrPtr)) != std::string::npos)
    {
      bFound = true;
      break;
    }
  }
  if (!bFound)
    return false;

  /* check whether all of the AND terms match and return false if one of them wasn't found */
  for (unsigned int iAndPtr = 0; iAndPtr < m_AND.size(); iAndPtr++)
  {
    if (strSearch.find(m_AND[iAndPtr]) == std::string::npos)
      return false;
  }

  /* all ok, return true */
  return true;
}

void CTextSearch::GetAndCutNextTerm(std::string &strSearchTerm, std::string &strNextTerm)
{
  std::string strFindNext(" ");

  if (StringUtils::EndsWith(strSearchTerm, "\""))
  {
    strSearchTerm.erase(0, 1);
    strFindNext = "\"";
  }

  size_t iNextPos = strSearchTerm.find(strFindNext);
  if (iNextPos != std::string::npos)
  {
    strNextTerm = strSearchTerm.substr(0, iNextPos);
    strSearchTerm.erase(0, iNextPos + 1);
  }
  else
  {
    strNextTerm = strSearchTerm;
    strSearchTerm.clear();
  }
}

void CTextSearch::ExtractSearchTerms(const std::string &strSearchTerm, TextSearchDefault defaultSearchMode)
{
  std::string strParsedSearchTerm(strSearchTerm);
  StringUtils::Trim(strParsedSearchTerm);

  if (!m_bCaseSensitive)
    StringUtils::ToLower(strParsedSearchTerm);

  bool bNextAND(defaultSearchMode == SEARCH_DEFAULT_AND);
  bool bNextOR(defaultSearchMode == SEARCH_DEFAULT_OR);
  bool bNextNOT(defaultSearchMode == SEARCH_DEFAULT_NOT);

  while (strParsedSearchTerm.length() > 0)
  {
    StringUtils::TrimLeft(strParsedSearchTerm);

    if (StringUtils::StartsWith(strParsedSearchTerm, "!") || StringUtils::StartsWithNoCase(strParsedSearchTerm, "not"))
    {
      std::string strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextNOT = true;
    }
    else if (StringUtils::StartsWith(strParsedSearchTerm, "+") || StringUtils::StartsWithNoCase(strParsedSearchTerm, "and"))
    {
      std::string strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextAND = true;
    }
    else if (StringUtils::StartsWith(strParsedSearchTerm, "|") || StringUtils::StartsWithNoCase(strParsedSearchTerm, "or"))
    {
      std::string strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextOR = true;
    }
    else
    {
      std::string strTerm;
      GetAndCutNextTerm(strParsedSearchTerm, strTerm);
      if (strTerm.length() > 0)
      {
        if (bNextAND)
          m_AND.push_back(strTerm);
        else if (bNextOR)
          m_OR.push_back(strTerm);
        else if (bNextNOT)
          m_NOT.push_back(strTerm);
      }
      else
      {
        break;
      }

      bNextAND = (defaultSearchMode == SEARCH_DEFAULT_AND);
      bNextOR  = (defaultSearchMode == SEARCH_DEFAULT_OR);
      bNextNOT = (defaultSearchMode == SEARCH_DEFAULT_NOT);
    }

    StringUtils::TrimLeft(strParsedSearchTerm);
  }
}
