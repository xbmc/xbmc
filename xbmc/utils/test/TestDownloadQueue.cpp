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

#include "utils/DownloadQueue.h"
#include "threads/Thread.h"
#include "settings/GUISettings.h"
#include "test/TestUtils.h"

#include "gtest/gtest.h"

class CTestDownloadQueueThread : public CThread
{
public:
  CTestDownloadQueueThread() :
    CThread("CTestDownloadQueueThread"){}
};

/* Need to set some settings for network connectivity when an
 * http/https url is tested.
 */
class TestDownloadQueue : public testing::Test
{
protected:
  TestDownloadQueue()
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

  ~TestDownloadQueue()
  {
    g_guiSettings.Clear();
  }
};

TEST_F(TestDownloadQueue, RequestContent)
{
  IDownloadQueueObserver observer;
  CDownloadQueue queue;
  CTestDownloadQueueThread thread;
  unsigned int count;

  std::vector<CStdString> urls =
    CXBMCTestUtils::Instance().getTestDownloadQueueUrls();

  std::vector<CStdString>::iterator it;
  count = 0;
  for (it = urls.begin(); it < urls.end(); it++)
  {
    std::cout << "Testing URL: " << *it << std::endl;
    TICKET t = queue.RequestContent(*it, &observer);
    EXPECT_EQ(count, t.dwItemId);
    count++;
  }

  thread.Sleep(1000);
  queue.Flush();
  EXPECT_EQ(0, queue.Size());
}

TEST_F(TestDownloadQueue, RequestFile)
{
  IDownloadQueueObserver observer;
  CDownloadQueue queue;
  CTestDownloadQueueThread thread;
  unsigned int count;

  std::vector<CStdString> urls =
    CXBMCTestUtils::Instance().getTestDownloadQueueUrls();

  std::vector<CStdString>::iterator it;
  count = 0;
  for (it = urls.begin(); it < urls.end(); it++)
  {
    std::cout << "Testing URL: " << *it << std::endl;
    TICKET t = queue.RequestFile(*it, &observer);
    EXPECT_EQ(count, t.dwItemId);
    count++;
  }

  thread.Sleep(1000);
  queue.Flush();
  EXPECT_EQ(0, queue.Size());
}
