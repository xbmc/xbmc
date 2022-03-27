/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBDateTime.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"

#include <gtest/gtest.h>

TEST(TestXMLUtils, GetHex)
{
  CXBMCTinyXML a;
  uint32_t ref, val, val2;

  a.Parse(std::string("<root><node>0xFF</node></root>"));
  EXPECT_TRUE(XMLUtils::GetHex(a.RootElement(), "node", val));

  ref = 0xFF;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>0xFF</node></root>"));
  EXPECT_TRUE(XMLUtils::GetHex(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, GetUInt)
{
  CXBMCTinyXML a;
  uint32_t ref, val, val2;

  a.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetUInt(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetUInt(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, GetLong)
{
  CXBMCTinyXML a;
  long ref, val, val2;

  a.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetLong(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetLong(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, GetFloat)
{
  CXBMCTinyXML a;
  float ref, val, val2;

  a.Parse(std::string("<root><node>1000.1f</node></root>"));
  EXPECT_TRUE(XMLUtils::GetFloat(a.RootElement(), "node", val));
  EXPECT_TRUE(XMLUtils::GetFloat(a.RootElement(), "node", val, 1000.0f,
                                 1000.2f));
  ref = 1000.1f;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>1000.1f</node></root>"));
  EXPECT_TRUE(XMLUtils::GetFloat(b.RootElement(), "node", val2));
  EXPECT_TRUE(XMLUtils::GetFloat(b.RootElement(), "node", val2, 1000.0f, 1000.2f));

  EXPECT_EQ(ref, val);
}

TEST(TestXMLUtils, GetDouble)
{
  CXBMCTinyXML a;
  double val, val2;
  std::string refstr, valstr, valstr2;

  a.Parse(std::string("<root><node>1000.1f</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDouble(a.RootElement(), "node", val));

  refstr = "1000.100000";
  valstr = StringUtils::Format("{:f}", val);
  EXPECT_STREQ(refstr.c_str(), valstr.c_str());

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>1000.1f</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDouble(b.RootElement(), "node", val2));

  valstr2 = StringUtils::Format("{:f}", val2);
  EXPECT_STREQ(refstr.c_str(), valstr2.c_str());
}

TEST(TestXMLUtils, GetInt)
{
  CXBMCTinyXML a;
  int ref, val, val2;

  a.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetInt(a.RootElement(), "node", val));
  EXPECT_TRUE(XMLUtils::GetInt(a.RootElement(), "node", val, 999, 1001));

  ref = 1000;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>1000</node></root>"));
  EXPECT_TRUE(XMLUtils::GetInt(b.RootElement(), "node", val2));
  EXPECT_TRUE(XMLUtils::GetInt(b.RootElement(), "node", val2, 999, 1001));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, GetBoolean)
{
  CXBMCTinyXML a;
  bool ref, val, val2;

  a.Parse(std::string("<root><node>true</node></root>"));
  EXPECT_TRUE(XMLUtils::GetBoolean(a.RootElement(), "node", val));

  ref = true;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>true</node></root>"));
  EXPECT_TRUE(XMLUtils::GetBoolean(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, GetString)
{
  CXBMCTinyXML a;
  std::string ref, val, val2;

  a.Parse(std::string("<root><node>some string</node></root>"));
  EXPECT_TRUE(XMLUtils::GetString(a.RootElement(), "node", val));

  ref = "some string";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>some string</node></root>"));
  EXPECT_TRUE(XMLUtils::GetString(b.RootElement(), "node", val2));

  EXPECT_STREQ(ref.c_str(), val2.c_str());
}

TEST(TestXMLUtils, GetAdditiveString)
{
  CXBMCTinyXML a, b;
  std::string ref, val, val2;

  a.Parse(std::string("<root>\n"
          "  <node>some string1</node>\n"
          "  <node>some string2</node>\n"
          "  <node>some string3</node>\n"
          "  <node>some string4</node>\n"
          "  <node>some string5</node>\n"
          "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetAdditiveString(a.RootElement(), "node", ",", val));

  ref = "some string1,some string2,some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  val.clear();
  b.Parse(std::string("<root>\n"
          "  <node>some string1</node>\n"
          "  <node>some string2</node>\n"
          "  <node clear=\"true\">some string3</node>\n"
          "  <node>some string4</node>\n"
          "  <node>some string5</node>\n"
          "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetAdditiveString(b.RootElement(), "node", ",", val));

  ref = "some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  CXBMCTinyXML2 c, d;

  c.Parse(std::string("<root>\n"
                      "  <node>some string1</node>\n"
                      "  <node>some string2</node>\n"
                      "  <node>some string3</node>\n"
                      "  <node>some string4</node>\n"
                      "  <node>some string5</node>\n"
                      "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetAdditiveString(c.RootElement(), "node", ",", val2));

  ref = "some string1,some string2,some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val2.c_str());

  val2.clear();
  d.Parse(std::string("<root>\n"
                      "  <node>some string1</node>\n"
                      "  <node>some string2</node>\n"
                      "  <node clear=\"true\">some string3</node>\n"
                      "  <node>some string4</node>\n"
                      "  <node>some string5</node>\n"
                      "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetAdditiveString(d.RootElement(), "node", ",", val2));

  ref = "some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val2.c_str());
}

TEST(TestXMLUtils, GetStringArray)
{
  CXBMCTinyXML a;
  std::vector<std::string> strarray, strarray2;

  a.Parse(std::string("<root>\n"
          "  <node>some string1</node>\n"
          "  <node>some string2</node>\n"
          "  <node>some string3</node>\n"
          "  <node>some string4</node>\n"
          "  <node>some string5</node>\n"
          "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetStringArray(a.RootElement(), "node", strarray));

  EXPECT_STREQ("some string1", strarray.at(0).c_str());
  EXPECT_STREQ("some string2", strarray.at(1).c_str());
  EXPECT_STREQ("some string3", strarray.at(2).c_str());
  EXPECT_STREQ("some string4", strarray.at(3).c_str());
  EXPECT_STREQ("some string5", strarray.at(4).c_str());

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root>\n"
                      "  <node>some string1</node>\n"
                      "  <node>some string2</node>\n"
                      "  <node>some string3</node>\n"
                      "  <node>some string4</node>\n"
                      "  <node>some string5</node>\n"
                      "</root>\n"));
  EXPECT_TRUE(XMLUtils::GetStringArray(b.RootElement(), "node", strarray2));

  EXPECT_STREQ("some string1", strarray2.at(0).c_str());
  EXPECT_STREQ("some string2", strarray2.at(1).c_str());
  EXPECT_STREQ("some string3", strarray2.at(2).c_str());
  EXPECT_STREQ("some string4", strarray2.at(3).c_str());
  EXPECT_STREQ("some string5", strarray2.at(4).c_str());
}

TEST(TestXMLUtils, GetPath)
{
  CXBMCTinyXML a, b;
  std::string ref, val, val2;

  a.Parse(std::string("<root><node urlencoded=\"yes\">special://xbmc/</node></root>"));
  EXPECT_TRUE(XMLUtils::GetPath(a.RootElement(), "node", val));

  ref = "special://xbmc/";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  val.clear();
  b.Parse(std::string("<root><node>special://xbmcbin/</node></root>"));
  EXPECT_TRUE(XMLUtils::GetPath(b.RootElement(), "node", val));

  ref = "special://xbmcbin/";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  CXBMCTinyXML2 c, d;

  c.Parse(std::string("<root><node urlencoded=\"yes\">special://xbmc/</node></root>"));
  EXPECT_TRUE(XMLUtils::GetPath(c.RootElement(), "node", val2));

  ref = "special://xbmc/";
  EXPECT_STREQ(ref.c_str(), val2.c_str());

  val2.clear();
  d.Parse(std::string("<root><node>special://xbmcbin/</node></root>"));
  EXPECT_TRUE(XMLUtils::GetPath(d.RootElement(), "node", val2));

  ref = "special://xbmcbin/";
  EXPECT_STREQ(ref.c_str(), val2.c_str());
}

TEST(TestXMLUtils, GetDate)
{
  CXBMCTinyXML a;
  CDateTime ref, val, val2;

  a.Parse(std::string("<root><node>2012-07-08</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDate(a.RootElement(), "node", val));
  ref.SetDate(2012, 7, 8);
  EXPECT_TRUE(ref == val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>2012-07-08</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDate(b.RootElement(), "node", val2));
  EXPECT_TRUE(ref == val2);
}

TEST(TestXMLUtils, GetDateTime)
{
  CXBMCTinyXML a;
  CDateTime ref, val, val2;

  a.Parse(std::string("<root><node>2012-07-08 01:02:03</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDateTime(a.RootElement(), "node", val));
  ref.SetDateTime(2012, 7, 8, 1, 2, 3);
  EXPECT_TRUE(ref == val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root><node>2012-07-08 01:02:03</node></root>"));
  EXPECT_TRUE(XMLUtils::GetDateTime(b.RootElement(), "node", val2));
  EXPECT_TRUE(ref == val2);
}

TEST(TestXMLUtils, SetString)
{
  CXBMCTinyXML a;
  std::string ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetString(a.RootElement(), "node", "some string");
  EXPECT_TRUE(XMLUtils::GetString(a.RootElement(), "node", val));

  ref = "some string";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetString(b.RootElement(), "node", "some string");
  EXPECT_TRUE(XMLUtils::GetString(b.RootElement(), "node", val2));

  EXPECT_STREQ(ref.c_str(), val2.c_str());
}

TEST(TestXMLUtils, SetAdditiveString)
{
  CXBMCTinyXML a;
  std::string ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetAdditiveString(a.RootElement(), "node", ",",
    "some string1,some string2,some string3,some string4,some string5");
  EXPECT_TRUE(XMLUtils::GetAdditiveString(a.RootElement(), "node", ",", val));

  ref = "some string1,some string2,some string3,some string4,some string5";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetAdditiveString(b.RootElement(), "node", ",",
                              "some string1,some string2,some string3,some string4,some string5");
  EXPECT_TRUE(XMLUtils::GetAdditiveString(b.RootElement(), "node", ",", val2));

  EXPECT_STREQ(ref.c_str(), val2.c_str());
}

TEST(TestXMLUtils, SetStringArray)
{
  CXBMCTinyXML a;
  std::vector<std::string> strarray;
  strarray.emplace_back("some string1");
  strarray.emplace_back("some string2");
  strarray.emplace_back("some string3");
  strarray.emplace_back("some string4");
  strarray.emplace_back("some string5");

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetStringArray(a.RootElement(), "node", strarray);
  EXPECT_TRUE(XMLUtils::GetStringArray(a.RootElement(), "node", strarray));

  EXPECT_STREQ("some string1", strarray.at(0).c_str());
  EXPECT_STREQ("some string2", strarray.at(1).c_str());
  EXPECT_STREQ("some string3", strarray.at(2).c_str());
  EXPECT_STREQ("some string4", strarray.at(3).c_str());
  EXPECT_STREQ("some string5", strarray.at(4).c_str());

  CXBMCTinyXML2 b;

  strarray.clear();
  strarray.emplace_back("some string1");
  strarray.emplace_back("some string2");
  strarray.emplace_back("some string3");
  strarray.emplace_back("some string4");
  strarray.emplace_back("some string5");

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
  CXBMCTinyXML a;
  int ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetInt(a.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetInt(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetInt(b.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetInt(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, SetFloat)
{
  CXBMCTinyXML a;
  float ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetFloat(a.RootElement(), "node", 1000.1f);
  EXPECT_TRUE(XMLUtils::GetFloat(a.RootElement(), "node", val));

  ref = 1000.1f;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetFloat(b.RootElement(), "node", 1000.1f);
  EXPECT_TRUE(XMLUtils::GetFloat(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, SetBoolean)
{
  CXBMCTinyXML a;
  bool ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetBoolean(a.RootElement(), "node", true);
  EXPECT_TRUE(XMLUtils::GetBoolean(a.RootElement(), "node", val));

  ref = true;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetBoolean(b.RootElement(), "node", true);
  EXPECT_TRUE(XMLUtils::GetBoolean(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, SetHex)
{
  CXBMCTinyXML a;
  uint32_t ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetHex(a.RootElement(), "node", 0xFF);
  EXPECT_TRUE(XMLUtils::GetHex(a.RootElement(), "node", val));

  ref = 0xFF;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetHex(b.RootElement(), "node", 0xFF);
  EXPECT_TRUE(XMLUtils::GetHex(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, SetPath)
{
  CXBMCTinyXML a;
  std::string ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetPath(a.RootElement(), "node", "special://xbmc/");
  EXPECT_TRUE(XMLUtils::GetPath(a.RootElement(), "node", val));

  ref = "special://xbmc/";
  EXPECT_STREQ(ref.c_str(), val.c_str());

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetPath(b.RootElement(), "node", "special://xbmc/");
  EXPECT_TRUE(XMLUtils::GetPath(b.RootElement(), "node", val2));

  EXPECT_STREQ(ref.c_str(), val2.c_str());
}

TEST(TestXMLUtils, SetLong)
{
  CXBMCTinyXML a;
  long ref, val, val2;

  a.Parse(std::string("<root></root>"));
  XMLUtils::SetLong(a.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetLong(a.RootElement(), "node", val));

  ref = 1000;
  EXPECT_EQ(ref, val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetLong(b.RootElement(), "node", 1000);
  EXPECT_TRUE(XMLUtils::GetLong(b.RootElement(), "node", val2));

  EXPECT_EQ(ref, val2);
}

TEST(TestXMLUtils, SetDate)
{
  CXBMCTinyXML a;
  CDateTime ref, val, val2;

  a.Parse(std::string("<root></root>"));
  ref.SetDate(2012, 7, 8);
  XMLUtils::SetDate(a.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDate(a.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetDate(b.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDate(b.RootElement(), "node", val2));
  EXPECT_TRUE(ref == val);
}

TEST(TestXMLUtils, SetDateTime)
{
  CXBMCTinyXML a;
  CDateTime ref, val, val2;

  a.Parse(std::string("<root></root>"));
  ref.SetDateTime(2012, 7, 8, 1, 2, 3);
  XMLUtils::SetDateTime(a.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDateTime(a.RootElement(), "node", val));
  EXPECT_TRUE(ref == val);

  CXBMCTinyXML2 b;

  b.Parse(std::string("<root></root>"));
  XMLUtils::SetDateTime(b.RootElement(), "node", ref);
  EXPECT_TRUE(XMLUtils::GetDateTime(b.RootElement(), "node", val2));
  EXPECT_TRUE(ref == val2);
}
