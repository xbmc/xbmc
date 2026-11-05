/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUIFontTTF.h"
#include "guilib/Texture.h"
#include "guilib/test/TestGUIStubs.h"
#include "test/TestUtils.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class CTestTexture : public CTexture
{
public:
  using CTexture::CTexture;
  void CreateTextureObject() override {}
  void DestroyTextureObject() override {}
  void LoadToGPU() override {}
  void BindToUnit(unsigned int unit) override {}
};

class CTestFont : public CGUIFontTTF
{
public:
  CTestFont() : CGUIFontTTF("test") {}

  using CGUIFontTTF::FontFace;
  using CGUIFontTTF::GetCharacter;
  using CGUIFontTTF::GetHarfBuzzShapedGlyphs;
  using CGUIFontTTF::m_color;
  using CGUIFontTTF::RenderCharacter;

  std::vector<uint8_t> m_pixels;

private:
  std::unique_ptr<CTexture> ReallocTexture(unsigned int& newHeight) override
  {
    newHeight = CTexture::PadPow2(newHeight);
    m_textureHeight = newHeight;
    m_textureScaleY = 1.0f / newHeight;
    return std::make_unique<CTestTexture>(m_textureWidth, newHeight);
  }
  bool CopyCharToTexture(const uint8_t* pixels,
                         unsigned int pitch,
                         unsigned int x1,
                         unsigned int y1,
                         unsigned int x2,
                         unsigned int y2) override
  {
    m_pixels.assign(pixels, pixels + pitch * (y2 - y1));
    return true;
  }
  void DeleteHardwareTexture() override {}
  bool FirstBegin() override { return false; }
  void LastEnd() override {}
};

using FontFace = CTestFont::FontFace;
constexpr char PRIMARY_FONT[] = "addons/skin.estuary/fonts/NotoSans-Regular.ttf";
constexpr character_t SYMBOL = 0x25D3;
} // namespace

class TestGUIFontTTF : public testing::Test
{
protected:
  void SetUp() override
  {
    m_previousWinSystem = CServiceBroker::GetWinSystem();
    CServiceBroker::RegisterWinSystem(&m_winSystem);
    m_font = std::make_unique<CTestFont>();
    ASSERT_TRUE(m_font->Load(XBMC_REF_FILE_PATH(PRIMARY_FONT)));
  }

  void TearDown() override
  {
    m_font.reset();
    if (m_previousWinSystem)
      CServiceBroker::RegisterWinSystem(m_previousWinSystem);
    else
      CServiceBroker::UnregisterWinSystem();
  }

  KODI::GUILIB::TEST::CTestWinSystem m_winSystem;
  CWinSystemBase* m_previousWinSystem{nullptr};
  std::unique_ptr<CTestFont> m_font;
};

TEST_F(TestGUIFontTTF, MissingSymbolUsesFallback)
{
  const auto glyphs = m_font->GetHarfBuzzShapedGlyphs(vecText{SYMBOL});
  ASSERT_EQ(glyphs.size(), 1u);
  EXPECT_NE(glyphs[0].m_glyphInfo.codepoint, 0u);
  EXPECT_EQ(glyphs[0].m_fontFace, FontFace::Symbols);
}

TEST_F(TestGUIFontTTF, SymbolInCaptionKeepsTextClusters)
{
  const std::u32string caption = U"8 Badges [👁️121/251] [◓145/251] [🅰26/26]";
  const vecText text(caption.begin(), caption.end());
  const auto glyphs = m_font->GetHarfBuzzShapedGlyphs(text);
  ASSERT_EQ(glyphs.size(), text.size() - 1);
  for (const auto& glyph : glyphs)
  {
    EXPECT_NE(glyph.m_glyphInfo.codepoint, 0u);
    ASSERT_LT(glyph.m_glyphInfo.cluster, text.size());
    const character_t codepoint = text[glyph.m_glyphInfo.cluster];
    EXPECT_EQ(glyph.m_fontFace, codepoint == SYMBOL                            ? FontFace::Symbols
                                : codepoint == 0x1F441 || codepoint == 0x1F170 ? FontFace::Emoji
                                                                               : FontFace::Primary);
  }
}

