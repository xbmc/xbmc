/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "utils/HTMLUtil.h"

#include "gtest/gtest.h"

TEST(TestHTMLUtil, RemoveTags)
{
  std::string str;
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
  std::wstring inw, refstrw, varstrw;
  inw =     L"&aring;&amp;&euro;";
  refstrw = L"\u00e5&\u20ac";
  HTML::CHTMLUtil::ConvertHTMLToW(inw, varstrw);
  EXPECT_STREQ(refstrw.c_str(), varstrw.c_str());
}
