/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string.h>
#include "utils/HttpHeader.h"
#include "gtest/gtest.h"

#define CHECK_CNT_TYPE_NAME "Content-Type"
#define CHECK_CONTENT_TYPE_HTML "text/html"
#define CHECK_CONTENT_TYPE_HTML_CHRS "text/html; charset=WINDOWS-1251"
#define CHECK_CONTENT_TYPE_XML_CHRS "text/xml; charset=uTf-8"
#define CHECK_CONTENT_TYPE_TEXT "text/plain"
#define CHECK_DATE_NAME "Date"
#define CHECK_DATE_VALUE1 "Thu, 09 Jan 2014 17:58:30 GMT"
#define CHECK_DATE_VALUE2 "Thu, 09 Jan 2014 20:21:20 GMT"
#define CHECK_DATE_VALUE3 "Thu, 09 Jan 2014 20:25:02 GMT"
#define CHECK_PROT_LINE_200 "HTTP/1.1 200 OK"
#define CHECK_PROT_LINE_301 "HTTP/1.1 301 Moved Permanently"

#define CHECK_HEADER_SMPL  CHECK_PROT_LINE_200 "\r\n" \
  CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_HTML "\r\n" \
  "\r\n"

#define CHECK_HEADER_L1  CHECK_PROT_LINE_200 "\r\n" \
  "Server: nginx/1.4.4\r\n" \
  CHECK_DATE_NAME ": " CHECK_DATE_VALUE1 "\r\n" \
  CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_HTML_CHRS "\r\n" \
  "Transfer-Encoding: chunked\r\n" \
  "Connection: close\r\n" \
  "Set-Cookie: PHPSESSID=90857d437518db8f0944ca012761048a; path=/; domain=example.com\r\n" \
  "Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n" \
  "Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n" \
  "Pragma: no-cache\r\n" \
  "Set-Cookie: user_country=ot; expires=Thu, 09-Jan-2014 18:58:30 GMT; path=/; domain=.example.com\r\n" \
  "\r\n"

#define CHECK_HEADER_R  CHECK_PROT_LINE_301 "\r\n" \
  "Server: nginx/1.4.4\r\n" \
  CHECK_DATE_NAME ": " CHECK_DATE_VALUE2 "\r\n" \
  CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_HTML "\r\n" \
  "Content-Length: 150\r\n" \
  "Connection: close\r\n" \
  "Location: http://www.Example.Com\r\n" \
  "\r\n"

#define CHECK_HEADER_L2 CHECK_PROT_LINE_200 "\r\n" \
  CHECK_DATE_NAME ": " CHECK_DATE_VALUE3 "\r\n" \
  "Server: Apache/2.4.7 (Unix) mod_wsgi/3.4 Python/2.7.5 OpenSSL/1.0.1e\r\n" \
  "Last-Modified: Thu, 09 Jan 2014 20:10:28 GMT\r\n" \
  "ETag: \"9a97-4ef8f335ebd10\"\r\n" \
  "Accept-Ranges: bytes\r\n" \
  "Content-Length: 33355\r\n" \
  "Vary: Accept-Encoding\r\n" \
  "Cache-Control: max-age=3600\r\n" \
  "Expires: Thu, 09 Jan 2014 21:25:02 GMT\r\n" \
  "Connection: close\r\n" \
  CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_XML_CHRS "\r\n" \
  "\r\n"

// local helper function: replace substrings
std::string strReplc(const std::string& str, const std::string& from, const std::string& to)
{
  std::string result;
  size_t prevPos = 0;
  size_t pos;
  const size_t len = str.length();

  do
  {
    pos = str.find(from, prevPos);
    result.append(str, prevPos, pos - prevPos);
    if (pos >= len)
      break;
    result.append(to);
    prevPos = pos + from.length();
  } while (true);

  return result;
}

