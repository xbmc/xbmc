/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "BlurayDirectory.h"

#include "BlurayDiscCache.h"
#include "File.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/BlurayCallback.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/LangCodeExpander.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <ranges>
#include <regex>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <libbluray/bluray-version.h>
#include <libbluray/bluray.h>
#include <libbluray/log_control.h>

using namespace std::chrono_literals;

namespace XFILE
{
namespace // Bit operations
{
// bits 32->1
constexpr unsigned int GetBits(unsigned int value, unsigned int firstBit, unsigned int numBits)
{
  return (value >> (firstBit - numBits)) & ((1 << numBits) - 1);
}

// bits 64->1
constexpr uint64_t GetBits64(uint64_t value, unsigned int firstBit, unsigned int numBits)
{
  return (value >> (firstBit - numBits)) & ((1 << numBits) - 1);
}

constexpr uint64_t GetQWord(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 8)
    throw std::out_of_range("Not enough bytes to extract a QWORD");
  return std::to_integer<uint64_t>(bytes[offset + 7]) |
         std::to_integer<uint64_t>(bytes[offset + 6]) << 8 |
         std::to_integer<uint64_t>(bytes[offset + 5]) << 16 |
         std::to_integer<uint64_t>(bytes[offset + 4]) << 24 |
         std::to_integer<uint64_t>(bytes[offset + 3]) << 32 |
         std::to_integer<uint64_t>(bytes[offset + 2]) << 40 |
         std::to_integer<uint64_t>(bytes[offset + 1]) << 48 |
         std::to_integer<uint64_t>(bytes[offset]) << 56;
}

constexpr uint32_t GetDWord(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 4)
    throw std::out_of_range("Not enough bytes to extract a DWORD");
  return std::to_integer<uint32_t>(bytes[offset + 3]) |
         std::to_integer<uint32_t>(bytes[offset + 2]) << 8 |
         std::to_integer<uint32_t>(bytes[offset + 1]) << 16 |
         std::to_integer<uint32_t>(bytes[offset]) << 24;
}

constexpr uint16_t GetWord(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 2)
    throw std::out_of_range("Not enough bytes to extract a WORD");
  return static_cast<uint16_t>(std::to_integer<uint16_t>(bytes[offset + 1]) |
                               std::to_integer<uint16_t>(bytes[offset]) << 8);
}

constexpr uint8_t GetByte(const std::span<std::byte> bytes, unsigned int offset)
{
  if (bytes.size() < offset + 1)
    throw std::out_of_range("Not enough bytes to extract a BYTE");
  return std::to_integer<uint8_t>(bytes[offset]);
}

std::string GetString(const std::span<std::byte> bytes, unsigned int offset, unsigned int length)
{
  if (bytes.size() < offset + length)
    throw std::out_of_range("Not enough bytes to extract a STRING");
  return std::string{reinterpret_cast<const char*>(bytes.data() + offset), length};
}

class BitReader
{
  const std::byte* data;
  uint32_t size;
  uint32_t offset;
  uint32_t bitPosition;

public:
  BitReader(const std::span<std::byte>& buffer)
    : data(buffer.data()),
      size(static_cast<uint32_t>(buffer.size())),
      offset(0),
      bitPosition(0)
  {
  }

  uint32_t ReadBits(uint32_t n)
  {
    uint32_t value{0};

    while (n > 0 && offset < size)
    {
      uint32_t bitsAvailable{8 - bitPosition};
      uint32_t bitsToRead{(n < bitsAvailable) ? n : bitsAvailable};

      value = (value << bitsToRead) |
              ((std::to_integer<uint32_t>(data[offset]) >> (bitsAvailable - bitsToRead)) &
               ((1u << bitsToRead) - 1));

      bitPosition += bitsToRead;
      n -= bitsToRead;

      if (bitPosition == 8)
      {
        bitPosition = 0;
        offset++;
      }
    }

    return value << n; // Shift remaining bits if we ran out of data
  }

  void SkipBits(uint32_t n)
  {
    uint32_t totalBits{n + bitPosition};
    offset += totalBits >> 3;
    bitPosition = totalBits & 7;
  }

  uint32_t ReadUE()
  {
    int zeros{0};
    while (offset < size && zeros < 32)
    {
      uint32_t bit{(std::to_integer<uint32_t>(data[offset]) >> (7 - bitPosition)) & 1};
      bitPosition++;
      if (bitPosition == 8)
      {
        bitPosition = 0;
        offset++;
      }

      if (bit)
        break;
      zeros++;
    }

    if (zeros == 0)
      return 0;

    return ((1u << zeros) - 1) + ReadBits(zeros);
  }

  void SkipUE() { ReadUE(); }

  int ReadSE()
  {
    uint32_t value{ReadUE()};
    return (value & 1) ? static_cast<int>((value + 1) >> 1) : -static_cast<int>(value >> 1);
  }

  void SkipSE() { ReadSE(); }

  void ByteAlign()
  {
    offset += (bitPosition != 0);
    bitPosition = 0;
  }
};
} // namespace

namespace // BDAV stream parsing
{
// Packet parsing constants
constexpr int BDAV_PACKET_SIZE = 192;
constexpr int TS_PACKET_SIZE = 188;
constexpr unsigned int SYNC_BYTE = 0x47;
constexpr unsigned int TS_HEADER_SIZE = 4;
constexpr unsigned int ADAPTATION_FIELD_MASK = 0x02;
constexpr unsigned int PACKETS_TO_PARSE = 10000;
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
    std::byte{0xF1}, std::byte{0x40}, std::byte{0x00}, std::byte{0xD0}}; //DTS-HD MA + IMAX

inline constexpr std::array DTS_SAMPLE_RATES{8000u,  16000u, 32000u,  64000u,  128000u, 22050u,
                                             44100u, 88200u, 176400u, 352800u, 12000u,  24000u,
                                             48000u, 96000u, 192000u, 384000u};
inline constexpr std::array DTS_CHANNEL_COUNTS{1u, 2u, 2u, 2u, 2u, 3u, 3u, 4u,
                                               4u, 5u, 6u, 6u, 6u, 7u, 8u, 8u};

constexpr unsigned int DTS_HEADER_SIZE = 14;
constexpr unsigned int HEADERS_PARSED_FOR_COMPLETE = 10;

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
enum class VideoCodec : uint8_t
{
  H264,
  H265
};

constexpr unsigned int H264_PROFILE_HIGH = 100;
constexpr unsigned int H264_PROFILE_MAIN = 77;

inline constexpr std::array NAL_START_CODE_3 = {std::byte{0}, std::byte{0}, std::byte{1}};

constexpr unsigned int H264_NAL_SPS = 7;
constexpr unsigned int H265_NAL_SPS = 33;
constexpr unsigned int H265_NAL_SEI_PREFIX = 39;
constexpr unsigned int H265_NAL_SEI_SUFFIX = 40;
constexpr unsigned int DOLBY_VISION_RPU = 62;
constexpr unsigned int DOLBY_VISION_EL = 63;
constexpr unsigned int DOLBY_VISION_RPU_HEADER = 0x7C01;
constexpr unsigned int DOLBY_VISION_EL_HEADER = 0x7E01;

constexpr unsigned int H265_EXTENDED_SAR = 255;
inline constexpr std::array ASPECT_RATIOS{
    0.0,         1.0,          12.0 / 11.0, 10.0 / 11.0, 16.0 / 11.0, 40.0 / 33.0,
    24.0 / 11.0, 20.0 / 11.0,  32.0 / 11.0, 80.0 / 33.0, 18.0 / 11.0, 15.0 / 11.0,
    64.0 / 33.0, 160.0 / 99.0, 4.0 / 3.0,   3.0 / 2.0,   2.0 / 1.0};

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
inline constexpr std::array VC1_ASPECT_RATIOS{0.0,         1.0,          12.0 / 11.0, 10.0 / 11.0,
                                              16.0 / 11.0, 40.0 / 33.0,  24.0 / 11.0, 20.0 / 11.0,
                                              32.0 / 11.0, 80.0 / 33.0,  18.0 / 11.0, 15.0 / 11.0,
                                              64.0 / 33.0, 160.0 / 99.0, 0.0,         0.0};

// MPEG-2
inline constexpr std::array MPEG2_SEQUENCE_HEADER_START_CODE = {std::byte{0x00}, std::byte{0x00},
                                                                std::byte{0x01}, std::byte{0xB3}};

inline constexpr std::array MPEG2_DISPLAY_ASPECT_RATIOS{0.0, 1.0, 3.0 / 4.0, 9.0 / 16.0,
                                                        1.0 / 2.21};       

// Stream structures

struct Descriptor
{
  unsigned int tag;
  int length;
  std::vector<std::byte> data;
};

struct TSStreamInfo
{
  unsigned int pid{};
  ENCODING_TYPE streamType{};
  std::vector<Descriptor> descriptors{};

  // Determine if details complete
  unsigned int seen{0};
  bool completed{false};

  // Methods
  TSStreamInfo() = default;
  virtual ~TSStreamInfo() = default;
  TSStreamInfo(const TSStreamInfo&) = default;
  TSStreamInfo& operator=(const TSStreamInfo&) = default;
  TSStreamInfo(TSStreamInfo&&) noexcept = default;
  TSStreamInfo& operator=(TSStreamInfo&&) noexcept = default;
};

struct TSAudioStreamInfo : TSStreamInfo
{
  unsigned int channels{0};
  unsigned int sampleRate{0};

  // DTS
  bool isXLL{false}; // DTS-HD MA - needs to be true for DTS:X
  bool hasSubstream{false}; // Needs to be true for DTS:X
  bool isXLLX{false};
  bool isXLLXIMAX{false};

  // AC3 / Dolby
  bool hasDependantStream{false};
  bool isAtmos{false};
};

struct TSVideoStreamInfo : TSStreamInfo
{
  unsigned int height{0};
  unsigned int width{0};
  unsigned int bitDepth{0};
  double aspectRatio{0.0};

  bool hdr10{false};
  bool hdr10Plus{false};
  bool dolbyVision{false};
  bool isEnhancementLayer{false};
};

std::set<unsigned int> pmtPIDs;
std::unordered_map<unsigned int, std::unique_ptr<TSStreamInfo>> streams;

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

      const unsigned int availableLength{static_cast<unsigned int>(payload.size()) - offset};

      // Add section part
      if (availableLength >= needed)
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

// Assemblers
std::unordered_map<unsigned int, SectionAssembler> pidSection;
std::unordered_map<unsigned int, PESAssembler> pesSection;

// Helpers

bool IsVideoStream(ENCODING_TYPE streamType)
{
  using enum ENCODING_TYPE;
  return streamType == VIDEO_HEVC || streamType == VIDEO_H264 || streamType == VIDEO_MPEG2 ||
         streamType == VIDEO_VC1;
}

bool IsAudioStream(ENCODING_TYPE streamType)
{
  using enum ENCODING_TYPE;
  return streamType == AUDIO_LPCM || streamType == AUDIO_AC3 || streamType == AUDIO_DTS ||
         streamType == AUDIO_TRUHD || streamType == AUDIO_AC3PLUS || streamType == AUDIO_DTSHD ||
         streamType == AUDIO_DTSHD_MASTER || streamType == AUDIO_AC3PLUS_SECONDARY ||
         streamType == AUDIO_DTSHD_SECONDARY;
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

  const auto tableId{static_cast<TABLE_IDS>(GetByte(packet, offset))};
  if (tableId != wantedTableId)
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

bool ParsePAT(const std::span<std::byte>& PAT, bool payloadUnitStartIndicator)
{
  CLog::LogF(LOGDEBUG, "Parsing PAT");

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
      pidSection.emplace(pmPID, SectionAssembler{}); // Prepare assembler
      CLog::LogF(LOGDEBUG, "Found pmPID {} for program {}", fmt::format("{:04x}", pmPID),
                 programNumber);
    }
  }

  return true;
}

bool ParsePMT(unsigned int pid, const std::span<std::byte>& PMT, bool payloadUnitStartIndicator)
{
  CLog::LogF(LOGDEBUG, "Parsing PMT - PID 0x{}", fmt::format("{:04x}", pid));

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
    {
      const auto streamType{static_cast<ENCODING_TYPE>(GetByte(section, offset + i))};
      const unsigned int elementaryPID{GetWord(section, offset + i + 1) & PID_MASK};
      const int esInfoLength{static_cast<int>(GetWord(section, offset + i + 3) & LENGTH_MASK)};
      const std::string streamTypeName{GetStreamTypeName(streamType)};

      if (!streams.contains(elementaryPID))
      {
        // Parse descriptors
        int descriptorOffset{offset + i + 5};
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

          descriptors.emplace_back(Descriptor{
              .tag = desc_tag,
              .length = desc_length,
              .data = std::vector(section.begin() + descriptorOffset + 2,
                                  section.begin() + descriptorOffset + 2 + desc_length)});

          // ISO 639 language descriptor
          if (desc_tag == 0x0A && desc_length >= 4)
            language = GetString(section, descriptorOffset + 2, 3);

          descriptorOffset += 2 + desc_length;
        }

        if (IsVideoStream(streamType))
          streams[elementaryPID] = std::make_unique<TSVideoStreamInfo>();
        else if (IsAudioStream(streamType))
          streams[elementaryPID] = std::make_unique<TSAudioStreamInfo>();
        else
        {
          streams[elementaryPID] = std::make_unique<TSStreamInfo>();
          streams[elementaryPID]->completed = true; // No further parsing
        }

        streams[elementaryPID]->pid = elementaryPID;
        streams[elementaryPID]->streamType = streamType;
        streams[elementaryPID]->descriptors = std::move(descriptors);

        pesSection.emplace(elementaryPID, PESAssembler{}); // Prepare assembler

        CLog::Log(LOGDEBUG, "Found stream at offset 0x{} - type: {} (0x{}), pid: 0x{}, lang {}",
                  fmt::format("{:06x}", offset + i), streamTypeName,
                  fmt::format("{:02x}", static_cast<int>(streamType)),
                  fmt::format("{:04x}", elementaryPID), language);
      }

      i += ELEMENTARY_STREAM_HEADER_SIZE + static_cast<int>(esInfoLength);
    }
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
  constexpr auto sampleRates = AC3_SAMPLE_RATES;
  if (fscod < sampleRates.size())
    streamInfo->sampleRate = sampleRates[fscod];

  // Set channel count
  constexpr auto channelCounts = AC3_CHANNEL_COUNTS;
  if (acmod < channelCounts.size())
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

  const bool lfeon{GetBits(header, 13 - bits, 1) == 1};
  if (lfeon)
    streamInfo->channels++;

  streamInfo->completed = (streamInfo->seen++) == HEADERS_PARSED_FOR_COMPLETE;

  return true;
}

