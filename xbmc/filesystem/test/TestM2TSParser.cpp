/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/bluray/M2TSParser.h"

#include <algorithm>
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

// E-AC-3 independent frame: 48 kHz, six blocks, acmod 7 with LFE (5.1), no metadata
const std::vector<uint8_t> EAC3_FRAME{0x0B, 0x77, 0x00, 0x00, 0x3F, 0x86, 0xC0, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
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
