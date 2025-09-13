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
         <id>12345</id>
         <uniqueid type="imdb">64043</uniqueid>
         <id>23456</id>
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

TEST(TestNfoUtils, UpgradeRating)
{
  // with all possible attributes
  std::string document =
      R"(<tvshow>
           <rating max="12.34">1.23</rating>
           <votes>234</votes>
         </tvshow>)";

  std::string expectedXml = R"(<tvshow version="1">
    <ratings>
        <rating max="12.34">
            <value>1.23</value>
            <votes>234</votes>
        </rating>
    </ratings>
</tvshow>
)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  EXPECT_TRUE(CNfoUtils::Upgrade(doc.RootElement()));

  //! @todo compare in TinyXml representation. Less sensitive to formatting (indentation, carriage returns...)
  std::string result;
  {
    TiXmlPrinter printer;
    doc.Accept(&printer);
    result = printer.Str();
  }
  EXPECT_EQ(result, expectedXml);

  // with minimum attributes
  document =
      R"(<tvshow>
           <rating>1.23</rating>
         </tvshow>)";

  expectedXml = R"(<tvshow version="1">
    <ratings>
        <rating>
            <value>1.23</value>
        </rating>
    </ratings>
</tvshow>
)";

  doc.Clear();
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  EXPECT_TRUE(CNfoUtils::Upgrade(doc.RootElement()));

  //! @todo compare in TinyXml representation. Less sensitive to formatting (indentation, carriage returns...)
  {
    TiXmlPrinter printer;
    doc.Accept(&printer);
    result = printer.Str();
  }
  EXPECT_EQ(result, expectedXml);
}

TEST(TestNfoUtils, UpgradeRating2)
{
  // Presence of <ratings> node cancels conversion of existing <rating> or <votes> nodes.
  const std::string document =
      R"(<tvshow>
           <ratings><rating max="12.34"><value>1.23</value><votes>234</votes></rating></ratings>
           <rating max="12.34">1.23</rating>
           <rating max="23.45">2.34</rating>
           <votes>234</votes>
           <votes>345</votes>
         </tvshow>)";

  const std::string expectedXml = R"(<tvshow version="1">
    <ratings>
        <rating max="12.34">
            <value>1.23</value>
            <votes>234</votes>
        </rating>
    </ratings>
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

TEST(TestNfoUtils, UpgradeSet)
{
  // legacy-only tags
  std::string document =
      R"(<movie>
           <set></set>
           <set>set name</set>
           <set>other set</set>
         </movie>)";

  std::string expectedXml = R"(<movie version="1">
    <set>
        <name>set name</name>
    </set>
</movie>
)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  EXPECT_TRUE(CNfoUtils::Upgrade(doc.RootElement()));

  //! @todo compare in TinyXml representation. Less sensitive to formatting (indentation, carriage returns...)
  std::string result;
  {
    TiXmlPrinter printer;
    doc.Accept(&printer);
    result = printer.Str();
  }
  EXPECT_EQ(result, expectedXml);
}

TEST(TestNfoUtils, UpgradeSet2)
{
  // With both old and new style tags
  std::string document =
      R"(<movie>
           <set>name</set>
           <set><name>Set 1</name></set>
           <set><name>Set 2</name></set>
           <set>other set</set>
         </movie>)";

  std::string expectedXml = R"(<movie version="1">
    <set>
        <name>Set 1</name>
    </set>
    <set>
        <name>Set 2</name>
    </set>
</movie>
)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  EXPECT_TRUE(CNfoUtils::Upgrade(doc.RootElement()));

  //! @todo compare in TinyXml representation. Less sensitive to formatting (indentation, carriage returns...)
  std::string result;
  {
    TiXmlPrinter printer;
    doc.Accept(&printer);
    result = printer.Str();
  }
  EXPECT_EQ(result, expectedXml);
}
