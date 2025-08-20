/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NfoUtils.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <string>

#include <gtest/gtest.h>

TEST(TestNfoUtils, UpgradeUniqueId)
{
  const std::string document =
      R"(<tvshow>
         <id>64043</id>
         <id>65048</id>
         </tvshow>)";

  const std::string expectedXml = R"(<tvshow version="1">
    <uniqueid>64043</uniqueid>
    <uniqueid>65048</uniqueid>
</tvshow>
)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  EXPECT_TRUE(CNfoUtils::Upgrade(doc.RootElement()));

  //! @todo compare in TinyXml representation. Less sensitive to formatting (indentation, carriage returns...)
  TiXmlPrinter printer;
  doc.Accept(&printer);
  std::string result = printer.Str();

  EXPECT_EQ(result, expectedXml);
}

TEST(TestNfoUtils, UpgradeUniqueId2)
{
  const std::string document =
      R"(<tvshow>
         <id>64043</id>
         <id>65048</id>
         <uniqueid type="imdb">64043</uniqueid>
         </tvshow>)";

  const std::string expectedXml = R"(<tvshow version="1">
    <uniqueid type="imdb">64043</uniqueid>
</tvshow>
)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  EXPECT_TRUE(CNfoUtils::Upgrade(doc.RootElement()));

  //! @todo compare in TinyXml representation. Less sensitive to formatting (indentation, carriage returns...)
  TiXmlPrinter printer;
  doc.Accept(&printer);
  std::string result = printer.Str();

  EXPECT_EQ(result, expectedXml);
}
