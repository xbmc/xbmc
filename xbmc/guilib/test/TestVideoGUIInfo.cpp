/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/guiinfo/VideoGUIInfo.h"
#include "video/VideoInfoTag.h"

#include <gtest/gtest.h>

using namespace KODI::GUILIB::GUIINFO;

TEST(TestVideoGUIInfo, ListItemVideoProfileReturnsProfile)
{
  CFileItem item{"/videos/test.mkv", false};
  auto* video = new CStreamDetailVideo();
  video->m_strProfile = "Main 10";
  item.GetVideoInfoTag()->m_streamDetails.AddStream(video);
  item.GetVideoInfoTag()->m_streamDetails.DetermineBestStreams();

  CVideoGUIInfo videoGUIInfo;
  std::string value;

  EXPECT_TRUE(videoGUIInfo.GetLabel(value, &item, 0, CGUIInfo(LISTITEM_VIDEO_PROFILE), nullptr));
  EXPECT_EQ(value, "Main 10");
}

TEST(TestVideoGUIInfo, ListItemVideoProfileReturnsEmptyWhenUnavailable)
{
  CFileItem item{"/videos/test.mkv", false};
  auto* video = new CStreamDetailVideo();
  video->m_strCodec = "hevc";
  item.GetVideoInfoTag()->m_streamDetails.AddStream(video);
  item.GetVideoInfoTag()->m_streamDetails.DetermineBestStreams();

  CVideoGUIInfo videoGUIInfo;
  std::string value;

  EXPECT_TRUE(videoGUIInfo.GetLabel(value, &item, 0, CGUIInfo(LISTITEM_VIDEO_PROFILE), nullptr));
  EXPECT_TRUE(value.empty());
}