bool ParseEAC3Bitstream(const std::span<std::byte>& buffer,
                        int offset,
                        TSAudioStreamInfo* streamInfo)
{
  BitReader br(buffer.subspan(2));

  // Parse bsi
  const unsigned int strmtyp{br.ReadBits(2)};
  br.SkipBits(14); // substreamid, frmsiz
  const unsigned int fscod{br.ReadBits(2)};
  const unsigned int fscod2_numblkscod{br.ReadBits(2)};
  const unsigned int acmod{br.ReadBits(3)};
  const bool lfeon{br.ReadBits(1) == 1};
  unsigned int bsid{br.ReadBits(5)};
  if (bsid <= 10)
    return false; // Not EAC-3 stream

  br.SkipBits(5); // dialnorm

  // Find potential position of channel map
  if (br.ReadBits(1) == 1) //  compre
    br.SkipBits(8); // compr
  if (acmod == 0)
  {
    br.SkipBits(5);
    if (br.ReadBits(1) == 1) //  compr2e
      br.SkipBits(8); // compr2
  }

  // See if channel map present for dependant streams
  bool chanmape{false};
  uint32_t chanmap{0};
  if (strmtyp == 1)
    if (chanmape = br.ReadBits(1) == 1; chanmape)
      chanmap = br.ReadBits(16);

  // Set sample rate
  constexpr auto sampleRates = AC3_SAMPLE_RATES;
  if (fscod < 3)
    streamInfo->sampleRate = sampleRates[fscod];
  else if (fscod == 3 && fscod2_numblkscod < 3)
    streamInfo->sampleRate = sampleRates[fscod2_numblkscod] / 2;

  // Set channel count
  unsigned int channelCount{0};
  if (!chanmape)
  {
    // If no channel map, then use the AC3 way
    constexpr auto channelCounts = AC3_CHANNEL_COUNTS;
    if (acmod < channelCounts.size())
      channelCount = channelCounts[acmod] + (lfeon ? 1 : 0);
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

  if (strmtyp == 0)
    streamInfo->channels = channelCount;
  else if (strmtyp == 1)
  {
    streamInfo->channels += channelCount;
    streamInfo->hasDependantStream = true;
  }

  // Continue to parse for JOC
  // Mixing metadata exists
  if (br.ReadBits(1)) // mixmdate
  {
    if (acmod > 2)
      br.SkipBits(2); // dmixmod
    if ((acmod & 1) && (acmod > 2))
      br.SkipBits(6); // ltrctmixlev, lorocmixlev
    if (acmod & 4)
      br.SkipBits(6); // ltrtsurmixlev, lortsurmixlev
    if (lfeon)
    {
      if (br.ReadBits(1) == 1) // lfemixlevcode
        br.SkipBits(5); // lfemixlevcod
    }
    if (strmtyp == 0)
    {
      if (br.ReadBits(1) == 1) // pgmscle
        br.SkipBits(6); // pgmscl
      if (acmod == 0)
        if (br.ReadBits(1) == 1) // pgmscl2e
          br.SkipBits(6); // pgmscl2
      if (br.ReadBits(1) == 1) // extpgmscle
        br.SkipBits(6); // extpgmscl

      const unsigned int mixdef{br.ReadBits(2)};
      switch (mixdef)
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
      if (acmod < 2)
      {
        if (br.ReadBits(1) == 1) // paninfoe
          br.SkipBits(14); // panmean, paninfo
        if (acmod == 0)
          if (br.ReadBits(1) == 1) // paninfo2e
            br.SkipBits(14); // panmean2, paninfo2
      }
      if (br.ReadBits(1) == 1) // frmmixcfginfoe
      {
        if (fscod2_numblkscod == 0)
          br.SkipBits(5); // blkmixcfginfo[0]
        else
        {
          unsigned int number_of_blocks_per_syncframe{
              NUM_AUDIO_BLOCK_PER_SYNCFRAME[fscod2_numblkscod]};
          for (unsigned int blk = 0; blk < number_of_blocks_per_syncframe; blk++)
          {
            if (br.ReadBits(1) == 1) // blkmixcfginfoe
              br.SkipBits(14); // blkmixcfginfo[blk]
          }
        }
      }
    }
  }

  // Info metadata exists
  if (br.ReadBits(1)) // infomdate
  {
    br.SkipBits(3); // bsmod
    br.SkipBits(1); // copyrightb
    br.SkipBits(1); // origbs
    if (acmod == 2)
      br.SkipBits(4); // dsurmod, dheadphonmod
    if (acmod >= 6)
      br.SkipBits(2); // dsurexmod
    if (br.ReadBits(1)) // audioprodie
    {
      br.SkipBits(5); // mixlevel
      br.SkipBits(2); // roomtyp
      br.SkipBits(1); // adconvtyp
    }
    if (acmod == 0)
    {
      if (br.ReadBits(1)) // audprodi2e
      {
        br.SkipBits(5); // mixlevel2
        br.SkipBits(2); // roomtyp2
        br.SkipBits(1); // adconvtyp2
      }
    }
    if (fscod < 3)
      br.SkipBits(1); // sourcefscod
  }

  if (strmtyp == 0 && fscod2_numblkscod != 3)
    br.SkipBits(1); // convsync

  if (strmtyp == 2)
    if (fscod2_numblkscod == 3 || br.ReadBits(1)) // blkid
      br.SkipBits(6); // frmsizecod

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
        return true;
      }
    }
  }

  streamInfo->completed = (streamInfo->seen++) == HEADERS_PARSED_FOR_COMPLETE;

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
    return ParseEAC3Bitstream(buffer, offset, streamInfo);
  if (bsid == 6 or bsid == 8)
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
      dtsFrame.syncWord = static_cast<DTSSyncWords>(sync);
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
  const auto result{std::ranges::search(searchRange, syncWord)};

  if (!result.empty())
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

    // Set parameters
    constexpr auto sampleRates = DTS_SAMPLE_RATES;
    constexpr auto channelCounts = DTS_CHANNEL_COUNTS;

    if (sfreq < sampleRates.size())
      streamInfo->sampleRate = sampleRates[sfreq];

    if (amode < channelCounts.size())
      streamInfo->channels = channelCounts[amode];

    // See if there is a DTS substream header in this block
    if (auto substream{
            FindDTSSyncWord(buffer, dtsData->syncPos + DTS_HEADER_SIZE, DTS_SYNCWORD_SUBSTREAM)};
        !substream)
      return true;
    else
      offset = *substream;
  }

  if (dtsData->syncWord == DTSSyncWords::SUBSTREAM || offset > 0)
  {
    // Substream header found
    streamInfo->hasSubstream = true;
    if (auto xll{FindDTSSyncWord(buffer, dtsData->syncPos + 10, DTS_SYNCWORD_XLL)}; xll)
      streamInfo->isXLL = true;

    if (auto xllx{FindDTSSyncWord(buffer, dtsData->syncPos + 10, DTS_SYNCWORD_XLL_X)}; xllx)
      streamInfo->isXLLX = true;

    if (auto xllximax{FindDTSSyncWord(buffer, dtsData->syncPos + 10, DTS_SYNCWORD_XLL_X_IMAX)};
        xllximax)
      streamInfo->isXLLXIMAX = true;
  }

  streamInfo->completed = (streamInfo->seen++) == HEADERS_PARSED_FOR_COMPLETE;

  return true;
}

bool ParseTrueHDHeader(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  unsigned int format_info{GetDWord(buffer, 4)};
  unsigned int flags{GetWord(buffer, 10)};

  // Set sample rate
  unsigned int audio_sampling_frequency{GetBits(format_info, 32, 4)};
  constexpr auto sampleRates = TRUEHD_SAMPLE_RATES;
  if (audio_sampling_frequency < sampleRates.size())
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
  bool extra_channel_meaning_present{(GetBits64(channel_meaning, 1, 1) == 1)};
  if (extra_channel_meaning_present && ch16_present)
  {
    unsigned int extra{GetDWord(buffer, 26)};
    unsigned int ch16_channel_count{GetBits(extra, 17, 5)};
    streamInfo->channels = ch16_channel_count + 1;
  }

  streamInfo->isAtmos = ch16_present && substreams == 4;

  streamInfo->completed = (streamInfo->seen++) == HEADERS_PARSED_FOR_COMPLETE;

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
    if (result.empty() || static_cast<unsigned int>(data.end() - result.begin()) < TRUEHD_MINIMUM_HEADER_SIZE)
      break;

    offset += std::distance(data.begin(), result.begin());

    // Check signature
    if (GetWord(buffer, offset + 8) != TRUEHD_HEADER_SIGNATURE)
      break;

    const unsigned int flag{GetByte(buffer, offset + 3)};
    if (flag == DOLBY_FLAG)
      ParseTrueHDHeader(buffer.subspan(offset), streamInfo);

    offset += TRUEHD_MINIMUM_HEADER_SIZE;
  }

  return true;
}

