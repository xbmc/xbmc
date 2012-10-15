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

#include "utils/SystemInfo.h"
#include "settings/GUISettings.h"
#include "GUIInfoManager.h"

#include "gtest/gtest.h"

class TestSystemInfo : public testing::Test
{
protected:
  TestSystemInfo()
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
  ~TestSystemInfo()
  {
    g_guiSettings.Clear();
  }
};

#if defined(_LINUX) && !defined(TARGET_DARWIN) && !defined(__FreeBSD__)
TEST_F(TestSystemInfo, GetLinuxDistro)
{
  std::cout << "GetLinuxDistro(): " << g_sysinfo.GetLinuxDistro() << std::endl;
}
#endif

#ifdef _LINUX
TEST_F(TestSystemInfo, GetUnameVersion)
{
  std::cout << "GetUnameVersion(): " << g_sysinfo.GetUnameVersion() << std::endl;
}
#endif

TEST_F(TestSystemInfo, GetUserAgent)
{
  std::cout << "GetUserAgent(): " << g_sysinfo.GetUserAgent() << std::endl;
}

TEST_F(TestSystemInfo, HasInternet)
{
  std::cout << "HasInternet(): " <<
    testing::PrintToString(g_sysinfo.HasInternet()) << std::endl;
}

TEST_F(TestSystemInfo, IsAppleTV2)
{
  std::cout << "IsAppleTV2(): " <<
    testing::PrintToString(g_sysinfo.IsAppleTV2()) << std::endl;
}

TEST_F(TestSystemInfo, HasVDADecoder)
{
  std::cout << "HasVDADecoder(): " <<
    testing::PrintToString(g_sysinfo.HasVDADecoder()) << std::endl;
}

TEST_F(TestSystemInfo, HasVideoToolBoxDecoder)
{
  std::cout << "HasVideoToolBoxDecoder(): " <<
    testing::PrintToString(g_sysinfo.HasVideoToolBoxDecoder()) << std::endl;
}

TEST_F(TestSystemInfo, IsAeroDisabled)
{
  std::cout << "IsAeroDisabled(): " <<
    testing::PrintToString(g_sysinfo.IsAeroDisabled()) << std::endl;
}

TEST_F(TestSystemInfo, IsVistaOrHigher)
{
  std::cout << "IsVistaOrHigher(): " <<
    testing::PrintToString(g_sysinfo.IsVistaOrHigher()) << std::endl;
}

TEST_F(TestSystemInfo, GetKernelVersion)
{
  std::cout << "GetKernelVersion(): " << CSysInfo::GetKernelVersion() << std::endl;
}

TEST_F(TestSystemInfo, GetCPUModel)
{
  std::cout << "GetCPUModel(): " << g_sysinfo.GetCPUModel() <<  std::endl;
}

TEST_F(TestSystemInfo, GetCPUBogoMips)
{
  std::cout << "GetCPUBogoMips(): " << g_sysinfo.GetCPUBogoMips() <<  std::endl;
}

TEST_F(TestSystemInfo, GetCPUHardware)
{
  std::cout << "GetCPUHardware(): " << g_sysinfo.GetCPUHardware() <<  std::endl;
}

TEST_F(TestSystemInfo, GetCPURevision)
{
  std::cout << "GetCPURevision(): " << g_sysinfo.GetCPURevision() <<  std::endl;
}

TEST_F(TestSystemInfo, GetCPUSerial)
{
  std::cout << "GetCPUSerial(): " << g_sysinfo.GetCPUSerial() <<  std::endl;
}

TEST_F(TestSystemInfo, GetDiskSpace)
{
  int iTotal, iTotalFree, iTotalUsed, iPercentFree, iPercentUsed;

  iTotal = iTotalFree = iTotalUsed = iPercentFree = iPercentUsed = 0;

  std::cout << "GetDiskSpace(): " <<
    testing::PrintToString(g_sysinfo.GetDiskSpace("*", iTotal, iTotalFree,
                                                  iTotalUsed, iPercentFree,
                                                  iPercentUsed)) << std::endl;
  std::cout << "iTotal: " << testing::PrintToString(iTotal) << std::endl;
  std::cout << "iTotalFree: " << testing::PrintToString(iTotalFree) << std::endl;
  std::cout << "iTotalUsed: " << testing::PrintToString(iTotalUsed) << std::endl;
  std::cout << "iPercentFree: " << testing::PrintToString(iPercentFree) << std::endl;
  std::cout << "iPercentUsed: " << testing::PrintToString(iPercentUsed) << std::endl;
}

TEST_F(TestSystemInfo, GetHddSpaceInfo)
{
  int percent;

  std::cout << "GetHddSpaceInfo(SYSTEM_FREE_SPACE): " <<
    g_sysinfo.GetHddSpaceInfo(SYSTEM_FREE_SPACE) << std::endl;
  std::cout << "GetHddSpaceInfo(SYSTEM_USED_SPACE): " <<
    g_sysinfo.GetHddSpaceInfo(SYSTEM_USED_SPACE) << std::endl;
  std::cout << "GetHddSpaceInfo(SYSTEM_TOTAL_SPACE): " <<
    g_sysinfo.GetHddSpaceInfo(SYSTEM_TOTAL_SPACE) << std::endl;
  std::cout << "GetHddSpaceInfo(SYSTEM_FREE_SPACE_PERCENT): " <<
    g_sysinfo.GetHddSpaceInfo(SYSTEM_FREE_SPACE_PERCENT) << std::endl;
  std::cout << "GetHddSpaceInfo(SYSTEM_USED_SPACE_PERCENT): " <<
    g_sysinfo.GetHddSpaceInfo(SYSTEM_USED_SPACE_PERCENT) << std::endl;

  percent = 0;
  std::cout << "GetHddSpaceInfo(percent, SYSTEM_FREE_SPACE, true): " <<
    g_sysinfo.GetHddSpaceInfo(percent, SYSTEM_FREE_SPACE, true) << std::endl;
  std::cout << "percent: " << testing::PrintToString(percent) << std::endl;
  percent = 0;
  std::cout << "GetHddSpaceInfo(percent, SYSTEM_USED_SPACE, true): " <<
    g_sysinfo.GetHddSpaceInfo(percent, SYSTEM_USED_SPACE, true) << std::endl;
  std::cout << "percent: " << testing::PrintToString(percent) << std::endl;
}
