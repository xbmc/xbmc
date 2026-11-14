/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/AddonVersion.h"
#include "addons/Scraper.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/HttpClientFactory.h"
#include "filesystem/IHttpClient.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "utils/ContentNames.h"
#include "utils/ScraperUrl.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

#include <chrono>
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
    <platform>all</platform>
  </extension>
</addon>)";

//! The details requests made of the scraper: how many, how many of the first fail, and the threads
//! they ran on
struct SRequests
{
  std::mutex lock;
  int made{0};
  int failing{0};
  std::vector<int> threads;
};

/*!
 \brief Stands in for the python interpreter running a scraper: a details request reads a movie
 called 'Details', but for the first that fail, which end without one, as a request the scraper's
 site drops does
 */
class CDetailsInvoker : public ILanguageInvoker
{
public:
  CDetailsInvoker(ILanguageInvocationHandler* handler, SRequests& requests)
    : ILanguageInvoker(handler),
      m_requests(requests)
  {
  }

protected:
  bool execute(const std::string& script, const std::vector<std::string>& arguments) override
  {
    setState(InvokerStateRunning);
    // As a plugin is run: its url, handle and options
    const int handle{std::stoi(arguments[1])};
    bool failing{false};
    {
      std::unique_lock lock(m_requests.lock);
      failing = ++m_requests.made <= m_requests.failing;
      m_requests.threads.push_back(GetId());
    }

    if (failing)
      XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
    else
    {
      CFileItem movie{"Details"};
      movie.GetVideoInfoTag()->SetTitle("Details");
      XFILE::CPluginDirectory::SetResolvedUrl(handle, true, &movie);
    }

    setState(InvokerStateScriptDone);
    return true;
  }

  bool stop(bool abort) override { return true; }

private:
  SRequests& m_requests;
};

class CDetailsHandler : public ILanguageInvocationHandler
{
public:
  std::shared_ptr<ILanguageInvoker> CreateInvoker() override
  {
    return std::make_shared<CDetailsInvoker>(this, m_requests);
  }

  SRequests m_requests;
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

  //! Reads the details of a movie, with the requests made for them failing \p failing times
  bool ReadDetails(int failing, CVideoInfoTag& details)
  {
    {
      std::unique_lock lock(m_handler.m_requests.lock);
      m_handler.m_requests.failing = failing;
    }
    CScraperUrl url;
    CScraperUrl::SUrlEntry entry;
    entry.m_url = R"({"tmdb": "1"})";
    url.AppendUrl(entry);
    const std::unique_ptr<XFILE::IHttpClient> http{XFILE::CreateHttpClient()};
    return m_scraper->GetVideoDetails(*http, {}, url, true, details);
  }

  int Requests()
  {
    std::unique_lock lock(m_handler.m_requests.lock);
    return m_handler.m_requests.made;
  }

  std::vector<int> Threads()
  {
    std::unique_lock lock(m_handler.m_requests.lock);
    return m_handler.m_requests.threads;
  }

  CDetailsHandler m_handler;
  bool m_registered{false};
  std::string m_folder;
  ScraperPtr m_scraper;
};

TEST_F(TestPythonScraper, ADetailsRequestThatFailsIsMadeOnceMore)
{
  CVideoInfoTag details;
  EXPECT_TRUE(ReadDetails(1, details));
  EXPECT_EQ("Details", details.GetTitle());
  EXPECT_EQ(2, Requests());
}

TEST_F(TestPythonScraper, ADetailsRequestIsMadeOnlyOnceMore)
{
  CVideoInfoTag details;
  EXPECT_FALSE(ReadDetails(2, details));
  EXPECT_EQ(2, Requests());
}

TEST_F(TestPythonScraper, ADetailsRequestThatSucceedsIsMadeOnce)
{
  CVideoInfoTag details;
  EXPECT_TRUE(ReadDetails(0, details));
  EXPECT_EQ(1, Requests());
}
