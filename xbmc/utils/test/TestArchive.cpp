/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "filesystem/File.h"
#include "test/TestUtils.h"
#include "utils/Archive.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"

#include <gtest/gtest.h>

class TestArchive : public testing::Test
{
protected:
  TestArchive()
  {
    file = XBMC_CREATETEMPFILE(".ar");
  }
  ~TestArchive() override
  {
    EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
  }
  XFILE::CFile *file;
};

TEST_F(TestArchive, IsStoring)
{
  ASSERT_NE(nullptr, file);
  CArchive arstore(file, CArchive::store);
  EXPECT_TRUE(arstore.IsStoring());
  EXPECT_FALSE(arstore.IsLoading());
  arstore.Close();
}

TEST_F(TestArchive, IsLoading)
{
  ASSERT_NE(nullptr, file);
  CArchive arload(file, CArchive::load);
  EXPECT_TRUE(arload.IsLoading());
  EXPECT_FALSE(arload.IsStoring());
  arload.Close();
}

TEST_F(TestArchive, FloatArchive)
{
  ASSERT_NE(nullptr, file);
  float float_ref = 1, float_var = 0;

  CArchive arstore(file, CArchive::store);
  arstore << float_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> float_var;
  arload.Close();

  EXPECT_EQ(float_ref, float_var);
}

TEST_F(TestArchive, DoubleArchive)
{
  ASSERT_NE(nullptr, file);
  double double_ref = 2, double_var = 0;

  CArchive arstore(file, CArchive::store);
  arstore << double_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> double_var;
  arload.Close();

  EXPECT_EQ(double_ref, double_var);
}

TEST_F(TestArchive, IntegerArchive)
{
  ASSERT_NE(nullptr, file);
  int int_ref = 3, int_var = 0;

  CArchive arstore(file, CArchive::store);
  arstore << int_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> int_var;
  arload.Close();

  EXPECT_EQ(int_ref, int_var);
}

TEST_F(TestArchive, UnsignedIntegerArchive)
{
  ASSERT_NE(nullptr, file);
  unsigned int unsigned_int_ref = 4, unsigned_int_var = 0;

  CArchive arstore(file, CArchive::store);
  arstore << unsigned_int_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> unsigned_int_var;
  arload.Close();

  EXPECT_EQ(unsigned_int_ref, unsigned_int_var);
}

TEST_F(TestArchive, Int64tArchive)
{
  ASSERT_NE(nullptr, file);
  int64_t int64_t_ref = 5, int64_t_var = 0;

  CArchive arstore(file, CArchive::store);
  arstore << int64_t_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> int64_t_var;
  arload.Close();

  EXPECT_EQ(int64_t_ref, int64_t_var);
}

TEST_F(TestArchive, UInt64tArchive)
{
  ASSERT_NE(nullptr, file);
  uint64_t uint64_t_ref = 6, uint64_t_var = 0;

  CArchive arstore(file, CArchive::store);
  arstore << uint64_t_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> uint64_t_var;
  arload.Close();

  EXPECT_EQ(uint64_t_ref, uint64_t_var);
}

TEST_F(TestArchive, BoolArchive)
{
  ASSERT_NE(nullptr, file);
  bool bool_ref = true, bool_var = false;

  CArchive arstore(file, CArchive::store);
  arstore << bool_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> bool_var;
  arload.Close();

  EXPECT_EQ(bool_ref, bool_var);
}

TEST_F(TestArchive, CharArchive)
{
  ASSERT_NE(nullptr, file);
  char char_ref = 'A', char_var = '\0';

  CArchive arstore(file, CArchive::store);
  arstore << char_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> char_var;
  arload.Close();

  EXPECT_EQ(char_ref, char_var);
}

TEST_F(TestArchive, WStringArchive)
{
  ASSERT_NE(nullptr, file);
  std::wstring wstring_ref = L"test wstring", wstring_var;

  CArchive arstore(file, CArchive::store);
  arstore << wstring_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> wstring_var;
  arload.Close();

  EXPECT_STREQ(wstring_ref.c_str(), wstring_var.c_str());
}

