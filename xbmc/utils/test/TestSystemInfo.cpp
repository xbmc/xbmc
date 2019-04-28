/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/SystemInfo.h"
#include "settings/Settings.h"
#include "GUIInfoManager.h"
#if defined(TARGET_WINDOWS)
#include "platform/win32/CharsetConverter.h"
#endif

#include "gtest/gtest.h"

class TestSystemInfo : public testing::Test
{
protected:
  TestSystemInfo()
  = default;
  ~TestSystemInfo() override
  = default;
};

TEST_F(TestSystemInfo, Print_System_Info)
{
  std::cout << "'GetKernelName(false)': \"" << g_sysinfo.GetKernelName(true) << "\"\n";
  std::cout << "'GetKernelVersion()': \"" << g_sysinfo.GetKernelVersion() << "\"\n";
  std::cout << "'GetKernelVersionFull()': \"" << g_sysinfo.GetKernelVersionFull() << "\"\n";
  std::cout << "'GetOsPrettyNameWithVersion()': \"" << g_sysinfo.GetOsPrettyNameWithVersion() << "\"\n";
  std::cout << "'GetOsName(false)': \"" << g_sysinfo.GetOsName(false) << "\"\n";
  std::cout << "'GetOsVersion()': \"" << g_sysinfo.GetOsVersion() << "\"\n";
  std::cout << "'GetKernelCpuFamily()': \"" << g_sysinfo.GetKernelCpuFamily() << "\"\n";
  std::cout << "'GetKernelBitness()': \"" << g_sysinfo.GetKernelBitness() << "\"\n";
  std::cout << "'GetBuildTargetPlatformName()': \"" << g_sysinfo.GetBuildTargetPlatformName() << "\"\n";
  std::cout << "'GetBuildTargetPlatformVersionDecoded()': \"" << g_sysinfo.GetBuildTargetPlatformVersionDecoded() << "\"\n";
  std::cout << "'GetBuildTargetPlatformVersion()': \"" << g_sysinfo.GetBuildTargetPlatformVersion() << "\"\n";
  std::cout << "'GetBuildTargetCpuFamily()': \"" << g_sysinfo.GetBuildTargetCpuFamily() << "\"\n";
  std::cout << "'GetXbmcBitness()': \"" << g_sysinfo.GetXbmcBitness() << "\"\n";
  std::cout << "'GetUsedCompilerNameAndVer()': \"" << g_sysinfo.GetUsedCompilerNameAndVer() << "\"\n";
  std::cout << "'GetManufacturerName()': \"" << g_sysinfo.GetManufacturerName() << "\"\n";
  std::cout << "'GetModelName()': \"" << g_sysinfo.GetModelName() << "\"\n";
  std::cout << "'GetUserAgent()': \"" << g_sysinfo.GetUserAgent() << "\"\n";
}

TEST_F(TestSystemInfo, GetKernelName)
{
  EXPECT_FALSE(g_sysinfo.GetKernelName(true).empty()) << "'GetKernelName(true)' must not return empty kernel name";
  EXPECT_FALSE(g_sysinfo.GetKernelName(false).empty()) << "'GetKernelName(false)' must not return empty kernel name";
  EXPECT_STRCASENE("Unknown kernel", g_sysinfo.GetKernelName(true).c_str()) << "'GetKernelName(true)' must not return 'Unknown kernel'";
  EXPECT_STRCASENE("Unknown kernel", g_sysinfo.GetKernelName(false).c_str()) << "'GetKernelName(false)' must not return 'Unknown kernel'";
#ifndef TARGET_DARWIN
  EXPECT_EQ(g_sysinfo.GetBuildTargetPlatformName(), g_sysinfo.GetKernelName(true)) << "'GetKernelName(true)' must match GetBuildTargetPlatformName()";
  EXPECT_EQ(g_sysinfo.GetBuildTargetPlatformName(), g_sysinfo.GetKernelName(false)) << "'GetKernelName(false)' must match GetBuildTargetPlatformName()";
#endif // !TARGET_DARWIN
#if defined(TARGET_WINDOWS)
  EXPECT_NE(std::string::npos, g_sysinfo.GetKernelName(true).find("Windows")) << "'GetKernelName(true)' must contain 'Windows'";
  EXPECT_NE(std::string::npos, g_sysinfo.GetKernelName(false).find("Windows")) << "'GetKernelName(false)' must contain 'Windows'";
#elif defined(TARGET_FREEBSD)
  EXPECT_STREQ("FreeBSD", g_sysinfo.GetKernelName(true).c_str()) << "'GetKernelName(true)' must return 'FreeBSD'";
  EXPECT_STREQ("FreeBSD", g_sysinfo.GetKernelName(false).c_str()) << "'GetKernelName(false)' must return 'FreeBSD'";
#elif defined(TARGET_DARWIN)
  EXPECT_STREQ("Darwin", g_sysinfo.GetKernelName(true).c_str()) << "'GetKernelName(true)' must return 'Darwin'";
  EXPECT_STREQ("Darwin", g_sysinfo.GetKernelName(false).c_str()) << "'GetKernelName(false)' must return 'Darwin'";
#elif defined(TARGET_LINUX)
  EXPECT_STREQ("Linux", g_sysinfo.GetKernelName(true).c_str()) << "'GetKernelName(true)' must return 'Linux'";
  EXPECT_STREQ("Linux", g_sysinfo.GetKernelName(false).c_str()) << "'GetKernelName(false)' must return 'Linux'";
#endif
}

