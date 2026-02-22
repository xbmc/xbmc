/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/i18n/Bcp47Registry/SubTagRegistryFile.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS::I18N;

class CRegistryFileTest : public CRegistryFile
{
public:
  CRegistryFileTest(std::string filePath) : CRegistryFile(filePath) {}

  std::vector<RegistryFileField> CallProcessRecordLines(std::vector<std::string> lines)
  {
    return ProcessRecordLines(lines);
  }
};

TEST(TestI18nRegistryFile, Read)
{
  // Examples of real records
  CRegistryFile f(
      XBMC_REF_FILE_PATH("xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry.txt"));
  EXPECT_TRUE(f.Load());
}

TEST(TestI18nRegistryFile, ReadMissing)
{
  CRegistryFile f(XBMC_REF_FILE_PATH("xbmc/utils/i18n/Bcp47Registry/test/inexistent"));
  EXPECT_FALSE(f.Load());
}

TEST(TestI18nRegistryFile, ReadInvalid1)
{
  CRegistryFile f(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-invalid1.txt"));
  EXPECT_FALSE(f.Load());
}

TEST(TestI18nRegistryFile, ReadInvalid2)
{
  CRegistryFile f(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-invalid2.txt"));
  EXPECT_FALSE(f.Load());
}

TEST(TestI18nRegistryFile, Type)
{
  CRegistryFile f(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-type.txt"));
  EXPECT_TRUE(f.Load());
  auto result = f.GetRecords();

  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].m_type, SubTagType::Language);
}

TEST(TestI18nRegistryFile, Scope)
{
  CRegistryFile f(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-scope.txt"));
  EXPECT_TRUE(f.Load());
  auto result = f.GetRecords();

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].m_scope, SubTagScope::Individual);
  EXPECT_EQ(result[1].m_scope, SubTagScope::MacroLanguage);
}

TEST(TestI18nRegistryFile, DateValid)
{
  CRegistryFile f(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-date-valid.txt"));
  EXPECT_TRUE(f.Load());
  auto result = f.GetRecords();

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].m_added, "2005-10-16");
  EXPECT_EQ(result[1].m_deprecated, "2024-02-29");
}

