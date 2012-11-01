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

#include "utils/HTMLTable.h"

#include "gtest/gtest.h"

// class CHTMLRow
// {
// public:
//   CHTMLRow(void);
//   virtual ~CHTMLRow(void);
//   int GetColumns() const;
//   const CStdString& GetColumValue(int iColumn) const;
//   void Parse(const CStdString& strTableRow);
// 
// protected:
//   std::vector<CStdString> m_vecColums;
// };
// 
// class CHTMLTable
// {
// public:
//   CHTMLTable(void);
//   virtual ~CHTMLTable(void);
//   void Parse(const CStdString& strHTML);
//   int GetRows() const;
//   const CHTMLRow& GetRow(int iRow) const;
// protected:
//   std::vector<CHTMLRow> m_vecRows;
// };

TEST(TestHTMLTable, General)
{
  HTML::CHTMLTable table;
  HTML::CHTMLRow row1, row2;
  CStdString str;
  str = "<table>\n"
        "  <tr>\n"
        "    <td>r1c1</td>\n"
        "    <td>r1c2</td>\n"
        "  </tr>\n"
        "  <tr>\n"
        "    <td>r2c1</td>\n"
        "    <td>r2c2</td>\n"
        "  </tr>\n"
        "  <tr>\n"
        "    <td>r3c1</td>\n"
        "    <td>r3c2</td>\n"
        "  </tr>\n"
        "</table>\n";
  table.Parse(str);
  EXPECT_EQ(3, table.GetRows());

  row1 = table.GetRow(0);
  EXPECT_EQ(2, row1.GetColumns());
  EXPECT_STREQ("r1c1", row1.GetColumValue(0));

  str = "<tr>\n"
        "  <td>new row1 column1</td>\n"
        "  <td>new row1 column2</td>\n"
        "  <td>new row1 column3</td>\n"
        "</tr>\n";
  row2.Parse(str);
  EXPECT_EQ(3, row2.GetColumns());
  EXPECT_STREQ("new row1 column2", row2.GetColumValue(1));
}
