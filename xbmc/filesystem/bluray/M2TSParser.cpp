/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "M2TSParser.h"

#include "BitReader.h"
#include "PlaylistStructure.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

namespace XFILE
{
namespace
{
// Packet parsing constants
constexpr int BDAV_PACKET_SIZE = 192;
constexpr int TS_PACKET_SIZE = 188;
constexpr unsigned int SYNC_BYTE = 0x47;
constexpr unsigned int TS_HEADER_SIZE = 4;
constexpr unsigned int ADAPTATION_FIELD_MASK = 0x02;
constexpr unsigned int PACKETS_TO_PARSE =
    2000; // Based on testing this is enough to analyse all streams
constexpr unsigned int BUFFER_SIZE{BDAV_PACKET_SIZE * PACKETS_TO_PARSE};
constexpr int TIMESTAMP_SIZE = 4;

// PSI parsing constants
constexpr int PSI_HEADER_SIZE = 3;
constexpr int LONG_HEADER_SIZE = 5;
constexpr int CRC_SIZE = 4;
constexpr unsigned int SECTION_SYNTAX_INDICATOR_MASK = 0x8000;
constexpr unsigned int LENGTH_MASK = 0x03FF;
constexpr unsigned int PID_MASK = 0x1FFF;

enum class TABLE_IDS : uint8_t
{
  PAT_TABLE_ID = 0x00,
  PMT_TABLE_ID = 0x02
};

struct TableInformation
{
  int offset{-1};
  int sectionLength{-1}; // includes long header, data, and CRC value
  int dataLength{-1};
  int size{-1};
};

// PMT parsing constants
constexpr int PMT_HEADER_SIZE = 4;

// PES parsing constants
inline constexpr std::array SYSTEM_STREAM_IDS{0xBCu, 0xBEu, 0xBFu, 0xF0u,
                                              0xF1u, 0xF2u, 0xF8u, 0xFFu};
constexpr unsigned int PES_HEADER_SIZE = 9;
constexpr unsigned int PES_HEADER_OFFSET = 8;

// Elementary stream parsing constants

// (E)AC3
constexpr int ELEMENTARY_STREAM_HEADER_SIZE = 5;
constexpr unsigned int AC3_SYNCWORD = 0x0B77;
inline constexpr std::array AC3_SAMPLE_RATES{48000u, 44100u, 32000u, 0u};
inline constexpr std::array AC3_CHANNEL_COUNTS{2u, 1u, 2u, 3u, 3u, 4u, 4u, 5u};
inline constexpr std::array NUM_AUDIO_BLOCK_PER_SYNCFRAME{1u, 2u, 3u, 6u};
constexpr unsigned int EAC3_SINGLE_CHANNEL_MASK = 0b0000000110001010;
constexpr unsigned int EAC3_DUAL_CHANNEL_MASK = 0b0000011001110100;

// DTS
enum class DTSSyncWords : uint32_t
{
  CORE_16BIT_BE = 0x7FFE8001, // DTS Core (only 16 bit big-endian supported on blurays)
  SUBSTREAM = 0x64582025, // DTS-HD and DTS-HD HRA
};
inline constexpr std::array DTS_SYNCWORD_SUBSTREAM{std::byte{0x64}, std::byte{0x58},
                                                   std::byte{0x20}, std::byte{0x25}};
inline constexpr std::array DTS_SYNCWORD_XLL{std::byte{0x41}, std::byte{0xA2}, std::byte{0x95},
                                             std::byte{0x47}}; // DTS-HD Master Audio
inline constexpr std::array DTS_SYNCWORD_XLL_X{std::byte{0x02}, std::byte{0x00}, std::byte{0x08},
                                               std::byte{0x50}}; // DTS:X
inline constexpr std::array DTS_SYNCWORD_XLL_X_IMAX{
    std::byte{0xF1}, std::byte{0x40}, std::byte{0x00}, std::byte{0xD0}}; // DTS-HD MA + IMAX

inline constexpr std::array DTS_SAMPLE_RATES{8000u,  16000u, 32000u,  64000u,  128000u, 22050u,
                                             44100u, 88200u, 176400u, 352800u, 12000u,  24000u,
                                             48000u, 96000u, 192000u, 384000u};
inline constexpr std::array DTS_CHANNEL_COUNTS{1u, 2u, 2u, 2u, 2u, 3u, 3u, 4u,
                                               4u, 5u, 6u, 6u, 6u, 7u, 8u, 8u};

constexpr unsigned int DTS_HEADER_SIZE = 14;
constexpr unsigned int HEADERS_PARSED_FOR_COMPLETE = 2;

// TrueHD
inline constexpr std::array TRUEHD_COMMON_SYNC = {std::byte{0xF8}, std::byte{0x72},
                                                  std::byte{0x6F}};
inline constexpr std::array TRUEHD_SAMPLE_RATES{48000u, 96000u, 192000u, 0u, 0u, 0u, 0u, 0u,
                                                44100u, 88200u, 176400u, 0u, 0u, 0u, 0u, 0u};

constexpr unsigned int TRUEHD_MINIMUM_HEADER_SIZE = 26;
constexpr unsigned int TRUEHD_HEADER_SIGNATURE = 0xB752;
constexpr unsigned int DOLBY_FLAG = 0xBA;
constexpr unsigned int CH8_SINGLE_CHANNEL_MASK = 0b1100110000110;
constexpr unsigned int CH8_DUAL_CHANNEL_MASK = 0b0011001111001;
constexpr unsigned int CH8_16_SINGLE_CHANNEL_ALTERNATE_MASK = 0b00110;
constexpr unsigned int CH8_16_DUAL_CHANNEL_ALTERNATE_MASK = 0b11001;

// LPCM
inline const std::map<unsigned int, unsigned int> LPCM_SAMPLE_RATES = {
    {1u, 48000u}, {4u, 96000u}, {5u, 192000u}};
inline constexpr std::array LPCM_CHANNEL_COUNTS{0u, 1u, 0u, 2u, 3u, 3u, 4u, 4u,
                                                5u, 6u, 7u, 8u, 0u, 0u, 0u, 0u};

// Video

// H264/H265
enum class VideoCodec : uint8_t
{
  H264,
  H265
};

constexpr unsigned int H264_PROFILE_HIGH = 100;
constexpr unsigned int H264_PROFILE_MAIN = 77;

inline constexpr std::array NAL_START_CODE_3 = {std::byte{0}, std::byte{0}, std::byte{1}};

constexpr unsigned int H264_NAL_SEI = 6;
constexpr unsigned int H264_NAL_SPS = 7;
constexpr unsigned int H264_PREFIX_NAL_UNIT = 14;
constexpr unsigned int H264_CODED_SLICE_EXTENSION = 20;
constexpr unsigned int H264_CODED_SLICE_EXTENSION_FOR_DEPTH_VIEW = 21;
constexpr unsigned int H265_NAL_SPS = 33;
constexpr unsigned int H265_NAL_SEI_PREFIX = 39;
constexpr unsigned int H265_NAL_SEI_SUFFIX = 40;
constexpr unsigned int DOLBY_VISION_RPU = 62;
constexpr unsigned int DOLBY_VISION_EL = 63;
constexpr unsigned int DOLBY_VISION_RPU_HEADER = 0x7C01;
constexpr unsigned int DOLBY_VISION_EL_HEADER = 0x7E01;

constexpr unsigned int H265_EXTENDED_SAR = 255;
inline constexpr std::array ASPECT_RATIOS{
    0.0f,          1.0f,           12.0f / 11.0f, 10.0f / 11.0f, 16.0f / 11.0f, 40.0f / 33.0f,
    24.0f / 11.0f, 20.0f / 11.0f,  32.0f / 11.0f, 80.0f / 33.0f, 18.0f / 11.0f, 15.0f / 11.0f,
    64.0f / 33.0f, 160.0f / 99.0f, 4.0f / 3.0f,   3.0f / 2.0f,   2.0f / 1.0f};

constexpr unsigned int SEI_PAYLOAD_REGISTERED_ITU_T_T35 = 4;
constexpr unsigned int SEI_PAYLOAD_UNREGISTERED = 5;
constexpr unsigned int SEI_PAYLOAD_MASTERING_DISPLAY_COLOUR_VOLUME = 137;
constexpr unsigned int SEI_PAYLOAD_CONTENT_LIGHT_LEVEL_INFO = 144;

inline constexpr std::array DOLBY_VISION_PROFILE_7_UUID = {
    std::byte{0x17}, std::byte{0xFC}, std::byte{0x11}, std::byte{0xB4},
    std::byte{0x2D}, std::byte{0xE2}, std::byte{0x4E}, std::byte{0x96},
    std::byte{0xA9}, std::byte{0xA4}, std::byte{0x23}, std::byte{0xE0},
    std::byte{0xD9}, std::byte{0x01}, std::byte{0x68}, std::byte{0xE9}};

// VC1
inline constexpr std::array VC1_SEQUENCE_HEADER_START_CODE = {std::byte{0x00}, std::byte{0x00},
                                                              std::byte{0x01}, std::byte{0x0F}};

constexpr unsigned int VC1_EXTENDED_SAR = 15;
inline constexpr std::array VC1_ASPECT_RATIOS{
    0.0f,          1.0f,           12.0f / 11.0f, 10.0f / 11.0f, 16.0f / 11.0f, 40.0f / 33.0f,
    24.0f / 11.0f, 20.0f / 11.0f,  32.0f / 11.0f, 80.0f / 33.0f, 18.0f / 11.0f, 15.0f / 11.0f,
    64.0f / 33.0f, 160.0f / 99.0f, 0.0f,          0.0f};

// MPEG-2
inline constexpr std::array MPEG2_SEQUENCE_HEADER_START_CODE = {std::byte{0x00}, std::byte{0x00},
                                                                std::byte{0x01}, std::byte{0xB3}};

inline constexpr std::array MPEG2_DISPLAY_ASPECT_RATIOS{0.0f, 1.0f, 3.0f / 4.0f, 9.0f / 16.0f,
                                                        1.0f / 2.21f};

// Structures for parsers

struct TSPacket
{
  unsigned int pid{};
  bool payloadUnitStartIndicator{false};
  unsigned int continuityCounter{};
  std::span<std::byte> payload;
};

struct PESPacket
{
  unsigned int streamId{};
  std::span<std::byte> data;
};

struct DTSFrame
{
  unsigned int syncPos{};
  DTSSyncWords syncWord{};
  std::span<std::byte> data;
};

// Parsers

std::optional<TSPacket> ParseTSPacket(const std::span<std::byte>& packet)
{
  if (GetByte(packet, 0) != SYNC_BYTE)
    return std::nullopt;

  unsigned int header{GetWord(packet, 1)};
  const bool payloadUnitStartIndicator{GetBits(header, 15, 1) == 1};
  const unsigned int pid{GetBits(header, 13, 13)};
  header = GetByte(packet, 3);
  const unsigned int adaptationFieldControl{GetBits(header, 6, 2)};
  const unsigned int continuityCounter{GetBits(header, 4, 4)};

  // Calculate payload start position
  const unsigned int start{TS_HEADER_SIZE + (adaptationFieldControl & ADAPTATION_FIELD_MASK
                                                 ? GetByte(packet, 4) + 1
                                                 : 0)};
  if (start > packet.size())
    return std::nullopt;

  std::span<std::byte> payload;
  if ((adaptationFieldControl & 1) == 1)
    payload = std::span(packet.begin() + start, packet.end());

  return TSPacket{.pid = pid,
                  .payloadUnitStartIndicator = payloadUnitStartIndicator,
                  .continuityCounter = continuityCounter,
                  .payload = payload};
}

std::optional<PESPacket> ParsePESPacket(const std::span<std::byte>& packet)
{
  if (GetByte(packet, 0) != 0x00 || GetByte(packet, 1) != 0x00 || GetByte(packet, 2) != 0x01)
    return std::nullopt;

  // Skip system streams
  const unsigned int streamId{GetByte(packet, 3)};
  if (std::ranges::find(SYSTEM_STREAM_IDS, streamId) != SYSTEM_STREAM_IDS.end())
    return std::nullopt;

  unsigned int pesAdditionalHeaderLength{GetByte(packet, PES_HEADER_OFFSET)};
  unsigned int offset{PES_HEADER_SIZE + pesAdditionalHeaderLength};
  if (offset > packet.size())
    return std::nullopt;

  return PESPacket{.streamId = streamId, .data = packet.subspan(offset)};
}

// Assembler Structs

struct SectionAssembler
{
private:
  // Reassembles PSI sections split across TS packets for a given PID.
  std::vector<std::byte> buffer;
  unsigned int needed{0};

public:
  // Feed TS payload; return complete sections as vector (can be multiple if PUSI)
  std::vector<std::vector<std::byte>> push(bool payloadUnitStartIndicator,
                                           const std::span<std::byte>& payload)
  {
    unsigned int pointerSize;
    unsigned int start;
    if (payloadUnitStartIndicator)
    {
      // Flush any incomplete sections
      buffer.clear();
      needed = 0;
      pointerSize = GetByte(payload, 0);
      start = 1; // Adjust for pointer
    }
    else
      start = pointerSize = 0;

    std::vector<std::vector<std::byte>> sections; // Complete sections to return

    // There may be multiple sections after the pointer
    while (start + pointerSize < payload.size())
    {
      unsigned int offset{start + pointerSize};

      if (needed == 0)
      {
        if (offset + PSI_HEADER_SIZE > payload.size())
          return sections; // Not enough for header

        const unsigned int sectionLength{GetWord(payload, offset + 1) & LENGTH_MASK};
        needed = sectionLength + PSI_HEADER_SIZE;
      }

      // Add section part
      if (const unsigned int availableLength{static_cast<unsigned int>(payload.size()) - offset};
          availableLength >= needed)
      {
        // Complete section available
        buffer.insert(buffer.end(), payload.begin() + offset, payload.begin() + offset + needed);
        sections.emplace_back(std::move(buffer));
        buffer.clear();
        start = offset + needed;
        needed = 0;
      }
      else
      {
        // Incomplete section
        buffer.insert(buffer.end(), payload.begin() + offset, payload.end());
        needed -= availableLength; // Need more data
        return sections; // Return any completed sections
      }

      if (start >= payload.size())
        return sections; // Finished payload

      pointerSize = 0; // Subsequent sections are contiguous
    }
    return sections;
  }
};

struct PESAssembler
{
private:
  std::vector<std::byte> buffer;
  bool started{false};
  unsigned int lastContinuityCounter{0xFF};

public:
  // returns complete PES payloads (PES packet including header)
  std::vector<std::vector<std::byte>> push(const TSPacket& tsPacket)
  {
    std::vector<std::vector<std::byte>> section;

    // Continuity counter loss detection
    if (lastContinuityCounter != 0xFF &&
        tsPacket.continuityCounter != ((lastContinuityCounter + 1) & 0x0F))
    {
      // Discontinuity -> reset
      buffer.clear();
      started = false;
    }
    lastContinuityCounter = tsPacket.continuityCounter;

    if (tsPacket.payloadUnitStartIndicator)
    {
      if (!buffer.empty())
        section.emplace_back(std::move(buffer)); // Completed section
      buffer.clear();
      started = true;
    }

    if (started)
      buffer.insert(buffer.end(), tsPacket.payload.begin(), tsPacket.payload.end());

    return section;
  }
};

// Helpers

bool IsVideoStream(ENCODING_TYPE streamType)
{
  using enum ENCODING_TYPE;
  return streamType == VIDEO_HEVC || streamType == VIDEO_H264 || streamType == VIDEO_H264_MVC ||
         streamType == VIDEO_MPEG2 || streamType == VIDEO_VC1;
}

bool IsAudioStream(ENCODING_TYPE streamType)
{
  using enum ENCODING_TYPE;
  return streamType == AUDIO_LPCM || streamType == AUDIO_AC3 || streamType == AUDIO_DTS ||
         streamType == AUDIO_TRUHD || streamType == AUDIO_AC3PLUS || streamType == AUDIO_DTSHD ||
         streamType == AUDIO_DTSHD_MASTER || streamType == AUDIO_AC3PLUS_SECONDARY ||
         streamType == AUDIO_DTSHD_SECONDARY;
}

bool IsSubtitleStream(ENCODING_TYPE streamType)
{
  using enum ENCODING_TYPE;
  return streamType == SUB_PG;
}

std::string GetStreamTypeName(ENCODING_TYPE streamType)
{
  switch (streamType)
  {
    using enum ENCODING_TYPE;
    case VIDEO_MPEG2:
      return "MPEG-2 Video";
    case VIDEO_H264:
      return "H.264/AVC Video";
    case VIDEO_H264_MVC:
      return "H.264/MVC Video";
    case VIDEO_VC1:
      return "VC-1 Video";
    case VIDEO_HEVC:
      return "H.265/HEVC Video";
    case AUDIO_LPCM:
      return "HDMV LPCM Audio";
    case AUDIO_AC3:
      return "AC-3 Audio";
    case AUDIO_DTS:
      return "DTS Audio";
    case AUDIO_TRUHD:
      return "TrueHD Audio";
    case AUDIO_AC3PLUS:
      return "EAC-3 Audio";
    case AUDIO_DTSHD:
      return "DTS-HD Audio";
    case AUDIO_DTSHD_MASTER:
      return "DTS-HD Master Audio";
    case AUDIO_AC3PLUS_SECONDARY:
      return "EAC-3 Audio (Secondary)";
    case AUDIO_DTSHD_SECONDARY:
      return "DTS-HD Audio (Secondary)";
    case SUB_PG:
      return "HDMV PGS Subtitles";
    case SUB_IG:
      return "HDMV Interactive Graphics";
    case SUB_TEXT:
      return "Text Subtitles";
    default:
    {
      const std::string unknown{
          fmt::format("Unknown (0x{:02x})", static_cast<unsigned int>(streamType))};
      return unknown;
    }
  }
}

// Main functions

bool ProcessCommonHeader(const std::span<std::byte>& packet,
                         TABLE_IDS wantedTableId,
                         bool payloadUnitStartIndicator,
                         TableInformation& tableInformation)
{
  const int size{static_cast<int>(packet.size())};

  // Pointer field to identify skipped bytes
  int offset{payloadUnitStartIndicator ? GetByte(packet, 0) + 1 : 0};

  if (const auto tableId{static_cast<TABLE_IDS>(GetByte(packet, offset))}; tableId != wantedTableId)
    return false;

  const unsigned int header{GetWord(packet, offset + 1)};
  bool sectionSyntaxIndicator{(header & SECTION_SYNTAX_INDICATOR_MASK) !=
                              0}; // Long section header syntax is used
  const int sectionLength{static_cast<int>(header & LENGTH_MASK)};
  offset += PSI_HEADER_SIZE + (sectionSyntaxIndicator ? LONG_HEADER_SIZE : 0);
  if (sectionLength + PSI_HEADER_SIZE > size)
    return false;

  tableInformation.offset = offset;
  tableInformation.sectionLength = sectionLength;
  tableInformation.dataLength =
      sectionLength - (sectionSyntaxIndicator ? LONG_HEADER_SIZE : 0) - CRC_SIZE;
  tableInformation.size = size;

  return true;
}

bool ParsePAT(const std::span<std::byte>& PAT,
              bool payloadUnitStartIndicator,
              std::set<unsigned int>& pmtPIDs,
              std::unordered_map<unsigned int, SectionAssembler>& pidSection)
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing PAT");

