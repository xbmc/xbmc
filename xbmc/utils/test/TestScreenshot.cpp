/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "jobs/JobManager.h"
#include "rendering/capture/CapturePixels.h"
#include "rendering/capture/CaptureService.h"
#include "rendering/capture/CaptureTypes.h"
#include "settings/Settings.h"
#include "test/TestUtils.h"
#include "utils/Screenshot.h"
#include "utils/URIUtils.h"

#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using Error = CScreenShot::ScreenshotError;
using namespace KODI::RENDERING::CAPTURE;
using namespace std::chrono_literals;

//! \brief Exercises deletion against a real screenshot folder.
class TestScreenshotDeletion : public testing::Test
{
protected:
  void SetUp() override
  {
    m_dir = URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://temp/"), "shots");
    ASSERT_TRUE(XFILE::CDirectory::Create(m_dir));
    m_screenshotPath.emplace(CSettings::SETTING_DEBUG_SCREENSHOTPATH, m_dir);
  }

  void TearDown() override
  {
    m_screenshotPath.reset();
    XFILE::CDirectory::RemoveRecursive(m_dir);
  }

  void Write(const std::string& name, const std::string& content = "")
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(URIUtils::AddFileToFolder(m_dir, name), true));
    if (!content.empty())
      ASSERT_EQ(static_cast<ssize_t>(content.size()), file.Write(content.data(), content.size()));
    file.Close();
  }

  std::string Read(const std::string& name) const
  {
    XFILE::CFile file;
    std::vector<uint8_t> content;
    if (file.LoadFile(URIUtils::AddFileToFolder(m_dir, name), content) < 0)
      return {};
    return {content.begin(), content.end()};
  }

  bool Exists(const std::string& name) const
  {
    return XFILE::CFile::Exists(URIUtils::AddFileToFolder(m_dir, name));
  }

  int FileCount() const
  {
    CFileItemList items;
    XFILE::CDirectory::GetDirectory(m_dir, items, "",
                                    XFILE::DIR_FLAG_NO_FILE_DIRS | XFILE::DIR_FLAG_GET_HIDDEN);
    return items.Size();
  }

  std::string m_dir;
  std::optional<CScopedSetting> m_screenshotPath;
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

TEST_F(TestScreenshotDeletion, ClearsAScreenshotTheFolderHides)
{
  Write(".hidden.png");

  const CScreenShot::ScreenshotDeletion removed = CScreenShot::DeleteScreenshots();

  EXPECT_EQ(Error::NONE, removed.error);
  EXPECT_EQ(1u, removed.deleted);
  EXPECT_FALSE(Exists(".hidden.png"));
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

TEST_F(TestScreenshotDeletion, ClearingSaysWhenAScreenshotCouldNotBeDeleted)
{
#ifndef TARGET_WINDOWS
  GTEST_SKIP() << "a file held open can still be deleted on this platform";
#else
  Write("screenshot00000.png");
  XFILE::CFile held;
  ASSERT_TRUE(held.OpenForWrite(URIUtils::AddFileToFolder(m_dir, "screenshot00001.png"), true));

  const CScreenShot::ScreenshotDeletion removed = CScreenShot::DeleteScreenshots();
  held.Close();

  EXPECT_EQ(Error::FAILED, removed.error);
  EXPECT_EQ(1u, removed.deleted);
#endif
}

//! \brief Takes screenshots with the test standing in for the render loop.
class TestScreenshotTaking : public TestScreenshotDeletion
{
protected:
  void SetUp() override
  {
    TestScreenshotDeletion::SetUp();
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    m_service = std::make_shared<CCaptureService>();
    CServiceBroker::RegisterCaptureService(m_service);
  }

  void TearDown() override
  {
    CServiceBroker::UnregisterCaptureService();
    m_service.reset();
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    CServiceBroker::UnregisterJobManager();
    TestScreenshotDeletion::TearDown();
  }

  //! Draw one frame of the given content for the request on top, once there is one
  bool Deliver(CaptureContent content) { return Deliver(content, true); }

  //! \p readable false delivers a frame whose pixels cannot be locked, so nothing is written
  bool Deliver(CaptureContent content, bool readable)
  {
    for (int attempt = 0; attempt < 400; ++attempt)
    {
      m_service->LatchFrame();
      const auto active = m_service->TakeActive(content);
      if (!active.empty())
      {
        CaptureResult result;
        result.width = 4;
        result.height = 2;
        result.stride = 16;
        result.format = AV_PIX_FMT_BGRA;
        result.content = content;
        result.pixels = std::make_shared<CHeapCapturePixels>(
            readable ? std::make_unique<uint8_t[]>(32) : nullptr);
        m_service->Complete(active[0], result);
        return true;
      }
      std::this_thread::sleep_for(5ms);
    }
    return false;
  }

  std::shared_ptr<CCaptureService> m_service;
};

TEST_F(TestScreenshotTaking, WritesTheFrameAndAnswersItsPath)
{
  auto taken = std::async(std::launch::async, []
                          { return CScreenShot::TakeScreenshotSync(CaptureContent::COMPOSITE); });
  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE));

  const CScreenShot::ScreenshotFiles files = taken.get();
  EXPECT_EQ(Error::NONE, files.error);
  EXPECT_EQ("special://screenshots/screenshot00000.png", files.composite);
  EXPECT_TRUE(Exists("screenshot00000.png"));
}