bool ParseLPCMBitstream(const std::span<std::byte>& buffer, TSAudioStreamInfo* streamInfo)
{
  // Sub-stream ID (should be 0xA0-0xAF for LPCM)
  unsigned int sub_stream_id{GetByte(buffer, 1)};
  if ((sub_stream_id & 0xA0) != 0xA0)
    return false;

  unsigned int header{GetByte(buffer, 2)};
  unsigned int channel_info{GetBits(header, 8, 4)};
  unsigned int sample_rate_code{GetBits(header, 4, 4)};

  if (channel_info < LPCM_CHANNEL_COUNTS.size())
    streamInfo->channels = LPCM_CHANNEL_COUNTS[channel_info];
  if (LPCM_SAMPLE_RATES.contains(sample_rate_code))
    streamInfo->sampleRate = LPCM_SAMPLE_RATES.find(sample_rate_code)->second;

  streamInfo->completed = (streamInfo->seen++) == HEADERS_PARSED_FOR_COMPLETE;

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
  if (idx != 0)
  {
    if (br.ReadBits(1) == 1) // inter_ref_pic_set_prediction_flag
    {
      if (idx == numSets)
        br.SkipUE(); // delta_idx_minus1
      br.SkipBits(1); // delta_rps_sign
      br.SkipUE(); // abs_delta_rps_minus1
      return;
    }
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

double ParseVUI(BitReader& br)
{
  double ar{0.0};

  if (br.ReadBits(1) == 1) // aspect_ratio_info_present_flag
  {
    unsigned int aspectRatioIdc{br.ReadBits(8)};
    if (aspectRatioIdc == H265_EXTENDED_SAR)
    {
      unsigned int width{br.ReadBits(16)};
      unsigned int height{br.ReadBits(16)};
      ar = static_cast<double>(height) / width;
    }
    else if (aspectRatioIdc < ASPECT_RATIOS.size())
      ar = ASPECT_RATIOS[aspectRatioIdc];
  }

  return ar;
}

bool ParseH265SPS(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  CLog::LogF(LOGDEBUG, "Parsing H265 SPS");

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

  if (br.ReadBits(1) == 1) // scaling_list_enabled_flag
    if (br.ReadBits(1) == 1) // sps_scaling_list_data_present_flag
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
  CLog::LogF(LOGDEBUG, "Parsing H264 SPS");

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
  unsigned int pic_order_cnt_type{br.ReadUE()};

  if (pic_order_cnt_type == 0)
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
  streamInfo->height = (2 - frame_mbs_only_flag) * pic_height_in_map_units * 16 -
                       (frame_crop_top_offset + frame_crop_bottom_offset) * 2;

  if (br.ReadBits(1) == 1) // vui_parameters_present_flag
    streamInfo->aspectRatio = ParseVUI(br);

  streamInfo->completed = true;

  return true;
}

bool ParseITUT35UserData(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  unsigned int length{static_cast<unsigned int>(buffer.size())};
  if (length < 3)
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

bool ParseH265SEI(const std::span<std::byte>& buffer, TSVideoStreamInfo* streamInfo)
{
  unsigned int length{static_cast<unsigned int>(buffer.size())};
  if (length < 2)
    return false;

  unsigned int offset{0};
  while (offset < length - 2)
  {
    // Read payload type
    unsigned int payloadType{0};
    while (offset < length && GetByte(buffer, offset) == 0xFF)
    {
      payloadType += 255;
      offset++;
    }
    if (offset >= length)
      break;
    payloadType += GetByte(buffer, offset);
    offset++;

    // Read payload size
    unsigned int payloadSize{0};
    while (offset < length && GetByte(buffer, offset) == 0xFF)
    {
      payloadSize += 255;
      offset++;
    }
    if (offset >= length)
      break;
    payloadSize += GetByte(buffer, offset);
    offset++;

    CLog::LogF(LOGDEBUG, "Parsing SEI - payload type {}", payloadType);

    // Check for HDR SEI messages
    if (payloadType == SEI_PAYLOAD_MASTERING_DISPLAY_COLOUR_VOLUME ||
        payloadType == SEI_PAYLOAD_CONTENT_LIGHT_LEVEL_INFO)
      streamInfo->hdr10 = true;
    else if (payloadType == SEI_PAYLOAD_UNREGISTERED)
    {
      // User data unregistered - check for Dolby Vision
      if (offset + 16 <= length)
      {
        auto uuid{buffer.subspan(offset, 16)};
        if (std::ranges::equal(uuid, DOLBY_VISION_PROFILE_7_UUID))
          streamInfo->dolbyVision = true;
      }
    }
    else if (payloadType == SEI_PAYLOAD_REGISTERED_ITU_T_T35)
      ParseITUT35UserData(buffer.subspan(offset), streamInfo);

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
  switch (videoCodec)
  {
    using enum VideoCodec;
    case H265:
      unit.push_back(buffer[1]);
      offset += 1;
      break;
    case H264:
    default:
      break;
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
    const size_t relativeOffset =
        static_cast<unsigned int>(std::distance(data.begin(), result.begin())) + 3;
    const auto end{std::ranges::search(data.subspan(relativeOffset), NAL_START_CODE_3)};
    const size_t length{end.empty() ? data.size() - relativeOffset
                                    : end.data() - data.data() - relativeOffset};

    std::vector<std::byte> unit{
        RemoveEmulationPreventionBytes(data.subspan(relativeOffset, length), videoCodec)};

    // Extract NAL unit type from header
    unsigned int nal_unit_type{0};
    unsigned int header{0};
    switch (videoCodec)
    {
      using enum VideoCodec;
      case H264:
      {
        header = GetByte(unit, 0);
        nal_unit_type = GetBits(header, 5, 5);
        break;
      }
      case H265:
      {
        header = GetWord(unit, 0);
        nal_unit_type = GetBits(header, 15, 6);
        unsigned int nuh_layer_id{GetBits(header, 9, 6)};
        if (nuh_layer_id > 0)
          streamInfo->isEnhancementLayer = true;
        break;
      }
      default:
        break;
    }

    CLog::LogF(LOGDEBUG, "Parsing NAL - type {}", nal_unit_type);

    switch (nal_unit_type)
    {
      case H265_NAL_SPS:
        ParseH265SPS(std::span(unit).subspan(2), streamInfo);
        break;
      case H264_NAL_SPS:
        ParseH264SPS(std::span(unit).subspan(1), streamInfo);
        break;
      case H265_NAL_SEI_PREFIX:
      case H265_NAL_SEI_SUFFIX:
        ParseH265SEI(std::span(unit).subspan(2), streamInfo);
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

  CLog::LogF(LOGDEBUG, "Parsing VC1 Sequence Header");

  unsigned int offset{static_cast<unsigned int>(result.begin() - buffer.begin()) + 4};

  unsigned int header{GetWord(buffer, offset)};
  unsigned int profile{GetBits(header, 16, 2)};
  if (profile != 3)
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
        streamInfo->aspectRatio = static_cast<double>(aspect_horiz_size) / aspect_vert_size;
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
  auto result{std::ranges::search(buffer, MPEG2_SEQUENCE_HEADER_START_CODE)};
  if (result.empty() || buffer.end() - result.begin() < 8)
    return false;

  CLog::LogF(LOGDEBUG, "Parsing MPEG2 Sequence Header");

  unsigned int header{GetDWord(buffer, 4)};
  unsigned int horizontal_size_value{GetBits(header, 32, 12)};
  unsigned int vertical_size_value{GetBits(header, 20, 12)};
  streamInfo->width = horizontal_size_value;
  streamInfo->height = vertical_size_value;

  unsigned int aspect_ratio_information{GetBits(header, 8, 4)};
  if (aspect_ratio_information < 2)
    streamInfo->aspectRatio = MPEG2_DISPLAY_ASPECT_RATIOS[aspect_ratio_information];
  else if (aspect_ratio_information < MPEG2_DISPLAY_ASPECT_RATIOS.size())
    streamInfo->aspectRatio = MPEG2_DISPLAY_ASPECT_RATIOS[aspect_ratio_information] *
                              static_cast<double>(horizontal_size_value) / vertical_size_value;

  streamInfo->bitDepth = 8;

  streamInfo->completed = true;

  return true;
}

bool ParsePES(const TSPacket& tsPacket)
{
  CLog::LogF(LOGDEBUG, "Parsing PES - PID 0x{}", fmt::format("{:04x}", tsPacket.pid));

  auto& pesAssembler{pesSection[tsPacket.pid]};
  for (auto& section : pesAssembler.push(tsPacket))
  {
    // Parse PES packet
    auto pesPacket{ParsePESPacket(section)};
    if (!pesPacket)
      continue; // May not contain audio

    auto& streamInfo{streams[tsPacket.pid]};
    CLog::LogF(LOGDEBUG, "Parsing PES - Stream PID 0x{} - Type 0x{}",
               fmt::format("{:04x}", streamInfo->pid),
               fmt::format("{:02x}", static_cast<int>(streamInfo->streamType)));

    switch (streamInfo->streamType)
    {
      using enum ENCODING_TYPE;
      case VIDEO_HEVC:
        ParseNAL(pesPacket->data, VideoCodec::H265,
                 dynamic_cast<TSVideoStreamInfo*>(streamInfo.get()));
        break;
      case VIDEO_H264:
        ParseNAL(pesPacket->data, VideoCodec::H264,
                 dynamic_cast<TSVideoStreamInfo*>(streamInfo.get()));
        break;
      case VIDEO_VC1:
        ParseVC1(pesPacket->data, dynamic_cast<TSVideoStreamInfo*>(streamInfo.get()));
        break;
      case VIDEO_MPEG2:
        ParseMPEG2(pesPacket->data, dynamic_cast<TSVideoStreamInfo*>(streamInfo.get()));
        break;
      case AUDIO_AC3:
      case AUDIO_AC3PLUS:
      case AUDIO_AC3PLUS_SECONDARY:
        ParseAC3Bitstream(pesPacket->data, dynamic_cast<TSAudioStreamInfo*>(streamInfo.get()));
        break;
      case AUDIO_DTS:
      case AUDIO_DTSHD:
      case AUDIO_DTSHD_MASTER:
      case AUDIO_DTSHD_SECONDARY:
        ParseDTSBitstream(pesPacket->data, dynamic_cast<TSAudioStreamInfo*>(streamInfo.get()));
        break;
      case AUDIO_TRUHD:
        ParseTrueHDBitstream(pesPacket->data, dynamic_cast<TSAudioStreamInfo*>(streamInfo.get()));
        break;
      case AUDIO_LPCM:
        if (pesPacket->streamId == 0xBD)
          ParseLPCMBitstream(pesPacket->data, dynamic_cast<TSAudioStreamInfo*>(streamInfo.get()));
        break;
      case SUB_PG:
      case SUB_IG:
      case SUB_TEXT:
      default:
        streamInfo->completed = true;
        break;
    }
  }

  return true;
}

bool ParseTSPacket(const std::span<std::byte>& packet, bool& patParsed, bool& pmtParsed)
{
  // Parse TS header
  auto tsPacket{ParseTSPacket(packet)};
  if (!tsPacket)
    return false;

  if (tsPacket->pid == 0x00)
    patParsed = ParsePAT(tsPacket->payload, tsPacket->payloadUnitStartIndicator);
  else if (pmtPIDs.contains(tsPacket->pid))
    pmtParsed |= ParsePMT(tsPacket->pid, tsPacket->payload, tsPacket->payloadUnitStartIndicator);
  else if (streams.contains(tsPacket->pid) && !streams[tsPacket->pid]->completed)
    ParsePES(*tsPacket);

  return true;
}

bool GetStreamDetails(const CURL& url, PlaylistInformation& playlistInformation)
{
  // Find longest MT2S in playlist
  const auto& it{std::ranges::max_element(playlistInformation.playItems, {},
                                          [](const PlayItemInformation& item)
                                          { return item.outTime - item.inTime; })};

  const std::string& path{url.GetHostName()};
  unsigned int clip{it->angleClips.begin()->clip};
  const std::string clipFile{
      URIUtils::AddFileToFolder(path, "BDMV", "STREAM", StringUtils::Format("{:05}.m2ts", clip))};
  CFile file;
  if (!file.Open(clipFile))
    return false;

  CLog::Log(LOGDEBUG, "Analysing file {} for stream details.", clipFile);

  std::vector<std::byte> buffer;
  buffer.resize(BUFFER_SIZE);
  file.Read(buffer.data(), BUFFER_SIZE);
  file.Close();

  bool patParsed{false};
  bool pmtParsed{false};
  int offset{0};
  unsigned int packetCount{0};
  const int size{static_cast<int>(buffer.size())};
  while (packetCount < PACKETS_TO_PARSE && offset + BDAV_PACKET_SIZE <= size)
  {
    if (offset + BDAV_PACKET_SIZE <= size)
    {
      CLog::LogF(LOGDEBUG, "Parsing BDAV packet at offset 0x{}", fmt::format("{:06x}", offset));
      std::span packet{buffer.data() + offset + TIMESTAMP_SIZE, TS_PACKET_SIZE};
      ParseTSPacket(packet, patParsed, pmtParsed);
      packetCount++;
      offset += BDAV_PACKET_SIZE;

      if (pmtParsed && std::ranges::all_of(streams,
                                           [](const auto& stream)
                                           {
                                             auto& [_, streamInfo] = stream;
                                             return streamInfo && streamInfo->completed;
                                           }))
        break; // All streams completed
    }
  }

  // Deal with Dolby Vision enhancement streams
  auto videoStreams =
      streams | std::views::values |
      std::views::filter([](const auto& streamInfo)
                         { return streamInfo && IsVideoStream(streamInfo->streamType); }) |
      std::views::transform([](const auto& streamInfo)
                            { return static_cast<TSVideoStreamInfo*>(streamInfo.get()); });

  if (std::ranges::distance(videoStreams) == 2 &&
      std::ranges::any_of(videoStreams, std::identity{}, &TSVideoStreamInfo::dolbyVision))
  {
    // If two video streams and one is DV, set DV flag for all video streams
    std::ranges::for_each(videoStreams, [](auto* stream) { stream->dolbyVision = true; });
  }

  CLog::Log(LOGDEBUG, "Finished analysing.");

  // Update StreamInfo

  return true;
}
} // namespace

namespace // Bluray parsing
{
void AddOptionsAndSort(const CURL& url, CFileItemList& items, bool blurayMenuSupport)
{
  // Add all titles and menu options
  CDiscDirectoryHelper::AddRootOptions(
      url, items, blurayMenuSupport ? AddMenuOption::ADD_MENU : AddMenuOption::NO_MENU);

  items.AddSortMethod(SortByTrackNumber, 554,
                      LABEL_MASKS("%L", "%D", "%L", "")); // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize, 553,
                      LABEL_MASKS("%L", "%I", "%L", "%I")); // FileName, Size | Foldername, Size
}

constexpr unsigned int OFFSET_STREAM_TYPE = 1;
constexpr unsigned int OFFSET_PLAYITEM_PACKET_IDENTIFIER = 2;
constexpr unsigned int OFFSET_SUBPATH_SUBPATH_ID = 2;
constexpr unsigned int OFFSET_SUBPATH_SUBCLIP_ID = 3;
constexpr unsigned int OFFSET_SUBPATH_PACKET_IDENTIFIER = 4;
constexpr unsigned int OFFSET_DV_PIP_SUBPATH_ID = 2;
constexpr unsigned int OFFSET_DV_PIP_PACKET_IDENTIFIER = 3;
constexpr unsigned int OFFSET_STREAM_CODING = 1;
constexpr unsigned int OFFSET_STREAM_VIDEO_FLAG_1 = 2;
constexpr unsigned int OFFSET_STREAM_VIDEO_FLAG_2 = 3;
constexpr unsigned int OFFSET_STREAM_VIDEO_FLAG_3 = 4;
constexpr unsigned int OFFSET_STREAM_AUDIO_FLAG_1 = 2;
constexpr unsigned int OFFSET_STREAM_AUDIO_LANGUAGE = 3;
constexpr unsigned int OFFSET_STREAM_SUB_LANGUAGE = 2;
constexpr unsigned int OFFSET_STREAM_TEXT_ENCODING = 2;
constexpr unsigned int OFFSET_STREAM_TEXT_LANGUAGE = 3;
constexpr unsigned int OFFSET_STREAM_SECONDARY_REFERENCES = 2;

StreamInformation ParseStream(std::vector<std::byte>& buffer,
                              unsigned int& offset,
                              STREAM_TYPE streamType)
{
  StreamInformation streamInformation;
  unsigned int length{GetByte(buffer, offset)};

  const BLURAY_STREAM_TYPE type{GetByte(buffer, offset + OFFSET_STREAM_TYPE)};
  streamInformation.type = type;
  switch (type)
  {
    using enum BLURAY_STREAM_TYPE;
    case PLAYITEM: // a stream of the clip used by the PlayItem
    {
      streamInformation.packetIdentifier =
          GetWord(buffer, offset + OFFSET_PLAYITEM_PACKET_IDENTIFIER);
      break;
    }
    case SUBPATH: // a stream of the clip used by a SubPath (types 2, 3, 4, 5, 6, 8 or 9)
    {
      streamInformation.subpathId = GetWord(buffer, offset + OFFSET_SUBPATH_SUBPATH_ID);
      streamInformation.subclipId = GetWord(buffer, offset + OFFSET_SUBPATH_SUBCLIP_ID);
      streamInformation.packetIdentifier =
          GetWord(buffer, offset + OFFSET_SUBPATH_PACKET_IDENTIFIER);
      break;
    }
    case SUBPATH_INMUX_SYNCHRONOUS_PIP: // a stream of the clip used by a SubPath (type 7)
    case SUBPATH_DOLBY_VISION_LAYER: // a stream of the clip used by a SubPath (type 10)
    {
      streamInformation.subpathId = GetWord(buffer, offset + OFFSET_DV_PIP_SUBPATH_ID);
      streamInformation.packetIdentifier =
          GetWord(buffer, offset + OFFSET_DV_PIP_PACKET_IDENTIFIER);
      break;
    }
  }
  offset += length + 1;

  length = GetByte(buffer, offset);
  const ENCODING_TYPE coding{GetByte(buffer, offset + OFFSET_STREAM_CODING)};
  streamInformation.coding = coding;
  switch (coding)
  {
    using enum ENCODING_TYPE;
    case VIDEO_MPEG2:
    case VIDEO_H264:
    case VIDEO_HEVC:
    case VIDEO_VC1:
    {
      unsigned int flag{GetByte(buffer, offset + OFFSET_STREAM_VIDEO_FLAG_1)};
      streamInformation.format = GetBits(flag, 8, 4);
      streamInformation.rate = GetBits(flag, 4, 4);
      if (coding == VIDEO_HEVC)
      {
        flag = GetByte(buffer, offset + OFFSET_STREAM_VIDEO_FLAG_2);
        streamInformation.dynamicRangeType = GetBits(flag, 8, 4);
        streamInformation.colorSpace = GetBits(flag, 4, 4);
        flag = GetByte(buffer, offset + OFFSET_STREAM_VIDEO_FLAG_3);
        streamInformation.copyRestricted = GetBits(flag, 8, 1);
        streamInformation.HDRPlus = GetBits(flag, 7, 1);
      }
      break;
    }
    case AUDIO_LPCM:
    case AUDIO_AC3:
    case AUDIO_DTS:
    case AUDIO_TRUHD:
    case AUDIO_AC3PLUS:
    case AUDIO_DTSHD:
    case AUDIO_DTSHD_MASTER:
    case AUDIO_AC3PLUS_SECONDARY:
    case AUDIO_DTSHD_SECONDARY:
    {
      const unsigned int flag{GetByte(buffer, offset + OFFSET_STREAM_AUDIO_FLAG_1)};
      streamInformation.format = GetBits(flag, 8, 4);
      streamInformation.rate = GetBits(flag, 4, 4);
      streamInformation.language = GetString(buffer, offset + OFFSET_STREAM_AUDIO_LANGUAGE, 3);
      break;
    }
    case SUB_PG:
    case SUB_IG:
    {
      streamInformation.language = GetString(buffer, offset + OFFSET_STREAM_SUB_LANGUAGE, 3);
      break;
    }
    case SUB_TEXT:
    {
      streamInformation.characterEncoding = GetByte(buffer, offset + OFFSET_STREAM_TEXT_ENCODING);
      streamInformation.language = GetString(buffer, offset + OFFSET_STREAM_TEXT_LANGUAGE, 3);
      break;
    }
    default:
      break;
  }
  offset += length + 1;

  switch (streamType)
  {
    using enum STREAM_TYPE;
    case SECONDARY_AUDIO_STREAM:
    {
      const unsigned int numAudioReferences{GetByte(buffer, offset)};
      streamInformation.secondaryAudio_audioReferences.reserve(numAudioReferences);
      for (unsigned int i = 0; i < numAudioReferences; ++i)
      {
        streamInformation.secondaryAudio_audioReferences.emplace_back(
            GetByte(buffer, offset + OFFSET_STREAM_SECONDARY_REFERENCES + i));
      }
      offset += numAudioReferences + OFFSET_STREAM_SECONDARY_REFERENCES +
                (numAudioReferences % 2); // next word boundary
      break;
    }
    case SECONDARY_VIDEO_STREAM:
    {
      const unsigned int numAudioReferences{GetByte(buffer, offset)};
      streamInformation.secondaryVideo_audioReferences.reserve(numAudioReferences);
      for (unsigned int i = 0; i < numAudioReferences; ++i)
      {
        streamInformation.secondaryVideo_audioReferences.emplace_back(
            GetByte(buffer, offset + OFFSET_STREAM_SECONDARY_REFERENCES + i));
      }
      offset += numAudioReferences + OFFSET_STREAM_SECONDARY_REFERENCES +
                (numAudioReferences % 2); // next word boundary
      const unsigned int numPGReferences{GetByte(buffer, offset)};
      streamInformation.secondaryVideo_audioReferences.reserve(numPGReferences);
      for (unsigned int i = 0; i < numPGReferences; ++i)
      {
        streamInformation.secondaryVideo_presentationGraphicReferences.emplace_back(
            GetByte(buffer, offset + OFFSET_STREAM_SECONDARY_REFERENCES + i));
      }
      offset += numPGReferences + OFFSET_STREAM_SECONDARY_REFERENCES +
                (numPGReferences % 2); // next word boundary
      break;
    }
    case VIDEO_STREAM:
    case AUDIO_STREAM:
    case PRESENTATION_GRAPHIC_STREAM:
    case INTERACTIVE_GRAPHIC_STREAM:
    case PICTURE_IN_PICTURE_SUBTITLE_STREAM:
    case DOLBY_VISION_STREAM:
      break;
  }

  CLog::Log(LOGDEBUG, "  Stream - type {}, coding 0x{}",
            static_cast<unsigned int>(streamInformation.type),
            fmt::format("{:02x}", static_cast<int>(streamInformation.coding)));

  return streamInformation;
}

constexpr unsigned int CLPI_HEADER_SIZE = 28;
constexpr unsigned int OFFSET_CLPI_HEADER = 0;
constexpr unsigned int OFFSET_CLPI_VERSION = 4;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION = 12;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_NUM_PROGRAMS = 5;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_SEQUENCE_START = 0;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_PROGRAM_ID = 4;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_NUM_STREAMS = 6;
constexpr unsigned int OFFSET_CLPI_PROGRAM_INFORMATION_NUM_GROUPS = 7;
constexpr unsigned int OFFSET_CLPI_STREAM_INFORMATION_CODING = 1;
constexpr unsigned int OFFSET_CLPI_STREAM_VIDEO_FLAGS = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_AUDIO_FLAGS = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_AUDIO_LANGUAGE = 3;
constexpr unsigned int OFFSET_CLPI_STREAM_SUB_LANGUAGE = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_TEXT_ENCODING = 2;
constexpr unsigned int OFFSET_CLPI_STREAM_TEXT_LANGUAGE = 3;

void ParseProgramInformation(std::vector<std::byte>& buffer,
                             unsigned int& offset,
                             ProgramInformation& programInformation)
{
  programInformation.spnProgramSequenceStart =
      GetDWord(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_SEQUENCE_START);
  programInformation.programId =
      GetWord(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_PROGRAM_ID);
  const unsigned int numStreams{
      GetByte(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_NUM_STREAMS)};
  programInformation.numGroups =
      GetByte(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_NUM_GROUPS);
  offset += 8;

  programInformation.streams.reserve(numStreams);
  for (unsigned int j = 0; j < numStreams; ++j)
  {
    StreamInformation streamInformation;
    streamInformation.packetIdentifier = GetWord(buffer, offset);
    offset += 2;

    const unsigned int streamLength{GetByte(buffer, offset)};
    const ENCODING_TYPE coding{GetByte(buffer, offset + OFFSET_CLPI_STREAM_INFORMATION_CODING)};
    streamInformation.coding = coding;
    switch (coding)
    {
      using enum ENCODING_TYPE;
      case VIDEO_MPEG2:
      case VIDEO_H264:
      case VIDEO_HEVC:
      case VIDEO_VC1:
      {
        const unsigned int flag{GetDWord(buffer, offset + OFFSET_CLPI_STREAM_VIDEO_FLAGS)};
        streamInformation.format = GetBits(flag, 32, 4);
        streamInformation.rate = GetBits(flag, 28, 4);
        streamInformation.aspect = static_cast<ASPECT_RATIO>(GetBits(flag, 24, 4));
        streamInformation.outOfMux = GetBits(flag, 18, 1);
        if (coding == VIDEO_HEVC)
        {
          streamInformation.copyRestricted = GetBits(flag, 17, 1);
          streamInformation.dynamicRangeType = GetBits(flag, 16, 4);
          streamInformation.colorSpace = GetBits(flag, 12, 4);
          streamInformation.HDRPlus = GetBits(flag, 8, 1);
        }
        break;
      }
      case AUDIO_LPCM:
      case AUDIO_AC3:
      case AUDIO_DTS:
      case AUDIO_TRUHD:
      case AUDIO_AC3PLUS:
      case AUDIO_DTSHD:
      case AUDIO_DTSHD_MASTER:
      case AUDIO_AC3PLUS_SECONDARY:
      case AUDIO_DTSHD_SECONDARY:
      {
        const unsigned int flag{GetByte(buffer, offset + OFFSET_CLPI_STREAM_AUDIO_FLAGS)};
        streamInformation.format = GetBits(flag, 8, 4);
        streamInformation.rate = GetBits(flag, 4, 4);
        streamInformation.language =
            GetString(buffer, offset + OFFSET_CLPI_STREAM_AUDIO_LANGUAGE, 3);
        break;
      }
      case SUB_PG:
      case SUB_IG:
      {
        streamInformation.language = GetString(buffer, offset + OFFSET_CLPI_STREAM_SUB_LANGUAGE, 3);
        break;
      }
      case SUB_TEXT:
      {
        streamInformation.characterEncoding =
            GetByte(buffer, offset + OFFSET_CLPI_STREAM_TEXT_ENCODING);
        streamInformation.language =
            GetString(buffer, offset + OFFSET_CLPI_STREAM_TEXT_LANGUAGE, 3);
        break;
      }
      default:
        break;
    }
    programInformation.streams.emplace_back(streamInformation);

    CLog::Log(LOGDEBUG, "  Stream - coding 0x{}",
              fmt::format("{:02x}", static_cast<int>(streamInformation.coding)));

    offset += streamLength + 1;
  }
}

bool ParseCLPI(std::vector<std::byte>& buffer, ClipInformation& clipInformation, unsigned int clip)
{
  // Check size
  if (buffer.size() < CLPI_HEADER_SIZE)
  {
    CLog::Log(LOGDEBUG, "Invalid CLPI - header too small");
    return false;
  }

  // Check header
  const std::string header{GetString(buffer, OFFSET_CLPI_HEADER, 4)};
  const std::string version{GetString(buffer, OFFSET_CLPI_VERSION, 4)};
  if (header != "HDMV")
  {
    CLog::Log(LOGDEBUG, "Invalid CLPI header");
    return false;
  }
  CLog::Log(LOGDEBUG, "Valid CLPI header for clip {} header version {}", clip, version);

  clipInformation.version = version;
  clipInformation.clip = clip;

  const unsigned int programInformationStartAddress{
      GetDWord(buffer, OFFSET_CLPI_PROGRAM_INFORMATION)};
  unsigned int offset{programInformationStartAddress};
  if (const unsigned int length{GetDWord(buffer, offset)}; buffer.size() < length + offset)
  {
    CLog::Log(LOGDEBUG, "Invalid CLPI - too small for Program Information");
    return false;
  }

  const unsigned int numPrograms{
      GetByte(buffer, offset + OFFSET_CLPI_PROGRAM_INFORMATION_NUM_PROGRAMS)};
  offset += 6;
  clipInformation.programs.reserve(numPrograms);
  for (unsigned int i = 0; i < numPrograms; ++i)
  {
    ProgramInformation programInformation;
    CLog::Log(LOGDEBUG, " Program {}", i);
    ParseProgramInformation(buffer, offset, programInformation);
    clipInformation.programs.emplace_back(programInformation);
  }

  return true;
}

bool ReadCLPI(const CURL& url, unsigned int clip, ClipInformation& clipInformation)
{
  const std::string& path{url.GetHostName()};
  const std::string clipFile{
      URIUtils::AddFileToFolder(path, "BDMV", "CLIPINF", StringUtils::Format("{:05}.clpi", clip))};
  CFile file;
  if (!file.Open(clipFile))
    return false;

  const int64_t size{file.GetLength()};
  std::vector<std::byte> buffer;
  buffer.resize(size);
  ssize_t read = file.Read(buffer.data(), size);
  file.Close();

  if (read == size)
    return ParseCLPI(buffer, clipInformation, clip);
  return false;
}

constexpr unsigned int PLAYITEM_HEADER_SIZE = 18;
constexpr unsigned int OFFSET_PLAYITEM_CLIP_ID = 2;
constexpr unsigned int OFFSET_PLAYITEM_CODEC_ID = 7;
constexpr unsigned int OFFSET_PLAYITEM_FLAGS = 11;
constexpr unsigned int OFFSET_PLAYITEM_IN_TIME = 14;
constexpr unsigned int OFFSET_PLAYITEM_OUT_TIME = 18;
constexpr unsigned int OFFSET_PLAYITEM_FLAGS_2 = 30;
constexpr unsigned int OFFSET_PLAYITEM_STILL_MODE = 31;
constexpr unsigned int OFFSET_PLAYITEM_STILL_TIME = 32;
constexpr unsigned int OFFSET_PLAYITEM_NUM_ANGLES = 34;
constexpr unsigned int OFFSET_ANGLE_CODEC_ID = 5;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_VIDEO_STREAMS = 4;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_AUDIO_STREAMS = 5;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_PG_STREAMS = 6;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_IG_STREAMS = 7;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_SECONDARY_VIDEO_STREAMS = 8;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_SECONDARY_AUDIO_STREAMS = 9;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_PIP_STREAMS = 10;
constexpr unsigned int OFFSET_STREAM_TABLE_NUM_DV_STREAMS = 11;

bool ParsePlayItem(std::vector<std::byte>& buffer,
                   unsigned int& offset,
                   PlayItemInformation& playItem)
{
  const unsigned int saveOffset{offset};
  const unsigned int length{GetWord(buffer, offset)};
  if (length < PLAYITEM_HEADER_SIZE)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - Playitem too small");
    return false;
  }
  if (buffer.size() < length + offset)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - too small for Playitem");
    return false;
  }

  std::string clipId{GetString(buffer, offset + OFFSET_PLAYITEM_CLIP_ID, 5)};
  std::string codecId{GetString(buffer, offset + OFFSET_PLAYITEM_CODEC_ID, 4)};
  if (codecId != "M2TS" && codecId != "FMTS")
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - invalid PlayItem codec identifier - {}", codecId);
    return false;
  }

  unsigned int flags{GetWord(buffer, offset + OFFSET_PLAYITEM_FLAGS)};
  const bool isMultiAngle{static_cast<bool>(GetBits(flags, 5, 1))};
  const BLURAY_CONNECTION connectionCondition{GetBits(flags, 4, 4)};
  if (connectionCondition != BLURAY_CONNECTION::SEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::NONSEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::BRANCHING)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - invalid PlayItem connection condition - {}",
              static_cast<unsigned int>(connectionCondition));
    return false;
  }
  playItem.isMultiAngle = isMultiAngle;
  playItem.connectionCondition = connectionCondition;

  const std::chrono::milliseconds inTime{GetDWord(buffer, offset + OFFSET_PLAYITEM_IN_TIME) /
                                         45}; // 45KHz clock
  const std::chrono::milliseconds outTime{GetDWord(buffer, offset + OFFSET_PLAYITEM_OUT_TIME) /
                                          45}; // 45KHz clock
  playItem.inTime = inTime;
  playItem.outTime = outTime;

  flags = GetByte(buffer, offset + OFFSET_PLAYITEM_FLAGS_2);
  const bool randomAccessFlag{static_cast<bool>(GetBits(flags, 8, 1))};
  const unsigned int stillMode{GetByte(buffer, offset + OFFSET_PLAYITEM_STILL_MODE)};
  std::chrono::milliseconds stillTime{0ms};
  if (stillMode == BLURAY_STILL_TIME)
    stillTime = std::chrono::milliseconds(GetWord(buffer, offset + OFFSET_PLAYITEM_STILL_TIME));
  playItem.randomAccessFlag = randomAccessFlag;
  playItem.stillMode = stillMode;
  playItem.stillTime = stillTime;

  unsigned int angleCount{1};
  if (isMultiAngle)
  {
    angleCount = GetByte(buffer, offset + OFFSET_PLAYITEM_NUM_ANGLES);
    offset += 36;
  }
  else
    offset += 34;

  CLog::Log(LOGDEBUG,
            "PlayItem entry - clip id {}, codec id {}, in time {}, out time {}, still mode "
            "{} (still time {}), angles {}",
            clipId, codecId, fmt::format("{:%H:%M:%S}", inTime),
            fmt::format("{:%H:%M:%S}", outTime), stillMode, stillTime, angleCount);

  // First/only angle
  ClipInformation angleInformation;
  angleInformation.clip = std::stoi(clipId);
  angleInformation.codec = codecId;
  playItem.angleClips.emplace_back(angleInformation);

  // Get additional angles (if any)
  for (unsigned int j = 1; j < angleCount; ++j)
  {
    ClipInformation additionalAngleInformation;
    const std::string angleClipId{GetString(buffer, offset, 5)};
    const std::string angleCodecId{GetString(buffer, offset + OFFSET_ANGLE_CODEC_ID, 4)};
    if (angleCodecId != "M2TS" && angleCodecId != "FMTS")
    {
      CLog::Log(LOGDEBUG, "Invalid MPLS - invalid PlayItem angle {} codec identifier - {}", j,
                angleCodecId);
      return false;
    }
    CLog::Log(LOGDEBUG, "  Additional angle {} - clip id {}, codec id {}", j, angleClipId,
              angleCodecId);

    additionalAngleInformation.clip = std::stoi(angleClipId);
    additionalAngleInformation.codec = angleCodecId;
    playItem.angleClips.emplace_back(additionalAngleInformation);
    offset += 10;
  }

  // Parse stream number table
  if (const unsigned int stnLength{GetWord(buffer, offset)}; buffer.size() < stnLength + offset)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - too small for Stream Number Table");
    return false;
  }

  const unsigned int numVideoStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_VIDEO_STREAMS)};
  const unsigned int numAudioStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_AUDIO_STREAMS)};
  const unsigned int numPresentationGraphicStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_PG_STREAMS)};
  const unsigned int numInteractiveGraphicStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_IG_STREAMS)};
  const unsigned int numSecondaryVideoStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_SECONDARY_VIDEO_STREAMS)};
  const unsigned int numSecondaryAudioStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_SECONDARY_AUDIO_STREAMS)};
  const unsigned int numPictureInPictureSubtitleStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_PIP_STREAMS)};
  const unsigned int numDolbyVisionStreams{
      GetByte(buffer, offset + OFFSET_STREAM_TABLE_NUM_DV_STREAMS)};

  CLog::Log(LOGDEBUG,
            " Stream number table - video {}, audio {}, presentation graphic (subtitle) {}, "
            "interactive graphic {}, "
            "secondary video {}, secondary audio {}, PIP subtitle {}, dolby vision {}",
            numVideoStreams, numAudioStreams, numPresentationGraphicStreams,
            numInteractiveGraphicStreams, numSecondaryVideoStreams, numSecondaryAudioStreams,
            numPictureInPictureSubtitleStreams, numDolbyVisionStreams);

  offset += 16;

  playItem.videoStreams.reserve(numVideoStreams);
  for (unsigned int k = 0; k < numVideoStreams; ++k)
  {
    playItem.videoStreams.emplace_back(ParseStream(buffer, offset, STREAM_TYPE::VIDEO_STREAM));
  }

  playItem.audioStreams.reserve(numAudioStreams);
  for (unsigned int k = 0; k < numAudioStreams; ++k)
  {
    playItem.audioStreams.emplace_back(ParseStream(buffer, offset, STREAM_TYPE::AUDIO_STREAM));
  }

  playItem.presentationGraphicStreams.reserve(numPresentationGraphicStreams);
  for (unsigned int k = 0; k < numPresentationGraphicStreams; ++k)
  {
    playItem.presentationGraphicStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::PRESENTATION_GRAPHIC_STREAM));
  }

  playItem.interactiveGraphicStreams.reserve(numInteractiveGraphicStreams);
  for (unsigned int k = 0; k < numInteractiveGraphicStreams; ++k)
  {
    playItem.interactiveGraphicStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::INTERACTIVE_GRAPHIC_STREAM));
  }

  playItem.secondaryAudioStreams.reserve(numSecondaryAudioStreams);
  for (unsigned int k = 0; k < numSecondaryAudioStreams; ++k)
  {
    playItem.secondaryAudioStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::SECONDARY_AUDIO_STREAM));
  }

  playItem.secondaryVideoStreams.reserve(numSecondaryVideoStreams);
  for (unsigned int k = 0; k < numSecondaryVideoStreams; ++k)
  {
    playItem.secondaryVideoStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::SECONDARY_VIDEO_STREAM));
  }

  playItem.dolbyVisionStreams.reserve(numDolbyVisionStreams);
  for (unsigned int k = 0; k < numDolbyVisionStreams; ++k)
  {
    playItem.dolbyVisionStreams.emplace_back(
        ParseStream(buffer, offset, STREAM_TYPE::DOLBY_VISION_STREAM));
  }

  offset = saveOffset + length + 2;
  return true;
}

