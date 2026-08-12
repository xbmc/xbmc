/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LanguageTag.h"
#include "utils/StreamDetails.h"
#include "utils/Variant.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using KODI::UTILS::CLanguageTag;

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

namespace
{
// Builds a CStreamDetails carrying the given audio streams and returns the codec of the one
// picked as best, so that the choice can be asserted the way the GUI observes it.
std::string BestAudioCodecOf(const std::vector<std::pair<std::string, int>>& codecsAndChannels)
{
  CStreamDetails details;
  for (const auto& [codec, channels] : codecsAndChannels)
  {
    auto* audio = new CStreamDetailAudio();
    audio->m_strCodec = codec;
    audio->m_iChannels = channels;
    audio->m_strLanguage = "eng";
    audio->SetSource(CStreamDetail::MEDIA);
    details.AddStream(audio);
  }
  details.DetermineBestStreams();
  return details.GetAudioCodec();
}

// As above, but reporting the channel count of the stream picked as best.
int BestAudioChannelsOf(const std::vector<std::pair<std::string, int>>& codecsAndChannels)
{
  CStreamDetails details;
  for (const auto& [codec, channels] : codecsAndChannels)
  {
    auto* audio = new CStreamDetailAudio();
    audio->m_strCodec = codec;
    audio->m_iChannels = channels;
    audio->m_strLanguage = "eng";
    audio->SetSource(CStreamDetail::MEDIA);
    details.AddStream(audio);
  }
  details.DetermineBestStreams();
  return details.GetAudioChannels();
}
} // namespace

TEST(TestStreamDetails, BestAudio_CodecBeatsChannelsBeyondStereo)
{
  // Beyond stereo the codec is the better description of the stream, so a lossless 5.1 track
  // is preferred over a lossy one carrying more channels.
  EXPECT_EQ("truehd", BestAudioCodecOf({{"ac3", 8}, {"truehd", 6}}));
  EXPECT_EQ("truehd", BestAudioCodecOf({{"truehd", 6}, {"ac3", 8}})); // order must not matter
  EXPECT_EQ("dtshd_ma", BestAudioCodecOf({{"dts", 8}, {"dtshd_ma", 6}}));
  EXPECT_EQ("eac3", BestAudioCodecOf({{"ac3", 8}, {"eac3", 6}}));

  // Uncompressed LPCM is lossless, so a disc offering it alongside lossy tracks of the same
  // width must not report one of those instead (which would show a foreign language track as
  // the item's audio when the LPCM one is the disc's own default).
  EXPECT_EQ("pcm_bluray", BestAudioCodecOf({{"pcm_bluray", 6}, {"dts", 6}}));
  EXPECT_EQ("pcm_bluray", BestAudioCodecOf({{"dts", 6}, {"pcm_bluray", 6}}));
  EXPECT_EQ("pcm_bluray", BestAudioCodecOf({{"dtshd_hra", 8}, {"pcm_bluray", 6}}));

  // But it remains the poorest of the lossless codecs.
  EXPECT_EQ("dtshd_ma", BestAudioCodecOf({{"pcm_bluray", 6}, {"dtshd_ma", 6}}));
  EXPECT_EQ("truehd", BestAudioCodecOf({{"pcm_bluray", 8}, {"truehd", 6}}));
}

TEST(TestStreamDetails, BestAudio_ChannelsBreakACodecTie)
{
  // Same codec on both sides leaves the channel count to decide.
  EXPECT_EQ(8, BestAudioChannelsOf({{"ac3", 6}, {"ac3", 8}}));
  EXPECT_EQ(8, BestAudioChannelsOf({{"ac3", 8}, {"ac3", 6}}));

  // The 5.1 TrueHD track wins on codec, and reports its own channel count with it.
  EXPECT_EQ(6, BestAudioChannelsOf({{"ac3", 8}, {"truehd", 6}}));
}

