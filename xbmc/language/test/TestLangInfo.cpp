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

// What the add-on and Python interfaces are handed when they ask what Kodi is running in
TEST(TestLangInfo, DescribeLanguage)
{
  using KODI::LANGUAGE::DescribeLanguage;

  CLangInfoTest langInfo;
  ASSERT_TRUE(langInfo.LoadLang("en_gb"));
  langInfo.SetCurrentRegion("USA (12h)");

  KODI::LANGUAGE::CLanguage language;
  language.SetUI(CLanguageTag::Parse("en-GB"));

  EXPECT_EQ(DescribeLanguage(CLanguageTag::ISO_639_1, language, langInfo, false), "en");
  EXPECT_EQ(DescribeLanguage(CLanguageTag::ISO_639_2, language, langInfo, false), "eng");
  EXPECT_EQ(DescribeLanguage(CLanguageTag::ISO_NAME, language, langInfo, false), "English");

  // The place is the region profile's, not the language's, so a British pack under the USA
  // profile is named for where the viewer is
  EXPECT_EQ(DescribeLanguage(CLanguageTag::ISO_639_1, language, langInfo, true), "en-US");
  EXPECT_EQ(DescribeLanguage(CLanguageTag::ISO_639_2, language, langInfo, true), "eng-USA");
  EXPECT_EQ(DescribeLanguage(CLanguageTag::ISO_NAME, language, langInfo, true), "English-USA");

  // A language with no code in the notation asked for is answered with nothing, rather than with
  // a bare place
  language.SetUI(CLanguageTag::Parse("ast"));
  EXPECT_TRUE(DescribeLanguage(CLanguageTag::ISO_639_1, language, langInfo, true).empty());
  EXPECT_EQ(DescribeLanguage(CLanguageTag::ISO_639_2, language, langInfo, true), "ast-USA");

  // Without a pack there is no name for a pack to have stated
  EXPECT_TRUE(DescribeLanguage(CLanguageTag::ENGLISH_NAME, language, langInfo, true).empty());
}

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

  // The region states one place, held in one form on every platform, and the notation is the
  // caller's to ask for
  const KODI::LANGUAGE::CTerritory& territory{langInfo.GetRegionTerritory()};

  EXPECT_EQ(territory.ToString(), "US");
  EXPECT_EQ(territory.AsIso3166_1Alpha2(), "US");
  EXPECT_EQ(territory.AsIso3166_1Alpha3(), "USA");
  EXPECT_TRUE(territory.IsCountry());

  EXPECT_EQ(langInfo.GetSpeedUnit(), CSpeed::UnitMilesPerHour);
  EXPECT_EQ(langInfo.GetTemperatureUnit(), CTemperature::UnitFahrenheit);
}
