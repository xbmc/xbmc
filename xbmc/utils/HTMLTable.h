#pragma once

namespace HTML
{
class CHTMLRow
{
public:
  CHTMLRow(void);
  virtual ~CHTMLRow(void);
  int GetColumns() const;
  const CStdString& GetColumValue(int iColumn) const;
  void Parse(const CStdString& strTableRow);

protected:
  vector<CStdString> m_vecColums;
};

class CHTMLTable
{
public:
  CHTMLTable(void);
  virtual ~CHTMLTable(void);
  void Parse(const CStdString& strHTML);
  int GetRows() const;
  const CHTMLRow& GetRow(int iRow) const;
protected:
  vector<CHTMLRow> m_vecRows;
};
}
