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

#include "utils/JSONVariantParser.h"
#include "utils/Variant.h"

#include "gtest/gtest.h"

TEST(TestJSONVariantParser, CannotParseNullptr)
{
  CVariant variant;
  ASSERT_FALSE(CJSONVariantParser::Parse(nullptr, variant));
}

TEST(TestJSONVariantParser, CannotParseEmptyString)
{
  CVariant variant;
  ASSERT_FALSE(CJSONVariantParser::Parse("", variant));
  ASSERT_FALSE(CJSONVariantParser::Parse(std::string(), variant));
}

TEST(TestJSONVariantParser, CannotParseInvalidJson)
{
  CVariant variant;
  ASSERT_FALSE(CJSONVariantParser::Parse("{", variant));
  ASSERT_FALSE(CJSONVariantParser::Parse("}", variant));
  ASSERT_FALSE(CJSONVariantParser::Parse("[", variant));
  ASSERT_FALSE(CJSONVariantParser::Parse("]", variant));
  ASSERT_FALSE(CJSONVariantParser::Parse("foo", variant));
}

TEST(TestJSONVariantParser, CanParseNull)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("null", variant));
  ASSERT_TRUE(variant.isNull());
}

TEST(TestJSONVariantParser, CanParseBoolean)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("true", variant));
  ASSERT_TRUE(variant.isBoolean());
  ASSERT_TRUE(variant.asBoolean());

  ASSERT_TRUE(CJSONVariantParser::Parse("false", variant));
  ASSERT_TRUE(variant.isBoolean());
  ASSERT_FALSE(variant.asBoolean());
}

TEST(TestJSONVariantParser, CanParseSignedInteger)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("-1", variant));
  ASSERT_TRUE(variant.isInteger());
  ASSERT_EQ(-1, variant.asInteger());
}

TEST(TestJSONVariantParser, CanParseUnsignedInteger)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("0", variant));
  ASSERT_TRUE(variant.isUnsignedInteger());
  ASSERT_EQ(0, variant.asUnsignedInteger());

  ASSERT_TRUE(CJSONVariantParser::Parse("1", variant));
  ASSERT_TRUE(variant.isUnsignedInteger());
  ASSERT_EQ(1, variant.asUnsignedInteger());
}

TEST(TestJSONVariantParser, CanParseSignedInteger64)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("-4294967296", variant));
  ASSERT_TRUE(variant.isInteger());
  ASSERT_EQ(-4294967296, variant.asInteger());
}

TEST(TestJSONVariantParser, CanParseUnsignedInteger64)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("4294967296", variant));
  ASSERT_TRUE(variant.isUnsignedInteger());
  ASSERT_EQ(4294967296, variant.asUnsignedInteger());
}

TEST(TestJSONVariantParser, CanParseDouble)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("0.0", variant));
  ASSERT_TRUE(variant.isDouble());
  ASSERT_EQ(0.0, variant.asDouble());

  ASSERT_TRUE(CJSONVariantParser::Parse("1.0", variant));
  ASSERT_TRUE(variant.isDouble());
  ASSERT_EQ(1.0, variant.asDouble());

  ASSERT_TRUE(CJSONVariantParser::Parse("-1.0", variant));
  ASSERT_TRUE(variant.isDouble());
  ASSERT_EQ(-1.0, variant.asDouble());
}

TEST(TestJSONVariantParser, CanParseString)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("\"\"", variant));
  ASSERT_TRUE(variant.isString());
  ASSERT_TRUE(variant.empty());

  ASSERT_TRUE(CJSONVariantParser::Parse("\"foo\"", variant));
  ASSERT_TRUE(variant.isString());
  ASSERT_STREQ("foo", variant.asString().c_str());

  ASSERT_TRUE(CJSONVariantParser::Parse("\"foo bar\"", variant));
  ASSERT_TRUE(variant.isString());
  ASSERT_STREQ("foo bar", variant.asString().c_str());
}

TEST(TestJSONVariantParser, CanParseObject)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("{}", variant));
  ASSERT_TRUE(variant.isObject());
  ASSERT_TRUE(variant.empty());

  variant.clear();
  ASSERT_TRUE(CJSONVariantParser::Parse("{ \"foo\": \"bar\" }", variant));
  ASSERT_TRUE(variant.isObject());
  ASSERT_TRUE(variant.isMember("foo"));
  ASSERT_TRUE(variant["foo"].isString());
  ASSERT_STREQ("bar", variant["foo"].asString().c_str());

  variant.clear();
  ASSERT_TRUE(CJSONVariantParser::Parse("{ \"foo\": \"bar\", \"bar\": true }", variant));
  ASSERT_TRUE(variant.isObject());
  ASSERT_TRUE(variant.isMember("foo"));
  ASSERT_TRUE(variant["foo"].isString());
  ASSERT_STREQ("bar", variant["foo"].asString().c_str());
  ASSERT_TRUE(variant.isMember("bar"));
  ASSERT_TRUE(variant["bar"].isBoolean());
  ASSERT_TRUE(variant["bar"].asBoolean());

  variant.clear();
  ASSERT_TRUE(CJSONVariantParser::Parse("{ \"foo\": { \"sub-foo\": \"bar\" } }", variant));
  ASSERT_TRUE(variant.isObject());
  ASSERT_TRUE(variant.isMember("foo"));
  ASSERT_TRUE(variant["foo"].isObject());
  ASSERT_TRUE(variant["foo"].isMember("sub-foo"));
  ASSERT_TRUE(variant["foo"]["sub-foo"].isString());
  ASSERT_STREQ("bar", variant["foo"]["sub-foo"].asString().c_str());
}

TEST(TestJSONVariantParser, CanParseArray)
{
  CVariant variant;
  ASSERT_TRUE(CJSONVariantParser::Parse("[]", variant));
  ASSERT_TRUE(variant.isArray());
  ASSERT_TRUE(variant.empty());

  variant.clear();
  ASSERT_TRUE(CJSONVariantParser::Parse("[ true ]", variant));
  ASSERT_TRUE(variant.isArray());
  ASSERT_EQ(1, variant.size());
  ASSERT_TRUE(variant[0].isBoolean());
  ASSERT_TRUE(variant[0].asBoolean());

  variant.clear();
  ASSERT_TRUE(CJSONVariantParser::Parse("[ true, \"foo\" ]", variant));
  ASSERT_TRUE(variant.isArray());
  ASSERT_EQ(2, variant.size());
  ASSERT_TRUE(variant[0].isBoolean());
  ASSERT_TRUE(variant[0].asBoolean());
  ASSERT_TRUE(variant[1].isString());
  ASSERT_STREQ("foo", variant[1].asString().c_str());

  variant.clear();
  ASSERT_TRUE(CJSONVariantParser::Parse("[ { \"foo\": \"bar\" } ]", variant));
  ASSERT_TRUE(variant.isArray());
  ASSERT_EQ(1, variant.size());
  ASSERT_TRUE(variant[0].isObject());
  ASSERT_TRUE(variant[0].isMember("foo"));
  ASSERT_TRUE(variant[0]["foo"].isString());
  ASSERT_STREQ("bar", variant[0]["foo"].asString().c_str());
}
