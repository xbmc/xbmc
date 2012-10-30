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

#include "utils/DownloadQueueManager.h"
#include "threads/Thread.h"
#include "settings/GUISettings.h"
#include "test/TestUtils.h"

#include "gtest/gtest.h"

class CTestDownloadQueueManagerThread : public CThread
{
public:
  CTestDownloadQueueManagerThread() :
    CThread("CTestDownloadQueueManagerThread"){}
};

/* Need to set some settings for network connectivity when an
 * http/https url is tested.
 */
class TestDownloadQueueManager : public testing::Test
{
protected:
  TestDownloadQueueManager()
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

  ~TestDownloadQueueManager()
  {
    g_guiSettings.Clear();
  }
};

TEST_F(TestDownloadQueueManager, RequestContent)
{
  IDownloadQueueObserver observer;
  CTestDownloadQueueManagerThread thread;
  int count;

  std::vector<CStdString> urls =
    CXBMCTestUtils::Instance().getTestDownloadQueueUrls();

  std::vector<CStdString>::iterator it;
  count = 0;
  for (it = urls.begin(); it < urls.end(); it++)
  {
    std::cout << "Testing URL: " << *it << std::endl;
    TICKET t = g_DownloadManager.RequestContent(*it, &observer);
    std::cout << "  Ticket Item ID: " << t.dwItemId << std::endl;
    count++;
  }

  thread.Sleep(1000);
}

TEST_F(TestDownloadQueueManager, RequestFile)
{
  IDownloadQueueObserver observer;
  CTestDownloadQueueManagerThread thread;
  int count;

  std::vector<CStdString> urls =
    CXBMCTestUtils::Instance().getTestDownloadQueueUrls();

  std::vector<CStdString>::iterator it;
  count = 0;
  for (it = urls.begin(); it < urls.end(); it++)
  {
    std::cout << "Testing URL: " << *it << std::endl;
    TICKET t = g_DownloadManager.RequestFile(*it, &observer);
    std::cout << "  Ticket Item ID: " << t.dwItemId << std::endl;
    count++;
  }

  thread.Sleep(1000);
}
