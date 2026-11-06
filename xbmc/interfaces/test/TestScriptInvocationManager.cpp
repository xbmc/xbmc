/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "threads/Event.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
constexpr auto TIMEOUT = 5s;

class CMockLanguageInvoker : public ILanguageInvoker
{
public:
  explicit CMockLanguageInvoker(ILanguageInvocationHandler* handler) : ILanguageInvoker(handler) {}

  void SetBlockExecution(bool block) { m_blockExecution = block; }

  void ContinueExecution() { m_continueExecution.Set(); }

  bool execute(const std::string& script, const std::vector<std::string>& arguments) override
  {
    m_workerThreadId.store(std::this_thread::get_id());
    setState(InvokerStateRunning);
    m_executedScript = script;
    m_executeCount++;
    m_executeStarted.Set();

    if (m_blockExecution)
    {
      m_continueExecution.Wait(TIMEOUT);
    }

    setState(InvokerStateScriptDone);
    m_executeFinished.Set();
    return true;
  }

  InvokerState GetState() const override
  {
    auto state = ILanguageInvoker::GetState();
    if (state == InvokerStateScriptDone && std::this_thread::get_id() == m_workerThreadId.load())
    {
      const_cast<CMockLanguageInvoker*>(this)->m_reusableCommitted.Set();
    }
    return state;
  }

  bool stop(bool abort) override
  {
    m_stopCalled = true;
    m_continueExecution.Set();
    setState(InvokerStateExecutionDone);
    return true;
  }

  void onExecutionDone() override
  {
    ILanguageInvoker::onExecutionDone();
    m_executionDone.Set();
  }

  std::atomic<std::thread::id> m_workerThreadId{};
  std::atomic<bool> m_stopCalled{false};
  std::atomic<bool> m_blockExecution{false};
  std::atomic<int> m_executeCount{0};
  std::string m_executedScript;
  CEvent m_executeStarted;
  CEvent m_continueExecution;
  CEvent m_executeFinished;
  CEvent m_reusableCommitted;
  CEvent m_executionDone;
};

class CMockLanguageInvocationHandler : public ILanguageInvocationHandler
{
public:
  std::shared_ptr<ILanguageInvoker> CreateInvoker() override
  {
    auto invoker = std::make_shared<CMockLanguageInvoker>(this);
    invoker->SetBlockExecution(m_defaultBlockExecution);
    m_invokers.push_back(invoker);
    return invoker;
  }

  std::atomic<bool> m_defaultBlockExecution{false};
  std::vector<std::shared_ptr<CMockLanguageInvoker>> m_invokers;
};
} // namespace

class TestScriptInvocationManager : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_tempDir = std::filesystem::temp_directory_path() /
                ("kodi_test_invoker_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(m_tempDir);

    m_scriptA = (m_tempDir / "script_a.py").string();
    m_scriptB = (m_tempDir / "script_b.py").string();

    std::ofstream(m_scriptA).close();
    std::ofstream(m_scriptB).close();

    CScriptInvocationManager::GetInstance().RegisterLanguageInvocationHandler(&m_handler, ".py");
  }

  void TearDown() override
  {
    CScriptInvocationManager::GetInstance().UnregisterLanguageInvocationHandler(&m_handler);
    CScriptInvocationManager::GetInstance().Uninitialize();

    std::error_code ec;
    std::filesystem::remove_all(m_tempDir, ec);
  }

  std::filesystem::path m_tempDir;
  std::string m_scriptA;
  std::string m_scriptB;
  CMockLanguageInvocationHandler m_handler;
};

TEST_F(TestScriptInvocationManager, DoesNotReleaseActiveRunningInvokerOnConcurrentInvocation)
{
  auto& manager = CScriptInvocationManager::GetInstance();

  // Configure handler to block script execution so invoker stays in InvokerStateRunning
  m_handler.m_defaultBlockExecution = true;

  int idA = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */, 100);
  ASSERT_GT(idA, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);
  auto invokerA = m_handler.m_invokers[0];

  ASSERT_TRUE(invokerA->m_executeStarted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateRunning);

  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), -1);
  EXPECT_FALSE(invokerA->m_stopCalled);

  auto invokerB = manager.GetLanguageInvoker(m_scriptB);
  ASSERT_NE(invokerB, nullptr);
  EXPECT_NE(invokerB, invokerA);

  EXPECT_FALSE(invokerA->m_stopCalled);
  EXPECT_EQ(invokerA->GetState(), InvokerStateRunning);

  // Allow script A to complete cleanly
  invokerA->ContinueExecution();
  ASSERT_TRUE(invokerA->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateScriptDone);

  // After completing, script A is still retained in m_lastInvokerThread for reuse
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), 100);
  EXPECT_EQ(manager.GetLanguageInvoker(m_scriptA), invokerA);
}