constexpr unsigned int SUBPLAYITEM_HEADER_SIZE = 24;
constexpr unsigned int OFFSET_SUBPLAYITEM_CLIP_ID = 2;
constexpr unsigned int OFFSET_SUBPLAYITEM_CODEC_ID = 7;
constexpr unsigned int OFFSET_SUBPLAYITEM_FLAGS = 11;
constexpr unsigned int OFFSET_SUBPLAYITEM_IN_TIME = 16;
constexpr unsigned int OFFSET_SUBPLAYITEM_OUT_TIME = 20;
constexpr unsigned int OFFSET_SUBPLAYITEM_SYNC_PLAYITEM_ID = 24;
constexpr unsigned int OFFSET_SUBPLAYITEM_NUM_CLIPS = 26;
constexpr unsigned int OFFSET_SUBPLAYITEM_CLIP_CODEC_ID = 5;

bool ParseSubPlayItem(std::vector<std::byte>& buffer,
                      unsigned int& offset,
                      SubPlayItemInformation& subPlayItemInformation)
{
  const unsigned int saveOffset{offset};
  unsigned int length{GetWord(buffer, offset)};
  if (length < SUBPLAYITEM_HEADER_SIZE)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - SubPlayItem too small");
    return false;
  }
  if (buffer.size() < length + offset)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - too small for SubPlayItem");
    return false;
  }

  std::string clipId{GetString(buffer, offset + OFFSET_SUBPLAYITEM_CLIP_ID, 5)};
  std::string codecId{GetString(buffer, offset + OFFSET_SUBPLAYITEM_CODEC_ID, 4)};
  if (codecId != "M2TS" && codecId != "FMTS")
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - invalid PlayItem codec identifier - {}", codecId);
    return false;
  }

  unsigned int flags{GetDWord(buffer, offset + OFFSET_SUBPLAYITEM_FLAGS)};
  const bool isMultiClip{static_cast<bool>(GetBits(flags, 1, 1))};
  const BLURAY_CONNECTION connectionCondition{GetBits(flags, 5, 4)};
  if (connectionCondition != BLURAY_CONNECTION::SEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::NONSEAMLESS &&
      connectionCondition != BLURAY_CONNECTION::BRANCHING)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - invalid PlayItem connection condition - {}",
              static_cast<unsigned int>(connectionCondition));
    return false;
  }
  subPlayItemInformation.isMultiClip = isMultiClip;
  subPlayItemInformation.connectionCondition = connectionCondition;

  const std::chrono::milliseconds inTime{GetDWord(buffer, offset + OFFSET_SUBPLAYITEM_IN_TIME) /
                                         45}; // 45KHz clock
  const std::chrono::milliseconds outTime{GetDWord(buffer, offset + OFFSET_SUBPLAYITEM_OUT_TIME) /
                                          45}; // 45KHz clock
  subPlayItemInformation.inTime = inTime;
  subPlayItemInformation.outTime = outTime;

  const unsigned int syncPlayItemId{GetWord(buffer, offset + OFFSET_SUBPLAYITEM_SYNC_PLAYITEM_ID)};
  subPlayItemInformation.syncPlayItemId = syncPlayItemId;

  unsigned int numClips{1};
  if (isMultiClip)
  {
    numClips = GetByte(buffer, offset + OFFSET_SUBPLAYITEM_NUM_CLIPS);
    offset += 27;
  }
  else
    offset += 26;

  CLog::Log(LOGDEBUG,
            " SubPlayItem entry - clip id {}, codec id {}, in time {}, out time {}, clips {}",
            clipId, codecId, fmt::format("{:%H:%M:%S}", inTime),
            fmt::format("{:%H:%M:%S}", outTime), numClips);

  // First/only clip
  ClipInformation clipInformation;
  clipInformation.clip = std::stoi(clipId);
  clipInformation.codec = codecId;
  subPlayItemInformation.clips.emplace_back(clipInformation);

  // Get additional clips (if any)
  for (unsigned int j = 1; j < numClips; ++j)
  {
    ClipInformation additionalClipInformation;
    const std::string additionalClipId{GetString(buffer, offset, 5)};
    const std::string additionalCodecId{
        GetString(buffer, offset + OFFSET_SUBPLAYITEM_CLIP_CODEC_ID, 4)};
    if (additionalCodecId != "M2TS" && additionalCodecId != "FMTS")
    {
      CLog::Log(LOGDEBUG, "Invalid MPLS - invalid SubPlayItem clip {} codec identifier - {}", j,
                additionalCodecId);
      return false;
    }
    CLog::Log(LOGDEBUG, "  Additional clip {} - clip id {}, codec id {}", j, additionalClipId,
              additionalCodecId);

    additionalClipInformation.clip = std::stoi(additionalClipId);
    additionalClipInformation.codec = additionalCodecId;
    subPlayItemInformation.clips.emplace_back(additionalClipInformation);
    offset += 10;
  }

  offset = saveOffset + length + 2;

  return true;
}

