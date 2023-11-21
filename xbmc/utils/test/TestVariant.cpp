/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/Variant.h"

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

TEST(TestVariant, VariantTypeInteger)
{
  CVariant a((int)0), b((int64_t)1);

  EXPECT_TRUE(a.isInteger());
  EXPECT_EQ(CVariant::VariantTypeInteger, a.type());
  EXPECT_TRUE(b.isInteger());
  EXPECT_EQ(CVariant::VariantTypeInteger, b.type());

  EXPECT_EQ((int64_t)1, b.asInteger());
}

TEST(TestVariant, VariantTypeUnsignedInteger)
{
  CVariant a((unsigned int)0), b((uint64_t)1);

  EXPECT_TRUE(a.isUnsignedInteger());
  EXPECT_EQ(CVariant::VariantTypeUnsignedInteger, a.type());
  EXPECT_TRUE(b.isUnsignedInteger());
  EXPECT_EQ(CVariant::VariantTypeUnsignedInteger, b.type());

  EXPECT_EQ((uint64_t)1, b.asUnsignedInteger());
}

TEST(TestVariant, VariantTypeBoolean)
{
  CVariant a(true);

  EXPECT_TRUE(a.isBoolean());
  EXPECT_EQ(CVariant::VariantTypeBoolean, a.type());

  EXPECT_TRUE(a.asBoolean());
}

TEST(TestVariant, VariantTypeString)
{
  CVariant a("VariantTypeString");
  CVariant b("VariantTypeString2", sizeof("VariantTypeString2") - 1);
  std::string str("VariantTypeString3");
  CVariant c(str);

  EXPECT_TRUE(a.isString());
  EXPECT_EQ(CVariant::VariantTypeString, a.type());
  EXPECT_TRUE(b.isString());
  EXPECT_EQ(CVariant::VariantTypeString, b.type());
  EXPECT_TRUE(c.isString());
  EXPECT_EQ(CVariant::VariantTypeString, c.type());

  EXPECT_STREQ("VariantTypeString", a.asString().c_str());
  EXPECT_STREQ("VariantTypeString2", b.asString().c_str());
  EXPECT_STREQ("VariantTypeString3", c.asString().c_str());
}

TEST(TestVariant, VariantTypeWideString)
{
  CVariant a(L"VariantTypeWideString");
  CVariant b(L"VariantTypeWideString2", sizeof(L"VariantTypeWideString2") - 1);
  std::wstring str(L"VariantTypeWideString3");
  CVariant c(str);

  EXPECT_TRUE(a.isWideString());
  EXPECT_EQ(CVariant::VariantTypeWideString, a.type());
  EXPECT_TRUE(b.isWideString());
  EXPECT_EQ(CVariant::VariantTypeWideString, b.type());
  EXPECT_TRUE(c.isWideString());
  EXPECT_EQ(CVariant::VariantTypeWideString, c.type());

  EXPECT_STREQ(L"VariantTypeWideString", a.asWideString().c_str());
  EXPECT_STREQ(L"VariantTypeWideString2", b.asWideString().c_str());
  EXPECT_STREQ(L"VariantTypeWideString3", c.asWideString().c_str());
}

TEST(TestVariant, VariantTypeDouble)
{
  CVariant a((float)0.0f), b((double)0.1f);

  EXPECT_TRUE(a.isDouble());
  EXPECT_EQ(CVariant::VariantTypeDouble, a.type());
  EXPECT_TRUE(b.isDouble());
  EXPECT_EQ(CVariant::VariantTypeDouble, b.type());

  EXPECT_EQ((float)0.0f, a.asDouble());
  EXPECT_EQ((double)0.1f, b.asDouble());
}

TEST(TestVariant, VariantTypeArray)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("string1");
  strarray.emplace_back("string2");
  strarray.emplace_back("string3");
  strarray.emplace_back("string4");
  CVariant a(strarray);

  EXPECT_TRUE(a.isArray());
  EXPECT_EQ(CVariant::VariantTypeArray, a.type());
}

TEST(TestVariant, VariantTypeObject)
{
  CVariant a;
  a["key"] = "value";

  EXPECT_TRUE(a.isObject());
  EXPECT_EQ(CVariant::VariantTypeObject, a.type());
}

TEST(TestVariant, VariantTypeNull)
{
  CVariant a;

  EXPECT_TRUE(a.isNull());
  EXPECT_EQ(CVariant::VariantTypeNull, a.type());
}

