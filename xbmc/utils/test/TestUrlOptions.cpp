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

#include "utils/UrlOptions.h"
#include "utils/Variant.h"

#include "gtest/gtest.h"

TEST(TestUrlOptions, Clear)
{
  const char *key = "foo";

  CUrlOptions urlOptions;
  urlOptions.AddOption(key, "bar");
  EXPECT_TRUE(urlOptions.HasOption(key));

  urlOptions.Clear();
  EXPECT_FALSE(urlOptions.HasOption(key));
}

TEST(TestUrlOptions, AddOption)
{
  const char *keyChar = "char";
  const char *keyString = "string";
  const char *keyEmpty = "empty";
  const char *keyInt = "int";
  const char *keyFloat = "float";
  const char *keyDouble = "double";
  const char *keyBool = "bool";

  const char *valueChar = "valueChar";
  const std::string valueString = "valueString";
  const char *valueEmpty = "";
  int valueInt = 1;
  float valueFloat = 1.0f;
  double valueDouble = 1.0;
  bool valueBool = true;

  CVariant variantValue;

  CUrlOptions urlOptions;
  urlOptions.AddOption(keyChar, valueChar);
  {
    CVariant variantValue;
    EXPECT_TRUE(urlOptions.GetOption(keyChar, variantValue));
    EXPECT_TRUE(variantValue.isString());
    EXPECT_STREQ(valueChar, variantValue.asString().c_str());
  }

  urlOptions.AddOption(keyString, valueString);
  {
    CVariant variantValue;
    EXPECT_TRUE(urlOptions.GetOption(keyString, variantValue));
    EXPECT_TRUE(variantValue.isString());
    EXPECT_STREQ(valueString.c_str(), variantValue.asString().c_str());
  }

  urlOptions.AddOption(keyEmpty, valueEmpty);
  {
    CVariant variantValue;
    EXPECT_TRUE(urlOptions.GetOption(keyEmpty, variantValue));
    EXPECT_TRUE(variantValue.isString());
    EXPECT_STREQ(valueEmpty, variantValue.asString().c_str());
  }

  urlOptions.AddOption(keyInt, valueInt);
  {
    CVariant variantValue;
    EXPECT_TRUE(urlOptions.GetOption(keyInt, variantValue));
    EXPECT_TRUE(variantValue.isInteger());
    EXPECT_EQ(valueInt, (int)variantValue.asInteger());
  }

  urlOptions.AddOption(keyFloat, valueFloat);
  {
    CVariant variantValue;
    EXPECT_TRUE(urlOptions.GetOption(keyFloat, variantValue));
    EXPECT_TRUE(variantValue.isDouble());
    EXPECT_EQ(valueFloat, variantValue.asFloat());
  }

  urlOptions.AddOption(keyDouble, valueDouble);
  {
    CVariant variantValue;
    EXPECT_TRUE(urlOptions.GetOption(keyDouble, variantValue));
    EXPECT_TRUE(variantValue.isDouble());
    EXPECT_EQ(valueDouble, variantValue.asDouble());
  }

  urlOptions.AddOption(keyBool, valueBool);
  {
    CVariant variantValue;
    EXPECT_TRUE(urlOptions.GetOption(keyBool, variantValue));
    EXPECT_TRUE(variantValue.isBoolean());
    EXPECT_EQ(valueBool, variantValue.asBoolean());
  }
}

TEST(TestUrlOptions, AddOptions)
{
  std::string ref = "foo=bar&key=value";

  CUrlOptions urlOptions(ref);
  {
    CVariant value;
    EXPECT_TRUE(urlOptions.GetOption("foo", value));
    EXPECT_TRUE(value.isString());
    EXPECT_STREQ("bar", value.asString().c_str());
  }
  {
    CVariant value;
    EXPECT_TRUE(urlOptions.GetOption("key", value));
    EXPECT_TRUE(value.isString());
    EXPECT_STREQ("value", value.asString().c_str());
  }

  ref = "foo=bar&key";
  urlOptions.Clear();
  urlOptions.AddOptions(ref);
  {
    CVariant value;
    EXPECT_TRUE(urlOptions.GetOption("foo", value));
    EXPECT_TRUE(value.isString());
    EXPECT_STREQ("bar", value.asString().c_str());
  }
  {
    CVariant value;
    EXPECT_TRUE(urlOptions.GetOption("key", value));
    EXPECT_TRUE(value.isString());
    EXPECT_TRUE(value.empty());
  }
}

TEST(TestUrlOptions, RemoveOption)
{
  const char *key = "foo";

  CUrlOptions urlOptions;
  urlOptions.AddOption(key, "bar");
  EXPECT_TRUE(urlOptions.HasOption(key));

  urlOptions.RemoveOption(key);
  EXPECT_FALSE(urlOptions.HasOption(key));
}

TEST(TestUrlOptions, HasOption)
{
  const char *key = "foo";

  CUrlOptions urlOptions;
  urlOptions.AddOption(key, "bar");
  EXPECT_TRUE(urlOptions.HasOption(key));
  EXPECT_FALSE(urlOptions.HasOption("bar"));
}

TEST(TestUrlOptions, GetOptions)
{
  const char *key1 = "foo";
  const char *key2 = "key";
  const char *value1 = "bar";
  const char *value2 = "value";

  CUrlOptions urlOptions;
  urlOptions.AddOption(key1, value1);
  urlOptions.AddOption(key2, value2);
  const CUrlOptions::UrlOptions &options = urlOptions.GetOptions();
  EXPECT_FALSE(options.empty());
  EXPECT_EQ(2, options.size());

  CUrlOptions::UrlOptions::const_iterator it1 = options.find(key1);
  EXPECT_TRUE(it1 != options.end());
  CUrlOptions::UrlOptions::const_iterator it2 = options.find(key2);
  EXPECT_TRUE(it2 != options.end());
  EXPECT_FALSE(options.find("wrong") != options.end());
  EXPECT_TRUE(it1->second.isString());
  EXPECT_TRUE(it2->second.isString());
  EXPECT_STREQ(value1, it1->second.asString().c_str());
  EXPECT_STREQ(value2, it2->second.asString().c_str());
}

TEST(TestUrlOptions, GetOptionsString)
{
  const char *ref = "foo=bar&key";

  CUrlOptions urlOptions(ref);
  std::string value = urlOptions.GetOptionsString();
  EXPECT_STREQ(ref, value.c_str());
}