TEST_F(TestScriptInvocationManager, ReusesLanguageInvokerWhenScriptDone)
{
  auto& manager = CScriptInvocationManager::GetInstance();

  int id1 = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */, 42);
  ASSERT_GT(id1, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);
  auto invoker1 = m_handler.m_invokers[0];

  ASSERT_TRUE(invoker1->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invoker1->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invoker1->GetState(), InvokerStateScriptDone);
  EXPECT_EQ(invoker1->m_executeCount.load(), 1);

  // Reusable handle should now be available to first caller
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), 42);
  // Reusable handle was claimed, so concurrent callers cannot get the same handle
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), -1);

  // Subsequent call for the same script should reuse the existing invoker
  auto reusedInvoker = manager.GetLanguageInvoker(m_scriptA);
  EXPECT_EQ(reusedInvoker, invoker1);
  EXPECT_EQ(m_handler.m_invokers.size(), 1);

  int id2 = manager.ExecuteAsync(m_scriptA, reusedInvoker, nullptr, {}, true, 42);
  ASSERT_EQ(id2, id1);
  ASSERT_TRUE(invoker1->m_executeFinished.Wait(TIMEOUT));
  EXPECT_EQ(invoker1->m_executeCount.load(), 2);
}

TEST_F(TestScriptInvocationManager, ConcurrentReusablePluginHandleNotShared)
{
  auto& manager = CScriptInvocationManager::GetInstance();

  // Run script A with reusable handle 10
  int id1 = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */, 10);
  ASSERT_GT(id1, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);
  auto invoker = m_handler.m_invokers[0];

  ASSERT_TRUE(invoker->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invoker->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invoker->GetState(), InvokerStateScriptDone);

  // First caller retrieves and claims the reusable handle
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), 10);

  // Concurrent caller attempting to retrieve reusable handle before execution completes
  // must receive -1 so that a new unique handle is allocated, preventing handle sharing.
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), -1);

  // Subsequent execution reuses the handle and restores it upon completion
  auto reusedInvoker = manager.GetLanguageInvoker(m_scriptA);
  EXPECT_EQ(reusedInvoker, invoker);

  int id2 = manager.ExecuteAsync(m_scriptA, reusedInvoker, nullptr, {}, true, 10);
  ASSERT_EQ(id2, id1);

  ASSERT_TRUE(invoker->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invoker->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invoker->GetState(), InvokerStateScriptDone);

  // Handle is once again available for sequential reuse
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), 10);
}

TEST_F(TestScriptInvocationManager, ReleasesIdleInvokerWhenDifferentScriptRequested)
{
  auto& manager = CScriptInvocationManager::GetInstance();

  int id1 = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */);
  ASSERT_GT(id1, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);
  auto invokerA = m_handler.m_invokers[0];

  ASSERT_TRUE(invokerA->m_executeStarted.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateScriptDone);

  // When a different script is requested while script A is done (idle),
  // script A's reusable invoker thread should be released cleanly.
  auto invokerB = manager.GetLanguageInvoker(m_scriptB);
  ASSERT_NE(invokerB, nullptr);
  EXPECT_NE(invokerB, invokerA);
  ASSERT_TRUE(invokerA->m_executionDone.Wait(TIMEOUT));
}

TEST_F(TestScriptInvocationManager, NonReusableInvocationDoesNotClobberReusableInvoker)
{
  auto& manager = CScriptInvocationManager::GetInstance();

  // Configure handler to block script execution so invoker stays in InvokerStateRunning
  m_handler.m_defaultBlockExecution = true;

  // Execute script A as reusable
  int idA = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */);
  ASSERT_GT(idA, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);
  auto invokerA = m_handler.m_invokers[0];

  ASSERT_TRUE(invokerA->m_executeStarted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateRunning);

  // Execute script B as non-reusable while script A is running
  int idB = manager.ExecuteAsync(m_scriptB, nullptr, {}, false /* non-reusable */);
  ASSERT_GT(idB, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 2);
  auto invokerB = m_handler.m_invokers[1];

  // Allow both to complete
  invokerB->ContinueExecution();
  ASSERT_TRUE(invokerB->m_executeFinished.Wait(TIMEOUT));

  invokerA->ContinueExecution();
  ASSERT_TRUE(invokerA->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateScriptDone);

  // Script A should still be retained as the reusable invoker
  auto reusedInvoker = manager.GetLanguageInvoker(m_scriptA);
  EXPECT_EQ(reusedInvoker, invokerA);
}