TEST_F(TestScreenshotTaking, BothWithNoVideoFrameFailsAtOnceAndLeavesNoFile)
{
  auto taken = std::async(std::launch::async,
                          [] { return CScreenShot::TakeScreenshotSync(CaptureContent::BOTH); });
  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE));

  ASSERT_EQ(std::future_status::ready, taken.wait_for(2s));
  EXPECT_EQ(Error::FAILED, taken.get().error);
  EXPECT_FALSE(Exists("screenshot00000.png"));
}

TEST_F(TestScreenshotTaking, AFailedScreenshotLeavesTheOneItsNameAlreadyHeld)
{
  Write("named.png");

  auto taken = std::async(
      std::launch::async,
      [] { return CScreenShot::TakeScreenshotSync(CaptureContent::COMPOSITE, "named.png"); });
  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE, false));

  EXPECT_EQ(Error::FAILED, taken.get().error);
  EXPECT_TRUE(Exists("named.png"));
}

TEST_F(TestScreenshotTaking, AFailedPairLeavesBothScreenshotsItsNameAlreadyHeld)
{
  Write("named.png", "old composite");
  Write("named-video.png", "old video");

  auto taken =
      std::async(std::launch::async,
                 [] { return CScreenShot::TakeScreenshotSync(CaptureContent::BOTH, "named.png"); });
  ASSERT_TRUE(Deliver(CaptureContent::VIDEO));
  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE, false));

  EXPECT_EQ(Error::FAILED, taken.get().error);
  EXPECT_EQ("old composite", Read("named.png"));
  EXPECT_EQ("old video", Read("named-video.png"));
  EXPECT_EQ(2, FileCount());
}

TEST_F(TestScreenshotTaking, ANamedPairReplacesTheScreenshotsItsNameHeld)
{
  Write("named.png", "old composite");
  Write("named-video.png", "old video");

  auto taken =
      std::async(std::launch::async,
                 [] { return CScreenShot::TakeScreenshotSync(CaptureContent::BOTH, "named.png"); });
  ASSERT_TRUE(Deliver(CaptureContent::VIDEO));
  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE));

  EXPECT_EQ(Error::NONE, taken.get().error);
  EXPECT_EQ("\x89PNG", Read("named.png").substr(0, 4));
  EXPECT_EQ("\x89PNG", Read("named-video.png").substr(0, 4));
  EXPECT_EQ(2, FileCount());
}

TEST_F(TestScreenshotTaking, ANumberItsVideoFrameStillHoldsIsNotTakenAgain)
{
  Write("screenshot00000-video.png");

  auto taken = std::async(std::launch::async, []
                          { return CScreenShot::TakeScreenshotSync(CaptureContent::COMPOSITE); });
  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE));

  EXPECT_EQ("special://screenshots/screenshot00001.png", taken.get().composite);
  EXPECT_TRUE(Exists("screenshot00000-video.png"));
}

TEST_F(TestScreenshotTaking, TwoRequestsWaitingTogetherTakeDifferentNames)
{
  const auto take = [] { return CScreenShot::TakeScreenshotSync(CaptureContent::COMPOSITE); };
  auto first = std::async(std::launch::async, take);
  auto second = std::async(std::launch::async, take);
  std::this_thread::sleep_for(200ms);

  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE));
  ASSERT_TRUE(Deliver(CaptureContent::COMPOSITE));

  const CScreenShot::ScreenshotFiles one = first.get();
  const CScreenShot::ScreenshotFiles other = second.get();
  EXPECT_EQ(Error::NONE, one.error);
  EXPECT_EQ(Error::NONE, other.error);
  EXPECT_NE(one.composite, other.composite);
  EXPECT_TRUE(Exists("screenshot00000.png"));
  EXPECT_TRUE(Exists("screenshot00001.png"));
}

TEST(TestScreenshotPath, OnlyAPlainPngUnderTheScreenshotFolderIsAScreenshot)
{
  const CScopedSetting folder(CSettings::SETTING_DEBUG_SCREENSHOTPATH,
                              CSpecialProtocol::TranslatePath("special://temp/"));

  EXPECT_TRUE(CScreenShot::IsScreenshotPath("special://screenshots/screenshot00000.png"));
  EXPECT_TRUE(CScreenShot::IsScreenshotPath("special://screenshots/named.png"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://screenshots/passwords.xml"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://screenshots/sub/shot.png"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://screenshots/"));
  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://temp/shot.png"));
}

TEST(TestScreenshotPath, NothingIsAScreenshotWhileNoFolderIsSet)
{
  const CScopedSetting noFolder(CSettings::SETTING_DEBUG_SCREENSHOTPATH, "");

  EXPECT_FALSE(CScreenShot::IsScreenshotPath("special://screenshots/screenshot00000.png"));
}
