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

  EXPECT_TRUE(CLanguagePreference::ForAudio("mediadefault").GetLanguage().IsUndetermined());
  EXPECT_TRUE(CLanguagePreference::ForSubtitles("none").GetLanguage().IsUndetermined());
  EXPECT_NE(CLanguagePreference::ForAudio("mediadefault"),
            CLanguagePreference::ForAudio("original"));
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

TEST(TestLanguage, HoldsTheThreeSystemValues)
{
  CLanguage language;
  language.SetUI(CLanguageTag::Parse("en-GB"));
  language.SetAudio("fr");
  language.SetSubtitle("es");

  EXPECT_EQ(language.UI(), CLanguageTag::Parse("en-GB"));
  EXPECT_EQ(language.Audio(), CLanguageTag::Parse("fr"));
  EXPECT_EQ(language.Subtitle(), CLanguageTag::Parse("es"));
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

  // A stated language is the answer whether or not there is a fallback
  language.SetAudio("fr");
  language.SetSubtitle("es");
  EXPECT_EQ(language.Audio(false), CLanguageTag::Parse("fr"));
  EXPECT_EQ(language.Subtitle(false), CLanguageTag::Parse("es"));
}

TEST(TestLanguage, KeepsTheChoiceThatNamesNoLanguageReadable)
{
  CLanguage language;
  language.SetUI(CLanguageTag::Parse("en"));
  language.SetAudio("original");
  language.SetSubtitle("forced_only");

  EXPECT_TRUE(language.AudioPreference().Is(Kind::Original));
  EXPECT_TRUE(language.SubtitlePreference().Is(Kind::ForcedOnly));
  EXPECT_TRUE(language.Audio(false).IsUndetermined());
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
