/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BitstreamConverter.h"
#include "BitstreamIoWriter.h"
#include "Crc32.h"

#include "cores/DataCacheCore.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "ServiceBroker.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <fmt/format.h>

extern "C"
{
#include <libdovi/rpu_parser.h>
}

enum
{
  HEVC_NAL_UNSPEC62 = 62, // Dolby Vision RPU
  HEVC_NAL_UNSPEC63 = 63 // Dolby Vision EL
};

namespace
{
bool IsValidPtsForInjection(double pts)
{
  return std::isfinite(pts) && pts >= 0.0;
}

constexpr char PTS_MARKER[] = "PTS_US64=";

bool AppendPtsToDoviRpuNalu(std::vector<uint8_t>& nalu, uint64_t ptsUs64)
{
  // Expect the final rbsp_trailing_bits byte.
  if (nalu.size() < 1 || nalu.back() != 0x80)
    return false;

  std::vector<uint8_t> trailer;
  trailer.reserve(sizeof(PTS_MARKER) - 1 + 16 + 1);
  trailer.insert(trailer.end(), reinterpret_cast<const uint8_t*>(PTS_MARKER),
                 reinterpret_cast<const uint8_t*>(PTS_MARKER) + (sizeof(PTS_MARKER) - 1));
  const std::string ptsHex = fmt::format("{:016X}", ptsUs64);
  trailer.insert(trailer.end(), ptsHex.begin(), ptsHex.end());
  trailer.push_back(static_cast<uint8_t>(';'));

  // Insert trailer right before the final 0x80 byte.
  nalu.insert(nalu.end() - 1, trailer.begin(), trailer.end());
  return true;
}

bool CachedRpuInputMatches(const std::vector<uint8_t>& cachedNalu,
                           const uint8_t* nalBuf,
                           int32_t nalSize)
{
  if (!nalBuf || nalSize <= 0) return false;

  const size_t size = static_cast<size_t>(nalSize);

  if (cachedNalu.size() != size) return false;

  // DoVi RPU RBSPs end with a CRC32 followed by rbsp_trailing_bits (0x80).
  // On the encoded NAL bytes, start-code emulation prevention can insert up to
  // two 0x03 bytes inside that 5-byte tail, so compare the last 7 bytes first
  // for a cheap early reject, then fall back to a full compare on a match.
  constexpr size_t crcAndTrailingSize = 7;
  if (size <= crcAndTrailingSize)
    return std::equal(cachedNalu.begin(), cachedNalu.end(), nalBuf);

  const auto cachedSuffixBegin = cachedNalu.end() - crcAndTrailingSize;
  if (!std::equal(cachedSuffixBegin, cachedNalu.end(), nalBuf + (size - crcAndTrailingSize)))
    return false;

  return std::equal(cachedNalu.begin(), cachedSuffixBegin, nalBuf);
}

bool IsCMv29NoL2(const DoviRpuDataHeader* header,
                 const DoviVdrDmData* vdrDmData)
{
  if (!header || !vdrDmData) return false;

  if (vdrDmData->dm_data.level254) return false;

  if (vdrDmData->dm_data.level2.len > 0) return false;

  return true;
}

inline void PopulateDoviRpuInfo(DoviRpuOpaque* opaque,
                                bool firstFrame,
                                DOVIELType& doviElType,
                                AVDOVIDecoderConfigurationRecord& dovi,
                                double pts,
                                CDataCacheCore& dataCacheCore,
                                DOVIFrameMetadata* outDoViFrameMetadata = nullptr)
{
  const DoviVdrDmData* vdrDmData = dovi_rpu_get_vdr_dm_data(opaque);

  if (vdrDmData)
  {
    DOVIFrameMetadata doviFrameMetadata;

    if (vdrDmData->dm_data.level1)
    {
      doviFrameMetadata.level1_min_pq = vdrDmData->dm_data.level1->min_pq;
      doviFrameMetadata.level1_max_pq = vdrDmData->dm_data.level1->max_pq;
      doviFrameMetadata.level1_avg_pq = vdrDmData->dm_data.level1->avg_pq;
      doviFrameMetadata.pts = pts;
    }

    if (vdrDmData->dm_data.level5)
    {
      doviFrameMetadata.has_level5_metadata = true;
      doviFrameMetadata.level5_active_area_left_offset =
          vdrDmData->dm_data.level5->active_area_left_offset;
      doviFrameMetadata.level5_active_area_right_offset =
          vdrDmData->dm_data.level5->active_area_right_offset;
      doviFrameMetadata.level5_active_area_top_offset =
          vdrDmData->dm_data.level5->active_area_top_offset;
      doviFrameMetadata.level5_active_area_bottom_offset =
          vdrDmData->dm_data.level5->active_area_bottom_offset;
    }

    dataCacheCore.SetVideoDoViFrameMetadata(doviFrameMetadata);
    if (outDoViFrameMetadata)
      *outDoViFrameMetadata = doviFrameMetadata;
  }

  if (firstFrame)
  {
    DOVIStreamMetadata doviStreamMetadata;

    if (vdrDmData)
    {
      doviStreamMetadata.source_min_pq = vdrDmData->source_min_pq;
      doviStreamMetadata.source_max_pq = vdrDmData->source_max_pq;
    }

    if (vdrDmData && vdrDmData->dm_data.level6)
    {
      doviStreamMetadata.has_level6_metadata = true;

      doviStreamMetadata.level6_max_lum =
          vdrDmData->dm_data.level6->max_display_mastering_luminance;
      doviStreamMetadata.level6_min_lum =
          vdrDmData->dm_data.level6->min_display_mastering_luminance;

      doviStreamMetadata.level6_max_cll = vdrDmData->dm_data.level6->max_content_light_level;
      doviStreamMetadata.level6_max_fall =
          vdrDmData->dm_data.level6->max_frame_average_light_level;
    }

    std::string metaVersion;
    bool hasLevel254 = false;
    unsigned int level2Count = 0;
    unsigned int level8Count = 0;
    if (vdrDmData && vdrDmData->dm_data.level254)
    {
      hasLevel254 = true;
      level8Count = vdrDmData->dm_data.level8.len;
      const unsigned int noL8 = vdrDmData->dm_data.level8.len;
      if (noL8 > 0)
        metaVersion = fmt::format("CMv4.0 {}-{} {}-L8", vdrDmData->dm_data.level254->dm_version_index,
                                 vdrDmData->dm_data.level254->dm_mode, noL8);
      else
        metaVersion = fmt::format("CMv4.0 {}-{}", vdrDmData->dm_data.level254->dm_version_index,
                                 vdrDmData->dm_data.level254->dm_mode);
    }
    else if (vdrDmData && vdrDmData->dm_data.level1)
    {
      level2Count = vdrDmData->dm_data.level2.len;
      const unsigned int noL2 = vdrDmData->dm_data.level2.len;
      if (noL2 > 0)
        metaVersion = fmt::format("CMv2.9 {}-L2", noL2);
      else
        metaVersion = "CMv2.9";
    }

    static bool loggedParsedMetadata = false;
    if (!loggedParsedMetadata)
    {
      loggedParsedMetadata = true;
      logM(LOGINFO, "CBitstreamConverterDoVi",
           "Parsed DoVi metadata (first frame): meta='{}' has_l254={} l2_count={} l8_count={}",
           metaVersion, hasLevel254, level2Count, level8Count);
    }

    doviStreamMetadata.meta_version = metaVersion;
    dataCacheCore.SetVideoDoViStreamMetadata(doviStreamMetadata);
    aml_dv_send_md_levels();

    DOVIStreamInfo doviStreamInfo;
    const DoviRpuDataHeader* header = dovi_rpu_get_header(opaque);
    doviElType = DOVIELType::TYPE_NONE;
    aml_dv_send_profile(header->guessed_profile);

    if (header && ((header->guessed_profile == 4) || (header->guessed_profile == 7)) && header->el_type)
    {
      if (StringUtils::EqualsNoCase(header->el_type, "FEL"))
        doviElType = DOVIELType::TYPE_FEL;
      else if (StringUtils::EqualsNoCase(header->el_type, "MEL"))
        doviElType = DOVIELType::TYPE_MEL;
    }

    doviStreamInfo.dovi_el_type = doviElType;
    doviStreamInfo.dovi = dovi;

    doviStreamInfo.has_config =
        (memcmp(&dovi, &CDVDStreamInfo::empty_dovi, sizeof(AVDOVIDecoderConfigurationRecord)) != 0);
    doviStreamInfo.has_header = (header != nullptr);

    dataCacheCore.SetVideoDoViStreamInfo(doviStreamInfo);
    aml_dv_send_el_type();
    dovi_rpu_free_header(header);
  }

  dovi_rpu_free_vdr_dm_data(vdrDmData);
}

void GetDoviRpuInfo(uint8_t* nalBuf,
                    uint32_t nalSize,
                    bool firstFrame,
                    DOVIELType& doviElType,
                    AVDOVIDecoderConfigurationRecord& dovi,
                    double pts,
                    CDataCacheCore& dataCacheCore)
{
  // https://professionalsupport.dolby.com/s/article/Dolby-Vision-Metadata-Levels?language=en_US

  DoviRpuOpaque* opaque = dovi_parse_unspec62_nalu(nalBuf, nalSize);
  PopulateDoviRpuInfo(opaque, firstFrame, doviElType, dovi, pts, dataCacheCore);
  dovi_rpu_free(opaque);
}

void AppendCMv40ExtensionBlock(BitstreamIoWriter& writer)
{
  // CM v4.0 extension metadata (allowed levels: 3, 8, 9, 10, 11, 254)
  // -----------------------------------------------------------------
  writer.write_ue(4);                         // (00101) num_ext_blocks
  writer.byte_align();                        // dm_alignment_zero_bit

  // Currently the extension block content is fixed, if we need for dynamic values in the future
  // then need to gate and check changes and recreate for each change, see HDR10+ dynamic metadata handling for example.
  static const std::vector<uint8_t> cached_ext_blocks = []() {
    BitstreamIoWriter cacheWriter;

    // L3 ------------ (53 bits)
    cacheWriter.write_ue(5);                         // (00110)          length_bytes (payload only)
    cacheWriter.write_n<uint8_t>(3, 8);              // (00000011)       level
    cacheWriter.write_n<uint16_t>(2048, 12);         // (100000000000)   min_pq_offset
    cacheWriter.write_n<uint16_t>(2048, 12);         // (100000000000)   max_pq_offset
    cacheWriter.write_n<uint16_t>(2048, 12);         // (100000000000)   avg_pq_offset
    cacheWriter.write_n<uint8_t>(0, 4);              // (0000)           alignment of 4 bits. (40)

    // L9 ------------ (19 bits)
    cacheWriter.write_ue(1);                         // (010)            length_bytes (payload only)
    cacheWriter.write_n<uint8_t>(9, 8);              // (00001001)       level
    cacheWriter.write_n<uint8_t>(0, 8);              // (00000000)       source_primary_index

    // L11 ----------- (45 bits)
    cacheWriter.write_ue(4);                         // (00101)          length_bytes (payload only)
    cacheWriter.write_n<uint8_t>(11, 8);             // (00001011)       level
    cacheWriter.write_n<uint8_t>(1, 8);              // (00000001)       content_type
    cacheWriter.write_n<uint8_t>(0, 8);              // (00000000)       whitepoint
    cacheWriter.write_n<uint8_t>(0, 8);              // (00000000)       reserved_byte2
    cacheWriter.write_n<uint8_t>(0, 8);              // (00000000)       reserved_byte3

    // L254 ---------- (27 bits)
    cacheWriter.write_ue(2);                         // (011)            length_bytes (payload only)
    cacheWriter.write_n<uint8_t>(254, 8);            // (11111110)       level
    cacheWriter.write_n<uint8_t>(0, 8);              // (00000000)       dm_mode
    cacheWriter.write_n<uint8_t>(2, 8);              // (00000010)       dm_version_index

    cacheWriter.byte_align();                        // ext_dm_alignment_zero_bit
    return cacheWriter.into_inner();
  }();

  writer.write_bytes(cached_ext_blocks.data(), cached_ext_blocks.size());
}

bool PayloadSize(const std::vector<uint8_t>& rbsp, size_t& payloadSize)
{
  if (rbsp.size() < 6) return false;

  if (rbsp.back() != 0x80) return false;

  payloadSize = rbsp.size() - 5;
  if (payloadSize <= 1) return false;

  return true;
}

// Build a NAL with CMv4.0 extension inserted at a specific offset.
// trimBits = number of rpu_alignment_zero_bits to strip from the end of the payload
// before inserting the CMv4.0 extension data.
bool BuildCMv40Nalu(const std::vector<uint8_t>& rbsp,
                    size_t payloadSize,
                    uint8_t nalHeader0,
                    uint8_t nalHeader1,
                    int trimBits,
                    std::vector<uint8_t>& naluOut)
{
  const int contentBits = (8 - trimBits);

  if ((contentBits <= 0) || (payloadSize < 1)) return false;

  BitstreamIoWriter writer(payloadSize + 26); // extension (21) + CRC32 (4) + FINAL_BYTE (1)

  // Copy all complete payload bytes except the last one
  if (payloadSize > 1)
    writer.write_bytes(rbsp.data(), payloadSize - 1);

  // Copy only the content bits of the last payload byte (strip alignment zeros)
  const uint8_t lastByte = rbsp[payloadSize - 1];
  writer.write_n<uint8_t>(static_cast<uint8_t>(lastByte >> trimBits), contentBits);

  // Append CMv4.0 extension at exact bit position (no alignment gap)
  AppendCMv40ExtensionBlock(writer);

  // rpu_alignment_zero_bit: pad to byte boundary
  writer.byte_align();

  writer.write_n<uint32_t>(Crc32::Compute(writer.as_slice() + 1, writer.as_slice_size() - 1), 32);
  writer.write_n<uint8_t>(0x80, 8);  // FINAL_BYTE

  std::vector<uint8_t> newRbsp = writer.into_inner();

  HevcAddStartCodeEmulationPrevention3Byte(newRbsp);

  naluOut.clear();
  naluOut.reserve(2 + newRbsp.size());
  naluOut.push_back(nalHeader0);
  naluOut.push_back(nalHeader1);
  naluOut.insert(naluOut.end(), newRbsp.begin(), newRbsp.end());

  return true;
}

// Parse a candidate NAL with libdovi and check that L254 is present.
// Returns the opaque RPU handle on success (caller must free), nullptr on failure.
DoviRpuOpaque* ParseAndValidateCmv40Nalu(const std::vector<uint8_t>& nalu)
{
  DoviRpuOpaque* opaque = dovi_parse_unspec62_nalu(nalu.data(), nalu.size());
  if (!opaque)
    return nullptr;

  const DoviVdrDmData* dm = dovi_rpu_get_vdr_dm_data(opaque);
  const bool valid = (dm && dm->dm_data.level254);
  dovi_rpu_free_vdr_dm_data(dm);

  if (valid)
    return opaque;

  dovi_rpu_free(opaque);
  return nullptr;
}

// Append CMv4.0 extension to an RPU NAL. On success, populates |out| with the
// new NAL and returns the validated DoviRpuOpaque* (caller must free).
// Returns nullptr on failure.
//
// |trim| is a hint for the number of rpu_alignment_zero_bits to strip.
// Most commonly 1 (L6 is 79 bits → 1 bit padding). Updated on success.
DoviRpuOpaque* AppendCMv40ToRpuNalu(uint8_t* nalBuf,
                                    int32_t nalSize,
                                    std::vector<uint8_t>& out,
                                    uint8_t& trim)
{
  if (!nalBuf || (nalSize <= 2)) return nullptr;

  const uint8_t nal0 = nalBuf[0];
  const uint8_t nal1 = nalBuf[1];

  std::vector<uint8_t> rbsp;
  HevcClearStartCodeEmulationPrevention3Byte(nalBuf + 2, static_cast<size_t>(nalSize - 2), rbsp);

  if (rbsp.size() < 2) return nullptr;

  size_t payloadSize = 0;
  if (!PayloadSize(rbsp, payloadSize)) return nullptr;

  // The RPU bitstream has rpu_alignment_zero_bit padding (0-7 bits) between the
  // CMv2.9 DM data section end and the CRC. We must strip them before
  // inserting the CMv4.0 extension.
  //
  // |trim| hints where to start (most commonly 1 for L6's 79 bits).
  // If the LSB at trim is a 1-bit (data), the payload is already byte-aligned,
  // so skip straight to 0 instead of searching upward through all values.
  std::vector<uint8_t> naluOut;

  if (trim > 0 && (rbsp[payloadSize - 1] & ((1 << trim) - 1))) trim = 0;

  for (uint8_t i = 0; i <= 7; ++i)
  {
    const uint8_t trimBits = static_cast<uint8_t>((trim + i) % 8);

    naluOut.clear();
    if (BuildCMv40Nalu(rbsp, payloadSize, nal0, nal1, trimBits, naluOut))
    {
      DoviRpuOpaque* opaque = ParseAndValidateCmv40Nalu(naluOut);
      if (opaque)
      {
        if (trim != trimBits)
        {
          logM(LOGINFO, "CBitstreamConverterDoVi",
                        "CMv4 alignment: last_byte=0x{:02X} padding={}",
                        rbsp[payloadSize - 1], trimBits);
          trim = trimBits;
        }
        out.swap(naluOut);
        return opaque;
      }
    }
  }

  return nullptr;
}

inline DOVIELType GetElTypeFromHeader(const DoviRpuDataHeader* header)
{
  if (header && ((header->guessed_profile == 4) || (header->guessed_profile == 7)) &&
      header->el_type)
  {
    if (StringUtils::EqualsNoCase(header->el_type, "FEL"))
      return DOVIELType::TYPE_FEL;
    if (StringUtils::EqualsNoCase(header->el_type, "MEL"))
      return DOVIELType::TYPE_MEL;
  }

  return DOVIELType::TYPE_NONE;
}

inline void ConvertDoVi(DOVIMode convertMode,
                        bool firstFrame,
                        DoviRpuOpaque* opaque,
                        const DoviRpuDataHeader*& header,
                        const DoviVdrDmData*& vdrDmData,
                        CDVDStreamInfo& hints,
                        CDataCacheCore& dataCacheCore,
                        uint8_t*& nalBuf,
                        int32_t& nalSize,
                        const DoviData*& rpuData)
{
  if (!header || (header->guessed_profile != 7)) return;

  if (firstFrame)
  {
    DOVIStreamInfo doviStreamInfo;
    doviStreamInfo.dovi_el_type = GetElTypeFromHeader(header);
    doviStreamInfo.dovi = hints.dovi;
    dataCacheCore.SetVideoSourceDoViStreamInfo(doviStreamInfo);
  }

  if (dovi_convert_rpu_with_mode(opaque, convertMode) >= 0)
    rpuData = dovi_write_unspec62_nalu(opaque);

  if (!rpuData) return;

  nalBuf = const_cast<uint8_t*>(rpuData->data);
  nalSize = rpuData->len;

  hints.dovi.el_present_flag = 0; // EL removed in both conversion cases - to MEL and to P8.1
  if (convertMode == DOVIMode::MODE_TO81)
  {
    hints.dovi.dv_profile = 8;
    hints.dovi.dv_bl_signal_compatibility_id = 1;
  }

  dovi_rpu_free_header(header);

  header = dovi_rpu_get_header(opaque);
  dovi_rpu_free_vdr_dm_data(vdrDmData);
  vdrDmData = dovi_rpu_get_vdr_dm_data(opaque);
}

inline void AppendCMv40(DOVICMv40Mode cmv40Mode,
                        const DoviRpuDataHeader* header,
                        const DoviVdrDmData* vdrDmData,
                        uint8_t*& nalBuf,
                        int32_t& nalSize,
                        std::vector<uint8_t>& nalu,
                        DoviRpuOpaque*& opaque,
                        uint8_t& trim)
{
  if (!header || !vdrDmData) return;

  DOVIStreamMetadata dovi_stream_metadata;
  dovi_stream_metadata = CServiceBroker::GetDataCacheCore().GetVideoDoViStreamMetadata();
  int source_max_nits = max_pq_to_nits(static_cast<int>(dovi_stream_metadata.source_max_pq));
  int max_lum_nits_value(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM));
  bool is_displayML_higher_sourceMDL = (max_lum_nits_value >= source_max_nits);
  int dv_type(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE));

  const bool hasLevel254 = (vdrDmData->dm_data.level254 != nullptr);
  if (!((((cmv40Mode == DOVICMv40Mode::CMV40_ALWAYS) && !hasLevel254) ||
         ((cmv40Mode == DOVICMv40Mode::CMV40_AUTO) && (IsCMv29NoL2(header, vdrDmData) || (!hasLevel254 && is_displayML_higher_sourceMDL))) ||
         ((cmv40Mode == DOVICMv40Mode::CMV40_NO_L2) && IsCMv29NoL2(header, vdrDmData))) &&
        (dv_type == 0))) return;

  opaque = AppendCMv40ToRpuNalu(nalBuf, nalSize, nalu, trim);
  if (opaque)
  {
    nalBuf = nalu.data();
    nalSize = static_cast<int32_t>(nalu.size());
  }
}

