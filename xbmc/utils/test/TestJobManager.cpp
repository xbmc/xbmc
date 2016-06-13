/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/JobManager.h"
#include "settings/Settings.h"
#include "utils/SystemInfo.h"

#include "gtest/gtest.h"

/* CSysInfoJob::GetInternetState() will test for network connectivity. */
class TestJobManager : public testing::Test
{
protected:
  TestJobManager()
  {
    //! @todo implement
    /*
    CSettingsCategory* net = CSettings::GetInstance().AddCategory(4, "network", 798);
    CSettings::GetInstance().AddBool(net, CSettings::SETTING_NETWORK_USEHTTPPROXY, 708, false);
    CSettings::GetInstance().AddString(net, CSettings::SETTING_NETWORK_HTTPPROXYSERVER, 706, "",
                            EDIT_CONTROL_INPUT);
    CSettings::GetInstance().AddString(net, CSettings::SETTING_NETWORK_HTTPPROXYPORT, 730, "8080",
                            EDIT_CONTROL_NUMBER_INPUT, false, 707);
    CSettings::GetInstance().AddString(net, CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME, 1048, "",
                            EDIT_CONTROL_INPUT);
    CSettings::GetInstance().AddString(net, CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD, 733, "",
                            EDIT_CONTROL_HIDDEN_INPUT,true,733);
    CSettings::GetInstance().AddInt(net, CSettings::SETTING_NETWORK_BANDWIDTH, 14041, 0, 0, 512, 100*1024,
                         SPIN_CONTROL_INT_PLUS, 14048, 351);
    */
  }

  ~TestJobManager()
  {
    /* Always cancel jobs test completion */
    CJobManager::GetInstance().CancelJobs();
    CJobManager::GetInstance().Restart();
    CSettings::GetInstance().Unload();
  }
};

TEST_F(TestJobManager, AddJob)
{
  CJob* job = new CSysInfoJob();
  CJobManager::GetInstance().AddJob(job, NULL);
}

TEST_F(TestJobManager, CancelJob)
{
  unsigned int id;
  CJob* job = new CSysInfoJob();
  id = CJobManager::GetInstance().AddJob(job, NULL);
  CJobManager::GetInstance().CancelJob(id);
}

namespace
{
struct JobControlPackage
{
  JobControlPackage() :
    ready (false)
  {
    // We're not ready to wait yet
    jobCreatedMutex.lock();
  }

  ~JobControlPackage()
  {
    jobCreatedMutex.unlock();
  }

  bool ready;
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

  const char * GetType() const
  {
    return "BroadcastingJob";
  }

  bool DoWork()
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
