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

#include "utils/HttpHeader.h"

#include "gtest/gtest.h"

TEST(TestHttpHeader, General)
{
  CHttpHeader a;
  CStdString str = "Host: xbmc.org\r\n"
                   "Accept: text/*, text/html, text/html;level=1, */*\r\n"
                   "Accept-Language: en\r\n"
                   "Accept-Encoding: gzip, deflate\r\n"
                   "Content-Type: text/html; charset=ISO-8859-4\r\n"
                   "User-Agent: XBMC/snapshot (compatible; MSIE 5.5; Windows NT"
                     " 4.0)\r\n"
                   "Connection: Keep-Alive\r\n";
  CStdString refstr, varstr;

  a.Parse(str);

  refstr = "accept: text/*, text/html, text/html;level=1, */*\n"
           "accept-encoding: gzip, deflate\n"
           "accept-language: en\n"
           "connection: Keep-Alive\n"
           "content-type: text/html; charset=ISO-8859-4\n"
           "host: xbmc.org\n"
           "user-agent: XBMC/snapshot (compatible; MSIE 5.5; Windows NT 4.0)\n"
           "\n";
  varstr.clear();
  a.GetHeader(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "XBMC/snapshot (compatible; MSIE 5.5; Windows NT 4.0)";
  varstr = a.GetValue("User-Agent");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "text/html; charset=ISO-8859-4";
  varstr = a.GetMimeType();
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = a.GetProtoLine();
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  a.Clear();
  refstr = "";
  varstr = a.GetMimeType();
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}
