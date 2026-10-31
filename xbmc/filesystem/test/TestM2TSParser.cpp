/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/bluray/M2TSParser.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
constexpr unsigned int PMT_PID = 0x100;
constexpr unsigned int VIDEO_PID = 0x1011;
constexpr unsigned int AUDIO_PID = 0x1100;

constexpr size_t TS_PAYLOAD_SIZE = 184;

struct ClipStream
{
  ENCODING_TYPE type;
  unsigned int pid;
};

// Writes an M2TS clip: a PAT, a PMT listing the streams, then PES packets. Each PES ends exactly
// where its data ends, using adaptation field stuffing in its last TS packet.
class CClipBuilder
{
public:
  explicit CClipBuilder(std::vector<ClipStream> streams) : m_streams(std::move(streams))
  {
    const auto pmtPidHigh{static_cast<uint8_t>(0xE0 | (PMT_PID >> 8))};
    const auto pmtPidLow{static_cast<uint8_t>(PMT_PID & 0xFF)};

    // The CRCs are not checked
    // clang-format off
    const std::vector<uint8_t> pat{
        0x00,                               // pointer_field
        0x00, 0xB0, 0x0D,                   // table_id, section_length 13
        0x00, 0x01, 0xC1, 0x00, 0x00,       // transport_stream_id, version, section numbers
        0x00, 0x01, pmtPidHigh, pmtPidLow,  // program 1
        0x00, 0x00, 0x00, 0x00};            // CRC_32
    std::vector<uint8_t> pmt{
        0x00,                               // pointer_field
        0x02, 0xB0, static_cast<uint8_t>(13 + 5 * m_streams.size()), // table_id, section_length
        0x00, 0x01, 0xC1, 0x00, 0x00,       // program_number, version, section numbers
        pmtPidHigh, pmtPidLow,              // PCR_PID
        0xF0, 0x00};                        // program_info_length
    // clang-format on
    WritePacket(0, true, pat);

    for (const auto& stream : m_streams)
    {
      pmt.insert(pmt.end(),
                 {static_cast<uint8_t>(stream.type), static_cast<uint8_t>(0xE0 | (stream.pid >> 8)),
                  static_cast<uint8_t>(stream.pid & 0xFF), 0xF0, 0x00});
    }
    pmt.insert(pmt.end(), {0x00, 0x00, 0x00, 0x00});
    WritePacket(PMT_PID, true, pmt);
  }

  void AddPES(unsigned int pid, const std::vector<uint8_t>& data)
  {
    std::vector<uint8_t> pes{PESHeader(pid)};
    pes.insert(pes.end(), data.begin(), data.end());

    const std::span<const uint8_t> remaining{pes};
    for (size_t offset = 0; offset < remaining.size(); offset += TS_PAYLOAD_SIZE)
    {
      WritePacket(pid, offset == 0,
                  remaining.subspan(offset, std::min(TS_PAYLOAD_SIZE, remaining.size() - offset)));
    }
  }

  // A PES is only handed over when the next one starts, so end each stream with a PES start
  std::vector<uint8_t> Finish()
  {
    for (const auto& stream : m_streams)
      WritePacket(stream.pid, true, PESHeader(stream.pid));
    return m_data;
  }

private:
  static std::vector<uint8_t> PESHeader(unsigned int pid)
  {
    const bool video{pid < AUDIO_PID};
    return {0x00, 0x00, 0x01, static_cast<uint8_t>(video ? 0xE0 : 0xBD), 0x00, 0x00,
            0x80, 0x00, 0x00};
  }

  void WritePacket(unsigned int pid, bool start, std::span<const uint8_t> payload)
  {
    const size_t stuffing{TS_PAYLOAD_SIZE - payload.size()};
    const uint8_t continuity{static_cast<uint8_t>(m_continuity[pid]++ & 0x0F)};

    m_data.insert(m_data.end(), 4, 0x00); // TP_extra_header
    m_data.push_back(0x47);
    m_data.push_back(static_cast<uint8_t>((start ? 0x40 : 0x00) | (pid >> 8)));
    m_data.push_back(static_cast<uint8_t>(pid & 0xFF));
    if (stuffing == 0)
      m_data.push_back(0x10 | continuity);
    else
    {
      m_data.push_back(0x30 | continuity);
      m_data.push_back(static_cast<uint8_t>(stuffing - 1)); // adaptation_field_length
      if (stuffing > 1)
      {
        m_data.push_back(0x00);
        m_data.insert(m_data.end(), stuffing - 2, 0xFF);
      }
    }
    m_data.insert(m_data.end(), payload.begin(), payload.end());
  }