TEST(TestHttpHeader, General)
{
  /* check freshly created object */
  CHttpHeader testHdr;
  EXPECT_TRUE(testHdr.GetHeader().empty()) << "Newly created object is not empty";
  EXPECT_TRUE(testHdr.GetProtoLine().empty()) << "Newly created object has non-empty protocol line";
  EXPECT_TRUE(testHdr.GetMimeType().empty()) << "Newly created object has non-empty MIME-type";
  EXPECT_TRUE(testHdr.GetCharset().empty()) << "Newly created object has non-empty charset";
  EXPECT_TRUE(testHdr.GetValue("foo").empty()) << "Newly created object has some parameter";
  EXPECT_TRUE(testHdr.GetValues("bar").empty()) << "Newly created object has some parameters";
  EXPECT_FALSE(testHdr.IsHeaderDone()) << "Newly created object has \"parsing finished\" state";

  /* check general functions in simple case */
  testHdr.Parse(CHECK_HEADER_SMPL);
  EXPECT_FALSE(testHdr.GetHeader().empty()) << "Parsed header is empty";
  EXPECT_FALSE(testHdr.GetProtoLine().empty()) << "Parsed header has empty protocol line";
  EXPECT_FALSE(testHdr.GetMimeType().empty()) << "Parsed header has empty MIME-type";
  EXPECT_FALSE(testHdr.GetValue(CHECK_CNT_TYPE_NAME).empty()) << "Parsed header has empty \"" CHECK_CNT_TYPE_NAME "\" parameter";
  EXPECT_FALSE(testHdr.GetValues(CHECK_CNT_TYPE_NAME).empty()) << "Parsed header has no \"" CHECK_CNT_TYPE_NAME "\" parameters";
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Parsed header has \"parsing not finished\" state";

  /* check clearing of object */
  testHdr.Clear();
  EXPECT_TRUE(testHdr.GetHeader().empty()) << "Cleared object is not empty";
  EXPECT_TRUE(testHdr.GetProtoLine().empty()) << "Cleared object has non-empty protocol line";
  EXPECT_TRUE(testHdr.GetMimeType().empty()) << "Cleared object has non-empty MIME-type";
  EXPECT_TRUE(testHdr.GetCharset().empty()) << "Cleared object has non-empty charset";
  EXPECT_TRUE(testHdr.GetValue(CHECK_CNT_TYPE_NAME).empty()) << "Cleared object has some parameter";
  EXPECT_TRUE(testHdr.GetValues(CHECK_CNT_TYPE_NAME).empty()) << "Cleared object has some parameters";
  EXPECT_FALSE(testHdr.IsHeaderDone()) << "Cleared object has \"parsing finished\" state";

  /* check general functions after object clearing */
  testHdr.Parse(CHECK_HEADER_R);
  EXPECT_FALSE(testHdr.GetHeader().empty()) << "Parsed header is empty";
  EXPECT_FALSE(testHdr.GetProtoLine().empty()) << "Parsed header has empty protocol line";
  EXPECT_FALSE(testHdr.GetMimeType().empty()) << "Parsed header has empty MIME-type";
  EXPECT_FALSE(testHdr.GetValue(CHECK_CNT_TYPE_NAME).empty()) << "Parsed header has empty \"" CHECK_CNT_TYPE_NAME "\" parameter";
  EXPECT_FALSE(testHdr.GetValues(CHECK_CNT_TYPE_NAME).empty()) << "Parsed header has no \"" CHECK_CNT_TYPE_NAME "\" parameters";
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Parsed header has \"parsing not finished\" state";
}

