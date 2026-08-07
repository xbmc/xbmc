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

TEST(TestStreamDetails, VideoDimsToResolutionDescriptionCommonResolutions)
{
  EXPECT_STREQ("480", CStreamDetails::VideoDimsToResolutionDescription(640, 480).c_str());
  EXPECT_STREQ("480", CStreamDetails::VideoDimsToResolutionDescription(720, 480).c_str());
  EXPECT_STREQ("480", CStreamDetails::VideoDimsToResolutionDescription(854, 480).c_str());
  EXPECT_STREQ("540", CStreamDetails::VideoDimsToResolutionDescription(960, 540).c_str());
  EXPECT_STREQ("540", CStreamDetails::VideoDimsToResolutionDescription(960, 544).c_str());
  EXPECT_STREQ("576", CStreamDetails::VideoDimsToResolutionDescription(720, 576).c_str());
  EXPECT_STREQ("576", CStreamDetails::VideoDimsToResolutionDescription(1024, 576).c_str());
  EXPECT_STREQ("720", CStreamDetails::VideoDimsToResolutionDescription(1280, 720).c_str());
  EXPECT_STREQ("1080", CStreamDetails::VideoDimsToResolutionDescription(1920, 1080).c_str());
  EXPECT_STREQ("4K", CStreamDetails::VideoDimsToResolutionDescription(3840, 2160).c_str());
  EXPECT_STREQ("4K", CStreamDetails::VideoDimsToResolutionDescription(4096, 2160).c_str());
  EXPECT_STREQ("8K", CStreamDetails::VideoDimsToResolutionDescription(7680, 4320).c_str());
  EXPECT_STREQ("8K", CStreamDetails::VideoDimsToResolutionDescription(8192, 4320).c_str());

  // Scope framing within a 1080p container.
  EXPECT_STREQ("1080", CStreamDetails::VideoDimsToResolutionDescription(1920, 800).c_str());
  // Cropped 4K, as HandBrake writes a 2.35:1 title.
  EXPECT_STREQ("4K", CStreamDetails::VideoDimsToResolutionDescription(3840, 1632).c_str());
}

TEST(TestStreamDetails, VideoDimsToResolutionDescriptionEdgeCases)
{
  EXPECT_STREQ("", CStreamDetails::VideoDimsToResolutionDescription(0, 1080).c_str());
  EXPECT_STREQ("", CStreamDetails::VideoDimsToResolutionDescription(1920, 0).c_str());
  EXPECT_STREQ("", CStreamDetails::VideoDimsToResolutionDescription(0, 0).c_str());

  // Anything past the widest bucket is not described at all, rather than clamped to 8K.
  EXPECT_STREQ("", CStreamDetails::VideoDimsToResolutionDescription(9000, 5000).c_str());

  // Each bucket bounds width AND height, so portrait content falls through to whichever
  // bucket is tall enough to hold it. 1080x1920 is described as 4K.
  EXPECT_STREQ("4K", CStreamDetails::VideoDimsToResolutionDescription(1080, 1920).c_str());
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
  EXPECT_STREQ("1.43", CStreamDetails::VideoAspectToAspectDescription(1.43f).c_str());
  EXPECT_STREQ("1.50", CStreamDetails::VideoAspectToAspectDescription(1.50f).c_str());
  EXPECT_STREQ("1.66", CStreamDetails::VideoAspectToAspectDescription(1.66f).c_str());
  EXPECT_STREQ("1.78", CStreamDetails::VideoAspectToAspectDescription(1.78f).c_str());
  EXPECT_STREQ("1.85", CStreamDetails::VideoAspectToAspectDescription(1.85f).c_str());
  EXPECT_STREQ("1.90", CStreamDetails::VideoAspectToAspectDescription(1.90f).c_str());
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

  EXPECT_STREQ("1.37", CStreamDetails::VideoAspectToAspectDescription(1.3987f).c_str());
  EXPECT_STREQ("1.43", CStreamDetails::VideoAspectToAspectDescription(1.4007f).c_str());

  EXPECT_STREQ("1.43", CStreamDetails::VideoAspectToAspectDescription(1.4636f).c_str());
  EXPECT_STREQ("1.50", CStreamDetails::VideoAspectToAspectDescription(1.4656f).c_str());

  EXPECT_STREQ("1.50", CStreamDetails::VideoAspectToAspectDescription(1.5770f).c_str());
  EXPECT_STREQ("1.66", CStreamDetails::VideoAspectToAspectDescription(1.5790f).c_str());

  EXPECT_STREQ("1.66", CStreamDetails::VideoAspectToAspectDescription(1.7180f).c_str());
  EXPECT_STREQ("1.78", CStreamDetails::VideoAspectToAspectDescription(1.7200f).c_str());

  EXPECT_STREQ("1.78", CStreamDetails::VideoAspectToAspectDescription(1.8137f).c_str());
  EXPECT_STREQ("1.85", CStreamDetails::VideoAspectToAspectDescription(1.8157f).c_str());

  EXPECT_STREQ("1.85", CStreamDetails::VideoAspectToAspectDescription(1.8738f).c_str());
  EXPECT_STREQ("1.90", CStreamDetails::VideoAspectToAspectDescription(1.8758f).c_str());

  EXPECT_STREQ("1.90", CStreamDetails::VideoAspectToAspectDescription(1.9484f).c_str());
  EXPECT_STREQ("2.00", CStreamDetails::VideoAspectToAspectDescription(1.9504f).c_str());

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

TEST(TestStreamDetails, VideoAspectToAspectDescriptionRealContent)
{
  // Measured aspects taken from real releases rather than from the table itself.
  EXPECT_STREQ("1.43",
               CStreamDetails::VideoAspectToAspectDescription(1.4375f).c_str()); // IMAX 15/70
  EXPECT_STREQ("1.90", CStreamDetails::VideoAspectToAspectDescription(2048.0f / 1080.0f)
                           .c_str()); // DCI full container
  EXPECT_STREQ("1.78", CStreamDetails::VideoAspectToAspectDescription(1920.0f / 1080.0f).c_str());
  EXPECT_STREQ("2.35", CStreamDetails::VideoAspectToAspectDescription(3840.0f / 1632.0f).c_str());
}

TEST(TestStreamDetails, VideoAspectToAspectDescriptionRelabelledRanges)
{
  // Adding a ratio necessarily moves the cutoffs either side of it, so these four ranges are
  // reported differently than they were before 1.43, 1.50 and 1.90 existed in the table. Each
  // is a correction - content at 1.90 is not 1.85 - but the change is user visible, so pin it.
  EXPECT_STREQ("1.43", CStreamDetails::VideoAspectToAspectDescription(1.42f).c_str()); // was 1.37
  EXPECT_STREQ("1.50", CStreamDetails::VideoAspectToAspectDescription(1.48f).c_str()); // was 1.37
  EXPECT_STREQ("1.50", CStreamDetails::VideoAspectToAspectDescription(1.55f).c_str()); // was 1.66
  EXPECT_STREQ("1.90", CStreamDetails::VideoAspectToAspectDescription(1.90f).c_str()); // was 1.85
}