  // Process common header
  TableInformation tableInformation;
  if (!ProcessCommonHeader(PAT, TABLE_IDS::PAT_TABLE_ID, payloadUnitStartIndicator,
                           tableInformation))
    return false;

  // Find program map PIDs
  const int offset{tableInformation.offset};
  for (int i = 0; i + 4 <= tableInformation.dataLength; i += 4)
  {
    unsigned int programNumber{GetWord(PAT, offset + i)};
    if (programNumber != 0)
    {
      unsigned int pmPID{GetWord(PAT, offset + i + 2) & PID_MASK};
      pmtPIDs.insert(pmPID);
      pidSection.try_emplace(pmPID); // Prepare assembler
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Found pmPID {} for program {}",
                  fmt::format("{:04x}", pmPID), programNumber);
    }
  }

  return true;
}

void ProcessPMTEntry(std::vector<std::byte>& section,
                     const int offset,
                     int& indexOffset,
                     std::unordered_map<unsigned int, PESAssembler>& pesSection,
                     StreamMap& streams)
{
  const auto streamType{static_cast<ENCODING_TYPE>(GetByte(section, offset + indexOffset))};
  const unsigned int elementaryPID{GetWord(section, offset + indexOffset + 1) & PID_MASK};
  const int esInfoLength{
      static_cast<int>(GetWord(section, offset + indexOffset + 3) & LENGTH_MASK)};
  const std::string streamTypeName{GetStreamTypeName(streamType)};

  if (!streams.contains(elementaryPID))
  {
    // Parse descriptors
    int descriptorOffset{offset + indexOffset + 5};
    int end{descriptorOffset + esInfoLength};
    std::vector<Descriptor> descriptors;
    std::string language{"und"};
    while (descriptorOffset + 2 <= end && descriptorOffset < static_cast<int>(section.size()))
    {
      const unsigned int desc_tag{GetByte(section, descriptorOffset)};
      const int desc_length{GetByte(section, descriptorOffset + 1)};
      const std::string desc_data{GetString(section, descriptorOffset + 2, desc_length)};

      if (descriptorOffset + 2 + desc_length > static_cast<int>(section.size()))
        break;

      descriptors.emplace_back(
          Descriptor{.tag = desc_tag,
                     .length = desc_length,
                     .data = std::vector(section.begin() + descriptorOffset + 2,
                                         section.begin() + descriptorOffset + 2 + desc_length)});

      // ISO 639 language descriptor
      if (desc_tag == 0x0A && desc_length >= 4)
        language = GetString(section, descriptorOffset + 2, 3);

      descriptorOffset += 2 + desc_length;
    }

    if (IsVideoStream(streamType))
      streams[elementaryPID] = std::make_shared<TSVideoStreamInfo>();
    else if (IsAudioStream(streamType))
      streams[elementaryPID] = std::make_shared<TSAudioStreamInfo>();
    else
    {
      streams[elementaryPID] = std::make_shared<TSStreamInfo>();
      streams[elementaryPID]->completed = true; // No further parsing
    }

    streams[elementaryPID]->pid = elementaryPID;
    streams[elementaryPID]->streamType = streamType;
    streams[elementaryPID]->descriptors = std::move(descriptors);

    pesSection.try_emplace(elementaryPID); // Prepare assembler

    CLog::LogFC(LOGDEBUG, LOGBLURAY,
                "Found stream at offset 0x{} - type: {} (0x{}), pid: 0x{}, lang {}",
                fmt::format("{:06x}", offset + indexOffset), streamTypeName,
                fmt::format("{:02x}", static_cast<int>(streamType)),
                fmt::format("{:04x}", elementaryPID), language);
  }

  indexOffset += ELEMENTARY_STREAM_HEADER_SIZE + esInfoLength;
}

