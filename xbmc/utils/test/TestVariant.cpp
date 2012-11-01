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

#include "utils/Variant.h"

#include "gtest/gtest.h"

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
  strarray.push_back("string1");
  strarray.push_back("string2");
  strarray.push_back("string3");
  strarray.push_back("string4");
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
  strarray.push_back("string1");
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

TEST(TestVariant, interator_array)
{
  std::vector<std::string> strarray;
  strarray.push_back("string");
  strarray.push_back("string");
  strarray.push_back("string");
  strarray.push_back("string");
  CVariant a(strarray);

  EXPECT_TRUE(a.isArray());
  EXPECT_EQ(CVariant::VariantTypeArray, a.type());

  CVariant::iterator_array it;
  for (it = a.begin_array(); it < a.end_array(); it++)
  {
    EXPECT_STREQ("string", it->c_str());
  }

  CVariant::const_iterator_array const_it;
  for (const_it = a.begin_array(); const_it < a.end_array(); const_it++)
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

  CVariant::iterator_map it;
  for (it = a.begin_map(); it != a.end_map(); it++)
  {
    EXPECT_STREQ("string", it->second.c_str());
  }

  CVariant::const_iterator_map const_it;
  for (const_it = a.begin_map(); const_it != a.end_map(); const_it++)
  {
    EXPECT_STREQ("string", const_it->second.c_str());
  }
}

TEST(TestVariant, size)
{
  std::vector<std::string> strarray;
  strarray.push_back("string");
  strarray.push_back("string");
  strarray.push_back("string");
  strarray.push_back("string");
  CVariant a(strarray);

  EXPECT_EQ((unsigned int)4, a.size());
}

TEST(TestVariant, empty)
{
  std::vector<std::string> strarray;
  CVariant a(strarray);

  EXPECT_TRUE(a.empty());
}

TEST(TestVariant, clear)
{
  std::vector<std::string> strarray;
  strarray.push_back("string");
  strarray.push_back("string");
  strarray.push_back("string");
  strarray.push_back("string");
  CVariant a(strarray);

  EXPECT_FALSE(a.empty());
  a.clear();
  EXPECT_TRUE(a.empty());
}

TEST(TestVariant, erase)
{
  std::vector<std::string> strarray;
  strarray.push_back("string1");
  strarray.push_back("string2");
  strarray.push_back("string3");
  strarray.push_back("string4");
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
