/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DolbyVisionRpu.h"

#include "utils/BitReader.h"
#include "utils/Crc32.h"

#include <algorithm>
#include <bit>
#include <cstring>

#ifdef HAVE_LIBDOVI
#include <libdovi/rpu_parser.h>
#endif

namespace
{
constexpr uint8_t NAL_HEADER[] = {0x7C, 0x01};

#ifdef HAVE_LIBDOVI
constexpr uint8_t RPU_PREFIX = 0x19;

bool ReadLeb128(const uint8_t* data, size_t size, size_t& pos, uint64_t& value)
{
  value = 0;
  for (int i = 0; i < 8 && pos < size; ++i)
  {
    const uint8_t byte = data[pos++];
    value |= static_cast<uint64_t>(byte & 0x7F) << (i * 7);
    if (!(byte & 0x80))
      return true;
  }
  return false;
}

// The T.35 payload carries the RPU in an EMDF container, which libdovi unwraps.
std::vector<uint8_t> ParseAv1T35(const uint8_t* data, size_t size)
{
  DoviRpuOpaque* parsed = dovi_parse_itu_t35_dovi_metadata_obu(data, size);
  const DoviData* written = dovi_rpu_get_error(parsed) ? nullptr : dovi_write_rpu(parsed);
  std::vector<uint8_t> nal;
  if (written && written->len)
  {
    nal.assign(std::begin(NAL_HEADER), std::end(NAL_HEADER));
    if (written->data[0] != RPU_PREFIX)
      nal.push_back(RPU_PREFIX);
    nal.insert(nal.end(), written->data, written->data + written->len);
  }
  if (written)
    dovi_data_free(written);
  dovi_rpu_free(parsed);
  return nal;
}
#endif
} // namespace

namespace KODI::UTILS::DOLBYVISION
{
std::vector<uint8_t> HevcRpuToNal(const uint8_t* data, size_t size)
{
  std::vector<uint8_t> nal(std::begin(NAL_HEADER), std::end(NAL_HEADER));
  nal.reserve(size + sizeof(NAL_HEADER));
  int zeros = 0;
  for (size_t i = 0; i < size; ++i)
  {
    if (zeros >= 2 && data[i] == 0x03)
    {
      zeros = 0;
      continue;
    }
    zeros = data[i] == 0 ? zeros + 1 : 0;
    nal.push_back(data[i]);
  }
  return nal;
}

std::vector<uint8_t> ExtractAv1Rpu([[maybe_unused]] const uint8_t* data,
                                   [[maybe_unused]] size_t size)
{
#ifdef HAVE_LIBDOVI
  constexpr int OBU_METADATA = 5;
  constexpr uint64_t METADATA_TYPE_ITUT_T35 = 4;
  // country code US, provider code Dolby, provider oriented code
  constexpr uint8_t dolbyT35[] = {0xB5, 0x00, 0x3B, 0x00, 0x00, 0x08, 0x00};

  size_t pos = 0;
  while (pos < size)
  {
    const uint8_t header = data[pos++];
    if (header & 0x04) // obu_extension_flag
      ++pos;
    uint64_t obuSize = pos < size ? size - pos : 0;
    if ((header & 0x02) && !ReadLeb128(data, size, pos, obuSize)) // obu_has_size_field
      return {};
    if (pos > size || obuSize > size - pos)
      return {};

    const uint8_t* obu = data + pos;
    pos += obuSize;
    if (((header >> 3) & 0x0F) != OBU_METADATA)
      continue;

    size_t offset = 0;
    uint64_t metadataType = 0;
    if (!ReadLeb128(obu, obuSize, offset, metadataType) || metadataType != METADATA_TYPE_ITUT_T35 ||
        obuSize - offset <= sizeof(dolbyT35) ||
        std::memcmp(obu + offset, dolbyT35, sizeof(dolbyT35)) != 0)
      continue;

    std::vector<uint8_t> nal = ParseAv1T35(obu + offset, obuSize - offset);
    if (!nal.empty())
      return nal;
  }
#endif
  return {};
}
} // namespace KODI::UTILS::DOLBYVISION

