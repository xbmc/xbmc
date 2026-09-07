/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "jobs/Job.h"
#include "jobs/JobManager.h"
#include "test/MtTestUtils.h"
#include "utils/XTimeUtils.h"

#include <atomic>
#include <mutex>

#include <gtest/gtest.h>

using namespace ConditionPoll;

struct Flags
{
  std::atomic<bool> lingerAtWork{true};
  std::atomic<bool> started{false};
  std::atomic<bool> finished{false};
  std::atomic<bool> wasCanceled{false};
};

class DummyJob : public CJob
{
  Flags* m_flags;
public:
  inline DummyJob(Flags* flags) : m_flags(flags)
  {
  }

  bool DoWork() override
  {
    m_flags->started = true;
    while (m_flags->lingerAtWork)
      std::this_thread::yield();

    if (ShouldCancel(0,0))
      m_flags->wasCanceled = true;

    m_flags->finished = true;
    return true;
  }
};

class ReallyDumbJob : public CJob
{
  Flags* m_flags;
public:
  inline ReallyDumbJob(Flags* flags) : m_flags(flags) {}

  bool DoWork() override
  {
    m_flags->finished = true;
    return true;
  }
};

class TestJobManager : public testing::Test
{
protected:
  TestJobManager() { CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>()); }

  ~TestJobManager() override
  {
    /* Always cancel jobs test completion */
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    CServiceBroker::UnregisterJobManager();
  }
};

TEST_F(TestJobManager, AddJob)
{
  Flags* flags = new Flags();
  ReallyDumbJob* job = new ReallyDumbJob(flags);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr);
  ASSERT_TRUE(poll([flags]() -> bool { return flags->finished; }));
  delete flags;
}

TEST_F(TestJobManager, CancelJob)
{
  unsigned int id;
  Flags* flags = new Flags();
  DummyJob* job = new DummyJob(flags);
  id = CServiceBroker::GetJobManager()->AddJob(job, nullptr);

  // wait for the worker thread to be entered
  ASSERT_TRUE(poll([flags]() -> bool { return flags->started; }));

  // cancel the job
  CServiceBroker::GetJobManager()->CancelJob(id);

  // let the worker thread continue
  flags->lingerAtWork = false;

  // make sure the job finished.
  ASSERT_TRUE(poll([flags]() -> bool { return flags->finished; }));

  // ... and that it was canceled.
  EXPECT_TRUE(flags->wasCanceled);
  delete flags;
}

namespace
{
struct JobControlPackage
{
  JobControlPackage()
  {
    // We're not ready to wait yet
    jobCreatedMutex.lock();
  }

  ~JobControlPackage()
  {
    jobCreatedMutex.unlock();
  }

  bool ready = false;
  XbmcThreads::ConditionVariable jobCreatedCond;
  CCriticalSection jobCreatedMutex;
};

class BroadcastingJob :
  public CJob
{
public:
  BroadcastingJob(JobControlPackage& package) : m_package(package) {}

  void FinishAndStopBlocking()
  {
    std::unique_lock lock(m_blockMutex);

    m_finish = true;
    m_block.notifyAll();
  }

  const char * GetType() const override
  {
    return "BroadcastingJob";
  }

  bool DoWork() override
  {
    {
      std::unique_lock lock(m_package.jobCreatedMutex);

      m_package.ready = true;
      m_package.jobCreatedCond.notifyAll();
    }

    std::unique_lock blockLock(m_blockMutex);

    // Block until we're told to go away
    while (!m_finish)
      m_block.wait(m_blockMutex);
    return true;
  }

private:

  JobControlPackage &m_package;

  XbmcThreads::ConditionVariable m_block;
  CCriticalSection m_blockMutex;
  bool m_finish = false;
};

BroadcastingJob *
WaitForJobToStartProcessing(CJob::PRIORITY priority, JobControlPackage &package)
{
  BroadcastingJob* job = new BroadcastingJob(package);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr, priority);

  // We're now ready to wait, wait and then unblock once ready
  while (!package.ready)
    package.jobCreatedCond.wait(package.jobCreatedMutex);

  return job;
}
}

TEST_F(TestJobManager, PauseLowPriorityJob)
{
  JobControlPackage package;
  BroadcastingJob *job (WaitForJobToStartProcessing(CJob::PRIORITY_LOW_PAUSABLE, package));

  EXPECT_TRUE(CServiceBroker::GetJobManager()->IsProcessing(CJob::PRIORITY_LOW_PAUSABLE));
  CServiceBroker::GetJobManager()->PauseJobs();
  EXPECT_FALSE(CServiceBroker::GetJobManager()->IsProcessing(CJob::PRIORITY_LOW_PAUSABLE));
  CServiceBroker::GetJobManager()->UnPauseJobs();
  EXPECT_TRUE(CServiceBroker::GetJobManager()->IsProcessing(CJob::PRIORITY_LOW_PAUSABLE));

  job->FinishAndStopBlocking();
}

