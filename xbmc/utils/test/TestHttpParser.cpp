/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/HttpParser.h"

#include "gtest/gtest.h"

TEST(TestHttpParser, General)
{
  HttpParser a;
  std::string str = "POST /path/script.cgi HTTP/1.0\r\n"
                   "From: amejia@xbmc.org\r\n"
                   "User-Agent: XBMC/snapshot (compatible; MSIE 5.5; Windows NT"
                     " 4.0)\r\n"
                   "Content-Type: application/x-www-form-urlencoded\r\n"
                   "Content-Length: 35\r\n"
                   "\r\n"
                   "home=amejia&favorite+flavor=orange\r\n";
  std::string refstr, varstr;

  EXPECT_EQ(a.Done, a.addBytes(str.c_str(), str.length()));

  refstr = "POST";
  varstr = a.getMethod();
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "/path/script.cgi";
  varstr = a.getUri();
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = a.getQueryString();
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "home=amejia&favorite+flavor=orange\r\n";
  varstr = a.getBody();
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "application/x-www-form-urlencoded";
  varstr = a.getValue("content-type");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ((unsigned)35, a.getContentLength());
}
