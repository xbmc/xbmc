/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIColorManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIFontParsing.h"
#include "utils/XBMCTinyXML.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
// Stands in for FontEntry. The real one holds a CGUIFont, which needs a GL
// context, so the erase logic is tested through the same template on a stub.
struct StubEntry
{
  std::string name;
  int info;
};

std::string StubName(const StubEntry& e)
{
  return e.name;
}
} // namespace

TEST(TestGUIFontParsing, EraseFirstByNameRemovesMatch)
{
  std::vector<StubEntry> entries{{"font12", 12}, {"font13", 13}, {"font14", 14}};

  EXPECT_TRUE(EraseFirstByName(entries, "font13", StubName));

  ASSERT_EQ(entries.size(), 2U);
  EXPECT_EQ(entries[0].name, "font12");
  EXPECT_EQ(entries[1].name, "font14");
}

TEST(TestGUIFontParsing, EraseFirstByNameKeepsNameAndInfoPaired)
{
  // Regression: the old code erased from m_vecFonts but not the index-aligned
  // m_vecFontInfo, so every later entry's info belonged to a different font.
  std::vector<StubEntry> entries{{"font12", 12}, {"font13", 13}, {"font14", 14}};

  EraseFirstByName(entries, "font12", StubName);

  for (const auto& e : entries)
    EXPECT_EQ(e.info, std::stoi(e.name.substr(4)));
}

TEST(TestGUIFontParsing, EraseFirstByNameIsCaseInsensitive)
{
  std::vector<StubEntry> entries{{"Font13", 13}};

  EXPECT_TRUE(EraseFirstByName(entries, "font13", StubName));
  EXPECT_TRUE(entries.empty());
}

TEST(TestGUIFontParsing, EraseFirstByNameReturnsFalseWhenAbsent)
{
  std::vector<StubEntry> entries{{"font12", 12}};

  EXPECT_FALSE(EraseFirstByName(entries, "font99", StubName));
  EXPECT_EQ(entries.size(), 1U);
}

namespace
{
// Parses a <fontset> document and hands back the first <font> child, which is
// what ParseFontSet expects, mirroring LoadFontsFromFile's call.
const TiXmlNode* FirstFontNode(CXBMCTinyXML& doc, const std::string& xml)
{
  EXPECT_TRUE(doc.Parse(xml));
  const TiXmlElement* root = doc.RootElement();
  EXPECT_NE(root, nullptr);
  return root->FirstChild("font");
}
} // namespace

TEST(TestGUIFontParsing, ParseFontSetReadsNameFileAndSize)
{
  CXBMCTinyXML doc;
  const auto* node = FirstFontNode(doc, R"(
    <fontset id="Default">
      <font><name>font12</name><filename>MyFont.ttf</filename><size>24</size></font>
      <font><name>font14</name><filename>Other.ttf</filename><size>30</size></font>
    </fontset>)");

  const std::vector<FontDefinition> fonts = ParseFontSet(node);

  ASSERT_EQ(fonts.size(), 2U);
  EXPECT_EQ(fonts[0].name, "font12");
  EXPECT_EQ(fonts[0].fileName, "MyFont.ttf");
  EXPECT_EQ(fonts[0].size, 24);
  EXPECT_EQ(fonts[1].name, "font14");
  EXPECT_EQ(fonts[1].size, 30);
}

TEST(TestGUIFontParsing, ParseFontSetSkipsEmptyNameAndNonTtf)
{
  CXBMCTinyXML doc;
  const auto* node = FirstFontNode(doc, R"(
    <fontset id="Default">
      <font><name></name><filename>MyFont.ttf</filename></font>
      <font><name>font12</name><filename>MyFont.otf</filename></font>
      <font><name>font13</name><filename>Good.ttf</filename></font>
    </fontset>)");

  const std::vector<FontDefinition> fonts = ParseFontSet(node);

  ASSERT_EQ(fonts.size(), 1U);
  EXPECT_EQ(fonts[0].name, "font13");
}

TEST(TestGUIFontParsing, ParseFontSetKeepsEntryWithNoFilename)
{
  // An omitted <filename> means "inherit the active skin's typeface". The
  // parser must pass it through; only the skin's own loader rejects it.
  CXBMCTinyXML doc;
  const auto* node = FirstFontNode(doc, R"(
    <fontset id="Default">
      <font><name>title</name><size>46</size><style>bold</style></font>
    </fontset>)");

  const std::vector<FontDefinition> fonts = ParseFontSet(node);

  ASSERT_EQ(fonts.size(), 1U);
  EXPECT_EQ(fonts[0].name, "title");
  EXPECT_TRUE(fonts[0].fileName.empty());
  EXPECT_EQ(fonts[0].size, 46);
  EXPECT_EQ(fonts[0].style, FONT_STYLE_BOLD);
}

TEST(TestGUIFontParsing, ParseFontSetStillSkipsNonTtfWhenFilenamePresent)
{
  // Allowing an empty filename must not let a non-ttf through.
  CXBMCTinyXML doc;
  const auto* node = FirstFontNode(doc, R"(
    <fontset id="Default">
      <font><name>bad</name><filename>MyFont.otf</filename></font>
      <font><name>inherit</name></font>
    </fontset>)");

  const std::vector<FontDefinition> fonts = ParseFontSet(node);

  ASSERT_EQ(fonts.size(), 1U);
  EXPECT_EQ(fonts[0].name, "inherit");
}