TEST_F(TestSystemInfo, GetKernelVersionFull)
{
  EXPECT_FALSE(g_sysinfo.GetKernelVersionFull().empty()) << "'GetKernelVersionFull()' must not return empty string";
  EXPECT_STRNE("0.0.0", g_sysinfo.GetKernelVersionFull().c_str()) << "'GetKernelVersionFull()' must not return '0.0.0'";
  EXPECT_STRNE("0.0", g_sysinfo.GetKernelVersionFull().c_str()) << "'GetKernelVersionFull()' must not return '0.0'";
  EXPECT_EQ(0U, g_sysinfo.GetKernelVersionFull().find_first_of("0123456789")) << "'GetKernelVersionFull()' must not return version not starting from digit";
}

TEST_F(TestSystemInfo, GetKernelVersion)
{
  EXPECT_FALSE(g_sysinfo.GetKernelVersion().empty()) << "'GetKernelVersion()' must not return empty string";
  EXPECT_STRNE("0.0.0", g_sysinfo.GetKernelVersion().c_str()) << "'GetKernelVersion()' must not return '0.0.0'";
  EXPECT_STRNE("0.0", g_sysinfo.GetKernelVersion().c_str()) << "'GetKernelVersion()' must not return '0.0'";
  EXPECT_EQ(0U, g_sysinfo.GetKernelVersion().find_first_of("0123456789")) << "'GetKernelVersion()' must not return version not starting from digit";
  EXPECT_EQ(std::string::npos, g_sysinfo.GetKernelVersion().find_first_not_of("0123456789.")) << "'GetKernelVersion()' must not return version with not only digits and dots";
}

