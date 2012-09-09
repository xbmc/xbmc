/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "settings/GUISettings.h"
#include "utils/SystemInfo.h"

#include "gtest/gtest.h"

/* CSysInfoJob::GetInternetState() will test for network connectivity. */
class TestJobManager : public testing::Test
{
protected:
  TestJobManager()
  {
    CSettingsCategory* net = g_guiSettings.AddCategory(4, "network", 798);
    g_guiSettings.AddBool(net, "network.usehttpproxy", 708, false);
    g_guiSettings.AddString(net, "network.httpproxyserver", 706, "",
                            EDIT_CONTROL_INPUT);
    g_guiSettings.AddString(net, "network.httpproxyport", 730, "8080",
                            EDIT_CONTROL_NUMBER_INPUT, false, 707);
    g_guiSettings.AddString(net, "network.httpproxyusername", 1048, "",
                            EDIT_CONTROL_INPUT);
    g_guiSettings.AddString(net, "network.httpproxypassword", 733, "",
                            EDIT_CONTROL_HIDDEN_INPUT,true,733);
    g_guiSettings.AddInt(net, "network.bandwidth", 14041, 0, 0, 512, 100*1024,
                         SPIN_CONTROL_INT_PLUS, 14048, 351);
  }

  ~TestJobManager()
  {
    g_guiSettings.Clear();
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
  CJobManager::GetInstance().AddJob(job, NULL);

  EXPECT_FALSE(CJobManager::GetInstance().IsPaused(""));
  CJobManager::GetInstance().Pause("");
  EXPECT_TRUE(CJobManager::GetInstance().IsPaused(""));
  CJobManager::GetInstance().UnPause("");
  EXPECT_FALSE(CJobManager::GetInstance().IsPaused(""));

  CJobManager::GetInstance().CancelJobs();
}

TEST_F(TestJobManager, IsProcessing)
{
  CJob* job = new CSysInfoJob();
  CJobManager::GetInstance().AddJob(job, NULL);

  EXPECT_EQ(0, CJobManager::GetInstance().IsProcessing(""));

  CJobManager::GetInstance().CancelJobs();
}
