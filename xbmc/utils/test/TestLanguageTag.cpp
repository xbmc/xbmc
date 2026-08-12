/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LanguageTag.h"

#include <gtest/gtest.h>

using namespace KODI::UTILS;

TEST(TestLanguageTag, DefaultConstructedIsEmpty)
{
  const CLanguageTag tag;

  EXPECT_TRUE(tag.IsEmpty());
  EXPECT_EQ(tag.AsBcp47(), "");
  EXPECT_EQ(tag.AsIso6392B(), "");
}

TEST(TestLanguageTag, ParsesEveryNotationToTheSameTag)
{
  const CLanguageTag expected = CLanguageTag::Parse("en");
  ASSERT_EQ(expected.AsBcp47(), "en"); // the other cases compare against this, so it is pinned

  EXPECT_EQ(CLanguageTag::Parse("eng"), expected); // ISO 639-2/B and /T
  EXPECT_EQ(CLanguageTag::Parse("EN"), expected); // case insensitive
  EXPECT_EQ(CLanguageTag::Parse(" en "), expected); // surrounding whitespace
  EXPECT_EQ(CLanguageTag::Parse("English"), expected); // English language name
}

TEST(TestLanguageTag, KeepsSubtagsThatIso639CannotExpress)
{
  const CLanguageTag tag = CLanguageTag::Parse("en-AU");

  EXPECT_EQ(tag.AsBcp47(), "en-AU");
  EXPECT_EQ(tag.AsIso6392B(), "eng"); // narrowing drops the region

  EXPECT_EQ(CLanguageTag::Parse("zh-Hant-HK").AsBcp47(), "zh-Hant-HK");
  EXPECT_EQ(CLanguageTag::Parse("zh-Hant-HK").AsIso6392B(), "chi");
}