bool ParsePMT(unsigned int pid,
              const std::span<std::byte>& PMT,
              bool payloadUnitStartIndicator,
              std::unordered_map<unsigned int, SectionAssembler>& pidSection,
              std::unordered_map<unsigned int, PESAssembler>& pesSection,
              StreamMap& streams)
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing PMT - PID 0x{}", fmt::format("{:04x}", pid));

  bool parsed{false};
  auto& sectionAssembler{pidSection[pid]};
  for (auto& section : sectionAssembler.push(payloadUnitStartIndicator, PMT))
  {
    // Process common header
    TableInformation tableInformation;
    if (!ProcessCommonHeader(section, TABLE_IDS::PMT_TABLE_ID, false, tableInformation))
      return false;

    int offset{tableInformation.offset};
    const int programInfoLength{static_cast<int>(GetWord(section, offset + 2) & LENGTH_MASK)};
    offset += PMT_HEADER_SIZE;

    for (int i = programInfoLength; i < tableInformation.dataLength - PMT_HEADER_SIZE;)
      ProcessPMTEntry(section, offset, i, pesSection, streams);
    parsed = true;
  }
  return parsed;
}

// Audio related

bool ParseAC3Bitstream(const std::span<std::byte>& buffer,
                       int offset,
                       TSAudioStreamInfo* streamInfo)
{
  // Parse syncinfo
  unsigned int header{GetByte(buffer, offset + 4)};
  const unsigned int fscod{GetBits(header, 8, 2)};

  // Parse bsi
  header = GetWord(buffer, offset + 6);
  const unsigned int acmod{GetBits(header, 16, 3)};

  // Set sample rate
  if (const auto& sampleRates{AC3_SAMPLE_RATES}; fscod < sampleRates.size())
    streamInfo->sampleRate = sampleRates[fscod];

  // Set channel count
  if (const auto& channelCounts{AC3_CHANNEL_COUNTS}; acmod < channelCounts.size())
    streamInfo->channels = channelCounts[acmod];

  // Check for LFE
  // First determine offset of lfeon bit
  int bits{0};
  if ((acmod & 0x01) && acmod != 0x01)
    bits += 2; // 3 front channels - skip cmixlev
  if (acmod & 0x04)
    bits += 2; // 1 center channel - skip surmixlev
  if (acmod & 0x02)
    bits += 2; // 2/0 mode - skip dsurmod

  if (const bool lfeon{GetBits(header, 13 - bits, 1) == 1}; lfeon)
    streamInfo->channels++;

  streamInfo->completed = (++streamInfo->seen) >= HEADERS_PARSED_FOR_COMPLETE;

  return true;
}

struct EAC3BitStreamInfo
{
  unsigned int strmtyp;
  unsigned int fscod;
  unsigned int fscod2_numblkscod;
  unsigned int acmod;
  bool lfeon;
};

bool GetEAC3ChannelCountAndSampleRate(BitReader& br,
                                      EAC3BitStreamInfo& bsi,
                                      TSAudioStreamInfo* streamInfo)
{
  // Parse bsi
  bsi.strmtyp = br.ReadBits(2);
  br.SkipBits(14); // substreamid, frmsiz
  bsi.fscod = br.ReadBits(2);
  bsi.fscod2_numblkscod = br.ReadBits(2);
  bsi.acmod = br.ReadBits(3);
  bsi.lfeon = br.ReadBits(1) == 1;

  if (unsigned int bsid{br.ReadBits(5)}; bsid <= 10)
    return false; // Not EAC-3 stream

  br.SkipBits(5); // dialnorm

  // Find potential position of channel map
  if (br.ReadBits(1) == 1) //  compre
    br.SkipBits(8); // compr
  if (bsi.acmod == 0)
  {
    br.SkipBits(5);
    if (br.ReadBits(1) == 1) //  compr2e
      br.SkipBits(8); // compr2
  }

  // See if channel map present for dependant streams
  bool chanmape{false};
  uint32_t chanmap{0};
  if (bsi.strmtyp == 1)
  {
    chanmape = br.ReadBits(1) == 1;
    if (chanmape)
      chanmap = br.ReadBits(16);
  }

  // Set sample rate
  constexpr auto sampleRates = AC3_SAMPLE_RATES;
  if (bsi.fscod < 3)
    streamInfo->sampleRate = sampleRates[bsi.fscod];
  else if (bsi.fscod == 3 && bsi.fscod2_numblkscod < 3)
    streamInfo->sampleRate = sampleRates[bsi.fscod2_numblkscod] / 2;

  // Set channel count
  unsigned int channelCount{0};
  if (!chanmape)
  {
    // If no channel map, then use the AC3 way
    if (const auto& channelCounts{AC3_CHANNEL_COUNTS}; bsi.acmod < channelCounts.size())
      channelCount = channelCounts[bsi.acmod] + (bsi.lfeon ? 1 : 0);
  }
  else
  {
    // Derive additional channels from channel map
    // Mask bits that represent 2 channels (L/R pair)
    const uint32_t chanmap2{chanmap & EAC3_DUAL_CHANNEL_MASK};
    channelCount = std::popcount(chanmap2) * 2;

    // Then mask bits that represent 1 channel
    const uint32_t chanmap1{chanmap & EAC3_SINGLE_CHANNEL_MASK};
    channelCount += std::popcount(chanmap1);
  }

  if (bsi.strmtyp == 0)
    streamInfo->channels = channelCount;
  else if (bsi.strmtyp == 1)
  {
    streamInfo->channels += channelCount;
    streamInfo->hasDependantStream = true;
  }

  return true;
}

