/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDVideoCodecWebCodecsBitstream.h"

#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace
{
constexpr int CODEC_STRING_BUFFER_SIZE = 24;
constexpr int H264_AVCC_MIN_EXTRADATA_SIZE = 7;
constexpr uint8_t H264_AVCC_CONFIG_VERSION = 1;
constexpr int H264_AVCC_PROFILE_OFFSET = 1;
constexpr int H264_AVCC_COMPAT_OFFSET = 2;
constexpr int H264_AVCC_LEVEL_OFFSET = 3;
constexpr int H264_AVCC_LENGTH_SIZE_OFFSET = 4;
constexpr uint8_t H264_AVCC_LENGTH_SIZE_MASK = 0x03;
constexpr int H264_PROFILE_IDC_MASK = 0xFF;
constexpr int H264_MAX_LEVEL_IDC = 0xFF;
constexpr uint8_t H264_CONSTRAINT_SET1_FLAG = 0x40;

constexpr uint8_t H264_NAL_TYPE_MASK = 0x1F;
constexpr uint8_t H264_NAL_TYPE_IDR = 5;

constexpr int HEVC_HVCC_MIN_EXTRADATA_SIZE = 23;
constexpr uint8_t HEVC_HVCC_CONFIG_VERSION = 1;
constexpr int HEVC_HVCC_PROFILE_OFFSET = 1;
constexpr int HEVC_HVCC_COMPAT_FLAGS_OFFSET = 2;
constexpr int HEVC_HVCC_CONSTRAINT_FLAGS_OFFSET = 6;
constexpr int HEVC_HVCC_CONSTRAINT_FLAGS_SIZE = 6;
constexpr int HEVC_HVCC_LEVEL_OFFSET = 12;
constexpr int HEVC_HVCC_LENGTH_SIZE_OFFSET = 21;
constexpr uint8_t HEVC_HVCC_LENGTH_SIZE_MASK = 0x03;
constexpr int HEVC_MAX_PROFILE_IDC = 31;
constexpr int HEVC_DEFAULT_LEVEL_IDC = 120;

constexpr uint8_t HEVC_NAL_TYPE_MASK = 0x3F;
constexpr uint8_t HEVC_NAL_TYPE_BLA_W_LP = 16;
constexpr uint8_t HEVC_NAL_TYPE_RSV_IRAP_VCL23 = 23;

constexpr int AV1_AV1C_MIN_EXTRADATA_SIZE = 4;
constexpr uint8_t AV1_AV1C_MARKER_AND_VERSION = 0x81;
constexpr int AV1_OBU_SEQUENCE_HEADER = 1;
constexpr int AV1_OBU_FRAME_HEADER = 3;
constexpr int AV1_OBU_FRAME = 6;
constexpr int AV1_FRAME_TYPE_KEY = 0;
constexpr int AV1_MAX_LEVEL_IDX = 31;
constexpr int AV1_DEFAULT_LEVEL_IDX = 8;

constexpr uint8_t ANNEXB_START_CODE_BYTE = 0x01;

constexpr int VP9_FRAME_MARKER = 2;
constexpr int VP9_PROFILE_WITH_RESERVED_BIT = 3;
constexpr int VP9_FRAME_TYPE_KEY = 0;

// An AVCDecoderConfigurationRecord is only complete once numOfSequenceParameterSets,
// its last fixed field, is present at byte 6.
bool HasAVCCExtradata(const CDVDStreamInfo& hints)
{
  return hints.extradata.GetSize() >= H264_AVCC_MIN_EXTRADATA_SIZE &&
         hints.extradata.GetData()[0] == H264_AVCC_CONFIG_VERSION;
}

bool HasAnnexBExtradata(const CDVDStreamInfo& hints)
{
  const uint8_t* data = hints.extradata.GetData();
  const size_t size = hints.extradata.GetSize();
  return (size > 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == ANNEXB_START_CODE_BYTE) ||
         (size > 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 &&
          data[3] == ANNEXB_START_CODE_BYTE);
}

// An HEVCDecoderConfigurationRecord has 22 fixed bytes followed by numOfArrays.
bool HasHVCCExtradata(const CDVDStreamInfo& hints)
{
  return hints.extradata.GetSize() >= HEVC_HVCC_MIN_EXTRADATA_SIZE &&
         hints.extradata.GetData()[0] == HEVC_HVCC_CONFIG_VERSION;
}

// An AV1CodecConfigurationRecord starts with marker = 1 and version = 1.
bool HasAV1CExtradata(const CDVDStreamInfo& hints)
{
  return hints.extradata.GetSize() >= AV1_AV1C_MIN_EXTRADATA_SIZE &&
         hints.extradata.GetData()[0] == AV1_AV1C_MARKER_AND_VERSION;
}

std::string BuildH264CodecString(const CDVDStreamInfo& hints)
{
  const auto& extradata = hints.extradata;
  if (HasAVCCExtradata(hints))
  {
    const uint8_t profile = extradata.GetData()[H264_AVCC_PROFILE_OFFSET];
    const uint8_t compat = extradata.GetData()[H264_AVCC_COMPAT_OFFSET];
    const uint8_t level = extradata.GetData()[H264_AVCC_LEVEL_OFFSET];

    char buffer[CODEC_STRING_BUFFER_SIZE];
    std::snprintf(buffer, sizeof(buffer), "avc1.%02X%02X%02X", profile, compat, level);
    return buffer;
  }

  // Parameter sets travel in-band; describe the stream from its hints.
  if (hints.profile > 0 && hints.level > 0 && hints.level <= H264_MAX_LEVEL_IDC)
  {
    const int compat =
        hints.profile == AV_PROFILE_H264_CONSTRAINED_BASELINE ? H264_CONSTRAINT_SET1_FLAG : 0;
    char buffer[CODEC_STRING_BUFFER_SIZE];
    std::snprintf(buffer, sizeof(buffer), "avc1.%02X%02X%02X",
                  hints.profile & H264_PROFILE_IDC_MASK, compat, hints.level);
    return buffer;
  }

  return "avc1.42E01E";
}

// ISO/IEC 14496-15 Annex E: hvc1.<profile space + idc>.<compatibility flags,
// bit-reversed hex>.<tier><level>.<constraint bytes, trailing zeros dropped>.
std::string BuildHEVCCodecString(const CDVDStreamInfo& hints)
{
  if (!HasHVCCExtradata(hints))
  {
    // Parameter sets travel in-band. A Main stream is also Main 10 compatible.
    const int profile =
        hints.profile > 0 ? std::min(hints.profile, HEVC_MAX_PROFILE_IDC) : AV_PROFILE_HEVC_MAIN;
    const unsigned int compat =
        (1u << profile) | (profile == AV_PROFILE_HEVC_MAIN ? 1u << AV_PROFILE_HEVC_MAIN_10 : 0u);
    const int level = hints.level > 0 ? hints.level : HEVC_DEFAULT_LEVEL_IDC;
    return StringUtils::Format("hev1.{}.{:X}.L{}.B0", profile, compat, level);
  }

  const uint8_t* record = hints.extradata.GetData();
  const uint8_t profileByte = record[HEVC_HVCC_PROFILE_OFFSET];
  const int profileSpace = profileByte >> 6;
  const bool highTier = (profileByte & 0x20) != 0;
  const int profileIdc = profileByte & 0x1F;

  uint32_t compat = 0;
  for (int i = 0; i < 4; ++i)
    compat = (compat << 8) | record[HEVC_HVCC_COMPAT_FLAGS_OFFSET + i];
  uint32_t reversedCompat = 0;
  for (int i = 0; i < 32; ++i, compat >>= 1)
    reversedCompat = (reversedCompat << 1) | (compat & 1u);

  std::string codec = "hvc1.";
  if (profileSpace > 0)
    codec += static_cast<char>('A' + profileSpace - 1);
  codec += StringUtils::Format("{}.{:X}.{}{}", profileIdc, reversedCompat, highTier ? 'H' : 'L',
                               static_cast<int>(record[HEVC_HVCC_LEVEL_OFFSET]));

  int constraintBytes = HEVC_HVCC_CONSTRAINT_FLAGS_SIZE;
  while (constraintBytes > 0 &&
         record[HEVC_HVCC_CONSTRAINT_FLAGS_OFFSET + constraintBytes - 1] == 0)
    --constraintBytes;
  for (int i = 0; i < constraintBytes; ++i)
    codec += StringUtils::Format(".{:X}",
                                 static_cast<int>(record[HEVC_HVCC_CONSTRAINT_FLAGS_OFFSET + i]));
  return codec;
}

// AV1 ISOBMFF binding: av01.<profile>.<level><tier>.<bit depth>. The optional
// colour fields default to the 4:2:0 stream types WebCodecs decodes.
std::string BuildAV1CodecString(const CDVDStreamInfo& hints)
{
  int profile = std::clamp(hints.profile, 0, static_cast<int>(AV_PROFILE_AV1_PROFESSIONAL));
  int levelIdx =
      hints.level >= 0 && hints.level <= AV1_MAX_LEVEL_IDX ? hints.level : AV1_DEFAULT_LEVEL_IDX;
  bool highTier = false;
  int bitDepth = hints.bitdepth > 0 ? hints.bitdepth : 8;

  if (HasAV1CExtradata(hints))
  {
    const uint8_t* record = hints.extradata.GetData();
    profile = record[1] >> 5;
    levelIdx = record[1] & 0x1F;
    highTier = (record[2] & 0x80) != 0;
    const bool highBitdepth = (record[2] & 0x40) != 0;
    const bool twelveBit = (record[2] & 0x20) != 0;
    bitDepth = highBitdepth ? (twelveBit ? 12 : 10) : 8;
  }

  return StringUtils::Format("av01.{}.{:02}{}.{:02}", profile, levelIdx, highTier ? 'H' : 'M',
                             bitDepth);
}

int ClampTwoDigitCodecValue(int value, int fallback)
{
  if (value < 0)
    value = fallback;
  return std::clamp(value, 0, 99);
}

int VP9ProfileFromHints(const CDVDStreamInfo& hints)
{
  switch (hints.profile)
  {
    case AV_PROFILE_VP9_0:
      return 0;
    case AV_PROFILE_VP9_1:
      return 1;
    case AV_PROFILE_VP9_2:
      return 2;
    case AV_PROFILE_VP9_3:
      return 3;
    default:
      return hints.bitdepth > 8 ? 2 : 0;
  }
}

std::string BuildVP9CodecString(const CDVDStreamInfo& hints)
{
  const int profile = ClampTwoDigitCodecValue(VP9ProfileFromHints(hints), 0);
  const int level = ClampTwoDigitCodecValue(hints.level, 10);
  const int bitDepth = ClampTwoDigitCodecValue(hints.bitdepth > 0 ? hints.bitdepth : 8, 8);

  char buffer[CODEC_STRING_BUFFER_SIZE];
  std::snprintf(buffer, sizeof(buffer), "vp09.%02d.%02d.%02d", profile, level, bitDepth);
  return buffer;
}

using NalPredicate = bool (*)(uint8_t firstHeaderByte);

bool H264NalIsIDR(uint8_t firstHeaderByte)
{
  return (firstHeaderByte & H264_NAL_TYPE_MASK) == H264_NAL_TYPE_IDR;
}

bool HEVCNalIsIRAP(uint8_t firstHeaderByte)
{
  const uint8_t type = (firstHeaderByte >> 1) & HEVC_NAL_TYPE_MASK;
  return type >= HEVC_NAL_TYPE_BLA_W_LP && type <= HEVC_NAL_TYPE_RSV_IRAP_VCL23;
}

// Walks the NAL units of a sample, length-prefixed when nalLengthSize > 0 and
// Annex B otherwise, and returns true if any satisfies isKeyNal.
bool SampleHasKeyNalUnit(const uint8_t* data, int size, int nalLengthSize, NalPredicate isKeyNal)
{
  if (!data || size <= 0)
    return false;

  if (nalLengthSize > 0)
  {
    int offset = 0;
    while (offset + nalLengthSize <= size)
    {
      uint32_t nalSize = 0;
      for (int i = 0; i < nalLengthSize; ++i)
        nalSize = (nalSize << 8) | data[offset + i];
      offset += nalLengthSize;
      if (nalSize == 0 || nalSize > static_cast<uint32_t>(size - offset))
        break;
      if (isKeyNal(data[offset]))
        return true;
      offset += static_cast<int>(nalSize);
    }
    return false;
  }

  for (int i = 0; i + 3 < size; ++i)
  {
    const bool threeByteStart =
        data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == ANNEXB_START_CODE_BYTE;
    const bool fourByteStart = data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 &&
                               data[i + 3] == ANNEXB_START_CODE_BYTE;
    if (!threeByteStart && !fourByteStart)
      continue;

    const int nalStart = threeByteStart ? i + 3 : i + 4;
    if (nalStart < size && isKeyNal(data[nalStart]))
      return true;
  }
  return false;
}

// MSB-first bit reader; reads past the end yield zero bits.
class CBitReader
{
public:
  CBitReader(const uint8_t* data, int size) : m_data(data), m_bits(size * 8) {}

  uint32_t Read(int count)
  {
    uint32_t value = 0;
    for (int i = 0; i < count; ++i)
    {
      const uint32_t bit = m_pos < m_bits ? (m_data[m_pos >> 3] >> (7 - (m_pos & 7))) & 1u : 0u;
      ++m_pos;
      value = (value << 1) | bit;
    }
    return value;
  }

private:
  const uint8_t* m_data;
  int m_bits;
  int m_pos{0};
};

bool ReadLeb128(const uint8_t* data, int size, int& offset, uint32_t& value)
{
  uint64_t result = 0;
  for (int i = 0; i < 8; ++i)
  {
    if (offset >= size)
      return false;
    const uint8_t byte = data[offset++];
    result |= static_cast<uint64_t>(byte & 0x7F) << (7 * i);
    if ((byte & 0x80) == 0)
    {
      if (result > UINT32_MAX)
        return false;
      value = static_cast<uint32_t>(result);
      return true;
    }
  }
  return false;
}

// A key temporal unit starts with a key frame. frame_type follows
// show_existing_frame in the uncompressed header unless the sequence header
// sets reduced_still_picture_header, which makes every frame a key frame. A
// sequence header in `data` updates reducedStillPictureHeader.
bool AV1TemporalUnitIsKeyFrame(const uint8_t* data, int size, bool& reducedStillPictureHeader)
{
  if (!data || size <= 0)
    return false;

  int offset = 0;
  while (offset < size)
  {
    const uint8_t header = data[offset++];
    if ((header & 0x80) != 0)
      return false;
    const int obuType = (header >> 3) & 0x0F;
    const bool hasExtension = (header & 0x04) != 0;
    const bool hasSize = (header & 0x02) != 0;
    if (hasExtension && offset++ >= size)
      return false;

    uint32_t obuSize = static_cast<uint32_t>(size - offset);
    if (hasSize && !ReadLeb128(data, size, offset, obuSize))
      return false;
    if (obuSize > static_cast<uint32_t>(size - offset))
      return false;

    CBitReader bits(data + offset, static_cast<int>(obuSize));
    if (obuType == AV1_OBU_SEQUENCE_HEADER)
    {
      bits.Read(3); // seq_profile
      bits.Read(1); // still_picture
      reducedStillPictureHeader = bits.Read(1) != 0;
    }
    else if (obuType == AV1_OBU_FRAME_HEADER || obuType == AV1_OBU_FRAME)
    {
      if (reducedStillPictureHeader)
        return true;
      if (bits.Read(1) != 0) // show_existing_frame
        return false;
      return bits.Read(2) == AV1_FRAME_TYPE_KEY;
    }
    offset += static_cast<int>(obuSize);
  }
  return false;
}

bool VP8SampleIsKeyFrame(const uint8_t* data, int size)
{
  return data && size > 0 && (data[0] & 0x01) == 0;
}

bool VP9SampleIsKeyFrame(const uint8_t* data, int size)
{
  if (!data || size <= 0)
    return false;

  // The VP9 uncompressed header is a plain f(n) bit stream, read MSB first, so the
  // first field starts at bit 7. Everything needed to classify the frame fits in the
  // first byte.
  const uint8_t firstByte = data[0];
  int bit = 7;
  const auto readBit = [&firstByte, &bit]() { return (firstByte >> bit--) & 0x01; };

  const int markerHigh = readBit();
  const int markerLow = readBit();
  if (((markerHigh << 1) | markerLow) != VP9_FRAME_MARKER)
    return false;

  const int profileLowBit = readBit();
  const int profileHighBit = readBit();
  if (((profileHighBit << 1) | profileLowBit) == VP9_PROFILE_WITH_RESERVED_BIT)
    readBit(); // reserved_zero

  const bool showExistingFrame = readBit() != 0;
  if (showExistingFrame)
    return false;

  return readBit() == VP9_FRAME_TYPE_KEY;
}
} // namespace