TEST(TestGUIFontParsing, ParseFontSetAppliesDefaultsWhenAbsent)
{
  CXBMCTinyXML doc;
  const auto* node = FirstFontNode(doc, R"(
    <fontset id="Default">
      <font><name>font13</name><filename>Good.ttf</filename></font>
    </fontset>)");

  const std::vector<FontDefinition> fonts = ParseFontSet(node);

  ASSERT_EQ(fonts.size(), 1U);
  EXPECT_EQ(fonts[0].size, 20);
  EXPECT_FLOAT_EQ(fonts[0].aspect, 1.0f);
  EXPECT_FLOAT_EQ(fonts[0].lineSpacing, 1.0f);
  EXPECT_EQ(fonts[0].style, FONT_STYLE_NORMAL);
}

TEST(TestGUIFontParsing, ParseFontSetReadsStyleBitmask)
{
  CXBMCTinyXML doc;
  const auto* node = FirstFontNode(doc, R"(
    <fontset id="Default">
      <font><name>f</name><filename>a.ttf</filename><style>bold italics</style></font>
    </fontset>)");

  const std::vector<FontDefinition> fonts = ParseFontSet(node);

  ASSERT_EQ(fonts.size(), 1U);
  EXPECT_EQ(fonts[0].style, FONT_STYLE_BOLD | FONT_STYLE_ITALICS);
}

TEST(TestGUIFontParsing, ParseFontSetOnNullNodeYieldsEmpty)
{
  EXPECT_TRUE(ParseFontSet(nullptr).empty());
}

namespace
{
// ParseFontSet resolves <color>/<shadow> names through the colour manager, so
// this fixture registers a CGUIComponent the same way
// TestGUIControlFactory.cpp does for its GetColor tests.
class CGUITestColorManager : public CGUIColorManager
{
public:
  CGUITestColorManager()
  {
    CXBMCTinyXML xmlDoc;
    xmlDoc.Parse(std::string(R"(<colors>
                      <color name="red">ffff0000</color>
                      <color name="blue">ff0000ff</color>
                    </colors>)"));
    LoadXML(xmlDoc);
  }
};

class CGUITestComponent : public CGUIComponent
{
public:
  CGUITestComponent() : CGUIComponent(false)
  {
    m_guiColorManager = std::make_unique<CGUITestColorManager>();
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
  }
};
} // namespace

TEST(TestGUIFontParsing, ParseFontSetReadsColorAndShadow)
{
  CGUITestComponent comp;

  CXBMCTinyXML doc;
  const auto* node = FirstFontNode(doc, R"(
    <fontset id="Default">
      <font>
        <name>font12</name>
        <filename>MyFont.ttf</filename>
        <color>red</color>
        <shadow>blue</shadow>
      </font>
    </fontset>)");

  const std::vector<FontDefinition> fonts = ParseFontSet(node);

  ASSERT_EQ(fonts.size(), 1U);
  EXPECT_EQ(fonts[0].textColor, 0xFFFF0000U);
  EXPECT_EQ(fonts[0].shadowColor, 0xFF0000FFU);
}

TEST(TestGUIFontParsing, StripIncludesRemovesFileAndConditionIncludes)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string(R"xml(
    <fonts>
      <include file="resource://resource.font.evil/Font.xml" />
      <include condition="System.HasAddon(x)">Something</include>
      <fontset id="Default"><font><name>f</name><filename>a.ttf</filename></font></fontset>
    </fonts>)xml")));

  EXPECT_EQ(StripIncludes(doc.RootElement()), 2);
  EXPECT_EQ(doc.RootElement()->FirstChildElement("include"), nullptr);
  EXPECT_NE(doc.RootElement()->FirstChildElement("fontset"), nullptr);
}

TEST(TestGUIFontParsing, StripIncludesLeavesCleanDocumentAlone)
{
  CXBMCTinyXML doc;
  ASSERT_TRUE(doc.Parse(std::string(
      R"(<fonts><fontset id="Default"><font><name>f</name><filename>a.ttf</filename></font></fontset></fonts>)")));

  EXPECT_EQ(StripIncludes(doc.RootElement()), 0);
  EXPECT_NE(doc.RootElement()->FirstChildElement("fontset"), nullptr);
}

TEST(TestGUIFontParsing, StripIncludesOnNullRootIsSafe)
{
  EXPECT_EQ(StripIncludes(nullptr), 0);
}

TEST(TestGUIFontParsing, EscapeFontNameNeutralisesLogInjection)
{
  EXPECT_EQ(EscapeFontName("font12"), "font12");
  EXPECT_EQ(EscapeFontName("a\nERROR: forged"), "a\\nERROR: forged");
  EXPECT_EQ(EscapeFontName("a\r\nb"), "a\\r\\nb");
  EXPECT_EQ(EscapeFontName("a\tb"), "a\\tb");
}

TEST(TestGUIFontParsing, EscapeFontNameTruncatesAbsurdlyLongNames)
{
  const std::string huge(1000, 'x');
  EXPECT_LE(EscapeFontName(huge).size(), 128U);
}

TEST(TestGUIFontParsing, IsAddonSafeFontFilenameRejectsEscapes)
{
  EXPECT_TRUE(IsAddonSafeFontFilename("MyFont.ttf"));
  EXPECT_TRUE(IsAddonSafeFontFilename("sub/MyFont.ttf"));

  EXPECT_FALSE(IsAddonSafeFontFilename("/etc/passwd.ttf"));
  EXPECT_FALSE(IsAddonSafeFontFilename("special://home/media/Fonts/x.ttf"));
  EXPECT_FALSE(IsAddonSafeFontFilename("smb://host/share/x.ttf"));
  EXPECT_FALSE(IsAddonSafeFontFilename("C:\\Windows\\Fonts\\x.ttf"));
  EXPECT_FALSE(IsAddonSafeFontFilename("../../../../etc/x.ttf"));
}