void SkipEAC3MixingMetadata(BitReader& br, const EAC3BitStreamInfo& bsi)
{
  if (bsi.acmod > 2)
    br.SkipBits(2); // dmixmod
  if ((bsi.acmod & 1) && (bsi.acmod > 2))
    br.SkipBits(6); // ltrctmixlev, lorocmixlev
  if (bsi.acmod & 4)
    br.SkipBits(6); // ltrtsurmixlev, lortsurmixlev
  if (bsi.lfeon && br.ReadBits(1) == 1) // lfemixlevcode
    br.SkipBits(5); // lfemixlevcod
  if (bsi.strmtyp == 0)
  {
    if (br.ReadBits(1) == 1) // pgmscle
      br.SkipBits(6); // pgmscl
    if (bsi.acmod == 0 && br.ReadBits(1) == 1) // pgmscl2e
      br.SkipBits(6); // pgmscl2
    if (br.ReadBits(1) == 1) // extpgmscle
      br.SkipBits(6); // extpgmscl

    switch (const unsigned int mixdef{br.ReadBits(2)}; mixdef)
    {
      case 1:
        br.SkipBits(5); // premixcmpsel, drcsrc, premixcmpscl
        break;
      case 2:
        br.SkipBits(12); // mixdata
        break;
      case 3:
      {
        unsigned int mixdeflen{br.ReadBits(5)};
        if (br.ReadBits(1) == 1) // mixdata2e
        {
          br.SkipBits(5); // premixcmpsel, drcsrc, premixcmpscl
          for (int i = 0; i < 7; ++i)
          {
            if (br.ReadBits(1) == 1)
              br.SkipBits(4);
          }
          if (br.ReadBits(1) == 1) // addche
          {
            for (int i = 0; i < 2; ++i)
            {
              if (br.ReadBits(1) == 1)
                br.SkipBits(4);
            }
          }
        }
        if (br.ReadBits(1) == 1) // mixdata3e
        {
          br.SkipBits(5); // spchdat
          if (br.ReadBits(1) == 1) // addspchdate
          {
            br.SkipBits(7); // spchdat1, spchan1att
            if (br.ReadBits(1) == 1) // addspchdat1e
              br.SkipBits(8); // spchdat2, spchan2att
          }
        }
        unsigned int mixdata{8 * (mixdeflen + 2)};
        br.SkipBits(mixdata); // mixdata
        br.ByteAlign();
        break;
      }
      default:
        break;
    }
    if (bsi.acmod < 2)
    {
      if (br.ReadBits(1) == 1) // paninfoe
        br.SkipBits(14); // panmean, paninfo
      if (bsi.acmod == 0 && br.ReadBits(1) == 1) // paninfo2e
        br.SkipBits(14); // panmean2, paninfo2
    }
    if (br.ReadBits(1) == 1) // frmmixcfginfoe
    {
      if (bsi.fscod2_numblkscod == 0)
        br.SkipBits(5); // blkmixcfginfo[0]
      else
      {
        unsigned int number_of_blocks_per_syncframe{
            NUM_AUDIO_BLOCK_PER_SYNCFRAME[bsi.fscod2_numblkscod]};
        for (unsigned int blk = 0; blk < number_of_blocks_per_syncframe; blk++)
        {
          if (br.ReadBits(1) == 1) // blkmixcfginfoe
            br.SkipBits(14); // blkmixcfginfo[blk]
        }
      }
    }
  }
}

void SkipEAC3InfoMetadata(BitReader& br, const EAC3BitStreamInfo& bsi)
{
  br.SkipBits(3); // bsmod
  br.SkipBits(1); // copyrightb
  br.SkipBits(1); // origbs
  if (bsi.acmod == 2)
    br.SkipBits(4); // dsurmod, dheadphonmod
  if (bsi.acmod >= 6)
    br.SkipBits(2); // dsurexmod
  if (br.ReadBits(1)) // audioprodie
  {
    br.SkipBits(5); // mixlevel
    br.SkipBits(2); // roomtyp
    br.SkipBits(1); // adconvtyp
  }
  if (bsi.acmod == 0 && br.ReadBits(1) == 1) // audprodi2e
  {
    br.SkipBits(5); // mixlevel2
    br.SkipBits(2); // roomtyp2
    br.SkipBits(1); // adconvtyp2
  }

  if (bsi.fscod < 3)
    br.SkipBits(1); // sourcefscod
}

void SkipEAC3HeaderBits(BitReader& br, const EAC3BitStreamInfo& bsi)
{
  // Mixing metadata exists
  if (br.ReadBits(1)) // mixmdate
    SkipEAC3MixingMetadata(br, bsi);

  // Info metadata exists
  if (br.ReadBits(1)) // infomdate
    SkipEAC3InfoMetadata(br, bsi);

  if (bsi.strmtyp == 0 && bsi.fscod2_numblkscod != 3)
    br.SkipBits(1); // convsync

  if (bsi.strmtyp == 2 && (bsi.fscod2_numblkscod == 3 || br.ReadBits(1) == 1)) // blkid
    br.SkipBits(6); // frmsizecod
}

void GetAtmos(BitReader& br, TSAudioStreamInfo* streamInfo)
{
  // Additional bitstream info exists (contains JOC/ATMOS)
  if (br.ReadBits(1)) // addbsie
  {
    unsigned int addbsil{br.ReadBits(6)};
    for (unsigned int i = 0; i < addbsil + 1; i++) // addbsi bytes
    {
      unsigned int addbsi{br.ReadBits(8)};
      // Check for JOC extension (addbsi == 1)
      if (addbsi == 1)
      {
        // Has JOC
        streamInfo->isAtmos = true;
        return;
      }
    }
  }
}

bool ParseEAC3Bitstream(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  BitReader br(buffer.subspan(2));

  EAC3BitStreamInfo bsi;
  if (!GetEAC3ChannelCountAndSampleRate(br, bsi, streamInfo))
    return false; // Not EAC3 bitstream

  // Continue to parse for JOC (Atmos)
  SkipEAC3HeaderBits(br, bsi);
  GetAtmos(br, streamInfo);

  streamInfo->completed = (++streamInfo->seen) >= HEADERS_PARSED_FOR_COMPLETE;

  return true;
}

bool ParseAC3Bitstream(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  if (!streamInfo)
    return false;

  // Find (E)AC-3 sync word
  const int end{static_cast<int>(buffer.size())};
  const auto indices{std::views::iota(0, end - 1)};
  const auto it = std::ranges::find_if(indices, [&buffer](const unsigned int i)
                                       { return GetWord(buffer, i) == AC3_SYNCWORD; });
  if (it == indices.end() || *it + 8 >= end)
    return false;
  int offset{*it};

  // Parse bsi to determine if EAC-3
  const unsigned int header{GetDWord(buffer, offset + 2)};
  const unsigned int bsid{GetBits(header, 8, 5)};

  if (bsid == 16)
    return ParseEAC3Bitstream(buffer, streamInfo);
  if (bsid == 6 || bsid == 8)
    return ParseAC3Bitstream(buffer, offset, streamInfo);
  return false;
}

std::optional<DTSFrame> ParseDTSFrame(const std::span<std::byte>& data)
{
  DTSFrame dtsFrame;
  for (unsigned int i = 0; i + 4 <= data.size(); i++)
  {
    const DTSSyncWords sync{GetDWord(data, i)};
    using enum DTSSyncWords;
    if (sync == CORE_16BIT_BE || sync == SUBSTREAM)
    {
      dtsFrame.syncPos = i;
      dtsFrame.syncWord = sync;
      dtsFrame.data = data.subspan(i);
      return dtsFrame;
    }
  }
  return std::nullopt;
}

std::optional<unsigned int> FindDTSSyncWord(const std::span<std::byte>& buffer,
                                            unsigned int start,
                                            const std::array<std::byte, 4>& syncWord)
{
  if (start > buffer.size() - syncWord.size())
    return std::nullopt;

  const auto searchRange{buffer.subspan(start, buffer.size() - start)};
  if (const auto result{std::ranges::search(searchRange, syncWord)}; !result.empty())
    return static_cast<unsigned int>(result.begin() - searchRange.begin());
  return std::nullopt;
}

bool ParseDTSBitstream(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  if (buffer.size() < DTS_HEADER_SIZE)
    return false;

  auto dtsData{ParseDTSFrame(buffer)};
  if (!dtsData)
    return false;

  unsigned int offset{0};
  if (dtsData->syncWord == DTSSyncWords::CORE_16BIT_BE)
  {
    // Parse 16-bit DTS core header
    if (dtsData->syncPos + DTS_HEADER_SIZE >= buffer.size())
      return false;

    // Parse DTS header
    const uint64_t header{GetQWord(buffer, 4 + dtsData->syncPos)};
    unsigned int amode{static_cast<unsigned int>(GetBits64(header, 36, 6))};
    unsigned int sfreq{static_cast<unsigned int>(GetBits64(header, 30, 4))};
    unsigned int lff{static_cast<unsigned int>(GetBits64(header, 11, 2))};

    // Set parameters
    const auto& sampleRates{DTS_SAMPLE_RATES};
    const auto& channelCounts{DTS_CHANNEL_COUNTS};

    if (sfreq < sampleRates.size())
      streamInfo->sampleRate = sampleRates[sfreq];

    if (amode < channelCounts.size())
      streamInfo->channels = channelCounts[amode];

    if (lff == 1 || lff == 2)
      streamInfo->channels++; // Add LFE channel

    // See if there is a DTS substream header in this block
    if (auto substream{
            FindDTSSyncWord(buffer, dtsData->syncPos + DTS_HEADER_SIZE, DTS_SYNCWORD_SUBSTREAM)};
        !substream.has_value())
      return true;
    else
      offset = *substream;
  }

  if (dtsData->syncWord == DTSSyncWords::SUBSTREAM || offset > 0)
  {
    // Substream header found
    streamInfo->hasSubstream = true;
    if (auto xll{FindDTSSyncWord(buffer, dtsData->syncPos + 10, DTS_SYNCWORD_XLL)}; xll.has_value())
      streamInfo->isXLL = true;

    if (auto xllx{FindDTSSyncWord(buffer, dtsData->syncPos + 10, DTS_SYNCWORD_XLL_X)};
        xllx.has_value())
      streamInfo->isXLLX = true;

    if (auto xllximax{FindDTSSyncWord(buffer, dtsData->syncPos + 10, DTS_SYNCWORD_XLL_X_IMAX)};
        xllximax.has_value())
      streamInfo->isXLLXIMAX = true;
  }

  streamInfo->completed = (++streamInfo->seen) >= HEADERS_PARSED_FOR_COMPLETE;

  return true;
}

