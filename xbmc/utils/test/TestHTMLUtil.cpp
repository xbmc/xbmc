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

#include "utils/HTMLUtil.h"

#include "gtest/gtest.h"

TEST(TestHTMLUtil, FindTag)
{
  HTML::CHTMLUtil util;
  CStdString str, found;
  str = "<!DOCTYPE html>\n"
        "<html>\n"
        "  <head class=\"someclass\">\n"
        "    <body>\n"
        "      <p>blah blah blah</p>\n"
        "    </body>\n"
        "  </head>\n"
        "</html>\n";
  EXPECT_EQ(54, util.FindTag(str, "<body", found));
  EXPECT_STREQ("<body>", found.c_str());

  found.clear();
  EXPECT_EQ(-1, util.FindTag(str, "<span", found));
  EXPECT_STREQ("", found.c_str());
}

TEST(TestHTMLUtil, FindClosingTag)
{
  HTML::CHTMLUtil util;
  CStdString str, found;
  str = "<!DOCTYPE html>\n"
        "<html>\n"
        "  <head class=\"someclass\">\n"
        "    <body>\n"
        "      <p>blah blah blah</p>\n"
        "    </body>\n"
        "  </head>\n"
        "</html>\n";
  /* Need to set position past '<body>' tag */
  EXPECT_EQ(93, util.FindClosingTag(str, "body", found, 61));
  EXPECT_STREQ("</body>", found.c_str());
}

TEST(TestHTMLUtil, getValueOfTag)
{
  HTML::CHTMLUtil util;
  CStdString str, value;
  str = "<p>blah blah blah</p>";
  util.getValueOfTag(str, value);
  EXPECT_STREQ("blah blah blah", value.c_str());
}

TEST(TestHTMLUtil, getAttributeOfTag)
{
  HTML::CHTMLUtil util;
  CStdString str, tag, value;
  str = "<head class=\"someclass\"></head>\n";
  util.getAttributeOfTag(str, "class", value);
  EXPECT_STREQ("\"someclass", value.c_str());
}

TEST(TestHTMLUtil, RemoveTags)
{
  CStdString str;
  str = "<!DOCTYPE html>\n"
        "<html>\n"
        "  <head class=\"someclass\">\n"
        "    <body>\n"
        "      <p>blah blah blah</p>\n"
        "    </body>\n"
        "  </head>\n"
        "</html>\n";
  HTML::CHTMLUtil::RemoveTags(str);
  EXPECT_STREQ("\n\n  \n    \n      blah blah blah\n    \n  \n\n",
               str.c_str());
}

TEST(TestHTMLUtil, ConvertHTMLToW)
{
  CStdString str;
  CStdStringW refstrw, varstrw;
  str = "<!DOCTYPE html>\n"
        "<html>\n"
        "  <head class=\"someclass\">\n"
        "    <body>\n"
        "      <p>blah blah blah</p>\n"
        "    </body>\n"
        "  </head>\n"
        "</html>\n";
  refstrw = str;
  HTML::CHTMLUtil::ConvertHTMLToW(str, varstrw);
  EXPECT_STREQ(refstrw.c_str(), varstrw.c_str());
}