TEST(TestStreamDetails, BestAudio_ChannelsDecideWithinALosslessGroup)
{
  // The codecs of a lossless group all reconstruct the original samples, so none of them
  // describes a better stream than the others and the channel count is left to decide - a 7.1
  // track is not passed over for a 5.1 one carrying the same audio.
  EXPECT_EQ(8, BestAudioChannelsOf({{"truehd", 8}, {"dtshd_ma", 6}}));
  EXPECT_EQ(8, BestAudioChannelsOf({{"dtshd_ma", 6}, {"truehd", 8}}));
  EXPECT_EQ(8, BestAudioChannelsOf({{"flac", 8}, {"pcm_bluray", 6}}));
  EXPECT_EQ(8, BestAudioChannelsOf({{"pcm_bluray", 6}, {"flac", 8}}));

  // The same of the object-based codecs, which are rival systems rather than tiers.
  EXPECT_EQ(8, BestAudioChannelsOf({{"truehd_atmos", 8}, {"dtshd_ma_x", 6}}));
  EXPECT_EQ(8, BestAudioChannelsOf({{"dtshd_ma_x", 6}, {"truehd_atmos", 8}}));
  EXPECT_EQ(8, BestAudioChannelsOf({{"dtshd_ma_x_imax", 8}, {"truehd_atmos", 6}}));
}

TEST(TestStreamDetails, BestAudio_LosslessCarryingExtensionsOutranksLosslessWithout)
{
  // A truehd or dtshd_ma stream may be carrying objects that were never spotted, so it describes
  // the item better than an equally lossless stream that cannot carry them, whatever the widths.
  EXPECT_EQ("truehd", BestAudioCodecOf({{"flac", 8}, {"truehd", 6}}));
  EXPECT_EQ("truehd", BestAudioCodecOf({{"truehd", 6}, {"flac", 8}})); // order must not matter
  EXPECT_EQ("dtshd_ma", BestAudioCodecOf({{"pcm_bluray", 8}, {"dtshd_ma", 6}}));

  // And where the extensions were spotted, those codecs outrank both groups.
  EXPECT_EQ("truehd_atmos", BestAudioCodecOf({{"truehd", 8}, {"truehd_atmos", 6}}));
  EXPECT_EQ("dtshd_ma_x", BestAudioCodecOf({{"flac", 8}, {"dtshd_ma_x", 6}}));
}

TEST(TestStreamDetails, BestAudio_ChannelsStillWinAtOrBelowStereo)
{
  // The codec-first rule applies only when both streams are beyond stereo - a multichannel
  // track is still the better choice than a stereo one of any codec.
  EXPECT_EQ("ac3", BestAudioCodecOf({{"flac", 2}, {"ac3", 6}}));
  EXPECT_EQ("ac3", BestAudioCodecOf({{"truehd", 2}, {"ac3", 6}}));

  // Both stereo, so the codec decides as it always did.
  EXPECT_EQ("flac", BestAudioCodecOf({{"ac3", 2}, {"flac", 2}}));
}

TEST(TestStreamDetails, BestAudio_UnknownChannelCountIsThePoorestChoice)
{
  // Zero channels means unknown rather than none, so such a stream must not be promoted by
  // carrying a good codec - anything with a known count describes the item better.
  EXPECT_EQ("ac3", BestAudioCodecOf({{"truehd", 0}, {"ac3", 6}}));
  EXPECT_EQ("ac3", BestAudioCodecOf({{"truehd", 0}, {"ac3", 2}}));

  // With nothing else to go on the unknown stream is still reported.
  EXPECT_EQ("truehd", BestAudioCodecOf({{"truehd", 0}}));
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

TEST(TestStreamDetails, GetSource_PerStreamAndOutOfRange)
{
  // GetSource() addresses an individual stream; index 1 is the first stream of
  // that type. An index with no matching stream must report UNDEFINED rather
  // than the item's overall source.
  const CStreamDetails details =
      MakeTypicalStreamDetails(CStreamDetail::NFO, CStreamDetail::MEDIA, CStreamDetail::EXTERNAL);

  EXPECT_EQ(CStreamDetail::NFO, details.GetSource(CStreamDetail::VIDEO, 1));
  EXPECT_EQ(CStreamDetail::MEDIA, details.GetSource(CStreamDetail::AUDIO, 1));
  EXPECT_EQ(CStreamDetail::EXTERNAL, details.GetSource(CStreamDetail::SUBTITLE, 1));

  EXPECT_EQ(CStreamDetail::UNDEFINED, details.GetSource(CStreamDetail::VIDEO, 2));
  EXPECT_EQ(CStreamDetail::UNDEFINED, details.GetSource(CStreamDetail::AUDIO, 99));
}

TEST(TestStreamDetails, GetSources_Empty)
{
  // With no streams at all there is no source to report.
  const CStreamDetails details;
  EXPECT_EQ(CStreamDetail::UNDEFINED, details.GetSources());
}

TEST(TestStreamDetails, GetSources_ReturnsHighestOfMixedSources)
{
  // GetSources() collapses the per-stream sources to the single highest-ranked
  // one, so an item is only as overwritable as its most authoritative stream.
  EXPECT_EQ(CStreamDetail::NFO, MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::NFO,
                                                         CStreamDetail::EXTERNAL)
                                    .GetSources());

  EXPECT_EQ(CStreamDetail::MEDIA,
            MakeTypicalStreamDetails(CStreamDetail::UNDEFINED, CStreamDetail::EXTERNAL,
                                     CStreamDetail::MEDIA)
                .GetSources());

  EXPECT_EQ(
      CStreamDetail::LEGACY,
      MakeTypicalStreamDetails(CStreamDetail::LEGACY, CStreamDetail::MEDIA, CStreamDetail::MEDIA)
          .GetSources());
}

