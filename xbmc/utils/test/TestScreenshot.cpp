/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Screenshot.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using Error = CScreenShot::ScreenshotError;

//! \brief Exercises deletion against a real screenshot folder. Taking one is not covered:
//!        it needs a render loop to deliver a frame.
class TestScreenshotDeletion : public testing::Test
{
protected:
  void SetUp() override
  {
    m_dir = URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://temp/"), "shots");
    ASSERT_TRUE(XFILE::CDirectory::Create(m_dir));
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(
        CSettings::SETTING_DEBUG_SCREENSHOTPATH, m_dir);
  }

  void TearDown() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(
        CSettings::SETTING_DEBUG_SCREENSHOTPATH, "");
    XFILE::CDirectory::RemoveRecursive(m_dir);
  }

  void Write(const std::string& name)
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(URIUtils::AddFileToFolder(m_dir, name), true));
    file.Close();
  }

  bool Exists(const std::string& name) const
  {
    return XFILE::CFile::Exists(URIUtils::AddFileToFolder(m_dir, name));
  }

  std::string m_dir;
};

TEST_F(TestScreenshotDeletion, ClearsEveryScreenshotAndSaysHowMany)
{
  Write("screenshot00000.png");
  Write("screenshot00001.png");
  Write("screenshot00001-video.png");

  const CScreenShot::ScreenshotDeletion removed = CScreenShot::DeleteScreenshots();

  EXPECT_EQ(Error::NONE, removed.error);
  EXPECT_EQ(3u, removed.deleted);
  EXPECT_FALSE(Exists("screenshot00000.png"));
  EXPECT_FALSE(Exists("screenshot00001-video.png"));
}

TEST_F(TestScreenshotDeletion, LeavesWhatIsNotAScreenshotAlone)
{
  Write("screenshot00000.png");
  Write("notes.txt");

  const CScreenShot::ScreenshotDeletion removed = CScreenShot::DeleteScreenshots();

  EXPECT_EQ(1u, removed.deleted);
  EXPECT_TRUE(Exists("notes.txt"));
}

TEST_F(TestScreenshotDeletion, ClearingAnEmptyFolderIsNotAFailure)
{
  const CScreenShot::ScreenshotDeletion removed = CScreenShot::DeleteScreenshots();

  EXPECT_EQ(Error::NONE, removed.error);
  EXPECT_EQ(0u, removed.deleted);
}

TEST_F(TestScreenshotDeletion, DeletesTheOneNamed)
{
  Write("screenshot00000.png");
  Write("screenshot00001.png");

  const CScreenShot::ScreenshotDeletion removed =
      CScreenShot::DeleteScreenshots("screenshot00000.png");

  EXPECT_EQ(Error::NONE, removed.error);
  EXPECT_EQ(1u, removed.deleted);
  EXPECT_FALSE(Exists("screenshot00000.png"));
  EXPECT_TRUE(Exists("screenshot00001.png"));
}

TEST_F(TestScreenshotDeletion, AcceptsThePathTakeScreenshotAnswered)
{
  Write("screenshot00000.png");

  const CScreenShot::ScreenshotDeletion removed =
      CScreenShot::DeleteScreenshots("special://screenshots/screenshot00000.png");

  EXPECT_EQ(Error::NONE, removed.error);
  EXPECT_EQ(1u, removed.deleted);
  EXPECT_FALSE(Exists("screenshot00000.png"));
}

TEST_F(TestScreenshotDeletion, RefusesANameThatWouldLeaveTheFolder)
{
  for (const auto* name :
       {"../secret.png", "sub/shot.png", "special://screenshots/../secret.png", "C:\\secret.png"})
  {
    const CScreenShot::ScreenshotDeletion removed = CScreenShot::DeleteScreenshots(name);

    EXPECT_EQ(Error::BAD_TARGET, removed.error) << name;
    EXPECT_EQ(0u, removed.deleted) << name;
  }
}

TEST_F(TestScreenshotDeletion, RefusesANameThatIsNotAPng)
{
  const CScreenShot::ScreenshotDeletion removed = CScreenShot::DeleteScreenshots("notes.txt");

  EXPECT_EQ(Error::BAD_TARGET, removed.error);
  EXPECT_EQ(0u, removed.deleted);
}

TEST_F(TestScreenshotDeletion, ReportsAScreenshotThatIsNotThere)
{
  const CScreenShot::ScreenshotDeletion removed =
      CScreenShot::DeleteScreenshots("screenshot00000.png");

  EXPECT_EQ(Error::NOT_FOUND, removed.error);
  EXPECT_EQ(0u, removed.deleted);
}

TEST(TestScreenshotPath, OnlyAPlainPngUnderTheScreenshotFolderIsAScreenshot)
{
  EXPECT_TRUE(CScreenShot::IsScreenshotPath("special://screenshots/screenshot00000.png"));
  EXPECT_TRUE(CScreenShot::IsScreenshotPath("special://screenshots/named.png"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://screenshots/passwords.xml"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://screenshots/sub/shot.png"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://screenshots/"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://temp/shot.png"));
}
