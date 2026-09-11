/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/Language.h"

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE;

using Kind = CLanguagePreference::Kind;

TEST(TestLanguagePreference, ReadsAStatedLanguage)
{
  const CLanguagePreference audio{CLanguagePreference::ForAudio("fr")};

  EXPECT_TRUE(audio.Is(Kind::Language));
  EXPECT_EQ(audio.GetKind(), Kind::Language);
  EXPECT_EQ(audio.GetLanguage(), CLanguageTag::Parse("fr"));

  EXPECT_EQ(CLanguagePreference::ForAudio("fre").GetLanguage(), CLanguageTag::Parse("fr"));
  EXPECT_EQ(CLanguagePreference::ForAudio("French").GetLanguage(), CLanguageTag::Parse("fr"));
}

TEST(TestLanguagePreference, ReadsTheChoicesThatNameNoLanguage)
{
  EXPECT_TRUE(CLanguagePreference::ForAudio("mediadefault").Is(Kind::MediaDefault));
  EXPECT_TRUE(CLanguagePreference::ForAudio("original").Is(Kind::Original));
  EXPECT_TRUE(CLanguagePreference::ForAudio("default").Is(Kind::FollowUI));

  EXPECT_TRUE(CLanguagePreference::ForSubtitles("none").Is(Kind::None));
  EXPECT_TRUE(CLanguagePreference::ForSubtitles("forced_only").Is(Kind::ForcedOnly));
  EXPECT_TRUE(CLanguagePreference::ForSubtitles("original").Is(Kind::Original));
  EXPECT_TRUE(CLanguagePreference::ForSubtitles("default").Is(Kind::FollowUI));

  // The kind is what a caller acting on the choice reads, where Is answers one question about it
  EXPECT_EQ(CLanguagePreference::ForAudio("default").GetKind(), Kind::FollowUI);
  EXPECT_EQ(CLanguagePreference::ForAudio("mediadefault").GetKind(), Kind::MediaDefault);
  EXPECT_EQ(CLanguagePreference::ForSubtitles("none").GetKind(), Kind::None);
  EXPECT_EQ(CLanguagePreference::ForSubtitles("forced_only").GetKind(), Kind::ForcedOnly);

  // A default-constructed preference is the one that states nothing
  EXPECT_EQ(CLanguagePreference{}.GetKind(), Kind::FollowUI);

  EXPECT_TRUE(CLanguagePreference::ForAudio("mediadefault").GetLanguage().IsUndetermined());
  EXPECT_TRUE(CLanguagePreference::ForSubtitles("none").GetLanguage().IsUndetermined());

  // The choices are not interchangeable, so neither are the values holding them
  EXPECT_NE(CLanguagePreference::ForAudio("mediadefault"),
            CLanguagePreference::ForAudio("original"));
  EXPECT_NE(CLanguagePreference::ForSubtitles("none"), CLanguagePreference::ForSubtitles("fr"));
}

TEST(TestLanguagePreference, TreatsTextNamingNothingAsNoPreference)
{
  const CLanguagePreference audio{CLanguagePreference::ForAudio("not a language")};

  EXPECT_TRUE(audio.Is(Kind::FollowUI));
  EXPECT_TRUE(audio.GetLanguage().IsUndetermined());

  EXPECT_TRUE(CLanguagePreference::ForAudio("").Is(Kind::FollowUI));
}

TEST(TestLanguagePreference, ResolvesOnlyTheChoicesAnsweredByALanguage)
{
  const CLanguageTag ui{CLanguageTag::Parse("de")};

  EXPECT_EQ(CLanguagePreference::ForAudio("fr").Resolve(ui), CLanguageTag::Parse("fr"));
  EXPECT_EQ(CLanguagePreference::ForAudio("default").Resolve(ui), ui);

  EXPECT_TRUE(CLanguagePreference::ForAudio("mediadefault").Resolve(ui).IsUndetermined());
  EXPECT_TRUE(CLanguagePreference::ForAudio("original").Resolve(ui).IsUndetermined());
  EXPECT_TRUE(CLanguagePreference::ForSubtitles("none").Resolve(ui).IsUndetermined());
  EXPECT_TRUE(CLanguagePreference::ForSubtitles("forced_only").Resolve(ui).IsUndetermined());
}

TEST(TestLanguage, AnswersAStatedLanguageWhateverTheFallback)
{
  CLanguage language;
  language.SetUI(CLanguageTag::Parse("en-GB"));
  language.SetAudio("fr");
  language.SetSubtitle("es");

  EXPECT_EQ(language.UI(), CLanguageTag::Parse("en-GB"));
  EXPECT_EQ(language.Audio(), CLanguageTag::Parse("fr"));
  EXPECT_EQ(language.Subtitle(), CLanguageTag::Parse("es"));
  EXPECT_EQ(language.Audio(false), CLanguageTag::Parse("fr"));
  EXPECT_EQ(language.Subtitle(false), CLanguageTag::Parse("es"));
}