TEST(TestStreamDetails, SetSources_AppliesToEveryStream)
{
  // SetSources() overwrites the source of every stream, whatever it was before.
  CStreamDetails details =
      MakeTypicalStreamDetails(CStreamDetail::MEDIA, CStreamDetail::EXTERNAL, CStreamDetail::MEDIA);
  details.SetSources(CStreamDetail::NFO);

  EXPECT_EQ(CStreamDetail::NFO, details.GetSource(CStreamDetail::VIDEO, 1));
  EXPECT_EQ(CStreamDetail::NFO, details.GetSource(CStreamDetail::AUDIO, 1));
  EXPECT_EQ(CStreamDetail::NFO, details.GetSource(CStreamDetail::SUBTITLE, 1));
  EXPECT_EQ(CStreamDetail::NFO, details.GetSources());
}

TEST(TestStreamDetails, ShouldUpdateWithNewDetails_HigherOrEqualSourceWins)
{
  // New details replace existing ones when their source ranks at least as high.
  const CStreamDetails media = MakeTypicalStreamDetails(CStreamDetail::MEDIA);
  const CStreamDetails nfo = MakeTypicalStreamDetails(CStreamDetail::NFO);
  const CStreamDetails external = MakeTypicalStreamDetails(CStreamDetail::EXTERNAL);

  // Equal source - an update of like with like is allowed
  EXPECT_TRUE(media.ShouldUpdateWithNewDetails(media));

  // Higher-ranked new source - allowed
  EXPECT_TRUE(media.ShouldUpdateWithNewDetails(nfo));
  EXPECT_TRUE(external.ShouldUpdateWithNewDetails(media));

  // Lower-ranked new source - refused
  EXPECT_FALSE(nfo.ShouldUpdateWithNewDetails(media));
  EXPECT_FALSE(media.ShouldUpdateWithNewDetails(external));
}

TEST(TestStreamDetails, ShouldUpdateWithNewDetails_LegacyIsNeverOverwritten)
{
  // Details migrated from a pre-source-tracking library are ranked LEGACY, above
  // every other source, because they may themselves have come from an nfo. Nothing
  // silently replaces them; only an explicit rescan (which writes directly) can.
  const CStreamDetails legacy = MakeTypicalStreamDetails(CStreamDetail::LEGACY);

  EXPECT_FALSE(legacy.ShouldUpdateWithNewDetails(MakeTypicalStreamDetails(CStreamDetail::MEDIA)));
  EXPECT_FALSE(legacy.ShouldUpdateWithNewDetails(MakeTypicalStreamDetails(CStreamDetail::NFO)));
  EXPECT_FALSE(
      legacy.ShouldUpdateWithNewDetails(MakeTypicalStreamDetails(CStreamDetail::EXTERNAL)));
  EXPECT_TRUE(legacy.ShouldUpdateWithNewDetails(legacy));
}

TEST(TestStreamDetails, ShouldUpdateWithNewDetails_EmptyDetails)
{
  // An item with no details yet accepts anything, and nothing is ever replaced by
  // an empty set (UNDEFINED ranks below every real source).
  const CStreamDetails empty;
  const CStreamDetails media = MakeTypicalStreamDetails(CStreamDetail::MEDIA);

  EXPECT_TRUE(empty.ShouldUpdateWithNewDetails(media));
  EXPECT_FALSE(media.ShouldUpdateWithNewDetails(empty));
}