TEST_F(TestScriptInvocationManager, DisplacedReusableInvokerReleasedOnceScriptDone)
{
  auto& manager = CScriptInvocationManager::GetInstance();

  // Execute script A as reusable (first run completes so it parks for reuse)
  int idA1 = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */, 10);
  ASSERT_GT(idA1, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);
  auto invokerA = m_handler.m_invokers[0];

  ASSERT_TRUE(invokerA->m_executeStarted.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateScriptDone);

  // Configure invokerA and handler to block on subsequent runs
  invokerA->SetBlockExecution(true);
  m_handler.m_defaultBlockExecution = true;

  // Execute script A a second time, reusing the parked invoker thread
  int idA2 = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */, 10);
  EXPECT_EQ(idA2, idA1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);

  ASSERT_TRUE(invokerA->m_executeStarted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateRunning);

  // Execute script B as reusable while script A's reused run is still active.
  // This displaces script A from m_lastInvokerThread and marks its release pending
  // until the active run finishes.
  int idB = manager.ExecuteAsync(m_scriptB, nullptr, {}, true /* reusable */, 20);
  ASSERT_GT(idB, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 2);
  auto invokerB = m_handler.m_invokers[1];

  ASSERT_TRUE(invokerB->m_executeStarted.Wait(TIMEOUT));
  EXPECT_FALSE(invokerA->m_stopCalled);
  EXPECT_EQ(invokerA->GetState(), InvokerStateRunning);

  // Allow script A's second run to complete
  invokerA->ContinueExecution();
  ASSERT_TRUE(invokerA->m_executeFinished.Wait(TIMEOUT));

  // Script A was displaced from m_lastInvokerThread while reserved;
  // as soon as its active execution completes, it must promote the pending release and exit cleanly.
  ASSERT_TRUE(invokerA->m_executionDone.Wait(TIMEOUT));

  // Allow script B to complete
  invokerB->ContinueExecution();
  ASSERT_TRUE(invokerB->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invokerB->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invokerB->GetState(), InvokerStateScriptDone);

  // Script B is retained as the active reusable invoker
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptB), 20);
  EXPECT_EQ(manager.GetLanguageInvoker(m_scriptB), invokerB);
}

TEST_F(TestScriptInvocationManager, SameScriptNonReusableInvocationDoesNotClobberReusableInvoker)
{
  auto& manager = CScriptInvocationManager::GetInstance();

  // Execute script A as reusable with handle 42
  int idA = manager.ExecuteAsync(m_scriptA, nullptr, {}, true /* reusable */, 42);
  ASSERT_GT(idA, -1);
  ASSERT_EQ(m_handler.m_invokers.size(), 1);
  auto invokerA = m_handler.m_invokers[0];

  ASSERT_TRUE(invokerA->m_executeStarted.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_executeFinished.Wait(TIMEOUT));
  ASSERT_TRUE(invokerA->m_reusableCommitted.Wait(TIMEOUT));
  EXPECT_EQ(invokerA->GetState(), InvokerStateScriptDone);

  // Execute same script A as non-reusable
  int idNonReusable = manager.ExecuteAsync(m_scriptA, nullptr, {}, false /* non-reusable */, -1);
  ASSERT_GT(idNonReusable, -1);
  ASSERT_NE(idNonReusable, idA);

  // A fresh invoker must have been instantiated rather than reusing invokerA
  ASSERT_EQ(m_handler.m_invokers.size(), 2);
  auto invokerNonReusable = m_handler.m_invokers[1];
  ASSERT_NE(invokerNonReusable, invokerA);

  ASSERT_TRUE(invokerNonReusable->m_executeFinished.Wait(TIMEOUT));

  // The cached reusable thread and handle must remain intact for script A
  EXPECT_EQ(manager.GetReusablePluginHandle(m_scriptA), 42);
  EXPECT_EQ(manager.GetLanguageInvoker(m_scriptA), invokerA);
}
