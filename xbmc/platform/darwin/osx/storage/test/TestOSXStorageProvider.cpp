/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"

#include "platform/darwin/osx/storage/OSXStorageProvider.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <sys/mount.h>

class TestOSXStorageProvider : public testing::Test
{
protected:
  void SetUp() override
  {
    const char* home = std::getenv("HOME");
    ASSERT_NE(nullptr, home);
    m_home = home;
    ASSERT_TRUE(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Load(
        g_langInfo.GetLanguagePath(), "resource.language.en_gb"));
    COSXStorageProvider provider;
    provider.GetLocalDrives(m_drives);
  }

  void TearDown() override { CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Clear(); }

  std::vector<CMediaSource> m_drives;
  std::string m_home;
};

TEST_F(TestOSXStorageProvider, LocalDrivesHaveNames)
{
  ASSERT_FALSE(m_drives.empty());
  for (const auto& drive : m_drives)
    EXPECT_FALSE(drive.strName.empty()) << drive.strPath;
}

TEST_F(TestOSXStorageProvider, LocalDrivesExcludeNonBrowsableMounts)
{
  for (const auto& drive : m_drives)
  {
    if (drive.strPath == m_home || drive.strPath == m_home + "/Desktop" ||
        drive.strPath == "/Volumes")
      continue;

    struct statfs mountInfo;
    ASSERT_EQ(0, statfs(drive.strPath.c_str(), &mountInfo)) << drive.strPath;
    EXPECT_EQ(drive.strPath, mountInfo.f_mntonname);
    EXPECT_EQ(0u, mountInfo.f_flags & MNT_DONTBROWSE) << drive.strPath;
  }
}

TEST_F(TestOSXStorageProvider, LocalDrivesIncludeStandardLocations)
{
  const std::vector<std::string> paths{m_home, m_home + "/Desktop", "/Volumes", "/"};
  for (const auto& path : paths)
  {
    EXPECT_EQ(1, std::ranges::count_if(m_drives, [&path](const CMediaSource& drive)
                                       { return drive.strPath == path; }))
        << path;
  }
}
