/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

#include "gtest/gtest.h"

TEST(TestJSONVariantWriter, CanWriteNull)
{
  CVariant variant;
  std::string str;

  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("null", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteBoolean)
{
  CVariant variant(true);
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("true", str.c_str());

  variant = false;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("false", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteSignedInteger)
{
  CVariant variant(-1);
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("-1", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteUnsignedInteger)
{
  CVariant variant(0);
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("0", str.c_str());

  variant = 1;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("1", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteSignedInteger64)
{
  CVariant variant(static_cast<int64_t>(-4294967296LL));
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("-4294967296", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteUnsignedInteger64)
{
  CVariant variant(static_cast<int64_t>(4294967296LL));
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("4294967296", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteDouble)
{
  CVariant variant(0.0);
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("0.0", str.c_str());

  variant = 1.0;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("1.0", str.c_str());

  variant = -1.0;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("-1.0", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteString)
{
  CVariant variant("");
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("\"\"", str.c_str());

  variant = "foo";
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("\"foo\"", str.c_str());

  variant = "foo bar";
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("\"foo bar\"", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteObject)
{
  CVariant variant(CVariant::VariantTypeObject);
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("{}", str.c_str());

  variant.clear();
  variant["foo"] = "bar";
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("{\n\t\"foo\": \"bar\"\n}", str.c_str());

  variant.clear();
  variant["foo"] = "bar";
  variant["bar"] = true;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("{\n\t\"bar\": true,\n\t\"foo\": \"bar\"\n}", str.c_str());

  variant.clear();
  variant["foo"]["sub-foo"] = "bar";
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("{\n\t\"foo\": {\n\t\t\"sub-foo\": \"bar\"\n\t}\n}", str.c_str());
}

TEST(TestJSONVariantWriter, CanWriteArray)
{
  CVariant variant(CVariant::VariantTypeArray);
  std::string str;
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("[]", str.c_str());

  variant.clear();
  variant.push_back(true);
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("[\n\ttrue\n]", str.c_str());

  variant.clear();
  variant.push_back(true);
  variant.push_back("foo");
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("[\n\ttrue,\n\t\"foo\"\n]", str.c_str());

  variant.clear();
  CVariant obj(CVariant::VariantTypeObject);
  obj["foo"] = "bar";
  variant.push_back(obj);
  ASSERT_TRUE(CJSONVariantWriter::Write(variant, str, false));
  ASSERT_STREQ("[\n\t{\n\t\t\"foo\": \"bar\"\n\t}\n]", str.c_str());
}