TEST_F(TestJobManager, IsProcessing)
{
  JobControlPackage package;
  BroadcastingJob *job (WaitForJobToStartProcessing(CJob::PRIORITY_LOW_PAUSABLE, package));

  EXPECT_EQ(0, CServiceBroker::GetJobManager()->IsProcessing(""));

  job->FinishAndStopBlocking();
}

namespace
{
/*! \brief Records how many of these run at once, holding each until released

 CJob::Equals() is false by default, so several of these are distinct jobs rather than one the
 manager collapses into the first.
 */
class CountingJob : public CJob
{
public:
  struct Shared
  {
    std::atomic<int> running{0};
    std::atomic<int> peak{0};
    std::atomic<int> finished{0};
    std::atomic<bool> release{false};
  };

  explicit CountingJob(Shared& shared) : m_shared(shared) {}

  const char* GetType() const override { return "CountingJob"; }

  bool DoWork() override
  {
    const int running{++m_shared.running};
    for (int peak = m_shared.peak;
         running > peak && !m_shared.peak.compare_exchange_weak(peak, running);)
      ;

    while (!m_shared.release)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

    --m_shared.running;
    ++m_shared.finished;
    return true;
  }

private:
  Shared& m_shared;
};

void AddCountingJobs(CountingJob::Shared& shared, unsigned int count, CJob::PRIORITY priority)
{
  for (unsigned int i = 0; i < count; ++i)
    CServiceBroker::GetJobManager()->AddJob(new CountingJob(shared), nullptr, priority);
}

/*! \brief Lets the jobs holding this finish, and waits until they have

 A failed assertion returns from the test at once, so without this the jobs would be left waiting
 to be released - which the fixture's CancelJobs() then waits on forever - and what they are
 waiting on would go out of scope underneath them.
 */
class Releaser
{
public:
  explicit Releaser(CountingJob::Shared& shared) : m_shared(shared) {}

  ~Releaser()
  {
    m_shared.release = true;
    poll([this]() { return m_shared.running == 0; });
  }

  Releaser(const Releaser&) = delete;
  Releaser& operator=(const Releaser&) = delete;

private:
  CountingJob::Shared& m_shared;
};
} // namespace

TEST_F(TestJobManager, PausableJobsRunInParallelFromCold)
{
  // Nothing has run yet, so there are no workers waiting for these and one has to be made for
  // each of them
  const auto expected{static_cast<int>(CJobManager::GetMaxPausableWorkers())};

  CountingJob::Shared shared;
  AddCountingJobs(shared, expected * 2, CJob::PRIORITY_LOW_PAUSABLE);

  EXPECT_TRUE(poll([&shared, expected]() { return shared.peak == expected; }));

  shared.release = true;
  ASSERT_TRUE(poll([&shared, expected]() { return shared.finished == expected * 2; }));
  EXPECT_EQ(expected, shared.peak);
}

TEST_F(TestJobManager, PausableJobsQueuedWhilePausedRunOnUnPause)
{
  CServiceBroker::GetJobManager()->PauseJobs();

  CountingJob::Shared shared;
  shared.release = true;
  AddCountingJobs(shared, 1, CJob::PRIORITY_LOW_PAUSABLE);

  EXPECT_FALSE(poll(500, [&shared]() { return shared.finished > 0; }));

  CServiceBroker::GetJobManager()->UnPauseJobs();

  EXPECT_TRUE(poll([&shared]() { return shared.finished == 1; }));
}

TEST_F(TestJobManager, PausableJobsDoNotConsumeTheBudgetOfOtherPriorities)
{
  const auto pausableLimit{CJobManager::GetMaxPausableWorkers()};

  // One at a time, waiting for each to be running before asking for the next, so that reaching
  // the limit doesn't depend on how a burst of them is dispatched
  CountingJob::Shared pausable;
  const Releaser releaser{pausable};
  for (unsigned int i = 1; i <= pausableLimit; ++i)
  {
    AddCountingJobs(pausable, 1, CJob::PRIORITY_LOW_PAUSABLE);
    ASSERT_TRUE(poll([&pausable, i]() { return pausable.running == static_cast<int>(i); }));
  }

  // PRIORITY_LOW has the smallest allowance of those sharing one, so it is the first to be
  // starved were the jobs above counted against it
  CountingJob::Shared low;
  low.release = true;
  AddCountingJobs(low, 1, CJob::PRIORITY_LOW);
  EXPECT_TRUE(poll([&low]() { return low.finished == 1; }));

  EXPECT_EQ(static_cast<int>(pausableLimit), pausable.peak);

  pausable.release = true;
  ASSERT_TRUE(poll([&pausable, pausableLimit]()
                   { return pausable.finished == static_cast<int>(pausableLimit); }));
}