TEST(TestStreamDetails, ShouldUpdateWithNewDetails_MixedSourcesUseHighest)
{
  // Only the highest-ranked stream of each side is considered, so a single NFO
  // stream protects the whole item.
  const CStreamDetails partialNfo =
      MakeTypicalStreamDetails(CStreamDetail::NFO, CStreamDetail::MEDIA, CStreamDetail::MEDIA);
  const CStreamDetails allMedia = MakeTypicalStreamDetails(CStreamDetail::MEDIA);

  EXPECT_FALSE(partialNfo.ShouldUpdateWithNewDetails(allMedia));
  EXPECT_TRUE(allMedia.ShouldUpdateWithNewDetails(partialNfo));
}

TEST(TestStreamDetails, Version_DefaultsToCurrentAndSurvivesCopy)
{
  // Every newly created stream carries the current version, and copying an item
  // (as happens whenever a CVideoInfoTag is copied) must preserve it.
  const CStreamDetails details = MakeTypicalStreamDetails(CStreamDetail::MEDIA);

  EXPECT_EQ(CStreamDetail::STREAM_DETAILS_VERSION, details.GetVersion(CStreamDetail::VIDEO, 1));
  EXPECT_EQ(CStreamDetail::STREAM_DETAILS_VERSION, details.GetVersion(CStreamDetail::AUDIO, 1));
  EXPECT_EQ(CStreamDetail::STREAM_DETAILS_VERSION, details.GetVersion(CStreamDetail::SUBTITLE, 1));

  const CStreamDetails copy{details};
  EXPECT_EQ(CStreamDetail::STREAM_DETAILS_VERSION, copy.GetVersion(CStreamDetail::VIDEO, 1));
  EXPECT_EQ(CStreamDetail::MEDIA, copy.GetSource(CStreamDetail::VIDEO, 1));
}

TEST(TestStreamDetails, Version_AbsentStreamReportsZero)
{
  // A stream that isn't there has no version, which must not be confused with
  // version 1 (the marker for details that predate source tracking).
  const CStreamDetails empty;
  EXPECT_EQ(0, empty.GetVersion(CStreamDetail::VIDEO, 1));
  EXPECT_EQ(0, MakeTypicalStreamDetails(CStreamDetail::MEDIA).GetVersion(CStreamDetail::VIDEO, 2));
}

TEST(TestStreamDetails, Source_SurvivesCopyAssignment)
{
  // CStreamDetailVideo and CStreamDetailSubtitle define their own operator=, which
  // must carry the source and version across along with the stream data.
  CStreamDetailVideo video;
  video.m_strCodec = "h264";
  video.SetSource(CStreamDetail::NFO);

  CStreamDetailVideo videoCopy;
  videoCopy = video;
  EXPECT_EQ(CStreamDetail::NFO, videoCopy.GetSource());
  EXPECT_EQ(CStreamDetail::STREAM_DETAILS_VERSION, videoCopy.GetVersion());

  CStreamDetailSubtitle subtitle;
  subtitle.m_strLanguage = "eng";
  subtitle.SetSource(CStreamDetail::EXTERNAL);

  CStreamDetailSubtitle subtitleCopy;
  subtitleCopy = subtitle;
  EXPECT_EQ(CStreamDetail::EXTERNAL, subtitleCopy.GetSource());
  EXPECT_EQ(CStreamDetail::STREAM_DETAILS_VERSION, subtitleCopy.GetVersion());
}

namespace
{
// Builds a CStreamDetails carrying audio and subtitle streams in the order given, as a bluray
// playlist's streams are stored in stream number order.
CStreamDetails MakeStreamDetailsWithLanguages(const std::vector<std::string>& audioLanguages,
                                              const std::vector<std::string>& subtitleLanguages)
{
  CStreamDetails details;
  for (const auto& language : audioLanguages)
  {
    auto* audio = new CStreamDetailAudio();
    audio->m_strCodec = "dtshd_ma";
    audio->m_iChannels = 6;
    audio->m_strLanguage = language;
    audio->SetSource(CStreamDetail::MEDIA);
    details.AddStream(audio);
  }
  for (const auto& language : subtitleLanguages)
  {
    auto* subtitle = new CStreamDetailSubtitle();
    subtitle->m_strLanguage = language;
    subtitle->SetSource(CStreamDetail::MEDIA);
    details.AddStream(subtitle);
  }
  details.DetermineBestStreams();
  return details;
}
} // namespace

