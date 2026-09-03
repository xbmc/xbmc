/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/LanguageTag.h"

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE;

TEST(TestLanguageTag, DefaultConstructedNamesNoLanguage)
{
  const CLanguageTag tag;

  EXPECT_TRUE(tag.IsUndetermined());
  EXPECT_EQ(tag.ToString(), "");
  EXPECT_EQ(tag.AsIso6392B(), "");
}

TEST(TestLanguageTag, ParsesEveryNotationToTheSameTag)
{
  const CLanguageTag expected = CLanguageTag::Parse("en");
  ASSERT_EQ(expected.ToString(), "en"); // the other cases compare against this, so it is pinned

  EXPECT_EQ(CLanguageTag::Parse("eng"), expected); // ISO 639-2/B and /T
  EXPECT_EQ(CLanguageTag::Parse("EN"), expected); // case insensitive
  EXPECT_EQ(CLanguageTag::Parse(" en "), expected); // surrounding whitespace
  EXPECT_EQ(CLanguageTag::Parse("English"), expected); // English language name
}

TEST(TestLanguageTag, TakesThePosixSpellingOfTheSubtagSeparator)
{
  EXPECT_EQ(CLanguageTag::Parse("en_GB"), CLanguageTag::Parse("en-GB"));
  EXPECT_EQ(CLanguageTag::Parse("pt_BR"), CLanguageTag::Parse("pt-BR"));
  EXPECT_EQ(CLanguageTag::Parse("zh_Hant_HK"), CLanguageTag::Parse("zh-Hant-HK"));

  EXPECT_EQ(CLanguageTag::Parse("en_GB").ToString(), "en-GB");
  EXPECT_EQ(CLanguageTag::Parse("en_GB").AsIso6392B(), "eng");

  EXPECT_TRUE(CLanguageTag::TryParse("fr_CA").has_value());

  EXPECT_EQ(CLanguageTag::Parse("1080p_x264").ToString(), "1080p_x264");
}

TEST(TestLanguageTag, TakesThePosixCodesetAndModifier)
{
  // A language pack names itself by a POSIX locale, and the Serbian packs qualify theirs by script
  EXPECT_EQ(CLanguageTag::Parse("sr_RS@latin").ToString(), "sr-Latn-RS");
  EXPECT_EQ(CLanguageTag::Parse("sr_RS@cyrillic").ToString(), "sr-Cyrl-RS");
  EXPECT_EQ(CLanguageTag::Parse("sr_RS@latin").AsIso6391(), "sr");
  EXPECT_EQ(CLanguageTag::Parse("sr_RS@latin").GetTerritory().ToString(), "RS");
  EXPECT_TRUE(CLanguageTag::Parse("sr_RS@latin").Matches(CLanguageTag::Parse("sr")));

  // A codeset says nothing about the language, and neither does a modifier that names no script
  EXPECT_EQ(CLanguageTag::Parse("en_GB.UTF-8").ToString(), "en-GB");
  EXPECT_EQ(CLanguageTag::Parse("de_DE@euro").ToString(), "de-DE");
  EXPECT_EQ(CLanguageTag::Parse("sr_RS.UTF-8@latin").ToString(), "sr-Latn-RS");
}

