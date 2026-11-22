/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonBuilder.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/AddonVersion.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/infoscanner/MusicAlbumInfo.h"
#include "music/infoscanner/MusicArtistInfo.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/ContentNames.h"
#include "utils/ScraperUrl.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XBMCTinyXML2.h"
#include "video/VideoInfoTag.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace ADDON;
using namespace std::chrono_literals;

namespace CONTENT = KODI::MEDIA::CONTENT;

TEST(TestScraper, TranslateContentNamesEachType)
{
  EXPECT_EQ(TranslateContent(ContentType::NONE), CONTENT::UNKNOWN);
  EXPECT_EQ(TranslateContent(ContentType::ALBUMS), CONTENT::ALBUMS);
  EXPECT_EQ(TranslateContent(ContentType::ARTISTS), CONTENT::ARTISTS);
  EXPECT_EQ(TranslateContent(ContentType::MOVIES), CONTENT::MOVIES);
  EXPECT_EQ(TranslateContent(ContentType::TVSHOWS), CONTENT::TVSHOWS);
  EXPECT_EQ(TranslateContent(ContentType::MUSICVIDEOS), CONTENT::MUSICVIDEOS);
}

TEST(TestScraper, TranslateContentReadsOnlyTheNamesItWrites)
{
  for (const auto type : {ContentType::NONE, ContentType::ALBUMS, ContentType::ARTISTS,
                          ContentType::MOVIES, ContentType::TVSHOWS, ContentType::MUSICVIDEOS})
    EXPECT_EQ(TranslateContent(TranslateContent(type)), type);

  EXPECT_EQ(TranslateContent("music"), ContentType::NONE);
}

namespace
{
constexpr auto TIMEOUT = 5s;

constexpr const char* PYTHON_SCRAPER_ID = "metadata.test.pythonscraper";

constexpr const char* PYTHON_SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.test.pythonscraper" name="Test python scraper" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies" library="scraper.py"/>
  <extension point="xbmc.addon.metadata">
    <platform>all</platform>
  </extension>
</addon>)";

//! The searches made of the scraper: whether they fail, and the threads they ran on
struct SSearches
{
  std::mutex lock;
  bool fail{false};
  std::vector<int> threads;
};

/*!
 \brief Stands in for the python interpreter running a scraper: a search finds nothing, or fails,
 ending its listing as unsuccessful
 */
class CSearchingInvoker : public ILanguageInvoker
{
public:
  CSearchingInvoker(ILanguageInvocationHandler* handler, SSearches& searches)
    : ILanguageInvoker(handler),
      m_searches(searches)
  {
  }

protected:
  bool execute(const std::string& script, const std::vector<std::string>& arguments) override
  {
    setState(InvokerStateRunning);
    // As a plugin is run: its url, handle and options
    const int handle{std::stoi(arguments[1])};
    bool fail{false};
    {
      std::unique_lock lock(m_searches.lock);
      fail = m_searches.fail;
      m_searches.threads.push_back(GetId());
    }

    XFILE::CPluginDirectory::EndOfDirectory(handle, !fail, false, false);

    setState(InvokerStateScriptDone);
    return true;
  }

  bool stop(bool abort) override { return true; }

private:
  SSearches& m_searches;
};

class CSearchingHandler : public ILanguageInvocationHandler
{
public:
  std::shared_ptr<ILanguageInvoker> CreateInvoker() override
  {
    return std::make_shared<CSearchingInvoker>(this, m_searches);
  }

  SSearches m_searches;
};

template<typename Condition>
bool Eventually(Condition condition)
{
  const auto end = std::chrono::steady_clock::now() + TIMEOUT;
  while (!condition())
  {
    if (std::chrono::steady_clock::now() > end)
      return false;
    std::this_thread::sleep_for(1ms);
  }
  return true;
}
} // namespace

