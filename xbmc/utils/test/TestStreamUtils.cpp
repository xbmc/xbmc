/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StreamUtils.h"

#include <gtest/gtest.h>

TEST(TestStreamUtils, General)
{
  EXPECT_EQ(0, StreamUtils::GetCodecPriority(""));
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("ac3"));
  EXPECT_EQ(2, StreamUtils::GetCodecPriority("dts"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("eac3"));
  EXPECT_EQ(4, StreamUtils::GetCodecPriority("eac3_ddp_atmos"));
  EXPECT_EQ(5, StreamUtils::GetCodecPriority("dtshd_hra"));
  // The lossless codecs are equally good within a group and must not be ordered against each
  // other, but those that can carry the object-based extensions rank above those that cannot.
  EXPECT_EQ(6, StreamUtils::GetCodecPriority("flac"));
  EXPECT_EQ(6, StreamUtils::GetCodecPriority("pcm_bluray"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("dtshd_ma"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("truehd"));

  // The object-based codecs are rival systems rather than tiers, so they are not ordered against
  // each other either.
  EXPECT_EQ(8, StreamUtils::GetCodecPriority("dtshd_ma_x"));
  EXPECT_EQ(8, StreamUtils::GetCodecPriority("dtshd_ma_x_imax"));
  EXPECT_EQ(8, StreamUtils::GetCodecPriority("truehd_atmos"));
}

TEST(TestStreamUtils, CompareAudioQuality_CodecBeatsChannelsBeyondStereo)
{
  // Beyond stereo the codec is the better description of the stream, so the lossless 5.1 track
  // is the better listen than the lossy one carrying more channels.
  EXPECT_GT(StreamUtils::CompareAudioQuality("truehd", 6, "ac3", 8), 0);
  EXPECT_LT(StreamUtils::CompareAudioQuality("ac3", 8, "truehd", 6), 0);

  // Codecs of equal rank leave the channel count to decide
  EXPECT_GT(StreamUtils::CompareAudioQuality("truehd", 8, "dtshd_ma", 6), 0);
  EXPECT_EQ(StreamUtils::CompareAudioQuality("truehd", 6, "dtshd_ma", 6), 0);
}

TEST(TestStreamUtils, CompareAudioQuality_ChannelsWinAtOrBelowStereo)
{
  // The codec-first rule applies only when both streams are beyond stereo - the step up from
  // stereo to surround outweighs any codec difference.
  EXPECT_GT(StreamUtils::CompareAudioQuality("ac3", 6, "truehd", 2), 0);
  EXPECT_LT(StreamUtils::CompareAudioQuality("flac", 2, "ac3", 6), 0);

  // Both stereo, so the codec decides
  EXPECT_GT(StreamUtils::CompareAudioQuality("flac", 2, "ac3", 2), 0);
  EXPECT_EQ(StreamUtils::CompareAudioQuality("ac3", 2, "ac3", 2), 0);
}

TEST(TestStreamUtils, CompareAudioQuality_UnknownChannelCountIsThePoorestChoice)
{
  // A channel count of zero or less means unknown rather than none, so such a stream must not be
  // promoted by carrying a good codec.
  EXPECT_LT(StreamUtils::CompareAudioQuality("truehd", 0, "ac3", 6), 0);
  EXPECT_LT(StreamUtils::CompareAudioQuality("truehd", -1, "ac3", 2), 0);
  EXPECT_EQ(StreamUtils::CompareAudioQuality("truehd", 0, "truehd", 0), 0);
}

TEST(TestStreamUtils, CompareAudioQuality_UnknownChannelCountSentinelsRankEqually)
{
  // The sources disagree on how to say "unknown": the player reports 0 whereas an NFO without a
  // <channels> element or a NULL database column yields -1. Both mean the same thing, so neither
  // sentinel may outrank the other and the codec has to decide.
  EXPECT_GT(StreamUtils::CompareAudioQuality("truehd", -1, "ac3", 0), 0);
  EXPECT_LT(StreamUtils::CompareAudioQuality("ac3", 0, "truehd", -1), 0);

  // Same codec, so two differently spelled unknowns are indistinguishable
  EXPECT_EQ(StreamUtils::CompareAudioQuality("ac3", 0, "ac3", -1), 0);
  EXPECT_EQ(StreamUtils::CompareAudioQuality("ac3", -1, "ac3", 0), 0);

  // And an unknown count still loses to any known one, however it is spelled
  EXPECT_LT(StreamUtils::CompareAudioQuality("truehd", -1, "ac3", 1), 0);
  EXPECT_LT(StreamUtils::CompareAudioQuality("truehd", 0, "ac3", 1), 0);
}
