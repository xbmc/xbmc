/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "addons/Addon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/RunningScriptsHandler.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/generic/ScriptRunner.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
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
  //! The ids of the runs started
  std::vector<int> ids;
  //! Told the plugin handle of a run as it ends, as a plugin ends its directory
  std::function<void(int)> ended;
  //! How many times a run was told to stop
  int stops{0};
  //! The ids of the runs told to stop
  std::vector<int> stopped;
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
    m_runs.ids.push_back(GetId());
    m_runs.most = std::max(m_runs.most, ++m_runs.running);
    m_runs.changed.notify_all();
    m_runs.changed.wait_for(lock, TIMEOUT, [this] { return m_runs.finish || m_stopped; });
    if (m_runs.ended && arguments.size() > 1)
      m_runs.ended(std::stoi(arguments[1]));
    setState(InvokerStateScriptDone);
    --m_runs.running;
    m_runs.changed.notify_all();
    return true;
  }

  bool stop(bool abort) override
  {
    std::unique_lock lock(m_runs.lock);
    m_stopped = true;
    ++m_runs.stops;
    m_runs.stopped.push_back(GetId());
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

//! Runs a script as CPluginDirectory does, and is told the end of its run by its handle
class CPluginRun : public CRunningScriptsHandler<CPluginRun>
{
public:
  bool Run(const ADDON::AddonPtr& addon)
  {
    return RunScript(this, addon, CURL("plugin://" + addon->ID() + "/"), false);
  }

  //! As CPluginDirectory::EndOfDirectory()
  static void End(int handle)
  {
    std::unique_lock lock(GetScriptsLock());
    CPluginRun* run{GetScriptFromHandle(handle)};
    if (run == nullptr)
      return;
    ++run->m_ends;
    run->SetDone();
  }

  int Ends() const { return m_ends; }

  //! As the plugin's directory is cancelled
  void Cancel() { m_cancelled = true; }

  //! Whether the wait for the run has seen it cancelled
  bool CancelSeen() const { return m_cancelSeen; }

protected:
  bool IsSuccessful() const override { return m_ends > 0; }
  bool IsCancelled() const override
  {
    if (m_cancelled)
      m_cancelSeen = true;
    return m_cancelled;
  }

private:
  std::atomic<int> m_ends{0};
  std::atomic<bool> m_cancelled{false};
  mutable std::atomic<bool> m_cancelSeen{false};
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
    // With the runs started as a plugin is run
    {
      std::unique_lock lock(m_handler.m_runs.lock);
      m_started.insert(m_started.end(), m_handler.m_runs.ids.begin(), m_handler.m_runs.ids.end());
    }
    // Not waiting in Stop(), which holds the manager's lock that an ending thread takes
    for (const int id : m_started)
      Manager().Stop(id);
    for (const int id : m_started)
      EXPECT_TRUE(Eventually([id] { return !Manager().IsRunning(id); }));
    // A test stopped by a failed assertion leaves the threads of its runs claimed, which would hold
    // the places of kept threads in the tests after it
    for (const int handle : m_handles)
      Manager().ReleasePluginHandle(handle);
    Manager().UnregisterLanguageInvocationHandler(&m_handler);
    XBMC_DELETETEMPFILE(m_file);
    if (m_otherFile != nullptr)
      XBMC_DELETETEMPFILE(m_otherFile);
    SetSlowRun(CScriptInvocationManager::SLOW_RUN);
  }

  //! How long another run of its affinity may have a thread before a run stops waiting for it
  static void SetSlowRun(std::chrono::milliseconds slowRun)
  {
    std::unique_lock lock(Manager().m_critSection);
    Manager().m_slowRun = slowRun;
  }

  static CScriptInvocationManager& Manager() { return CScriptInvocationManager::GetInstance(); }

  //! The path of a script other than the test's, deleted as the test ends, or empty if it can't be
  //! made
  std::string OtherScript()
  {
    if (m_otherFile == nullptr)
      m_otherFile = XBMC_CREATETEMPFILE(".waiting");
    return m_otherFile != nullptr ? XBMC_TEMPFILEPATH(m_otherFile) : "";
  }

  //! A scraper whose library is the script, and whose threads are kept to run it again, as a python
  //! scraper's are
  ADDON::AddonPtr Scraper() const
  {
    const ADDON::AddonInfoPtr info{
        ADDON::CAddonInfoBuilder::Generate("metadata.test", ADDON::AddonType::SCRAPER_MOVIES)};
    info->SetPath(URIUtils::GetDirectory(m_script));
    info->SetLibName(URIUtils::GetFileName(m_script));
    info->AddExtraInfo("reuselanguageinvoker", "true");
    return std::make_shared<ADDON::CAddon>(info, ADDON::AddonType::SCRAPER_MOVIES);
  }

  //! Starts the script as a plugin is run, with the handle of the thread it is given, if it is given
  //! one that has run it before, else with \p handle
  int Start(int& handle, const ADDON::AddonPtr& addon = nullptr)
  {
    if (const int reused = Manager().GetReusablePluginHandle(m_script); reused >= 0)
      handle = reused;
    const int id = Manager().ExecuteAsync(m_script, addon, {}, true, handle);
    m_started.push_back(id);
    m_handles.push_back(handle);
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
  XFILE::CFile* m_otherFile{nullptr};
  std::vector<int> m_started;
  //! The plugin handles of the runs Start() started
  std::vector<int> m_handles;
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

// A run without a handle that took a kept thread, but whose script is gone by the time the run would
// be queued, lets the thread go
TEST_F(TestScriptInvocationManager, ARunWithoutAHandleThatIsNotQueuedLetsItsThreadGo)
{
  int handle{1001};
  Start(handle);
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(handle);

  // As ExecuteAsync() takes the thread, and then finds the script gone as it queues the run
  const std::shared_ptr<ILanguageInvoker> taken{Manager().GetLanguageInvoker(m_script)};
  ASSERT_NE(nullptr, taken);
  EXPECT_EQ(-1, Manager().ExecuteAsync(m_script + ".gone", taken, nullptr, {}, true, -1));

  // The next run without a handle is given the thread
  EXPECT_EQ(taken, Manager().GetLanguageInvoker(m_script));
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

// The threads kept for a script are still known by its path once a run of it alongside has ended
TEST_F(TestScriptInvocationManager, AScriptsKeptThreadsAreKnownByItsPathOnceARunAlongsideEnds)
{
  std::vector<int> ids;
  for (int handle = 1001; handle <= 1005; ++handle)
  {
    int given{handle};
    ids.push_back(Start(given));
  }
  ASSERT_TRUE(RunningAtOnce(5));
  ASSERT_TRUE(Finish(5));
  // The fifth, which isn't kept, ended with its run, and is forgotten
  ASSERT_TRUE(Eventually([id = ids.back()] { return !Manager().IsRunning(id); }));
  Manager().Process();
  ids.pop_back();

  EXPECT_TRUE(Manager().IsRunning(m_script));
  EXPECT_TRUE(Manager().Stop(m_script));
  std::unique_lock lock(m_handler.m_runs.lock);
  EXPECT_EQ(std::set<int>(ids.begin(), ids.end()),
            std::set<int>(m_handler.m_runs.stopped.begin(), m_handler.m_runs.stopped.end()));
}

// Each run of a script running at once is stopped, not only one
TEST_F(TestScriptInvocationManager, StoppingTheRunningScriptsStopsEachRunOfAScript)
{
  int first{1001};
  int second{1002};
  const std::set<int> ids{Start(first), Start(second)};
  ASSERT_TRUE(RunningAtOnce(2));

  Manager().StopRunningScripts();
  std::unique_lock lock(m_handler.m_runs.lock);
  EXPECT_EQ(ids, std::set<int>(m_handler.m_runs.stopped.begin(), m_handler.m_runs.stopped.end()));
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

// A run without a handle that takes the thread kept for its script lets go of one kept for another
TEST_F(TestScriptInvocationManager, ARunWithoutAHandleLetsGoOfAThreadKeptForAnotherScript)
{
  const std::string other{OtherScript()};
  ASSERT_FALSE(other.empty());
  // A thread kept for each script
  const int otherId{Manager().ExecuteAsync(other, nullptr, {}, true, 1002)};
  m_started.push_back(otherId);
  m_handles.push_back(1002);
  int handle{1001};
  Start(handle);
  ASSERT_TRUE(Finish(2));
  Manager().ReleasePluginHandle(1001);
  Manager().ReleasePluginHandle(1002);

  EXPECT_NE(nullptr, Manager().GetLanguageInvoker(m_script));
  EXPECT_TRUE(Eventually([otherId] { return !Manager().IsRunning(otherId); }));
}

// A run given an invoker of its own, as a context menu item's is, lets go of a thread kept for
// another script, as other runs do
TEST_F(TestScriptInvocationManager, ARunGivenItsOwnInvokerLetsGoOfAThreadKeptForAnotherScript)
{
  int handle{1001};
  const int id{Start(handle)};
  ASSERT_TRUE(Finish(1));
  Manager().ReleasePluginHandle(handle);

  const std::string other{OtherScript()};
  ASSERT_FALSE(other.empty());
  const int otherId{Manager().ExecuteAsync(other, m_handler.CreateInvoker(), nullptr, {}, false)};
  m_started.push_back(otherId);
  EXPECT_NE(-1, otherId);
  EXPECT_TRUE(Eventually([id] { return !Manager().IsRunning(id); }));
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

// A caller that has stopped stops waiting for its run, which runs on to its end on its thread.
// What it then returns reaches neither the caller nor a run started meanwhile.
TEST_F(TestScriptInvocationManager, AStoppedCallerStopsWaitingForItsRun)
{
  m_handler.m_runs.ended = &CPluginRun::End;
  const ADDON::AddonPtr scraper{Scraper()};
  std::atomic<bool> stop{false};
  CPluginRun abandoned;
  auto abandonedRun{std::async(std::launch::async,
                               [&stop, &abandoned, &scraper]
                               {
                                 const CScriptRunner::CAbandonWhen abandonWhen{stop};
                                 return abandoned.Run(scraper);
                               })};
  ASSERT_TRUE(RunningAtOnce(1));
  stop = true;
  const bool stoppedWaiting{abandonedRun.wait_for(1s) == std::future_status::ready};

  // A run started while it goes on isn't given its thread
  CPluginRun meanwhile;
  auto meanwhileRun{
      std::async(std::launch::async, [&meanwhile, &scraper] { return meanwhile.Run(scraper); })};
  ASSERT_TRUE(RunningAtOnce(2));
  ASSERT_TRUE(Finish(2));
  ASSERT_EQ(std::future_status::ready, meanwhileRun.wait_for(TIMEOUT));
  ASSERT_EQ(std::future_status::ready, abandonedRun.wait_for(TIMEOUT));

  // Once it has ended, its thread runs the script again
  CPluginRun after;
  EXPECT_TRUE(after.Run(scraper));

  EXPECT_TRUE(stoppedWaiting);
  EXPECT_FALSE(abandonedRun.get());
  EXPECT_EQ(0, abandoned.Ends());
  EXPECT_TRUE(meanwhileRun.get());
  EXPECT_EQ(1, meanwhile.Ends());
  EXPECT_EQ(1, after.Ends());
  std::unique_lock lock(m_handler.m_runs.lock);
  ASSERT_EQ(3u, m_handler.m_runs.ids.size());
  EXPECT_NE(m_handler.m_runs.ids[0], m_handler.m_runs.ids[1]);
  EXPECT_EQ(m_handler.m_runs.ids[0], m_handler.m_runs.ids[2]);
}

// A caller that stops while its cancelled run is given time to end stops waiting for it as well,
// and doesn't stop it
TEST_F(TestScriptInvocationManager, AStoppedCallerStopsWaitingForItsCancelledRun)
{
  const ADDON::AddonPtr scraper{Scraper()};
  std::atomic<bool> stop{false};
  CPluginRun cancelled;
  auto cancelledRun{std::async(std::launch::async,
                               [&stop, &cancelled, &scraper]
                               {
                                 const CScriptRunner::CAbandonWhen abandonWhen{stop};
                                 return cancelled.Run(scraper);
                               })};
  ASSERT_TRUE(RunningAtOnce(1));
  cancelled.Cancel();
  // Once the wait has seen the run cancelled, it gives the run its 30 s to end
  ASSERT_TRUE(Eventually([&cancelled] { return cancelled.CancelSeen(); }));
  stop = true;
  const bool stoppedWaiting{cancelledRun.wait_for(1s) == std::future_status::ready};
  // A wait that stopped the run did so before it returned
  int stops{0};
  {
    std::unique_lock lock(m_handler.m_runs.lock);
    stops = m_handler.m_runs.stops;
  }
  ASSERT_TRUE(Finish(1));
  ASSERT_EQ(std::future_status::ready, cancelledRun.wait_for(TIMEOUT));

  EXPECT_TRUE(stoppedWaiting);
  EXPECT_EQ(0, stops);
  EXPECT_FALSE(cancelledRun.get());
}

// A caller that stops while its run waits for its thread, which another affinity has borrowed,
// stops waiting at once, and starts no run
TEST_F(TestScriptInvocationManager, AStoppedCallerStopsWaitingForItsThread)
{
  const ADDON::AddonPtr scraper{Scraper()};
  const Affinity other;
  LeaveIdleThread(other, 1001);
  const Affinity affinity;
  int slow{1002};
  {
    const Affinity::CScope scope{affinity};
    Start(slow);
  }
  ASSERT_TRUE(RunningAtOnce(1));
  std::this_thread::sleep_for(150ms);
  int borrowed{-1};
  {
    const Affinity::CScope scope{affinity};
    borrowed = Manager().GetReusablePluginHandle(m_script);
  }
  ASSERT_EQ(1001, borrowed);

  std::atomic<bool> stop{false};
  CPluginRun waiting;
  auto waitingRun{std::async(std::launch::async,
                             [&other, &stop, &waiting, &scraper]
                             {
                               const Affinity::CScope scope{other};
                               const CScriptRunner::CAbandonWhen abandonWhen{stop};
                               return waiting.Run(scraper);
                             })};
  const bool waited{waitingRun.wait_for(200ms) == std::future_status::timeout};
  stop = true;
  const bool stoppedWaiting{waitingRun.wait_for(1s) == std::future_status::ready};
  Manager().ReleasePluginHandle(borrowed);
  ASSERT_EQ(std::future_status::ready, waitingRun.wait_for(TIMEOUT));
  ASSERT_TRUE(Finish(2));
  Manager().ReleasePluginHandle(slow);

  EXPECT_TRUE(waited);
  EXPECT_TRUE(stoppedWaiting);
  EXPECT_FALSE(waitingRun.get());
  // The run left idle and the slow one, and none for the stopped caller
  std::unique_lock lock(m_handler.m_runs.lock);
  EXPECT_EQ(2, m_handler.m_runs.started);
}
