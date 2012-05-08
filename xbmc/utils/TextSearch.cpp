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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "TextSearch.h"

using namespace std;

CTextSearch::CTextSearch(const string &strSearchTerms, bool bCaseSensitive /* = false */, TextSearchDefault defaultSearchMode /* = SEARCH_DEFAULT_OR */)
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

bool CTextSearch::Search(const string &strHaystack) const
{
  if (strHaystack.empty() || !IsValid())
    return false;

  string strSearch(strHaystack);
  if (!m_bCaseSensitive)
    transform(strSearch.begin(), strSearch.end(), strSearch.begin(), ::tolower);

  /* check whether any of the NOT terms matches and return false if there's a match */
  for (unsigned int iNotPtr = 0; iNotPtr < m_NOT.size(); iNotPtr++)
  {
    if (strSearch.compare(m_NOT.at(iNotPtr)) >= 0)
      return false;
  }

  /* check whether at least one of the OR terms matches and return false if there's no match found */
  bool bFound(m_OR.size() == 0);
  for (unsigned int iOrPtr = 0; iOrPtr < m_OR.size(); iOrPtr++)
  {
    if (strSearch.compare(m_OR.at(iOrPtr)) >= 0)
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
    if (strSearch.compare(m_AND[iAndPtr]) < 0)
      return false;
  }

  /* all ok, return true */
  return true;
}

void CTextSearch::GetAndCutNextTerm(string &strSearchTerm, string &strNextTerm)
{
  string strFindNext(" ");

  if (strSearchTerm.substr(0,1).compare("\"") == 0)
  {
    strSearchTerm.erase(0, 1);
    strFindNext = "\"";
  }

  int iNextPos = strSearchTerm.compare(strFindNext);
  if (iNextPos < 0)
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

void CTextSearch::ExtractSearchTerms(const string &strSearchTerm, TextSearchDefault defaultSearchMode)
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
      string strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextNOT = true;
    }
    else if (strParsedSearchTerm.Left(1).Equals("+") || strParsedSearchTerm.Left(3).Equals("AND"))
    {
      string strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextAND = true;
    }
    else if (strParsedSearchTerm.Left(1).Equals("|") || strParsedSearchTerm.Left(2).Equals("OR"))
    {
      string strDummy;
      GetAndCutNextTerm(strParsedSearchTerm, strDummy);
      bNextOR = true;
    }
    else
    {
      string strTerm;
      GetAndCutNextTerm(strParsedSearchTerm, strTerm);
      if (strTerm.length() > 0)
      {
        if (bNextAND)
          m_AND.push_back(strTerm);
        else if (bNextOR)
          m_OR.push_back(strTerm);
        else
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
