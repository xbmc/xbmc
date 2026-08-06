/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StreamDetails.h"

#include <gtest/gtest.h>

TEST(TestStreamDetails, General)
{
  CStreamDetails a;
  CStreamDetailVideo *video = new CStreamDetailVideo();
  CStreamDetailAudio *audio = new CStreamDetailAudio();
  CStreamDetailSubtitle *subtitle = new CStreamDetailSubtitle();

  video->m_iWidth = 1920;
  video->m_iHeight = 1080;
  video->m_fAspect = 2.39f;
  video->m_iDuration = 30;
  video->m_strCodec = "h264";
  video->m_strStereoMode = "left_right";
  video->m_strLanguage = "eng";

  audio->m_iChannels = 2;
  audio->m_strCodec = "aac";
  audio->m_strLanguage = "eng";

  subtitle->m_strLanguage = "eng";

  a.AddStream(video);
  a.AddStream(audio);

  EXPECT_TRUE(a.HasItems());

  EXPECT_EQ(1, a.GetStreamCount(CStreamDetail::VIDEO));
  EXPECT_EQ(1, a.GetVideoStreamCount());
  EXPECT_STREQ("", a.GetVideoCodec().c_str());
  EXPECT_EQ(0.0f, a.GetVideoAspect());
  EXPECT_EQ(0, a.GetVideoWidth());
  EXPECT_EQ(0, a.GetVideoHeight());
  EXPECT_EQ(0, a.GetVideoDuration());
  EXPECT_STREQ("", a.GetStereoMode().c_str());

  EXPECT_EQ(1, a.GetStreamCount(CStreamDetail::AUDIO));
  EXPECT_EQ(1, a.GetAudioStreamCount());

  EXPECT_EQ(0, a.GetStreamCount(CStreamDetail::SUBTITLE));
  EXPECT_EQ(0, a.GetSubtitleStreamCount());

  a.AddStream(subtitle);
  EXPECT_EQ(1, a.GetStreamCount(CStreamDetail::SUBTITLE));
  EXPECT_EQ(1, a.GetSubtitleStreamCount());

  a.DetermineBestStreams();
  EXPECT_STREQ("h264", a.GetVideoCodec().c_str());
  EXPECT_EQ(2.39f, a.GetVideoAspect());
  EXPECT_EQ(1920, a.GetVideoWidth());
  EXPECT_EQ(1080, a.GetVideoHeight());
  EXPECT_EQ(30, a.GetVideoDuration());
  EXPECT_STREQ("left_right", a.GetStereoMode().c_str());
}

TEST(TestStreamDetails, VideoDimsToResolutionDescription)
{
  EXPECT_STREQ("1080",
               CStreamDetails::VideoDimsToResolutionDescription(1920, 1080).c_str());
}

TEST(TestStreamDetails, VideoAspectToAspectDescription)
{
  EXPECT_STREQ("2.40", CStreamDetails::VideoAspectToAspectDescription(2.39f).c_str());
}

TEST(TestStreamDetails, VideoAspectToAspectDescriptionCommonRatios)
{
  // Every entry in the table must classify as itself.
  EXPECT_STREQ("1.00", CStreamDetails::VideoAspectToAspectDescription(1.00f).c_str());
  EXPECT_STREQ("1.19", CStreamDetails::VideoAspectToAspectDescription(1.19f).c_str());
  EXPECT_STREQ("1.33", CStreamDetails::VideoAspectToAspectDescription(1.33f).c_str());
  EXPECT_STREQ("1.37", CStreamDetails::VideoAspectToAspectDescription(1.37f).c_str());
  EXPECT_STREQ("1.66", CStreamDetails::VideoAspectToAspectDescription(1.66f).c_str());
  EXPECT_STREQ("1.78", CStreamDetails::VideoAspectToAspectDescription(1.78f).c_str());
  EXPECT_STREQ("1.85", CStreamDetails::VideoAspectToAspectDescription(1.85f).c_str());
  EXPECT_STREQ("2.00", CStreamDetails::VideoAspectToAspectDescription(2.00f).c_str());
  EXPECT_STREQ("2.20", CStreamDetails::VideoAspectToAspectDescription(2.20f).c_str());
  EXPECT_STREQ("2.35", CStreamDetails::VideoAspectToAspectDescription(2.35f).c_str());
  EXPECT_STREQ("2.40", CStreamDetails::VideoAspectToAspectDescription(2.40f).c_str());
  EXPECT_STREQ("2.55", CStreamDetails::VideoAspectToAspectDescription(2.55f).c_str());
  EXPECT_STREQ("2.76", CStreamDetails::VideoAspectToAspectDescription(2.76f).c_str());
}