namespace
{
// From libavcodec/dovi_rpu.h and libavutil/dovi_meta.h
constexpr uint32_t COEF_FIXED = 0;
constexpr uint32_t COEF_FLOAT = 1;
constexpr uint32_t MAPPING_POLYNOMIAL = 0;
constexpr uint32_t NLQ_LINEAR_DEADZONE = 0;
constexpr uint32_t MAX_DM_ID = 15;
constexpr uint32_t MAX_PIECES = 8;
constexpr int MAX_STATIC_BLOCKS = 7;
constexpr int MAX_DYNAMIC_BLOCKS = 25;

// The largest value FFmpeg's get_ue_golomb_31() reads as the code it is
constexpr uint32_t GOLOMB_31_MAX = 30;

// A quantisation that is the identity: vdr_in_max of 1 at the usual coef_log2_denom of 23
constexpr uint64_t IDENTITY_VDR_IN_MAX = 8388608;

// ff_dovi_guess_profile_hevc()
uint8_t GuessProfile(uint32_t vdrRpuProfile,
                     bool blFullRange,
                     bool elSpatialResampling,
                     bool disableResidual,
                     uint32_t vdrBitDepth)
{
  if (vdrRpuProfile == 0)
    return blFullRange ? 5 : 0;
  if (vdrRpuProfile == 1)
  {
    if (elSpatialResampling && !disableResidual)
      return vdrBitDepth == 12 ? 7 : 4;
    return 8;
  }
  return 0;
}

// ff_dovi_rpu_extension_is_static()
bool IsStaticLevel(uint32_t level)
{
  return level == 6 || level == 10 || level == 32 || level == 254 || level == 255;
}

// get_se_coef()
void SkipSignedCoefficient(CBitReader& br, uint32_t type, unsigned int log2Denom)
{
  if (type == COEF_FIXED)
  {
    br.ReadUE();
    br.SkipBits(log2Denom);
  }
  else
    br.SkipBits(32);
}

// get_ue_coef()
std::optional<uint64_t> ReadUnsignedCoefficient(CBitReader& br,
                                                uint32_t type,
                                                unsigned int log2Denom)
{
  if (type == COEF_FIXED)
  {
    const uint64_t integer{br.ReadUE()};
    return (integer << log2Denom) | br.ReadBits(log2Denom);
  }
  const float value{std::bit_cast<float>(br.ReadBits(32)) *
                    static_cast<float>(int64_t{1} << log2Denom)};
  // FFmpeg converts the float to an unsigned 64 bit integer, which is undefined out of its range
  if (!(value >= 0.0f && value < 18446744073709551616.0f))
    return {};
  return static_cast<uint64_t>(value);
}

// The size of the fields FFmpeg reads of an extension block (parse_ext_v1(), parse_ext_v2()),
// but for a level 2 block's ms_weight.
size_t ExtensionBlockBits(int version, uint32_t level, uint32_t length)
{
  if (version == 1)
  {
    switch (level)
    {
      case 1:
        return 36;
      case 2:
        return 72;
      case 4:
        return 24;
      case 5:
        return 52;
      case 6:
        return 64;
      case 255:
        return 48;
      default:
        return 0;
    }
  }

  switch (level)
  {
    case 3:
      return 36;
    case 8:
      return 80 + (length >= 12 ? 12 : 0) + (length >= 13 ? 12 : 0) + (length >= 19 ? 48 : 0) +
             (length >= 25 ? 48 : 0);
    case 9:
      return 8 + (length >= 17 ? 128 : 0);
    case 10:
      return 40 + (length >= 21 ? 128 : 0);
    case 11:
      return 32;
    case 254:
      return 16;
    default:
      return 0;
  }
}
} // namespace

