/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CueDocument.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "music/tags/MusicInfoTag.h"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{

const std::string input =
    R"(PERFORMER "Pink Floyd"
TITLE "The Dark Side Of The Moon"
 FILE "The Dark Side Of The Moon.mp3" WAVE
   TRACK 01 AUDIO
     TITLE "Speak To Me / Breathe"
     PERFORMER "Pink Floyd"
     INDEX 00 00:00:00
     INDEX 01 00:00:32
   TRACK 02 AUDIO
     TITLE "On The Run"
     PERFORMER "Pink Floyd"
     INDEX 00 03:58:72
     INDEX 01 04:00:72
   TRACK 03 AUDIO
     TITLE "Time"
     PERFORMER "Pink Floyd"
     INDEX 00 07:31:70
     INDEX 01 07:33:70)";

}

TEST(TestCueDocument, LoadTracks)
{
  CCueDocument doc;
  doc.ParseTag(input);

  using namespace std::string_literals;

  CFileItem item("The Dark Side Of The Moon.mp3", false);
  auto& tag = *item.GetMusicInfoTag();
  tag.SetAlbum("TestAlbum"s);
  tag.SetLoaded(true);
  tag.SetAlbumArtist("TestAlbumArtist"s);
  tag.SetGenre("TestGenre"s);
  tag.SetArtist({"TestArtist1"s, "TestArtist2"s});
  tag.SetCueSheet("TestCueSheet"s);
  tag.SetYear(2005);
  tag.SetDuration(554);

  CFileItemList scannedItems;
  doc.LoadTracks(scannedItems, item);

  ASSERT_EQ(scannedItems.Size(), 3U);

  static const auto trackNames = std::array{
      "Speak To Me / Breathe"s,
      "On The Run"s,
      "Time"s,
  };
  static const auto duration = std::array{241, 213, 100};
  for (size_t i = 0; i < 3; ++i)
  {
    EXPECT_EQ(scannedItems[i]->GetPath(), "The Dark Side Of The Moon.mp3");
    ASSERT_TRUE(scannedItems[i]->GetMusicInfoTag() != nullptr);
    EXPECT_EQ(scannedItems[i]->GetMusicInfoTag()->GetArtist(), std::vector{"Pink Floyd"s});
    EXPECT_EQ(scannedItems[i]->GetMusicInfoTag()->GetAlbumArtist(), std::vector{"Pink Floyd"s});
    EXPECT_EQ(scannedItems[i]->GetMusicInfoTag()->GetCueSheet(), "TestCueSheet"s);
    EXPECT_EQ(scannedItems[i]->GetMusicInfoTag()->GetGenre(), std::vector{"TestGenre"s});
    EXPECT_EQ(scannedItems[i]->GetMusicInfoTag()->GetTitle(), trackNames[i]);
    EXPECT_EQ(scannedItems[i]->GetMusicInfoTag()->GetTrackNumber(), i + 1);
    EXPECT_EQ(scannedItems[i]->GetMusicInfoTag()->GetDuration(), duration[i]);
  }
}
