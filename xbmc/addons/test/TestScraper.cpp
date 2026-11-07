/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "addons/AddonVersion.h"
#include "addons/Scraper.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/HttpClientFactory.h"
#include "filesystem/IHttpClient.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/generic/HeldNotifications.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "utils/ContentNames.h"
#include "utils/ScraperUrl.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoDownloader.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
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
    <reuselanguageinvoker>true</reuselanguageinvoker>
    <platform>all</platform>
  </extension>
</addon>)";

//! What the runs of the scraper share: how many have started, how many run at once, whether they
//! may finish, and the threads they ran on; how many details requests and searches were made, how
//! many of the first of each fail, and how many notifications of a failure were shown
struct SSearches
{
  std::mutex lock;
  std::condition_variable changed;
  int started{0};
  int running{0};
  int most{0};
  bool finish{false};
  std::vector<int> threads;
  int details{0};
  int detailsFailing{0};
  int searches{0};
  int searchesFailing{0};
  int notified{0};
};

/*!
 \brief Stands in for the python interpreter running a scraper: a search for a title finds one
 movie, by that title, once the search may finish. A details request reads a movie called
 'Details'. A search or details request set to fail does as the TMDb scraper does: it raises a
 notification and ends the directory without a result.
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
    const CURL options{"plugin://" + std::string{PYTHON_SCRAPER_ID} + "/" + arguments[2]};
    const std::string title{options.GetOption("title")};
    bool failing{false};
    {
      std::unique_lock lock(m_searches.lock);
      ++m_searches.started;
      m_searches.most = std::max(m_searches.most, ++m_searches.running);
      if (std::ranges::find(m_searches.threads, GetId()) == m_searches.threads.end())
        m_searches.threads.push_back(GetId());
      if (options.GetOption("action") == "getdetails")
        failing = ++m_searches.details <= m_searches.detailsFailing;
      else
        failing = ++m_searches.searches <= m_searches.searchesFailing;
      m_searches.changed.notify_all();
      m_searches.changed.wait_for(lock, TIMEOUT, [this] { return m_searches.finish; });
    }

    if (failing)
    {
      // As xbmcgui.Dialog().notification() does
      auto show = [this]
      {
        std::unique_lock lock(m_searches.lock);
        ++m_searches.notified;
      };
      if (!CHeldNotifications::Hold(GetId(), show))
        show();
      XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
    }
    else if (options.GetOption("action") == "getdetails")
    {
      CFileItem movie{"Details"};
      movie.GetVideoInfoTag()->SetTitle("Details");
      XFILE::CPluginDirectory::SetResolvedUrl(handle, true, &movie);
    }
    else
    {
      CFileItem movie{"http://movies.invalid/" + title, false};
      movie.SetLabel(title);
      XFILE::CPluginDirectory::AddItem(handle, &movie, 1);
      XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
    }

    std::unique_lock lock(m_searches.lock);
    setState(InvokerStateScriptDone);
    --m_searches.running;
    m_searches.changed.notify_all();
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

    Finish();
    // The threads kept to run the scraper again go once another script is run
    static_cast<void>(Scripts().GetReusablePluginHandle("another.py"));
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

  //! Searches for \p title on a thread of its own, waited for however the test ends
  std::future<void> Search(const std::string& title, std::vector<CScraperUrl>& found)
  {
    return std::async(std::launch::async,
                      [this, title, &found]
                      {
                        const std::unique_ptr<XFILE::IHttpClient> http{XFILE::CreateHttpClient()};
                        found = m_scraper->FindMovie(*http, title, 2000, true);
                      });
  }

  template<typename Condition>
  bool WaitForSearches(Condition condition)
  {
    std::unique_lock lock(m_handler.m_searches.lock);
    return m_handler.m_searches.changed.wait_for(lock, TIMEOUT, [this, &condition]
                                                 { return condition(m_handler.m_searches); });
  }

  void Finish()
  {
    {
      std::unique_lock lock(m_handler.m_searches.lock);
      m_handler.m_searches.finish = true;
    }
    m_handler.m_searches.changed.notify_all();
  }

  //! Reads the details of a movie, with the requests made for them failing \p failing times
  bool ReadDetails(int failing, CVideoInfoTag& details)
  {
    {
      std::unique_lock lock(m_handler.m_searches.lock);
      m_handler.m_searches.detailsFailing = failing;
    }
    Finish();
    CScraperUrl url;
    CScraperUrl::SUrlEntry entry;
    entry.m_url = R"({"tmdb": "1"})";
    url.AppendUrl(entry);
    const std::unique_ptr<XFILE::IHttpClient> http{XFILE::CreateHttpClient()};
    return m_scraper->GetVideoDetails(*http, {}, url, true, details);
  }

  //! Searches for 'Alpha' as a scan does, with the first \p failing searches failing
  std::vector<CScraperUrl> SearchAsAScan(int failing)
  {
    {
      std::unique_lock lock(m_handler.m_searches.lock);
      m_handler.m_searches.searchesFailing = failing;
    }
    Finish();
    std::vector<CScraperUrl> found;
    CVideoInfoDownloader scan{m_scraper};
    static_cast<void>(scan.FindMovie("Alpha", 2000, found));
    return found;
  }

  int Notified()
  {
    std::unique_lock lock(m_handler.m_searches.lock);
    return m_handler.m_searches.notified;
  }

  int DetailsRequests()
  {
    std::unique_lock lock(m_handler.m_searches.lock);
    return m_handler.m_searches.details;
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

TEST_F(TestPythonScraper, ItsCallsRunAlongsideEachOther)
{
  std::vector<std::vector<CScraperUrl>> found(3);
  std::vector<std::future<void>> searches;
  for (const std::string title : {"Alpha", "Beta", "Gamma"})
    searches.push_back(Search(title, found[searches.size()]));

  const bool atOnce{
      WaitForSearches([](const SSearches& searches) { return searches.running == 3; })};
  Finish();
  for (std::future<void>& search : searches)
    search.wait();

  EXPECT_TRUE(atOnce);
  ASSERT_EQ(1u, found[0].size());
  EXPECT_EQ("Alpha", found[0][0].GetTitle());
  ASSERT_EQ(1u, found[2].size());
  EXPECT_EQ("Gamma", found[2][0].GetTitle());
}

TEST_F(TestPythonScraper, ASearchAlreadyBeingMadeIsWaitedForRatherThanMadeAgain)
{
  std::vector<CScraperUrl> first;
  std::vector<CScraperUrl> second;
  std::future<void> searching{Search("Alpha", first)};
  ASSERT_TRUE(WaitForSearches([](const SSearches& searches) { return searches.running == 1; }));
  std::future<void> again{Search("Alpha", second)};
  // Time for the second to start a search of its own, if it were going to
  std::this_thread::sleep_for(100ms);
  Finish();
  searching.wait();
  again.wait();

  std::unique_lock lock(m_handler.m_searches.lock);
  EXPECT_EQ(1, m_handler.m_searches.started);
  ASSERT_EQ(1u, second.size());
  EXPECT_EQ("Alpha", second[0].GetTitle());
}

TEST_F(TestPythonScraper, ItsInterpretersAreKeptForItsLaterCalls)
{
  std::vector<std::vector<CScraperUrl>> found(2);
  std::vector<std::future<void>> searches;
  for (const std::string title : {"Alpha", "Beta"})
    searches.push_back(Search(title, found[searches.size()]));
  ASSERT_TRUE(WaitForSearches([](const SSearches& searches) { return searches.running == 2; }));
  Finish();
  for (std::future<void>& search : searches)
    search.wait();
  ASSERT_TRUE(WaitForSearches([](const SSearches& searches) { return searches.running == 0; }));

  searches.clear();
  for (const std::string title : {"Gamma", "Delta"})
    searches.push_back(Search(title, found[searches.size()]));
  for (std::future<void>& search : searches)
    search.wait();

  std::unique_lock lock(m_handler.m_searches.lock);
  EXPECT_EQ(4, m_handler.m_searches.started);
  EXPECT_EQ(2u, m_handler.m_searches.threads.size());
}

TEST_F(TestPythonScraper, ADetailsRequestThatFailsIsMadeOnceMoreAndWhatItSaysIsNotShown)
{
  CVideoInfoTag details;
  EXPECT_TRUE(ReadDetails(1, details));
  EXPECT_EQ("Details", details.GetTitle());
  EXPECT_EQ(2, DetailsRequests());
  EXPECT_EQ(0, Notified());
}

TEST_F(TestPythonScraper, ADetailsRequestIsMadeOnlyOnceMoreAndWhatItSaysTwiceIsShownOnce)
{
  CVideoInfoTag details;
  EXPECT_FALSE(ReadDetails(2, details));
  EXPECT_EQ(2, DetailsRequests());
  EXPECT_EQ(1, Notified());
}

TEST_F(TestPythonScraper, ADetailsRequestThatSucceedsIsMadeOnce)
{
  CVideoInfoTag details;
  EXPECT_TRUE(ReadDetails(0, details));
  EXPECT_EQ(1, DetailsRequests());
}

TEST_F(TestPythonScraper, WhatASearchThatFailsSaysIsNotShownWhenTheNextSearchFindsTheMovie)
{
  const std::vector<CScraperUrl> found{SearchAsAScan(1)};
  ASSERT_EQ(1u, found.size());
  EXPECT_EQ("Alpha", found[0].GetTitle());
  EXPECT_EQ(0, Notified());
}

TEST_F(TestPythonScraper, WhatASearchThatNothingTriesAgainSaysIsShownAtOnce)
{
  {
    std::unique_lock lock(m_handler.m_searches.lock);
    m_handler.m_searches.searchesFailing = 1;
  }
  Finish();
  std::vector<CScraperUrl> found;
  Search("Alpha", found).wait();
  EXPECT_TRUE(found.empty());
  EXPECT_EQ(1, Notified());
}