TEST(TestLanguageTag, NamesTheTerritoryWhereTheSourceStatedOne)
{
  EXPECT_EQ(CLanguageTag::Parse("en-GB").GetTerritory().ToString(), "GB");
  EXPECT_EQ(CLanguageTag::Parse("en_gb").GetTerritory().ToString(), "GB");
  EXPECT_EQ(CLanguageTag::Parse("zh-Hant-HK").GetTerritory().ToString(), "HK");

  // BCP 47 names an area ISO 3166 does not cover with a UN M.49 code, which is what the
  // advancedsettings.xml <languagecodes> examples are written in. It is a territory, and the
  // type says it is not a country rather than leaving each caller to notice.
  const CTerritory latinAmerica{CLanguageTag::Parse("es-419").GetTerritory()};
  EXPECT_EQ(latinAmerica.ToString(), "419");
  EXPECT_FALSE(latinAmerica.IsCountry());

  // A variant qualifies the language rather than placing it, however it is spelled
  EXPECT_EQ(CLanguageTag::Parse("sl-rozaj").GetTerritory(), CTerritory{});
  EXPECT_EQ(CLanguageTag::Parse("de-1901").GetTerritory(), CTerritory{});
  EXPECT_EQ(CLanguageTag::Parse("de-DE-1901").GetTerritory().ToString(), "DE");

  // A tag naming no place answers with no territory, rather than one that is not a territory
  EXPECT_EQ(CLanguageTag::Parse("en").GetTerritory(), CTerritory{});
  EXPECT_EQ(CLanguageTag::Parse("eng").GetTerritory(), CTerritory{});
  EXPECT_EQ(CLanguageTag{}.GetTerritory(), CTerritory{});
  EXPECT_EQ(CLanguageTag::Parse("not a language").GetTerritory(), CTerritory{});
}

TEST(TestLanguageTag, KeepsSubtagsThatIso639CannotExpress)
{
  const CLanguageTag tag = CLanguageTag::Parse("en-AU");

  EXPECT_EQ(tag.ToString(), "en-AU");
  EXPECT_EQ(tag.AsIso6392B(), "eng"); // narrowing drops the region

  EXPECT_EQ(CLanguageTag::Parse("en-Latn-AU").ToString(), "en-Latn-AU");
  EXPECT_EQ(CLanguageTag::Parse("en-Latn-AU").AsIso6392B(), "eng");
}

TEST(TestLanguageTag, NarrowingIsLosslessForALanguageWithoutSubtags)
{
  // The notation a tag was built from must not matter to what it converts back to
  EXPECT_EQ(CLanguageTag::Parse("en").AsIso6392B(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("eng").AsIso6392B(), "eng");

  // Where the B and T forms differ, the B form is what this answers with, from either
  EXPECT_EQ(CLanguageTag::Parse("deu").AsIso6392B(), "ger");
  EXPECT_EQ(CLanguageTag::Parse("ger").AsIso6392B(), "ger");
}

TEST(TestLanguageTag, NamesTheTerminologyForm)
{
  // Where a language spells its two forms differently, whichever it arrived in answers with the
  // terminology one
  EXPECT_EQ(CLanguageTag::Parse("de").AsIso6392T(), "deu");
  EXPECT_EQ(CLanguageTag::Parse("ger").AsIso6392T(), "deu");
  EXPECT_EQ(CLanguageTag::Parse("deu").AsIso6392T(), "deu");

  // Where it does not, both forms are the same answer
  EXPECT_EQ(CLanguageTag::Parse("en").AsIso6392T(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("eng").AsIso6392T(), "eng");

  // Narrowing drops the subtags, as it does for the B form
  EXPECT_EQ(CLanguageTag::Parse("en-AU").AsIso6392T(), "eng");

  EXPECT_EQ(CLanguageTag().AsIso6392T(), "");
  EXPECT_EQ(CLanguageTag::Parse("not a language").AsIso6392T(), "not a language");
}

TEST(TestLanguageTag, NamesTheAlpha2Form)
{
  EXPECT_EQ(CLanguageTag::Parse("en").AsIso6391(), "en");
  EXPECT_EQ(CLanguageTag::Parse("eng").AsIso6391(), "en");
  EXPECT_EQ(CLanguageTag::Parse("ger").AsIso6391(), "de"); // either of a language's two forms
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
  // BCP 47 uses the alpha-3 code where no alpha-2 code is registered, and narrowing has nothing
  // shorter to reach for
  EXPECT_EQ(CLanguageTag::Parse("ady").ToString(), "ady");
  EXPECT_EQ(CLanguageTag::Parse("ady").AsIso6392B(), "ady");
  EXPECT_EQ(CLanguageTag::Parse("ady").AsIso6391(), "");
}

TEST(TestLanguageTag, SaysWhetherTheTextNamedALanguage)
{
  EXPECT_TRUE(CLanguageTag::Parse("en").IsValid());
  EXPECT_TRUE(CLanguageTag::Parse("eng").IsValid());
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").IsValid());
  EXPECT_TRUE(CLanguageTag::Parse("English").IsValid());
  EXPECT_TRUE(CLanguageTag::Undetermined().IsValid());

  EXPECT_FALSE(CLanguageTag{}.IsValid());
  EXPECT_FALSE(CLanguageTag::Parse("").IsValid());
  EXPECT_FALSE(CLanguageTag::Parse("not a language").IsValid());

  EXPECT_TRUE(CLanguageTag::Parse("ady").IsValid());
  EXPECT_EQ(CLanguageTag::Parse("ady").AsIso6391(), "");

  EXPECT_TRUE(CLanguageTag::TryParse("ady").has_value());
  EXPECT_FALSE(CLanguageTag::TryParse("not a language").has_value());
}

TEST(TestLanguageTag, KeepsUnrecognizedTextVerbatim)
{
  // A value from a NFO, an addon or as.xml must survive rather than be discarded
  const CLanguageTag tag = CLanguageTag::Parse("not a language");

  EXPECT_NE(tag, CLanguageTag{});
  EXPECT_EQ(tag.ToString(), "not a language");
  EXPECT_EQ(tag.AsIso6392B(), "not a language");
}

TEST(TestLanguageTag, TakesAnUnreadableStreamDeclarationAsUndetermined)
{
  // A stream's language reaches interfaces that promise BCP 47, so text naming no language must
  // not be passed on as though it were one
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("undefined").ToString(), "und");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("unknown").ToString(), "und");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("not a language").ToString(), "und");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("1080p_x264").ToString(), "und");

  // The two spellings a container can use for the same fact both arrive as the same tag
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("und").ToString(), "und");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("Undetermined").ToString(), "und");

  // A stream declaring no language at all is not a stream whose declaration could not be read
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage(""), CLanguageTag{});
  EXPECT_NE(CLanguageTag::ParseStreamLanguage(""), CLanguageTag::Undetermined());

  // zxx states that there is no speech to have a language, which is a declaration, not a failure
  // to read one
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("zxx").ToString(), "zxx");
  EXPECT_FALSE(CLanguageTag::ParseStreamLanguage("zxx").IsUndetermined());
}