// Structure of an MPLS File:
//
// Header: Contains general information about the playlist.
//
// PlayItems: Each PlayItem corresponds to a specific .m2ts file and includes details like
// In Time, Out Time, and System Time Clock ID for synchronization.
//
// SubPath: Optional, used for features like Picture-in-Picture (PiP) or secondary audio.

constexpr unsigned int MPLS_HEADER_SIZE = 40;
constexpr unsigned int MPLS_PLAYLISTMARK_SIZE = 14;
constexpr unsigned int OFFSET_MPLS_HEADER = 0;
constexpr unsigned int OFFSET_MPLS_VERSION = 4;
constexpr unsigned int OFFSET_MPLS_PLAYLIST_POSITION = 8;
constexpr unsigned int OFFSET_MPLS_PLAYLIST_MARK_POSITION = 12;
constexpr unsigned int OFFSET_MPLS_APP_INFO_PLAYLIST = 40;
constexpr unsigned int OFFSET_MPLS_APP_INFO_PLAYBACK_TYPE = 5;
constexpr unsigned int OFFSET_MPLS_NUM_PLAYITEMS = 6;
constexpr unsigned int OFFSET_MPLS_NUM_SUBPATHS = 8;
constexpr unsigned int OFFSET_MPLS_SUBPATH_NUM_SUBPLAYITEMS = 9;
constexpr unsigned int OFFSET_MPLS_NUM_PLAYLISTMARKS = 4;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_TYPE = 1;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_PLAYITEM = 2;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_TIME = 4;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_PACKET_IDENTIFIER = 8;
constexpr unsigned int OFFSET_MPLS_PLAYLISTMARK_DURATION = 10;