TEST(TestLanguage, AnswersAnUnstatedAudioLanguageWithTheInterfaceOne)
{
  CLanguage language;
  language.SetUI(CLanguageTag::Parse("de"));
  language.SetAudio("default");

  EXPECT_EQ(language.Audio(), CLanguageTag::Parse("de"));
}

TEST(TestLanguage, AnswersAnUnstatedSubtitleLanguageWithTheAudioOne)
{
  CLanguage language;
  language.SetUI(CLanguageTag::Parse("de"));
  language.SetAudio("fr");
  language.SetSubtitle("default");

  // Not the interface language; that asymmetry with Audio() is deliberate
  EXPECT_EQ(language.Subtitle(), CLanguageTag::Parse("fr"));

  language.SetAudio("default");
  EXPECT_TRUE(language.Subtitle(false).IsUndetermined());
}

TEST(TestLanguage, AnswersAChoiceThatNamesNoLanguageWithTheInterfaceOne)
{
  CLanguage language;
  language.SetUI(CLanguageTag::Parse("de"));

  // A caller with nothing better to match against still has a language to match
  for (const char* audio : {"original", "mediadefault"})
  {
    language.SetAudio(audio);
    EXPECT_EQ(language.Audio(), CLanguageTag::Parse("de")) << audio;
    EXPECT_TRUE(language.Audio(false).IsUndetermined()) << audio;
  }

  language.SetAudio("original");
  for (const char* subtitle : {"none", "forced_only", "original"})
  {
    language.SetSubtitle(subtitle);
    EXPECT_EQ(language.Subtitle(), CLanguageTag::Parse("de")) << subtitle;
    EXPECT_TRUE(language.Subtitle(false).IsUndetermined()) << subtitle;
  }

  // The choice itself stays readable for the caller that has to act on it
  language.SetSubtitle("forced_only");
  EXPECT_TRUE(language.AudioPreference().Is(Kind::Original));
  EXPECT_TRUE(language.SubtitlePreference().Is(Kind::ForcedOnly));
}

TEST(TestLanguage, RejectsASettingNamingNoLanguage)
{
  CLanguage language;

  language.SetAudio("french");
  EXPECT_TRUE(language.AudioPreference().GetLanguage().Matches(CLanguageTag::Parse("fr")));

  // The setting can arrive hand-edited or over JSON-RPC; a value naming no language must be
  // rejected rather than stored, or callers prefer a language no stream can ever match
  language.SetAudio("not a language");
  EXPECT_TRUE(language.AudioPreference().GetLanguage().IsUndetermined());

  // Rejected, it is treated as "default", which the interface language answers
  EXPECT_FALSE(language.Audio().IsUndetermined());
  EXPECT_TRUE(language.Audio().Matches(language.UI()));

  language.SetSubtitle("not a language");
  EXPECT_TRUE(language.SubtitlePreference().GetLanguage().IsUndetermined());

  // Subtitles follow the audio preference, and that preference names no language either, so
  // nothing here answers it - a caller that knows what is playing uses that instead
  EXPECT_TRUE(language.Subtitle(false).IsUndetermined());

  // Stated, the audio preference is what a subtitle without its own preference follows
  language.SetAudio("french");
  EXPECT_TRUE(language.Subtitle().Matches(CLanguageTag::Parse("fr")));
}

TEST(TestLanguage, WithoutAPackTheCharacterSetsAreTheBuiltInOnes)
{
  // Without a pack, and with the settings on their defaults, the character sets are the one
  // Kodi's own strings are in
  const CLanguage language;

  EXPECT_EQ(language.GuiCharset(), "CP1252");
  EXPECT_EQ(language.SubtitleCharset(), "CP1252");
}

TEST(TestLanguage, WithoutAPackTheSortTokensAreOnlyTheDeclaredOnes)
{
  // A pack ships the words a sort steps over, and advancedsettings.xml adds to them. With no
  // pack there is nothing to add to, so what comes back is what was declared and nothing else
  CLanguage language;
  EXPECT_EQ(language.SortTokens(), CLanguage::Tokens{});

  language.DeclareSortTokens({"the ", "the."});
  EXPECT_EQ(language.SortTokens(), (CLanguage::Tokens{"the ", "the."}));

  // A later declaration replaces the earlier one rather than adding to it
  language.DeclareSortTokens({"le "});
  EXPECT_EQ(language.SortTokens(), CLanguage::Tokens{"le "});

  // Losing the pack loses its words, not the declared ones
  language.SetPack(nullptr);
  EXPECT_EQ(language.SortTokens(), CLanguage::Tokens{"le "});
}

TEST(TestLanguage, WithoutAPackTheInterfaceIsInTheBuiltInLanguage)
{
  CLanguage language;
  language.SetUI(CLanguageTag::Parse("de"));
  language.SetPack(nullptr);

  EXPECT_EQ(language.UI(), CLanguageTag::English());
  EXPECT_EQ(language.Pack(), nullptr);
  EXPECT_TRUE(language.PackName().empty());
}