inline void InjectPtsForFel(DOVIMode convertMode,
                            DOVIELType doviElType,
                            double pts,
                            uint8_t*& nalBuf,
                            int32_t& nalSize,
                            std::vector<uint8_t>& nalu)
{
  if ((convertMode != DOVIMode::MODE_NONE) ||
      (doviElType != DOVIELType::TYPE_FEL) ||
      !IsValidPtsForInjection(pts) || !nalBuf || (nalSize <= 0)) return;

  if (nalu.empty())
    nalu.assign(nalBuf, nalBuf + nalSize);

  if (AppendPtsToDoviRpuNalu(nalu, static_cast<uint64_t>(pts)))
  {
    nalBuf = nalu.data();
    nalSize = static_cast<int32_t>(nalu.size());
  }
}
} // namespace

void CBitstreamConverter::ProcessDoViRpuWrap(
  uint8_t* nalBuf,
  int32_t nalSize,
  uint8_t** poutbuf,
  uint32_t& poutbufSize,
  double pts)
{
  int intPoutbufSize = poutbufSize;
  ProcessDoViRpu(nalBuf, nalSize, poutbuf, &intPoutbufSize, pts);
  poutbufSize = static_cast<uint32_t>(intPoutbufSize);
}

void CBitstreamConverter::ProcessDoViRpu(
  uint8_t* nalBuf,
  int32_t nalSize,
  uint8_t** poutbuf,
  int* poutbufSize,
  double pts)
{
  const DoviData* rpuData = nullptr;
  DoviRpuOpaque* appendOpaque = nullptr;
  std::vector<uint8_t> nalu;

  // Optimization: If the input RPU NAL is exactly identical to the previous frame's RPU NAL,
  // AND we are not processing the first frame (which parses stream metadata),
  // we can completely skip `dovi_parse_unspec62_nalu` and avoid all allocations & format processing.
  // This cache deliberately tracks the original input RPU, before any FEL PTS trailer is injected,
  // so a per-frame PTS never invalidates reuse.
  if (!m_first_frame && CachedRpuInputMatches(m_cached_dovi_rpu_in_nal, nalBuf, nalSize))
  {
    m_cached_dovi_frame_metadata.pts = pts;
    m_dataCacheCore.SetVideoDoViFrameMetadata(m_cached_dovi_frame_metadata);

    // Skip all processing and restore the fully configured/converted output NAL
    nalBuf = m_cached_dovi_rpu_out_nal.data();
    nalSize = static_cast<int32_t>(m_cached_dovi_rpu_out_nal.size());
  }
  else
  {
    // Save the original input stream bits before processing modifications
    m_cached_dovi_rpu_in_nal.assign(nalBuf, nalBuf + nalSize);

    DoviRpuOpaque* opaque = dovi_parse_unspec62_nalu(nalBuf, nalSize);
    const DoviRpuDataHeader* header = dovi_rpu_get_header(opaque);
    const DoviVdrDmData* vdrDmData = dovi_rpu_get_vdr_dm_data(opaque);

    if (m_convert_dovi != DOVIMode::MODE_NONE)
      ConvertDoVi(m_convert_dovi,
                  m_first_frame,
                  opaque,
                  header,
                  vdrDmData,
                  m_hints,
                  m_dataCacheCore,
                  nalBuf,
                  nalSize,
                  rpuData);

    if (m_append_cmv40 != DOVICMv40Mode::CMV40_NONE)
      AppendCMv40(m_append_cmv40,
                  header,
                  vdrDmData,
                  nalBuf,
                  nalSize,
                  nalu,
                  appendOpaque,
                  m_cmv40_trim);

    // Use the appendOpaque from the append CMv4.0 if available
    DoviRpuOpaque* metadataOpaque = appendOpaque ? appendOpaque : opaque;
    PopulateDoviRpuInfo(metadataOpaque,
                        m_first_frame,
                        m_hints.dovi_el_type,
                        m_hints.dovi,
                        pts,
                        m_dataCacheCore,
                        &m_cached_dovi_frame_metadata);

    dovi_rpu_free_header(header);
    dovi_rpu_free_vdr_dm_data(vdrDmData);
    dovi_rpu_free(opaque);
    if (appendOpaque)
    {
      dovi_rpu_free(appendOpaque);
      if (m_first_frame)
        logM(LOGINFO, "CBitstreamConverterDoVi", "CMv4.0 extension appended to RPU");
    }

    // Update cache with the newly calculated modified NAL out for the next frame
    m_cached_dovi_rpu_out_nal.assign(nalBuf, nalBuf + nalSize);
  }

  InjectPtsForFel(m_convert_dovi,
                  m_hints.dovi_el_type,
                  pts,
                  nalBuf,
                  nalSize,
                  nalu);

  // HEVC NAL unit type 62 is Dolby Vision RPU (UNSPEC62)
  BitstreamAllocAndCopy(poutbuf, poutbufSize, nullptr, 0, nalBuf, nalSize, 62);

  if (rpuData) dovi_data_free(rpuData);
}

