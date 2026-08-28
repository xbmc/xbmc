/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StreamUtils.h"

#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

TEST(TestStreamUtils, General)
{
  // Nothing GetCodecName() can produce should land here - 0 means a codec genuinely nobody knows
  EXPECT_EQ(0, StreamUtils::GetCodecPriority(""));
  EXPECT_EQ(0, StreamUtils::GetCodecPriority("nellymoser"));

  // Oldest, or built for low bitrate secondary audio
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("mp3"));
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("mp2"));
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("mp1"));
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("wmav2"));
  EXPECT_EQ(1, StreamUtils::GetCodecPriority("dts_express"));

  // The HE profiles signal low bitrate content, so they rank with Dolby Digital rather than with
  // the Low Complexity profile above it
  EXPECT_EQ(2, StreamUtils::GetCodecPriority("ac3"));
  EXPECT_EQ(2, StreamUtils::GetCodecPriority("he_aac"));
  EXPECT_EQ(2, StreamUtils::GetCodecPriority("he_aac_v2"));

  // A bare aac is ranked with aac_lc rather than at the family floor, as that is overwhelmingly
  // what it turns out to be
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("aac_lc"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("aac_latm"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("aac"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("aac_ltp"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("aac_ssr"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("vorbis"));
  EXPECT_EQ(3, StreamUtils::GetCodecPriority("wmapro"));

  EXPECT_EQ(4, StreamUtils::GetCodecPriority("dts"));

  // Opus is as good a prospect as Dolby Digital Plus
  EXPECT_EQ(5, StreamUtils::GetCodecPriority("eac3"));
  EXPECT_EQ(5, StreamUtils::GetCodecPriority("opus"));

  // The DTS extensions carry a discrete channel or 96kHz/24-bit on top of the core, at bitrates
  // that outweigh what Dolby Digital Plus is shipped at - consistent with dtshd_hra outranking
  // eac3_ddp_atmos further up the table
  EXPECT_EQ(6, StreamUtils::GetCodecPriority("ac4"));
  EXPECT_EQ(6, StreamUtils::GetCodecPriority("dts_es"));
  EXPECT_EQ(6, StreamUtils::GetCodecPriority("dts_96_24"));

  // Uncompressed LPCM is lossless, so it outranks the lossy codecs. 16 bit, DVD LPCM and the form
  // with nothing known about it at all rank below the 24 bit and better forms.
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("pcm"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("pcm_s16le"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("pcm_dvd"));

  EXPECT_EQ(8, StreamUtils::GetCodecPriority("eac3_ddp_atmos"));

  // A better DTS-HD flavour would have been detected, so an undetermined one is ranked as the
  // poorest it could be rather than alongside High Resolution.
  EXPECT_EQ(9, StreamUtils::GetCodecPriority("dtshd"));
  EXPECT_EQ(10, StreamUtils::GetCodecPriority("dtshd_hra"));

  // The lossless codecs are equally good within a group and must not be ordered against each
  // other, but those that can be bit-streamed rank above those that must be decoded locally,
  // and those carrying the object-based extensions rank above both.
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("flac"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("alac"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("ape"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("wavpack"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("pcm_bluray"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("pcm_f32le"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("pcm_s32le"));
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("pcm_s24le"));
  EXPECT_EQ(12, StreamUtils::GetCodecPriority("dtshd_ma"));
  EXPECT_EQ(12, StreamUtils::GetCodecPriority("truehd"));
  EXPECT_EQ(12, StreamUtils::GetCodecPriority("mlp"));

  // The object-based codecs are rival systems rather than tiers, so they are not ordered against
  // each other either.
  EXPECT_EQ(13, StreamUtils::GetCodecPriority("dtshd_ma_x"));
  EXPECT_EQ(13, StreamUtils::GetCodecPriority("dtshd_ma_x_imax"));
  EXPECT_EQ(13, StreamUtils::GetCodecPriority("truehd_atmos"));

  // Relationships that must hold whatever the absolute numbers are
  EXPECT_GT(StreamUtils::GetCodecPriority("pcm_s24le"), StreamUtils::GetCodecPriority("pcm_s16le"));
  EXPECT_GT(StreamUtils::GetCodecPriority("dts_es"), StreamUtils::GetCodecPriority("eac3"));
  EXPECT_GT(StreamUtils::GetCodecPriority("eac3"), StreamUtils::GetCodecPriority("dts"));
  EXPECT_GT(StreamUtils::GetCodecPriority("dts"), StreamUtils::GetCodecPriority("aac_lc"));
  EXPECT_GT(StreamUtils::GetCodecPriority("aac_lc"), StreamUtils::GetCodecPriority("ac3"));
  EXPECT_GT(StreamUtils::GetCodecPriority("ac3"), StreamUtils::GetCodecPriority("mp3"));
}

TEST(TestStreamUtils, LpcmIsRankedOnBitDepthAlone)
{
  // DVDDemux recognises the whole ffmpeg PCM range as one codec, so the family is matched on
  // prefix. Byte order and planarity say nothing about quality - the same audio must not change
  // rank because it was written big-endian.
  for (const auto* name : {"pcm_s24le", "pcm_s24be", "pcm_s24le_planar", "pcm_s24daud", "pcm_s32le",
                           "pcm_s32be", "pcm_s64le", "pcm_f32le", "pcm_f32be", "pcm_f64le"})
    EXPECT_EQ(11, StreamUtils::GetCodecPriority(name)) << name;

  for (const auto* name : {"pcm_s16le", "pcm_s16be", "pcm_s16le_planar", "pcm_u16le"})
    EXPECT_EQ(7, StreamUtils::GetCodecPriority(name)) << name;

  // 8 bit, or companded to it, is the one LPCM form that really is poor
  for (const auto* name : {"pcm_s8", "pcm_u8", "pcm_alaw", "pcm_mulaw"})
    EXPECT_EQ(1, StreamUtils::GetCodecPriority(name)) << name;

  // The two disc forms keep their own ranks, and must not be swept up by the prefixes
  EXPECT_EQ(11, StreamUtils::GetCodecPriority("pcm_bluray"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("pcm_dvd"));
  EXPECT_EQ(7, StreamUtils::GetCodecPriority("pcm"));

  // Byte order never changes the answer
  EXPECT_EQ(StreamUtils::GetCodecPriority("pcm_s24le"), StreamUtils::GetCodecPriority("pcm_s24be"));
  EXPECT_EQ(StreamUtils::GetCodecPriority("pcm_s16le"), StreamUtils::GetCodecPriority("pcm_s16be"));
}

TEST(TestStreamUtils, EveryCodecKodiRecognisesIsRanked)
{
  // A codec Kodi knows about but the table does not is worse off than a truly unknown one, because
  // CompareAudioQuality() orders surround streams by codec first - an unranked 7.1 track loses to a
  // ranked 5.1 one. That is how the AAC family and then ac4 came to be missed, so assert the whole
  // set rather than waiting for review to spot the next addition.
  //
  // Keep in sync with the audio codecs StreamUtils::GetCodecName() names and those
  // CDemuxStreamAudio::GetStreamType() recognises.
  const std::vector<std::string> recognised{"ac3",
                                            "ac4",
                                            "eac3",
                                            "eac3_ddp_atmos",
                                            "dts",
                                            "dts_es",
                                            "dts_96_24",
                                            "dts_express",
                                            "dtshd",
                                            "dtshd_hra",
                                            "dtshd_ma",
                                            "dtshd_ma_x",
                                            "dtshd_ma_x_imax",
                                            "truehd",
                                            "truehd_atmos",
                                            "mlp",
                                            "flac",
                                            "alac",
                                            "wavpack",
                                            "opus",
                                            "vorbis",
                                            "mp2",
                                            "mp3",
                                            "wmav2",
                                            "wmapro",
                                            "aac",
                                            "aac_lc",
                                            "aac_ltp",
                                            "aac_ssr",
                                            "he_aac",
                                            "he_aac_v2",
                                            "pcm",
                                            "pcm_bluray",
                                            "pcm_dvd",
                                            "pcm_s16le",
                                            "pcm_s24le",
                                            "pcm_s32le",
                                            "pcm_f32le",
                                            "pcm_s16be",
                                            "pcm_s24be",
                                            "ape",
                                            "aac_latm",
                                            "mp1"};

  for (const auto& codec : recognised)
    EXPECT_GT(StreamUtils::GetCodecPriority(codec), 0) << codec << " is not ranked";
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

TEST(TestStreamUtils, CompareAudioQuality_UnknownCodecIsThePoorestSurroundChoice)
{
  // Every codec the stream details can carry is ranked, so a rank of 0 is a codec genuinely
  // nobody knows and belongs below the ones that are. This is only safe because the common names
  // are ranked - it is what the AAC family being unranked used to get wrong.
  EXPECT_GT(StreamUtils::CompareAudioQuality("ac3", 6, "nellymoser", 8), 0);
  EXPECT_GT(StreamUtils::CompareAudioQuality("aac_lc", 8, "ac3", 6), 0);
  EXPECT_GT(StreamUtils::CompareAudioQuality("opus", 6, "aac_lc", 8), 0);
}

TEST(TestStreamUtils, CompareAudioQuality_IsAStrictWeakOrdering)
{
  // VideoPlayer hands this comparison to std::stable_sort, which is undefined behaviour unless it
  // is a strict weak ordering. A pair-dependent choice of key breaks that, and no amount of spot
  // assertions will catch it, so walk the whole relation.
  const std::vector<std::pair<std::string, int>> candidates{
      {"truehd", 6},   {"ac3", 8},       {"aac_lc", 7}, {"nellymoser", 7}, {"opus", 6},
      {"dts", 8},      {"pcm_s16le", 2}, {"flac", 2},   {"mp3", 1},        {"eac3", 0},
      {"dtshd_ma", 8}, {"he_aac", 6},    {"pcm", 0},    {"dts_es", 6},     {"truehd", 2}};

  const auto better{
      [&candidates](size_t i, size_t j)
      {
        return StreamUtils::CompareAudioQuality(candidates[i].first, candidates[i].second,
                                                candidates[j].first, candidates[j].second) > 0;
      }};

  for (size_t i = 0; i < candidates.size(); ++i)
  {
    // Irreflexive - nothing is better than itself
    EXPECT_FALSE(better(i, i)) << candidates[i].first;

    for (size_t j = 0; j < candidates.size(); ++j)
    {
      // Antisymmetric - the result must simply invert when the arguments swap
      EXPECT_EQ(StreamUtils::CompareAudioQuality(candidates[i].first, candidates[i].second,
                                                 candidates[j].first, candidates[j].second),
                -StreamUtils::CompareAudioQuality(candidates[j].first, candidates[j].second,
                                                  candidates[i].first, candidates[i].second))
          << candidates[i].first << " vs " << candidates[j].first;

      for (size_t k = 0; k < candidates.size(); ++k)
      {
        // Transitive - no cycles, for either "better than" or "equally good"
        if (better(i, j) && better(j, k))
        {
          EXPECT_TRUE(better(i, k))
              << candidates[i].first << " > " << candidates[j].first << " > " << candidates[k].first
              << " but not " << candidates[i].first << " > " << candidates[k].first;
        }

        if (!better(i, j) && !better(j, i) && !better(j, k) && !better(k, j))
        {
          EXPECT_TRUE(!better(i, k) && !better(k, i))
              << candidates[i].first << " == " << candidates[j].first
              << " == " << candidates[k].first;
        }
      }
    }
  }
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

TEST(TestStreamUtils, NormalizeAudioCodecName)
{
  EXPECT_EQ("dts", StreamUtils::NormalizeAudioCodecName("dca"));
  EXPECT_EQ("dts", StreamUtils::NormalizeAudioCodecName("dts"));
  EXPECT_EQ("ac3", StreamUtils::NormalizeAudioCodecName("ac3"));
  EXPECT_EQ("", StreamUtils::NormalizeAudioCodecName(""));
}
