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

#include "HTMLTable.h"
#include "HTMLUtil.h"


using namespace HTML;

CHTMLRow::CHTMLRow(void)
{}

CHTMLRow::~CHTMLRow(void)
{}

int CHTMLRow::GetColumns() const
{
  return (int)m_vecColums.size();
}

const std::string& CHTMLRow::GetColumValue(int iColumn) const
{
  return m_vecColums[iColumn];
}

void CHTMLRow::Parse(const std::string& strTable)
{
  CHTMLUtil util;
  std::string strTag;
  int iTableRowStart = 0;
  do
  {
    iTableRowStart = util.FindTag(strTable, "<td", strTag, iTableRowStart);
    if (iTableRowStart >= 0)
    {
      iTableRowStart += (int)strTag.size();
      int iTableRowEnd = util.FindClosingTag(strTable, "td", strTag, iTableRowStart) - 1;
      if (iTableRowEnd < -1)
        break;

      std::string strRow = strTable.substr(iTableRowStart, 1 + iTableRowEnd - iTableRowStart);
      m_vecColums.push_back(strRow);

      iTableRowStart = iTableRowEnd + 1;

    }
  }
  while (iTableRowStart >= 0);
}
//------------------------------------------------------------------------------
CHTMLTable::CHTMLTable(void)
{}

CHTMLTable::~CHTMLTable(void)
{}

int CHTMLTable::GetRows() const
{
  return (int)m_vecRows.size();
}

const CHTMLRow& CHTMLTable::GetRow(int iRow) const
{
  return m_vecRows[iRow];
}

void CHTMLTable::Parse(const std::string& strHTML)
{
  m_vecRows.erase(m_vecRows.begin(), m_vecRows.end());
  CHTMLUtil util;
  std::string strTag;
  int iPosStart = util.FindTag(strHTML, "<table", strTag);
  if (iPosStart >= 0)
  {
    iPosStart += (int)strTag.size();
    int iPosEnd = util.FindClosingTag(strHTML, "table", strTag, iPosStart) - 1;
    if (iPosEnd < 0)
    {
      iPosEnd = (int)strHTML.size();
    }

    std::string strTable = strHTML.substr(iPosStart, 1 + iPosEnd - iPosStart);
    int iTableRowStart = 0;
    do
    {
      iTableRowStart = util.FindTag(strTable, "<tr", strTag, iTableRowStart);
      if (iTableRowStart >= 0)
      {
        iTableRowStart += (int)strTag.size();
        int iTableRowEnd = util.FindClosingTag(strTable, "tr", strTag, iTableRowStart) - 1;
        if (iTableRowEnd < 0)
          break;

        std::string strRow = strTable.substr(iTableRowStart, 1 + iTableRowEnd - iTableRowStart);
        CHTMLRow row;
        row.Parse(strRow);
        m_vecRows.push_back(row);
        iTableRowStart = iTableRowEnd + 1;
      }
    }
    while (iTableRowStart >= 0);
  }
}