TEST(TestVariant, VariantFromMap)
{
  std::map<std::string, std::string> strMap;
  strMap["key"] = "value";
  CVariant a = strMap;

  EXPECT_TRUE(a.isObject());
  EXPECT_TRUE(a.size() == 1);
  EXPECT_EQ(CVariant::VariantTypeObject, a.type());
  EXPECT_TRUE(a.isMember("key"));
  EXPECT_TRUE(a["key"].isString());
  EXPECT_STREQ(a["key"].asString().c_str(), "value");

  std::map<std::string, CVariant> variantMap;
  variantMap["key"] = CVariant("value");
  CVariant b = variantMap;

  EXPECT_TRUE(b.isObject());
  EXPECT_TRUE(b.size() == 1);
  EXPECT_EQ(CVariant::VariantTypeObject, b.type());
  EXPECT_TRUE(b.isMember("key"));
  EXPECT_TRUE(b["key"].isString());
  EXPECT_STREQ(b["key"].asString().c_str(), "value");
}

TEST(TestVariant, operatorTest)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("string1");
  CVariant a, b, c(strarray), d;
  a["key"] = "value";
  b = a;
  c[0] = "value2";
  d = c;

  EXPECT_TRUE(a.isObject());
  EXPECT_EQ(CVariant::VariantTypeObject, a.type());
  EXPECT_TRUE(b.isObject());
  EXPECT_EQ(CVariant::VariantTypeObject, b.type());
  EXPECT_TRUE(c.isArray());
  EXPECT_EQ(CVariant::VariantTypeArray, c.type());
  EXPECT_TRUE(d.isArray());
  EXPECT_EQ(CVariant::VariantTypeArray, d.type());

  EXPECT_TRUE(a == b);
  EXPECT_TRUE(c == d);
  EXPECT_FALSE(a == d);

  EXPECT_STREQ("value", a["key"].asString().c_str());
  EXPECT_STREQ("value2", c[0].asString().c_str());
}

TEST(TestVariant, push_back)
{
  CVariant a, b("variant1"), c("variant2"), d("variant3");
  a.push_back(b);
  a.push_back(c);
  a.push_back(d);

  EXPECT_TRUE(a.isArray());
  EXPECT_EQ(CVariant::VariantTypeArray, a.type());
  EXPECT_STREQ("variant1", a[0].asString().c_str());
  EXPECT_STREQ("variant2", a[1].asString().c_str());
  EXPECT_STREQ("variant3", a[2].asString().c_str());
}

TEST(TestVariant, append)
{
  CVariant a, b("variant1"), c("variant2"), d("variant3");
  a.append(b);
  a.append(c);
  a.append(d);

  EXPECT_TRUE(a.isArray());
  EXPECT_EQ(CVariant::VariantTypeArray, a.type());
  EXPECT_STREQ("variant1", a[0].asString().c_str());
  EXPECT_STREQ("variant2", a[1].asString().c_str());
  EXPECT_STREQ("variant3", a[2].asString().c_str());
}

TEST(TestVariant, c_str)
{
  CVariant a("variant");

  EXPECT_STREQ("variant", a.c_str());
}

TEST(TestVariant, swap)
{
  CVariant a((int)0), b("variant");

  EXPECT_TRUE(a.isInteger());
  EXPECT_TRUE(b.isString());

  a.swap(b);
  EXPECT_TRUE(b.isInteger());
  EXPECT_TRUE(a.isString());
}

TEST(TestVariant, iterator_array)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  CVariant a(strarray);

  EXPECT_TRUE(a.isArray());
  EXPECT_EQ(CVariant::VariantTypeArray, a.type());

  for (auto it = a.begin_array(); it != a.end_array(); it++)
  {
    EXPECT_STREQ("string", it->c_str());
  }

  for (auto const_it = a.begin_array(); const_it != a.end_array(); const_it++)
  {
    EXPECT_STREQ("string", const_it->c_str());
  }
}

TEST(TestVariant, iterator_map)
{
  CVariant a;
  a["key1"] = "string";
  a["key2"] = "string";
  a["key3"] = "string";
  a["key4"] = "string";

  EXPECT_TRUE(a.isObject());
  EXPECT_EQ(CVariant::VariantTypeObject, a.type());

  for (auto it = a.begin_map(); it != a.end_map(); it++)
  {
    EXPECT_STREQ("string", it->second.c_str());
  }

  for (auto const_it = a.begin_map(); const_it != a.end_map(); const_it++)
  {
    EXPECT_STREQ("string", const_it->second.c_str());
  }
}

TEST(TestVariant, size)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  CVariant a(strarray);

  EXPECT_EQ((unsigned int)4, a.size());
}