bool ProcessClips(const CURL& url,
                  PlaylistInformation& playlistInformation,
                  std::map<unsigned int, ClipInformation>& clipCache)
{
  for (const auto& playItem : playlistInformation.playItems)
  {
    for (const auto& clip : playItem.angleClips)
    {
      if (const auto& it = clipCache.find(clip.clip); it != clipCache.end())
      {
        // In local cache
        playlistInformation.clips.emplace_back(it->second);
        continue;
      }

      // Not in local cache
      ClipInformation clipInformation;
      if (!ReadCLPI(url, clip.clip, clipInformation))
      {
        CLog::Log(LOGDEBUG, "Cannot read clip {} information", clip.clip);
        return false;
      }
      playlistInformation.clips.emplace_back(clipInformation);
      clipCache[clip.clip] = clipInformation;
    }
  }
  return true;
}

bool ParsePlaylistMark(std::vector<std::byte>& buffer,
                       unsigned int& offset,
                       PlaylistInformation& playlistInformation)
{
  if (const unsigned int playlistMarkSize{GetDWord(buffer, offset)};
      buffer.size() < playlistMarkSize + offset)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - too small for PlayListMark");
    return false;
  }

  const unsigned int numPlaylistMarks{GetWord(buffer, offset + OFFSET_MPLS_NUM_PLAYLISTMARKS)};
  if (buffer.size() < (numPlaylistMarks * MPLS_PLAYLISTMARK_SIZE) + offset + 6)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - too small for PlayListMark");
    return false;
  }

  offset += 6;
  playlistInformation.playlistMarks.reserve(numPlaylistMarks);
  for (unsigned int i = 0; i < numPlaylistMarks; ++i)
  {
    PlaylistMarkInformation playlistMark{
        .markType =
            static_cast<BLURAY_MARK_TYPE>(GetByte(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_TYPE)),
        .playItemReference = GetWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_PLAYITEM),
        .time = std::chrono::milliseconds(GetDWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_TIME) /
                                          45), // 45KHz clock
        .elementaryStreamPacketIdentifier =
            GetWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_PACKET_IDENTIFIER),
        .duration = std::chrono::milliseconds(
            GetDWord(buffer, offset + OFFSET_MPLS_PLAYLISTMARK_DURATION))};
    playlistInformation.playlistMarks.emplace_back(playlistMark);
    offset += MPLS_PLAYLISTMARK_SIZE;
  }

  return true;
}

void GetChaptersAndTimings(PlaylistInformation& playlistInformation)
{
  // Update clip timings from corresponding playItems
  std::chrono::milliseconds startTime{0ms};
  for (unsigned int i = 0; i < playlistInformation.playItems.size(); ++i)
  {
    const auto& playItem = playlistInformation.playItems[i];
    auto& clip = playlistInformation.clips[i];
    clip.duration = playItem.outTime - playItem.inTime;
    clip.time = startTime;
    startTime += clip.duration;
  }

  // Update playMark timings
  int prevChapter{-1};
  for (int i = 0; i < static_cast<int>(playlistInformation.playlistMarks.size()); ++i)
  {
    auto& playlistMark{playlistInformation.playlistMarks[i]};

    // Get referenced playItem and clip to calculate playMark time
    const auto& clip = playlistInformation.clips[playlistMark.playItemReference];
    const auto& playItem = playlistInformation.playItems[playlistMark.playItemReference];
    playlistMark.time = clip.time + playlistMark.time - playItem.inTime;

    if (playlistMark.markType != BLURAY_MARK_TYPE::ENTRY) // chapter
      continue;

    if (prevChapter >= 0)
    {
      auto& prevMark{playlistInformation.playlistMarks[prevChapter]};
      if (prevMark.duration == 0ms)
        prevMark.duration = playlistMark.time - prevMark.time;
    }
    prevChapter = i;
  }
  if (prevChapter >= 0 && playlistInformation.playlistMarks[prevChapter].duration == 0ms)
  {
    auto& prevMark{playlistInformation.playlistMarks[prevChapter]};
    prevMark.duration = playlistInformation.duration - prevMark.time;
  }

  // Derive chapters from playMarks
  unsigned int chapter{1};
  std::chrono::milliseconds start{0ms};
  for (const auto& playlistMark : playlistInformation.playlistMarks)
  {
    if (playlistMark.markType == BLURAY_MARK_TYPE::ENTRY)
    {
      ChapterInformation chapterInformation{
          .chapter = chapter, .start = start, .duration = playlistMark.duration};
      playlistInformation.chapters.emplace_back(chapterInformation);
      CLog::Log(LOGDEBUG, "Chapter {} start {} duration {} end {}", chapter,
                fmt::format("{:%H:%M:%S}", start),
                fmt::format("{:%H:%M:%S}", chapterInformation.duration),
                fmt::format("{:%H:%M:%S}", start + chapterInformation.duration));
      start += chapterInformation.duration;
      ++chapter;
    }
  }
}

bool ParseMPLS(const CURL& url,
               std::vector<std::byte>& buffer,
               PlaylistInformation& playlistInformation,
               unsigned int playlist,
               std::map<unsigned int, ClipInformation>& clipCache)
{
  // Check size
  if (buffer.size() < MPLS_HEADER_SIZE)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - header too small");
    return false;
  }

  // Check header
  const std::string header{GetString(buffer, OFFSET_MPLS_HEADER, 4)};
  const std::string version{GetString(buffer, OFFSET_MPLS_VERSION, 4)};
  if (header != "MPLS")
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS header");
    return false;
  }
  CLog::Log(LOGDEBUG, "*** Valid MPLS header for playlist {} version {}", playlist, version);

  playlistInformation.playlist = playlist;
  playlistInformation.version = version;

  const unsigned int playlistPosition{GetDWord(buffer, OFFSET_MPLS_PLAYLIST_POSITION)};
  const unsigned int playlistMarkPosition{GetDWord(buffer, OFFSET_MPLS_PLAYLIST_MARK_POSITION)};

  // AppInfoPlaylist
  unsigned int offset{OFFSET_MPLS_APP_INFO_PLAYLIST};
  if (const unsigned int appInfoSize{GetDWord(buffer, offset)};
      buffer.size() < appInfoSize + offset)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - too small for AppInfoPlaylist");
    return false;
  }

  BLURAY_PLAYBACK_TYPE playbackType{GetByte(buffer, offset + OFFSET_MPLS_APP_INFO_PLAYBACK_TYPE)};
  unsigned int playbackCount{0};
  if (playbackType == BLURAY_PLAYBACK_TYPE::RANDOM || playbackType == BLURAY_PLAYBACK_TYPE::SHUFFLE)
    playbackCount = GetWord(buffer, offset + 6);
  playlistInformation.playbackType = playbackType;
  playlistInformation.playbackCount = playbackCount;

  // Playlist
  offset = playlistPosition;
  if (const unsigned int playlistSize{GetDWord(buffer, offset)};
      buffer.size() < playlistSize + offset)
  {
    CLog::Log(LOGDEBUG, "Invalid MPLS - too small for Playlist");
    return false;
  }
  const unsigned int numPlayItems{GetWord(buffer, offset + OFFSET_MPLS_NUM_PLAYITEMS)};
  const unsigned int numSubPaths{GetWord(buffer, offset + OFFSET_MPLS_NUM_SUBPATHS)};
  offset += 10;

  playlistInformation.playItems.reserve(numPlayItems);
  for (unsigned int i = 0; i < numPlayItems; ++i)
  {
    PlayItemInformation playItem;
    if (!ParsePlayItem(buffer, offset, playItem))
      return false;
    playlistInformation.playItems.emplace_back(playItem);
  }

  // Calculate duration
  std::chrono::milliseconds duration{0ms};
  for (const auto& playItem : playlistInformation.playItems)
    duration += playItem.outTime - playItem.inTime;
  playlistInformation.duration = duration;
  CLog::Log(LOGDEBUG, "Playlist duration {}", fmt::format("{:%H:%M:%S}", duration));

  // Process clips
  if (!ProcessClips(url, playlistInformation, clipCache))
    return false;

  if (numSubPaths > 0)
  {
    for (unsigned int i = 0; i < numSubPaths; ++i)
    {
      const unsigned int numSubPlayItems{
          GetByte(buffer, offset + OFFSET_MPLS_SUBPATH_NUM_SUBPLAYITEMS)};
      offset += 10;

      if (numSubPlayItems == 0)
        continue;

      playlistInformation.subPlayItems.reserve(numSubPlayItems);
      for (unsigned int j = 0; j < numSubPlayItems; ++j)
      {
        SubPlayItemInformation subPlayItem;
        if (!ParseSubPlayItem(buffer, offset, subPlayItem))
          return false;
        playlistInformation.subPlayItems.emplace_back(subPlayItem);
      }
    }
  }

  // Parse PlayListMark
  offset = playlistMarkPosition;
  if (!ParsePlaylistMark(buffer, offset, playlistInformation))
    return false;

  GetChaptersAndTimings(playlistInformation);

  return true;
}