  std::vector<ClipStream> m_streams;
  std::map<unsigned int, unsigned int> m_continuity;
  std::vector<uint8_t> m_data;
};

// A disc folder holding one clip, 00001.m2ts, removed again when the test ends
class CTempDisc
{
public:
  CTempDisc()
    : m_root(std::filesystem::temp_directory_path() /
             (std::string("kodi-test-m2ts-") +
              testing::UnitTest::GetInstance()->current_test_info()->name()))
  {
    std::filesystem::remove_all(m_root);
    std::filesystem::create_directories(m_root / "BDMV" / "STREAM");
  }

  ~CTempDisc()
  {
    std::error_code error;
    std::filesystem::remove_all(m_root, error);
  }

  bool GetStreams(const std::vector<uint8_t>& clip, StreamMap& streams) const
  {
    {
      std::ofstream file(m_root / "BDMV" / "STREAM" / "00001.m2ts", std::ios::binary);
      file.write(reinterpret_cast<const char*>(clip.data()),
                 static_cast<std::streamsize>(clip.size()));
    }
    const std::u8string root{m_root.u8string()};
    return CM2TSParser::GetStreamsFromFile(std::string(root.begin(), root.end()), 1, "M2TS",
                                           streams);
  }

private:
  std::filesystem::path m_root;
};

const TSAudioStreamInfo* GetAudio(const StreamMap& streams, unsigned int pid)
{
  const auto it{streams.find(pid)};
  return it == streams.end() ? nullptr : dynamic_cast<const TSAudioStreamInfo*>(it->second.get());
}

const TSVideoStreamInfo* GetVideo(const StreamMap& streams, unsigned int pid)
{
  const auto it{streams.find(pid)};
  return it == streams.end() ? nullptr : dynamic_cast<const TSVideoStreamInfo*>(it->second.get());
}

