/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "language/LangInfo.h"
#include "language/Language.h"
#include "language/LanguageLoader.h"
#include "language/i18n/LanguageTable.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"

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

  using CLangInfo::PlatformLocaleName;
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

TEST(TestLangInfo, PlatformLocaleNamePairsLanguageAndPlace)
{
  using KODI::LANGUAGE::CTerritory;

#ifdef TARGET_WINDOWS
  EXPECT_EQ(
      CLangInfoTest::PlatformLocaleName(CLanguageTag::Parse("en-GB"), CTerritory::FromCode("AU")),
      "en-AU");
  EXPECT_EQ(
      CLangInfoTest::PlatformLocaleName(CLanguageTag::Parse("fil"), CTerritory::FromCode("PH")),
      "fil-PH");
#else
  EXPECT_EQ(
      CLangInfoTest::PlatformLocaleName(CLanguageTag::Parse("en-GB"), CTerritory::FromCode("AU")),
      "en_AU.UTF-8");
  EXPECT_EQ(
      CLangInfoTest::PlatformLocaleName(CLanguageTag::Parse("fil"), CTerritory::FromCode("PH")),
      "fil_PH.UTF-8");
#endif

  // Without a place, the platform's own locale
  EXPECT_TRUE(CLangInfoTest::PlatformLocaleName(CLanguageTag::Parse("de"), CTerritory{}).empty());
}

TEST(TestLangInfo, LoadingALanguageNamesTheLanguagesOfTheInstalledAddons)
{
  ASSERT_TRUE(
      KODI::LANGUAGE::CLanguageLoader::GetInstance().Load("resource.language.en_gb", false));

  // The name the installed addon gives, rather than one composed from the subtags
  EXPECT_EQ(CLanguageTag::Parse("en-GB").ToEnglishName(), "English");

  // All three are process-wide
  CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Clear();
  KODI::LANGUAGE::I18N::CLanguageTable::GetInstance().Reset();
  KODI::LANGUAGE::CLanguage::GetInstance().SetPack(nullptr);
}

TEST(TestLangInfo, ALanguageAddonInstalledOrEnabledLaterNamesItsLanguage)
{
  // The language settings offer every installed language addon by name, so one installed,
  // updated or enabled after the language loaded must name its language too
  auto& loader{KODI::LANGUAGE::CLanguageLoader::GetInstance()};
  auto& table{KODI::LANGUAGE::I18N::CLanguageTable::GetInstance()};
  const std::string addon{"resource.language.en_gb"};

  for (const bool enabled : {true, false})
  {
    ASSERT_FALSE(table.NameOf("en-GB").has_value());
    if (enabled)
      loader.OnAddonEvent(ADDON::AddonEvents::Enabled(addon));
    else
      loader.OnAddonEvent(ADDON::AddonEvents::ReInstalled(addon));

    EXPECT_EQ(CLanguageTag::Parse("en-GB").ToEnglishName(), "English");
    table.Reset();
  }

  // Nothing is declared for another kind of event
  loader.OnAddonEvent(ADDON::AddonEvents::Disabled(addon));
  EXPECT_FALSE(table.NameOf("en-GB").has_value());
}

TEST(TestLangInfo, RegionalDateFormatsAreTheRegions)
{
  CLangInfoTest langInfo;

  langInfo.SetShortDateFormat("regional");

  langInfo.SetLongDateFormat("regional");

  EXPECT_EQ(langInfo.GetShortDateFormat(), "DD/MM/YYYY");
  EXPECT_EQ(langInfo.GetLongDateFormat(), "DDDD, D MMMM YYYY");
}