TEST_F(TestSystemInfo, GetOsName)
{
  EXPECT_FALSE(g_sysinfo.GetOsName(true).empty()) << "'GetOsName(true)' must not return empty OS name";
  EXPECT_FALSE(g_sysinfo.GetOsName(false).empty()) << "'GetOsName(false)' must not return empty OS name";
  EXPECT_STRCASENE("Unknown OS", g_sysinfo.GetOsName(true).c_str()) << "'GetOsName(true)' must not return 'Unknown OS'";
  EXPECT_STRCASENE("Unknown OS", g_sysinfo.GetOsName(false).c_str()) << "'GetOsName(false)' must not return 'Unknown OS'";
#if defined(TARGET_WINDOWS)
  EXPECT_NE(std::string::npos, g_sysinfo.GetOsName(true).find("Windows")) << "'GetOsName(true)' must contain 'Windows'";
  EXPECT_NE(std::string::npos, g_sysinfo.GetOsName(false).find("Windows")) << "'GetOsName(false)' must contain 'Windows'";
#elif defined(TARGET_FREEBSD)
  EXPECT_STREQ("FreeBSD", g_sysinfo.GetOsName(true).c_str()) << "'GetOsName(true)' must return 'FreeBSD'";
  EXPECT_STREQ("FreeBSD", g_sysinfo.GetOsName(false).c_str()) << "'GetOsName(false)' must return 'FreeBSD'";
#elif defined(TARGET_DARWIN_IOS)
  EXPECT_STREQ("iOS", g_sysinfo.GetOsName(true).c_str()) << "'GetOsName(true)' must return 'iOS'";
  EXPECT_STREQ("iOS", g_sysinfo.GetOsName(false).c_str()) << "'GetOsName(false)' must return 'iOS'";
#elif defined(TARGET_DARWIN_TVOS)
  EXPECT_STREQ("tvOS", g_sysinfo.GetOsName(true).c_str()) << "'GetOsName(true)' must return 'tvOS'";
  EXPECT_STREQ("tvOS", g_sysinfo.GetOsName(false).c_str()) << "'GetOsName(false)' must return 'tvOS'";
#elif defined(TARGET_DARWIN_OSX)
  EXPECT_STREQ("OS X", g_sysinfo.GetOsName(true).c_str()) << "'GetOsName(true)' must return 'OS X'";
  EXPECT_STREQ("OS X", g_sysinfo.GetOsName(false).c_str()) << "'GetOsName(false)' must return 'OS X'";
#elif defined(TARGET_ANDROID)
  EXPECT_STREQ("Android", g_sysinfo.GetOsName(true).c_str()) << "'GetOsName(true)' must return 'Android'";
  EXPECT_STREQ("Android", g_sysinfo.GetOsName(false).c_str()) << "'GetOsName(false)' must return 'Android'";
#endif
#ifdef TARGET_DARWIN
  EXPECT_EQ(g_sysinfo.GetBuildTargetPlatformName(), g_sysinfo.GetOsName(true)) << "'GetOsName(true)' must match GetBuildTargetPlatformName()";
  EXPECT_EQ(g_sysinfo.GetBuildTargetPlatformName(), g_sysinfo.GetOsName(false)) << "'GetOsName(false)' must match GetBuildTargetPlatformName()";
#endif // TARGET_DARWIN
}

TEST_F(TestSystemInfo, DISABLED_GetOsVersion)
{
  EXPECT_FALSE(g_sysinfo.GetOsVersion().empty()) << "'GetOsVersion()' must not return empty string";
  EXPECT_STRNE("0.0.0", g_sysinfo.GetOsVersion().c_str()) << "'GetOsVersion()' must not return '0.0.0'";
  EXPECT_STRNE("0.0", g_sysinfo.GetOsVersion().c_str()) << "'GetOsVersion()' must not return '0.0'";
  EXPECT_EQ(0U, g_sysinfo.GetOsVersion().find_first_of("0123456789")) << "'GetOsVersion()' must not return version not starting from digit";
  EXPECT_EQ(std::string::npos, g_sysinfo.GetOsVersion().find_first_not_of("0123456789.")) << "'GetOsVersion()' must not return version with not only digits and dots";
}

TEST_F(TestSystemInfo, GetOsPrettyNameWithVersion)
{
  EXPECT_FALSE(g_sysinfo.GetOsPrettyNameWithVersion().empty()) << "'GetOsPrettyNameWithVersion()' must not return empty string";
  EXPECT_EQ(std::string::npos, g_sysinfo.GetOsPrettyNameWithVersion().find("Unknown")) << "'GetOsPrettyNameWithVersion()' must not contain 'Unknown'";
  EXPECT_EQ(std::string::npos, g_sysinfo.GetOsPrettyNameWithVersion().find("unknown")) << "'GetOsPrettyNameWithVersion()' must not contain 'unknown'";
#ifdef TARGET_WINDOWS
  EXPECT_NE(std::string::npos, g_sysinfo.GetOsPrettyNameWithVersion().find("Windows")) << "'GetOsPrettyNameWithVersion()' must contain 'Windows'";
#else  // ! TARGET_WINDOWS
  EXPECT_NE(std::string::npos, g_sysinfo.GetOsPrettyNameWithVersion().find(g_sysinfo.GetOsVersion())) << "'GetOsPrettyNameWithVersion()' must contain OS version";
#endif // ! TARGET_WINDOWS
}

TEST_F(TestSystemInfo, GetManufacturerName)
{
  EXPECT_STRCASENE("unknown", g_sysinfo.GetManufacturerName().c_str()) << "'GetManufacturerName()' must return empty string instead of 'Unknown'";
}

