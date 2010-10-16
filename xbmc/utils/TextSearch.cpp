/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "TextSearch.h"

using namespace std;

cTextSearch::cTextSearch(void)
{
  m_text          = "";
  m_searchText    = "";
  m_CaseSensitive = false;
}

cTextSearch::cTextSearch(CStdString text, CStdString searchText, bool caseSensitive)
{
  m_CaseSensitive = caseSensitive;
  m_text          = text;
  m_searchText    = searchText;
}

cTextSearch::~cTextSearch(void)
{
  m_AND.clear();
  m_OR.clear();
  m_NOT.clear();
}

void cTextSearch::SetText(CStdString text, CStdString searchText, bool caseSensitive)
{
  m_CaseSensitive = caseSensitive;
  m_text          = text;
  m_searchText    = searchText;
}

bool cTextSearch::DoSearch()
{
  m_AND.clear();
  m_OR.clear();
  m_NOT.clear();

  CStdString text       = m_text;
  CStdString searchStr  = m_searchText;

  if (text == "")
    return false;

  if (searchStr == "")
    return true;

  if (!m_CaseSensitive)
    text.ToLower();

  /* Only one word search */
  if (searchStr.Find(" ") == CStdString::npos)
  {
    if (searchStr[0] == '"')
      searchStr.erase(0, 1);
    if (searchStr[searchStr.size()-1] == '"')
      searchStr.erase(searchStr.size()-1, 1);

    if (!m_CaseSensitive)
      searchStr.ToLower();

    if (text.Find(searchStr) == CStdString::npos)
      return false;
    else
      return true;
  }
  else
  {
    while (searchStr.length() > 3)
    {
      /* Remove white spaces and begin of search line */
      if (searchStr[0] == ' ')
      {
        searchStr.erase(0, 1);
      }
      /* Search for AND */
      else if (searchStr.Find("AND") != CStdString::npos || searchStr.Find("+") != CStdString::npos)
      {
        size_t firstPos     = searchStr.Find("AND");
        size_t lastPos      = CStdString::npos;

        if (firstPos != CStdString::npos)
          searchStr.erase(firstPos, 3);
        else
        {
          firstPos = searchStr.Find("+");
          searchStr.erase(firstPos, 1);
        }

        ClearBlankCharacter(&searchStr, firstPos);

        if (searchStr[firstPos] == '"')
        {
          firstPos++; // Remove the phrase
          lastPos = searchStr.Find("\"", firstPos);
        }
        else
          lastPos = searchStr.Find(" ");

        size_t strSize = CStdString::npos;
        if (lastPos != CStdString::npos)
          strSize = lastPos-firstPos;

        m_AND.push_back(searchStr.substr(firstPos, strSize));
        searchStr.erase(firstPos, lastPos-firstPos);
      }
      /* Search for OR */
      else if (searchStr.Find("OR") != CStdString::npos || searchStr.Find("|") != CStdString::npos)
      {
        size_t firstPos     = searchStr.Find("OR");
        size_t lastPos      = CStdString::npos;

        if (firstPos != CStdString::npos)
          searchStr.erase(firstPos, 2);
        else
        {
          firstPos = searchStr.Find("|");
          searchStr.erase(firstPos, 1);
        }

        ClearBlankCharacter(&searchStr, firstPos);

        if (searchStr[firstPos] == '"')
        {
          firstPos++; // Remove the phrase
          lastPos = searchStr.Find("\"", firstPos);
        }
        else
          lastPos = searchStr.Find(" ");

        size_t strSize = CStdString::npos;
        if (lastPos != CStdString::npos)
          strSize = lastPos-firstPos;

        m_OR.push_back(searchStr.substr(firstPos, strSize));
        searchStr.erase(firstPos, lastPos-firstPos);
      }
      /* Search for OR */
      else if (searchStr.Find("NOT") != CStdString::npos || searchStr.Find("-") != CStdString::npos)
      {
        size_t firstPos     = searchStr.Find("NOT");
        size_t lastPos      = CStdString::npos;

        if (firstPos != CStdString::npos)
          searchStr.erase(firstPos, 3);
        else
        {
          firstPos = searchStr.Find("-");
          searchStr.erase(firstPos, 1);
        }

        ClearBlankCharacter(&searchStr, firstPos);

        if (searchStr[firstPos] == '"')
        {
          firstPos++; // Remove the phrase
          lastPos = searchStr.Find("\"", firstPos);
        }
        else
          lastPos = searchStr.Find(" ");

        size_t strSize = CStdString::npos;
        if (lastPos != CStdString::npos)
          strSize = lastPos-firstPos;

        m_NOT.push_back(searchStr.substr(firstPos, strSize));
        searchStr.erase(firstPos, lastPos-firstPos);
      }
      else
      {
        size_t lastPos = CStdString::npos;

        if (searchStr[0] == '"')
        {
          searchStr.erase(0, 1);
          lastPos = searchStr.Find("\"");
        }
        else
        {
          lastPos = searchStr.Find(" ");
        }

        m_AND.push_back(searchStr.substr(0, lastPos));
        searchStr.erase(0, lastPos);
      }
    }

    /* If no search words are present everything is true */
    if (m_AND.size() == 0 && m_OR.size() == 0 && m_NOT.size() == 0)
      return true;

    for (unsigned int i = 0; i < m_NOT.size(); i++)
    {
      CStdString word = m_NOT[i];
      if (!m_CaseSensitive)
        word.ToLower();

      if (text.Find(word) != CStdString::npos)
        return false;
    }

    for (unsigned int i = 0; i < m_OR.size(); i++)
    {
      CStdString word = m_OR[i];
      if (!m_CaseSensitive)
        word.ToLower();

      if (text.Find(word) != CStdString::npos)
        return true;
    }

    for (unsigned int i = 0; i < m_AND.size(); i++)
    {
      CStdString word = m_AND[i];
      if (!m_CaseSensitive)
        word.ToLower();

      if (text.Find(word) == CStdString::npos)
        return false;
    }

    return true;
  }
  return false;
}

bool cTextSearch::SearchText(CStdString text, CStdString searchText, bool caseSensitive)
{
  cTextSearch search(text, searchText, caseSensitive);
  return search.DoSearch();
}

void cTextSearch::ClearBlankCharacter(CStdString *text, int pos)
{
  if (text->at(pos) > ' ')
    return;
  while (text->length() > 0 && text->at(pos) == ' ')
    text->erase(pos, 1);
}
