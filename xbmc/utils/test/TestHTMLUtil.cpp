/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
