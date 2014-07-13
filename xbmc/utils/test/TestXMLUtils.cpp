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

#include "utils/XMLUtils.h"
#include "utils/StringUtils.h"
#include "XBDateTime.h"

#include "gtest/gtest.h"

TEST(TestXMLUtils, GetHex)
{
  CXBMCTinyXML a;
  uint32_t ref, val;

  a.Parse("<root><node>0xFF</node></root>");
  EXPECT_TRUE(XMLUtils::GetHex(a.RootElement(), "node", val));

  ref = 0xFF;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetUInt)
{
  CXBMCTinyXML a;
  uint32_t ref, val;

  a.Parse("<root><node>1000</node></root>");
  EXPECT_TRUE(XMLUtils::GetUInt(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetLong)
{
  CXBMCTinyXML a;
  long ref, val;

  a.Parse("<root><node>1000</node></root>");
  EXPECT_TRUE(XMLUtils::GetLong(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetFloat)
{
  CXBMCTinyXML a;
  float ref, val;

  a.Parse("<root><node>1000.1f</node></root>");
  EXPECT_TRUE(XMLUtils::GetFloat(a.RootElement(), "node", val));
  EXPECT_TRUE(XMLUtils::GetFloat(a.RootElement(), "node", val, 1000.0f,
                                 1000.2f));
  ref = 1000.1f;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetDouble)
{
  CXBMCTinyXML a;
  double val;
  std::string refstr, valstr;

  a.Parse("<root><node>1000.1f</node></root>");
  EXPECT_TRUE(XMLUtils::GetDouble(a.RootElement(), "node", val));

  refstr = "1000.100000";
  valstr = StringUtils::Format("%f", val);
  EXPECT_STREQ(refstr.c_str(), valstr.c_str());
}

TEST(TestXMLUtils, GetInt)
{
  CXBMCTinyXML a;
  int ref, val;

  a.Parse("<root><node>1000</node></root>");
  EXPECT_TRUE(XMLUtils::GetInt(a.RootElement(), "node", val));
  EXPECT_TRUE(XMLUtils::GetInt(a.RootElement(), "node", val, 999, 1001));

  ref = 1000;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetBoolean)
{
  CXBMCTinyXML a;
  bool ref, val;

  a.Parse("<root><node>true</node></root>");
  EXPECT_TRUE(XMLUtils::GetBoolean(a.RootElement(), "node", val));

  ref = true;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetString)
{
  CXBMCTinyXML a;
  std::string ref, val;

  a.Parse("<root><node>some string</node></root>");
  EXPECT_TRUE(XMLUtils::GetString(a.RootElement(), "node", val));

  ref = "some string";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, GetAdditiveString)
{
  CXBMCTinyXML a, b;
  std::string ref, val;

  a.Parse("<root>\n"
          "  <node>some string1</node>\n"
          "  <node>some string2</node>\n"
          "  <node>some string3</node>\n"
          "  <node>some string4</node>\n"
          "  <node>some string5</node>\n"
          "</root>\n");
  EXPECT_TRUE(XMLUtils::GetAdditiveString(a.RootElement(), "node", ",", val));

  ref = "some string1,some string2,some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  val.clear();
  b.Parse("<root>\n"
          "  <node>some string1</node>\n"
          "  <node>some string2</node>\n"
          "  <node clear=\"true\">some string3</node>\n"
          "  <node>some string4</node>\n"
          "  <node>some string5</node>\n"
          "</root>\n");
  EXPECT_TRUE(XMLUtils::GetAdditiveString(b.RootElement(), "node", ",", val));

  ref = "some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, GetStringArray)
{
  CXBMCTinyXML a;
  std::vector<std::string> strarray;

  a.Parse("<root>\n"
          "  <node>some string1</node>\n"
          "  <node>some string2</node>\n"
          "  <node>some string3</node>\n"
          "  <node>some string4</node>\n"
          "  <node>some string5</node>\n"
          "</root>\n");
  EXPECT_TRUE(XMLUtils::GetStringArray(a.RootElement(), "node", strarray));

  EXPECT_STREQ("some string1", strarray.at(0).c_str());
  EXPECT_STREQ("some string2", strarray.at(1).c_str());
  EXPECT_STREQ("some string3", strarray.at(2).c_str());
  EXPECT_STREQ("some string4", strarray.at(3).c_str());
  EXPECT_STREQ("some string5", strarray.at(4).c_str());
}

TEST(TestXMLUtils, GetPath)
{
  CXBMCTinyXML a, b;
  std::string ref, val;

  a.Parse("<root><node urlencoded=\"yes\">special://xbmc/</node></root>");
  EXPECT_TRUE(XMLUtils::GetPath(a.RootElement(), "node", val));

  ref = "special://xbmc/";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  val.clear();
  b.Parse("<root><node>special://xbmcbin/</node></root>");
  EXPECT_TRUE(XMLUtils::GetPath(b.RootElement(), "node", val));

  ref = "special://xbmcbin/";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, GetDate)
{
  CXBMCTinyXML a;
  CDateTime ref, val;

  a.Parse("<root><node>2012-07-08</node></root>");
  EXPECT_TRUE(XMLUtils::GetDate(a.RootElement(), "node", val));
  ref.SetDate(2012, 7, 8);
  EXPECT_TRUE(ref == val);
}

TEST(TestXMLUtils, GetDateTime)
{
  CXBMCTinyXML a;
  CDateTime ref, val;

  a.Parse("<root><node>2012-07-08 01:02:03</node></root>");
  EXPECT_TRUE(XMLUtils::GetDateTime(a.RootElement(), "node", val));
  ref.SetDateTime(2012, 7, 8, 1, 2, 3);
  EXPECT_TRUE(ref == val);
}

TEST(TestXMLUtils, SetString)
{
  CXBMCTinyXML a;
  std::string ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetString(a.RootElement(), "node", "some string");
  EXPECT_TRUE(XMLUtils::GetString(a.RootElement(), "node", val));

  ref = "some string";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, SetAdditiveString)
{
  CXBMCTinyXML a;
  std::string ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetAdditiveString(a.RootElement(), "node", ",",
    "some string1,some string2,some string3,some string4,some string5");
  EXPECT_TRUE(XMLUtils::GetAdditiveString(a.RootElement(), "node", ",", val));

  ref = "some string1,some string2,some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, SetStringArray)
{
  CXBMCTinyXML a;
  std::vector<std::string> strarray;
  strarray.push_back("some string1");
  strarray.push_back("some string2");
  strarray.push_back("some string3");
  strarray.push_back("some string4");
  strarray.push_back("some string5");

  a.Parse("<root></root>");
  XMLUtils::SetStringArray(a.RootElement(), "node", strarray);
  EXPECT_TRUE(XMLUtils::GetStringArray(a.RootElement(), "node", strarray));

  EXPECT_STREQ("some string1", strarray.at(0).c_str());
  EXPECT_STREQ("some string2", strarray.at(1).c_str());
  EXPECT_STREQ("some string3", strarray.at(2).c_str());
  EXPECT_STREQ("some string4", strarray.at(3).c_str());
  EXPECT_STREQ("some string5", strarray.at(4).c_str());
}

TEST(TestXMLUtils, SetInt)
{
  CXBMCTinyXML a;
  int ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetInt(a.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetInt(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetFloat)
{
  CXBMCTinyXML a;
  float ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetFloat(a.RootElement(), "node", 1000.1f);
  EXPECT_TRUE(XMLUtils::GetFloat(a.RootElement(), "node", val));

  ref = 1000.1f;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetBoolean)
{
  CXBMCTinyXML a;
  bool ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetBoolean(a.RootElement(), "node", true);
  EXPECT_TRUE(XMLUtils::GetBoolean(a.RootElement(), "node", val));

  ref = true;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetHex)
{
  CXBMCTinyXML a;
  uint32_t ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetHex(a.RootElement(), "node", 0xFF);
  EXPECT_TRUE(XMLUtils::GetHex(a.RootElement(), "node", val));

  ref = 0xFF;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetPath)
{
  CXBMCTinyXML a;
  std::string ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetPath(a.RootElement(), "node", "special://xbmc/");
  EXPECT_TRUE(XMLUtils::GetPath(a.RootElement(), "node", val));

  ref = "special://xbmc/";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, SetLong)
{
  CXBMCTinyXML a;
  long ref, val;

  a.Parse("<root></root>");
  XMLUtils::SetLong(a.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetLong(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetDate)
{
  CXBMCTinyXML a;
  CDateTime ref, val;

  a.Parse("<root></root>");
  ref.SetDate(2012, 7, 8);
  XMLUtils::SetDate(a.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDate(a.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);
}

TEST(TestXMLUtils, SetDateTime)
{
  CXBMCTinyXML a;
  CDateTime ref, val;

  a.Parse("<root></root>");
  ref.SetDateTime(2012, 7, 8, 1, 2, 3);
  XMLUtils::SetDateTime(a.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDateTime(a.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);
}