void CBitstreamConverter::AddDoViRpuNaluWrap(const Hdr10PlusMetadata& meta,
                                            uint8_t** poutbuf,
                                            uint32_t& poutbufSize,
                                            double pts)
{
  int intPoutbufSize = poutbufSize;
  AddDoViRpuNalu(meta, poutbuf, &intPoutbufSize, pts);
  poutbufSize = static_cast<uint32_t>(intPoutbufSize);
}

void CBitstreamConverter::AddDoViRpuNalu(const Hdr10PlusMetadata& meta,
                                        uint8_t** poutbuf,
                                        int* poutbufSize,
                                        double pts) const
{
  auto nalu = create_dovi_rpu_nalu_from_hdr10plus(meta, m_convert_Hdr10Plus_peak_brightness_source,
                                                  m_hdrStaticMetadataInfo);

  if (nalu.empty()) return;

  if (m_first_frame)
  {
    m_hints.hdrType = StreamHdrType::HDR_TYPE_DOLBYVISION;
    m_hints.dovi.dv_version_major = 1;
    m_hints.dovi.dv_version_minor = 0;
    m_hints.dovi.dv_profile = 8;
    m_hints.dovi.dv_level = 6;
    m_hints.dovi.rpu_present_flag = 1;
    m_hints.dovi.el_present_flag = 0;
    m_hints.dovi.bl_present_flag = 1;
    m_hints.dovi.dv_bl_signal_compatibility_id = 1;
  }

  GetDoviRpuInfo(nalu.data(), static_cast<uint32_t>(nalu.size()), m_first_frame, m_hints.dovi_el_type,
                m_hints.dovi, pts, m_dataCacheCore);

  BitstreamAllocAndCopy(poutbuf, poutbufSize, nullptr, 0, nalu.data(),
                        static_cast<uint32_t>(nalu.size()), HEVC_NAL_UNSPEC62);
}