class TestPythonScraper : public testing::Test
{
protected:
  void SetUp() override
  {
    // Python itself isn't run by the tests, so its scripts can be run by the stand-in
    if (Scripts().HasLanguageInvoker("scraper.py"))
      GTEST_SKIP() << "python scripts are run by python";
    Scripts().RegisterLanguageInvocationHandler(&m_handler, ".py");
    m_registered = true;

    const std::string addons{CSpecialProtocol::TranslatePath("special://home/addons/")};
    static_cast<void>(XFILE::CDirectory::Create(addons));
    m_folder = URIUtils::AddFileToFolder(addons, PYTHON_SCRAPER_ID);
    URIUtils::AddSlashAtEnd(m_folder);
    XFILE::CDirectory::RemoveRecursive(m_folder);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_folder));
    ASSERT_TRUE(Write("addon.xml", PYTHON_SCRAPER_ADDON));
    ASSERT_TRUE(Write("scraper.py", ""));
    ASSERT_TRUE(CServiceBroker::GetAddonMgr().FindAddon(PYTHON_SCRAPER_ID, ORIGIN_SYSTEM,
                                                        CAddonVersion{"1.0.0"}));

    AddonPtr addon;
    ASSERT_TRUE(
        CServiceBroker::GetAddonMgr().GetAddon(PYTHON_SCRAPER_ID, addon, OnlyEnabled::CHOICE_YES));
    m_scraper = std::dynamic_pointer_cast<CScraper>(addon);
    ASSERT_NE(nullptr, m_scraper);
  }

  void TearDown() override
  {
    if (!m_registered)
      return;

    for (const int id : Threads())
      EXPECT_TRUE(Eventually([id] { return !Scripts().IsRunning(id); }));
    Scripts().UnregisterLanguageInvocationHandler(&m_handler);

    m_scraper.reset();
    CServiceBroker::GetAddonMgr().UnloadAddon(PYTHON_SCRAPER_ID);
    XFILE::CDirectory::RemoveRecursive(m_folder);
  }

  static CScriptInvocationManager& Scripts() { return CScriptInvocationManager::GetInstance(); }

  bool Write(const std::string& name, const std::string& content) const
  {
    XFILE::CFile file;
    return file.OpenForWrite(URIUtils::AddFileToFolder(m_folder, name), true) &&
           file.Write(content.data(), content.size()) == static_cast<ssize_t>(content.size());
  }

  //! Makes the searches made of the scraper fail, or not
  void SearchesFail(bool fail)
  {
    std::unique_lock lock(m_handler.m_searches.lock);
    m_handler.m_searches.fail = fail;
  }

  std::vector<int> Threads()
  {
    std::unique_lock lock(m_handler.m_searches.lock);
    return m_handler.m_searches.threads;
  }

  CSearchingHandler m_handler;
  bool m_registered{false};
  std::string m_folder;
  ScraperPtr m_scraper;
};

TEST_F(TestPythonScraper, ASearchThatFailsIsAnError)
{
  SearchesFail(true);
  EXPECT_THROW(static_cast<void>(m_scraper->FindMovie("A search that fails", 2001)), CScraperError);
  EXPECT_EQ(1U, Threads().size()); // not made again
}

TEST_F(TestPythonScraper, ASearchThatFindsNothingIsNoError)
{
  std::vector<CScraperUrl> found;
  EXPECT_NO_THROW(found = m_scraper->FindMovie("A search that finds nothing", 2001));
  EXPECT_TRUE(found.empty());
  EXPECT_EQ(2U, Threads().size()); // made again, with the title cleaned less
}

TEST_F(TestPythonScraper, AMusicSearchThatFailsFindsNothing)
{
  // The music scanner doesn't catch a scraper's error
  SearchesFail(true);
  std::vector<MUSIC_GRABBER::CMusicAlbumInfo> albums;
  EXPECT_NO_THROW(albums = m_scraper->FindAlbum("Album", "Artist"));
  EXPECT_TRUE(albums.empty());
  std::vector<MUSIC_GRABBER::CMusicArtistInfo> artists;
  EXPECT_NO_THROW(artists = m_scraper->FindArtist("Artist"));
  EXPECT_TRUE(artists.empty());
}

TEST(TestScraper, LocalInformationOnlyLooksNothingUp)
{
  AddonPtr addon;
  ASSERT_TRUE(CServiceBroker::GetAddonMgr().GetAddon(
      "metadata.local", addon, AddonType::SCRAPER_MOVIES, OnlyEnabled::CHOICE_YES));
  const auto scraper{std::dynamic_pointer_cast<CScraper>(addon)};
  ASSERT_NE(nullptr, scraper);
  CScraperUrl url;
  url.AppendUrl(CScraperUrl::SUrlEntry(R"({"tmdb": "1"})"));
  CVideoInfoTag details;
  details.SetUniqueID("1", "tmdb", true);
  CAlbum album;
  CArtist artist;

  EXPECT_TRUE(scraper->LibPath().empty());
  EXPECT_TRUE(scraper->IsLocalInformationOnly());
  EXPECT_FALSE(scraper->NfoUrl("https://www.themoviedb.org/movie/1").HasUrls());
  EXPECT_TRUE(scraper->FindMovie("Film", 2001).empty());
  EXPECT_FALSE(scraper->GetArtwork(details));
  EXPECT_FALSE(scraper->GetVideoDetails({{"tmdb", "1"}}, url, true, details));
  EXPECT_TRUE(scraper->GetEpisodeList(url).empty());
  EXPECT_FALSE(scraper->ResolveIDToUrl("1").HasUrls());
  EXPECT_TRUE(scraper->FindAlbum("Album", "Artist").empty());
  EXPECT_TRUE(scraper->FindArtist("Artist").empty());
  EXPECT_FALSE(scraper->GetAlbumDetails(url, album));
  EXPECT_FALSE(scraper->GetArtistDetails(url, artist));
}