TEST(TestLanguageTag, LeavesAReadableStreamDeclarationAlone)
{
  // Whatever a container spells its language as, a language it names survives unchanged
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("en").ToString(), "en");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("eng").ToString(), "en");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("en-GB").ToString(), "en-GB");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("en_GB").ToString(), "en-GB");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("English").ToString(), "en");
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("zh-Hant-HK").ToString(), "zh-Hant-HK");

  // A language with no ISO 639-1 code is still a language
  EXPECT_EQ(CLanguageTag::ParseStreamLanguage("ady").ToString(), "ady");

  // Unlike Parse, which keeps unrecognized text so that stored values survive a round trip
  EXPECT_EQ(CLanguageTag::Parse("undefined").ToString(), "undefined");
}

TEST(TestLanguageTag, ComparesTagsRatherThanLanguages)
{
  // Same language, different tags. Whether two tags name the same language is what Matches asks.
  EXPECT_FALSE(CLanguageTag::Parse("en-AU") == CLanguageTag::Parse("en"));
  EXPECT_FALSE(CLanguageTag::Parse("en-AU") == CLanguageTag::Parse("en-GB"));

  EXPECT_TRUE(CLanguageTag::Parse("en-AU") == CLanguageTag::Parse("en-au"));
}

TEST(TestLanguageTag, RecognizesEnglishInAnyVariety)
{
  EXPECT_TRUE(CLanguageTag::Parse("en").IsEnglish());
  EXPECT_TRUE(CLanguageTag::Parse("eng").IsEnglish()); // whichever notation it arrived in
  EXPECT_TRUE(CLanguageTag::Parse("EN").IsEnglish());
  EXPECT_TRUE(CLanguageTag::Parse("English").IsEnglish());
  EXPECT_TRUE(
      CLanguageTag::Parse("en-GB").IsEnglish()); // a subtag qualifies it, it is still English
  EXPECT_TRUE(CLanguageTag::Parse("en_US").IsEnglish());

  EXPECT_FALSE(CLanguageTag::Parse("fr").IsEnglish());
  EXPECT_FALSE(CLanguageTag{}.IsEnglish());
  EXPECT_FALSE(CLanguageTag::Parse("not a language").IsEnglish());

  // Middle English is a language of its own
  EXPECT_FALSE(CLanguageTag::Parse("enm").IsEnglish());
}