std::shared_ptr<CFileItem> GetFileItem(const CURL& url,
                                       const PlaylistInfo& title,
                                       const std::string& label)
{
  CURL path{url};
  path.SetFileName(StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", title.playlist));
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};
  const int duration{static_cast<int>(title.duration.count() / 1000)};
  item->GetVideoInfoTag()->SetDuration(duration);
  item->SetProperty("bluray_playlist", title.playlist);
  const std::string buf{StringUtils::Format(label, title.playlist)};
  item->SetTitle(buf);
  item->SetLabel(buf);
  const std::string chap{StringUtils::Format(g_localizeStrings.Get(25007), title.chapters.size(),
                                             StringUtils::SecondsToTimeString(duration))};
  item->SetLabel2(chap);
  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  // Populate videoInfo
  if (!title.videoStreams.empty())
  {
    VideoStreamInfo videoInfo;
    const auto& video{title.videoStreams[0]};
    videoInfo.valid = true;
    videoInfo.bitrate = 0;
    switch (video.format)
    {
      case BLURAY_VIDEO_FORMAT_480I:
      case BLURAY_VIDEO_FORMAT_480P:
        videoInfo.height = 480;
        videoInfo.width = 640; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_576I:
      case BLURAY_VIDEO_FORMAT_576P:
        videoInfo.height = 576;
        videoInfo.width = 720; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_720P:
        videoInfo.height = 720;
        videoInfo.width = 1280; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_1080I:
      case BLURAY_VIDEO_FORMAT_1080P:
        videoInfo.height = 1080;
        videoInfo.width = 1920; // Guess but never displayed
        break;
      case BLURAY_VIDEO_FORMAT_2160P:
        videoInfo.height = 2160;
        videoInfo.width = 3840; // Guess but never displayed
        break;
      default:
        videoInfo.height = 0;
        videoInfo.width = 0;
        break;
    }
    switch (video.coding)
    {
      using enum ENCODING_TYPE;
      case VIDEO_MPEG2:
        videoInfo.codecName = "mpeg2";
        break;
      case VIDEO_VC1:
        videoInfo.codecName = "vc1";
        break;
      case VIDEO_H264:
        videoInfo.codecName = "h264";
        break;
      case VIDEO_HEVC:
        videoInfo.codecName = "hevc";
        break;
      default:
        videoInfo.codecName = "";
        break;
    }
    switch (video.aspect)
    {
      using enum ASPECT_RATIO;
      case RATIO_4_3:
        videoInfo.videoAspectRatio = 4.0f / 3.0f;
        break;
      case RATIO_16_9:
        videoInfo.videoAspectRatio = 16.0f / 9.0f;
        break;
      default:
        videoInfo.videoAspectRatio = 0.0f;
        break;
    }
    videoInfo.stereoMode = ""; // Not stored in BLURAY_TITLE_INFO
    videoInfo.flags = FLAG_NONE;
    videoInfo.hdrType = StreamHdrType::HDR_TYPE_NONE; // Not stored in BLURAY_TITLE_INFO
    videoInfo.fpsRate = 0; // Not in streamdetails
    videoInfo.fpsScale = 0; // Not in streamdetails

    CVideoInfoTag* info = item->GetVideoInfoTag();
    info->m_streamDetails.SetStreams(videoInfo, static_cast<int>(title.duration.count() / 1000),
                                     AudioStreamInfo{}, SubtitleStreamInfo{});

    for (const auto& audio : title.audioStreams)
    {
      AudioStreamInfo audioInfo;
      audioInfo.valid = true;
      audioInfo.bitrate = 0;
      audioInfo.channels = 0; // Only basic mono/stereo/multichannel is stored in BLURAY_TITLE_INFO

      switch (audio.coding)
      {
        using enum ENCODING_TYPE;
        case AUDIO_AC3:
          audioInfo.codecName = "ac3";
          break;
        case AUDIO_AC3PLUS:
        case AUDIO_AC3PLUS_SECONDARY:
          audioInfo.codecName = "eac3";
          break;
        case AUDIO_LPCM:
          audioInfo.codecName = "pcm";
          break;
        case AUDIO_DTS:
          audioInfo.codecName = "dts";
          break;
        case AUDIO_DTSHD:
        case AUDIO_DTSHD_SECONDARY:
          audioInfo.codecName = "dtshd";
          break;
        case AUDIO_DTSHD_MASTER:
          audioInfo.codecName = "dtshd_ma";
          break;
        case AUDIO_TRUHD:
          audioInfo.codecName = "truehd";
          break;
        default:
          audioInfo.codecName = "";
          break;
      }
      audioInfo.flags = FLAG_NONE;
      audioInfo.language = audio.lang;
      info->m_streamDetails.AddStream(new CStreamDetailAudio(audioInfo));
    }

    // Subtitles
    for (const auto& subtitle : title.pgStreams)
    {
      SubtitleStreamInfo subtitleInfo;
      subtitleInfo.valid = true;
      subtitleInfo.bitrate = 0;
      subtitleInfo.codecDesc = "";
      subtitleInfo.codecName = "";
      subtitleInfo.isExternal = false;
      subtitleInfo.name = "";
      subtitleInfo.flags = FLAG_NONE;
      subtitleInfo.language = subtitle.lang;
      info->m_streamDetails.AddStream(new CStreamDetailSubtitle(subtitleInfo));
    }
  }

  return item;
}

int GetMainPlaylistFromDisc(const CURL& url)
{
  const std::string& root{url.GetHostName()};
  const std::string discInfPath{URIUtils::AddFileToFolder(root, "disc.inf")};
  CFile file;
  std::string line;
  line.reserve(1024);
  int playlist{-1};

  if (file.Open(discInfPath))
  {
    CLog::LogF(LOGDEBUG, "disc.inf found");
    CRegExp pl{true, CRegExp::autoUtf8, R"((?:playlists=)(\d+))"};
    uint8_t maxLines{100};
    while ((maxLines > 0) && file.ReadLine(line))
    {
      maxLines--;
      if (pl.RegFind(line) != -1)
      {
        playlist = std::stoi(pl.GetMatch(1));
        break;
      }
    }
    file.Close();
  }
  return playlist;
}

std::string GetCachePath(const CURL& url, const std::string& realPath)
{
  if (url.Get().empty())
    return realPath;
  std::string path{url.GetHostName()};
  if (path.empty())
    path = url.Get(); // Could be drive letter
  return path;
}

bool ReadMPLS(const CURL& url,
              unsigned int playlist,
              PlaylistInformation& playlistInformation,
              std::map<unsigned int, ClipInformation>& clipCache)
{
  const std::string& path{url.GetHostName()};
  const std::string playlistFile{URIUtils::AddFileToFolder(
      path, "BDMV", "PLAYLIST", StringUtils::Format("{:05}.mpls", playlist))};
  CFile file;
  if (!file.Open(playlistFile))
    return false;

  const int64_t size{file.GetLength()};
  std::vector<std::byte> buffer;
  buffer.resize(size);
  ssize_t read = file.Read(buffer.data(), size);
  file.Close();

  if (read == size)
    return ParseMPLS(url, buffer, playlistInformation, playlist, clipCache);
  return false;
}

bool GetPlaylistInfoFromDisc(const CURL& url,
                             const std::string& realPath,
                             unsigned int playlist,
                             PlaylistInfo& playlistInfo,
                             std::map<unsigned int, ClipInformation>& clipCache)
{
  const std::string path{GetCachePath(url, realPath)};

  // Check cache
  PlaylistInformation p;
  if (!CServiceBroker::GetBlurayDiscCache()->GetPlaylistInfo(path, playlist, p))
  {
    // Retrieve from disc
    if (!ReadMPLS(url, playlist, p, clipCache))
      return false;

    // Cache and return
    CServiceBroker::GetBlurayDiscCache()->SetPlaylistInfo(path, playlist, p);
  }

  // Parse PlaylistInformation into PlayInfo
  playlistInfo.playlist = p.playlist;
  playlistInfo.duration = p.duration;
  playlistInfo.chapters.reserve(p.chapters.size());
  for (const ChapterInformation& chapter : p.chapters)
    playlistInfo.chapters.emplace_back(chapter.start);
  playlistInfo.clips.reserve(p.clips.size());
  for (const ClipInformation& clip : p.clips)
  {
    playlistInfo.clips.emplace_back(clip.clip);
    playlistInfo.clipDuration[clip.clip] = clip.duration;
  }
  if (!p.clips.empty() && !p.clips[0].programs.empty())
  {
    for (const StreamInformation& stream : p.clips[0].programs[0].streams)
    {
      switch (stream.coding)
      {
        using enum ENCODING_TYPE;
        case VIDEO_MPEG2:
        case VIDEO_VC1:
        case VIDEO_H264:
        case VIDEO_HEVC:
          playlistInfo.videoStreams.emplace_back(DiscStreamInfo{.coding = stream.coding,
                                                                .format = stream.format,
                                                                .rate = stream.rate,
                                                                .aspect = stream.aspect,
                                                                .lang = ""});
          break;
        case AUDIO_LPCM:
        case AUDIO_AC3:
        case AUDIO_DTS:
        case AUDIO_TRUHD:
        case AUDIO_AC3PLUS:
        case AUDIO_DTSHD:
        case AUDIO_DTSHD_MASTER:
          playlistInfo.audioStreams.emplace_back(DiscStreamInfo{.coding = stream.coding,
                                                                .format = stream.format,
                                                                .rate = stream.rate,
                                                                .aspect = stream.aspect,
                                                                .lang = stream.language});
          break;
        case SUB_PG:
        case SUB_TEXT:
          playlistInfo.pgStreams.emplace_back(DiscStreamInfo{.coding = stream.coding,
                                                             .format = stream.format,
                                                             .rate = stream.rate,
                                                             .aspect = stream.aspect,
                                                             .lang = stream.language});
          break;
        case SUB_IG:
        case AUDIO_AC3PLUS_SECONDARY:
        case AUDIO_DTSHD_SECONDARY:
        default:
          break;
      }
    }
  }
  return true;
}

bool GetPlaylistsFromDisc(const CURL& url,
                          const std::string& realPath,
                          int flags,
                          std::vector<PlaylistInfo>& playlists,
                          std::map<unsigned int, ClipInformation>& clipCache)
{
  const CURL url2{URIUtils::AddFileToFolder(url.GetHostName(), "BDMV", "PLAYLIST", "")};
  CDirectory::CHints hints;
  hints.flags = flags;
  CFileItemList allTitles;
  if (!CDirectory::GetDirectory(url2, allTitles, hints))
    return false;

  // Get information on all playlists
  const std::regex playlistPath("(\\d{5}.mpls)");
  std::smatch playlistMatch;
  for (const auto& title : allTitles)
  {
    const CURL url3{title->GetPath()};
    const std::string filename{URIUtils::GetFileName(url3.GetFileName())};
    if (std::regex_search(filename, playlistMatch, playlistPath))
    {
      const unsigned int playlist{static_cast<unsigned int>(std::stoi(playlistMatch[0]))};

      PlaylistInfo t{};
      if (!GetPlaylistInfoFromDisc(url, realPath, playlist, t, clipCache))
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
      else
        playlists.emplace_back(t);
    }
  }
  return true;
}

void RemoveDuplicatePlaylists(std::vector<PlaylistInfo>& playlists)
{
  std::unordered_set<unsigned int> duplicatePlaylists;
  for (unsigned int i = 0; i < playlists.size() - 1; ++i)
  {
    for (unsigned int j = i + 1; j < playlists.size(); ++j)
    {
      if (playlists[i].audioStreams == playlists[j].audioStreams &&
          playlists[i].pgStreams == playlists[j].pgStreams &&
          playlists[i].chapters == playlists[j].chapters &&
          playlists[i].clips == playlists[j].clips)
      {
        duplicatePlaylists.emplace(playlists[j].playlist);
      }
    }
  }
  std::erase_if(playlists, [&duplicatePlaylists](const PlaylistInfo& p)
                { return duplicatePlaylists.contains(p.playlist); });
}

void RemoveShortPlaylists(std::vector<PlaylistInfo>& playlists)
{
  const std::chrono::milliseconds minimumDuration{CServiceBroker::GetSettingsComponent()
                                                      ->GetAdvancedSettings()
                                                      ->m_minimumEpisodePlaylistDuration *
                                                  1000};
  if (std::ranges::any_of(playlists, [&minimumDuration](const PlaylistInfo& playlist)
                          { return playlist.duration >= minimumDuration; }))
  {
    std::erase_if(playlists, [&minimumDuration](const PlaylistInfo& playlist)
                  { return playlist.duration < minimumDuration; });
  }
}

void SortPlaylists(std::vector<PlaylistInfo>& playlists, SortTitles sort, int mainPlaylist)
{
  std::ranges::sort(playlists,
                    [&sort](const PlaylistInfo& i, const PlaylistInfo& j)
                    {
                      if (sort == SortTitles::SORT_TITLES_MOVIE)
                      {
                        if (i.duration == j.duration)
                          return i.playlist < j.playlist;
                        return i.duration > j.duration;
                      }
                      return i.playlist < j.playlist;
                    });

  const auto& pivot{
      std::ranges::find_if(playlists, [&mainPlaylist](const PlaylistInfo& title)
                           { return title.playlist == static_cast<unsigned int>(mainPlaylist); })};
  if (pivot != playlists.end())
    std::rotate(playlists.begin(), pivot, pivot + 1);
}

bool IncludePlaylist(GetTitles job,
                     const PlaylistInfo& title,
                     std::chrono::milliseconds minDuration,
                     int mainPlaylist,
                     unsigned int maxPlaylist)
{
  using enum GetTitles;
  return job == GET_TITLES_ALL || job == GET_TITLES_EPISODES ||
         (job == GET_TITLES_MAIN && title.duration >= minDuration) ||
         (job == GET_TITLES_ONE && (title.playlist == static_cast<unsigned int>(mainPlaylist) ||
                                    (mainPlaylist == -1 && title.playlist == maxPlaylist)));
}