TEST(TestStreamDetails, FirstLanguage_IsTheFirstStreamNotTheBestMatch)
{
  // Two bluray playlists of the same movie differing only in which stream they start on must
  // report different default languages, whatever the user's language preferences make "best".
  const CStreamDetails all{MakeStreamDetailsWithLanguages({"eng", "jpn"}, {"eng", "fra", "jpn"})};
  EXPECT_EQ("eng", all.GetFirstAudioLanguage());
  EXPECT_EQ("eng", all.GetFirstSubtitleLanguage());

  const CStreamDetails japanese{MakeStreamDetailsWithLanguages({"jpn", "eng"}, {"jpn", "eng"})};
  EXPECT_EQ("jpn", japanese.GetFirstAudioLanguage());
  EXPECT_EQ("jpn", japanese.GetFirstSubtitleLanguage());
}

TEST(TestStreamDetails, FirstLanguage_EmptyWhenThereIsNoSuchStream)
{
  const CStreamDetails none{MakeStreamDetailsWithLanguages({}, {})};
  EXPECT_EQ("", none.GetFirstAudioLanguage());
  EXPECT_EQ("", none.GetFirstSubtitleLanguage());

  // A playlist can offer audio without subtitles
  const CStreamDetails audioOnly{MakeStreamDetailsWithLanguages({"eng"}, {})};
  EXPECT_EQ("eng", audioOnly.GetFirstAudioLanguage());
  EXPECT_EQ("", audioOnly.GetFirstSubtitleLanguage());
}

TEST(TestStreamDetails, FirstLanguage_UnknownLanguageIsReportedAsEmpty)
{
  // A stream number table need not name a language, and an empty language must be passed
  // through rather than falling back to another stream.
  const CStreamDetails details{MakeStreamDetailsWithLanguages({"", "eng"}, {"", "eng"})};
  EXPECT_EQ("", details.GetFirstAudioLanguage());
  EXPECT_EQ("", details.GetFirstSubtitleLanguage());
}

namespace
{
// Builds a CStreamDetails carrying the given audio streams, in the order the source presented
// them, so that the index of the preferred stream can be asserted.
CStreamDetails MakeAudioStreams(
    const std::vector<std::tuple<std::string, std::string, int>>& langsCodecsAndChannels)
{
  CStreamDetails details;
  for (const auto& [language, codec, channels] : langsCodecsAndChannels)
  {
    auto* audio = new CStreamDetailAudio();
    audio->m_strLanguage = language;
    audio->m_strCodec = codec;
    audio->m_iChannels = channels;
    audio->SetSource(CStreamDetail::MEDIA);
    details.AddStream(audio);
  }
  details.DetermineBestStreams();
  return details;
}
} // namespace

TEST(TestStreamDetails, PreferredAudio_BestStreamInThePreferredLanguage)
{
  // The disc carries a better German track than English one, but an English speaker will hear
  // the English one, so that is the stream to describe.
  const CStreamDetails details{
      MakeAudioStreams({{"ger", "truehd", 8}, {"eng", "ac3", 6}, {"eng", "dtshd_ma", 6}})};

  EXPECT_EQ(3, details.GetPreferredAudioStreamIndex("eng"));
  EXPECT_EQ("dtshd_ma", details.GetAudioCodec(details.GetPreferredAudioStreamIndex("eng")));

  EXPECT_EQ(1, details.GetPreferredAudioStreamIndex("ger"));
  EXPECT_EQ("truehd", details.GetAudioCodec(details.GetPreferredAudioStreamIndex("ger")));
}

TEST(TestStreamDetails, PreferredAudio_IndexCountsAllAudioStreams)
{
  // The index is an ordinal into every audio stream, not just the matching ones, or it would not
  // address the stream it named.
  const CStreamDetails details{
      MakeAudioStreams({{"fra", "ac3", 6}, {"jpn", "ac3", 6}, {"eng", "ac3", 6}})};

  EXPECT_EQ(3, details.GetPreferredAudioStreamIndex("eng"));
  EXPECT_EQ("eng", details.GetAudioLanguage(details.GetPreferredAudioStreamIndex("eng")));
}