TEST(TestLanguageTag, SaysWhichLanguageItNames)
{
  // The language, not the text the tag begins with, so a language whose code starts with another
  // language's is not that language
  EXPECT_TRUE(CLanguageTag::Parse("en").IsLanguage("en"));
  EXPECT_TRUE(CLanguageTag::Parse("eng").IsLanguage("en"));
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").IsLanguage("en"));
  EXPECT_TRUE(CLanguageTag::Parse("English").IsLanguage("en"));
  EXPECT_TRUE(CLanguageTag::Parse("en").IsLanguage("EN"));

  // enm is Middle English, a language of its own
  EXPECT_FALSE(CLanguageTag::Parse("enm").IsLanguage("en"));

  // The subtag is the one canonical BCP 47 holds, which is the shortest a language has
  EXPECT_FALSE(CLanguageTag::Parse("en").IsLanguage("eng"));

  EXPECT_FALSE(CLanguageTag{}.IsLanguage("en"));
  EXPECT_FALSE(CLanguageTag::Parse("not a language").IsLanguage("en"));
  EXPECT_FALSE(CLanguageTag::Parse("en").IsLanguage(""));
}

TEST(TestLanguageTag, UndeterminedIsATagLikeAnyOther)
{
  const CLanguageTag tag = CLanguageTag::Parse("und");

  EXPECT_NE(tag, CLanguageTag{});
  EXPECT_EQ(tag.ToString(), "und");
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
  EXPECT_TRUE(CLanguageTag::Parse("English").Matches(CLanguageTag::Parse("en")));

  EXPECT_FALSE(CLanguageTag::Parse("en").Matches(CLanguageTag::Parse("fr")));

  // A subtag qualifies a language without changing which one it is, so a user asking for English
  // is given en-AU rather than nothing
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("en")));
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("eng")));
  EXPECT_TRUE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("en-GB")));
  EXPECT_TRUE(CLanguageTag::Parse("en-Latn-AU").Matches(CLanguageTag::Parse("en")));

  EXPECT_FALSE(CLanguageTag::Parse("en-AU").Matches(CLanguageTag::Parse("fr-CA")));

  // An extlang qualifies the language its prefix names, in the same way a region does, so zh-yue
  // names Chinese here. The canonical form RFC 5646 gives it, yue, names a language of its own
  EXPECT_TRUE(CLanguageTag::Parse("zh-yue").Matches(CLanguageTag::Parse("zh")));
  EXPECT_FALSE(CLanguageTag::Parse("zh-yue").Matches(CLanguageTag::Parse("yue")));

  // The tags remain distinguishable, matching is a question about the language they name
  EXPECT_NE(CLanguageTag::Parse("en-AU"), CLanguageTag::Parse("en-GB"));

  // Unrecognized text is compared as it stands, so a value from a NFO or an addon still matches
  EXPECT_TRUE(CLanguageTag::Parse("not a language").Matches(CLanguageTag::Parse("not a language")));
  EXPECT_FALSE(CLanguageTag::Parse("not a language").Matches(CLanguageTag::Parse("en")));

  // "no linguistic content" is a language of its own, and undetermined is not English
  EXPECT_FALSE(CLanguageTag::Parse("zxx").Matches(CLanguageTag::Parse("en")));
  EXPECT_FALSE(CLanguageTag::Undetermined().Matches(CLanguageTag::Parse("en")));
}