TEST_F(TestSystemInfo, GetModelName)
{
  EXPECT_STRCASENE("unknown", g_sysinfo.GetModelName().c_str()) << "'GetModelName()' must return empty string instead of 'Unknown'";
}

#ifndef TARGET_WINDOWS
TEST_F(TestSystemInfo, IsAeroDisabled)
{
  EXPECT_FALSE(g_sysinfo.IsAeroDisabled()) << "'IsAeroDisabled()' must return 'false'";
}
#endif // ! TARGET_WINDOWS

TEST_F(TestSystemInfo, IsWindowsVersion)
{
  EXPECT_FALSE(g_sysinfo.IsWindowsVersion(CSysInfo::WindowsVersionUnknown)) << "'IsWindowsVersion()' must return 'false' for 'WindowsVersionUnknown'";
#ifndef TARGET_WINDOWS
  EXPECT_FALSE(g_sysinfo.IsWindowsVersion(CSysInfo::WindowsVersionWin7)) << "'IsWindowsVersion()' must return 'false'";
#endif // ! TARGET_WINDOWS
}

TEST_F(TestSystemInfo, IsWindowsVersionAtLeast)
{
  EXPECT_FALSE(g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionUnknown)) << "'IsWindowsVersionAtLeast()' must return 'false' for 'WindowsVersionUnknown'";
  EXPECT_FALSE(g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionFuture)) << "'IsWindowsVersionAtLeast()' must return 'false' for 'WindowsVersionFuture'";
#ifndef TARGET_WINDOWS
  EXPECT_FALSE(g_sysinfo.IsWindowsVersion(CSysInfo::WindowsVersionWin7)) << "'IsWindowsVersionAtLeast()' must return 'false'";
#endif // ! TARGET_WINDOWS
}

TEST_F(TestSystemInfo, GetWindowsVersion)
{
#ifdef TARGET_WINDOWS
  EXPECT_NE(CSysInfo::WindowsVersionUnknown, g_sysinfo.GetWindowsVersion()) << "'GetWindowsVersion()' must not return 'WindowsVersionUnknown'";
  EXPECT_NE(CSysInfo::WindowsVersionFuture, g_sysinfo.GetWindowsVersion()) << "'GetWindowsVersion()' must not return 'WindowsVersionFuture'";
#else  // ! TARGET_WINDOWS
  EXPECT_EQ(CSysInfo::WindowsVersionUnknown, g_sysinfo.GetWindowsVersion()) << "'GetWindowsVersion()' must return 'WindowsVersionUnknown'";
#endif // ! TARGET_WINDOWS
}

TEST_F(TestSystemInfo, GetKernelBitness)
{
  EXPECT_TRUE(g_sysinfo.GetKernelBitness() == 32 || g_sysinfo.GetKernelBitness() == 64) << "'GetKernelBitness()' must return '32' or '64', but not '" << g_sysinfo.GetKernelBitness() << "'";
  EXPECT_LE(g_sysinfo.GetXbmcBitness(), g_sysinfo.GetKernelBitness()) << "'GetKernelBitness()' must be greater or equal to 'GetXbmcBitness()'";
}

TEST_F(TestSystemInfo, GetKernelCpuFamily)
{
  EXPECT_STRNE("unknown CPU family", g_sysinfo.GetKernelCpuFamily().c_str()) << "'GetKernelCpuFamily()' must not return 'unknown CPU family'";
#if defined(__thumb__) || defined(_M_ARMT) || defined(__arm__) || defined(_M_ARM) || defined (__aarch64__)
  EXPECT_STREQ("ARM", g_sysinfo.GetKernelCpuFamily().c_str()) << "'GetKernelCpuFamily()' must return 'ARM'";
#else  // ! ARM
  EXPECT_EQ(g_sysinfo.GetBuildTargetCpuFamily(), g_sysinfo.GetKernelCpuFamily()) << "'GetKernelCpuFamily()' must match 'GetBuildTargetCpuFamily()'";
#endif // ! ARM
}

TEST_F(TestSystemInfo, GetXbmcBitness)
{
  EXPECT_TRUE(g_sysinfo.GetXbmcBitness() == 32 || g_sysinfo.GetXbmcBitness() == 64) << "'GetXbmcBitness()' must return '32' or '64', but not '" << g_sysinfo.GetXbmcBitness() << "'";
  EXPECT_GE(g_sysinfo.GetKernelBitness(), g_sysinfo.GetXbmcBitness()) << "'GetXbmcBitness()' must be not greater than 'GetKernelBitness()'";
}