namespace KODI::UTILS::DOLBYVISION
{
void CRpuReader::SetConfiguration(uint8_t profile, bool metadataCompression)
{
  m_profile = profile;
  m_metadataCompression = metadataCompression;
}

bool CRpuReader::ReadExtensionBlocks(CBitReader& br, int version, int& dynamicBlocks)
{
  // With compressed metadata, FFmpeg reads a static block into a dummy
  const bool compression{m_profile != 0 && m_metadataCompression};

  uint32_t blocks{br.ReadUE(GOLOMB_31_MAX)};
  br.ByteAlign();
  for (; blocks > 0 && !br.Error(); --blocks)
  {
    const uint32_t length{br.ReadUE(GOLOMB_31_MAX)};
    const uint32_t level{br.ReadBits(8)};
    const size_t start{br.Position()};

    if (IsStaticLevel(level))
    {
      if (!compression && m_staticBlocks++ >= MAX_STATIC_BLOCKS)
        return false;
    }
    else if (dynamicBlocks++ >= MAX_DYNAMIC_BLOCKS)
      return false;

    br.SkipBits(ExtensionBlockBits(version, level, length));
    if (version == 1 && level == 2)
    {
      // ms_weight, a signed 13 bit value
      const uint32_t msWeight{br.ReadBits(13)};
      if (msWeight >= 0x1000 && msWeight < 0x1FFF)
        return false;
    }

    const size_t read{br.Position() - start};
    if (read > size_t{length} * 8)
      return false;
    br.SkipBits(size_t{length} * 8 - read);
  }
  return !br.Error();
}

std::optional<EnhancementLayer> CRpuReader::Read(std::span<const uint8_t> rpu)
{
  constexpr uint8_t RPU_PREFIX = 0x19;
  if (rpu.size() < 5 || rpu[0] != RPU_PREFIX)
    return {};
  rpu = rpu.subspan(1);
  while (!rpu.empty() && rpu.back() == 0)
    rpu = rpu.first(rpu.size() - 1);

  CBitReader br(rpu);

  // rpu_type. FFmpeg ignores an RPU of another type, keeping what the one before set.
  if (br.ReadBits(6) != 2)
    return {};
  const uint32_t rpuFormat{br.ReadBits(11)};
  const uint32_t vdrRpuProfile{br.ReadBits(4)};
  br.SkipBits(4); // vdr_rpu_level

  // vdr_seq_info_present_flag. FFmpeg reads no RPU without the sequence information.
  if (br.ReadBits(1) == 0)
    return {};
  br.SkipBits(1); // chroma_resampling_explicit_filter_flag
  const uint32_t coefDataType{br.ReadBits(2)};
  unsigned int coefLog2Denom{32};
  if (coefDataType == COEF_FIXED)
  {
    coefLog2Denom = br.ReadUE(32);
    if (coefLog2Denom < 13)
      return {};
  }
  else if (coefDataType != COEF_FLOAT)
    return {};
  br.SkipBits(2); // vdr_rpu_normalized_idc
  const bool blFullRange{br.ReadBits(1) != 0};

  if ((rpuFormat & 0x700) != 0)
    return {};
  const uint32_t blBitDepth{br.ReadUE(8) + 8};
  // ext_mapping_idc in the upper byte, el_bit_depth_minus8 in the lower
  const uint32_t elBitDepthMinus8{br.ReadUE(0xFFFF)};
  const uint32_t vdrBitDepth{br.ReadUE(8) + 8};
  if ((elBitDepthMinus8 & 0xFF) > 8)
    return {};
  const uint32_t elBitDepth{(elBitDepthMinus8 & 0xFF) + 8};
  br.SkipBits(1); // spatial_resampling_filter_flag
  const uint32_t dmCompression{br.ReadBits(3)};
  const bool elSpatialResampling{br.ReadBits(1) != 0};
  const bool disableResidual{br.ReadBits(1) != 0};

  const bool dmMetadataPresent{br.ReadBits(1) != 0};
  if (dmCompression > 1 || (dmCompression != 0 && !dmMetadataPresent))
    return {};

  const bool usePrevVdrRpu{br.ReadBits(1) != 0};
  const bool useNlq{!disableResidual};
  const uint8_t profile{m_profile != 0
                            ? m_profile
                            : GuessProfile(vdrRpuProfile, blFullRange, elSpatialResampling,
                                           disableResidual, vdrBitDepth)};
  if (br.Error() || (profile == 5 && useNlq))
    return {};

  uint32_t vdrRpuId{br.ReadUE(MAX_DM_ID)};
  if (br.Error())
    return {};
  if (usePrevVdrRpu)
  {
    // An unknown mapping is taken as the first one
    if (!m_mappings[vdrRpuId])
      vdrRpuId = 0;
    if (!m_mappings[vdrRpuId])
      return {};
  }
  else
  {
    Mapping& mapping{m_mappings[vdrRpuId] ? *m_mappings[vdrRpuId] : m_mappings[vdrRpuId].emplace()};
    br.ReadUE(GOLOMB_31_MAX); // mapping_color_space
    br.ReadUE(GOLOMB_31_MAX); // mapping_chroma_format_idc

    std::array<uint32_t, 3> pivots{};
    for (uint32_t& count : pivots)
    {
      count = br.ReadUE(MAX_PIECES - 1) + 2;
      br.SkipBits(size_t{count} * blBitDepth);
    }

    if (useNlq)
    {
      const uint32_t nlqMethod{br.ReadBits(3)};
      br.SkipBits(2 * blBitDepth); // nlq_pred_pivot_value
      if (nlqMethod != NLQ_LINEAR_DEADZONE)
        return {};
    }

    br.ReadUE(0xFFFE); // num_x_partitions_minus1
    br.ReadUE(0xFFFE); // num_y_partitions_minus1

    for (const uint32_t count : pivots)
    {
      for (uint32_t piece = 0; piece + 1 < count && !br.Error(); ++piece)
      {
        if (br.ReadUE(1) == MAPPING_POLYNOMIAL)
        {
          const uint32_t orderMinus1{br.ReadUE(1)};
          // linear_interp_flag, which FFmpeg doesn't read
          if (orderMinus1 == 0 && br.ReadBits(1) != 0)
            return {};
          for (uint32_t coefficient = 0; coefficient <= orderMinus1 + 1; ++coefficient)
            SkipSignedCoefficient(br, coefDataType, coefLog2Denom);
        }
        else
        {
          const uint32_t orderMinus1{br.ReadBits(2)};
          if (orderMinus1 > 2)
            return {};
          // the constant, then 7 coefficients for each order
          for (uint32_t coefficient = 0; coefficient < 1 + 7 * (orderMinus1 + 1); ++coefficient)
            SkipSignedCoefficient(br, coefDataType, coefLog2Denom);
        }
      }
    }

    if (useNlq)
    {
      for (Quantisation& quantisation : mapping)
      {
        quantisation.offset = static_cast<uint16_t>(br.ReadBits(elBitDepth));
        const std::optional<uint64_t> vdrInMax{
            ReadUnsignedCoefficient(br, coefDataType, coefLog2Denom)};
        const std::optional<uint64_t> slope{
            ReadUnsignedCoefficient(br, coefDataType, coefLog2Denom)};
        const std::optional<uint64_t> threshold{
            ReadUnsignedCoefficient(br, coefDataType, coefLog2Denom)};
        if (!vdrInMax || !slope || !threshold)
          return {};
        quantisation.vdrInMax = *vdrInMax;
        quantisation.deadzoneSlope = *slope;
        quantisation.deadzoneThreshold = *threshold;
      }
    }
  }

  if (dmMetadataPresent)
  {
    const uint32_t affectedDmId{br.ReadUE(MAX_DM_ID)};
    const uint32_t currentDmId{br.ReadUE(MAX_DM_ID)};
    if (br.Error() || affectedDmId != currentDmId)
      return {};
    br.ReadUE(GOLOMB_31_MAX); // scene_refresh_flag
    if (dmCompression == 0)
    {
      // The YCbCr to RGB matrix and offsets, the RGB to LMS matrix, the EOTF and its parameters
      br.SkipBits(9 * 16 + 3 * 32 + 9 * 16 + 3 * 16 + 32);
      const uint32_t signalBitDepth{br.ReadBits(5)};
      if (signalBitDepth < 8 || signalBitDepth > 16)
        return {};
      // colour space, chroma format, full range flag, source minimum and maximum PQ, diagonal
      br.SkipBits(2 + 2 + 2 + 12 + 12 + 10);
      m_staticBlocks = 0;
    }

    int dynamicBlocks{0};
    if (!ReadExtensionBlocks(br, 1, dynamicBlocks))
      return {};
    // Room for an empty block before the CRC32 and terminator
    if (br.BitsLeft() >= 3 + 1 + 8 + 32 + 8 && !ReadExtensionBlocks(br, 2, dynamicBlocks))
      return {};
  }
  else
    m_staticBlocks = 0;

  // The CRC32 and a 0x80 terminator, which some encoders pad with a zero byte when byte aligned
  const bool aligned{br.Position() % 8 == 0};
  br.ByteAlign();
  CBitReader next{br};
  const bool padded{aligned && next.ReadBits(8) == 0};
  br.SkipBits(32);
  const bool terminated{br.ReadBits(8) == 0x80};
  next = br;
  const bool paddedTerminated{padded && next.ReadBits(8) == 0x80};
  if (br.Error() || (!terminated && !paddedTerminated))
    return {};

  // The CRC32 of the RPU up to its terminator, with the CRC, is 0. Kodi's decoder doesn't have
  // FFmpeg check it (AV_EF_CRCCHECK), but an RPU that fails it is left to the decoder.
  const auto crc{[&rpu](size_t size)
                 {
                   Crc32 crc;
                   crc.Compute(reinterpret_cast<const char*>(rpu.data()), size);
                   return static_cast<uint32_t>(crc);
                 }};
  const size_t end{br.Position() / 8 + (terminated ? 0 : 1)};
  // Where both could be the terminator, the RPU is either padded or not
  if (crc(end - 1) != 0 && !(terminated && paddedTerminated && crc(end) == 0))
    return {};

  if (!elSpatialResampling || disableResidual)
    return EnhancementLayer::NONE;

  const auto isIdentity = [](const Quantisation& quantisation)
  {
    return quantisation.offset == 0 && quantisation.vdrInMax == IDENTITY_VDR_IN_MAX &&
           quantisation.deadzoneSlope == 0 && quantisation.deadzoneThreshold == 0;
  };
  return std::ranges::all_of(*m_mappings[vdrRpuId], isIdentity) ? EnhancementLayer::MEL
                                                                : EnhancementLayer::FEL;
}
} // namespace KODI::UTILS::DOLBYVISION