bool ParseTrueHDHeader(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  unsigned int format_info{GetDWord(buffer, 4)};
  unsigned int flags{GetWord(buffer, 10)};

  // Set sample rate
  const auto& sampleRates{TRUEHD_SAMPLE_RATES};
  if (unsigned int audio_sampling_frequency{GetBits(format_info, 32, 4)};
      audio_sampling_frequency < sampleRates.size())
    streamInfo->sampleRate = sampleRates[audio_sampling_frequency];

  bool ch6_multichannel_type{GetBits(format_info, 28, 1) == 1};
  bool ch8_multichannel_type{GetBits(format_info, 27, 1) == 1};
  unsigned int ch2_presentation_channel_modifier{GetBits(format_info, 26, 2)};
  unsigned int ch6_presentation_channel_assignment{GetBits(format_info, 22, 5)};
  unsigned int ch8_presentation_channel_assignment{GetBits(format_info, 15, 13)};
  bool ch8_flag{GetBits(flags, 12, 1) == 1};

  if (ch8_multichannel_type)
  {
    unsigned int ch8_1;
    unsigned int ch8_2;
    if (!ch8_flag)
    {
      ch8_1 = ch8_presentation_channel_assignment & CH8_SINGLE_CHANNEL_MASK;
      ch8_2 = ch8_presentation_channel_assignment & CH8_DUAL_CHANNEL_MASK;
    }
    else
    {
      ch8_1 = ch8_presentation_channel_assignment & CH8_16_SINGLE_CHANNEL_ALTERNATE_MASK;
      ch8_2 = ch8_presentation_channel_assignment & CH8_16_DUAL_CHANNEL_ALTERNATE_MASK;
    }
    streamInfo->channels = std::popcount(ch8_2) * 2 + std::popcount(ch8_1);
  }
  else if (ch6_multichannel_type)
  {
    unsigned int ch6_1{ch6_presentation_channel_assignment & CH8_16_SINGLE_CHANNEL_ALTERNATE_MASK};
    unsigned int ch6_2{ch6_presentation_channel_assignment & CH8_16_DUAL_CHANNEL_ALTERNATE_MASK};
    streamInfo->channels = std::popcount(ch6_2) * 2 + std::popcount(ch6_1);
  }
  else
    streamInfo->channels = (ch2_presentation_channel_modifier == 3) ? 1 : 2;

  // Look for extended channel info
  unsigned int header{GetByte(buffer, 16)};
  unsigned int substreams{GetBits(header, 8, 4)};
  unsigned int substream_info{GetByte(buffer, 17)};
  bool ch16_present{GetBits(substream_info, 8, 1) == 1};
  uint64_t channel_meaning{GetQWord(buffer, 18)};
  if (bool extra_channel_meaning_present{(GetBits64(channel_meaning, 1, 1) == 1)};
      extra_channel_meaning_present && ch16_present)
  {
    unsigned int extra{GetDWord(buffer, 26)};
    unsigned int ch16_channel_count{GetBits(extra, 17, 5)};
    streamInfo->channels = ch16_channel_count + 1;
  }

  streamInfo->isAtmos = ch16_present && substreams == 4;

  streamInfo->completed = (++streamInfo->seen) >= HEADERS_PARSED_FOR_COMPLETE;

  return true;
}

bool ParseTrueHDBitstream(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  unsigned int offset{0};
  while (true)
  {
    // Search for first 3 bytes of header
    const auto data{buffer.subspan(offset)};
    auto result{std::ranges::search(data, TRUEHD_COMMON_SYNC)};
    if (result.empty() ||
        static_cast<unsigned int>(data.end() - result.begin()) < TRUEHD_MINIMUM_HEADER_SIZE)
      break;

    offset += std::distance(data.begin(), result.begin());

    // Check signature
    if (GetWord(buffer, offset + 8) != TRUEHD_HEADER_SIGNATURE)
      break;

    if (const unsigned int flag{GetByte(buffer, offset + 3)}; flag == DOLBY_FLAG)
      ParseTrueHDHeader(buffer.subspan(offset), streamInfo);

    offset += TRUEHD_MINIMUM_HEADER_SIZE;
  }

  return true;
}

bool ParseLPCMBitstream(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  // Sub-stream ID (should be 0xA0-0xAF for LPCM)
  if (unsigned int sub_stream_id{GetByte(buffer, 1)}; (sub_stream_id & 0xA0) != 0xA0)
    return false;

  unsigned int header{GetByte(buffer, 2)};
  unsigned int channel_info{GetBits(header, 8, 4)};
  unsigned int sample_rate_code{GetBits(header, 4, 4)};

  if (channel_info < LPCM_CHANNEL_COUNTS.size())
    streamInfo->channels = LPCM_CHANNEL_COUNTS[channel_info];
  if (LPCM_SAMPLE_RATES.contains(sample_rate_code))
    streamInfo->sampleRate = LPCM_SAMPLE_RATES.find(sample_rate_code)->second;

  streamInfo->completed = (++streamInfo->seen) >= HEADERS_PARSED_FOR_COMPLETE;

  return true;
}

// Video related

void SkipProfileTierLevelBits(bool profilePresentFlag,
                              unsigned int sps_max_sub_layers_minus1,
                              BitReader& br)
{
  if (profilePresentFlag)
    br.SkipBits(88); // The general profile/tier/level section is always 88 bits

  br.SkipBits(8); // general_level_idc

  // Read the sub-layer presence flags to know what follows
  std::vector<unsigned int> sub_layer_profile_present_flag(sps_max_sub_layers_minus1);
  std::vector<unsigned int> sub_layer_level_present_flag(sps_max_sub_layers_minus1);

  for (unsigned int i = 0; i < sps_max_sub_layers_minus1; i++)
  {
    sub_layer_profile_present_flag[i] = br.ReadBits(1);
    sub_layer_level_present_flag[i] = br.ReadBits(1);
  }

  // Reserved zero bits
  if (sps_max_sub_layers_minus1 > 0)
    br.SkipBits((8 - sps_max_sub_layers_minus1) * 2);

  // Skip sub-layer bits based on presence flags
  for (unsigned int i = 0; i < sps_max_sub_layers_minus1; i++)
  {
    if (sub_layer_profile_present_flag[i])
      br.SkipBits(88); // Sub-layer profile/tier section is also always 88 bits
    if (sub_layer_level_present_flag[i])
      br.SkipBits(8); // sub_layer_level_idc
  }
}

void ParseScalingListData(BitReader& br)
{
  for (unsigned int sizeId = 0; sizeId < 4; sizeId++)
  {
    unsigned int limit{sizeId == 3 ? 2u : 6u};
    for (unsigned int matrixId = 0; matrixId < limit; matrixId++)
    {
      if (br.ReadBits(1) == 0) // scaling_list_pred_mode_flag
        br.SkipUE(); // scaling_list_pred_matrix_id_delta
      else
      {
        unsigned int coefNum{static_cast<unsigned int>(std::min(64, 1 << (4 + (sizeId << 1))))};
        if (sizeId > 1)
          br.SkipSE(); // scaling_list_dc_coef_minus8
        for (unsigned int i = 0; i < coefNum; i++)
          br.SkipSE(); // scaling_list_delta_coef
      }
    }
  }
}

void ParseShortTermRefPicSet(BitReader& br, unsigned int idx, unsigned int numSets)
{
  if (idx != 0 && br.ReadBits(1) == 1) // inter_ref_pic_set_prediction_flag
  {
    if (idx == numSets)
      br.SkipUE(); // delta_idx_minus1
    br.SkipBits(1); // delta_rps_sign
    br.SkipUE(); // abs_delta_rps_minus1
    return;
  }

  unsigned int numNegative{br.ReadUE()};
  unsigned int numPositive{br.ReadUE()};

  for (unsigned int i = 0; i < numNegative; i++)
  {
    br.SkipUE(); // delta_poc_s0_minus1
    br.SkipBits(1); // used_by_curr_pic_s0_flag
  }

  for (unsigned int i = 0; i < numPositive; i++)
  {
    br.SkipUE(); // delta_poc_s1_minus1
    br.SkipBits(1); // used_by_curr_pic_s1_flag
  }
}

float ParseVUI(BitReader& br)
{
  float ar{0.0};

  if (br.ReadBits(1) == 1) // aspect_ratio_info_present_flag
  {
    unsigned int aspectRatioIdc{br.ReadBits(8)};
    if (aspectRatioIdc == H265_EXTENDED_SAR)
    {
      unsigned int width{br.ReadBits(16)};
      unsigned int height{br.ReadBits(16)};
      ar = (width > 0) ? static_cast<float>(height) / static_cast<float>(width) : 0.0f;
    }
    else if (aspectRatioIdc < ASPECT_RATIOS.size())
      ar = ASPECT_RATIOS[aspectRatioIdc];
  }

  return ar;
}

