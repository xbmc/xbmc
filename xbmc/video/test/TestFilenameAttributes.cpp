/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "video/FilenameAttributes.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

using namespace KODI::VIDEO;

class TestFilenameAttributes : public Test
{
public:
  static void SetUpTestSuite()
  {
    // Inject list of known metadata sources for reliable results
    const std::shared_ptr<CAdvancedSettings> advancedSettings =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    ASSERT_TRUE(advancedSettings != nullptr);
    advancedSettings->m_videoScannerMetadataSources = {"tmdb", "imdb"};
  }

  static AttributeMap CallGetFilenameAttributePairs(const std::string& fileName,
                                                    KODI::REGEXP::RegExpCache* cache)
  {
    return CFilenameAttributes::GetFilenameAttributePairs(fileName, cache);
  }
};

struct TestUtilFilenameAttributesData
{
  std::string input;
  AttributeMap expected;
};

std::ostream& operator<<(std::ostream& os, const TestUtilFilenameAttributesData& rhs)
{
  return os << rhs.input;
}

class TestUtilFilenameAttributePairs : public TestFilenameAttributes,
                                       public WithParamInterface<TestUtilFilenameAttributesData>
{
};

TEST_P(TestUtilFilenameAttributePairs, RetrieveAttributes)
{
  const auto& params = GetParam();
  EXPECT_EQ(params.expected, CallGetFilenameAttributePairs(params.input, nullptr));
}

const TestUtilFilenameAttributesData filenameAttributePairsTests[] = {
    {"Some.MovieName.mkv", {}},
    // separators and braces
    {"Some.MovieName[key=value].mkv", {{"key", "value"}}},
    {"Some.MovieName[key-value].mkv", {{"key", "value"}}},
    {"Some.MovieName{key=value}.mkv", {{"key", "value"}}},
    {"Some.MovieName{key-value}.mkv", {{"key", "value"}}},
    // trim
    {"Some.MovieName[ key  =   value  ].mkv", {{"key", "value"}}},
    // multiple matches
    {"Some.MovieName [key1=value1]{key2 = value2}.mkv", {{"key1", "value1"}, {"key2", "value2"}}},
    {"Some.MovieName [key1=value1] foobar {key2 = value2}.mkv",
     {{"key1", "value1"}, {"key2", "value2"}}},
    // case
    {"Some.MovieName[KeY=vAlUe].mkv", {{"key", "vAlUe"}}},
    // repeated key
    {"Some.MovieName[key=value1][key=value2].mkv", {{"key", "value2"}}},
    {"Some.MovieName[key=value1][KEY=value2].mkv", {{"key", "value2"}}},
    // unicode
    {"Some.MovieName[key=r\u00E9alisateur].mkv", {{"key", "r\u00E9alisateur"}}},
    // no match
    {"Some.MovieName.key=value.mkv", {}},
    {"Some.MovieName[key=].mkv", {}},
    {"Some.MovieName[key=   ].mkv", {}},
    {"Some.MovieName[=value].mkv", {}},
    {"", {}},
};

INSTANTIATE_TEST_SUITE_P(TestFilenameAttributes,
                         TestUtilFilenameAttributePairs,
                         ValuesIn(filenameAttributePairsTests));

struct TestUtilCleanFilenameAttributesData
{
  std::string input;
  std::string expected;
};

std::ostream& operator<<(std::ostream& os, const TestUtilCleanFilenameAttributesData& rhs)
{
  return os << rhs.input;
}

class TestUtilCleanFilenameAttributePairs
  : public Test,
    public WithParamInterface<TestUtilCleanFilenameAttributesData>
{
};

TEST_P(TestUtilCleanFilenameAttributePairs, Clean)
{
  const auto& params = GetParam();
  std::string file = params.input;
  CFilenameAttributes::CleanFilenameAttributePairs(file, nullptr);
  EXPECT_EQ(params.expected, file);
}