TEST_F(TestGUIFontTTF, PrimarySymbolTakesPrecedence)
{
  CTestFont font;
  ASSERT_TRUE(font.Load(XBMC_REF_FILE_PATH("media/Fonts/arial.ttf")));
  const auto glyphs = font.GetHarfBuzzShapedGlyphs(vecText{SYMBOL});
  ASSERT_EQ(glyphs.size(), 1u);
  EXPECT_NE(glyphs[0].m_glyphInfo.codepoint, 0u);
  EXPECT_EQ(glyphs[0].m_fontFace, FontFace::Primary);
}

TEST_F(TestGUIFontTTF, EmojiSequencesStayTogetherBesideSymbols)
{
  for (const vecText& sequence : {vecText{0x263A, 0xFE0F}, vecText{0x1F469, 0x200D, 0x1F4BB},
                                  vecText{0x1F44D, 0x1F3FD}, vecText{0x1F1FA, 0x1F1F8}})
  {
    SCOPED_TRACE(testing::PrintToString(sequence));
    for (const bool prefixText : {false, true})
    {
      SCOPED_TRACE(prefixText);
      const size_t firstEmoji = prefixText ? 1 : 0;
      vecText referenceText(firstEmoji, U'A');
      referenceText.insert(referenceText.end(), sequence.begin(), sequence.end());
      const auto reference = m_font->GetHarfBuzzShapedGlyphs(referenceText);
      ASSERT_GT(reference.size(), firstEmoji);
      EXPECT_NE(reference[firstEmoji].m_glyphInfo.codepoint, 0u);

      vecText text = referenceText;
      text.insert(text.begin() + firstEmoji, SYMBOL);
      text.push_back(SYMBOL);
      const auto glyphs = m_font->GetHarfBuzzShapedGlyphs(text);
      ASSERT_EQ(glyphs.size(), reference.size() + 2);
      EXPECT_EQ(glyphs[firstEmoji].m_fontFace, FontFace::Symbols);
      for (size_t i = firstEmoji; i < reference.size(); ++i)
      {
        EXPECT_EQ(reference[i].m_fontFace, FontFace::Emoji);
        EXPECT_EQ(glyphs[i + 1].m_fontFace, FontFace::Emoji);
        EXPECT_EQ(glyphs[i + 1].m_glyphInfo.codepoint, reference[i].m_glyphInfo.codepoint);
        EXPECT_EQ(glyphs[i + 1].m_glyphInfo.cluster, reference[i].m_glyphInfo.cluster + 1);
        EXPECT_EQ(glyphs[i + 1].m_glyphPosition.x_advance, reference[i].m_glyphPosition.x_advance);
        EXPECT_EQ(glyphs[i + 1].m_glyphPosition.y_advance, reference[i].m_glyphPosition.y_advance);
        EXPECT_EQ(glyphs[i + 1].m_glyphPosition.x_offset, reference[i].m_glyphPosition.x_offset);
        EXPECT_EQ(glyphs[i + 1].m_glyphPosition.y_offset, reference[i].m_glyphPosition.y_offset);
      }
      EXPECT_EQ(glyphs.back().m_fontFace, FontFace::Symbols);
      EXPECT_EQ(glyphs.back().m_glyphInfo.cluster, text.size() - 1);
    }
  }

  const auto textGlyphs = m_font->GetHarfBuzzShapedGlyphs(vecText{0x263A, 0xFE0E});
  ASSERT_FALSE(textGlyphs.empty());
  for (const auto& glyph : textGlyphs)
    EXPECT_EQ(glyph.m_fontFace, FontFace::Primary);
}

