/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "test/MtTestUtils.h"
#include "utils/Job.h"
#include "utils/JobManager.h"
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
    std::unique_lock<CCriticalSection> lock(m_blockMutex);

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
      std::unique_lock<CCriticalSection> lock(m_package.jobCreatedMutex);

      m_package.ready = true;
      m_package.jobCreatedCond.notifyAll();
    }

    std::unique_lock<CCriticalSection> blockLock(m_blockMutex);

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