TEST(TestStreamDetails, VideoAspectToAspectDescriptionBoundaries)
{
  // The cutoff between two adjacent entries is their geometric mean. Probe either side of
  // every cutoff so that a change to the table cannot silently move one.
  EXPECT_STREQ("1.00", CStreamDetails::VideoAspectToAspectDescription(1.0899f).c_str());
  EXPECT_STREQ("1.19", CStreamDetails::VideoAspectToAspectDescription(1.0919f).c_str());

  EXPECT_STREQ("1.19", CStreamDetails::VideoAspectToAspectDescription(1.2571f).c_str());
  EXPECT_STREQ("1.33", CStreamDetails::VideoAspectToAspectDescription(1.2591f).c_str());

  EXPECT_STREQ("1.33", CStreamDetails::VideoAspectToAspectDescription(1.3489f).c_str());
  EXPECT_STREQ("1.37", CStreamDetails::VideoAspectToAspectDescription(1.3509f).c_str());

  EXPECT_STREQ("1.37", CStreamDetails::VideoAspectToAspectDescription(1.5070f).c_str());
  EXPECT_STREQ("1.66", CStreamDetails::VideoAspectToAspectDescription(1.5090f).c_str());

  EXPECT_STREQ("1.66", CStreamDetails::VideoAspectToAspectDescription(1.7180f).c_str());
  EXPECT_STREQ("1.78", CStreamDetails::VideoAspectToAspectDescription(1.7200f).c_str());

  EXPECT_STREQ("1.78", CStreamDetails::VideoAspectToAspectDescription(1.8137f).c_str());
  EXPECT_STREQ("1.85", CStreamDetails::VideoAspectToAspectDescription(1.8157f).c_str());

  EXPECT_STREQ("1.85", CStreamDetails::VideoAspectToAspectDescription(1.9225f).c_str());
  EXPECT_STREQ("2.00", CStreamDetails::VideoAspectToAspectDescription(1.9245f).c_str());

  EXPECT_STREQ("2.00", CStreamDetails::VideoAspectToAspectDescription(2.0966f).c_str());
  EXPECT_STREQ("2.20", CStreamDetails::VideoAspectToAspectDescription(2.0986f).c_str());

  EXPECT_STREQ("2.20", CStreamDetails::VideoAspectToAspectDescription(2.2728f).c_str());
  EXPECT_STREQ("2.35", CStreamDetails::VideoAspectToAspectDescription(2.2748f).c_str());

  EXPECT_STREQ("2.35", CStreamDetails::VideoAspectToAspectDescription(2.3739f).c_str());
  EXPECT_STREQ("2.40", CStreamDetails::VideoAspectToAspectDescription(2.3759f).c_str());

  EXPECT_STREQ("2.40", CStreamDetails::VideoAspectToAspectDescription(2.4729f).c_str());
  EXPECT_STREQ("2.55", CStreamDetails::VideoAspectToAspectDescription(2.4749f).c_str());

  EXPECT_STREQ("2.55", CStreamDetails::VideoAspectToAspectDescription(2.6519f).c_str());
  EXPECT_STREQ("2.76", CStreamDetails::VideoAspectToAspectDescription(2.6539f).c_str());
}

TEST(TestStreamDetails, VideoAspectToAspectDescriptionEdgeCases)
{
  // An unset aspect is not classified, and neither is a negative one - it is not a ratio.
  EXPECT_STREQ("", CStreamDetails::VideoAspectToAspectDescription(0.0f).c_str());
  EXPECT_STREQ("", CStreamDetails::VideoAspectToAspectDescription(-1.0f).c_str());

  // The cutoffs are derived from the table rather than written out to four decimal places,
  // so a value inside the rounding error of an old hand-written cutoff now falls on the side
  // the geometric mean actually puts it. sqrt(1.00*1.19) is 1.0908712, not 1.0909.
  EXPECT_STREQ("1.19", CStreamDetails::VideoAspectToAspectDescription(1.09088f).c_str());

  // The table classifies but never rejects: anything above the widest entry is reported as
  // that entry, however implausible. Pinned so that the behaviour is a decision, not an
  // accident, for any caller tempted to use this as a validity check.
  EXPECT_STREQ("2.76", CStreamDetails::VideoAspectToAspectDescription(4.5f).c_str());
}
