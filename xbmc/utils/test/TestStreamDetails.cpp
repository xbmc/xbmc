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
  video->SetSource(CStreamDetail::MEDIA);

  audio->m_iChannels = 2;
  audio->m_strCodec = "aac";
  audio->m_strLanguage = "eng";
  audio->SetSource(CStreamDetail::MEDIA);

  subtitle->m_strLanguage = "eng";
  subtitle->SetSource(CStreamDetail::MEDIA);

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
  EXPECT_EQ(CStreamDetail::MEDIA, a.GetSources());

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

namespace
{
// Builds a CStreamDetails with one video stream (h264, 1920x1080, 1.78, 5400s),
// one audio stream (aac, eng, 6 ch) and one subtitle stream (eng).
// Each stream type receives its own source value so individual source
// differences can be isolated per test.
CStreamDetails MakeTypicalStreamDetails(CStreamDetail::Source videoSrc,
                                        CStreamDetail::Source audioSrc,
                                        CStreamDetail::Source subtitleSrc)
{
  CStreamDetails details;

  auto* video = new CStreamDetailVideo();
  video->m_strCodec = "h264";
  video->m_iWidth = 1920;
  video->m_iHeight = 1080;
  video->m_fAspect = 1.78f;
  video->m_iDuration = 5400;
  video->m_strStereoMode = "left_right";
  video->m_strLanguage = "eng";
  video->SetSource(videoSrc);
  details.AddStream(video);

  auto* audio = new CStreamDetailAudio();
  audio->m_strCodec = "aac";
  audio->m_strLanguage = "eng";
  audio->m_iChannels = 6;
  audio->SetSource(audioSrc);
  details.AddStream(audio);

  auto* subtitle = new CStreamDetailSubtitle();
  subtitle->m_strLanguage = "eng";
  subtitle->SetSource(subtitleSrc);
  details.AddStream(subtitle);

  details.DetermineBestStreams();
  return details;
}

// Convenience overload: all streams get the same source
CStreamDetails MakeTypicalStreamDetails(CStreamDetail::Source source)
{
  return MakeTypicalStreamDetails(source, source, source);
}
} // namespace

TEST(TestStreamDetails, Equality_BothEmpty)
{
  // Two default-constructed objects carry no streams, so there is nothing
  // for the source comparison to trip on; they must be equal.
  const CStreamDetails a;
  const CStreamDetails b;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(TestStreamDetails, Equality_SelfComparison)
{
  // operator== has a `this == &right` identity fast-path - verify it fires
  // and that operator!= is its consistent negation.
  const CStreamDetails a = MakeTypicalStreamDetails(CStreamDetail::MEDIA);
  EXPECT_TRUE(a == a);
  EXPECT_FALSE(a != a);
}

TEST(TestStreamDetails, Equality_IdenticalContentAndSource)
{
  // Baseline: two independently built objects with the same content and the
  // same source must compare equal.
  const CStreamDetails a = MakeTypicalStreamDetails(CStreamDetail::MEDIA);
  const CStreamDetails b = MakeTypicalStreamDetails(CStreamDetail::MEDIA);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(TestStreamDetails, Equality_SameContentDifferentVideoSource_MediaVsNfo)
{
  // Content-identical objects whose VIDEO stream carries different sources
  // (MEDIA vs NFO) must NOT be equal.

  const CStreamDetails media =
      MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::MEDIA, CStreamDetail::MEDIA);
  const CStreamDetails nfo =
      MakeTypicalStreamDetails(CStreamDetail::NFO, CStreamDetail::MEDIA, CStreamDetail::MEDIA);

  EXPECT_FALSE(media == nfo);
  EXPECT_TRUE(media != nfo);
}

TEST(TestStreamDetails, Equality_SameContentDifferentAudioSource_MediaVsNfo)
{
  // As above, but only the AUDIO stream's source differs; the video and
  // subtitle sources match. Verifies the audio loop in operator== is reached.
  const CStreamDetails media =
      MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::MEDIA, CStreamDetail::MEDIA);
  const CStreamDetails nfo =
      MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::NFO, CStreamDetail::MEDIA);

  EXPECT_FALSE(media == nfo);
  EXPECT_TRUE(media != nfo);
}

TEST(TestStreamDetails, Equality_SameContentDifferentSubtitleSource_MediaVsNfo)
{
  // As above, but only the SUBTITLE stream's source differs. Verifies the
  // subtitle loop in operator== is reached even when video and audio match.
  const CStreamDetails media =
      MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::MEDIA, CStreamDetail::MEDIA);
  const CStreamDetails nfo =
      MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::MEDIA, CStreamDetail::NFO);

  EXPECT_FALSE(media == nfo);
  EXPECT_TRUE(media != nfo);
}

TEST(TestStreamDetails, Equality_BothNfoSource_SameContent)
{
  // When both sides have source=NFO and identical content they must still
  // compare equal.
  const CStreamDetails a = MakeTypicalStreamDetails(CStreamDetail::NFO);
  const CStreamDetails b = MakeTypicalStreamDetails(CStreamDetail::NFO);

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(TestStreamDetails, Equality_DifferentContentSameSource)
{
  // Content differs while source is identical - must NOT be equal.
  const CStreamDetails a =
      MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::MEDIA, CStreamDetail::MEDIA);

  CStreamDetails b;

  auto* video = new CStreamDetailVideo();
  video->m_strCodec = "hevc";
  video->m_iWidth = 1920;
  video->m_iHeight = 1080;
  video->m_fAspect = 1.78f;
  video->m_iDuration = 5400;
  video->m_strStereoMode = "left_right";
  video->m_strLanguage = "eng";
  video->SetSource(CStreamDetail::MEDIA);
  b.AddStream(video);

  auto* audio = new CStreamDetailAudio();
  audio->m_strCodec = "aac";
  audio->m_strLanguage = "eng";
  audio->m_iChannels = 6;
  audio->SetSource(CStreamDetail::MEDIA);
  b.AddStream(audio);

  auto* subtitle = new CStreamDetailSubtitle();
  subtitle->m_strLanguage = "eng";
  subtitle->SetSource(CStreamDetail::MEDIA);
  b.AddStream(subtitle);

  b.DetermineBestStreams();

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}