bool ParseH265SPS(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing H265 SPS");

  BitReader br(buffer);

  br.SkipBits(4); // sps_video_parameter_set_id
  const unsigned int sps_max_sub_layers_minus1{br.ReadBits(3)};
  br.SkipBits(1); // sps_temporal_id_nesting_flag

  SkipProfileTierLevelBits(true, sps_max_sub_layers_minus1, br);

  br.SkipUE(); // sps_seq_parameter_set_id
  const unsigned int chroma_format_idc{br.ReadUE()};
  if (chroma_format_idc == 3)
    br.SkipBits(1);

  streamInfo->width = br.ReadUE();
  streamInfo->height = br.ReadUE();

  if (br.ReadBits(1) == 1)
  { // conformance_window_flag
    const unsigned int left_offset{br.ReadUE()};
    const unsigned int right_offset{br.ReadUE()};
    const unsigned int top_offset{br.ReadUE()};
    const unsigned int bottom_offset{br.ReadUE()};

    const unsigned int sub_width_c{(chroma_format_idc == 1 || chroma_format_idc == 2) ? 2u : 1u};
    const unsigned int sub_height_c{(chroma_format_idc == 1) ? 2u : 1u};

    streamInfo->width -= (left_offset + right_offset) * sub_width_c;
    streamInfo->height -= (top_offset + bottom_offset) * sub_height_c;
  }

  streamInfo->bitDepth = br.ReadUE() + 8;

  br.SkipUE(); // bit_depth_chroma_minus8
  br.SkipUE(); // log2_max_pic_order_cnt_lsb_minus4

  // sps_sub_layer_ordering_info_present_flag
  bool sps_sub_layer_ordering_info_present_flag{br.ReadBits(1) == 1};
  unsigned int start{sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1};

  for (unsigned int i = start; i <= sps_max_sub_layers_minus1; i++)
  {
    br.SkipUE(); // sps_max_dec_pic_buffering_minus1
    br.SkipUE(); // sps_max_num_reorder_pics
    br.SkipUE(); // sps_max_latency_increase_plus1
  }

  br.SkipUE(); // log2_min_luma_coding_block_size_minus3
  br.SkipUE(); // log2_diff_max_min_luma_coding_block_size
  br.SkipUE(); // log2_min_luma_transform_block_size_minus2
  br.SkipUE(); // log2_diff_max_min_luma_transform_block_size
  br.SkipUE(); // max_transform_hierarchy_depth_inter
  br.SkipUE(); // max_transform_hierarchy_depth_intra

  if (br.ReadBits(1) == 1 && // scaling_list_enabled_flag
      br.ReadBits(1) == 1) // sps_scaling_list_data_present_flag
    ParseScalingListData(br);

  br.SkipBits(1); // amp_enabled_flag
  br.SkipBits(1); // sample_adaptive_offset_enabled_flag

  if (br.ReadBits(1) == 1) // pcm_enabled_flag
  {
    br.SkipBits(4); // pcm_sample_bit_depth_luma_minus1
    br.SkipBits(4); // pcm_sample_bit_depth_chroma_minus1
    br.SkipUE(); // log2_min_pcm_luma_coding_block_size_minus3
    br.SkipUE(); // log2_diff_max_min_pcm_luma_coding_block_size
    br.SkipBits(1); // pcm_loop_filter_disabled_flag
  }

  unsigned int num_short_term_ref_pic_sets{br.ReadUE()};
  for (unsigned int i = 0; i < num_short_term_ref_pic_sets; i++)
    ParseShortTermRefPicSet(br, i, num_short_term_ref_pic_sets);

  if (br.ReadBits(1) == 1) // long_term_ref_pics_present_flag
  {
    unsigned int num_long_term_ref_pics_sps{br.ReadUE()};
    for (unsigned int i = 0; i < num_long_term_ref_pics_sps; i++)
    {
      br.SkipUE(); // lt_ref_pic_poc_lsb_sps
      br.SkipBits(1); // used_by_curr_pic_lt_sps_flag
    }
  }

  br.SkipBits(1); // sps_temporal_mvp_enabled_flag
  br.SkipBits(1); // strong_intra_smoothing_enabled_flag

  if (br.ReadBits(1) == 1) // vui_parameters_present_flag
    streamInfo->aspectRatio = ParseVUI(br);

  streamInfo->completed = true;

  return true;
}