const TestUtilCleanFilenameAttributesData cleanFilenameAttributePairsTests[] = {
    {"Some.MovieName[key=value].mkv", "Some.MovieName.mkv"},
    {"Some.MovieName [key1=value1]{key2 = value2}.mkv", "Some.MovieName .mkv"},
    {"Some.MovieName [key1=value1] foobar {key2 = value2}.mkv", "Some.MovieName  foobar .mkv"},
    {"Some.MovieName.mkv", "Some.MovieName.mkv"},
    {"[key=value]Some.MovieName.mkv", "Some.MovieName.mkv"},
};

INSTANTIATE_TEST_SUITE_P(TestFilenameAttributes,
                         TestUtilCleanFilenameAttributePairs,
                         ValuesIn(cleanFilenameAttributePairsTests));
TEST(TestFilenameAttributes, GetEdition)
{
  auto fa = CFilenameAttributes("Some.MovieName[edition=Director's Cut].mkv", nullptr);
  EXPECT_EQ("Director's Cut", fa.GetEdition());
  fa = CFilenameAttributes("Some.MovieName[foo=bar].mkv", nullptr);
  EXPECT_EQ("", fa.GetEdition());
  fa = CFilenameAttributes("Some.MovieName.mkv", nullptr);
  EXPECT_EQ("", fa.GetEdition());
  fa = CFilenameAttributes("", nullptr);
  EXPECT_EQ("", fa.GetEdition());
}

struct TestFilenameIdentifierData
{
  std::string input;
  bool rc;
  std::string identifierType{};
  std::string identifier{};
};

std::ostream& operator<<(std::ostream& os, const TestFilenameIdentifierData& rhs)
{
  return os << rhs.input;
}

class TestFilenameIdentifier : public TestFilenameAttributes,
                               public WithParamInterface<TestFilenameIdentifierData>
{
};

TEST_P(TestFilenameIdentifier, GetIdentifier)
{
  const auto& params = GetParam();
  CFilenameAttributes fa(params.input, nullptr);
  std::string identifierType;
  std::string identifier;
  EXPECT_EQ(params.rc, fa.GetIdentifier(identifierType, identifier));
  EXPECT_EQ(params.identifierType, identifierType);
  EXPECT_EQ(params.identifier, identifier);
}

TEST_P(TestFilenameIdentifier, HasIdentifier)
{
  const auto& params = GetParam();
  CFilenameAttributes fa(params.input, nullptr);
  EXPECT_EQ(params.rc, fa.HasIdentifier());
}

const TestFilenameIdentifierData FilenameIdentifierTests[] = {
    {"", false},
    {"Some.MovieName[edition=test].mkv", false},
    // known identifiers
    {"Some.MovieName[tmdb=123].mkv", true, "tmdb", "123"},
    {"Some.MovieName[imdb=tt0012345].mkv", true, "imdb", "tt0012345"},
    // the suffix "id" is removed from the key
    {"Some.MovieName[tmdbid=123].mkv", true, "tmdb", "123"},
    {"Some.MovieName[TMDBID=123].mkv", true, "tmdb", "123"},
    {"Some.MovieName[fooid=bar].mkv", false},
    // the value must be alphanum.
    {"Some.MovieName[tmdb=test-12 3].mkv", false},
};

INSTANTIATE_TEST_SUITE_P(TestFilenameAttributes,
                         TestFilenameIdentifier,
                         ValuesIn(FilenameIdentifierTests));

TEST(TestFilenameAttributes, EmptyKnownIdentifiers)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  ASSERT_TRUE(advancedSettings != nullptr);
  advancedSettings->m_videoScannerMetadataSources = {};

  auto fa = CFilenameAttributes("Some.MovieName[tmdb=123].mkv", nullptr);
  EXPECT_FALSE(fa.HasIdentifier());
  std::string identifierType;
  std::string identifier;
  EXPECT_FALSE(fa.GetIdentifier(identifierType, identifier));
  EXPECT_TRUE(identifierType.empty());
  EXPECT_TRUE(identifier.empty());

  fa = CFilenameAttributes("", nullptr);
  EXPECT_FALSE(fa.HasIdentifier());
  EXPECT_FALSE(fa.GetIdentifier(identifierType, identifier));
  EXPECT_TRUE(identifierType.empty());
  EXPECT_TRUE(identifier.empty());
}
