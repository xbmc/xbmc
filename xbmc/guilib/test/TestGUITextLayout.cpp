/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUITextLayout.h"

#include <string>

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

// ---------------------------------------------------------------------------
// Plain text (no tags)
// ---------------------------------------------------------------------------

TEST(TestGUITextLayoutFilter, EmptyString)
{
  EXPECT_EQ("", Filter(""));
}

TEST(TestGUITextLayoutFilter, PlainTextPassesThrough)
{
  EXPECT_EQ("Hello world", Filter("Hello world"));
}

TEST(TestGUITextLayoutFilter, UnrecognisedBracketsPassThrough)
{
  EXPECT_EQ("price [3.99] per item", Filter("price [3.99] per item"));
}

// ---------------------------------------------------------------------------
// Matched single tag pairs
// ---------------------------------------------------------------------------

struct TagTestParam
{
  std::string input;
  std::string expected;
};

class TestGUITextLayoutMatchedTags : public testing::TestWithParam<TagTestParam>
{
};

TEST_P(TestGUITextLayoutMatchedTags, TagStrippedContentPreserved)
{
  EXPECT_EQ(GetParam().expected, Filter(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(StyleTags,
                         TestGUITextLayoutMatchedTags,
                         testing::Values(
                             // Basic style tags
                             TagTestParam{"[B]bold[/B]", "bold"},
                             TagTestParam{"[I]italic[/I]", "italic"},
                             TagTestParam{"[LIGHT]text[/LIGHT]", "text"},
                             // Case-transform tags also apply the transform
                             TagTestParam{"[UPPERCASE]hello[/UPPERCASE]", "HELLO"},
                             TagTestParam{"[LOWERCASE]HELLO[/LOWERCASE]", "hello"},
                             TagTestParam{"[CAPITALIZE]hello world[/CAPITALIZE]", "Hello World"},
                             // Tag with surrounding text
                             TagTestParam{"prefix [B]bold[/B] suffix", "prefix bold suffix"},
                             // CR tag becomes newline
                             TagTestParam{"a[CR]b", "a\nb"},
                             // TABS tag inserts the specified number of tab characters
                             TagTestParam{"[TABS]1[/TABS]label", "\tlabel"},
                             TagTestParam{"[TABS]2[/TABS]label", "\t\tlabel"},
                             TagTestParam{"[TABS]3[/TABS]label", "\t\t\tlabel"},
                             // Unmatched [TABS] without [/TABS]: the tag token is consumed but no
                             // tabs are emitted; content after the tag passes through unchanged.
                             TagTestParam{"[TABS]1 label", "1 label"},
                             // Stray [/TABS] with no matching open is consumed, not passed through
                             TagTestParam{"label[/TABS]", "label"}));

// ---------------------------------------------------------------------------
// Nested matched tag pairs (PR #28280)
// Depth counters must increment on open and decrement on close, keeping
// style active until the last matching close tag.
// ---------------------------------------------------------------------------

class TestGUITextLayoutNestedTags : public testing::TestWithParam<TagTestParam>
{
};

TEST_P(TestGUITextLayoutNestedTags, ContentPreserved)
{
  EXPECT_EQ(GetParam().expected, Filter(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(
    NestedStyleTags,
    TestGUITextLayoutNestedTags,
    testing::Values(TagTestParam{"[B][B]text[/B][/B]", "text"},
                    TagTestParam{"[B][B][B]text[/B][/B][/B]", "text"},
                    TagTestParam{"[I][I]text[/I][/I]", "text"},
                    TagTestParam{"[LIGHT][LIGHT]text[/LIGHT][/LIGHT]", "text"},
                    TagTestParam{"[UPPERCASE][UPPERCASE]hello[/UPPERCASE][/UPPERCASE]", "HELLO"},
                    TagTestParam{"[LOWERCASE][LOWERCASE]HELLO[/LOWERCASE][/LOWERCASE]", "hello"},
                    TagTestParam{"[CAPITALIZE][CAPITALIZE]hello world[/CAPITALIZE][/CAPITALIZE]",
                                 "Hello World"}));

// ---------------------------------------------------------------------------
// The primary regression scenario (PR #28356)
// A skin wraps a label in e.g. [B]…[/B] while an addon has already
// formatted its value with [B]…[/B], producing [B][B]value[/B][/B].
// ---------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    SkinWrappedAddonValues,
    TestGUITextLayoutNestedTags,
    testing::Values(TagTestParam{"[B][B]Episode Title[/B][/B]", "Episode Title"},
                    TagTestParam{"[I][I]Some Value[/I][/I]", "Some Value"},
                    TagTestParam{"[LIGHT][LIGHT]Some Value[/LIGHT][/LIGHT]", "Some Value"},
                    TagTestParam{"[UPPERCASE][UPPERCASE]Some Title[/UPPERCASE][/UPPERCASE]",
                                 "SOME TITLE"}));

// ---------------------------------------------------------------------------
// Unmatched opening tags (PR #28356 lookahead)
// An opening tag with no matching close must not activate its depth counter.
// The tag is consumed; surrounding content is preserved.
// ---------------------------------------------------------------------------

class TestGUITextLayoutUnmatchedOpen : public testing::TestWithParam<TagTestParam>
{
};

TEST_P(TestGUITextLayoutUnmatchedOpen, TagConsumedContentPreserved)
{
  const std::string result = Filter(GetParam().input);
  EXPECT_EQ(result.find(GetParam().expected), std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(UnmatchedOpenTags,
                         TestGUITextLayoutUnmatchedOpen,
                         testing::Values(TagTestParam{"[B]no close", "[B]"},
                                         TagTestParam{"[I]no close", "[I]"},
                                         TagTestParam{"[LIGHT]no close", "[LIGHT]"},
                                         TagTestParam{"[UPPERCASE]no close", "[UPPERCASE]"},
                                         TagTestParam{"[LOWERCASE]no close", "[LOWERCASE]"},
                                         TagTestParam{"[CAPITALIZE]no close", "[CAPITALIZE]"},
                                         TagTestParam{"text[B]", "[B]"}));

// ---------------------------------------------------------------------------
// Unmatched closing tags
// A closing tag with no prior open (depth == 0) must be silently ignored.
// ---------------------------------------------------------------------------

class TestGUITextLayoutUnmatchedClose : public testing::TestWithParam<TagTestParam>
{
};

TEST_P(TestGUITextLayoutUnmatchedClose, TagConsumedContentPreserved)
{
  const std::string result = Filter(GetParam().input);
  EXPECT_EQ(result.find(GetParam().expected), std::string::npos);
}

INSTANTIATE_TEST_SUITE_P(UnmatchedCloseTags,
                         TestGUITextLayoutUnmatchedClose,
                         testing::Values(TagTestParam{"[/B]text", "[/B]"},
                                         TagTestParam{"text [/B] more", "[/B]"},
                                         TagTestParam{"text [/I] more", "[/I]"},
                                         TagTestParam{"text [/LIGHT] more", "[/LIGHT]"},
                                         TagTestParam{"text [/UPPERCASE] more", "[/UPPERCASE]"},
                                         TagTestParam{"text [/LOWERCASE] more", "[/LOWERCASE]"},
                                         TagTestParam{"text [/CAPITALIZE] more", "[/CAPITALIZE]"},
                                         // Stray close after a valid matched pair (depth back to 0)
                                         TagTestParam{"[B]text[/B][/B]", "[/B]"},
                                         TagTestParam{"[B][B]text[/B][/B][/B]", "[/B]"}));

// ---------------------------------------------------------------------------
// Mixed / edge cases
// ---------------------------------------------------------------------------

TEST(TestGUITextLayoutFilter, MultipleStrayClosesBeforeValidPair)
{
  EXPECT_EQ("valid", Filter("[/B][/B][/B][B]valid[/B]"));
}

TEST(TestGUITextLayoutFilter, StrayCloseBetweenTwoValidPairs)
{
  EXPECT_EQ("first  second", Filter("[B]first[/B] [/B] [B]second[/B]"));
}

TEST(TestGUITextLayoutFilter, TwoDisjointBoldPairs)
{
  EXPECT_EQ("first mid second", Filter("[B]first[/B] mid [B]second[/B]"));
}

TEST(TestGUITextLayoutFilter, BoldWrappingItalic)
{
  EXPECT_EQ("bold+italic", Filter("[B][I]bold+italic[/I][/B]"));
}

TEST(TestGUITextLayoutFilter, AllStylesNested)
{
  EXPECT_EQ("content", Filter("[B][I][LIGHT]content[/LIGHT][/I][/B]"));
}

TEST(TestGUITextLayoutFilter, InterleavedTagsNoUnderflow)
{
  // Malformed nesting must not crash or underflow depth counters
  const std::string result = Filter("[B][I]text[/B][/I]");
  EXPECT_NE(result.find("text"), std::string::npos);
}

TEST(TestGUITextLayoutFilter, DepthZeroAfterNestedPair)
{
  EXPECT_EQ("styled plain", Filter("[B][B]styled[/B][/B] plain"));
}

TEST(TestGUITextLayoutFilter, EmptyBoldPair)
{
  EXPECT_EQ("", Filter("[B][/B]"));
}

TEST(TestGUITextLayoutFilter, EmptyNestedBoldPairs)
{
  EXPECT_EQ("", Filter("[B][B][/B][/B]"));
}

TEST(TestGUITextLayoutFilter, SingleOpenBracket)
{
  EXPECT_NO_FATAL_FAILURE(Filter("["));
}

TEST(TestGUITextLayoutFilter, LowercaseTagNotRecognised)
{
  // Tag matching is case-sensitive; "[b]" passes through as literal text
  const std::string result = Filter("[b]text[/b]");
  EXPECT_NE(result.find("text"), std::string::npos);
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
  // 100 opens, 99 closes – outermost [B] has no matching close so is ignored.
  // Parser must not crash; content must be extracted.
  std::string open, close;
  for (int i = 0; i < 100; ++i)
    open += "[B]";
  for (int i = 0; i < 99; ++i)
    close += "[/B]";
  const std::string result = Filter(open + "deep" + close);
  EXPECT_NE(result.find("deep"), std::string::npos);
}