TEST(TestVariant, empty)
{
  std::vector<std::string> strarray;
  EXPECT_TRUE(CVariant(strarray).empty());
  strarray.emplace_back("abc");
  EXPECT_FALSE(CVariant(strarray).empty());

  std::map<std::string, std::string> strmap;
  EXPECT_TRUE(CVariant(strmap).empty());
  strmap.emplace("key", "value");
  EXPECT_FALSE(CVariant(strmap).empty());

  std::string str;
  EXPECT_TRUE(CVariant(str).empty());
  str = "abc";
  EXPECT_FALSE(CVariant(str).empty());

  std::wstring wstr;
  EXPECT_TRUE(CVariant(wstr).empty());
  wstr = L"abc";
  EXPECT_FALSE(CVariant(wstr).empty());

  EXPECT_TRUE(CVariant().empty());

  EXPECT_FALSE(CVariant(CVariant::VariantTypeConstNull).empty());
  EXPECT_FALSE(CVariant(CVariant::VariantTypeInteger).empty());
  EXPECT_FALSE(CVariant(CVariant::VariantTypeUnsignedInteger).empty());
  EXPECT_FALSE(CVariant(CVariant::VariantTypeBoolean).empty());
  EXPECT_FALSE(CVariant(CVariant::VariantTypeDouble).empty());
}

TEST(TestVariant, clear)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  strarray.emplace_back("string");
  CVariant a(strarray);

  EXPECT_FALSE(a.empty());
  a.clear();
  EXPECT_TRUE(a.empty());
}

TEST(TestVariant, erase)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("string1");
  strarray.emplace_back("string2");
  strarray.emplace_back("string3");
  strarray.emplace_back("string4");
  CVariant a, b(strarray);
  a["key1"] = "string1";
  a["key2"] = "string2";
  a["key3"] = "string3";
  a["key4"] = "string4";

  EXPECT_STREQ("string2", a["key2"].c_str());
  EXPECT_STREQ("string2", b[1].c_str());
  a.erase("key2");
  b.erase(1);
  EXPECT_FALSE(a["key2"].c_str());
  EXPECT_STREQ("string3", b[1].c_str());
}

TEST(TestVariant, isMember)
{
  CVariant a;
  a["key1"] = "string1";

  EXPECT_TRUE(a.isMember("key1"));
  EXPECT_FALSE(a.isMember("key2"));
}

TEST(TestVariant, asBoolean)
{
  EXPECT_TRUE(CVariant("true").asBoolean());
  EXPECT_FALSE(CVariant("false").asBoolean());
  EXPECT_TRUE(CVariant("1").asBoolean());
  EXPECT_FALSE(CVariant("0").asBoolean());
  EXPECT_FALSE(CVariant("").asBoolean());

  EXPECT_TRUE(CVariant(L"true").asBoolean());
  EXPECT_FALSE(CVariant(L"false").asBoolean());
  EXPECT_TRUE(CVariant(L"1").asBoolean());
  EXPECT_FALSE(CVariant(L"0").asBoolean());
  EXPECT_FALSE(CVariant(L"").asBoolean());

  EXPECT_TRUE(CVariant(uint64_t{1}).asBoolean());
  EXPECT_TRUE(CVariant(uint64_t{999999999}).asBoolean());
  EXPECT_FALSE(CVariant(uint64_t{0}).asBoolean());

  EXPECT_TRUE(CVariant(int64_t{1}).asBoolean());
  EXPECT_TRUE(CVariant(int64_t{999999999}).asBoolean());
  EXPECT_TRUE(CVariant(int64_t{-999999999}).asBoolean());
  EXPECT_FALSE(CVariant(int64_t{0}).asBoolean());

  EXPECT_TRUE(CVariant(double{1}).asBoolean());
  EXPECT_TRUE(CVariant(double{999999999}).asBoolean());
  EXPECT_TRUE(CVariant(double{-999999999}).asBoolean());
  EXPECT_FALSE(CVariant(double{0}).asBoolean());

  EXPECT_TRUE(CVariant(true).asBoolean());
  EXPECT_FALSE(CVariant(false).asBoolean());

  EXPECT_FALSE(CVariant(CVariant::VariantTypeNull).asBoolean());
  EXPECT_FALSE(CVariant(CVariant::VariantTypeConstNull).asBoolean());
  EXPECT_FALSE(CVariant(CVariant::VariantTypeArray).asBoolean());
  EXPECT_FALSE(CVariant(CVariant::VariantTypeObject).asBoolean());
}
