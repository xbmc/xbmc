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
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"

#include <string>

#include <gtest/gtest.h>

class TestSettingsPersistence : public testing::Test
{
protected:
  void SetUp() override
  {
    m_file = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile();
    ASSERT_FALSE(XFILE::CFile::Exists(m_file));
    m_setting = std::make_shared<CSettingBool>("test.value", nullptr);
    ASSERT_TRUE(m_settings.GetSettingsManager()->AddSetting(
        m_setting, std::make_shared<CSettingSection>("test"),
        std::make_shared<CSettingCategory>("test"), std::make_shared<CSettingGroup>("test")));
    m_settings.GetSettingsManager()->SetInitialized();
  }

  void TearDown() override { XFILE::CFile::Delete(m_file); }

  void Write(const std::string& xml)
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(m_file, true));
    ASSERT_EQ(static_cast<ssize_t>(xml.size()), file.Write(xml.data(), xml.size()));
  }

  std::string Read() const
  {
    XFILE::CFile file;
    if (!file.Open(m_file))
      return {};
    std::string xml(file.GetLength(), '\0');
    file.Read(xml.data(), xml.size());
    return xml;
  }

  CSettings m_settings;
  std::shared_ptr<CSettingBool> m_setting;
  std::string m_file;
};

TEST_F(TestSettingsPersistence, FutureVersionDoesNotResetFile)
{
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version + 1) +
                          "\"><setting id=\"future.value\" future=\"yes\">keep</setting>"
                          "<future-section /></settings>";
  Write(xml);
  EXPECT_FALSE(m_settings.Load(m_file));
  EXPECT_EQ(xml, Read());
}

TEST_F(TestSettingsPersistence, LoadsCurrentVersion)
{
  const std::string xml = "<settings version=\"" + std::to_string(CSettingsManager::Version) +
                          "\"><setting id=\"test.value\">true</setting></settings>";
  Write(xml);
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_TRUE(m_setting->GetValue());
  EXPECT_EQ(xml, Read());
}

TEST_F(TestSettingsPersistence, RecoversMissingFile)
{
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_FALSE(m_setting->GetValue());
  EXPECT_FALSE(Read().empty());
}

TEST_F(TestSettingsPersistence, RecoversMalformedFile)
{
  Write("<settings><broken");
  EXPECT_TRUE(m_settings.Load(m_file));
  EXPECT_FALSE(m_setting->GetValue());
  EXPECT_NE("<settings><broken", Read());
}
