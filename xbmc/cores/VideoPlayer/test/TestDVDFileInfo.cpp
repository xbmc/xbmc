/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"
#include "filesystem/IFileTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/StreamDetails.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
std::string TestFile(const std::string& name)
{
  return XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/" + name);
}

//! Has every file opened for audio or video go through the file cache, local files too, for a test
class TestDVDFileInfo : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_bufferMode = m_settings->GetInt(CSettings::SETTING_FILECACHE_BUFFERMODE);
    m_settings->SetInt(CSettings::SETTING_FILECACHE_BUFFERMODE,
                       static_cast<int>(XFILE::CacheBufferMode::ALL));
  }

  void TearDown() override
  {
    m_settings->SetInt(CSettings::SETTING_FILECACHE_BUFFERMODE, m_bufferMode);
  }

  //! Whether a file opened as a probe opens it went through the file cache
  static bool OpensThroughTheCache(const std::string& path, unsigned int fileFlags)
  {
    CFileItem item{path, false};
    const auto stream{CDVDFactoryInputStream::CreateInputStream(nullptr, item, false, fileFlags)};
    EXPECT_NE(nullptr, stream);
    if (!stream || !stream->Open())
    {
      ADD_FAILURE() << "can't open " << path;
      return false;
    }
    XFILE::SCacheStatus status{};
    const bool cached{stream->GetCacheStatus(&status)};
    stream->Close();
    return cached;
  }

  std::shared_ptr<CSettings> m_settings;
  int m_bufferMode{0};
};
} // namespace

TEST_F(TestDVDFileInfo, AFileIsOpenedWithTheFlagsGiven)
{
  const std::string file{TestFile("h264_8bit_320x240.mkv")};

  EXPECT_TRUE(OpensThroughTheCache(file, 0));
  EXPECT_FALSE(OpensThroughTheCache(file, XFILE::READ_NO_CACHE));
}

// A probe without the file cache finds the same streams as one through it
TEST_F(TestDVDFileInfo, AFileProbedWithoutTheCacheHasTheSameStreams)
{
  for (const std::string name : {"h264_8bit_320x240.mkv", "letterbox_320x240_bars40.mp4"})
  {
    CFileItem cached{TestFile(name), false};
    CFileItem uncached{TestFile(name), false};
    ASSERT_TRUE(CDVDFileInfo::GetFileStreamDetails(&cached)) << name;
    ASSERT_TRUE(CDVDFileInfo::GetFileStreamDetails(&uncached, 0, nullptr, XFILE::READ_NO_CACHE))
        << name;

    const CStreamDetails& expected{cached.GetVideoInfoTag()->m_streamDetails};
    const CStreamDetails& actual{uncached.GetVideoInfoTag()->m_streamDetails};
    EXPECT_FALSE(expected.GetVideoCodec().empty()) << name;
    EXPECT_EQ(expected.GetVideoCodec(), actual.GetVideoCodec()) << name;
    EXPECT_EQ(expected.GetVideoWidth(), actual.GetVideoWidth()) << name;
    EXPECT_EQ(expected.GetVideoHeight(), actual.GetVideoHeight()) << name;
    EXPECT_EQ(expected.GetVideoDuration(), actual.GetVideoDuration()) << name;
    EXPECT_EQ(expected.GetAudioStreamCount(), actual.GetAudioStreamCount()) << name;
  }
}

// The later parts of a stack are probed for their durations as the first is
TEST_F(TestDVDFileInfo, AStackProbedWithoutTheCacheHasTheSameDuration)
{
  const std::string part{TestFile("h264_8bit_320x240.mkv")};
  const std::string stack{"stack://" + part + " , " + part};
  CFileItem cached{stack, false};
  CFileItem uncached{stack, false};
  ASSERT_TRUE(CDVDFileInfo::GetFileStreamDetails(&cached));
  ASSERT_TRUE(CDVDFileInfo::GetFileStreamDetails(&uncached, 0, nullptr, XFILE::READ_NO_CACHE));

  int duration{0}; // milliseconds
  ASSERT_TRUE(CDVDFileInfo::GetFileDuration(part, duration, XFILE::READ_NO_CACHE));
  EXPECT_EQ(2 * duration / 1000, uncached.GetVideoInfoTag()->m_streamDetails.GetVideoDuration());
  EXPECT_EQ(cached.GetVideoInfoTag()->m_streamDetails.GetVideoDuration(),
            uncached.GetVideoInfoTag()->m_streamDetails.GetVideoDuration());
}
