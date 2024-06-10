/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBDateTime.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"

#include <gtest/gtest.h>

TEST(TestXMLUtils, GetHex)
{
  uint32_t ref, val;

  CXBMCTinyXML2 b;

  ref = 0xFF;
  b.Parse(std::string("<root><node>0xFF</node></root>"));
  EXPECT_TRUE(XMLUtils::GetHex(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetUInt)
{
  uint32_t ref, val;

  CXBMCTinyXML2 b;
  ref = 1000;

  b.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetUInt(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetLong)
{
  long ref, val;

  CXBMCTinyXML2 b;
  ref = 1000;

  b.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetLong(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetFloat)
{
  float ref, val;

  CXBMCTinyXML2 b;
  ref = 1000.1f;

  b.Parse(std::string("<root><node>1000.1f</node></root>"));
  EXPECT_TRUE(XMLUtils::GetFloat(b.RootElement(), "node", val));
  EXPECT_TRUE(XMLUtils::GetFloat(b.RootElement(), "node", val, 1000.0f, 1000.2f));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetDouble)
{
  double val;
  std::string refstr, valstr;

  CXBMCTinyXML2 b;
  refstr = "1000.100000";

  b.Parse(std::string("<root><node>1000.1f</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDouble(b.RootElement(), "node", val));

  valstr = StringUtils::Format("{:f}", val);
  EXPECT_STREQ(refstr.c_str(), valstr.c_str());
}

TEST(TestXMLUtils, GetInt)
{
  int ref, val;
  CXBMCTinyXML2 b;
  ref = 1000;

  b.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetInt(b.RootElement(), "node", val));
  EXPECT_TRUE(XMLUtils::GetInt(b.RootElement(), "node", val, 999, 1001));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetBoolean)
{
  bool ref, val;

  CXBMCTinyXML2 b;
  ref = true;

  b.Parse(std::string("<root><node>true</node></root>"));
  EXPECT_TRUE(XMLUtils::GetBoolean(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetString)
{
  std::string ref, val;
  CXBMCTinyXML2 b;
  ref = "some string";

  b.Parse(std::string("<root><node>some string</node></root>"));
  EXPECT_TRUE(XMLUtils::GetString(b.RootElement(), "node", val));

  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, GetAdditiveString)
{
  std::string ref, val;

  CXBMCTinyXML2 c, d;

  c.Parse(std::string("<root>\n"
                      "  <node>some string1</node>\n"
                      "  <node>some string2</node>\n"
                      "  <node>some string3</node>\n"
                      "  <node>some string4</node>\n"
                      "  <node>some string5</node>\n"
                      "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetAdditiveString(c.RootElement(), "node", ",", val));

  ref = "some string1,some string2,some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  val.clear();
  d.Parse(std::string("<root>\n"
                      "  <node>some string1</node>\n"
                      "  <node>some string2</node>\n"
                      "  <node clear=\"true\">some string3</node>\n"
                      "  <node>some string4</node>\n"
                      "  <node>some string5</node>\n"
                      "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetAdditiveString(d.RootElement(), "node", ",", val));

  ref = "some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, GetStringArray)
{
  std::vector<std::string> strarray;

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root>\n"
                      "  <node>some string1</node>\n"
                      "  <node>some string2</node>\n"
                      "  <node>some string3</node>\n"
                      "  <node>some string4</node>\n"
                      "  <node>some string5</node>\n"
                      "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetStringArray(b.RootElement(), "node", strarray));

  EXPECT_STREQ("some string1", strarray.at(0).c_str());
  EXPECT_STREQ("some string2", strarray.at(1).c_str());
  EXPECT_STREQ("some string3", strarray.at(2).c_str());
  EXPECT_STREQ("some string4", strarray.at(3).c_str());
  EXPECT_STREQ("some string5", strarray.at(4).c_str());
}

TEST(TestXMLUtils, GetPath)
{
  std::string ref, val;

  CXBMCTinyXML2 c, d;

  c.Parse(std::string("<root><node urlencoded=\"yes\">special://xbmc/</node></root>"));
  EXPECT_TRUE(XMLUtils::GetPath(c.RootElement(), "node", val));

  ref = "special://xbmc/";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  val.clear();
  d.Parse(std::string("<root><node>special://xbmcbin/</node></root>"));
  EXPECT_TRUE(XMLUtils::GetPath(d.RootElement(), "node", val));

  ref = "special://xbmcbin/";
  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, GetDate)
{
  CDateTime ref, val;

  CXBMCTinyXML2 b;
  ref.SetDate(2012, 7, 8);

  b.Parse(std::string("<root><node>2012-07-08</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDate(b.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);
}

TEST(TestXMLUtils, GetDateTime)
{
  CDateTime ref, val;

  CXBMCTinyXML2 b;
  ref.SetDateTime(2012, 7, 8, 1, 2, 3);

  b.Parse(std::string("<root><node>2012-07-08 01:02:03</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDateTime(b.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);
}

TEST(TestXMLUtils, SetString)
{
  std::string ref, val;
  CXBMCTinyXML2 b;
  ref = "some string";

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetString(b.RootElement(), "node", "some string");
  EXPECT_TRUE(XMLUtils::GetString(b.RootElement(), "node", val));

  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, SetAdditiveString)
{
  std::string ref, val;
  CXBMCTinyXML2 b;
  ref = "some string1,some string2,some string3,some string4,some string5";

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetAdditiveString(b.RootElement(), "node", ",",
                              "some string1,some string2,some string3,some string4,some string5");
  EXPECT_TRUE(XMLUtils::GetAdditiveString(b.RootElement(), "node", ",", val));

  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, SetStringArray)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("some string1");
  strarray.emplace_back("some string2");
  strarray.emplace_back("some string3");
  strarray.emplace_back("some string4");
  strarray.emplace_back("some string5");

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetStringArray(b.RootElement(), "node", strarray);
  EXPECT_TRUE(XMLUtils::GetStringArray(b.RootElement(), "node", strarray));

  EXPECT_STREQ("some string1", strarray.at(0).c_str());
  EXPECT_STREQ("some string2", strarray.at(1).c_str());
  EXPECT_STREQ("some string3", strarray.at(2).c_str());
  EXPECT_STREQ("some string4", strarray.at(3).c_str());
  EXPECT_STREQ("some string5", strarray.at(4).c_str());
}

TEST(TestXMLUtils, SetInt)
{
  int ref, val;
  CXBMCTinyXML2 b;
  ref = 1000;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetInt(b.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetInt(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetFloat)
{
  float ref, val;
  CXBMCTinyXML2 b;
  ref = 1000.1f;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetFloat(b.RootElement(), "node", 1000.1f);
  EXPECT_TRUE(XMLUtils::GetFloat(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetBoolean)
{
  bool ref, val;
  CXBMCTinyXML2 b;
  ref = true;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetBoolean(b.RootElement(), "node", true);
  EXPECT_TRUE(XMLUtils::GetBoolean(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetHex)
{
  uint32_t ref, val;
  CXBMCTinyXML2 b;
  ref = 0xFF;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetHex(b.RootElement(), "node", 0xFF);
  EXPECT_TRUE(XMLUtils::GetHex(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetPath)
{
  std::string ref, val;
  CXBMCTinyXML2 b;
  ref = "special://xbmc/";

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetPath(b.RootElement(), "node", "special://xbmc/");
  EXPECT_TRUE(XMLUtils::GetPath(b.RootElement(), "node", val));

  EXPECT_STREQ(ref.c_str(), val.c_str());
}

TEST(TestXMLUtils, SetLong)
{
  long ref, val;
  CXBMCTinyXML2 b;
  ref = 1000;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetLong(b.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetLong(b.RootElement(), "node", val));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, SetDate)
{
  CDateTime ref, val;
  CXBMCTinyXML2 b;
  ref.SetDate(2012, 7, 8);

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetDate(b.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDate(b.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);
}

TEST(TestXMLUtils, SetDateTime)
{
  CDateTime ref, val;
  CXBMCTinyXML2 b;
  ref.SetDateTime(2012, 7, 8, 1, 2, 3);

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetDateTime(b.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDateTime(b.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);
}
