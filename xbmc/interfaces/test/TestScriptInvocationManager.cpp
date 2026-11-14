/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Addon.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "test/TestUtils.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
constexpr auto TIMEOUT = 5s;

//! What the runs of the scripts share: how many have started, how many run at once, and whether
//! they may finish
struct SRuns
{
  std::mutex lock;
  std::condition_variable changed;
  int started{0};
  int running{0};
  int most{0};
  bool finish{false};
};

//! Runs a script by waiting until it may finish
class CWaitingInvoker : public ILanguageInvoker
{
public:
  CWaitingInvoker(ILanguageInvocationHandler* handler, SRuns& runs)
    : ILanguageInvoker(handler),
      m_runs(runs)
  {
  }

protected:
  bool execute(const std::string& script, const std::vector<std::string>& arguments) override
  {
    setState(InvokerStateRunning);
    std::unique_lock lock(m_runs.lock);
    ++m_runs.started;
    m_runs.most = std::max(m_runs.most, ++m_runs.running);
    m_runs.changed.notify_all();
    m_runs.changed.wait_for(lock, TIMEOUT, [this] { return m_runs.finish || m_stopped; });
    setState(InvokerStateScriptDone);
    --m_runs.running;
    m_runs.changed.notify_all();
    return true;
  }

  bool stop(bool abort) override
  {
    std::unique_lock lock(m_runs.lock);
    m_stopped = true;
    m_runs.changed.notify_all();
    return true;
  }

private:
  SRuns& m_runs;
  bool m_stopped{false};
};

class CWaitingHandler : public ILanguageInvocationHandler
{
public:
  std::shared_ptr<ILanguageInvoker> CreateInvoker() override
  {
    return std::make_shared<CWaitingInvoker>(this, m_runs);
  }

  SRuns m_runs;
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

class TestScriptInvocationManager : public testing::Test
{
protected:
  void SetUp() override
  {
    m_file = XBMC_CREATETEMPFILE(".waiting");
    ASSERT_NE(nullptr, m_file);
    m_script = XBMC_TEMPFILEPATH(m_file);
    Manager().RegisterLanguageInvocationHandler(&m_handler, ".waiting");
  }

  void TearDown() override
  {
    Finish();
    // Not waiting in Stop(), which holds the manager's lock that an ending thread takes
    for (const int id : m_started)
      Manager().Stop(id);
    for (const int id : m_started)
      EXPECT_TRUE(Eventually([id] { return !Manager().IsRunning(id); }));
    Manager().UnregisterLanguageInvocationHandler(&m_handler);
    XBMC_DELETETEMPFILE(m_file);
  }

  static CScriptInvocationManager& Manager() { return CScriptInvocationManager::GetInstance(); }

  //! Starts the script as a plugin is run, with the handle of the thread it is given, if it is given
  //! one that has run it before, else with \p handle
  int Start(int& handle, const ADDON::AddonPtr& addon = nullptr)
  {
    if (const int reused = Manager().GetReusablePluginHandle(m_script); reused >= 0)
      handle = reused;
    const int id = Manager().ExecuteAsync(m_script, addon, {}, true, handle);
    m_started.push_back(id);
    return id;
  }

  template<typename Condition>
  bool WaitForRuns(Condition condition)
  {
    std::unique_lock lock(m_handler.m_runs.lock);
    return m_handler.m_runs.changed.wait_for(lock, TIMEOUT, [this, &condition]
                                             { return condition(m_handler.m_runs); });
  }

  bool RunningAtOnce(int count)
  {
    return WaitForRuns([count](const SRuns& runs) { return runs.running == count; });
  }

  //! Lets the runs finish, and waits for the \p started to have
  bool Finish(int started = 0)
  {
    {
      std::unique_lock lock(m_handler.m_runs.lock);
      m_handler.m_runs.finish = true;
    }
    m_handler.m_runs.changed.notify_all();
    return WaitForRuns([started](const SRuns& runs)
                       { return runs.started >= started && runs.running == 0; });
  }

