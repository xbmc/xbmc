/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/MtTestUtils.h"
#include "utils/Job.h"
#include "utils/JobManager.h"
#include "utils/XTimeUtils.h"

#include <atomic>

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
  TestJobManager() = default;

  ~TestJobManager() override
  {
    /* Always cancel jobs test completion */
    CJobManager::GetInstance().CancelJobs();
    CJobManager::GetInstance().Restart();
  }
};

TEST_F(TestJobManager, AddJob)
{
  Flags* flags = new Flags();
  ReallyDumbJob* job = new ReallyDumbJob(flags);
  CJobManager::GetInstance().AddJob(job, NULL);
  ASSERT_TRUE(poll([flags]() -> bool { return flags->finished; }));
  delete flags;
}

TEST_F(TestJobManager, CancelJob)
{
  unsigned int id;
  Flags* flags = new Flags();
  DummyJob* job = new DummyJob(flags);
  id = CJobManager::GetInstance().AddJob(job, NULL);

  // wait for the worker thread to be entered
  ASSERT_TRUE(poll([flags]() -> bool { return flags->started; }));

  // cancel the job
  CJobManager::GetInstance().CancelJob(id);

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

  BroadcastingJob(JobControlPackage &package) :
    m_package(package),
    m_finish(false)
  {
  }

  void FinishAndStopBlocking()
  {
    CSingleLock lock(m_blockMutex);

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
      CSingleLock lock(m_package.jobCreatedMutex);

      m_package.ready = true;
      m_package.jobCreatedCond.notifyAll();
    }

    CSingleLock blockLock(m_blockMutex);

    // Block until we're told to go away
    while (!m_finish)
      m_block.wait(m_blockMutex);
    return true;
  }

private:

  JobControlPackage &m_package;

  XbmcThreads::ConditionVariable m_block;
  CCriticalSection m_blockMutex;
  bool m_finish;
};

BroadcastingJob *
WaitForJobToStartProcessing(CJob::PRIORITY priority, JobControlPackage &package)
{
  BroadcastingJob* job = new BroadcastingJob(package);
  CJobManager::GetInstance().AddJob(job, NULL, priority);

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

  EXPECT_TRUE(CJobManager::GetInstance().IsProcessing(CJob::PRIORITY_LOW_PAUSABLE));
  CJobManager::GetInstance().PauseJobs();
  EXPECT_FALSE(CJobManager::GetInstance().IsProcessing(CJob::PRIORITY_LOW_PAUSABLE));
  CJobManager::GetInstance().UnPauseJobs();
  EXPECT_TRUE(CJobManager::GetInstance().IsProcessing(CJob::PRIORITY_LOW_PAUSABLE));

  job->FinishAndStopBlocking();
}

TEST_F(TestJobManager, IsProcessing)
{
  JobControlPackage package;
  BroadcastingJob *job (WaitForJobToStartProcessing(CJob::PRIORITY_LOW_PAUSABLE, package));

  EXPECT_EQ(0, CJobManager::GetInstance().IsProcessing(""));

  job->FinishAndStopBlocking();
}
