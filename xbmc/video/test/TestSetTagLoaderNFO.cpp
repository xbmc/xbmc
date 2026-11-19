/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoScanner.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/SetInfoTag.h"
#include "video/tags/SetTagLoaderNFO.h"

#include <fstream>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
//! A movie sets folder of the test's own, holding a set's folder
class TestSetTagLoaderNFO : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_previousSetsFolder = Settings()->GetString(CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER);
    XFILE::CDirectory::RemoveRecursive(m_sets);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_sets));
    ASSERT_TRUE(XFILE::CDirectory::Create(m_saga));
    Settings()->SetString(CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER, m_sets);
  }

  void TearDown() override
  {
    Settings()->SetString(CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER, m_previousSetsFolder);
    XFILE::CDirectory::RemoveRecursive(m_sets);
  }

  static std::shared_ptr<CSettings> Settings()
  {
    return CServiceBroker::GetSettingsComponent()->GetSettings();
  }

  const std::string m_sets{CSpecialProtocol::TranslatePath("special://temp/TestSetTagLoaderNFO/")};
  const std::string m_saga{URIUtils::AddFileToFolder(m_sets, "Saga/")};
  std::string m_previousSetsFolder;
};
} // namespace

// A set's nfo is read as a document of its own, as no scraper reads sets. Read as a scraper's nfo,
// the marker in its overview made it one to override a scraper's details with, and it wasn't read.
TEST_F(TestSetTagLoaderNFO, ASetNfoIsReadWithoutAScraper)
{
  std::ofstream(URIUtils::AddFileToFolder(m_saga, "set.nfo"), std::ios::out | std::ios::binary)
      << "<set><title>Saga</title><overview>Its [scrape url] marker</overview></set>";

  CSetTagLoaderNFO loader{"Saga"};
  ASSERT_TRUE(loader.HasInfo());

  CSetInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::FULL, loader.Load(tag, false));
  EXPECT_EQ("Saga", tag.GetTitle());
  EXPECT_EQ("Its [scrape url] marker", tag.GetOverview());
}