TEST(TestLanguageTag, NarrowingIsLosslessForALanguageWithoutSubtags)
{
  // The notation a tag was built from must not matter to what it converts back to
  EXPECT_EQ(CLanguageTag::Parse("en").AsIso6392B(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("eng").AsIso6392B(), "eng");

  // B and T forms differ, the B form is expected
  EXPECT_EQ(CLanguageTag::Parse("zho").AsIso6392B(), "chi");
  EXPECT_EQ(CLanguageTag::Parse("chi").AsIso6392B(), "chi");
}

TEST(TestLanguageTag, NamesTheTerminologyForm)
{
  // The twenty-odd languages whose bibliographic and terminology codes differ
  EXPECT_EQ(CLanguageTag::Parse("de").AsIso6392T(), "deu");
  EXPECT_EQ(CLanguageTag::Parse("ger").AsIso6392T(), "deu");
  EXPECT_EQ(CLanguageTag::Parse("deu").AsIso6392T(), "deu");
  EXPECT_EQ(CLanguageTag::Parse("chi").AsIso6392T(), "zho");

  EXPECT_EQ(CLanguageTag::Parse("sq").AsIso6392T(), "sqi");
  EXPECT_EQ(CLanguageTag::Parse("cy").AsIso6392T(), "cym");
  EXPECT_EQ(CLanguageTag::Parse("sk").AsIso6392T(), "slk");

  // Every other language spells both forms the same way
  EXPECT_EQ(CLanguageTag::Parse("en").AsIso6392T(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("eng").AsIso6392T(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("es").AsIso6392T(), "spa");
  EXPECT_EQ(CLanguageTag::Parse("pl").AsIso6392T(), "pol");
  EXPECT_EQ(CLanguageTag::Parse("pt").AsIso6392T(), "por");
  EXPECT_EQ(CLanguageTag::Parse("sv").AsIso6392T(), "swe");
  EXPECT_EQ(CLanguageTag::Parse("tr").AsIso6392T(), "tur");
  EXPECT_EQ(CLanguageTag::Parse("ady").AsIso6392T(), "ady");

  // Narrowing drops the subtags, as it does for the B form
  EXPECT_EQ(CLanguageTag::Parse("en-AU").AsIso6392T(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("zh-Hant-HK").AsIso6392T(), "zho");

  EXPECT_EQ(CLanguageTag().AsIso6392T(), "");
  EXPECT_EQ(CLanguageTag::Parse("not a language").AsIso6392T(), "not a language");
}

TEST(TestLanguageTag, NamesTheAlpha2Form)
{
  EXPECT_EQ(CLanguageTag::Parse("en").AsIso6391(), "en");
  EXPECT_EQ(CLanguageTag::Parse("eng").AsIso6391(), "en");
  EXPECT_EQ(CLanguageTag::Parse("ger").AsIso6391(), "de"); // B and T forms differ
  EXPECT_EQ(CLanguageTag::Parse("deu").AsIso6391(), "de");
  EXPECT_EQ(CLanguageTag::Parse("en-AU").AsIso6391(), "en"); // narrowing drops the subtags

  // Unlike the other two projections, which answer with the tag itself, this one answers with
  // nothing where there is no code to give
  EXPECT_EQ(CLanguageTag::Parse("ady").AsIso6391(), "");
  EXPECT_EQ(CLanguageTag().AsIso6391(), "");
  EXPECT_EQ(CLanguageTag::Parse("not a language").AsIso6391(), "");
}

TEST(TestLanguageTag, KeepsLanguagesWithNoAlpha2Code)
{
  // BCP 47 uses the alpha-3 code where no alpha-2 code is registered
  EXPECT_EQ(CLanguageTag::Parse("ady").AsBcp47(), "ady");
  EXPECT_EQ(CLanguageTag::Parse("ady").AsIso6392B(), "ady");

  // zyg is ISO 639-3 only, so it has no ISO 639-2 code to narrow to
  EXPECT_EQ(CLanguageTag::Parse("zyg").AsBcp47(), "zyg");
  EXPECT_EQ(CLanguageTag::Parse("zyg").AsIso6392B(), "zyg");
}

TEST(TestLanguageTag, KeepsUnrecognizedTextVerbatim)
{
  // A value from a NFO, an addon or as.xml must survive rather than be discarded
  const CLanguageTag tag = CLanguageTag::Parse("not a language");

  EXPECT_FALSE(tag.IsEmpty());
  EXPECT_EQ(tag.AsBcp47(), "not a language");
  EXPECT_EQ(tag.AsIso6392B(), "not a language");
}

TEST(TestLanguageTag, ComparesTagsRatherThanLanguages)
{
  // Same language, different tags. Whether two tags name the same language is what Matches asks.
  EXPECT_FALSE(CLanguageTag::Parse("en-AU") == CLanguageTag::Parse("en"));
  EXPECT_FALSE(CLanguageTag::Parse("en-AU") == CLanguageTag::Parse("en-GB"));

  EXPECT_TRUE(CLanguageTag::Parse("en-AU") == CLanguageTag::Parse("en-au"));
}

TEST(TestLanguageTag, UndeterminedIsATagLikeAnyOther)
{
  const CLanguageTag tag = CLanguageTag::Parse("und");

  EXPECT_FALSE(tag.IsEmpty());
  EXPECT_EQ(tag.AsBcp47(), "und");
  EXPECT_EQ(tag.AsIso6392B(), "und");
}

TEST(TestLanguageTag, TreatsAbsentAndUndeterminedAlike)
{
  EXPECT_TRUE(CLanguageTag().IsUndetermined());
  EXPECT_TRUE(CLanguageTag::Undetermined().IsUndetermined());
  EXPECT_TRUE(CLanguageTag::Parse("und").IsUndetermined());
  EXPECT_TRUE(CLanguageTag::Parse("Undetermined").IsUndetermined());

  // A tag built by the named constructor is the same tag media declares as "und"
  EXPECT_EQ(CLanguageTag::Undetermined(), CLanguageTag::Parse("und"));

  // "no linguistic content" is a positive statement, not an absence of one
  EXPECT_FALSE(CLanguageTag::Parse("zxx").IsUndetermined());

  EXPECT_FALSE(CLanguageTag::Parse("en").IsUndetermined());
  EXPECT_FALSE(CLanguageTag::Parse("not a language").IsUndetermined());
}

TEST(TestLanguageTag, MatchesTheSameLanguageInAnyNotation)
{
  EXPECT_TRUE(CLanguageTag::Parse("en").Matches(CLanguageTag::Parse("eng")));
  EXPECT_TRUE(CLanguageTag::Parse("zh").Matches(CLanguageTag::Parse("chi")));
  EXPECT_TRUE(CLanguageTag::Parse("chi").Matches(CLanguageTag::Parse("zho")));
  EXPECT_TRUE(CLanguageTag::Parse("English").Matches(CLanguageTag::Parse("en")));

  EXPECT_FALSE(CLanguageTag::Parse("en").Matches(CLanguageTag::Parse("fr")));

  // A subtag qualifies a language without changing which one it is, so a user asking for English
  // is given en-AU rather than nothing
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("en")));
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("eng")));
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("en-GB")));
  EXPECT_TRUE(CLanguageTag::Parse("pt-BR").Matches(CLanguageTag::Parse("por")));
  EXPECT_TRUE(CLanguageTag::Parse("zh-Hant-HK").Matches(CLanguageTag::Parse("zh")));

  EXPECT_FALSE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("fr-CA")));

  // An extlang qualifies the language its prefix names, in the same way a region does, so zh-yue
  // names Chinese here. The canonical form RFC 5646 gives it, yue, names a language of its own
  EXPECT_TRUE(CLanguageTag::Parse("zh-yue").Matches(CLanguageTag::Parse("zh")));
  EXPECT_FALSE(CLanguageTag::Parse("zh-yue").Matches(CLanguageTag::Parse("yue")));

  // The tags remain distinguishable, matching is a question about the language they name
  EXPECT_NE(CLanguageTag::Parse("en-AU"), CLanguageTag::Parse("en-GB"));

  // A language with no ISO 639-2 code to narrow to still matches itself, and only itself
  EXPECT_TRUE(CLanguageTag::Parse("zyg").Matches(CLanguageTag::Parse("zyg")));
  EXPECT_FALSE(CLanguageTag::Parse("zyg").Matches(CLanguageTag::Parse("en")));

  // Unrecognized text is compared as it stands, so a value from a NFO or an addon still matches
  EXPECT_TRUE(CLanguageTag::Parse("not a language").Matches(CLanguageTag::Parse("not a language")));
  EXPECT_FALSE(CLanguageTag::Parse("not a language").Matches(CLanguageTag::Parse("en")));

  // "no linguistic content" is a language of its own, and undetermined is not English
  EXPECT_FALSE(CLanguageTag::Parse("zxx").Matches(CLanguageTag::Parse("en")));
  EXPECT_FALSE(CLanguageTag::Undetermined().Matches(CLanguageTag::Parse("en")));
}

