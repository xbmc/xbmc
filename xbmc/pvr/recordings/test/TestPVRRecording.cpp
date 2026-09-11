/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h"
#include "pvr/recordings/PVRRecording.h"
#include "utils/Variant.h"

#include <string>

#include <gtest/gtest.h>

using namespace PVR;

namespace
{

constexpr unsigned int CLIENT_ID{1};

/*!
 \brief A recording as a client reports one, with the channel type stated so that the
        PVR manager is never consulted for it.
 */
PVR_RECORDING ClientRecording(const char* title, const char* episodeName)
{
  PVR_RECORDING recording{};
  recording.strRecordingId = "0815";
  recording.strTitle = title;
  recording.strEpisodeName = episodeName;
  recording.strChannelName = "Example Channel";
  recording.channelType = PVR_RECORDING_CHANNEL_TYPE_TV;
  recording.iGenreType = EPG_GENRE_USE_STRING;
  recording.strGenreDescription = "Drama";
  return recording;
}

} // unnamed namespace

/*!
 A client reports a programme title plus an episode name, the opposite way round to the
 CVideoInfoTag members a recording inherits.
 */
TEST(TestPVRRecording, AnEpisodeDescribesItselfAsAScannedOneDoes)
{
  const CPVRRecording recording{ClientRecording("Heroes", "Genesis"), CLIENT_ID};

  EXPECT_EQ("Genesis", recording.m_strTitle);
  EXPECT_EQ("Heroes", recording.m_strShowTitle);

  CVariant serialized;
  recording.Serialize(serialized);

  EXPECT_EQ("Genesis", serialized["title"].asString());
  EXPECT_EQ("Heroes", serialized["showtitle"].asString());
}

/*!
 The PVR side still reaches what the client sent; renaming a recording renames the programme.
 */
TEST(TestPVRRecording, AnEpisodeStillReportsWhatTheClientSent)
{
  CPVRRecording recording{ClientRecording("Heroes", "Genesis"), CLIENT_ID};

  EXPECT_EQ("Heroes", recording.ProgrammeTitle());
  EXPECT_EQ("Genesis", recording.EpisodeName());

  recording.SetProgrammeTitle("Heroes Reborn");

  EXPECT_EQ("Heroes Reborn", recording.ProgrammeTitle());
  EXPECT_EQ("Genesis", recording.EpisodeName());
}

/*!
 A recording that is not an episode has its own title and belongs to no show, as a film does.
 */
TEST(TestPVRRecording, ARecordingThatIsNotAnEpisodeBelongsToNoShow)
{
  CPVRRecording recording{ClientRecording("Casablanca", ""), CLIENT_ID};

  EXPECT_EQ("Casablanca", recording.m_strTitle);
  EXPECT_EQ("", recording.m_strShowTitle);
  EXPECT_EQ("Casablanca", recording.ProgrammeTitle());
  EXPECT_EQ("", recording.EpisodeName());

  recording.SetProgrammeTitle("Casablanca (1942)");

  EXPECT_EQ("Casablanca (1942)", recording.ProgrammeTitle());
  EXPECT_EQ("Casablanca (1942)", recording.m_strTitle);
}

/*!
 The path keys the recording's playback state in the video database, so it is still built
 from the programme title and then the episode name.
 */
TEST(TestPVRRecording, ThePathKeysOnTheProgrammeTitleAndThenTheEpisodeName)
{
  PVR_RECORDING clientEpisode{ClientRecording("Heroes", "Genesis")};
  clientEpisode.iSeriesNumber = 1;
  clientEpisode.iEpisodeNumber = 1;
  const CPVRRecording episode{clientEpisode, CLIENT_ID};

  EXPECT_EQ("pvr://recordings/tv/active/Heroes s01e01%20Genesis, TV%20(Example%20Channel), "
            "19700101_000000, 0815.pvr",
            episode.m_strFileNameAndPath);

  const CPVRRecording film{ClientRecording("Casablanca", ""), CLIENT_ID};

  EXPECT_EQ("pvr://recordings/tv/active/Casablanca, TV%20(Example%20Channel), "
            "19700101_000000, 0815.pvr",
            film.m_strFileNameAndPath);
}