TEST_F(TestArchive, StringArchive)
{
  ASSERT_NE(nullptr, file);
  std::string string_ref = "test string", string_var;

  CArchive arstore(file, CArchive::store);
  arstore << string_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> string_var;
  arload.Close();

  EXPECT_STREQ(string_ref.c_str(), string_var.c_str());
}

TEST_F(TestArchive, SystemTimeArchive)
{
  ASSERT_NE(nullptr, file);
  KODI::TIME::SystemTime SystemTime_ref = {1, 2, 3, 4, 5, 6, 7, 8};
  KODI::TIME::SystemTime SystemTime_var = {0, 0, 0, 0, 0, 0, 0, 0};

  CArchive arstore(file, CArchive::store);
  arstore << SystemTime_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> SystemTime_var;
  arload.Close();

  EXPECT_TRUE(!memcmp(&SystemTime_ref, &SystemTime_var, sizeof(KODI::TIME::SystemTime)));
}

TEST_F(TestArchive, CVariantArchive)
{
  ASSERT_NE(nullptr, file);
  CVariant CVariant_ref((int)1), CVariant_var;

  CArchive arstore(file, CArchive::store);
  arstore << CVariant_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> CVariant_var;
  arload.Close();

  EXPECT_TRUE(CVariant_var.isInteger());
  EXPECT_EQ(1, CVariant_var.asInteger());
}

TEST_F(TestArchive, CVariantArchiveString)
{
  ASSERT_NE(nullptr, file);
  CVariant CVariant_ref("teststring"), CVariant_var;

  CArchive arstore(file, CArchive::store);
  arstore << CVariant_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> CVariant_var;
  arload.Close();

  EXPECT_TRUE(CVariant_var.isString());
  EXPECT_STREQ("teststring", CVariant_var.asString().c_str());
}

TEST_F(TestArchive, StringVectorArchive)
{
  ASSERT_NE(nullptr, file);
  std::vector<std::string> strArray_ref, strArray_var;
  strArray_ref.emplace_back("test strArray_ref 0");
  strArray_ref.emplace_back("test strArray_ref 1");
  strArray_ref.emplace_back("test strArray_ref 2");
  strArray_ref.emplace_back("test strArray_ref 3");

  CArchive arstore(file, CArchive::store);
  arstore << strArray_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> strArray_var;
  arload.Close();

  EXPECT_STREQ("test strArray_ref 0", strArray_var.at(0).c_str());
  EXPECT_STREQ("test strArray_ref 1", strArray_var.at(1).c_str());
  EXPECT_STREQ("test strArray_ref 2", strArray_var.at(2).c_str());
  EXPECT_STREQ("test strArray_ref 3", strArray_var.at(3).c_str());
}

TEST_F(TestArchive, IntegerVectorArchive)
{
  ASSERT_NE(nullptr, file);
  std::vector<int> iArray_ref, iArray_var;
  iArray_ref.push_back(0);
  iArray_ref.push_back(1);
  iArray_ref.push_back(2);
  iArray_ref.push_back(3);

  CArchive arstore(file, CArchive::store);
  arstore << iArray_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  arload >> iArray_var;
  arload.Close();

  EXPECT_EQ(0, iArray_var.at(0));
  EXPECT_EQ(1, iArray_var.at(1));
  EXPECT_EQ(2, iArray_var.at(2));
  EXPECT_EQ(3, iArray_var.at(3));
}

