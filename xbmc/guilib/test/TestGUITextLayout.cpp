/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUITextLayout.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

// CGUITextLayout::Filter strips recognised format tags and returns the
// remaining plain UTF-8 text. It is used here as the test entry point for
// ParseText tag-parsing logic introduced in PRs #28280 and #28356.
//
// [COLOR] tags are not tested here because they call into CServiceBroker
// and require a full service-broker setup.

namespace
{

std::string Filter(const std::string& input)
{
  std::string text = input;
  CGUITextLayout::Filter(text);
  return text;
}

} // namespace

struct TagTestParam
{
  std::string input;
  std::string expected;
};

class TestGUITextLayoutFilterTags : public testing::TestWithParam<TagTestParam>
{
};

TEST_P(TestGUITextLayoutFilterTags, ReturnsExpectedText)
{
  EXPECT_EQ(GetParam().expected, Filter(GetParam().input));
}

static const std::vector<TagTestParam> filterTestCases{
    // Plain text
    {"", ""},
    {"Hello world", "Hello world"},

    // Matched style tags
    {"[B]bold[/B]", "bold"},
    {"[I]italic[/I]", "italic"},
    {"[LIGHT]text[/LIGHT]", "text"},
    {"prefix [B]bold[/B] suffix", "prefix bold suffix"},

    // Case-transform tags also apply the transform
    {"[UPPERCASE]hello[/UPPERCASE]", "HELLO"},
    {"[LOWERCASE]HELLO[/LOWERCASE]", "hello"},
    {"[CAPITALIZE]hello world[/CAPITALIZE]", "Hello World"},

    // CR tag becomes newline
    {"a[CR]b", "a\nb"},

    // TABS tag inserts the specified number of tab characters
    {"[TABS]1[/TABS]label", "\tlabel"},
    {"[TABS]2[/TABS]label", "\t\tlabel"},
    {"[TABS]3[/TABS]label", "\t\t\tlabel"},

    // Unmatched [TABS] without [/TABS]: the tag token is consumed but no
    // tabs are emitted; content after the tag passes through unchanged.
    {"[TABS]1 label", "1 label"},
    {"label[/TABS]", "label"},

    // Nested identical tags (PR #28280)
    {"[B][B]text[/B][/B]", "text"},
    {"[B][B][B]text[/B][/B][/B]", "text"},
    {"[I][I]text[/I][/I]", "text"},
    {"[LIGHT][LIGHT]text[/LIGHT][/LIGHT]", "text"},
    {"[UPPERCASE][UPPERCASE]hello[/UPPERCASE][/UPPERCASE]", "HELLO"},
    {"[LOWERCASE][LOWERCASE]HELLO[/LOWERCASE][/LOWERCASE]", "hello"},
    {"[CAPITALIZE][CAPITALIZE]hello world[/CAPITALIZE][/CAPITALIZE]", "Hello World"},

    // Unmatched opening tags (PR #28356 lookahead)
    {"[B]no close", "no close"},
    {"[I]no close", "no close"},
    {"[LIGHT]no close", "no close"},
    {"[UPPERCASE]no close", "no close"},
    {"[LOWERCASE]no close", "no close"},
    {"[CAPITALIZE]no close", "no close"},
    {"text[B]", "text"},

    // Unmatched closing tags
    {"[/B]text", "text"},
    {"text [/B] more", "text  more"},
    {"text [/I] more", "text  more"},
    {"text [/LIGHT] more", "text  more"},
    {"text [/UPPERCASE] more", "text  more"},
    {"text [/LOWERCASE] more", "text  more"},
    {"text [/CAPITALIZE] more", "text  more"},
    {"[B]text[/B][/B]", "text"},
    {"[B][B]text[/B][/B][/B]", "text"},
    {"[/B][/B][/B][B]valid[/B]", "valid"},
    {"[B]first[/B] [/B] [B]second[/B]", "first  second"},

    // Mixed / edge cases
    {"[B]first[/B] mid [B]second[/B]", "first mid second"},
    {"[B][I]bold+italic[/I][/B]", "bold+italic"},
    {"[B][I][LIGHT]content[/LIGHT][/I][/B]", "content"},
    {"[B][I]text[/B][/I]", "text"},
    {"[B][B]styled[/B][/B] plain", "styled plain"},
    {"[B][/B]", ""},
    {"[B][B][/B][/B]", ""},
    {"[b]text[/b]", "[b]text[/b]"},
};

INSTANTIATE_TEST_SUITE_P(TestGUITextLayoutFilter,
                         TestGUITextLayoutFilterTags,
                         testing::ValuesIn(filterTestCases));

TEST(TestGUITextLayoutFilter, VeryLongPlainText)
{
  const std::string text(10000, 'A');
  EXPECT_EQ(text, Filter(text));
}

TEST(TestGUITextLayoutFilter, VeryLongNestedBold)
{
  const std::string inner(5000, 'X');
  EXPECT_EQ(inner, Filter("[B]" + inner + "[/B]"));
}