bool ParseH264SPS(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing H264 SPS");

  BitReader br(buffer);

  unsigned int profile_idc{br.ReadBits(8)};
  br.SkipBits(8); // constraint flags
  br.SkipBits(8); // level_idc
  br.SkipUE(); // seq_parameter_set_id

  if (profile_idc != H264_PROFILE_MAIN && profile_idc != H264_PROFILE_HIGH)
    return false; // Blurays only support Main and High profiles

  if (profile_idc == H264_PROFILE_HIGH)
  {
    unsigned int chroma_format{br.ReadUE()};
    if (chroma_format == 3)
      br.SkipBits(1);

    streamInfo->bitDepth = br.ReadUE() + 8; // bit_depth_luma_minus8
    br.SkipUE(); // bit_depth_chroma_minus8
    br.SkipBits(1); // qpprime_y_zero_transform_bypass

    if (br.ReadBits(1) == 1) // seq_scaling_matrix_present_flag
    {
      for (int i = 0; i < ((chroma_format != 3) ? 8 : 12); i++)
        br.SkipBits(1);
    }
  }
  else
    streamInfo->bitDepth = 8;

  br.SkipUE(); // log2_max_frame_num_minus4

  if (unsigned int pic_order_cnt_type{br.ReadUE()}; pic_order_cnt_type == 0)
    br.SkipUE(); // log2_max_pic_order_cnt_lsb_minus4
  else if (pic_order_cnt_type == 1)
  {
    br.SkipBits(1); // delta_pic_order_always_zero_flag
    br.SkipSE(); // offset_for_non_ref_pic
    br.SkipSE(); // offset_for_top_to_bottom_field
    unsigned int num_ref_frames_in_pic_order_cnt_cycle{br.ReadUE()};
    for (unsigned int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
      br.SkipSE(); // offset_for_ref_frame[i]
  }

  br.SkipUE(); // max_num_ref_frames
  br.SkipBits(1); // gaps_in_frame_num_allowed_flag

  unsigned int pic_width_in_mbs{br.ReadUE() + 1};
  unsigned int pic_height_in_map_units{br.ReadUE() + 1};
  bool frame_mbs_only_flag{br.ReadBits(1) == 1};

  if (!frame_mbs_only_flag)
    br.SkipBits(1); // mb_adaptive_frame_field_flag
  br.SkipBits(1); // direct_8x8_inference

  bool frame_cropping_flag{br.ReadBits(1) == 1};
  unsigned int frame_crop_left_offset{0};
  unsigned int frame_crop_right_offset{0};
  unsigned int frame_crop_top_offset{0};
  unsigned int frame_crop_bottom_offset{0};

  if (frame_cropping_flag)
  {
    frame_crop_left_offset = br.ReadUE();
    frame_crop_right_offset = br.ReadUE();
    frame_crop_top_offset = br.ReadUE();
    frame_crop_bottom_offset = br.ReadUE();
  }

  streamInfo->width =
      pic_width_in_mbs * 16 - (frame_crop_left_offset + frame_crop_right_offset) * 2;
  streamInfo->height = (2 - (frame_mbs_only_flag ? 1 : 0)) * pic_height_in_map_units * 16 -
                       (frame_crop_top_offset + frame_crop_bottom_offset) * 2;

  if (br.ReadBits(1) == 1) // vui_parameters_present_flag
    streamInfo->aspectRatio = ParseVUI(br);

  streamInfo->completed = true;

  return true;
}

bool ParseITUT35UserData(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  if (unsigned int length{static_cast<unsigned int>(buffer.size())}; length < 3)
    return false;

  unsigned int countryCode{GetByte(buffer, 0)};
  unsigned int providerCode{GetWord(buffer, 1)};

  // HDR10+ detection
  // Country code 0xB5 (USA) + provider code for HDR10+
  if (countryCode == 0xB5 && providerCode == 0x003C)
    streamInfo->hdr10Plus = true;

  // Dolby Vision detection via ITU-T T.35
  // Country code 0xB5 (USA) + Dolby provider codes
  if (countryCode == 0xB5 && providerCode == 0x003B)
    streamInfo->dolbyVision = true;

  return true;
}

std::optional<unsigned int> GetCumulativeNumber(const std::span<std::byte>& buffer,
                                                unsigned int& offset)
{
  unsigned int length{static_cast<unsigned int>(buffer.size())};
  unsigned int cumulative{0};
  while (offset < length && GetByte(buffer, offset) == 0xFF)
  {
    cumulative += 255;
    offset++;
  }
  if (offset >= length)
    return std::nullopt;
  cumulative += GetByte(buffer, offset);
  offset++;
  return cumulative;
}

void CheckPayloadForDolbyVision(const std::span<std::byte>& buffer,
                                unsigned int offset,
                                TSVideoStreamInfo* streamInfo)
{
  auto uuid{buffer.subspan(offset, 16)};
  if (std::ranges::equal(uuid, DOLBY_VISION_PROFILE_7_UUID))
    streamInfo->dolbyVision = true;
}

bool ParseSEI(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  unsigned int length{static_cast<unsigned int>(buffer.size())};
  if (length < 2)
    return false;

  unsigned int offset{0};
  while (offset < length - 2)
  {
    // Read payload type
    unsigned int payloadType{0};
    if (const auto i{GetCumulativeNumber(buffer, offset)}; !i.has_value())
      break;
    else
      payloadType = *i;

    // Read payload size
    unsigned int payloadSize;
    if (const auto i{GetCumulativeNumber(buffer, offset)}; !i.has_value())
      break;
    else
      payloadSize = *i;

    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing SEI - payload type {}", payloadType);

    // Check for HDR SEI messages
    switch (payloadType)
    {
      case SEI_PAYLOAD_MASTERING_DISPLAY_COLOUR_VOLUME:
      case SEI_PAYLOAD_CONTENT_LIGHT_LEVEL_INFO:
      {
        streamInfo->hdr10 = true;
        break;
      }
      case SEI_PAYLOAD_UNREGISTERED:
      {
        // User data unregistered - check for Dolby Vision
        if (offset + 16 <= length)
          CheckPayloadForDolbyVision(buffer, offset, streamInfo);
        break;
      }
      case SEI_PAYLOAD_REGISTERED_ITU_T_T35:
      {
        ParseITUT35UserData(buffer.subspan(offset), streamInfo);
        break;
      }
      default:
        break;
    }

    offset += payloadSize;
  }

  return true;
}

std::vector<std::byte> RemoveEmulationPreventionBytes(const std::span<std::byte>& buffer,
                                                      VideoCodec videoCodec)
{
  if (buffer.size() < 3)
    return {buffer.begin(), buffer.end()};

  std::vector<std::byte> unit;
  unit.reserve(buffer.size());

  // Copy NAL unit header
  unsigned int offset{1};
  unit.push_back(buffer[0]);
  if (videoCodec == VideoCodec::H265)
  {
    unit.push_back(buffer[1]);
    offset += 1;
  }

  const std::byte* data{buffer.data()};
  const std::byte* end{data + buffer.size()};
  const std::byte* pos{data + offset};

  while (pos < end)
  {
    const void* zero_ptr{std::memchr(pos, 0x00, end - pos)};
    if (!zero_ptr)
    {
      // No more zeros, copy remaining bytes
      unit.insert(unit.end(), pos, end);
      break;
    }
    const std::byte* zero{static_cast<const std::byte*>(zero_ptr)};

    // Copy all bytes before the zero
    unit.insert(unit.end(), pos, zero);

    // Check for 0x000003 pattern
    if (zero + 2 < end && zero[1] == std::byte{0x00} && zero[2] == std::byte{0x03})
    {
      // Emulation prevention sequence found
      unit.push_back(std::byte{0x00});
      unit.push_back(std::byte{0x00});
      pos = zero + 3; // Skip 0x00, 0x00, 0x03
    }
    else
    {
      // Not an emulation sequence, copy the zero and continue
      unit.push_back(std::byte{0x00});
      pos = zero + 1;
    }
  }

  return unit;
}

bool GetNALType(const std::span<std::byte>& data,
                unsigned int& relativeOffset,
                VideoCodec videoCodec,
                unsigned int& nal_unit_type,
                unsigned int& header,
                TSVideoStreamInfo* streamInfo)
{
  switch (videoCodec)
  {
    using enum VideoCodec;
    case H264:
    {
      header = GetByte(data, relativeOffset);
      relativeOffset += 1;
      nal_unit_type = GetBits(header, 5, 5);
      break;
    }
    case H265:
    {
      header = GetWord(data, relativeOffset);
      relativeOffset += 2;
      nal_unit_type = GetBits(header, 15, 6);
      if (unsigned int nuh_layer_id{GetBits(header, 9, 6)}; nuh_layer_id > 0)
        streamInfo->isEnhancementLayer = true;
      break;
    }
    default:
      return false;
  }

  return true;
}

void CheckFor3D(const std::span<std::byte>& data,
                unsigned int& relativeOffset,
                unsigned int nal_unit_type,
                TSVideoStreamInfo* streamInfo)
{
  switch (nal_unit_type)
  {
    case H264_PREFIX_NAL_UNIT:
    case H264_CODED_SLICE_EXTENSION:
    case H264_CODED_SLICE_EXTENSION_FOR_DEPTH_VIEW:
    {
      const unsigned int extension{GetByte(data, relativeOffset)};
      if (const bool svc_or_avc_3d_extension_flag{GetBits(extension, 8, 1) == 1};
          nal_unit_type == H264_CODED_SLICE_EXTENSION_FOR_DEPTH_VIEW &&
          svc_or_avc_3d_extension_flag)
      {
        relativeOffset += 2;
        streamInfo->is3d = true; // AVC 3D
      }
      else if (nal_unit_type != H264_CODED_SLICE_EXTENSION_FOR_DEPTH_VIEW &&
               !svc_or_avc_3d_extension_flag)
      {
        relativeOffset += 3;
        streamInfo->is3d = true; // MVC
      }
      else
        relativeOffset += 3;

      break;
    }
    default:
      break;
  }
}

void ProcessNALUnit(std::vector<std::byte>& unit,
                    unsigned int nal_unit_type,
                    unsigned int header,
                    TSVideoStreamInfo* streamInfo)
{
  switch (nal_unit_type)
  {
    case H264_NAL_SPS:
      ParseH264SPS(unit, streamInfo);
      break;
    case H265_NAL_SPS:
      ParseH265SPS(unit, streamInfo);
      break;
    case H264_NAL_SEI:
    case H265_NAL_SEI_PREFIX:
    case H265_NAL_SEI_SUFFIX:
      ParseSEI(unit, streamInfo);
      break;
    case DOLBY_VISION_RPU:
      if (header == DOLBY_VISION_RPU_HEADER)
        streamInfo->dolbyVision = true;
      break;
    case DOLBY_VISION_EL:
      if (header == DOLBY_VISION_EL_HEADER)
        streamInfo->dolbyVision = true;
      break;
    default:
      break;
  }
}

bool ParseNAL(const std::span<std::byte>& buffer,
              VideoCodec videoCodec,
              TSVideoStreamInfo* streamInfo)
{
  if (buffer.size() < 4 || !streamInfo)
    return false;

  unsigned int offset{0};
  while (true)
  {
    const auto data{buffer.subspan(offset)};
    const auto result{std::ranges::search(data, NAL_START_CODE_3)};
    if (result.empty() || data.end() - result.begin() < 5)
      break;

    // See if the end of the NAL unit is within the buffer
    unsigned int relativeOffset{
        static_cast<unsigned int>(std::distance(data.begin(), result.begin())) + 3};
    const auto end{std::ranges::search(data.subspan(relativeOffset), NAL_START_CODE_3)};

    // Extract NAL unit type from header
    unsigned int nal_unit_type{0};
    unsigned int header;
    if (!GetNALType(data, relativeOffset, videoCodec, nal_unit_type, header, streamInfo))
      return false; // Not H264/5

    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing NAL - type {}", nal_unit_type);

    // Look for markers of a 3D stream
    CheckFor3D(data, relativeOffset, nal_unit_type, streamInfo);

    const size_t length{end.empty() ? data.size() - relativeOffset
                                    : end.data() - data.data() - relativeOffset};
    std::vector<std::byte> unit{
        RemoveEmulationPreventionBytes(data.subspan(relativeOffset, length), videoCodec)};

    ProcessNALUnit(unit, nal_unit_type, header, streamInfo);

    if (end.empty())
      break;
    offset += end.data() - data.data();
  }

  return false;
}

bool ParseVC1(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  auto result{std::ranges::search(buffer, VC1_SEQUENCE_HEADER_START_CODE)};
  if (result.empty() || buffer.end() - result.begin() < 8)
    return false;

  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing VC1 Sequence Header");

  unsigned int offset{static_cast<unsigned int>(result.begin() - buffer.begin()) + 4};

  unsigned int header{GetWord(buffer, offset)};
  if (unsigned int profile{GetBits(header, 16, 2)}; profile != 3)
    return false; // Not Advanced Profile

  header = GetDWord(buffer, offset + 2);

  // Max coded width (12 bits) - multiply by 2 and add 2
  unsigned int max_coded_width{GetBits(header, 32, 12)};
  streamInfo->width = (max_coded_width + 1) * 2;

  // Max coded height (12 bits) - multiply by 2 and add 2
  unsigned int max_coded_height{GetBits(header, 20, 12)};
  streamInfo->height = (max_coded_height + 1) * 2;

  // Display extension data
  if (bool display_ext{GetBits(header, 2, 1) == 1}; display_ext)
  {
    uint64_t data{(static_cast<uint64_t>(header) << 32) | GetDWord(buffer, offset + 6)};
    streamInfo->width = static_cast<unsigned int>(GetBits64(data, 33, 14)) + 1;
    streamInfo->height = static_cast<unsigned int>(GetBits64(data, 19, 14)) + 1;
    if (bool aspect_ratio_flag{GetBits64(data, 5, 1) == 1}; aspect_ratio_flag)
    {
      unsigned int aspect_ratio{static_cast<unsigned int>(GetBits64(data, 4, 4))};
      if (aspect_ratio == VC1_EXTENDED_SAR)
      {
        header = GetDWord(buffer, offset + 10);
        unsigned int aspect_horiz_size{GetBits(header, 32, 8)};
        unsigned int aspect_vert_size{GetBits(header, 24, 8)};
        streamInfo->aspectRatio = (aspect_vert_size > 0) ? static_cast<float>(aspect_horiz_size) /
                                                               static_cast<float>(aspect_vert_size)
                                                         : 0.0f;
      }
      else
        streamInfo->aspectRatio = VC1_ASPECT_RATIOS[aspect_ratio];
    }
  }

  streamInfo->bitDepth = 8;

  streamInfo->completed = true;

  return true;
}

bool ParseMPEG2(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  if (auto result{std::ranges::search(buffer, MPEG2_SEQUENCE_HEADER_START_CODE)};
      result.empty() || buffer.end() - result.begin() < 8)
    return false;

  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing MPEG2 Sequence Header");

  unsigned int header{GetDWord(buffer, 4)};
  unsigned int horizontal_size_value{GetBits(header, 32, 12)};
  unsigned int vertical_size_value{GetBits(header, 20, 12)};
  streamInfo->width = horizontal_size_value;
  streamInfo->height = vertical_size_value;

  if (unsigned int aspect_ratio_information{GetBits(header, 8, 4)}; aspect_ratio_information < 2)
    streamInfo->aspectRatio = MPEG2_DISPLAY_ASPECT_RATIOS[aspect_ratio_information];
  else if (aspect_ratio_information < MPEG2_DISPLAY_ASPECT_RATIOS.size())
    streamInfo->aspectRatio = vertical_size_value > 0
                                  ? MPEG2_DISPLAY_ASPECT_RATIOS[aspect_ratio_information] *
                                        static_cast<float>(horizontal_size_value) /
                                        static_cast<float>(vertical_size_value)
                                  : 0.0f;

  streamInfo->bitDepth = 8;

  streamInfo->completed = true;

  return true;
}

void ProcessStream(const PESPacket& pesPacket, const std::shared_ptr<TSStreamInfo>& streamInfo)
{
  switch (streamInfo->streamType)
  {
    using enum ENCODING_TYPE;
    case VIDEO_HEVC:
      if (auto* videoInfo{dynamic_cast<TSVideoStreamInfo*>(streamInfo.get())})
        ParseNAL(pesPacket.data, VideoCodec::H265, videoInfo);
      break;
    case VIDEO_H264:
      if (auto* videoInfo{dynamic_cast<TSVideoStreamInfo*>(streamInfo.get())})
        ParseNAL(pesPacket.data, VideoCodec::H264, videoInfo);
      break;
    case VIDEO_H264_MVC:
      if (auto* videoInfo{dynamic_cast<TSVideoStreamInfo*>(streamInfo.get())})
      {
        ParseNAL(pesPacket.data, VideoCodec::H264, videoInfo);
        videoInfo->is3d = true;
      }
      break;
    case VIDEO_VC1:
      if (auto* videoInfo{dynamic_cast<TSVideoStreamInfo*>(streamInfo.get())})
        ParseVC1(pesPacket.data, videoInfo);
      break;
    case VIDEO_MPEG2:
      if (auto* videoInfo{dynamic_cast<TSVideoStreamInfo*>(streamInfo.get())})
        ParseMPEG2(pesPacket.data, videoInfo);
      break;
    case AUDIO_AC3:
    case AUDIO_AC3PLUS:
    case AUDIO_AC3PLUS_SECONDARY:
      if (auto* audioInfo{dynamic_cast<TSAudioStreamInfo*>(streamInfo.get())})
        ParseAC3Bitstream(pesPacket.data, audioInfo);
      break;
    case AUDIO_DTS:
    case AUDIO_DTSHD:
    case AUDIO_DTSHD_MASTER:
    case AUDIO_DTSHD_SECONDARY:
      if (auto* audioInfo{dynamic_cast<TSAudioStreamInfo*>(streamInfo.get())})
        ParseDTSBitstream(pesPacket.data, audioInfo);
      break;
    case AUDIO_TRUHD:
      if (auto* audioInfo{dynamic_cast<TSAudioStreamInfo*>(streamInfo.get())})
        ParseTrueHDBitstream(pesPacket.data, audioInfo);
      break;
    case AUDIO_LPCM:
      if (pesPacket.streamId == 0xBD)
        if (auto* audioInfo{dynamic_cast<TSAudioStreamInfo*>(streamInfo.get())})
          ParseLPCMBitstream(pesPacket.data, audioInfo);
      break;
    case SUB_PG:
    case SUB_IG:
    case SUB_TEXT:
    default:
      streamInfo->completed = true;
      break;
  }
}

bool ParsePES(const TSPacket& tsPacket,
              std::unordered_map<unsigned int, PESAssembler>& pesSection,
              StreamMap& streams)
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing PES - PID 0x{}", fmt::format("{:04x}", tsPacket.pid));

  auto& pesAssembler{pesSection[tsPacket.pid]};
  for (auto& section : pesAssembler.push(tsPacket))
  {
    // Parse PES packet
    auto pesPacket{ParsePESPacket(section)};
    if (!pesPacket.has_value())
      continue; // May not contain audio

    const auto& streamInfo{streams[tsPacket.pid]};
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing PES - Stream PID 0x{} - Type 0x{}",
                fmt::format("{:04x}", streamInfo->pid),
                fmt::format("{:02x}", static_cast<int>(streamInfo->streamType)));

    ProcessStream(*pesPacket, streamInfo);
  }

  return true;
}

