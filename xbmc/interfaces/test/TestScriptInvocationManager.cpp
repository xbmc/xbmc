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
#include <future>
#include <memory>
#include <mutex>
#include <optional>
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
    SetSlowRun(CScriptInvocationManager::SLOW_RUN);
  }

  //! How long another run of its affinity may have a thread before a run stops waiting for it
  static void SetSlowRun(std::chrono::milliseconds slowRun)
  {
    std::unique_lock lock(Manager().m_critSection);
    Manager().m_slowRun = slowRun;
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

  //! Leaves a thread bound to \p affinity free, with \p handle, and lets the runs after wait again
  void LeaveIdleThread(const CScriptInvocationManager::CAffinity& affinity, int handle)
  {
    {
      const CScriptInvocationManager::CAffinity::CScope scope{affinity};
      Start(handle);
    }
    ASSERT_TRUE(Finish(1));
    Manager().ReleasePluginHandle(handle);
    std::unique_lock lock(m_handler.m_runs.lock);
    m_handler.m_runs.finish = false;
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

using Affinity = CScriptInvocationManager::CAffinity;

TEST_F(TestScriptInvocationManager, TheRunsOfAnAffinityKeepToItsThread)
{
  const Affinity affinity;
  const Affinity::CScope scope{affinity};
  int first{1001};
  const int id{Start(first)};
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(first);

  int again{1002};
  EXPECT_EQ(id, Start(again));
  EXPECT_EQ(1001, again);
  ASSERT_TRUE(Finish(2));
  Manager().ReleasePluginHandle(again);
}

TEST_F(TestScriptInvocationManager, AThreadBoundToAnAffinityIsLeftToItsRunsUntilItGoes)
{
  std::optional<Affinity> affinity{std::in_place};
  int handle{1001};
  {
    const Affinity::CScope scope{*affinity};
    Start(handle);
  }
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(handle);

  // Neither a run tied to no affinity nor one tied to another takes it
  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script));
  {
    const Affinity other;
    const Affinity::CScope scope{other};
    EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script));
  }

  affinity.reset();
  EXPECT_EQ(1001, Manager().GetReusablePluginHandle(m_script));
  Manager().ReleasePluginHandle(1001);
}

// Another script's run lets go of the threads kept for this one, but not of one bound to an affinity
TEST_F(TestScriptInvocationManager, AThreadBoundToAnAffinityIsKeptWhenAnotherScriptIsRun)
{
  std::optional<Affinity> affinity{std::in_place};
  LeaveIdleThread(*affinity, 1001);
  const int id{m_started.back()};

  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script + ".other.waiting"));
  {
    const Affinity::CScope scope{*affinity};
    EXPECT_EQ(1001, Manager().GetReusablePluginHandle(m_script));
    Manager().ReleasePluginHandle(1001);
  }

  // Once the affinity goes, it is let go of as any other
  affinity.reset();
  EXPECT_EQ(-1, Manager().GetReusablePluginHandle(m_script + ".other.waiting"));
  EXPECT_TRUE(Eventually([id] { return !Manager().IsRunning(id); }));
}

TEST_F(TestScriptInvocationManager, ARunWaitsWhileAnotherRunOfItsAffinityHasItsThread)
{
  // However long the waiting run takes to start
  SetSlowRun(TIMEOUT);
  const Affinity affinity;
  int first{1001};
  {
    const Affinity::CScope scope{affinity};
    Start(first);
  }
  ASSERT_TRUE(RunningAtOnce(1));

  auto waiting{std::async(std::launch::async,
                          [this, &affinity]
                          {
                            const Affinity::CScope scope{affinity};
                            return Manager().GetReusablePluginHandle(m_script);
                          })};
  const bool waited{waiting.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout};
  // Lets the first run end soon, as one served by what the plugin kept does, and free the thread,
  // which the waiting run then takes
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(first);
  ASSERT_EQ(std::future_status::ready, waiting.wait_for(TIMEOUT));
  const int taken{waiting.get()};
  Manager().ReleasePluginHandle(taken);

  EXPECT_TRUE(waited);
  EXPECT_EQ(1001, taken);
}

// A run doesn't wait for one of its affinity that has had the thread a while, as a run that slow
// gains little from keeping to it: it has a thread of its own
TEST_F(TestScriptInvocationManager, ARunDoesntWaitForASlowRunOfItsAffinity)
{
  const Affinity affinity;
  int first{1001};
  {
    const Affinity::CScope scope{affinity};
    Start(first);
  }
  ASSERT_TRUE(RunningAtOnce(1));
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  auto taking{std::async(std::launch::async,
                         [this, &affinity]
                         {
                           const Affinity::CScope scope{affinity};
                           return Manager().GetReusablePluginHandle(m_script);
                         })};
  const bool tookAnother{taking.wait_for(std::chrono::seconds(1)) == std::future_status::ready};
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(first);
  ASSERT_EQ(std::future_status::ready, taking.wait_for(TIMEOUT));
  const int taken{taking.get()};
  Manager().ReleasePluginHandle(taken);

  EXPECT_TRUE(tookAnother);
  EXPECT_EQ(-1, taken);
}

// Rather than start a thread of its own, a run that doesn't wait for its thread borrows an idle one
// bound to another affinity, which stays bound to it
TEST_F(TestScriptInvocationManager, ARunNotWaitingForItsThreadBorrowsAnIdleOneOfAnotherAffinity)
{
  const Affinity other;
  LeaveIdleThread(other, 1001);
  const Affinity affinity;
  const Affinity::CScope scope{affinity};
  int slow{1002};
  Start(slow);
  ASSERT_TRUE(RunningAtOnce(1));
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  const int borrowed{Manager().GetReusablePluginHandle(m_script)};
  Manager().ReleasePluginHandle(borrowed);
  ASSERT_TRUE(Finish(2));
  Manager().ReleasePluginHandle(slow);
  int boundToOther{-1};
  {
    const Affinity::CScope otherScope{other};
    boundToOther = Manager().GetReusablePluginHandle(m_script);
  }
  Manager().ReleasePluginHandle(boundToOther);

  EXPECT_EQ(1001, borrowed);
  EXPECT_EQ(1001, boundToOther);
}

// A run waits for its thread while another affinity has borrowed it, however long, so that what
// the plugin kept on it stays with the runs it was kept for
TEST_F(TestScriptInvocationManager, ARunWaitsForItsThreadWhileAnotherAffinityHasBorrowedIt)
{
  const Affinity other;
  LeaveIdleThread(other, 1001);
  const Affinity affinity;
  int slow{1002};
  {
    const Affinity::CScope scope{affinity};
    Start(slow);
  }
  ASSERT_TRUE(RunningAtOnce(1));
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  int borrowed{-1};
  {
    const Affinity::CScope scope{affinity};
    borrowed = Manager().GetReusablePluginHandle(m_script);
  }
  ASSERT_EQ(1001, borrowed);
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  auto waiting{std::async(std::launch::async,
                          [this, &other]
                          {
                            const Affinity::CScope scope{other};
                            return Manager().GetReusablePluginHandle(m_script);
                          })};
  const bool waited{waiting.wait_for(std::chrono::milliseconds(200)) ==
                    std::future_status::timeout};
  Manager().ReleasePluginHandle(borrowed);
  ASSERT_EQ(std::future_status::ready, waiting.wait_for(TIMEOUT));
  const int taken{waiting.get()};
  Manager().ReleasePluginHandle(taken);
  ASSERT_TRUE(Finish(2));
  Manager().ReleasePluginHandle(slow);

  EXPECT_TRUE(waited);
  EXPECT_EQ(1001, taken);
}