TEST(TestStreamDetails, PreferredAudio_LanguageIsMatchedAcrossISO639Forms)
{
  // A source may name a language in either ISO 639 form, and the setting is held in another, so
  // the two must be compared rather than string matched.
  const CStreamDetails details{MakeAudioStreams({{"ger", "ac3", 6}, {"en", "truehd", 6}})};

  EXPECT_EQ(2, details.GetPreferredAudioStreamIndex("eng"));
  EXPECT_EQ(1, details.GetPreferredAudioStreamIndex("deu"));
}

TEST(TestStreamDetails, PreferredAudio_FallsBackToTheBestStream)
{
  const CStreamDetails details{MakeAudioStreams({{"ger", "ac3", 6}, {"fra", "truehd", 6}})};

  // Nothing in the preferred language, so there is no better answer than the best listen the
  // file has to offer, which index 0 is.
  EXPECT_EQ(0, details.GetPreferredAudioStreamIndex("eng"));
  EXPECT_EQ("truehd", details.GetAudioCodec(details.GetPreferredAudioStreamIndex("eng")));

  // As when the preference cannot be expressed as a language at all
  EXPECT_EQ(0, details.GetPreferredAudioStreamIndex(""));
  EXPECT_EQ("truehd", details.GetAudioCodec(details.GetPreferredAudioStreamIndex("")));
}

TEST(TestStreamDetails, PreferredAudio_NoAudioStreams)
{
  const CStreamDetails details{MakeAudioStreams({})};
  EXPECT_EQ(0, details.GetPreferredAudioStreamIndex("eng"));
  EXPECT_EQ("", details.GetAudioCodec(details.GetPreferredAudioStreamIndex("eng")));
}

TEST(TestStreamDetails, FirstAudio_CodecAndChannelsComeFromTheFirstStream)
{
  // The disc starts on a modest Japanese track, the technically best stream is a German one, and
  // an English speaker would hear a third. Describing the disc default must not borrow from
  // either of the others.
  const CStreamDetails details{
      MakeAudioStreams({{"jpn", "ac3", 2}, {"ger", "truehd", 8}, {"eng", "dtshd_ma", 6}})};

  EXPECT_EQ("jpn", details.GetFirstAudioLanguage());
  EXPECT_EQ("ac3", details.GetFirstAudioCodec());
  EXPECT_EQ(2, details.GetFirstAudioChannels());

  // ie. not the technically best stream idx 0 describes
  EXPECT_EQ("truehd", details.GetAudioCodec());
  EXPECT_EQ(8, details.GetAudioChannels());

  // nor the stream the user's language preference would pick
  EXPECT_EQ("dtshd_ma", details.GetAudioCodec(details.GetPreferredAudioStreamIndex("eng")));
  EXPECT_EQ(6, details.GetAudioChannels(details.GetPreferredAudioStreamIndex("eng")));
}

TEST(TestStreamDetails, FirstAudio_CodecAndChannelsWhenThereIsNoSuchStream)
{
  // Matches GetAudioCodec()/GetAudioChannels() for a missing stream, so the GUI suppresses the
  // label rather than showing a placeholder.
  const CStreamDetails none{MakeAudioStreams({})};
  EXPECT_EQ("", none.GetFirstAudioCodec());
  EXPECT_EQ(-1, none.GetFirstAudioChannels());
}

TEST(TestStreamDetails, FirstAudio_UnknownCodecAndChannelsArePassedThrough)
{
  // A stream number table need not describe the stream fully, and the gap must be passed through
  // rather than falling back to another stream.
  const CStreamDetails details{MakeAudioStreams({{"", "", 0}, {"eng", "truehd", 8}})};
  EXPECT_EQ("", details.GetFirstAudioLanguage());
  EXPECT_EQ("", details.GetFirstAudioCodec());
  EXPECT_EQ(0, details.GetFirstAudioChannels());
}