namespace
{
//! A scraper extension's point and library
using Extension = std::pair<std::string, std::string>;

CAddonMgr& Addons()
{
  return CServiceBroker::GetAddonMgr();
}

//! Whether an add-on with these scrapers is compatible. Its info and the add-on built from it
//! must agree, and only a compatible one's scraper is built as one.
bool Compatible(const std::vector<Extension>& extensions)
{
  std::string xml{
      R"(<addon id="metadata.test.compatible" name="Test" version="1.0.0" provider-name="Team Kodi">
  <requires><import addon="xbmc.metadata" version="2.1.0"/></requires>
)"};
  for (const auto& [point, library] : extensions)
    xml += R"(  <extension point=")" + point + R"(" library=")" + library + "\"/>\n";
  xml += R"(  <extension point="xbmc.addon.metadata"><platform>all</platform></extension>
</addon>)";
  CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(xml));
  const AddonInfoPtr info{CAddonInfoBuilder::Generate(doc.RootElement(), RepositoryDirInfo{})};
  const AddonPtr addon{CAddonBuilder::Generate(info, AddonType::UNKNOWN)};
  const bool compatible{Addons().IsCompatible(info)};
  EXPECT_EQ(compatible, Addons().IsCompatible(addon));
  EXPECT_EQ(compatible, std::dynamic_pointer_cast<CScraper>(addon) != nullptr);
  return compatible;
}
} // namespace

TEST(TestScraper, AnAddonIsCompatibleOnlyIfItsScrapersArePython)
{
  const Extension python{"xbmc.metadata.scraper.movies", "scraper.py"};
  const Extension python2{"xbmc.metadata.scraper.tvshows", "scraper.py"};
  const Extension xml{"xbmc.metadata.scraper.albums", "scraper.xml"};

  EXPECT_TRUE(Compatible({python}));
  EXPECT_TRUE(Compatible({python, python2}));
  EXPECT_FALSE(Compatible({xml}));
  EXPECT_FALSE(Compatible({python, xml}));
  EXPECT_FALSE(Compatible({xml, python}));
  EXPECT_TRUE(Addons().IsCompatible(Addons().GetAddonInfo("metadata.local", AddonType::UNKNOWN)));
}

TEST(TestScraper, AScraperARepositoryListsIsKnownByItsLibrary)
{
  for (const bool python : {false, true})
  {
    // As the add-on database builds it, with no library path
    CAddonInfoBuilderFromDB builder;
    builder.SetId("metadata.test.compatible");
    builder.SetVersion(CAddonVersion{"1.0.0"});
    CAddonType extension{AddonType::SCRAPER_TVSHOWS};
    extension.Insert("@library", python ? "scraper.py" : "scraper.xml");
    builder.SetExtensions(extension);
    const AddonInfoPtr info{builder.get()};
    const AddonPtr addon{CAddonBuilder::Generate(info, AddonType::SCRAPER_TVSHOWS)};

    EXPECT_EQ(python, Addons().IsCompatible(info));
    EXPECT_EQ(python, Addons().IsCompatible(addon));
    // An xml scraper is made a plain add-on, so a path that names it doesn't keep it in use
    EXPECT_EQ(python, std::dynamic_pointer_cast<CScraper>(addon) != nullptr);
  }
}

TEST(TestScraper, ADefaultScraperKodiNoLongerHasGivesWayToTheSettingsDefault)
{
  const auto id{CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER};
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  const auto setting{std::dynamic_pointer_cast<const CSettingString>(settings->GetSetting(id))};
  ASSERT_NE(nullptr, setting);
  ASSERT_FALSE(setting->GetDefault().empty());
  const std::string was{settings->GetString(id)};
  // An installed scraper is the one used
  const std::vector<std::pair<std::string, std::string>> savedAndUsed{
      {"metadata.local", "metadata.local"}, {"metadata.album.universal", setting->GetDefault()}};
  for (const auto& [saved, used] : savedAndUsed)
  {
    CXBMCTinyXML xml;
    EXPECT_TRUE(xml.Parse(R"(<settings version="2"><setting id="musiclibrary.albumsscraper">)" +
                          saved + "</setting></settings>"));
    EXPECT_TRUE(settings->LoadSetting(xml.RootElement(), id));
    AddonPtr addon;
    EXPECT_TRUE(CAddonSystemSettings::GetInstance().GetActive(AddonType::SCRAPER_ALBUMS, addon));
    EXPECT_EQ(used, addon ? addon->ID() : "");
  }
  settings->SetString(id, was);
}
