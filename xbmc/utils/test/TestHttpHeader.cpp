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

#include <sstream>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "utils/HttpHeader.h"
#include "gtest/gtest.h"

using ::testing::ValuesIn;

namespace
{

class TestHttpHeader :
  public ::testing::Test
{
public:

  TestHttpHeader();

  CHttpHeader& Header();

private:

  std::string m_ReferenceString;
  CHttpHeader m_Header;
};

struct HeaderTokenData
{
  const char *tag;
  const char *data;
} headerTokenData[] =
{
  { "Host", "xbmc.org" },
  { "Accept", "text/*, text/html, text/html;level=1, */*" },
  { "Accept-Language", "en" },
  { "Accept-Encoding", "gzip, deflate" },
  { "Content-Type", "text/html; charset=ISO-8859-4" },
  { "User-Agent", "XBMC/snapshot (compatible; MSIE 5.5; Windows NT 4.0)" },
  { "Connection", "Keep-Alive" }
};

const char *headerResponse = "HTTP/1.1 200 OK";

typedef boost::function<void(const char *, std::string &)> HeaderModifier;

/* The HeaderModifier parameter allows the user of this function to pass
 * a boost::function object to be called for each data member of the
 * predetermiend header as specified in headerTokenData. This allows
 * the user to modify parts of that data while it is being added to this
 * stream, for instance, in order to test how the HttpHeader class handles
 * different types and formats of data for certain tokens without having
 * to re-specify all the tokens */
std::string
HeaderTokenDataInputString(HeaderModifier modifier = HeaderModifier())
{
  std::stringstream headerData;

  headerData << headerResponse << "\r\n";

  const size_t headerTokenDataSize = sizeof (headerTokenData) / sizeof (HeaderTokenData);
  for (size_t i = 0; i < headerTokenDataSize; ++i)
  {
    std::string headerTokenDataForTag(headerTokenData[i].data);
    if (!modifier.empty())
      modifier(headerTokenData[i].tag, headerTokenDataForTag);

    headerData << headerTokenData[i].tag
               << ": "
               << headerTokenDataForTag
               << "\r\n";
  }

  return headerData.str();
}

CHttpHeader &
TestHttpHeader::Header()
{
  return m_Header;
}

TestHttpHeader::TestHttpHeader() :
  m_ReferenceString(HeaderTokenDataInputString())
{
  m_Header.Parse(m_ReferenceString);
}

TEST_F(TestHttpHeader, ParseToExpectedHeader)
{
  const char *refstr = "HTTP/1.1 200 OK\n"
                       "host: xbmc.org\n"
                       "accept: text/*, text/html, text/html;level=1, */*\n"
                       "accept-language: en\n"
                       "accept-encoding: gzip, deflate\n"
                       "content-type: text/html; charset=ISO-8859-4\n"
                       "user-agent: XBMC/snapshot (compatible; MSIE 5.5; Windows NT 4.0)\n"
                       "connection: Keep-Alive\n"
                       "\n";

  /* Should be in the same order as above */
  EXPECT_STREQ(refstr, Header().GetHeader().c_str());
}

class TestHttpHeaderParameterizedByTags :
  public TestHttpHeader,
  public ::testing::WithParamInterface<HeaderTokenData>
{
};

/* These parameterized tests check that fetching each specified tag
 * in the header data always returns the same data as the input data */
TEST_P(TestHttpHeaderParameterizedByTags, GetValue)
{
  const HeaderTokenData &headerData(GetParam());
  EXPECT_STREQ(headerData.data, Header().GetValue(headerData.tag).c_str());
}

/* Check that once we've cleared the header, calling GetValue always
 * returns an empty string */
TEST_P(TestHttpHeaderParameterizedByTags, GetValueAfterClear)
{
  Header().Clear();
  const HeaderTokenData &headerData(GetParam());
  EXPECT_STREQ("", Header().GetValue(headerData.tag).c_str());
}

INSTANTIATE_TEST_CASE_P(XBMCHostData, TestHttpHeaderParameterizedByTags,
                        ValuesIn(headerTokenData));

typedef std::string (CHttpHeader::*GetFunction) () const;

class TestHttpHeaderParameterizeByMemberFunction :
  public TestHttpHeader,
  public ::testing::WithParamInterface<GetFunction>
{
};

/* Check that once we've cleared the header, calling any functions which
 * would get pre-defined data about this header (such as the charset, mimetype, etc)
 * also return an empty string */
TEST_P(TestHttpHeaderParameterizeByMemberFunction, GetFunctionAfterClearReturnsEmptyString)
{
  GetFunction function(GetParam());

  Header().Clear();

  const char *result = (Header().*function) ().c_str();
  EXPECT_STREQ("", result);
}

GetFunction getFunctions[] =
{
  &CHttpHeader::GetHeader,
  &CHttpHeader::GetMimeType,
  &CHttpHeader::GetCharset,
  &CHttpHeader::GetProtoLine
};

INSTANTIATE_TEST_CASE_P(CHttpHeaderMemberMethods, TestHttpHeaderParameterizeByMemberFunction,
                        ValuesIn(getFunctions));

TEST_F(TestHttpHeader, GetMimeTypeHasNoCharsetTags)
{
  EXPECT_STREQ("text/html", Header().GetMimeType().c_str());
}

TEST_F(TestHttpHeader, NoneOnGetGetProtoLine)
{
  EXPECT_STREQ(headerResponse, Header().GetProtoLine().c_str());
}

void InsertForContentType(const char *tag, std::string &data, const char *replace)
{
  if (strcmp(tag, "content-type"))
    return;

  data = replace;
}

/* HttpHeader handles the "charset" token in the "content-type" tag to either have
 * one space from the semicolon, or two. Test both */
TEST_F(TestHttpHeader, GetCharsetSingleSpace)
{
  Header().Parse(HeaderTokenDataInputString(boost::bind(InsertForContentType, _1, _2,
                                                        "text/html; charset=ISO-8859-4")));

  EXPECT_STREQ("ISO-8859-4", Header().GetCharset().c_str());
}

TEST_F(TestHttpHeader, GetCharsetNoSpace)
{
  Header().Parse(HeaderTokenDataInputString(boost::bind(InsertForContentType, _1, _2,
                                                        "text/html;charset=ISO-8859-4")));

  EXPECT_STREQ("ISO-8859-4", Header().GetCharset().c_str());
}

TEST_F(TestHttpHeader, AddParam)
{
  const char *tag = "custom";
  const char *value = "mock";
  Header().AddParam(tag, value);

  EXPECT_STREQ(value, Header().GetValue("custom").c_str());
}

TEST_F(TestHttpHeader, AddParamReplacesOld)
{
  const char *tag = "custom";
  const char *value = "mock";
  Header().AddParam(tag, value);

  const char *nextValue = "newmock";
  Header().AddParam(tag, nextValue);

  EXPECT_STREQ(nextValue, Header().GetValue("custom").c_str());
}

}