bool ParseTSPacket(const std::span<std::byte>& packet,
                   bool& patParsed,
                   bool& pmtParsed,
                   std::set<unsigned int>& pmtPIDs,
                   std::unordered_map<unsigned int, SectionAssembler>& pidSection,
                   std::unordered_map<unsigned int, PESAssembler>& pesSection,
                   StreamMap& streams)
{
  // Parse TS header
  auto tsPacket{ParseTSPacket(packet)};
  if (!tsPacket)
    return false;

  if (tsPacket->pid == 0x00)
    patParsed =
        ParsePAT(tsPacket->payload, tsPacket->payloadUnitStartIndicator, pmtPIDs, pidSection);
  else if (pmtPIDs.contains(tsPacket->pid))
    pmtParsed |= ParsePMT(tsPacket->pid, tsPacket->payload, tsPacket->payloadUnitStartIndicator,
                          pidSection, pesSection, streams);
  else if (streams.contains(tsPacket->pid) && !streams[tsPacket->pid]->completed)
    ParsePES(*tsPacket, pesSection, streams);

  return true;
}
} // namespace

bool CM2TSParser::GetStreamsFromFile(const std::string& path,
                                     unsigned int clip,
                                     const std::string& clipExtension,
                                     StreamMap& streams)
{
  try
  {
    std::string clipFile{URIUtils::AddFileToFolder(
        path, "BDMV", "STREAM",
        fmt::format("{:05}.{}", clip, ::StringUtils::ToLower(clipExtension)))};
    CFile file;
    if (!file.Open(clipFile))
    {
      CLog::LogF(LOGERROR, "Failed to open clip file {}", clipFile);
      return false;
    }

    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Analysing file {} for stream details.", clipFile);

    std::vector<std::byte> buffer;
    buffer.resize(BUFFER_SIZE);
    ssize_t bytesRead = file.Read(buffer.data(), BUFFER_SIZE);
    file.Close();
    if (bytesRead > 0)
      buffer.resize(bytesRead);
    else
      return false;

    // Assemblers
    std::unordered_map<unsigned int, SectionAssembler> pidSection;
    std::unordered_map<unsigned int, PESAssembler> pesSection;

    // Parse M2TS BDAV packets
    std::set<unsigned int> pmtPIDs;
    bool patParsed{false};
    bool pmtParsed{false};
    bool allStreamsComplete{false};
    int offset{0};
    unsigned int packetCount{0};
    const int size{static_cast<int>(buffer.size())};

    while (packetCount < PACKETS_TO_PARSE && offset + BDAV_PACKET_SIZE <= size)
    {
      if (offset + BDAV_PACKET_SIZE <= size)
      {
        CLog::LogFC(LOGDEBUG, LOGBLURAY, "Parsing BDAV packet at offset 0x{}",
                    fmt::format("{:06x}", offset));
        std::span packet{buffer.data() + offset + TIMESTAMP_SIZE, TS_PACKET_SIZE};
        ParseTSPacket(packet, patParsed, pmtParsed, pmtPIDs, pidSection, pesSection, streams);
        packetCount++;
        offset += BDAV_PACKET_SIZE;

        if (pmtParsed)
        {
          allStreamsComplete = std::ranges::all_of(streams,
                                                   [](const auto& stream)
                                                   {
                                                     auto& [_, streamInfo] = stream;
                                                     return streamInfo && streamInfo->completed;
                                                   });
          if (allStreamsComplete)
            break; // All streams completed
        }
      }
    }

    // Deal with Dolby Vision enhancement streams
    if (const auto videoStreamsVector{GetVideoStreams(streams)};
        videoStreamsVector.size() == 2 && videoStreamsVector[1].get().dolbyVision)
    {
      // If two video streams and second one is DV, set DV flag for first video stream
      videoStreamsVector[0].get().dolbyVision = true;

      // Remove the second video stream
      auto it2{std::ranges::find_if(streams, [ptr = &videoStreamsVector[1].get()](const auto& pair)
                                    { return pair.second.get() == ptr; })};
      streams.erase(it2);
    }

    if (!allStreamsComplete)
      CLog::LogFC(
          LOGDEBUG, LOGBLURAY,
          "Not all stream details determined from {} - may need PACKETS_TO_PARSE ({}) increase.",
          clipFile, PACKETS_TO_PARSE);

    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Finished analysing {} for stream details.", clipFile);

    return true;
  }
  catch (const std::out_of_range& e)
  {
    CLog::LogF(LOGERROR, "M2TS parsing failed - error {}", e.what());
    return false;
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "M2TS parsing failed - error {}", e.what());
    return false;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "M2TS parsing failed");
    return false;
  }
}

bool CM2TSParser::GetStreams(const CURL& url,
                             BlurayPlaylistInformation& playlistInformation,
                             StreamMap& streams)
{
  streams.clear();

  // Find longest MT2S in playlist
  const auto& it{std::ranges::max_element(playlistInformation.playItems, {},
                                          [](const PlayItemInformation& item)
                                          { return item.outTime - item.inTime; })};

  if (it == playlistInformation.playItems.end() || it->angleClips.empty())
    return false;

  if (unsigned int clip{it->angleClips.begin()->clip};
      !GetStreamsFromFile(url.GetHostName(), clip, it->angleClips.begin()->codec, streams))
    return false;

  if (!playlistInformation.extensionSubPlayItems.empty() &&
      !playlistInformation.extensionSubPlayItems.begin()->clips.empty())
  {
    // May be a 3D bluray so parse the stereo M2TS file too
    StreamMap extraStreams;
    if (unsigned int stereoClip{
            playlistInformation.extensionSubPlayItems.begin()->clips.begin()->clip};
        !GetStreamsFromFile(url.GetHostName(), stereoClip,
                            playlistInformation.extensionSubPlayItems.begin()->clips.begin()->codec,
                            extraStreams))
    {
      return false;
    }

    if (const auto extraVideoStreamsVector{GetVideoStreams(extraStreams)};
        extraVideoStreamsVector.size() == 1 && extraVideoStreamsVector.front().get().is3d)
    {
      const auto videoStreamsVector{GetVideoStreams(streams)};
      if (!videoStreamsVector.empty())
        videoStreamsVector.front().get().is3d = true;
    }
  }

  return true;
}

std::vector<std::reference_wrapper<TSVideoStreamInfo>> CM2TSParser::GetVideoStreams(
    const StreamMap& streams)
{

  std::vector<std::reference_wrapper<TSVideoStreamInfo>> vec;
  for (const auto& streamInfo : streams | std::views::values)
  {
    if (streamInfo && IsVideoStream(streamInfo->streamType))
      if (auto* videoInfo{dynamic_cast<TSVideoStreamInfo*>(streamInfo.get())})
        vec.emplace_back(*videoInfo);
  }
  std::ranges::sort(vec, {}, &TSVideoStreamInfo::pid);
  return vec;
}

std::vector<std::reference_wrapper<TSAudioStreamInfo>> CM2TSParser::GetAudioStreams(
    const StreamMap& streams)
{

  std::vector<std::reference_wrapper<TSAudioStreamInfo>> vec;
  for (const auto& streamInfo : streams | std::views::values)
  {
    if (streamInfo && IsAudioStream(streamInfo->streamType))
      if (auto* audioInfo{dynamic_cast<TSAudioStreamInfo*>(streamInfo.get())})
        vec.emplace_back(*audioInfo);
  }
  std::ranges::sort(vec, {}, &TSAudioStreamInfo::pid);
  return vec;
}

std::vector<std::reference_wrapper<TSStreamInfo>> CM2TSParser::GetSubtitleStreams(
    const StreamMap& streams)
{

  std::vector<std::reference_wrapper<TSStreamInfo>> vec;
  for (const auto& streamInfo : streams | std::views::values)
  {
    if (streamInfo && IsSubtitleStream(streamInfo->streamType))
      if (auto* subtitleInfo{streamInfo.get()})
        vec.emplace_back(*subtitleInfo);
  }
  std::ranges::sort(vec, {}, &TSStreamInfo::pid);
  return vec;
}
} // namespace XFILE
