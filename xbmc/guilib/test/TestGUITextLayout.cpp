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
    {"price [3.99] per item", "price [3.99] per item"},

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

    // Skin-wrapped addon values: [B]...[B]value[/B]...[/B]
    {"[B][B]Episode Title[/B][/B]", "Episode Title"},
    {"[I][I]Some Value[/I][/I]", "Some Value"},
    {"[LIGHT][LIGHT]Some Value[/LIGHT][/LIGHT]", "Some Value"},
    {"[UPPERCASE][UPPERCASE]Some Title[/UPPERCASE][/UPPERCASE]", "SOME TITLE"},

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

INSTANTIATE_TEST_SUITE_P(ParseTextTags,
                         TestGUITextLayoutFilterTags,
                         testing::ValuesIn(filterTestCases));

TEST(TestGUITextLayoutFilter, SingleOpenBracket)
{
  EXPECT_NO_FATAL_FAILURE(Filter("["));
}

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