// E-AC-3 independent frame: 48 kHz, six blocks, acmod 7 with LFE (5.1), no metadata
const std::vector<uint8_t> EAC3_FRAME{0x0B, 0x77, 0x00, 0x00, 0x3F, 0x86, 0xC0, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

class CBitWriter
{
public:
  void Put(uint32_t value, unsigned int bits)
  {
    for (unsigned int i = bits; i > 0; --i)
    {
      if (m_bits % 8 == 0)
        m_data.push_back(0);
      m_data.back() |= static_cast<uint8_t>(((value >> (i - 1)) & 1) << (7 - m_bits % 8));
      ++m_bits;
    }
  }

  void PutUE(uint32_t value)
  {
    const unsigned int length{static_cast<unsigned int>(std::bit_width(value + 1ull))};
    Put(0, length - 1);
    Put(value + 1, length);
  }

  size_t Bits() const { return m_bits; }
  const std::vector<uint8_t>& Bytes() const { return m_data; }

private:
  std::vector<uint8_t> m_data;
  size_t m_bits{0};
};

// An Annex B NAL unit, with emulation prevention bytes inserted into its RBSP
std::vector<uint8_t> MakeNal(const std::vector<uint8_t>& header, const std::vector<uint8_t>& rbsp)
{
  std::vector<uint8_t> nal{0x00, 0x00, 0x00, 0x01};
  nal.insert(nal.end(), header.begin(), header.end());
  unsigned int zeros{0};
  for (const uint8_t byte : rbsp)
  {
    if (zeros >= 2 && byte <= 0x03)
    {
      nal.push_back(0x03);
      zeros = 0;
    }
    nal.push_back(byte);
    zeros = byte == 0x00 ? zeros + 1 : 0;
  }
  return nal;
}

// An HEVC SPS for 1920x1080 up to pic_height_in_luma_samples, which ends on a byte boundary
CBitWriter HevcSpsToHeight()
{
  CBitWriter sps;
  sps.Put(0, 4); // sps_video_parameter_set_id
  sps.Put(0, 3); // sps_max_sub_layers_minus1
  sps.Put(1, 1); // sps_temporal_id_nesting_flag
  sps.Put(0x02, 8); // general_profile_space, general_tier_flag, general_profile_idc (Main 10)
  sps.Put(0x20000000, 32); // general_profile_compatibility_flag
  sps.Put(0x9000, 16); // progressive, interlaced, ..., reserved
  sps.Put(0, 32);
  sps.Put(153, 8); // general_level_idc
  sps.PutUE(1); // sps_seq_parameter_set_id
  sps.PutUE(1); // chroma_format_idc
  sps.PutUE(1920); // pic_width_in_luma_samples
  sps.PutUE(1080); // pic_height_in_luma_samples
  return sps;
}

// An H.264 Main profile SPS for 1920x1080 up to pic_height_in_map_units_minus1, which ends on a
// byte boundary
CBitWriter H264SpsToHeight()
{
  CBitWriter sps;
  sps.Put(77, 8); // profile_idc
  sps.Put(0, 8); // constraint flags
  sps.Put(41, 8); // level_idc
  sps.PutUE(0); // seq_parameter_set_id
  sps.PutUE(0); // log2_max_frame_num_minus4
  sps.PutUE(0); // pic_order_cnt_type
  sps.PutUE(0); // log2_max_pic_order_cnt_lsb_minus4
  sps.PutUE(0); // max_num_ref_frames
  sps.Put(0, 1); // gaps_in_frame_num_value_allowed_flag
  sps.PutUE(119); // pic_width_in_mbs_minus1
  sps.PutUE(67); // pic_height_in_map_units_minus1
  return sps;
}

std::vector<uint8_t> H264Sps()
{
  CBitWriter sps{H264SpsToHeight()};
  sps.Put(1, 1); // frame_mbs_only_flag
  sps.Put(1, 1); // direct_8x8_inference_flag
  sps.Put(1, 1); // frame_cropping_flag
  sps.PutUE(0); // frame_crop_left_offset
  sps.PutUE(0); // frame_crop_right_offset
  sps.PutUE(0); // frame_crop_top_offset
  sps.PutUE(4); // frame_crop_bottom_offset
  sps.Put(0, 1); // vui_parameters_present_flag
  sps.Put(1, 1); // rbsp_stop_one_bit
  return sps.Bytes();
}

// A DTS extension substream header after its sync word, for one asset of 8 channels
std::vector<uint8_t> DTSExtensionHeader()
{
  CBitWriter header;
  header.Put(0, 8); // UserDefinedBits
  header.Put(0, 2); // nExtSSIndex
  header.Put(0, 1); // bHeaderSizeType
  header.Put(13, 8); // nuExtSSHeaderSize
  header.Put(100, 16); // nuExtSSFsize
  header.Put(1, 1); // bStaticFieldsPresent
  header.Put(0, 2); // nuRefClockCode
  header.Put(0, 3); // nuExSSFrameDurationCode
  header.Put(0, 1); // bTimeStampFlag
  header.Put(0, 3); // nuNumAudioPresnt
  header.Put(0, 3); // nuNumAssets
  header.Put(1, 1); // nuActiveExSSMask
  header.Put(1, 8); // nuActiveAssetMask
  header.Put(0, 1); // bMixMetadataEnbl
  header.Put(100, 16); // nuAssetFsize
  header.Put(20, 9); // nuAssetDescriptFsize
  header.Put(0, 3); // nuAssetIndex
  header.Put(0, 1); // bAssetTypeDescrPresent
  header.Put(0, 1); // bLanguageDescrPresent
  header.Put(0, 1); // bInfoTextPresent
  header.Put(23, 5); // nuBitResolution
  header.Put(5, 4); // nuMaxSampleRate
  header.Put(7, 8); // nuTotalNumChs
  return header.Bytes();
}

// A DTS frame with only an extension substream, its header cut to the given number of bytes
std::vector<uint8_t> DTSSubstreamFrame(size_t headerBytes)
{
  // Six leading bytes, as a frame shorter than the 14 byte core header is not examined
  std::vector<uint8_t> frame{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x58, 0x20, 0x25};
  const auto header{DTSExtensionHeader()};
  frame.insert(frame.end(), header.begin(), header.begin() + std::min(headerBytes, header.size()));
  return frame;
}
} // namespace

TEST(TestM2TSParser, EAC3AtSyncWord)
{
  CClipBuilder clip({{ENCODING_TYPE::AUDIO_AC3PLUS, AUDIO_PID}});
  clip.AddPES(AUDIO_PID, EAC3_FRAME);
  clip.AddPES(AUDIO_PID, EAC3_FRAME);

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* audio{GetAudio(streams, AUDIO_PID)};
  ASSERT_NE(audio, nullptr);
  EXPECT_EQ(audio->channels, 6u);
  EXPECT_EQ(audio->sampleRate, 48000u);
  EXPECT_TRUE(audio->completed);
}

