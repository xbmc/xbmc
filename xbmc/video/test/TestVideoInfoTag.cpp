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

#include <gtest/gtest.h>

// Trick to make protected methods accessible for unit testing and maintain encapsulation
class CVideoInfoTagTest : public CVideoInfoTag
{
public:
  bool CallSaveTvShowSeasons(TiXmlNode* node) { return SaveTvShowSeasons(node); }
  bool CallSaveUniqueId(TiXmlNode* node) { return SaveUniqueId(node); }
};

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
  details.CallSaveTvShowSeasons(&xmlDoc);

  TiXmlPrinter printer;
  xmlDoc.Accept(&printer);
  std::string result = printer.Str();

  EXPECT_EQ(result, referenceXml);
}

TEST(TestVideoInfoTag, LoadUniqueId)
{
  const std::string document = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
  <tvshow version="1">
    <uniqueid type="imdb">tt4577466</uniqueid>
    <uniqueid type="tmdb" default="true">64043</uniqueid>
    <uniqueid type="tvdb">299350</uniqueid>
  </tvshow>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  std::map<std::string, std::string, std::less<>> reference = {
      {"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}};

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);
}

TEST(TestVideoInfoTag, LoadUniqueIdLegacy)
{
  const std::string document = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
  <tvshow version="1">
    <uniqueid>64043</uniqueid>
  </tvshow>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  std::map<std::string, std::string, std::less<>> reference = {{"unknown", "64043"}};

  EXPECT_EQ(details.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details.GetUniqueIDs(), reference);
}

TEST(TestVideoInfoTag, SaveUniqueId)
{
  CVideoInfoTagTest details;
  details.SetUniqueID("123456", "", false);
  details.SetUniqueID("tt4577466", "imdb", false);
  details.SetUniqueID("64043", "tmdb", true);
  details.SetUniqueID("299350", "tvdb", false);

  const std::string expectedXml =
      R"(<uniqueid type="imdb">tt4577466</uniqueid>
<uniqueid type="tmdb" default="true">64043</uniqueid>
<uniqueid type="tvdb">299350</uniqueid>
<uniqueid type="unknown">123456</uniqueid>
)";

  CXBMCTinyXML xmlDoc;
  details.CallSaveUniqueId(&xmlDoc);

  //! @todo compare in TinyXml representation. Less sensitive to formatting (indentation, carriage returns...)
  TiXmlPrinter printer;
  xmlDoc.Accept(&printer);
  std::string result = printer.Str();

  EXPECT_EQ(result, expectedXml);
}