TEST_F(TestArchive, MultiTypeArchive)
{
  ASSERT_NE(nullptr, file);
  float float_ref = 1, float_var = 0;
  double double_ref = 2, double_var = 0;
  int int_ref = 3, int_var = 0;
  unsigned int unsigned_int_ref = 4, unsigned_int_var = 0;
  int64_t int64_t_ref = 5, int64_t_var = 0;
  uint64_t uint64_t_ref = 6, uint64_t_var = 0;
  bool bool_ref = true, bool_var = false;
  char char_ref = 'A', char_var = '\0';
  std::string string_ref = "test string", string_var;
  std::wstring wstring_ref = L"test wstring", wstring_var;
  KODI::TIME::SystemTime SystemTime_ref = {1, 2, 3, 4, 5, 6, 7, 8};
  KODI::TIME::SystemTime SystemTime_var = {0, 0, 0, 0, 0, 0, 0, 0};
  CVariant CVariant_ref((int)1), CVariant_var;
  std::vector<std::string> strArray_ref, strArray_var;
  strArray_ref.emplace_back("test strArray_ref 0");
  strArray_ref.emplace_back("test strArray_ref 1");
  strArray_ref.emplace_back("test strArray_ref 2");
  strArray_ref.emplace_back("test strArray_ref 3");
  std::vector<int> iArray_ref, iArray_var;
  iArray_ref.push_back(0);
  iArray_ref.push_back(1);
  iArray_ref.push_back(2);
  iArray_ref.push_back(3);

  CArchive arstore(file, CArchive::store);
  EXPECT_TRUE(arstore.IsStoring());
  EXPECT_FALSE(arstore.IsLoading());
  arstore << float_ref;
  arstore << double_ref;
  arstore << int_ref;
  arstore << unsigned_int_ref;
  arstore << int64_t_ref;
  arstore << uint64_t_ref;
  arstore << bool_ref;
  arstore << char_ref;
  arstore << string_ref;
  arstore << wstring_ref;
  arstore << SystemTime_ref;
  arstore << CVariant_ref;
  arstore << strArray_ref;
  arstore << iArray_ref;
  arstore.Close();

  ASSERT_EQ(0, file->Seek(0, SEEK_SET));
  CArchive arload(file, CArchive::load);
  EXPECT_TRUE(arload.IsLoading());
  EXPECT_FALSE(arload.IsStoring());
  arload >> float_var;
  arload >> double_var;
  arload >> int_var;
  arload >> unsigned_int_var;
  arload >> int64_t_var;
  arload >> uint64_t_var;
  arload >> bool_var;
  arload >> char_var;
  arload >> string_var;
  arload >> wstring_var;
  arload >> SystemTime_var;
  arload >> CVariant_var;
  arload >> strArray_var;
  arload >> iArray_var;
  arload.Close();

  EXPECT_EQ(float_ref, float_var);
  EXPECT_EQ(double_ref, double_var);
  EXPECT_EQ(int_ref, int_var);
  EXPECT_EQ(unsigned_int_ref, unsigned_int_var);
  EXPECT_EQ(int64_t_ref, int64_t_var);
  EXPECT_EQ(uint64_t_ref, uint64_t_var);
  EXPECT_EQ(bool_ref, bool_var);
  EXPECT_EQ(char_ref, char_var);
  EXPECT_STREQ(string_ref.c_str(), string_var.c_str());
  EXPECT_STREQ(wstring_ref.c_str(), wstring_var.c_str());
  EXPECT_TRUE(!memcmp(&SystemTime_ref, &SystemTime_var, sizeof(KODI::TIME::SystemTime)));
  EXPECT_TRUE(CVariant_var.isInteger());
  EXPECT_STREQ("test strArray_ref 0", strArray_var.at(0).c_str());
  EXPECT_STREQ("test strArray_ref 1", strArray_var.at(1).c_str());
  EXPECT_STREQ("test strArray_ref 2", strArray_var.at(2).c_str());
  EXPECT_STREQ("test strArray_ref 3", strArray_var.at(3).c_str());
  EXPECT_EQ(0, iArray_var.at(0));
  EXPECT_EQ(1, iArray_var.at(1));
  EXPECT_EQ(2, iArray_var.at(2));
  EXPECT_EQ(3, iArray_var.at(3));
}