namespace WEBCODECS
{
// Only the AVC and HEVC configuration records are passed as description; with
// them the packets are length-prefixed, without them they are Annex B with the
// parameter sets in-band. VP8, VP9 and AV1 carry everything in the bitstream.
std::optional<CodecConfiguration> GetCodecConfiguration(const CDVDStreamInfo& hints)
{
  CodecConfiguration config;
  switch (hints.codec)
  {
    case AV_CODEC_ID_H264:
      config.codecString = BuildH264CodecString(hints);
      config.hasDescription = HasAVCCExtradata(hints);
      config.annexB = !config.hasDescription;
      config.prependExtradata = config.annexB && HasAnnexBExtradata(hints);
      if (config.hasDescription)
        config.nalLengthSize =
            (hints.extradata.GetData()[H264_AVCC_LENGTH_SIZE_OFFSET] & H264_AVCC_LENGTH_SIZE_MASK) +
            1;
      return config;
    case AV_CODEC_ID_HEVC:
      config.codecString = BuildHEVCCodecString(hints);
      config.hasDescription = HasHVCCExtradata(hints);
      config.annexB = !config.hasDescription;
      config.prependExtradata = config.annexB && HasAnnexBExtradata(hints);
      if (config.hasDescription)
        config.nalLengthSize =
            (hints.extradata.GetData()[HEVC_HVCC_LENGTH_SIZE_OFFSET] & HEVC_HVCC_LENGTH_SIZE_MASK) +
            1;
      return config;
    case AV_CODEC_ID_VP8:
      config.codecString = "vp8";
      return config;
    case AV_CODEC_ID_VP9:
      config.codecString = BuildVP9CodecString(hints);
      return config;
    case AV_CODEC_ID_AV1:
      config.codecString = BuildAV1CodecString(hints);
      return config;
    default:
      return std::nullopt;
  }
}

bool IsChroma420Profile(const CDVDStreamInfo& hints)
{
  switch (hints.codec)
  {
    case AV_CODEC_ID_H264:
      switch (hints.profile & H264_PROFILE_IDC_MASK)
      {
        case AV_PROFILE_H264_CAVLC_444:
        case AV_PROFILE_H264_HIGH_422:
        case AV_PROFILE_H264_HIGH_444:
        case AV_PROFILE_H264_HIGH_444_PREDICTIVE:
          return false;
        default:
          return true;
      }
    case AV_CODEC_ID_HEVC:
      return hints.profile != AV_PROFILE_HEVC_REXT && hints.profile != AV_PROFILE_HEVC_SCC;
    case AV_CODEC_ID_VP9:
      return hints.profile != AV_PROFILE_VP9_1 && hints.profile != AV_PROFILE_VP9_3;
    case AV_CODEC_ID_AV1:
      return hints.profile != AV_PROFILE_AV1_HIGH && hints.profile != AV_PROFILE_AV1_PROFESSIONAL;
    default:
      return true;
  }
}

bool PacketIsKeyFrame(const CDVDStreamInfo& hints, const DemuxPacket& packet, int nalLengthSize)
{
  switch (hints.codec)
  {
    case AV_CODEC_ID_H264:
      return SampleHasKeyNalUnit(packet.pData, packet.iSize, nalLengthSize, H264NalIsIDR);
    case AV_CODEC_ID_HEVC:
      return SampleHasKeyNalUnit(packet.pData, packet.iSize, nalLengthSize, HEVCNalIsIRAP);
    case AV_CODEC_ID_VP8:
      return VP8SampleIsKeyFrame(packet.pData, packet.iSize);
    case AV_CODEC_ID_VP9:
      return VP9SampleIsKeyFrame(packet.pData, packet.iSize);
    case AV_CODEC_ID_AV1:
    {
      // The sequence header may travel only in the av1C configOBUs.
      bool reducedStillPictureHeader = false;
      if (HasAV1CExtradata(hints))
        AV1TemporalUnitIsKeyFrame(hints.extradata.GetData() + AV1_AV1C_MIN_EXTRADATA_SIZE,
                                  static_cast<int>(hints.extradata.GetSize()) -
                                      AV1_AV1C_MIN_EXTRADATA_SIZE,
                                  reducedStillPictureHeader);
      return AV1TemporalUnitIsKeyFrame(packet.pData, packet.iSize, reducedStillPictureHeader);
    }
    default:
      return false;
  }
}
} // namespace WEBCODECS