TEST(TestLanguageTag, NamesTheLanguageInEnglish)
{
  EXPECT_EQ(CLanguageTag::Parse("en").ToEnglishName(), "English");
  EXPECT_EQ(CLanguageTag::Parse("eng").ToEnglishName(), "English");
  EXPECT_EQ(CLanguageTag::Parse("en-AU").ToEnglishName(), "English (Australia)");

  // Unrecognized languages have no name, and the caller decides what to show instead
  EXPECT_EQ(CLanguageTag().ToEnglishName(), "");
  EXPECT_EQ(CLanguageTag::Parse("not a language").ToEnglishName(), "");
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
  EXPECT_EQ(GetParam().expected, tag.has_value() ? tag->ToString() : "");
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
  // Both spellings are held so that media tagged with the withdrawn one is understood, but a tag
  // Kodi hands out names the current code
  EXPECT_EQ(CLanguageTag::Parse("heb").ToString(), "he");
  EXPECT_EQ(CLanguageTag::Parse("iw").AsIso6392B(), "heb");

  // A track tagged with the withdrawn code satisfies a preference stated with the current one
  EXPECT_TRUE(CLanguageTag::Parse("iw").Matches(CLanguageTag::Parse("he")));
}

TEST(TestLanguageTag, NarrowingDropsTheRegionFromTheEnglishName)
{
  // A subtitle service addon is given a language by name, and knows the bare language only
  const CLanguageTag regional{CLanguageTag::Parse("en-AU")};
  EXPECT_EQ(regional.ToEnglishName(), "English (Australia)");
  EXPECT_EQ(regional.ToEnglishLanguageName(), "English");

  // A tag with nothing to drop names the same language either way
  EXPECT_EQ(CLanguageTag::Parse("eng").ToEnglishLanguageName(), "English");
}

TEST(TestLanguageTag, ShortensAComposedNameThatIsTooLongToShow)
{
  // Nothing names this tag, so the name is composed from its subtags and cut to fit a list. The
  // tag is appended so the row still says which language it is.
  EXPECT_EQ(CLanguageTag::Parse("zh-yue-Hant-HK").ToEnglishName(),
            "Chinese (Cantonese, Han (Tr... [zh-yue-Hant-HK]");
}

TEST(TestLanguageTag, ResolvesALanguageByAnyNameRecordedForIt)
{
  // A name narrows like any other notation, whatever case it is written in
  EXPECT_EQ(CLanguageTag::Parse("English").AsIso6392B(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("english").AsIso6392B(), "eng");

  // ISO 639-2 records more than one name for some languages, and any of them resolves
  EXPECT_EQ(CLanguageTag::Parse("Valencian"), CLanguageTag::Parse("ca"));

  // A name only the BCP 47 subtag registry records is searched after the tables miss
  EXPECT_EQ(CLanguageTag::Parse("Yang Zhuang"), CLanguageTag::Parse("zyg"));
}

TEST(TestLanguageTag, DoesNotReadARegionCodeAsALanguage)
{
  // ISO 3166-1 and ISO 639 share the three letter namespace without sharing meanings. bol is
  // Bolivia to one and Bole to the other, and only the latter is a language.
  EXPECT_EQ(CLanguageTag::Parse("bol").ToString(), "bol");
  EXPECT_EQ(CLanguageTag::Parse("bol").AsIso6392B(), "bol"); // ISO 639-2 codes no such language
  EXPECT_EQ(CLanguageTag::Parse("bol").AsIso6391(), ""); // and bo is Tibetan, not this

  EXPECT_FALSE(CLanguageTag::TryParse("zzz").has_value());
}

TEST(TestLanguageTag, DropsEverySubtagWhenNarrowing)
{
  EXPECT_EQ(CLanguageTag::Parse("en-Latn-AU-1996").AsIso6392B(), "eng");
  EXPECT_EQ(CLanguageTag::Parse("EN").AsIso6392B(), "eng");
  EXPECT_EQ(CLanguageTag::Parse(" en ").AsIso6392B(), "eng");
}