TEST_F(TestSystemInfo, GetUserAgent)
{
  EXPECT_STREQ(g_sysinfo.GetAppName().c_str(), g_sysinfo.GetUserAgent().substr(0, g_sysinfo.GetAppName().size()).c_str()) << "'GetUserAgent()' string must start with app name'";
  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find('(')) << "'GetUserAgent()' must contain brackets around second parameter";
  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find(')')) << "'GetUserAgent()' must contain brackets around second parameter";
  EXPECT_EQ(g_sysinfo.GetUserAgent().find(' '), g_sysinfo.GetUserAgent().find(" (")) << "Second parameter in 'GetUserAgent()' string must be in brackets";
  EXPECT_EQ(g_sysinfo.GetUserAgent().find(" (") + 1, g_sysinfo.GetUserAgent().find('(')) << "'GetUserAgent()' string must not contain any opening brackets before second parameter";
  EXPECT_GT(g_sysinfo.GetUserAgent().find(')'), g_sysinfo.GetUserAgent().find('(')) << "'GetUserAgent()' string must not contain any closing brackets before second parameter";
  EXPECT_EQ(g_sysinfo.GetUserAgent().find(") "), g_sysinfo.GetUserAgent().find(')')) << "'GetUserAgent()' string must not contain any closing brackets before end of second parameter";
#if defined(TARGET_WINDOWS)
  EXPECT_EQ(g_sysinfo.GetUserAgent().find('('), g_sysinfo.GetUserAgent().find("(Windows")) << "Second parameter in 'GetUserAgent()' string must start from `Windows`";
  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find("Windows")) << "'GetUserAgent()' must contain 'Windows'";
#elif defined(TARGET_DARWIN_IOS)
  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find("like Mac OS X")) << "'GetUserAgent()' must contain ' like Mac OS X'";
  EXPECT_TRUE(g_sysinfo.GetUserAgent().find("CPU OS ") != std::string::npos || g_sysinfo.GetUserAgent().find("CPU iPhone OS ") != std::string::npos) << "'GetUserAgent()' must contain 'CPU OS ' or 'CPU iPhone OS '";
#elif defined(TARGET_DARWIN_TVOS)
  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find("like Mac OS X")) << "'GetUserAgent()' must contain ' like Mac OS X'";
  EXPECT_TRUE(g_sysinfo.GetUserAgent().find("CPU TVOS ") != std::string::npos) << "'GetUserAgent()' must contain 'CPU TVOS '";
#elif defined(TARGET_DARWIN_OSX)
  EXPECT_EQ(g_sysinfo.GetUserAgent().find('('), g_sysinfo.GetUserAgent().find("(Macintosh; ")) << "Second parameter in 'GetUserAgent()' string must start from 'Macintosh; '";
#elif defined(TARGET_ANDROID)
  EXPECT_EQ(g_sysinfo.GetUserAgent().find('('), g_sysinfo.GetUserAgent().find("(Linux; Android ")) << "Second parameter in 'GetUserAgent()' string must start from 'Linux; Android '";
#elif defined(TARGET_POSIX)
  EXPECT_EQ(g_sysinfo.GetUserAgent().find('('), g_sysinfo.GetUserAgent().find("(X11; ")) << "Second parameter in 'GetUserAgent()' string must start from 'X11; '";
#if defined(TARGET_FREEBSD)
  EXPECT_EQ(g_sysinfo.GetUserAgent().find('('), g_sysinfo.GetUserAgent().find("(X11; FreeBSD ")) << "Second parameter in 'GetUserAgent()' string must start from 'X11; FreeBSD '";
#elif defined(TARGET_LINUX)
  EXPECT_EQ(g_sysinfo.GetUserAgent().find('('), g_sysinfo.GetUserAgent().find("(X11; Linux ")) << "Second parameter in 'GetUserAgent()' string must start from 'X11; Linux '";
#endif // defined(TARGET_LINUX)
#endif // defined(TARGET_POSIX)

#ifdef TARGET_RASPBERRY_PI
  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find(" XBMC_HW_RaspberryPi/")) << "'GetUserAgent()' must contain ' XBMC_HW_RaspberryPi/'";