TEST(TestLanguageTag, NamesTheLanguageInEnglish)
{
  EXPECT_EQ(CLanguageTag::Parse("en").GetEnglishName(), "English");
  EXPECT_EQ(CLanguageTag::Parse("eng").GetEnglishName(), "English");
  EXPECT_EQ(CLanguageTag::Parse("en-AU").GetEnglishName(), "English (Australia)");

  // Unrecognized languages have no name, and the caller decides what to show instead
  EXPECT_EQ(CLanguageTag().GetEnglishName(), "");
  EXPECT_EQ(CLanguageTag::Parse("not a language").GetEnglishName(), "");
}

struct TestFindInText
{
  std::string input;
  std::string expected;
};

// clang-format off
const TestFindInText FindInTextTests[] = {
    {"track name", ""},
    {"track name {en}", "en"},
    {"track name {en-US}", "en-US"},
    {"track name {es-419}", "es-419"},
    {"track name {en} more text", "en"},
    {"{en} track name", "en"},
    {"track name {", ""},
    {"track name {}", ""},
    {"}{en}{fr}", "en"},
    {"track name {EN}", "en"},
    {"track name { en }", "en"},
};
// clang-format on

class FindInTextTester : public testing::Test, public testing::WithParamInterface<TestFindInText>
{
};