TEST_F(TestGUIFontTTF, CacheSeparatesEqualGlyphIndicesFromDifferentFaces)
{
  const auto glyphs = m_font->GetHarfBuzzShapedGlyphs(vecText{SYMBOL});
  ASSERT_EQ(glyphs.size(), 1u);
  const FT_UInt glyphIndex = glyphs[0].m_glyphInfo.codepoint;
  const std::array faces{FontFace::Primary, FontFace::Symbols, FontFace::Emoji};
  std::array<character_t, 3> keys{};
  for (size_t i = 0; i < faces.size(); ++i)
  {
    const auto* character = m_font->GetCharacter(SYMBOL, glyphIndex, faces[i]);
    ASSERT_NE(character, nullptr);
    keys[i] = character->m_glyphAndStyle;
  }
  EXPECT_NE(keys[0], keys[1]);
  EXPECT_NE(keys[0], keys[2]);
  EXPECT_NE(keys[1], keys[2]);
  for (size_t i = 0; i < faces.size(); ++i)
  {
    const auto* character = m_font->GetCharacter(SYMBOL, glyphIndex, faces[i]);
    ASSERT_NE(character, nullptr);
    EXPECT_EQ(character->m_glyphAndStyle, keys[i]);
  }
}

TEST_F(TestGUIFontTTF, SymbolKeepsTextStylesAndColor)
{
  const auto glyphs = m_font->GetHarfBuzzShapedGlyphs(vecText{SYMBOL});
  ASSERT_EQ(glyphs.size(), 1u);
  const FT_UInt glyphIndex = glyphs[0].m_glyphInfo.codepoint;
  const auto* normal = m_font->GetCharacter(SYMBOL, glyphIndex, FontFace::Symbols);
  ASSERT_NE(normal, nullptr);
  const auto normalPixels = m_font->m_pixels;
  const auto normalKey = normal->m_glyphAndStyle;
  ASSERT_FALSE(normalPixels.empty());
  bool visible = false;
  for (size_t i = 0; i < normalPixels.size(); i += 4)
  {
    EXPECT_EQ(normalPixels[i], 255);
    EXPECT_EQ(normalPixels[i + 1], 255);
    EXPECT_EQ(normalPixels[i + 2], 255);
    visible |= normalPixels[i + 3] != 0;
  }
  EXPECT_TRUE(visible);

  constexpr KODI::UTILS::COLOR::Color color = 0x7F129B30;
  std::vector<SVertex> vertices;
  m_font->RenderCharacter(m_winSystem.GetGfxContext(), 0, 0, normal, color, true, vertices);
  EXPECT_FALSE(vertices.empty());
  EXPECT_EQ(m_font->m_color, color);

  const auto* shadow = m_font->GetCharacter(SYMBOL, glyphIndex, FontFace::Symbols, true);
  ASSERT_NE(shadow, nullptr);
  EXPECT_EQ(shadow->m_glyphAndStyle, normalKey);

  for (const uint32_t style : {FONT_STYLE_BOLD, FONT_STYLE_ITALICS, FONT_STYLE_LIGHT})
  {
    const auto* styled = m_font->GetCharacter(SYMBOL | (static_cast<character_t>(style) << 40),
                                              glyphIndex, FontFace::Symbols);
    ASSERT_NE(styled, nullptr);
    EXPECT_NE(styled->m_glyphAndStyle, normalKey);
    EXPECT_NE(m_font->m_pixels, normalPixels);
  }
}

TEST_F(TestGUIFontTTF, SymbolReceivesTextBorder)
{
  const auto glyphs = m_font->GetHarfBuzzShapedGlyphs(vecText{SYMBOL});
  ASSERT_EQ(glyphs.size(), 1u);
  const FT_UInt glyphIndex = glyphs[0].m_glyphInfo.codepoint;
  const auto* normal = m_font->GetCharacter(SYMBOL, glyphIndex, FontFace::Symbols);
  ASSERT_NE(normal, nullptr);

  CTestFont borderFont;
  ASSERT_TRUE(borderFont.Load(XBMC_REF_FILE_PATH(PRIMARY_FONT), 20.0f, 1.0f, 1.0f, true));
  const auto* border = borderFont.GetCharacter(SYMBOL, glyphIndex, FontFace::Symbols, true);
  ASSERT_NE(border, nullptr);
  EXPECT_GT(border->m_right - border->m_left, normal->m_right - normal->m_left);
  EXPECT_GT(border->m_bottom - border->m_top, normal->m_bottom - normal->m_top);
}
