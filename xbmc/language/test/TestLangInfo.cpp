/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "filesystem/SpecialProtocol.h"
#include "language/LangInfo.h"
#include "language/Language.h"
#include "language/LanguageLoader.h"
#include "language/i18n/LanguageTable.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>

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

  //! Read region profiles from this text
  bool LoadText(const std::string& text)
  {
    const std::string path{CSpecialProtocol::TranslatePath("special://temp/TestLangInfo.xml")};
    std::ofstream(path) << text;
    const bool loaded{Load(path)};
    std::remove(path.c_str());
    return loaded;
  }

  using CLangInfo::PlatformLocaleName;
  using CLangInfo::PrepareTimeFormat;
};

//! Puts the locale settings a test changes back as they were
class CLocaleSettingsRestorer
{
public:
  CLocaleSettingsRestorer()
  {
    for (const char* id : IDS)
      m_values.emplace_back(id, Settings().GetString(id));
  }
  ~CLocaleSettingsRestorer()
  {
    for (const auto& [id, value] : m_values)
      Settings().SetString(id, value);
  }

  static CSettings& Settings() { return *CServiceBroker::GetSettingsComponent()->GetSettings(); }

private:
  static constexpr const char* IDS[] = {
      CSettings::SETTING_LOCALE_COUNTRY, CSettings::SETTING_LOCALE_USE24HOURCLOCK,
      CSettings::SETTING_LOCALE_SHORTDATEFORMAT, CSettings::SETTING_LOCALE_LONGDATEFORMAT,
      CSettings::SETTING_LOCALE_TIMEFORMAT};
  std::vector<std::pair<std::string, std::string>> m_values;
};

using KODI::LANGUAGE::CLanguageTag;

} // namespace

// What the add-on and Python interfaces are handed when they ask what Kodi is running in
TEST(TestLangInfo, DescribeLanguage)
{
  using KODI::LANGUAGE::DescribeLanguage;

  CLangInfoTest langInfo;
  ASSERT_TRUE(langInfo.LoadLang("en_gb"));
  langInfo.SetCurrentRegion("US");

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
  std::map<std::string, std::string> regions;
  for (const auto& region : langInfo.GetRegions())
    regions.emplace(region.id, region.name);

  // A region per place, named as the first profile the pack lists for it
  const std::map<std::string, std::string> ref{
      {"AU", "Australia (12h)"}, {"CA", "Canada"},      {"DE", "Central Europe"},
      {"GB", "UK (24h)"},        {"IN", "India (12h)"}, {"US", "USA (12h)"},
  };
  EXPECT_EQ(ref, regions);

  langInfo.SetCurrentRegion("US");

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

TEST(TestLangInfo, APlaceListedTwiceIsOneRegionAndItsFirstProfile)
{
  const std::string pack{
      "<language locale=\"en\"><regions>"
      "<region name=\"Here (24h)\" locale=\"GB\"><time>H:mm:ss</time></region>"
      "<region name=\"Here (12h)\" locale=\"GB\"><time>h:mm:ss xx</time></region>"
      "<region locale=\"US\"><time>h:mm:ss xx</time></region>"
      "</regions></language>"};
  CLangInfoTest langInfo;
  ASSERT_TRUE(langInfo.LoadText(pack));

  const auto regions{langInfo.GetRegions()};
  ASSERT_EQ(2u, regions.size());
  EXPECT_EQ("GB", regions[0].id);
  EXPECT_EQ("Here (24h)", regions[0].name);
  // A profile with no name is shown as one, and never keyed by what that reads as
  EXPECT_EQ("US", regions[1].id);
  EXPECT_EQ(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(10005),
            regions[1].name);
}

TEST(TestLangInfo, AProfileNameStillFindsItsRegionAndKeepsItsClock)
{
  const CLocaleSettingsRestorer restorer;
  CSettings& settings{CLocaleSettingsRestorer::Settings()};
  settings.SetString(CSettings::SETTING_LOCALE_USE24HOURCLOCK, "regional");

  CLangInfoTest langInfo;
  ASSERT_TRUE(langInfo.LoadLang("en_gb"));
  langInfo.SetCurrentRegion("UK (12h)");

  EXPECT_EQ("GB", langInfo.GetRegionTerritory().ToString());
  EXPECT_EQ("12hours", settings.GetString(CSettings::SETTING_LOCALE_USE24HOURCLOCK));
}

TEST(TestLangInfo, A24HourTimeOnA12HourClockGainsItsMeridiem)
{
  EXPECT_EQ("h:mm:ss xx", CLangInfoTest::PrepareTimeFormat("H:mm:ss", false));
  EXPECT_EQ("h:mm:ss xx", CLangInfoTest::PrepareTimeFormat("h:mm:ss xx", false));
  EXPECT_EQ("h:mm:ss", CLangInfoTest::PrepareTimeFormat("h:mm:ss", false));
  EXPECT_EQ("H:mm:ss", CLangInfoTest::PrepareTimeFormat("h:mm:ss xx", true));
}