// Disable test for posix platforms because CDateTime does not properly validate calendar dates.
// They're normalized first (ex. Oct 40th is silently changed to Nov 9th) and failure is never
// reported, even for non-representable dates.
//! @todo provide proper date validate for posix
#ifdef TARGET_WINDOWS
TEST(TestI18nRegistryFile, DateInvalid)
{
  CRegistryFile f(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-date-invalid.txt"));
  EXPECT_FALSE(f.Load());
  EXPECT_EQ(f.GetRecords().size(), 0);
}
#endif

TEST(TestI18nRegistryFile, Strings)
{
  CRegistryFile f(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-strings.txt"));
  EXPECT_TRUE(f.Load());
  auto result = f.GetRecords();

  std::vector<std::string> expected;

  EXPECT_EQ(result.size(), 9);
  EXPECT_EQ(result[0].m_subTag, "lang1"); // Subtag
  EXPECT_EQ(result[1].m_subTag, "gf1"); // Tag
  expected = {{"desc1"}};
  EXPECT_EQ(result[2].m_descriptions, expected); // Description
  expected = {{"desc2"}, {"desc3"}};
  EXPECT_EQ(result[3].m_descriptions, expected);
  EXPECT_EQ(result[4].m_preferredValue, "pref1"); // Preferred-Value
  expected = {{"pref1"}};
  EXPECT_EQ(result[5].m_prefixes, expected); // Prefix
  expected = {{"pref2"}, {"pref3"}};
  EXPECT_EQ(result[6].m_prefixes, expected);
  EXPECT_EQ(result[7].m_suppressScript, "script1"); // Suppress-Script
  EXPECT_EQ(result[8].m_macroLanguage, "macro1"); // Macrolanguage
}

namespace KODI::UTILS::I18N
{
bool operator==(const RegistryFileField& a, const RegistryFileField& b)
{
  return a.m_name == b.m_name && a.m_body == b.m_body;
}
} // namespace KODI::UTILS::I18N

struct TestRegistryFileRecordLines
{
  std::vector<std::string> input;
  std::vector<RegistryFileField> expected;
};

const TestRegistryFileRecordLines RegistryFileRecordLinesTests[] = {
    // separator no space
    {{{"name:body"}}, {{"name", "body"}}},
    // multiple spaces around separator
    {{{"name  :     body"}}, {{"name", "body"}}},
    // body split over multiple lines
    {{{"name  :   "}, {" body"}}, {{"name", "body"}}},
    {{{"name: multi"}, {" line"}}, {{"name", "multiline"}}},
    {{{"name: multi"}, {"  line"}}, {{"name", "multi line"}}},
    {{{"name: multi"}, {"   line"}}, {{"name", "multi  line"}}},
    {{{"name: line1"}, {"  line2"}, {"  line3"}}, {{"name", "line1 line2 line3"}}},
    // not compliant with the spec - tested to ensure reasonable outcome
    {{{"non compliant - not a field body continuation and no separator"}}, {}},
    {{{" non compliant - field body continuation line before field name definition"}}, {}},
    // case preservation
    {{{"NaMe:bOdY"}}, {{"NaMe", "bOdY"}}},
    // multiple records
    {{{"name1: body1"}, {"name2: body2"}}, {{"name1", "body1"}, {"name2", "body2"}}},
    {{{"name1: body1"}, {"name1: body2"}}, {{"name1", "body1"}, {"name1", "body2"}}},
    {{{"name1: line1"}, {"  line2"}, {"  line3"}, {"name2: body2"}},
     {{"name1", "line1 line2 line3"}, {"name2", "body2"}}},
};

class RegistryFileRecordLinesTester
  : public testing::WithParamInterface<TestRegistryFileRecordLines>,
    public testing::Test
{
};

TEST_P(RegistryFileRecordLinesTester, Process)
{
  CRegistryFileTest f("");
  EXPECT_EQ(GetParam().expected, f.CallProcessRecordLines(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(TestI18nRegistryFile,
                         RegistryFileRecordLinesTester,
                         testing::ValuesIn(RegistryFileRecordLinesTests));

std::vector<std::vector<std::string>> ReadRecords(std::string filePath)
{
  CRegistryFile::CRegFile f;
  f.Open(filePath);

  std::vector<std::vector<std::string>> records;
  std::vector<std::string> lines;

  while (f.ReadRecord(lines))
    records.emplace_back(std::move(lines));

  return records;
}

TEST(TestI18nRegistryFile, ParseRecords1)
{
  std::vector<std::vector<std::string>> expected = {
      {{"record1"}},
      {{"record2-1"}, {"record2-2"}, {""}, {"record2-3"}, {""}},
      {},
      {{"record3"}},
  };

  std::vector<std::vector<std::string>> actual = ReadRecords(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-records1.txt"));

  EXPECT_EQ(expected, actual);
}

TEST(TestI18nRegistryFile, ParseRecords2)
{
  std::vector<std::vector<std::string>> expected = {
      {{"record1"}},
      {{"record2"}},
  };

  std::vector<std::vector<std::string>> actual = ReadRecords(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-records2.txt"));

  EXPECT_EQ(expected, actual);
}

TEST(TestI18nRegistryFile, LineEndingsCrLf)
{
  std::vector<std::vector<std::string>> expected = {
      {{"File-Date: 2025-08-25"}},
      {{"name1: line1"}, {"  line2"}, {"name2: body2"}, {""}},
      {{"name: line1"}, {"  line2"}},
  };

  std::vector<std::vector<std::string>> actual = ReadRecords(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-lineendings-crlf.txt"));

  EXPECT_EQ(expected, actual);
}

TEST(TestI18nRegistryFile, LineEndingsLf)
{
  std::vector<std::vector<std::string>> expected = {
      {{"File-Date: 2025-08-25"}},
      {{"name1: line1"}, {"  line2"}, {"name2: body2"}, {""}},
      {{"name: line1"}, {"  line2"}},
  };

  std::vector<std::vector<std::string>> actual = ReadRecords(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-lineendings-lf.txt"));

  EXPECT_EQ(expected, actual);
}

TEST(TestI18nRegistryFile, CasePreservation)
{
  std::vector<std::vector<std::string>> expected = {{{"ReCoRd1-1"}, {"rEcOrD2-1"}}};

  std::vector<std::vector<std::string>> actual = ReadRecords(XBMC_REF_FILE_PATH(
      "xbmc/utils/i18n/Bcp47Registry/test/test-language-subtag-registry-case.txt"));

  EXPECT_EQ(expected, actual);
}
