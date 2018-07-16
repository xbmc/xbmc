/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/JobManager.h"
#include "utils/Job.h"

#include "gtest/gtest.h"
#include <atomic>

#ifdef TARGET_POSIX
#include "platform/linux/XTimeUtils.h"
#endif

std::atomic<bool> cancelled(false);

class DummyJob : public CJob
{
public:
  bool DoWork() override
  {
    Sleep(100);
    if (ShouldCancel(0,0))
      cancelled = true;

    return true;
  }
};

class TestJobManager : public testing::Test
{
protected:
  TestJobManager()
  {
  }

  ~TestJobManager() override
  {
    /* Always cancel jobs test completion */
    CJobManager::GetInstance().CancelJobs();
    CJobManager::GetInstance().Restart();
  }
};

TEST_F(TestJobManager, AddJob)
{
  CJob* job = new DummyJob();
  CJobManager::GetInstance().AddJob(job, NULL);
}

TEST_F(TestJobManager, CancelJob)
{
  unsigned int id;
  CJob* job = new DummyJob();
  id = CJobManager::GetInstance().AddJob(job, NULL);
  Sleep(50);
  CJobManager::GetInstance().CancelJob(id);
  Sleep(100);
  EXPECT_TRUE(cancelled);
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
