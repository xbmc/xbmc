/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangInfo.h"

#include <algorithm>

#include <gtest/gtest.h>

namespace
{

class CLangInfoTest : public CLangInfo
{
public:
  bool LoadLang(const std::string& language) { return Load(language); }
};

using KODI::UTILS::CLanguageTag;

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

  // Compares easily accessible only
#ifdef TARGET_WINDOWS
  EXPECT_EQ(langInfo.GetRegionLocale(), "usa");
#else
  EXPECT_EQ(langInfo.GetRegionLocale(), "US");
#endif

  // The stored form varies by platform, the region code does not
  EXPECT_EQ(langInfo.GetRegionCodeAlpha2(), "us");
  EXPECT_EQ(langInfo.GetRegionCodeAlpha3(), "usa");

  EXPECT_EQ(langInfo.GetSpeedUnit(), CSpeed::UnitMilesPerHour);
  EXPECT_EQ(langInfo.GetTemperatureUnit(), CTemperature::UnitFahrenheit);
}

TEST(TestLangInfo, FallsBackWhenTheLanguageSettingNamesNoLanguage)
{
  CLangInfoTest langInfo;
  ASSERT_TRUE(langInfo.LoadLang("en_gb"));

  langInfo.SetAudioLanguage("french");
  EXPECT_TRUE(langInfo.GetAudioLanguage(false).Matches(CLanguageTag::Parse("fr")));

  // A value that names nothing - hand edited, or set over JSON-RPC - must not stick, or the
  // caller is left preferring a language no stream can ever be
  langInfo.SetAudioLanguage("not a language");
  EXPECT_TRUE(langInfo.GetAudioLanguage(false).IsEmpty());
  EXPECT_FALSE(langInfo.GetAudioLanguage(true).IsEmpty());

  langInfo.SetSubtitleLanguage("not a language");
  EXPECT_TRUE(langInfo.GetSubtitleLanguage(false).IsEmpty());
  EXPECT_FALSE(langInfo.GetSubtitleLanguage(true).IsEmpty());
}
