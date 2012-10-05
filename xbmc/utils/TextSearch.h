#pragma once
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

#include <vector>
#include "StringUtils.h"

typedef enum TextSearchDefault
{
  SEARCH_DEFAULT_AND = 0,
  SEARCH_DEFAULT_OR,
  SEARCH_DEFAULT_NOT
} TextSearchDefault;

class CTextSearch
{
public:
  CTextSearch(const CStdString &strSearchTerms, bool bCaseSensitive = false, TextSearchDefault defaultSearchMode = SEARCH_DEFAULT_OR);
  virtual ~CTextSearch(void);

  bool Search(const CStdString &strHaystack) const;
  bool IsValid(void) const;

private:
  void GetAndCutNextTerm(CStdString &strSearchTerm, CStdString &strNextTerm);
  void ExtractSearchTerms(const CStdString &strSearchTerm, TextSearchDefault defaultSearchMode);

  bool                     m_bCaseSensitive;
  std::vector<CStdString>  m_AND;
  std::vector<CStdString>  m_OR;
  std::vector<CStdString>  m_NOT;
};