TEST_P(FindInTextTester, Find)
{
  const auto tag = CLanguageTag::FindInText(GetParam().input);
  EXPECT_EQ(GetParam().expected, tag.has_value() ? tag->AsBcp47() : "");
}

INSTANTIATE_TEST_SUITE_P(TestLanguageTag, FindInTextTester, testing::ValuesIn(FindInTextTests));

TEST(TestLanguageTag, RecognizesOnlyRealLanguages)
{
  EXPECT_TRUE(CLanguageTag::TryParse("en").has_value());
  EXPECT_TRUE(CLanguageTag::TryParse("eng").has_value());
  EXPECT_TRUE(CLanguageTag::TryParse("en-AU").has_value());
  EXPECT_TRUE(CLanguageTag::TryParse("English").has_value());

  // A filename token that is not a language belongs to the stream name instead
  EXPECT_FALSE(CLanguageTag::TryParse("director").has_value());
  EXPECT_FALSE(CLanguageTag::TryParse("").has_value());
}

TEST(TestLanguageTag, PrefersTheCurrentCodeOverTheDeprecatedOne)
{
  // ISO 639-1 withdrew these five. Both spellings are still in the tables so that media tagged
  // with the old one is understood, but a tag Kodi hands out names the current code.
  EXPECT_EQ(CLanguageTag::Parse("heb").AsBcp47(), "he");
  EXPECT_EQ(CLanguageTag::Parse("ind").AsBcp47(), "id");
  EXPECT_EQ(CLanguageTag::Parse("yid").AsBcp47(), "yi");
  EXPECT_EQ(CLanguageTag::Parse("jav").AsBcp47(), "jv");
  EXPECT_EQ(CLanguageTag::Parse("rum").AsBcp47(), "ro");

  // Reading the deprecated spelling still yields the language, so nothing stops being understood
  EXPECT_EQ(CLanguageTag::Parse("iw").AsIso6392B(), "heb");
  EXPECT_EQ(CLanguageTag::Parse("in").AsIso6392B(), "ind");
  EXPECT_EQ(CLanguageTag::Parse("ji").AsIso6392B(), "yid");
  EXPECT_EQ(CLanguageTag::Parse("jw").AsIso6392B(), "jav");
  EXPECT_EQ(CLanguageTag::Parse("mo").AsIso6392B(), "rum");

  // A track tagged with the deprecated code satisfies a preference stated with the current one
  EXPECT_TRUE(CLanguageTag::Parse("iw").Matches(CLanguageTag::Parse("he")));
  EXPECT_TRUE(CLanguageTag::Parse("in").Matches(CLanguageTag::Parse("id")));
  EXPECT_TRUE(CLanguageTag::Parse("ji").Matches(CLanguageTag::Parse("yi")));
  EXPECT_TRUE(CLanguageTag::Parse("jw").Matches(CLanguageTag::Parse("jv")));
  EXPECT_TRUE(CLanguageTag::Parse("mo").Matches(CLanguageTag::Parse("ro")));
}

TEST(TestLanguageTag, NarrowingDropsTheRegionFromTheEnglishName)
{
  // A subtitle service addon is given a language by name, and knows the bare language only
  const CLanguageTag regional{CLanguageTag::Parse("en-AU")};
  EXPECT_EQ(regional.GetEnglishName(), "English (Australia)");
  EXPECT_EQ(regional.GetEnglishLanguageName(), "English");

  // A tag with nothing to drop names the same language either way
  EXPECT_EQ(CLanguageTag::Parse("eng").GetEnglishLanguageName(), "English");
}