TEST(TestGUITextLayoutFilter, DeeplyNested100Levels)
{
  std::string open, close;
  for (int i = 0; i < 100; ++i)
  {
    open += "[B]";
    close = "[/B]" + close;
  }
  EXPECT_EQ("deep", Filter(open + "deep" + close));
}

TEST(TestGUITextLayoutFilter, DeeplyNested100Levels_OneMissingClose)
{
  // 100 opens, 99 closes - outermost [B] has no matching close so is ignored.
  // Parser must not crash; content must be extracted.
  std::string open, close;
  for (int i = 0; i < 100; ++i)
    open += "[B]";
  for (int i = 0; i < 99; ++i)
    close += "[/B]";
  const std::string result = Filter(open + "deep" + close);
  EXPECT_NE(result.find("deep"), std::string::npos);
}

// ---------------------------------------------------------------------------
// CGUITextLayout::WrapText
//
// Runs the real wrapping code path (UpdateW -> ParseText -> WrapText -> Bidi)
// against a fixed-width fake font so no window system or FreeType is needed.
// Latin glyphs are 1 unit wide, CJK glyphs 2 units wide.
// ---------------------------------------------------------------------------

#include "guilib/GUIFont.h"

#include <span>

namespace
{

class CFixedWidthFont : public CGUIFont
{
public:
  CFixedWidthFont() : CGUIFont("fixed", 0, 0, 0, 1.0f, 10.0f, nullptr) {}

  static float GlyphWidth(character_t letter) { return (letter & 0xffff) >= 0x2E80 ? 2.0f : 1.0f; }

  float GetTextWidth(std::span<const character_t> text) override
  {
    float width = 0.0f;
    for (character_t letter : text)
      width += GlyphWidth(letter);
    return width;
  }
  float GetTextHeight(int numLines) const override { return static_cast<float>(numLines); }
  float GetLineHeight() const override { return 1.0f; }
};

class CWrapLayout : public CGUITextLayout
{
public:
  CWrapLayout(CGUIFont* font, float maxHeight) : CGUITextLayout(font, true, maxHeight) {}

