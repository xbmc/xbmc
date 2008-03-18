#include "stdafx.h"
#include "./HTMLTable.h"
#include "./HTMLUtil.h"


using namespace HTML;

CHTMLRow::CHTMLRow(void)
{}

CHTMLRow::~CHTMLRow(void)
{}

int CHTMLRow::GetColumns() const
{
  return (int)m_vecColums.size();
}

const CStdString& CHTMLRow::GetColumValue(int iColumn) const
{
  return m_vecColums[iColumn];
}

void CHTMLRow::Parse(const CStdString& strTable)
{
  CHTMLUtil util;
  CStdString strTag;
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

      CStdString strRow = strTable.Mid(iTableRowStart, 1 + iTableRowEnd - iTableRowStart);
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

void CHTMLTable::Parse(const CStdString& strHTML)
{
  m_vecRows.erase(m_vecRows.begin(), m_vecRows.end());
  CHTMLUtil util;
  CStdString strTag;
  int iPosStart = util.FindTag(strHTML, "<table", strTag);
  if (iPosStart >= 0)
  {
    iPosStart += (int)strTag.size();
    int iPosEnd = util.FindClosingTag(strHTML, "table", strTag, iPosStart) - 1;
    if (iPosEnd < 0)
    {
      iPosEnd = (int)strHTML.size();
    }

    CStdString strTable = strHTML.Mid(iPosStart, 1 + iPosEnd - iPosStart);
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

        CStdString strRow = strTable.Mid(iTableRowStart, 1 + iTableRowEnd - iTableRowStart);
        CHTMLRow row;
        row.Parse(strRow);
        m_vecRows.push_back(row);
        iTableRowStart = iTableRowEnd + 1;
      }
    }
    while (iTableRowStart >= 0);
  }
}

