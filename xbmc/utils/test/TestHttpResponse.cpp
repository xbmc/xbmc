/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/HttpResponse.h"

#include "gtest/gtest.h"

TEST(TestHttpResponse, General)
{
  CHttpResponse a(HTTP::POST, HTTP::OK);
  std::string response, content, refstr;

  a.AddHeader("date", "Sun, 01 Jul 2012 00:00:00 -0400");
  a.AddHeader("content-type", "text/html");
  content = "<html>\r\n"
             "  <body>\r\n"
             "    <h1>XBMC TestHttpResponse Page</h1>\r\n"
             "    <p>blah blah blah</p>\r\n"
             "  </body>\r\n"
             "</html>\r\n";
  a.SetContent(content.c_str(), content.length());

  response = a.Create();;
  EXPECT_EQ((unsigned int)210, response.size());

  refstr = "HTTP/1.1 200 OK\r\n"
           "date: Sun, 01 Jul 2012 00:00:00 -0400\r\n"
           "content-type: text/html\r\n"
           "Content-Length: 106\r\n"
           "\r\n"
           "<html>\r\n"
           "  <body>\r\n"
           "    <h1>XBMC TestHttpResponse Page</h1>\r\n"
           "    <p>blah blah blah</p>\r\n"
           "  </body>\r\n"
           "</html>\r\n";
  EXPECT_STREQ(refstr.c_str(), response.c_str());
}