TEST(TestM2TSParser, EAC3AfterLeadingBytes)
{
  std::vector<uint8_t> data{0x00, 0x00};
  data.insert(data.end(), EAC3_FRAME.begin(), EAC3_FRAME.end());

  CClipBuilder clip({{ENCODING_TYPE::AUDIO_AC3PLUS, AUDIO_PID}});
  clip.AddPES(AUDIO_PID, data);
  clip.AddPES(AUDIO_PID, data);

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* audio{GetAudio(streams, AUDIO_PID)};
  ASSERT_NE(audio, nullptr);
  EXPECT_EQ(audio->channels, 6u);
  EXPECT_EQ(audio->sampleRate, 48000u);
  EXPECT_TRUE(audio->completed);
}

TEST(TestM2TSParser, OversizedShortTermRpsCount)
{
  CBitWriter sps{HevcSpsToHeight()};
  sps.Put(0, 1); // conformance_window_flag
  sps.PutUE(2); // bit_depth_luma_minus8
  sps.PutUE(2); // bit_depth_chroma_minus8
  sps.PutUE(4); // log2_max_pic_order_cnt_lsb_minus4
  sps.Put(1, 1); // sps_sub_layer_ordering_info_present_flag
  sps.PutUE(4); // sps_max_dec_pic_buffering_minus1
  sps.PutUE(2); // sps_max_num_reorder_pics
  sps.PutUE(0); // sps_max_latency_increase_plus1
  sps.PutUE(0); // log2_min_luma_coding_block_size_minus3
  sps.PutUE(2); // log2_diff_max_min_luma_coding_block_size
  sps.PutUE(0); // log2_min_luma_transform_block_size_minus2
  sps.PutUE(3); // log2_diff_max_min_luma_transform_block_size
  sps.PutUE(0); // max_transform_hierarchy_depth_inter
  sps.PutUE(0); // max_transform_hierarchy_depth_intra
  sps.Put(0, 1); // scaling_list_enabled_flag
  sps.Put(1, 1); // amp_enabled_flag
  sps.Put(1, 1); // sample_adaptive_offset_enabled_flag
  sps.Put(0, 1); // pcm_enabled_flag
  sps.PutUE(0x80000000); // num_short_term_ref_pic_sets, at most 64
  sps.Put(1, 1); // rbsp_stop_one_bit

  CClipBuilder clip({{ENCODING_TYPE::VIDEO_HEVC, VIDEO_PID}});
  clip.AddPES(VIDEO_PID, MakeNal({0x42, 0x01}, sps.Bytes()));

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* video{GetVideo(streams, VIDEO_PID)};
  ASSERT_NE(video, nullptr);
  EXPECT_EQ(video->width, 0u);
  EXPECT_FALSE(video->completed);
}

TEST(TestM2TSParser, TruncatedSpsWritesNothing)
{
  // Cut before conformance_window_flag, after the width and height have been read
  const CBitWriter sps{HevcSpsToHeight()};
  ASSERT_EQ(sps.Bits() % 8, 0u);

  CClipBuilder clip({{ENCODING_TYPE::VIDEO_HEVC, VIDEO_PID}});
  clip.AddPES(VIDEO_PID, MakeNal({0x42, 0x01}, sps.Bytes()));

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* video{GetVideo(streams, VIDEO_PID)};
  ASSERT_NE(video, nullptr);
  EXPECT_EQ(video->width, 0u);
  EXPECT_EQ(video->height, 0u);
  EXPECT_EQ(video->bitDepth, 0u);
  EXPECT_FALSE(video->completed);
}

TEST(TestM2TSParser, TruncatedSpsThenValidSps)
{
  // Cut before frame_mbs_only_flag
  const CBitWriter truncated{H264SpsToHeight()};
  ASSERT_EQ(truncated.Bits() % 8, 0u);

  CClipBuilder clip({{ENCODING_TYPE::VIDEO_H264, VIDEO_PID}});
  clip.AddPES(VIDEO_PID, MakeNal({0x67}, truncated.Bytes()));
  clip.AddPES(VIDEO_PID, MakeNal({0x67}, H264Sps()));

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* video{GetVideo(streams, VIDEO_PID)};
  ASSERT_NE(video, nullptr);
  EXPECT_EQ(video->width, 1920u);
  EXPECT_EQ(video->height, 1080u);
  EXPECT_EQ(video->bitDepth, 8u);
  EXPECT_TRUE(video->completed);
}

