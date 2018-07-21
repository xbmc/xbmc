/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
