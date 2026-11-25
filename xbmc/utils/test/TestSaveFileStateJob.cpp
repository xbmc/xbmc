/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "utils/ItemProperties.h"
#include "utils/SaveFileStateJob.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

#include <gtest/gtest.h>

using namespace KODI;

// Playback reads a disc's state back by its label and serial, not by the drive's files
TEST(TestSaveFileStateJob, ADvdIsTrackedByTheDisc)
{
  CFileItem item{"/media/dvd/VIDEO_TS/VIDEO_TS.IFO", false};
  item.GetVideoInfoTag()->m_strFileNameAndPath = "removable://FILM_1234";

  EXPECT_EQ("removable://FILM_1234", CSaveFileState::ProgressTrackingFileOf(item));
}

// The stack helper reads a resumed stack's state back by the add-on's url
TEST(TestSaveFileStateJob, AStackAnAddOnResolvedToIsTrackedByTheAddOnsUrl)
{
  CFileItem item{"stack:///videos/film-cd1.mkv , /videos/film-cd2.mkv", false};
  item.SetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL, "plugin://plugin.video.example/?id=1");

  EXPECT_EQ("plugin://plugin.video.example/?id=1", CSaveFileState::ProgressTrackingFileOf(item));
}

TEST(TestSaveFileStateJob, AStackIsTrackedByItsStackPath)
{
  const std::string stack{"stack:///videos/film-cd1.mkv , /videos/film-cd2.mkv"};
  const CFileItem item{stack, false};

  EXPECT_EQ(stack, CSaveFileState::ProgressTrackingFileOf(item));
}
