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

TEST(TestVideoInfoTag, ReadTVShowSeasons)
{
  const std::string document =
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
         <tvshow>
         <seasondetails number="1"><name>season 1</name><plot>plot 1</plot></seasondetails>
         <seasondetails number="2"><name></name><plot>plot 2</plot></seasondetails>
         <seasondetails number="3"><plot>plot 3</plot></seasondetails>
         <seasondetails number="4"><name>season 4</name></seasondetails>
         <seasondetails number="5"></seasondetails>
         <seasondetails number="abc"><name>season abc</name></seasondetails>
         <seasondetails><name>season abc</name></seasondetails>
         </tvshow>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", "plot 1"}}, {2, {"", "plot 2"}}, {3, {"", "plot 3"}}, {4, {"season 4", ""}}};

  EXPECT_EQ(details.m_seasons, reference);
}

TEST(TestVideoInfoTag, SaveTVShowSeasons)
{
  std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", "plot 1"}}, {2, {"", "plot 2"}}, {3, {"season 3", ""}}, {4, {"", ""}}};

  std::string referenceXml = R"(<seasondetails number="1">
    <name>season 1</name>
    <plot>plot 1</plot>
</seasondetails>
<seasondetails number="2">
    <plot>plot 2</plot>
</seasondetails>
<seasondetails number="3">
    <name>season 3</name>
</seasondetails>
)";

  CVideoInfoTag details;
  details.SetSeasons(reference);

  CXBMCTinyXML xmlDoc;
  details.SaveTvShowSeasons(&xmlDoc);

  TiXmlPrinter printer;
  xmlDoc.Accept(&printer);
  std::string result = printer.Str();

  EXPECT_EQ(result, referenceXml);
}