TEST(TestHttpHeader, Parse)
{
  CHttpHeader testHdr;

  /* check parsing line-by-line */
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  EXPECT_FALSE(testHdr.IsHeaderDone()) << "Not completed header has \"parsing finished\" state";
  testHdr.Parse(CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_HTML "\r\n");
  EXPECT_FALSE(testHdr.IsHeaderDone()) << "Not completed header has \"parsing finished\" state";
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ(CHECK_PROT_LINE_200, testHdr.GetProtoLine().c_str()) << "Wrong protocol line";
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";

  /* check autoclearing when new header is parsed */
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  EXPECT_FALSE(testHdr.IsHeaderDone()) << "Not completed header has \"parsing finished\" state";
  EXPECT_TRUE(testHdr.GetValues(CHECK_CNT_TYPE_NAME).empty()) << "Cleared header has some parameters";
  testHdr.Clear();
  EXPECT_TRUE(testHdr.GetHeader().empty()) << "Cleared object is not empty";

  /* general check parsing */
  testHdr.Parse(CHECK_HEADER_SMPL);
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STRCASEEQ(CHECK_HEADER_SMPL, testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  testHdr.Parse(CHECK_HEADER_L1);
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STRCASEEQ(CHECK_HEADER_L1, testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  EXPECT_STREQ("Thu, 09 Jan 2014 17:58:30 GMT", testHdr.GetValue("Date").c_str()); // case-sensitive match of value
  testHdr.Parse(CHECK_HEADER_L2);
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STRCASEEQ(CHECK_HEADER_L2, testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  EXPECT_STREQ("Thu, 09 Jan 2014 20:10:28 GMT", testHdr.GetValue("Last-Modified").c_str()); // case-sensitive match of value
  testHdr.Parse(CHECK_HEADER_R);
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STRCASEEQ(CHECK_HEADER_R, testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  EXPECT_STREQ("http://www.Example.Com", testHdr.GetValue("Location").c_str()); // case-sensitive match of value

  /* check support for '\n' line endings */
  testHdr.Parse(strReplc(CHECK_HEADER_SMPL, "\r\n", "\n"));
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STRCASEEQ(CHECK_HEADER_SMPL, testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  testHdr.Parse(strReplc(CHECK_HEADER_L1, "\r\n", "\n"));
  EXPECT_STRCASEEQ(CHECK_HEADER_L1, testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  testHdr.Parse(strReplc(CHECK_HEADER_L2, "\r\n", "\n"));
  EXPECT_STRCASEEQ(CHECK_HEADER_L2, testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  testHdr.Parse(CHECK_PROT_LINE_200 "\n" CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_HTML "\r\n"); // mixed "\n" and "\r\n"
  testHdr.Parse("\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STRCASEEQ(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_HTML "\r\n\r\n", testHdr.GetHeader().c_str()) << "Parsed header mismatch the original header";
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";

  /* check trimming of whitespaces for parameter name and value */
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME ":     " CHECK_CONTENT_TYPE_HTML "\r\n\r\n");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME ":     " CHECK_CONTENT_TYPE_HTML "     \r\n\r\n");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME ":" CHECK_CONTENT_TYPE_HTML "     \r\n\r\n");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME ":\t" CHECK_CONTENT_TYPE_HTML " \t    \r\n\r\n");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME ":\t " CHECK_CONTENT_TYPE_HTML " \t    \r\n\r\n");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME "\t:" CHECK_CONTENT_TYPE_HTML " \t    \r\n\r\n");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_CNT_TYPE_NAME " \t : " CHECK_CONTENT_TYPE_HTML " \t    \r\n\r\n");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong value of parameter \"" CHECK_CNT_TYPE_NAME "\"";
}

TEST(TestHttpHeader, Parse_Multiline)
{
  CHttpHeader testHdr;

  /* Check multiline parameter parsing line-by-line */
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  testHdr.Parse(CHECK_DATE_NAME ": " CHECK_DATE_VALUE3 "\r\n");
  testHdr.Parse("X-Comment: This\r\n"); // between singleline parameters
  testHdr.Parse(" is\r\n");
  testHdr.Parse(" multi\r\n");
  testHdr.Parse(" line\r\n");
  testHdr.Parse(" value\r\n");
  testHdr.Parse(CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_TEXT "\r\n");
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  testHdr.Parse("X-Comment: This\r\n"); // first parameter
  testHdr.Parse(" is\r\n");
  testHdr.Parse(" multi\r\n");
  testHdr.Parse(" line\r\n");
  testHdr.Parse(" value\r\n");
  testHdr.Parse(CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_TEXT "\r\n");
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  testHdr.Parse(CHECK_DATE_NAME ": " CHECK_DATE_VALUE3 "\r\n");
  testHdr.Parse("X-Comment: This\r\n"); // last parameter
  testHdr.Parse(" is\r\n");
  testHdr.Parse(" multi\r\n");
  testHdr.Parse(" line\r\n");
  testHdr.Parse(" value\r\n");
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  testHdr.Parse("X-Comment: This\r\n"); // the only parameter
  testHdr.Parse(" is\r\n");
  testHdr.Parse(" multi\r\n");
  testHdr.Parse(" line\r\n");
  testHdr.Parse(" value\r\n");
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  testHdr.Parse("X-Comment: This\n"); // the only parameter with mixed ending style
  testHdr.Parse(" is\r\n");
  testHdr.Parse(" multi\n");
  testHdr.Parse(" line\r\n");
  testHdr.Parse(" value\n");
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  /* Check multiline parameter parsing as one line */
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_DATE_NAME ": " CHECK_DATE_VALUE3 "\r\nX-Comment: This\r\n is\r\n multi\r\n line\r\n value\r\n" \
                CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_TEXT "\r\n\r\n"); // between singleline parameters
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nX-Comment: This\r\n is\r\n multi\r\n line\r\n value\r\n" \
                CHECK_CNT_TYPE_NAME ": " CHECK_CONTENT_TYPE_TEXT "\r\n\r\n");  // first parameter
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n" CHECK_DATE_NAME ": " CHECK_DATE_VALUE3 "\r\nX-Comment: This\r\n is\r\n multi\r\n line\r\n value\r\n\r\n"); // last parameter
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nX-Comment: This\r\n is\r\n multi\r\n line\r\n value\r\n\r\n"); // the only parameter
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nX-Comment: This\n is\r\n multi\r\n line\n value\r\n\n"); // the only parameter with mixed ending style
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  /* Check multiline parameter parsing as mixed one/many lines */
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nX-Comment: This\n is\r\n multi\r\n");
  testHdr.Parse(" line\n value\r\n\n"); // the only parameter with mixed ending style
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is multi line value", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  /* Check parsing of multiline parameter with ':' in value */
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nX-Comment: This\r\n is:\r\n mul:ti\r\n");
  testHdr.Parse(" :line\r\n valu:e\r\n\n"); // the only parameter
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("This is: mul:ti :line valu:e", testHdr.GetValue("X-Comment").c_str()) << "Wrong multiline value";

  /* Check multiline parameter parsing with trimming */
  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  testHdr.Parse(CHECK_DATE_NAME ": " CHECK_DATE_VALUE3 "\r\n");
  testHdr.Parse("Server: Apache/2.4.7 (Unix)\r\n"); // last parameter, line-by-line parsing
  testHdr.Parse("     mod_wsgi/3.4 \r\n");
  testHdr.Parse("\tPython/2.7.5\r\n");
  testHdr.Parse("\t \t \tOpenSSL/1.0.1e\r\n");
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_GE(strlen("Apache/2.4.7 (Unix)     mod_wsgi/3.4 \tPython/2.7.5\t \t \tOpenSSL/1.0.1e"), testHdr.GetValue("Server").length()) << "Length of miltiline value is greater than length of original string";
  EXPECT_LE(strlen("Apache/2.4.7 (Unix) mod_wsgi/3.4 Python/2.7.5 OpenSSL/1.0.1e"), testHdr.GetValue("Server").length()) << "Length of miltiline value is less than length of trimmed original string";
  EXPECT_STREQ("Apache/2.4.7(Unix)mod_wsgi/3.4Python/2.7.5OpenSSL/1.0.1e", strReplc(strReplc(testHdr.GetValue("Server"), " ", ""), "\t", "").c_str()) << "Multiline value with removed whitespaces does not match original string with removed whitespaces";

  testHdr.Clear();
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\n");
  testHdr.Parse(CHECK_DATE_NAME ": " CHECK_DATE_VALUE3 "\r\n");
  testHdr.Parse("Server: Apache/2.4.7 (Unix)\r\n     mod_wsgi/3.4 \n"); // last parameter, mixed line-by-line/one line parsing, mixed line ending
  testHdr.Parse("\tPython/2.7.5\n\t \t \tOpenSSL/1.0.1e\r\n");
  testHdr.Parse("\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_GE(strlen("Apache/2.4.7 (Unix)     mod_wsgi/3.4 \tPython/2.7.5\t \t \tOpenSSL/1.0.1e"), testHdr.GetValue("Server").length()) << "Length of miltiline value is greater than length of original string";
  EXPECT_LE(strlen("Apache/2.4.7 (Unix) mod_wsgi/3.4 Python/2.7.5 OpenSSL/1.0.1e"), testHdr.GetValue("Server").length()) << "Length of miltiline value is less than length of trimmed original string";
  EXPECT_STREQ("Apache/2.4.7(Unix)mod_wsgi/3.4Python/2.7.5OpenSSL/1.0.1e", strReplc(strReplc(testHdr.GetValue("Server"), " ", ""), "\t", "").c_str()) << "Multiline value with removed whitespaces does not match original string with removed whitespaces";
}

TEST(TestHttpHeader, GetValue)
{
  CHttpHeader testHdr;

  /* Check that all parameters values can be retrieved */
  testHdr.Parse(CHECK_HEADER_R);
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("nginx/1.4.4", testHdr.GetValue("Server").c_str()) << "Wrong parameter value";
  EXPECT_STREQ(CHECK_DATE_VALUE2, testHdr.GetValue(CHECK_DATE_NAME).c_str()) << "Wrong parameter value";
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong parameter value";
  EXPECT_STREQ("150", testHdr.GetValue("Content-Length").c_str()) << "Wrong parameter value";
  EXPECT_STREQ("close", testHdr.GetValue("Connection").c_str()) << "Wrong parameter value";
  EXPECT_STREQ("http://www.Example.Com", testHdr.GetValue("Location").c_str()) << "Wrong parameter value";
  EXPECT_TRUE(testHdr.GetValue("foo").empty()) << "Some value is returned for non-existed parameter";

  /* Check that all parameters values can be retrieved in random order */
  testHdr.Parse(CHECK_HEADER_R);
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Completed header has \"parsing not finished\" state";
  EXPECT_STREQ("http://www.Example.Com", testHdr.GetValue("Location").c_str()) << "Wrong parameter value";
  EXPECT_STREQ(CHECK_CONTENT_TYPE_HTML, testHdr.GetValue(CHECK_CNT_TYPE_NAME).c_str()) << "Wrong parameter value";
  EXPECT_STREQ("http://www.Example.Com", testHdr.GetValue("Location").c_str()) << "Wrong parameter value";
  EXPECT_STREQ("close", testHdr.GetValue("Connection").c_str()) << "Wrong parameter value";
  EXPECT_STREQ("nginx/1.4.4", testHdr.GetValue("Server").c_str()) << "Wrong parameter value";
  EXPECT_STREQ("150", testHdr.GetValue("Content-Length").c_str()) << "Wrong parameter value";
  EXPECT_STREQ(CHECK_DATE_VALUE2, testHdr.GetValue(CHECK_DATE_NAME).c_str()) << "Wrong parameter value";
  EXPECT_STREQ("nginx/1.4.4", testHdr.GetValue("Server").c_str()) << "Wrong parameter value";
  EXPECT_TRUE(testHdr.GetValue("foo").empty()) << "Some value is returned for non-existed parameter";

  /* Check that parameters name is case-insensitive and value is case-sensitive*/
  EXPECT_STREQ("http://www.Example.Com", testHdr.GetValue("location").c_str()) << "Wrong parameter value for lowercase name";
  EXPECT_STREQ("http://www.Example.Com", testHdr.GetValue("LOCATION").c_str()) << "Wrong parameter value for UPPERCASE name";
  EXPECT_STREQ("http://www.Example.Com", testHdr.GetValue("LoCAtIOn").c_str()) << "Wrong parameter value for MiXEdcASe name";

  /* Check value of last added parameter with the same name is returned */
  testHdr.Parse(CHECK_HEADER_L1);
  EXPECT_STREQ("close", testHdr.GetValue("Connection").c_str()) << "Wrong parameter value";
  EXPECT_STREQ("user_country=ot; expires=Thu, 09-Jan-2014 18:58:30 GMT; path=/; domain=.example.com", testHdr.GetValue("Set-Cookie").c_str()) << "Wrong parameter value";
  EXPECT_STREQ("user_country=ot; expires=Thu, 09-Jan-2014 18:58:30 GMT; path=/; domain=.example.com", testHdr.GetValue("set-cookie").c_str()) << "Wrong parameter value for lowercase name";
}

TEST(TestHttpHeader, GetValues)
{
  CHttpHeader testHdr;

  /* Check that all parameter values can be retrieved and order of values is correct */
  testHdr.Parse(CHECK_HEADER_L1);
  EXPECT_EQ(1, testHdr.GetValues("Server").size()) << "Wrong number of values for parameter \"Server\"";
  EXPECT_STREQ("nginx/1.4.4", testHdr.GetValues("Server")[0].c_str()) << "Wrong parameter value";
  EXPECT_EQ(2, testHdr.GetValues("Set-Cookie").size()) << "Wrong number of values for parameter \"Set-Cookie\"";
  EXPECT_STREQ("PHPSESSID=90857d437518db8f0944ca012761048a; path=/; domain=example.com", testHdr.GetValues("Set-Cookie")[0].c_str()) << "Wrong parameter value";
  EXPECT_STREQ("user_country=ot; expires=Thu, 09-Jan-2014 18:58:30 GMT; path=/; domain=.example.com", testHdr.GetValues("Set-Cookie")[1].c_str()) << "Wrong parameter value";
  EXPECT_TRUE(testHdr.GetValues("foo").empty()) << "Some values are returned for non-existed parameter";
}

TEST(TestHttpHeader, AddParam)
{
  CHttpHeader testHdr;

  /* General functionality */
  testHdr.AddParam("server", "Microsoft-IIS/8.0");
  EXPECT_STREQ("Microsoft-IIS/8.0", testHdr.GetValue("Server").c_str()) << "Wrong parameter value";

  /* Interfere with parsing */
  EXPECT_FALSE(testHdr.IsHeaderDone()) << "\"AddParam\" set \"parsing finished\" state";
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nServer: nginx/1.4.4\r\n\r\n");
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Parsed header has \"parsing not finished\" state";
  EXPECT_STREQ("nginx/1.4.4", testHdr.GetValue("Server").c_str()) << "Wrong parameter value";
  testHdr.AddParam("server", "Apache/2.4.7");
  EXPECT_STREQ("Apache/2.4.7", testHdr.GetValue("Server").c_str()) << "Wrong parameter value";
  EXPECT_EQ(3, testHdr.GetValues("Server").size()) << "Wrong number of values for parameter \"Server\"";

  /* Multiple values */
  testHdr.AddParam("X-foo", "bar1");
  testHdr.AddParam("x-foo", "bar2");
  testHdr.AddParam("x-fOO", "bar3");
  EXPECT_EQ(3, testHdr.GetValues("X-FOO").size()) << "Wrong number of values for parameter \"X-foo\"";
  EXPECT_STREQ("bar1", testHdr.GetValues("X-FOo")[0].c_str()) << "Wrong parameter value";
  EXPECT_STREQ("bar2", testHdr.GetValues("X-fOo")[1].c_str()) << "Wrong parameter value";
  EXPECT_STREQ("bar3", testHdr.GetValues("x-fOo")[2].c_str()) << "Wrong parameter value";
  EXPECT_STREQ("bar3", testHdr.GetValue("x-foo").c_str()) << "Wrong parameter value";

  /* Overwrite value */
  EXPECT_TRUE(testHdr.IsHeaderDone()) << "Parsed header has \"parsing not finished\" state";
  testHdr.AddParam("x-fOO", "superbar", true);
  EXPECT_EQ(1, testHdr.GetValues("X-FoO").size()) << "Wrong number of values for parameter \"X-foo\"";
  EXPECT_STREQ("superbar", testHdr.GetValue("x-foo").c_str()) << "Wrong parameter value";

  /* Check name trimming */
  testHdr.AddParam("\tx-fOO\t ", "bar");
  EXPECT_EQ(2, testHdr.GetValues("X-FoO").size()) << "Wrong number of values for parameter \"X-foo\"";
  EXPECT_STREQ("bar", testHdr.GetValue("x-foo").c_str()) << "Wrong parameter value";
  testHdr.AddParam(" SerVer \t ", "fakeSrv", true);
  EXPECT_EQ(1, testHdr.GetValues("serveR").size()) << "Wrong number of values for parameter \"Server\"";
  EXPECT_STREQ("fakeSrv", testHdr.GetValue("Server").c_str()) << "Wrong parameter value";

  /* Check value trimming */
  testHdr.AddParam("X-TestParam", " testValue1");
  EXPECT_STREQ("testValue1", testHdr.GetValue("X-TestParam").c_str()) << "Wrong parameter value";
  testHdr.AddParam("X-TestParam", "\ttestValue2 and more \t  ");
  EXPECT_STREQ("testValue2 and more", testHdr.GetValue("X-TestParam").c_str()) << "Wrong parameter value";

  /* Empty name or value */
  testHdr.Clear();
  testHdr.AddParam("X-TestParam", "  ");
  EXPECT_TRUE(testHdr.GetHeader().empty()) << "Parameter with empty value was added";
  testHdr.AddParam("\t\t", "value");
  EXPECT_TRUE(testHdr.GetHeader().empty());
  testHdr.AddParam(" ", "\t");
  EXPECT_TRUE(testHdr.GetHeader().empty());
}

TEST(TestHttpHeader, GetMimeType)
{
  CHttpHeader testHdr;

  /* General functionality */
  EXPECT_TRUE(testHdr.GetMimeType().empty()) << "Newly created object has non-empty MIME-type";
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nServer: nginx/1.4.4\r\n\r\n");
  EXPECT_TRUE(testHdr.GetMimeType().empty()) << "Non-empty MIME-type for header without MIME-type";
  testHdr.Parse(CHECK_HEADER_SMPL);
  EXPECT_STREQ("text/html", testHdr.GetMimeType().c_str()) << "Wrong MIME-type";
  testHdr.Parse(CHECK_HEADER_L1);
  EXPECT_STREQ("text/html", testHdr.GetMimeType().c_str()) << "Wrong MIME-type";
  testHdr.Parse(CHECK_HEADER_L2);
  EXPECT_STREQ("text/xml", testHdr.GetMimeType().c_str()) << "Wrong MIME-type";
  testHdr.Parse(CHECK_HEADER_R);
  EXPECT_STREQ("text/html", testHdr.GetMimeType().c_str()) << "Wrong MIME-type";

  /* Overwrite by AddParam */
  testHdr.AddParam(CHECK_CNT_TYPE_NAME, CHECK_CONTENT_TYPE_TEXT);
  EXPECT_STREQ(CHECK_CONTENT_TYPE_TEXT, testHdr.GetMimeType().c_str()) << "MIME-type was not overwritten by \"AddParam\"";

  /* Correct trimming */
  testHdr.AddParam(CHECK_CNT_TYPE_NAME, "  " CHECK_CONTENT_TYPE_TEXT " \t ;foo=bar");
  EXPECT_STREQ(CHECK_CONTENT_TYPE_TEXT, testHdr.GetMimeType().c_str()) << "MIME-type is not trimmed correctly";
}


TEST(TestHttpHeader, GetCharset)
{
  CHttpHeader testHdr;

  /* General functionality */
  EXPECT_TRUE(testHdr.GetCharset().empty()) << "Newly created object has non-empty charset";
  testHdr.Parse(CHECK_PROT_LINE_200 "\r\nServer: nginx/1.4.4\r\n\r\n");
  EXPECT_TRUE(testHdr.GetCharset().empty()) << "Non-empty charset for header without charset";
  testHdr.Parse(CHECK_HEADER_SMPL);
  EXPECT_TRUE(testHdr.GetCharset().empty()) << "Non-empty charset for header without charset";
  testHdr.Parse(CHECK_HEADER_L1);
  EXPECT_STREQ("WINDOWS-1251", testHdr.GetCharset().c_str()) << "Wrong charset value";
  testHdr.Parse(CHECK_HEADER_L2);
  EXPECT_STREQ("UTF-8", testHdr.GetCharset().c_str()) << "Wrong charset value";

  /* Overwrite by AddParam */
  testHdr.AddParam(CHECK_CNT_TYPE_NAME, CHECK_CONTENT_TYPE_TEXT "; charset=WINDOWS-1252");
  EXPECT_STREQ("WINDOWS-1252", testHdr.GetCharset().c_str()) << "Charset was not overwritten by \"AddParam\"";

  /* Correct trimming */
  testHdr.AddParam(CHECK_CNT_TYPE_NAME, "text/plain;charset=WINDOWS-1251");
  EXPECT_STREQ("WINDOWS-1251", testHdr.GetCharset().c_str()) << "Wrong charset value";
  testHdr.AddParam(CHECK_CNT_TYPE_NAME, "text/plain ;\tcharset=US-AScII\t");
  EXPECT_STREQ("US-ASCII", testHdr.GetCharset().c_str()) << "Wrong charset value";
  testHdr.AddParam(CHECK_CNT_TYPE_NAME, "text/html ; \tcharset=\"uTF-8\"\t");
  EXPECT_STREQ("UTF-8", testHdr.GetCharset().c_str()) << "Wrong charset value";
  testHdr.AddParam(CHECK_CNT_TYPE_NAME, " \ttext/xml\t;\tcharset=uTF-16  ");
  EXPECT_STREQ("UTF-16", testHdr.GetCharset().c_str()) << "Wrong charset value";
}
