/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "threads/Event.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
constexpr auto TIMEOUT = 10s;

//! Runs until stopped, and takes as long to stop as the test allows, like a script that needs
//! time to dismantle itself.
class CSlowStopInvoker : public ILanguageInvoker
{
public:
  CSlowStopInvoker() : ILanguageInvoker(nullptr) {}

  CEvent m_started{true};
  CEvent m_stopEntered{true};
  CEvent m_releaseStop{true};
  std::atomic<int> m_maxConcurrentStops{0};
  //! Stop the script again from within its first stop, as a message handled while stopping can
  std::atomic_bool m_stopAgainFromStop{false};

protected:
  bool execute(const std::string&, const std::vector<std::string>&) override
  {
    setState(InvokerStateRunning);
    m_started.Set();
    m_scriptEnd.Wait(TIMEOUT);
    setState(InvokerStateScriptDone);
    return true;
  }

  bool stop(bool) override
  {
    const int concurrent = ++m_concurrentStops;
    int max = m_maxConcurrentStops;
    while (concurrent > max && !m_maxConcurrentStops.compare_exchange_weak(max, concurrent))
    {
    }

    setState(InvokerStateStopping);
    m_stopEntered.Set();
    if (m_stopAgainFromStop.exchange(false))
      CScriptInvocationManager::GetInstance().Stop(GetId());
    m_releaseStop.Wait(TIMEOUT);
    m_scriptEnd.Set();
    --m_concurrentStops;
    return true;
  }

private:
  CEvent m_scriptEnd{true};
  std::atomic<int> m_concurrentStops{0};
};

class TestScriptInvocationManager : public testing::Test
{
protected:
  void SetUp() override
  {
    // Unique per test, as ctest may run the tests in parallel processes
    const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
    m_script = std::filesystem::temp_directory_path() / ("kodi-test-" + name + ".fake");
    std::ofstream(m_script).put('\n');
    m_invoker = std::make_shared<CSlowStopInvoker>();
    m_id = m_manager.ExecuteAsync(m_script.string(), m_invoker);
    ASSERT_GE(m_id, 0);
    ASSERT_TRUE(m_invoker->m_started.Wait(TIMEOUT));
  }

  void TearDown() override
  {
    m_invoker->m_releaseStop.Set();
    for (int i = 0; i < 100 && m_manager.IsRunning(m_id); ++i)
      std::this_thread::sleep_for(50ms);
    m_manager.Process();
    std::filesystem::remove(m_script);
  }

  //! The manager must stay usable while a script takes its time to stop.
  void ExpectManagerResponsiveWhileStopping(std::future<bool> stop)
  {
    ASSERT_TRUE(m_invoker->m_stopEntered.Wait(TIMEOUT));

    auto isRunning = std::async(std::launch::async, [this] { return m_manager.IsRunning(m_id); });
    const bool responsive = isRunning.wait_for(2s) == std::future_status::ready;

    m_invoker->m_releaseStop.Set();
    EXPECT_TRUE(stop.get());
    EXPECT_TRUE(responsive);
  }

  CScriptInvocationManager& m_manager{CScriptInvocationManager::GetInstance()};
  std::filesystem::path m_script;
  std::shared_ptr<CSlowStopInvoker> m_invoker;
  int m_id{-1};
};
} // namespace

TEST_F(TestScriptInvocationManager, StopByIdDoesNotBlockManager)
{
  ExpectManagerResponsiveWhileStopping(
      std::async(std::launch::async, [this] { return m_manager.Stop(m_id); }));
}

TEST_F(TestScriptInvocationManager, StopByPathDoesNotBlockManager)
{
  ExpectManagerResponsiveWhileStopping(
      std::async(std::launch::async, [this] { return m_manager.Stop(m_script.string()); }));
}

TEST_F(TestScriptInvocationManager, ConcurrentStopsOfOneScriptAreSerialized)
{
  auto first = std::async(std::launch::async, [this] { return m_manager.Stop(m_id); });
  ASSERT_TRUE(m_invoker->m_stopEntered.Wait(TIMEOUT));
  auto second = std::async(std::launch::async, [this] { return m_manager.Stop(m_id); });

  // Give the second stop time to reach the invoker
  std::this_thread::sleep_for(500ms);
  m_invoker->m_releaseStop.Set();
  first.get();
  second.get();

  EXPECT_EQ(1, m_invoker->m_maxConcurrentStops);
}

TEST_F(TestScriptInvocationManager, StopFromWithinStopDoesNotDeadlock)
{
  m_invoker->m_stopAgainFromStop = true;
  m_invoker->m_releaseStop.Set();

  auto stop = std::async(std::launch::async, [this] { return m_manager.Stop(m_id); });
  ASSERT_EQ(std::future_status::ready, stop.wait_for(5s));
  EXPECT_NO_THROW(stop.get());
}
