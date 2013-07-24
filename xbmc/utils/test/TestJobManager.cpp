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
    /* TODO
    CSettingsCategory* net = CSettings::Get().AddCategory(4, "network", 798);
    CSettings::Get().AddBool(net, "network.usehttpproxy", 708, false);
    CSettings::Get().AddString(net, "network.httpproxyserver", 706, "",
                            EDIT_CONTROL_INPUT);
    CSettings::Get().AddString(net, "network.httpproxyport", 730, "8080",
                            EDIT_CONTROL_NUMBER_INPUT, false, 707);
    CSettings::Get().AddString(net, "network.httpproxyusername", 1048, "",
                            EDIT_CONTROL_INPUT);
    CSettings::Get().AddString(net, "network.httpproxypassword", 733, "",
                            EDIT_CONTROL_HIDDEN_INPUT,true,733);
    CSettings::Get().AddInt(net, "network.bandwidth", 14041, 0, 0, 512, 100*1024,
                         SPIN_CONTROL_INT_PLUS, 14048, 351);
    */
  }

  ~TestJobManager()
  {
    CSettings::Get().Unload();
  }
};

TEST_F(TestJobManager, AddJob)
{
  CJob* job = new CSysInfoJob();
  CJobManager::GetInstance().AddJob(job, NULL);
  CJobManager::GetInstance().CancelJobs();
}

TEST_F(TestJobManager, CancelJob)
{
  unsigned int id;
  CJob* job = new CSysInfoJob();
  id = CJobManager::GetInstance().AddJob(job, NULL);
  CJobManager::GetInstance().CancelJob(id);
}

TEST_F(TestJobManager, Pause)
{
  CJob* job = new CSysInfoJob();
  CJobManager::GetInstance().AddJob(job, NULL, CJob::PRIORITY_NORMAL);

  EXPECT_FALSE(CJobManager::GetInstance().IsPaused(CJob::PRIORITY_NORMAL));
  CJobManager::GetInstance().Pause(CJob::PRIORITY_NORMAL);
  EXPECT_TRUE(CJobManager::GetInstance().IsPaused(CJob::PRIORITY_NORMAL));
  CJobManager::GetInstance().UnPause(CJob::PRIORITY_NORMAL);
  EXPECT_FALSE(CJobManager::GetInstance().IsPaused(CJob::PRIORITY_NORMAL));

  CJobManager::GetInstance().CancelJobs();
}

TEST_F(TestJobManager, IsProcessing)
{
  CJob* job = new CSysInfoJob();
  CJobManager::GetInstance().AddJob(job, NULL);

  EXPECT_EQ(0, CJobManager::GetInstance().IsProcessing(""));

  CJobManager::GetInstance().CancelJobs();
}