TEST(TestM2TSParser, FailedEAC3HeaderContributesNothing)
{
  // A dependent frame with a channel map, cut inside its mixing metadata
  const std::vector<uint8_t> dependent{0x0B, 0x77, 0x40, 0x00, 0x3F, 0x86, 0xDF, 0xFF, 0xF8, 0x00};

  CClipBuilder clip({{ENCODING_TYPE::AUDIO_AC3PLUS, AUDIO_PID}});
  clip.AddPES(AUDIO_PID, dependent);
  clip.AddPES(AUDIO_PID, EAC3_FRAME);
  clip.AddPES(AUDIO_PID, EAC3_FRAME);

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* audio{GetAudio(streams, AUDIO_PID)};
  ASSERT_NE(audio, nullptr);
  EXPECT_FALSE(audio->hasDependantStream);
  EXPECT_EQ(audio->channels, 6u);
  EXPECT_EQ(audio->seen, 2u);
}

TEST(TestM2TSParser, TruncatedDTSSubstreamThenValid)
{
  // Cut before bStaticFieldsPresent
  const auto truncated{DTSSubstreamFrame(4)};
  const auto valid{DTSSubstreamFrame(DTSExtensionHeader().size())};

  CClipBuilder clip({{ENCODING_TYPE::AUDIO_DTSHD, AUDIO_PID}});
  clip.AddPES(AUDIO_PID, truncated);
  clip.AddPES(AUDIO_PID, truncated);
  clip.AddPES(AUDIO_PID, valid);
  clip.AddPES(AUDIO_PID, valid);

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* audio{GetAudio(streams, AUDIO_PID)};
  ASSERT_NE(audio, nullptr);
  EXPECT_EQ(audio->channels, 8u);
  EXPECT_EQ(audio->seen, 2u);
  EXPECT_TRUE(audio->completed);
}

TEST(TestM2TSParser, EmptyDTSSubstreamThenValid)
{
  // The sync word ends the frame, with no header after it. Four more leading bytes make the frame
  // as long as a core header, so that it is examined at all.
  auto empty{DTSSubstreamFrame(0)};
  empty.insert(empty.begin(), 4, 0x00);
  const auto valid{DTSSubstreamFrame(DTSExtensionHeader().size())};

  CClipBuilder clip({{ENCODING_TYPE::AUDIO_DTSHD, AUDIO_PID}});
  clip.AddPES(AUDIO_PID, empty);
  clip.AddPES(AUDIO_PID, empty);
  clip.AddPES(AUDIO_PID, valid);
  clip.AddPES(AUDIO_PID, valid);

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* audio{GetAudio(streams, AUDIO_PID)};
  ASSERT_NE(audio, nullptr);
  EXPECT_EQ(audio->channels, 8u);
  EXPECT_EQ(audio->seen, 2u);
  EXPECT_TRUE(audio->completed);
}

TEST(TestM2TSParser, DTSCoreWithTruncatedExtension)
{
  // A core header for 5 channels (amode 9) at 48 kHz (sfreq 13) with LFE (lff 1)
  const uint64_t header{(9ull << 30) | (13ull << 26) | (1ull << 9)};
  std::vector<uint8_t> frame{0x7F, 0xFE, 0x80, 0x01};
  for (int shift = 56; shift >= 0; shift -= 8)
    frame.push_back(static_cast<uint8_t>(header >> shift));
  frame.insert(frame.end(), {0x00, 0x00});

  // The extension substream after the core, cut before bStaticFieldsPresent
  const auto extension{DTSSubstreamFrame(4)};
  frame.insert(frame.end(), extension.begin() + 6, extension.end());

  CClipBuilder clip({{ENCODING_TYPE::AUDIO_DTSHD, AUDIO_PID}});
  clip.AddPES(AUDIO_PID, frame);
  clip.AddPES(AUDIO_PID, frame);

  StreamMap streams;
  ASSERT_TRUE(CTempDisc().GetStreams(clip.Finish(), streams));

  const auto* audio{GetAudio(streams, AUDIO_PID)};
  ASSERT_NE(audio, nullptr);
  EXPECT_EQ(audio->channels, 6u);
  EXPECT_EQ(audio->sampleRate, 48000u);
  EXPECT_EQ(audio->seen, 2u);
  EXPECT_TRUE(audio->completed);
}