  CWaitingHandler m_handler;
  XFILE::CFile* m_file{nullptr};
  std::string m_script;
  std::vector<int> m_started;
};

TEST_F(TestScriptInvocationManager, AFinishedRunsThreadRunsTheScriptAgainWithItsHandle)
{
  int handle{1001};
  const int id{Start(handle)};
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(handle);

  int again{1002};
  EXPECT_EQ(id, Start(again));
  EXPECT_EQ(1001, again);
  // The thread is taken by that run until it is released
  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script));
  ASSERT_TRUE(Finish(2));
  Manager().ReleasePluginHandle(again);
  EXPECT_EQ(1001, Manager().GetReusablePluginHandle(m_script));
  Manager().ReleasePluginHandle(1001);
}

// A run without a plugin handle, such as a plugin's action, runs on the thread kept for the script,
// which is free again once the run ends
TEST_F(TestScriptInvocationManager, ARunWithoutAHandleRunsOnAKeptThread)
{
  int handle{1001};
  const int id{Start(handle)};
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(handle);
  {
    std::unique_lock lock(m_handler.m_runs.lock);
    m_handler.m_runs.finish = false;
  }

  const int again{Manager().ExecuteAsync(m_script, nullptr, {}, true, -1)};
  m_started.push_back(again);
  EXPECT_EQ(id, again);
  ASSERT_TRUE(RunningAtOnce(1));
  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script));
  ASSERT_TRUE(Finish(2));
  EXPECT_EQ(1001, Manager().GetReusablePluginHandle(m_script));
  Manager().ReleasePluginHandle(1001);
}

// A plugin other than a scraper keeps the thread of one run, however many run at once
TEST_F(TestScriptInvocationManager, APluginOtherThanAScraperKeepsOneThread)
{
  const ADDON::AddonPtr plugin{std::make_shared<ADDON::CAddon>(
      ADDON::CAddonInfoBuilder::Generate("plugin.video.test", ADDON::AddonType::PLUGIN),
      ADDON::AddonType::PLUGIN)};
  int first{1001};
  int second{1002};
  Start(first, plugin);
  Start(second, plugin);
  ASSERT_TRUE(RunningAtOnce(2));
  ASSERT_TRUE(Finish(2));
  Manager().ReleasePluginHandle(first);
  Manager().ReleasePluginHandle(second);

  EXPECT_EQ(1001, Manager().GetReusablePluginHandle(m_script));
  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script));
  Manager().ReleasePluginHandle(1001);
}

TEST_F(TestScriptInvocationManager, OnlyAFewThreadsAreKept)
{
  std::vector<int> handles;
  std::vector<int> ids;
  for (int handle = 1001; handle <= 1005; ++handle)
  {
    int given{handle};
    ids.push_back(Start(given));
    handles.push_back(given);
  }
  // Each run has a thread and a handle of its own
  ASSERT_TRUE(RunningAtOnce(5));
  EXPECT_EQ(5u, std::set<int>(ids.begin(), ids.end()).size());
  EXPECT_EQ((std::vector<int>{1001, 1002, 1003, 1004, 1005}), handles);
  ASSERT_TRUE(Finish(5));
  for (const int handle : handles)
    Manager().ReleasePluginHandle(handle);

  // The fifth ended with its run
  EXPECT_TRUE(Eventually([id = ids.back()] { return !Manager().IsRunning(id); }));
  std::vector<int> reused;
  for (int i = 0; i < 4; ++i)
    reused.push_back(Manager().GetReusablePluginHandle(m_script));
  EXPECT_EQ((std::vector<int>{1001, 1002, 1003, 1004}), reused);
  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script));
  for (const int handle : reused)
    Manager().ReleasePluginHandle(handle);
}

TEST_F(TestScriptInvocationManager, AThreadKeptForAScriptIsLetGoWhenAnotherIsRun)
{
  int handle{1001};
  const int id{Start(handle)};
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(handle);

  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script + ".other.waiting"));
  EXPECT_TRUE(Eventually([id] { return !Manager().IsRunning(id); }));
  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script));
}