bool GetPlaylists(const CURL& url,
                  const std::string& realPath,
                  int flags,
                  GetTitles job,
                  CFileItemList& items,
                  SortTitles sort,
                  std::map<unsigned int, ClipInformation>& clipCache)
{
  std::vector<PlaylistInfo> playlists;
  int mainPlaylist{-1};

  // See if disc.inf for main playlist
  if (job != GetTitles::GET_TITLES_ALL && job != GetTitles::GET_TITLES_EPISODES)
  {
    mainPlaylist = GetMainPlaylistFromDisc(url);
    if (mainPlaylist != -1)
    {
      // Only main playlist is needed
      PlaylistInfo t{};
      if (!GetPlaylistInfoFromDisc(url, realPath, mainPlaylist, t, clipCache))
        CLog::LogF(LOGDEBUG, "Unable to get playlist {}", mainPlaylist);
      else
        playlists.emplace_back(t);
    }
  }

  if (playlists.empty() && !GetPlaylistsFromDisc(url, realPath, flags, playlists, clipCache))
    return false;

  // Remove playlists with no clips
  std::erase_if(playlists, [](const PlaylistInfo& playlist) { return playlist.clips.empty(); });

  // Remove all clips less than a second in length
  std::erase_if(playlists, [](const PlaylistInfo& playlist) { return playlist.duration < 1s; });

  // Remove playlists with duplicate clips
  std::erase_if(playlists,
                [](const PlaylistInfo& playlist)
                {
                  std::unordered_set<unsigned int> clips;
                  for (const auto& clip : playlist.clips)
                    clips.emplace(clip);
                  return clips.size() < playlist.clips.size();
                });

  // Remove duplicate playlists
  // For episodes playlist selection happens in CDiscDirectoryHelper
  if (job != GetTitles::GET_TITLES_ALL && job != GetTitles::GET_TITLES_EPISODES &&
      playlists.size() > 1)
    RemoveDuplicatePlaylists(playlists);

  // Remove playlists below minimum duration (default 5 minutes) unless that would leave no playlists
  if (job != GetTitles::GET_TITLES_ALL)
    RemoveShortPlaylists(playlists);

  // No playlists found
  if (playlists.empty())
    return false;

  // Now we have curated playlists, find longest (for main title derivation)
  const auto& it{std::ranges::max_element(playlists, {}, &PlaylistInfo::duration)};
  const std::chrono::milliseconds maxDuration{it->duration};
  const unsigned int maxPlaylist{it->playlist};

  // Sort
  // Movies - placing main title - if present - first, then by duration
  // Episodes - by playlist number
  if (sort != SortTitles::SORT_TITLES_NONE)
    SortPlaylists(playlists, sort, mainPlaylist);

  const std::chrono::milliseconds minDuration{maxDuration * MAIN_TITLE_LENGTH_PERCENT / 100};
  for (const auto& title : playlists)
  {
    if (IncludePlaylist(job, title, minDuration, mainPlaylist, maxPlaylist))
    {
      items.Add(GetFileItem(url, title,
                            title.playlist == static_cast<unsigned int>(mainPlaylist)
                                ? g_localizeStrings.Get(25004) /* Main Title */
                                : g_localizeStrings.Get(25005) /* Title */));
    }
  }

  return !items.IsEmpty();
}

void ProcessPlaylist(PlaylistMap& playlists, PlaylistInfo& titleInfo, ClipMap& clips)
{
  const unsigned int playlist{titleInfo.playlist};

  // Save playlist
  PlaylistInfo info;
  info.playlist = playlist;

  // Save playlist duration and chapters
  info.duration = titleInfo.duration;
  info.chapters = titleInfo.chapters;

  // Get clips
  for (const auto& clip : titleInfo.clips)
  {
    // Add clip to playlist
    info.clips.emplace_back(clip);

    // Add/extend clip information
    const auto& it = clips.find(clip);
    if (it == clips.end())
    {
      // First reference to clip
      ClipInfo clipInfo;
      clipInfo.duration = titleInfo.clipDuration[clip];
      clipInfo.playlists.emplace_back(playlist);
      clips[clip] = clipInfo;
    }
    else
    {
      // Additional reference to clip, add this playlist
      it->second.playlists.emplace_back(playlist);
    }
  }

  // Get languages
  const std::string langs{fmt::format(
      "{}", fmt::join(titleInfo.audioStreams | std::views::transform(&DiscStreamInfo::lang), ","))};
  info.languages = langs;
  titleInfo.languages = langs;

  playlists[playlist] = info;
}

void GetPlaylistsInformation(const CURL& url,
                             const std::string& realPath,
                             int flags,
                             ClipMap& clips,
                             PlaylistMap& playlists,
                             std::map<unsigned int, ClipInformation>& clipCache)
{
  // Check cache
  const std::string& path{url.GetHostName()};
  if (CServiceBroker::GetBlurayDiscCache()->GetMaps(path, playlists, clips))
  {
    CLog::LogF(LOGDEBUG, "Playlist information for {} retrieved from cache", path);
    return;
  }

  // Get all titles on disc
  // Sort by playlist for grouping later
  CFileItemList allTitles;
  GetPlaylists(url, realPath, flags, GetTitles::GET_TITLES_EPISODES, allTitles,
               SortTitles::SORT_TITLES_EPISODE, clipCache);

  // Get information on all playlists
  // Including relationship between clips and playlists
  // List all playlists
  CLog::LogF(LOGDEBUG, "*** Playlist information ***");

  for (const auto& title : allTitles)
  {
    const int playlist{title->GetProperty("bluray_playlist").asInteger32(0)};
    PlaylistInfo titleInfo;
    if (!GetPlaylistInfoFromDisc(url, realPath, playlist, titleInfo, clipCache))
    {
      CLog::LogF(LOGDEBUG, "Unable to get playlist {}", playlist);
      continue;
    }

    ProcessPlaylist(playlists, titleInfo, clips);

    CLog::LogF(LOGDEBUG, "Playlist {}, Duration {}, Langs {}, Clips {} ", playlist,
               title->GetVideoInfoTag()->GetDuration(), titleInfo.languages,
               fmt::join(titleInfo.clips, ","));
  }

  // List clip info (automatically sorted as map)
  for (const auto& c : clips)
  {
    const auto& [clip, clipInformation] = c;
    CLog::LogF(LOGDEBUG, "Clip {:d} duration {:d} - playlists {}", clip,
               clipInformation.duration.count() / 1000, fmt::join(clipInformation.playlists, ","));
  }

  CLog::LogF(LOGDEBUG, "*** Playlist information End ***");

  // Cache
  CServiceBroker::GetBlurayDiscCache()->SetMaps(path, playlists, clips);
  CLog::LogF(LOGDEBUG, "Playlist information for {} cached", path);
}
} // namespace

CBlurayDirectory::CBlurayDirectory()
{
  m_clipCache.clear();
}

CBlurayDirectory::~CBlurayDirectory()
{
  Dispose();
}

void CBlurayDirectory::Dispose()
{
  if (m_bd)
  {
    bd_close(m_bd);
    m_bd = nullptr;
  }
}

bool CBlurayDirectory::Resolve(CFileItem& item) const
{
  const std::string originalPath{item.GetDynPath()};
  if (CURL::Decode(originalPath).find("removable://") != std::string::npos)
  {
    std::string newPath;
    if (URIUtils::GetExtension(originalPath) == ".mpls")
    {
      // Playlist (.mpls) so return bluray:// path with removable:// resolved to physical disc
      const CURL pathUrl{originalPath};
      newPath = URIUtils::GetBlurayPlaylistPath(item.GetPath());
      newPath = URIUtils::AddFileToFolder(newPath, pathUrl.GetFileNameWithoutPath());
    }
    else
    {
      // Not a playlist resolve removable:// to physical disc
      newPath = item.GetPath();
    }

    item.SetDynPath(newPath);
    CLog::LogF(LOGDEBUG, "Resolved removable bluray path from {} to {}", originalPath, newPath);
  }
  return true;
}

std::string CBlurayDirectory::GetBasePath(const CURL& url)
{
  if (!url.IsProtocol("bluray"))
    return {};

  const CURL url2(url.GetHostName()); // strip bluray://
  if (url2.IsProtocol("udf")) // ISO
    return URIUtils::GetDirectory(url2.GetHostName()); // strip udf://
  return url2.Get(); // BDMV
}

std::string CBlurayDirectory::GetBlurayTitle() const
{
  return GetDiscInfoString(DiscInfo::TITLE);
}

std::string CBlurayDirectory::GetBlurayID() const
{
  return GetDiscInfoString(DiscInfo::ID);
}

std::string CBlurayDirectory::GetDiscInfoString(DiscInfo info) const
{
  if (!m_blurayInitialized)
    return "";

  const BLURAY_DISC_INFO* discInfo{GetDiscInfo()};
  if (!discInfo || !discInfo->bluray_detected)
    return {};

  switch (info)
  {
    case DiscInfo::TITLE:
    {
      std::string title;

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      title = discInfo->disc_name ? discInfo->disc_name : "";
#endif

      return title;
    }
    case DiscInfo::ID:
    {
      std::string id;

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      id = discInfo->udf_volume_id ? discInfo->udf_volume_id : "";
      if (id.empty())
        id = CUtil::GetHexString(discInfo->disc_id, 10);
#endif

      return id;
    }
  }

  return "";
}

bool CBlurayDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  Dispose();
  m_url = url;
  std::string root{m_url.GetHostName()};
  std::string file{m_url.GetFileName()};
  URIUtils::RemoveSlashAtEnd(file);
  URIUtils::RemoveSlashAtEnd(root);

  if (!InitializeBluray(root))
    return false;

  // /root                              - get main (length >70% longest) playlists
  // /root/titles                       - get all playlists
  // /root/episode/<season>/<episode>   - get playlists that correspond with S<season>E<episode>
  //                                      if none found then return all playlists
  // /root/episode/all                  - get all episodes
  if (file == "root")
  {
    GetPlaylists(url, m_realPath, m_flags, GetTitles::GET_TITLES_MAIN, items,
                 SortTitles::SORT_TITLES_MOVIE, m_clipCache);
    AddOptionsAndSort(m_url, items, m_blurayMenuSupport);
    return (items.Size() > 2);
  }

  if (file == "root/titles")
    return GetPlaylists(url, m_realPath, m_flags, GetTitles::GET_TITLES_ALL, items,
                        SortTitles::SORT_TITLES_MOVIE, m_clipCache);

  if (StringUtils::StartsWith(file, "root/episode"))
  {
    // Get episodes on disc
    const std::vector<CVideoInfoTag> episodesOnDisc{CDiscDirectoryHelper::GetEpisodesOnDisc(url)};

    int season{-1};
    int episode{-1};
    int episodeIndex{-1};
    if (file != "root/episode/all")
    {
      // Get desired episode from path
      CRegExp regex{true, CRegExp::autoUtf8, R"((root\/episode\/)(\d{1,4})\/(\d{1,4}))"};
      if (regex.RegFind(file) == -1)
        return false; // Invalid episode path
      season = std::stoi(regex.GetMatch(2));
      episode = std::stoi(regex.GetMatch(3));

      // Check desired episode is on disc
      const auto& it{
          std::ranges::find_if(episodesOnDisc, [&season, &episode](const CVideoInfoTag& e)
                               { return e.m_iSeason == season && e.m_iEpisode == episode; })};
      if (it == episodesOnDisc.end())
        return false; // Episode not on disc
      episodeIndex = static_cast<int>(std::distance(episodesOnDisc.begin(), it));
    }

    // Get playlist, clip and language information
    ClipMap clips;
    PlaylistMap playlists;
    GetPlaylistsInformation(url, m_realPath, m_flags, clips, playlists, m_clipCache);

    // Get episode playlists
    CDiscDirectoryHelper helper;
    helper.GetEpisodePlaylists(m_url, items, episodeIndex, episodesOnDisc, clips, playlists);

    // Heuristics failed so return all playlists
    if (items.IsEmpty())
      GetPlaylists(url, m_realPath, m_flags, GetTitles::GET_TITLES_EPISODES, items,
                   SortTitles::SORT_TITLES_EPISODE, m_clipCache);

    // Add all titles and menu options
    AddOptionsAndSort(m_url, items, m_blurayMenuSupport);

    return (items.Size() > 2);
  }

  const CURL url2{CURL(URIUtils::GetDiscUnderlyingFile(m_url))};
  CDirectory::CHints hints;
  hints.flags = m_flags;
  if (!CDirectory::GetDirectory(url2, items, hints))
    return false;

  // Found items will have underlying protocol (eg. udf:// or smb://)
  // in path so add back bluray://
  // (so properly recognised in cache as bluray:// files for CFile:Exists() etc..)
  CURL url3{m_url};
  const std::string baseFileName{url3.GetFileName()};
  for (const auto& item : items)
  {
    std::string path{item->GetPath()};
    URIUtils::RemoveSlashAtEnd(path);
    std::string fileName{URIUtils::GetFileName(path)};

    if (URIUtils::HasSlashAtEnd(item->GetPath()))
      URIUtils::AddSlashAtEnd(fileName);

    url3.SetFileName(URIUtils::AddFileToFolder(baseFileName, fileName));
    item->SetPath(url3.Get());
  }

  url3.SetFileName("menu");
  const std::shared_ptr<CFileItem> item{std::make_shared<CFileItem>()};
  item->SetPath(url3.Get());
  items.Add(item);

  return true;
}

bool CBlurayDirectory::InitializeBluray(const std::string& root)
{
  bd_set_debug_handler(CBlurayCallback::bluray_logger);
  bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  m_bd = bd_init();

  if (!m_bd)
  {
    CLog::LogF(LOGERROR, "Failed to initialize libbluray");
    return false;
  }

  std::string langCode;
  g_LangCodeExpander.ConvertToISO6392T(g_langInfo.GetDVDMenuLanguage(), langCode);
  bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_MENU_LANG, langCode.c_str());

  m_realPath = root;

  if (const auto fileHandler{CDirectoryFactory::Create(CURL{root})}; fileHandler)
    m_realPath = fileHandler->ResolveMountPoint(root);

  if (!bd_open_files(m_bd, &m_realPath, CBlurayCallback::dir_open, CBlurayCallback::file_open))
  {
    CLog::LogF(LOGERROR, "Failed to open {}", CURL::GetRedacted(root));
    return false;
  }
  m_blurayInitialized = true;

  const BLURAY_DISC_INFO* discInfo{GetDiscInfo()};
  m_blurayMenuSupport = discInfo && !discInfo->no_menu_support;

  return true;
}

const BLURAY_DISC_INFO* CBlurayDirectory::GetDiscInfo() const
{
  return bd_get_disc_info(m_bd);
}

} // namespace XFILE
