/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "jobs/Job.h"
#include "jobs/JobManager.h"
#include "jobs/JobQueue.h"
#include "test/MtTestUtils.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

using namespace ConditionPoll;
using namespace std::chrono_literals;

namespace
{
//! \brief Holds until released, so that a test can decide when the queue drains
class BlockingJob : public CJob
{
public:
  struct Shared
  {
    std::atomic<int> started{0};
    std::atomic<int> finished{0};
    std::atomic<bool> release{false};
  };

  explicit BlockingJob(Shared& shared) : m_shared(shared) {}

  const char* GetType() const override { return "BlockingJob"; }

  bool DoWork() override
  {
    ++m_shared.started;
    while (!m_shared.release)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

    ++m_shared.finished;
    return true;
  }

private:
  Shared& m_shared;
};

/*! \brief Lets the jobs holding this finish, and waits until they have

 A failed assertion returns from the test at once, so without this the jobs would be left waiting
 to be released - which the queue's destructor then waits on forever - and what they are waiting
 on would go out of scope underneath them.
 */
class Releaser
{
public:
  explicit Releaser(BlockingJob::Shared& shared) : m_shared(shared) {}

  ~Releaser()
  {
    m_shared.release = true;
    poll([this]() { return m_shared.finished == m_shared.started; });
  }

  Releaser(const Releaser&) = delete;
  Releaser& operator=(const Releaser&) = delete;

private:
  BlockingJob::Shared& m_shared;
};

//! \brief How long a drained queue should take to report itself idle
constexpr auto SETTLE{2000ms};

//! \brief Long enough to show a queue is not reporting itself idle, short enough not to drag
constexpr auto NOT_IDLE{300ms};
} // namespace

class TestJobQueue : public testing::Test
{
protected:
  TestJobQueue() { CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>()); }

  ~TestJobQueue() override
  {
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    CServiceBroker::UnregisterJobManager();
  }
};

TEST_F(TestJobQueue, WaitForCompletionReturnsWhenNothingWasEverQueued)
{
  CJobQueue queue{false, 1, CJob::PRIORITY_LOW};

  EXPECT_TRUE(queue.WaitForCompletion(NOT_IDLE));
}

TEST_F(TestJobQueue, WaitForCompletionReturnsOnceJobsFinish)
{
  CJobQueue queue{false, 2, CJob::PRIORITY_LOW};

  BlockingJob::Shared shared;
  const Releaser releaser{shared};
  ASSERT_TRUE(queue.AddJob(new BlockingJob(shared)));
  ASSERT_TRUE(queue.AddJob(new BlockingJob(shared)));
  ASSERT_TRUE(poll([&shared]() { return shared.started == 2; }));

  EXPECT_FALSE(queue.WaitForCompletion(NOT_IDLE));

  shared.release = true;
  EXPECT_TRUE(queue.WaitForCompletion(SETTLE));
  EXPECT_EQ(2, shared.finished);
}

TEST_F(TestJobQueue, WaitForCompletionReturnsOnceCancellingLeavesNothingQueued)
{
  CJobQueue queue{false, 1, CJob::PRIORITY_LOW};

  BlockingJob::Shared shared;
  const Releaser releaser{shared};
  auto* running = new BlockingJob(shared);
  auto* waiting = new BlockingJob(shared);

  ASSERT_TRUE(queue.AddJob(running));
  ASSERT_TRUE(poll([&shared]() { return shared.started == 1; }));

  // One at a time, so this one is still held by the queue rather than handed to the manager
  ASSERT_TRUE(queue.AddJob(waiting));

  queue.CancelJob(waiting);
  EXPECT_FALSE(queue.WaitForCompletion(NOT_IDLE));

  // Nothing is left for the queue to report on, even though what it handed over is still going
  queue.CancelJob(running);
  EXPECT_TRUE(queue.WaitForCompletion(NOT_IDLE));
}