  std::vector<std::wstring> GetLines() const
  {
    std::vector<std::wstring> lines;
    for (const CGUIString& line : m_lines)
    {
      std::wstring text;
      for (character_t letter : line.m_text)
        text.push_back(static_cast<wchar_t>(letter & 0xffff));
      lines.push_back(std::move(text));
    }
    return lines;
  }
};

std::vector<std::wstring> Wrap(const std::wstring& text, float maxWidth, float maxHeight = 0.0f)
{
  CFixedWidthFont font;
  CWrapLayout layout(&font, maxHeight);
  layout.UpdateW(text, maxWidth, true, true);
  return layout.GetLines();
}

std::wstring StripSpaces(const std::wstring& text)
{
  std::wstring out;
  for (wchar_t ch : text)
    if (ch != L' ' && ch != L'　' && ch != L'\n')
      out.push_back(ch);
  return out;
}

std::wstring Join(const std::vector<std::wstring>& lines)
{
  std::wstring out;
  for (const std::wstring& line : lines)
    out += line;
  return out;
}

using Lines = std::vector<std::wstring>;

struct WrapTestParam
{
  std::string name;
  std::wstring text;
  float maxWidth;
  Lines expected;
};

// U+4E2D U+6587 = "中文", U+7684 = "的", U+3000 = ideographic space
const std::wstring kZhongWen = L"中文";
const std::wstring kDe = L"的";
const std::wstring kIdeographicSpace = L"　";

const std::vector<WrapTestParam> wrapCases{
    // Latin behaviour that must not change
    {"LatinWords", L"hello world foo", 5.0f, {L"hello", L"world", L"foo"}},
    {"LatinKeepsWordOnLineWhenItFits", L"ab cd ef", 5.0f, {L"ab cd", L"ef"}},
    {"LatinCollapsesSurroundingSpaces", L"  hello   world  ", 5.0f, {L"hello", L"world"}},
    {"LatinSplitsOverlongWordByCharacter", L"abcdefgh", 3.0f, {L"abc", L"def", L"gh"}},
    {"LatinOverlongWordAfterShortWord", L"ab cdefgh", 3.0f, {L"ab", L"cde", L"fgh"}},
    {"LatinParagraphBreakIsKept", L"hello\nworld", 20.0f, {L"hello", L"world"}},
    {"LatinUnlimitedWidthIsOneLine", L"hello world", 0.0f, {L"hello world"}},
    {"EmptyTextHasNoLines", L"", 5.0f, {}},

    // CJK: a break is allowed between any two ideographs
    {"CjkWrapsBetweenIdeographs",
     kZhongWen + kZhongWen + kZhongWen,
     4.0f,
     {kZhongWen, kZhongWen, kZhongWen}},
    {"CjkKeepsLatinWordIntact", kZhongWen + L"Kodi", 6.0f, {kZhongWen, L"Kodi"}},
    {"CjkAfterLatinWordStartsNewLine", L"Kodi" + kZhongWen, 5.0f, {L"Kodi", kZhongWen}},
    {"CjkAndLatinInterleaved",
     L"中"
     L"a"
     L"文"
     L"b",
     3.0f,
     {L"中a", L"文b"}},
    {"CjkWithAsciiSpaces", kZhongWen + L" abc", 4.0f, {kZhongWen, L"abc"}},
    {"CjkIdeographicSpaceIsASeparator",
     kZhongWen + kIdeographicSpace + kZhongWen,
     4.0f,
     {kZhongWen, kZhongWen}},
    {"CjkLeadingIdeographicSpaceIsStripped", kIdeographicSpace + kZhongWen, 4.0f, {kZhongWen}},
    {"CjkParagraphBreakIsKept", kZhongWen + L"\n" + kZhongWen, 20.0f, {kZhongWen, kZhongWen}},

    // Wrapped lines never end in a separator
    {"NoTrailingSpaceBeforeCjk", L"ab " + kZhongWen, 4.0f, {L"ab", kZhongWen}},
    {"NoTrailingIdeographicSpace",
     kZhongWen + kIdeographicSpace + L"abcd",
     6.0f,
     {kZhongWen, L"abcd"}},
};

class TestGUITextLayoutWrap : public testing::TestWithParam<WrapTestParam>
{
};

TEST_P(TestGUITextLayoutWrap, ProducesExpectedLines)
{
  const WrapTestParam& p = GetParam();
  EXPECT_EQ(p.expected, Wrap(p.text, p.maxWidth));
}

INSTANTIATE_TEST_SUITE_P(TestGUITextLayout,
                         TestGUITextLayoutWrap,
                         testing::ValuesIn(wrapCases),
                         [](const testing::TestParamInfo<WrapTestParam>& info)
                         { return info.param.name; });

// A glyph wider than the available width must still be emitted on its own line
// so that wrapping always makes progress.
TEST(TestGUITextLayoutWrap, GlyphWiderThanMaxWidthIsEmittedAlone)
{
  const Lines lines = Wrap(kZhongWen, 1.5f, /*maxHeight=*/10.0f);
  EXPECT_EQ(Lines({L"中", L"文"}), lines);
}

TEST(TestGUITextLayoutWrap, LatinGlyphWiderThanMaxWidthIsEmittedAlone)
{
  const Lines lines = Wrap(L"ab", 0.5f, /*maxHeight=*/10.0f);
  EXPECT_EQ(Lines({L"a", L"b"}), lines);
}

TEST(TestGUITextLayoutWrap, MaxHeightLimitsLineCount)
{
  const Lines lines = Wrap(kZhongWen + kZhongWen + kZhongWen, 4.0f, /*maxHeight=*/2.0f);
  EXPECT_EQ(Lines({kZhongWen, kZhongWen}), lines);
}

// Every non-separator character of the input must appear exactly once in the
// output, in order, and no multi-character line may exceed the width.
TEST(TestGUITextLayoutWrap, PreservesContent)
{
  const std::vector<std::wstring> texts{
      L"the quick brown fox jumps",
      kZhongWen + kDe + L"Kodi" + kZhongWen + L" 2024 " + kDe,
      L"a" + kZhongWen + L"bb" + kZhongWen + L"ccc",
      kZhongWen + kIdeographicSpace + kZhongWen + L" " + kDe,
      L"abc" + kZhongWen + L"defgh" + kZhongWen + kZhongWen,
  };
  for (const std::wstring& text : texts)
  {
    // Start at the widest glyph so every character fits on a line of its own.
    for (float maxWidth = 2.0f; maxWidth <= 12.0f; maxWidth += 0.5f)
    {
      const Lines lines = Wrap(text, maxWidth, /*maxHeight=*/100.0f);
      EXPECT_EQ(StripSpaces(text), StripSpaces(Join(lines))) << "width " << maxWidth;
      for (const std::wstring& line : lines)
      {
        EXPECT_FALSE(line.empty()) << "width " << maxWidth;
        if (line.size() > 1)
        {
          float width = 0.0f;
          for (wchar_t ch : line)
            width += CFixedWidthFont::GlyphWidth(ch);
          EXPECT_LE(width, maxWidth) << "line overflows at width " << maxWidth;
        }
      }
    }
  }
}

// Style and colour bits live above the code point; they must not affect break decisions.
TEST(TestGUITextLayoutWrap, StyleBitsDoNotAffectBreaks)
{
  CFixedWidthFont font;
  CWrapLayout layout(&font, 0.0f);

  vecText styled;
  for (wchar_t ch : kZhongWen + L"Kodi")
    styled.push_back((static_cast<character_t>(FONT_STYLE_BOLD) << 24) | (1u << 16) |
                     static_cast<character_t>(ch));

  layout.UpdateStyled(styled, {0xffffffff, 0xffffffff}, 6.0f, true);
  EXPECT_EQ(Lines({kZhongWen, L"Kodi"}), layout.GetLines());
}

} // namespace