TEST(TestStreamDetails, Flags_CopiedFromStreamInfo)
{
  // The scanner and the bluray parser both hand over a StreamInfo with the flags
  // already set; the detail must keep them rather than drop them on the floor.
  AudioStreamInfo audioInfo;
  audioInfo.language = CLanguageTag::Parse("eng");
  audioInfo.channels = 6;
  audioInfo.flags =
      static_cast<StreamFlags>(StreamFlags::FLAG_DEFAULT | StreamFlags::FLAG_ORIGINAL);

  SubtitleStreamInfo subtitleInfo;
  subtitleInfo.language = CLanguageTag::Parse("eng");
  subtitleInfo.flags = StreamFlags::FLAG_FORCED;

  CStreamDetails details;
  details.AddStream(new CStreamDetailAudio(audioInfo, CStreamDetail::MEDIA));
  details.AddStream(new CStreamDetailSubtitle(subtitleInfo, CStreamDetail::MEDIA));
  details.DetermineBestStreams();

  EXPECT_EQ(StreamFlags::FLAG_DEFAULT | StreamFlags::FLAG_ORIGINAL, details.GetAudioFlags(1));
  EXPECT_EQ(StreamFlags::FLAG_FORCED, details.GetSubtitleFlags(1));
}

TEST(TestStreamDetails, Flags_DefaultToNoneAndAbsentStreamReportsNone)
{
  // Details that predate the flags column read back with no flags set, which must
  // look the same as a stream that genuinely has none.
  const CStreamDetails empty;
  EXPECT_EQ(StreamFlags::FLAG_NONE, empty.GetAudioFlags(1));
  EXPECT_EQ(StreamFlags::FLAG_NONE, empty.GetSubtitleFlags(1));

  CStreamDetails details;
  details.AddStream(new CStreamDetailAudio());
  details.AddStream(new CStreamDetailSubtitle());
  details.DetermineBestStreams();

  EXPECT_EQ(StreamFlags::FLAG_NONE, details.GetAudioFlags(1));
  EXPECT_EQ(StreamFlags::FLAG_NONE, details.GetSubtitleFlags(1));
}

TEST(TestStreamDetails, Flags_SurviveCopy)
{
  AudioStreamInfo audioInfo;
  audioInfo.flags = StreamFlags::FLAG_VISUAL_IMPAIRED;

  SubtitleStreamInfo subtitleInfo;
  subtitleInfo.flags = StreamFlags::FLAG_HEARING_IMPAIRED;

  CStreamDetails details;
  details.AddStream(new CStreamDetailAudio(audioInfo, CStreamDetail::MEDIA));
  details.AddStream(new CStreamDetailSubtitle(subtitleInfo, CStreamDetail::MEDIA));
  details.DetermineBestStreams();

  const CStreamDetails copy{details};
  EXPECT_EQ(StreamFlags::FLAG_VISUAL_IMPAIRED, copy.GetAudioFlags(1));
  EXPECT_EQ(StreamFlags::FLAG_HEARING_IMPAIRED, copy.GetSubtitleFlags(1));

  // CStreamDetailSubtitle has its own operator=, which must carry the flags too.
  CStreamDetailSubtitle subtitle;
  subtitle.m_flags = StreamFlags::FLAG_FORCED;
  CStreamDetailSubtitle subtitleCopy;
  subtitleCopy = subtitle;
  EXPECT_EQ(StreamFlags::FLAG_FORCED, subtitleCopy.m_flags);
}

TEST(TestStreamDetails, Flags_ParticipateInEquality)
{
  // SaveFileStateJob only writes stream details back to the database when the
  // player's details differ from the stored ones, so a flags-only difference has
  // to register as a difference. Otherwise flags are never backfilled for items
  // scanned before the flags column existed.
  AudioStreamInfo audioInfo;
  audioInfo.codecName = "dts";
  audioInfo.language = CLanguageTag::Parse("eng");
  audioInfo.channels = 6;

  SubtitleStreamInfo subtitleInfo;
  subtitleInfo.language = CLanguageTag::Parse("eng");

  const auto makeDetails = [&](StreamFlags audioFlags, StreamFlags subtitleFlags)
  {
    AudioStreamInfo audio{audioInfo};
    audio.flags = audioFlags;
    SubtitleStreamInfo subtitle{subtitleInfo};
    subtitle.flags = subtitleFlags;

    CStreamDetails details;
    details.AddStream(new CStreamDetailAudio(audio, CStreamDetail::MEDIA));
    details.AddStream(new CStreamDetailSubtitle(subtitle, CStreamDetail::MEDIA));
    details.DetermineBestStreams();
    return details;
  };

  const CStreamDetails flagless = makeDetails(StreamFlags::FLAG_NONE, StreamFlags::FLAG_NONE);

  EXPECT_NE(flagless, makeDetails(StreamFlags::FLAG_DEFAULT, StreamFlags::FLAG_NONE));
  EXPECT_NE(flagless, makeDetails(StreamFlags::FLAG_NONE, StreamFlags::FLAG_FORCED));
  EXPECT_EQ(flagless, makeDetails(StreamFlags::FLAG_NONE, StreamFlags::FLAG_NONE));
}

