/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

using namespace std;

CTextSearch::CTextSearch(const CStdString &strSearchTerms, bool bCaseSensitive /* = false */, TextSearchDefault defaultSearchMode /* = SEARCH_DEFAULT_OR */)
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

bool CTextSearch::Search(const CStdString &strHaystack) const
{
  if (strHaystack.IsEmpty() || !IsValid())
    return false;

  CStdString strSearch(strHaystack);
  if (!m_bCaseSensitive)
    strSearch = strSearch.ToLower();

  /* check whether any of the NOT terms matches and return false if there's a match */
  for (unsigned int iNotPtr = 0; iNotPtr < m_NOT.size(); iNotPtr++)
  {
    if (strSearch.Find(m_NOT.at(iNotPtr)) != -1)
      return false;
  }

  /* check whether at least one of the OR terms matches and return false if there's no match found */
  bool bFound(m_OR.size() == 0);
  for (unsigned int iOrPtr = 0; iOrPtr < m_OR.size(); iOrPtr++)
  {
    if (strSearch.Find(m_OR.at(iOrPtr)) != -1)
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
    if (strSearch.Find(m_AND[iAndPtr]) == -1)
      return false;
  }

  /* all ok, return true */
  return true;
}

void CTextSearch::GetAndCutNextTerm(CStdString &strSearchTerm, CStdString &strNextTerm)
{
  CStdString strFindNext(" ");

  if (strSearchTerm.Left(1).Equals("\""))
  {
    strSearchTerm.erase(0, 1);
    strFindNext = "\"";
  }

  int iNextPos = strSearchTerm.Find(strFindNext);
  if (iNextPos != -1)
  {
    strNextTerm = strSearchTerm.Left(iNextPos);
    strSearchTerm.erase(0, iNextPos + 1);
  }
  else
  {
    strNextTerm = strSearchTerm;
    strSearchTerm.clear();
  }
}

void CTextSearch::ExtractSearchTerms(const CStdString &strSearchTerm, TextSearchDefault defaultSearchMode)
{
  CStdString strParsedSearchTerm(strSearchTerm);
  strParsedSearchTerm = strParsedSearchTerm.Trim();

  if (!m_bCaseSensitive)
    strParsedSearchTerm = strParsedSearchTerm.ToLower();

  bool bNextAND(defaultSearchMode == SEARCH_DEFAULT_AND);
  bool bNextOR(defaultSearchMode == SEARCH_DEFAULT_OR);
  bool bNextNOT(defaultSearchMode == SEARCH_DEFAULT_NOT);

  while (strParsedSearchTerm.length() > 0)
  {
    strParsedSearchTerm = strParsedSearchTerm.TrimLeft();

    if (strParsedSearchTerm.Left(1).Equals("!") || strParsedSearchTerm.Left(3).Equals("NOT"))
    {
      CStdString strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextNOT = true;
    }
    else if (strParsedSearchTerm.Left(1).Equals("+") || strParsedSearchTerm.Left(3).Equals("AND"))
    {
      CStdString strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextAND = true;
    }
    else if (strParsedSearchTerm.Left(1).Equals("|") || strParsedSearchTerm.Left(2).Equals("OR"))
    {
      CStdString strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextOR = true;
    }
    else
    {
      CStdString strTerm;
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

    strParsedSearchTerm = strParsedSearchTerm.TrimLeft();
  }
}
