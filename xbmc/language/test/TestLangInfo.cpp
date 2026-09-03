/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/LangInfo.h"
#include "language/Language.h"
#include "language/LanguageLoader.h"

#include <algorithm>

#include <gtest/gtest.h>

namespace
{

class CLangInfoTest : public KODI::LANGUAGE::CLangInfo
{
public:
  //! Read the region profiles a pack ships, as the loader does
  bool LoadLang(const std::string& language)
  {
    return Load(KODI::LANGUAGE::CLanguageLoader::GetLanguageInfoPath(language));
  }
};

using KODI::LANGUAGE::CLanguageTag;

} // namespace

TEST(TestLangInfo, Load)
{
  CLangInfoTest langInfo;

  ASSERT_TRUE(langInfo.LoadLang("en_gb"));
  std::vector<std::string> regions;
  langInfo.GetRegionNames(regions);
  std::ranges::sort(regions);

  using namespace std::string_literals;
  const auto ref = std::set{
      "USA (12h)"s,       "USA (24h)"s,       "UK (12h)"s,       "UK (24h)"s,    "Canada"s,
      "Australia (12h)"s, "Australia (24h)"s, "Central Europe"s, "India (12h)"s, "India (24h)"s,
  };

  EXPECT_TRUE(std::ranges::includes(regions, ref));

  langInfo.SetCurrentRegion("USA (12h)");

  // The region states one place, and the notation is the caller's to ask for. The stored form
  // used to differ by platform, Windows being handed the alpha-3, which made System.Locale(iso)
  // disagree with its own documentation there.
  const KODI::LANGUAGE::CTerritory& territory{langInfo.GetRegionTerritory()};

  EXPECT_EQ(territory.ToString(), "US");
  EXPECT_EQ(territory.AsIso3166_1Alpha2(), "US");
  EXPECT_EQ(territory.AsIso3166_1Alpha3(), "USA");
  EXPECT_TRUE(territory.IsCountry());

  EXPECT_EQ(langInfo.GetSpeedUnit(), CSpeed::UnitMilesPerHour);
  EXPECT_EQ(langInfo.GetTemperatureUnit(), CTemperature::UnitFahrenheit);
}

TEST(TestLangInfo, FallsBackWhenTheLanguageSettingNamesNoLanguage)
{
  CLangInfoTest langInfo;
  ASSERT_TRUE(langInfo.LoadLang("en_gb"));

  KODI::LANGUAGE::CLanguage& language = KODI::LANGUAGE::CLanguage::GetInstance();

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