TEST(TestStreamDetails, StreamFlagNames_EveryFlagRoundTrips)
{
  // A name missing from the table would be silently dropped on export, so the table has to
  // cover every StreamFlags bit and each name has to map back to the flag it came from.
  StreamFlags all{StreamFlags::FLAG_NONE};
  for (const auto& [flag, name] : CStreamDetails::STREAM_FLAG_NAMES)
  {
    EXPECT_EQ(flag, CStreamDetails::StreamFlagFromName(name)) << "name: " << name;
    all = static_cast<StreamFlags>(all | flag);
  }

  EXPECT_EQ(CStreamDetails::STREAM_FLAG_NAMES.size(),
            CStreamDetails::StreamFlagsToNames(all).size());
  EXPECT_TRUE(CStreamDetails::StreamFlagsToNames(StreamFlags::FLAG_NONE).empty());
}

TEST(TestStreamDetails, StreamFlagNames_UnknownAndUntidyNames)
{
  // NFOs are hand-edited, so leading/trailing space and casing must not matter, and a name
  // from a newer Kodi that this build doesn't know must be ignored rather than misread.
  EXPECT_EQ(StreamFlags::FLAG_DEFAULT, CStreamDetails::StreamFlagFromName("  DeFaUlT  "));
  EXPECT_EQ(StreamFlags::FLAG_NONE, CStreamDetails::StreamFlagFromName("notaflag"));
  EXPECT_EQ(StreamFlags::FLAG_NONE, CStreamDetails::StreamFlagFromName(""));
}

TEST(TestStreamDetails, StreamFlagNames_ReportedAlphabetically)
{
  // Names come out sorted regardless of the bit order they were set in, so an exported NFO
  // is stable and two items with the same flags produce byte-identical output.
  const std::vector<std::string> names =
      CStreamDetails::StreamFlagsToNames(static_cast<StreamFlags>(
          StreamFlags::FLAG_ORIGINAL | StreamFlags::FLAG_DEFAULT | StreamFlags::FLAG_FORCED));

  EXPECT_EQ(std::vector<std::string>({"default", "forced", "original"}), names);

  const std::vector<std::string> all = CStreamDetails::StreamFlagsToNames(
      static_cast<StreamFlags>(StreamFlags::FLAG_WEBVTT_DATA_PACKETS |
                               StreamFlags::FLAG_HEARING_IMPAIRED | StreamFlags::FLAG_COMMENT));

  EXPECT_EQ(std::vector<std::string>({"comment", "hearingimpaired", "webvttdatapackets"}), all);
}

// The classes store ISO 639-2/B, because the streamdetails table is filtered by smart playlist
// SQL, but JSON-RPC is served BCP 47. Serialize is where that widening happens.
TEST(TestStreamDetails, SerializeWidensLanguageToBcp47)
{
  CStreamDetailAudio audio;
  CStreamDetailSubtitle subtitle;
  CStreamDetailVideo video;

  CVariant value;

  // BCP 47 prefers the alpha-2 code where the language has one
  audio.m_strLanguage = "eng";
  audio.Serialize(value);
  EXPECT_EQ(value["language"].asString(), "en");

  // A language whose B and T forms differ still resolves to its alpha-2
  audio.m_strLanguage = "chi";
  audio.Serialize(value);
  EXPECT_EQ(value["language"].asString(), "zh");

  // One with no alpha-2 keeps its three letter form, so length cannot tell the notations apart
  subtitle.m_strLanguage = "ady";
  subtitle.Serialize(value);
  EXPECT_EQ(value["language"].asString(), "ady");

  // Anything the standards do not know is passed through rather than dropped
  video.m_strLanguage = "not a language";
  video.Serialize(value);
  EXPECT_EQ(value["language"].asString(), "not a language");

  subtitle.m_strLanguage = "";
  subtitle.Serialize(value);
  EXPECT_EQ(value["language"].asString(), "");
}
