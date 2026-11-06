/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

namespace
{
using MetadataSources = std::unordered_set<std::string>;

const MetadataSources DEFAULT_METADATA_SOURCES{"tmdb", "imdb", "tvdb", "anidb"};

class TestAdvancedSettingsMetadataSources : public ::testing::Test
{
protected:
  void TearDown() override
  {
    for (XFILE::CFile* file : m_files)
      EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
  }

  /*!
   * \brief Load a fresh CAdvancedSettings with an extra advancedsettings.xml for each of the
   * given contents, loaded in order.
   */
  void Load(std::initializer_list<std::string_view> xmls = {})
  {
    for (const std::string_view xml : xmls)
    {
      XFILE::CFile* file = XBMC_CREATETEMPFILE(".xml");
      ASSERT_NE(file, nullptr);
      m_files.push_back(file);
      ASSERT_EQ(file->Write(xml.data(), xml.size()), static_cast<ssize_t>(xml.size()));
      file->Close();
      m_settings.AddSettingsFile(XBMC_TEMPFILEPATH(file));
    }

    const std::shared_ptr<CProfileManager> profileManager =
        CServiceBroker::GetSettingsComponent()->GetProfileManager();
    ASSERT_NE(profileManager, nullptr);
    ASSERT_TRUE(m_settings.Load(*profileManager));
  }

  CAdvancedSettings m_settings;
  std::vector<XFILE::CFile*> m_files;
};
} // namespace

TEST_F(TestAdvancedSettingsMetadataSources, DefaultsWithoutSettingsFile)
{
  Load();
  EXPECT_EQ(m_settings.m_videoScannerMetadataSources, DEFAULT_METADATA_SOURCES);
}

TEST_F(TestAdvancedSettingsMetadataSources, DefaultsWithoutVideoScannerSection)
{
  Load({"<advancedsettings></advancedsettings>"});
  EXPECT_EQ(m_settings.m_videoScannerMetadataSources, DEFAULT_METADATA_SOURCES);
}

TEST_F(TestAdvancedSettingsMetadataSources, DefaultsWithoutMetadataSourcesElement)
{
  Load({"<advancedsettings><videoscanner><ignoreerrors>false</ignoreerrors></videoscanner>"
        "</advancedsettings>"});
  EXPECT_EQ(m_settings.m_videoScannerMetadataSources, DEFAULT_METADATA_SOURCES);
}

TEST_F(TestAdvancedSettingsMetadataSources, AddAndRemove)
{
  Load({"<advancedsettings><videoscanner><metadatasources>"
        "<add>TVmaze|mal</add><remove>anidb</remove>"
        "</metadatasources></videoscanner></advancedsettings>"});
  EXPECT_EQ(m_settings.m_videoScannerMetadataSources,
            MetadataSources({"tmdb", "imdb", "tvdb", "tvmaze", "mal"}));
}

TEST_F(TestAdvancedSettingsMetadataSources, RemoveSourceAddedByEarlierFile)
{
  Load({"<advancedsettings><videoscanner><metadatasources>"
        "<add>TVmaze</add>"
        "</metadatasources></videoscanner></advancedsettings>",
        "<advancedsettings><videoscanner><metadatasources>"
        "<remove>tvmaze</remove>"
        "</metadatasources></videoscanner></advancedsettings>"});
  EXPECT_EQ(m_settings.m_videoScannerMetadataSources, DEFAULT_METADATA_SOURCES);
}