#endif // TARGET_RASPBERRY_PI

  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find(" App_Bitness/")) << "'GetUserAgent()' must contain ' App_Bitness/'";
  EXPECT_NE(std::string::npos, g_sysinfo.GetUserAgent().find(" Version/")) << "'GetUserAgent()' must contain ' Version/'";
}

#ifndef TARGET_DARWIN
TEST_F(TestSystemInfo, HasVideoToolBoxDecoder)
{
  EXPECT_FALSE(g_sysinfo.HasVideoToolBoxDecoder()) << "'HasVideoToolBoxDecoder()' must return 'false'";
}
#endif

TEST_F(TestSystemInfo, GetBuildTargetPlatformName)
{
  EXPECT_EQ(std::string::npos, g_sysinfo.GetBuildTargetPlatformName().find("Unknown")) << "'GetBuildTargetPlatformName()' must not contain 'Unknown', actual value: '" << g_sysinfo.GetBuildTargetPlatformName() << "'";
  EXPECT_EQ(std::string::npos, g_sysinfo.GetBuildTargetPlatformName().find("unknown")) << "'GetBuildTargetPlatformName()' must not contain 'unknown', actual value: '" << g_sysinfo.GetBuildTargetPlatformName() << "'";
}

TEST_F(TestSystemInfo, GetBuildTargetPlatformVersion)
{
  EXPECT_EQ(std::string::npos, g_sysinfo.GetBuildTargetPlatformVersion().find("Unknown")) << "'GetBuildTargetPlatformVersion()' must not contain 'Unknown', actual value: '" << g_sysinfo.GetBuildTargetPlatformVersion() << "'";
  EXPECT_EQ(std::string::npos, g_sysinfo.GetBuildTargetPlatformVersion().find("unknown")) << "'GetBuildTargetPlatformVersion()' must not contain 'unknown', actual value: '" << g_sysinfo.GetBuildTargetPlatformVersion() << "'";
}

TEST_F(TestSystemInfo, GetBuildTargetPlatformVersionDecoded)
{
  EXPECT_EQ(std::string::npos, g_sysinfo.GetBuildTargetPlatformVersionDecoded().find("Unknown")) << "'GetBuildTargetPlatformVersionDecoded()' must not contain 'Unknown', actual value: '" << g_sysinfo.GetBuildTargetPlatformVersion() << "'";
  EXPECT_EQ(std::string::npos, g_sysinfo.GetBuildTargetPlatformVersionDecoded().find("unknown")) << "'GetBuildTargetPlatformVersionDecoded()' must not contain 'unknown', actual value: '" << g_sysinfo.GetBuildTargetPlatformVersion() << "'";
#ifdef TARGET_ANDROID
  EXPECT_STREQ("API level ", g_sysinfo.GetBuildTargetPlatformVersionDecoded().substr(0, 10).c_str()) << "'GetBuildTargetPlatformVersionDecoded()' must start from 'API level '";
#else
  EXPECT_STREQ("version ", g_sysinfo.GetBuildTargetPlatformVersionDecoded().substr(0, 8).c_str()) << "'GetBuildTargetPlatformVersionDecoded()' must start from 'version'";
#endif
}

TEST_F(TestSystemInfo, GetBuildTargetCpuFamily)
{
  EXPECT_STRNE("unknown CPU family", g_sysinfo.GetBuildTargetCpuFamily().c_str()) << "'GetBuildTargetCpuFamily()' must not return 'unknown CPU family'";
#if defined(__thumb__) || defined(_M_ARMT) || defined(__arm__) || defined(_M_ARM) || defined (__aarch64__)
  EXPECT_STREQ("ARM", g_sysinfo.GetBuildTargetCpuFamily().substr(0, 3).c_str()) << "'GetKernelCpuFamily()' string must start from 'ARM'";
#else  // ! ARM
  EXPECT_EQ(g_sysinfo.GetKernelCpuFamily(), g_sysinfo.GetBuildTargetCpuFamily()) << "'GetBuildTargetCpuFamily()' must match 'GetKernelCpuFamily()'";
#endif // ! ARM
}

TEST_F(TestSystemInfo, GetUsedCompilerNameAndVer)
{
  EXPECT_STRNE("unknown compiler", g_sysinfo.GetUsedCompilerNameAndVer().c_str()) << "'GetUsedCompilerNameAndVer()' must not return 'unknown compiler'";
}

