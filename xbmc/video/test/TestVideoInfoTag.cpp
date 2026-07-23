/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "utils/XBMCTinyXML.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
std::vector<std::string> GetSavedTags(const CXBMCTinyXML& doc)
{
  std::vector<std::string> tags;

  const TiXmlElement* movie = doc.RootElement();
  if (!movie)
    return tags;

  const TiXmlElement* tag = movie->FirstChildElement("tag");
  while (tag)
  {
    if (tag->FirstChild())
      tags.emplace_back(tag->FirstChild()->ValueStr());
    tag = tag->NextSiblingElement("tag");
  }

  return tags;
}
} // namespace

TEST(TestVideoInfoTag, ReadTVShowSeasons)
{
  const std::string document =
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
         <tvshow>
         <namedseason number="1">season 1</namedseason>
         <namedseason number="2"></namedseason>
         <namedseason number="3"></namedseason>
         <namedseason number="4">season 4</namedseason>
         <seasonplot number="3">plot 3</seasonplot>
         <seasonplot number="4">plot 4</seasonplot>
         <seasonplot number="5"></seasonplot>
         </tvshow>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  const std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", ""}}, {3, {"", "plot 3"}}, {4, {"season 4", "plot 4"}}};

  EXPECT_EQ(details.m_seasons, reference);
}

TEST(TestVideoInfoTag, LoadNativePreservesTagOrder)
{
  const std::string document =
      R"(<movie>
           <tag>secret agent</tag>
           <tag>undercover</tag>
           <tag>killing</tag>
           <tag>british secret service</tag>
           <tag>mi6</tag>
         </movie>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  const std::vector<std::string> reference = {"secret agent", "undercover", "killing",
                                              "british secret service", "mi6"};
  EXPECT_EQ(details.m_tags, reference);
}

TEST(TestVideoInfoTag, SaveAfterLoadNativePreservesTagOrder)
{
  const std::string document =
      R"(<movie>
           <tag>secret agent</tag>
           <tag>undercover</tag>
           <tag>killing</tag>
           <tag>british secret service</tag>
           <tag>mi6</tag>
         </movie>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  CXBMCTinyXML savedDoc;
  EXPECT_TRUE(details.Save(&savedDoc, "movie"));

  const std::vector<std::string> reference = {"secret agent", "undercover", "killing",
                                              "british secret service", "mi6"};
  EXPECT_EQ(GetSavedTags(savedDoc), reference);
}

TEST(TestVideoInfoTag, SetTagsDeduplicatesWithoutReordering)
{
  CVideoInfoTag details;
  details.SetTags({"secret agent", "undercover", "secret agent", "mi6"});

  const std::vector<std::string> reference = {"secret agent", "undercover", "mi6"};
  EXPECT_EQ(details.m_tags, reference);
}

TEST(TestVideoInfoTag, SetTagsKeepsAssignedOrder)
{
  CVideoInfoTag details;
  details.SetTags({"mi6", "secret agent", "undercover"});

  const std::vector<std::string> reference = {"mi6", "secret agent", "undercover"};
  EXPECT_EQ(details.m_tags, reference);
}

// Trick to make protected methods accessible for testing
class CVideoInfoTagTest : public CVideoInfoTag
{
public:
  bool ForwardSaveTvShowSeasons(TiXmlNode* node) { return SaveTvShowSeasons(node); }
};

TEST(TestVideoInfoTag, SaveTVShowSeasons)
{
  const std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", "plot 1"}}, {2, {"", "plot 2"}}, {3, {"season 3", ""}}, {4, {"", ""}}};

  const std::string referenceXml = R"(<namedseason number="1">season 1</namedseason>
<seasonplot number="1">plot 1</seasonplot>
<seasonplot number="2">plot 2</seasonplot>
<namedseason number="3">season 3</namedseason>
)";

  CVideoInfoTagTest details;
  details.SetSeasons(reference);

  CXBMCTinyXML xmlDoc;
  details.ForwardSaveTvShowSeasons(&xmlDoc);

  TiXmlPrinter printer;
  xmlDoc.Accept(&printer);
  std::string result = printer.Str();

  EXPECT_EQ(result, referenceXml);
}

TEST(TestVideoInfoTag, SetUniqueIDs)
{
  // initial state: no default, empty list.
  CVideoInfoTag details;
  std::map<std::string, std::string, std::less<>> reference = {};

  EXPECT_EQ(details.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // usual flow: initialize from initial state with a list.
  // entries with blank type or uniqueid are ignored
  std::map<std::string, std::string, std::less<>> test = {
      {"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}, {"", "123456"}, {"foo", ""}};
  reference = {{"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}};

  details.SetUniqueIDs(test);
  details.SetUniqueID("64043", "tmdb", true);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // current update behavior, not sure why:
  // the former default type and value from the previous list of uniqueids are added back when
  // omitted from the new list - instead of reverting to "unknown" default and setting the list as provided.
  test = {{"imdb", "tt4577466"}, {"tvdb", "299350"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // setting a blank list clears all except the previous default
  test = {};
  reference = {{"tmdb", "64043"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // except when there is no explicit default, then setting a blank list clears the list.
  CVideoInfoTag details2;
  details2.SetUniqueIDs(reference);
  details2.SetUniqueIDs(test);

  EXPECT_EQ(details2.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details2.GetUniqueIDs(), test);
}

struct TestOriginalLanguage
{
  std::string input;
  std::string expected;
  CVideoInfoTag::LanguageTagSource source = CVideoInfoTag::LanguageTagSource::SOURCE_EXTERNAL;
  bool status = true;
};

std::ostream& operator<<(std::ostream& os, const TestOriginalLanguage& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestOriginalLanguage OriginalLanguageTests[] = {
    {"en", "en", CVideoInfoTag::LanguageTagSource::SOURCE_INTERNAL},
    {"foobarbaz", "foobarbaz", CVideoInfoTag::LanguageTagSource::SOURCE_INTERNAL},
    {"en", "en"}, // ISO 639-1
    {"eng", "en"}, // ISO 639-2
    {"fra", "fr"}, // ISO 639-2/T
    {"fre", "fr"}, // ISO 639-2/B
    {"en-US", "en-US"}, // BCP 47 lang-region
    {"zh-guoyu", "zh-guoyu"}, // Grandfathered BCP 47
    // Future: expected to be rewritten to the preferred language defined in the registry
    // Other tests for canonicalization will be needed as well
    {"english", "en"}, // English name
    {"foobarbaz", "", CVideoInfoTag::LanguageTagSource::SOURCE_EXTERNAL, false}, // Unknown English name
};
// clang-format on

class OriginalLanguageTester : public testing::Test,
                               public testing::WithParamInterface<TestOriginalLanguage>
{
};

TEST_P(OriginalLanguageTester, SetOriginalLanguage)
{
  auto& param = GetParam();

  CVideoInfoTag tag;
  bool status = tag.SetOriginalLanguage(param.input, param.source);
  EXPECT_EQ(param.status, status);
  if (status)
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, tag.GetOriginalLanguage());
  }
}

INSTANTIATE_TEST_SUITE_P(TestVideoInfoTag,
                         OriginalLanguageTester,
                         testing::ValuesIn(OriginalLanguageTests));