TEST_F(TestSystemInfo, GetDiskSpace)
{
  int iTotal, iTotalFree, iTotalUsed, iPercentFree, iPercentUsed;

  iTotal = iTotalFree = iTotalUsed = iPercentFree = iPercentUsed = 0;
  EXPECT_TRUE(g_sysinfo.GetDiskSpace("*", iTotal, iTotalFree, iTotalUsed, iPercentFree, iPercentUsed)) << "'GetDiskSpace()' return 'false' for disk '*'";
  EXPECT_NE(0, iTotal) << "'GetDiskSpace()' return zero total space for disk '*'";
  EXPECT_EQ(iTotal, iTotalFree + iTotalUsed) << "'GetDiskSpace()' return 'TotalFree + TotalUsed' not equal to 'Total' for disk '*'";
  EXPECT_EQ(100, iPercentFree + iPercentUsed) << "'GetDiskSpace()' return 'PercentFree + PercentUsed' not equal to '100' for disk '*'";

  iTotal = iTotalFree = iTotalUsed = iPercentFree = iPercentUsed = 0;
  EXPECT_TRUE(g_sysinfo.GetDiskSpace("", iTotal, iTotalFree, iTotalUsed, iPercentFree, iPercentUsed)) << "'GetDiskSpace()' return 'false' for disk ''";
  EXPECT_NE(0, iTotal) << "'GetDiskSpace()' return zero total space for disk ''";
  EXPECT_EQ(iTotal, iTotalFree + iTotalUsed) << "'GetDiskSpace()' return 'TotalFree + TotalUsed' not equal to 'Total' for disk ''";
  EXPECT_EQ(100, iPercentFree + iPercentUsed) << "'GetDiskSpace()' return 'PercentFree + PercentUsed' not equal to '100' for disk ''";

#ifdef TARGET_WINDOWS
  using KODI::PLATFORM::WINDOWS::FromW;
  wchar_t sysDrive[300];
  DWORD res = GetEnvironmentVariableW(L"SystemDrive", sysDrive, sizeof(sysDrive) / sizeof(wchar_t));
  std::string sysDriveLtr;
  if (res != 0 && res <= sizeof(sysDrive) / sizeof(wchar_t))
    sysDriveLtr.assign(FromW(sysDrive), 0, 1);
  else
    sysDriveLtr = "C"; // fallback

  iTotal = iTotalFree = iTotalUsed = iPercentFree = iPercentUsed = 0;
  EXPECT_TRUE(g_sysinfo.GetDiskSpace(sysDriveLtr, iTotal, iTotalFree, iTotalUsed, iPercentFree, iPercentUsed)) << "'GetDiskSpace()' return 'false' for disk '" << sysDriveLtr << ":'";
  EXPECT_NE(0, iTotal) << "'GetDiskSpace()' return zero total space for disk '" << sysDriveLtr << ":'";
  EXPECT_EQ(iTotal, iTotalFree + iTotalUsed) << "'GetDiskSpace()' return 'TotalFree + TotalUsed' not equal to 'Total' for disk '" << sysDriveLtr << ":'";
  EXPECT_EQ(100, iPercentFree + iPercentUsed) << "'GetDiskSpace()' return 'PercentFree + PercentUsed' not equal to '100' for disk '" << sysDriveLtr << ":'";
#elif defined(TARGET_POSIX)
  iTotal = iTotalFree = iTotalUsed = iPercentFree = iPercentUsed = 0;
  EXPECT_TRUE(g_sysinfo.GetDiskSpace("/", iTotal, iTotalFree, iTotalUsed, iPercentFree, iPercentUsed)) << "'GetDiskSpace()' return 'false' for directory '/'";
  EXPECT_NE(0, iTotal) << "'GetDiskSpace()' return zero total space for directory '/'";
  EXPECT_EQ(iTotal, iTotalFree + iTotalUsed) << "'GetDiskSpace()' return 'TotalFree + TotalUsed' not equal to 'Total' for directory '/'";
  EXPECT_EQ(100, iPercentFree + iPercentUsed) << "'GetDiskSpace()' return 'PercentFree + PercentUsed' not equal to '100' for directory '/'";
#endif
}
