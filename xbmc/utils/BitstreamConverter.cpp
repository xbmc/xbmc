/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BitstreamConverter.h"

#include "HevcSei.h"
#include "utils/log.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <tuple>
#include <vector>

enum
{
  AVC_NAL_SLICE = 1,
  AVC_NAL_DPA,
  AVC_NAL_DPB,
  AVC_NAL_DPC,
  AVC_NAL_IDR_SLICE,
  AVC_NAL_SEI,
  AVC_NAL_SPS,
  AVC_NAL_PPS,
  AVC_NAL_AUD,
  AVC_NAL_END_SEQUENCE,
  AVC_NAL_END_STREAM,
  AVC_NAL_FILLER_DATA,
  AVC_NAL_SPS_EXT,
  AVC_NAL_AUXILIARY_SLICE = 19
};

enum
{
  HEVC_NAL_TRAIL_N = 0,
  HEVC_NAL_TRAIL_R = 1,
  HEVC_NAL_TSA_N = 2,
  HEVC_NAL_TSA_R = 3,
  HEVC_NAL_STSA_N = 4,
  HEVC_NAL_STSA_R = 5,
  HEVC_NAL_RADL_N = 6,
  HEVC_NAL_RADL_R = 7,
  HEVC_NAL_RASL_N = 8,
  HEVC_NAL_RASL_R = 9,
  HEVC_NAL_BLA_W_LP = 16,
  HEVC_NAL_BLA_W_RADL = 17,
  HEVC_NAL_BLA_N_LP = 18,
  HEVC_NAL_IDR_W_RADL = 19,
  HEVC_NAL_IDR_N_LP = 20,
  HEVC_NAL_CRA_NUT = 21,
  HEVC_NAL_VPS = 32,
  HEVC_NAL_SPS = 33,
  HEVC_NAL_PPS = 34,
  HEVC_NAL_AUD = 35,
  HEVC_NAL_EOS_NUT = 36,
  HEVC_NAL_EOB_NUT = 37,
  HEVC_NAL_FD_NUT = 38,
  HEVC_NAL_SEI_PREFIX = 39,
  HEVC_NAL_SEI_SUFFIX = 40,
  HEVC_NAL_UNSPEC62 = 62, // Dolby Vision RPU
  HEVC_NAL_UNSPEC63 = 63 // Dolby Vision EL
};

enum
{
  SEI_BUFFERING_PERIOD = 0,
  SEI_PIC_TIMING,
  SEI_PAN_SCAN_RECT,
  SEI_FILLER_PAYLOAD,
  SEI_USER_DATA_REGISTERED_ITU_T_T35,
  SEI_USER_DATA_UNREGISTERED,
  SEI_RECOVERY_POINT,
  SEI_DEC_REF_PIC_MARKING_REPETITION,
  SEI_SPARE_PIC,
  SEI_SCENE_INFO,
  SEI_SUB_SEQ_INFO,
  SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
  SEI_SUB_SEQ_CHARACTERISTICS,
  SEI_FULL_FRAME_FREEZE,
  SEI_FULL_FRAME_FREEZE_RELEASE,
  SEI_FULL_FRAME_SNAPSHOT,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
  SEI_FILM_GRAIN_CHARACTERISTICS,
  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
  SEI_STEREO_VIDEO_INFO,
  SEI_POST_FILTER_HINTS,
  SEI_TONE_MAPPING
};

/*
 *  GStreamer h264 parser
 *  Copyright (C) 2005 Michal Benes <michal.benes@itonis.tv>
 *            (C) 2008 Wim Taymans <wim.taymans@gmail.com>
 *  gsth264parse.c
 *  
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */
static void nal_bs_init(nal_bitstream* bs, const uint8_t* data, size_t size)
{
  bs->data = data;
  bs->end = data + size;
  bs->head = 0;
  // fill with something other than 0 to detect
  //  emulation prevention bytes
  bs->cache = 0xffffffff;
}

static uint32_t nal_bs_read(nal_bitstream* bs, int n)
{
  uint32_t res = 0;
  int shift;

  if (n == 0)
    return res;

  // fill up the cache if we need to
  while (bs->head < n)
  {
    uint8_t a_byte;
    bool check_three_byte;

    check_three_byte = true;
  next_byte:
    if (bs->data >= bs->end)
    {
      // we're at the end, can't produce more than head number of bits
      n = bs->head;
      break;
    }
    // get the byte, this can be an emulation_prevention_three_byte that we need
    // to ignore.
    a_byte = *bs->data++;
    if (check_three_byte && a_byte == 0x03 && ((bs->cache & 0xffff) == 0))
    {
      // next byte goes unconditionally to the cache, even if it's 0x03
      check_three_byte = false;
      goto next_byte;
    }
    // shift bytes in cache, moving the head bits of the cache left
    bs->cache = (bs->cache << 8) | a_byte;
    bs->head += 8;
  }

  // bring the required bits down and truncate
  if ((shift = bs->head - n) > 0)
    res = static_cast<uint32_t>(bs->cache >> shift);
  else
    res = static_cast<uint32_t>(bs->cache);

  // mask out required bits
  if (n < 32)
    res &= (1 << n) - 1;
  bs->head = shift;

  return res;
}

static bool nal_bs_eos(nal_bitstream* bs)
{
  return (bs->data >= bs->end) && (bs->head == 0);
}

// read unsigned Exp-Golomb code
static int nal_bs_read_ue(nal_bitstream* bs)
{
  int i = 0;

  while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 31)
    i++;

  return ((1 << i) - 1 + nal_bs_read(bs, i));
}

static const uint8_t* avc_find_startcode_internal(const uint8_t* p, const uint8_t* end)
{
  const uint8_t* a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4)
  {
    uint32_t x = *(const uint32_t*)p;
    if ((x - 0x01010101) & (~x) & 0x80808080) // generic
    {
      if (p[1] == 0)
      {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p + 1;
      }
      if (p[3] == 0)
      {
        if (p[2] == 0 && p[4] == 1)
          return p + 2;
        if (p[4] == 0 && p[5] == 1)
          return p + 3;
      }
    }
  }

  for (end += 3; p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}

static const uint8_t* avc_find_startcode(const uint8_t* p, const uint8_t* end)
{
  const uint8_t* out = avc_find_startcode_internal(p, end);
  if (p < out && out < end && !out[-1])
    out--;
  return out;
}

static bool has_sei_recovery_point(const uint8_t* p, const uint8_t* end)
{
  int pt(0), ps(0), offset(1);

  do
  {
    pt = 0;
    do
    {
      pt += p[offset];
    } while (p[offset++] == 0xFF);

    ps = 0;
    do
    {
      ps += p[offset];
    } while (p[offset++] == 0xFF);

    if (pt == SEI_RECOVERY_POINT)
    {
      nal_bitstream bs;
      nal_bs_init(&bs, p + offset, ps);
      return nal_bs_read_ue(&bs) >= 0;
    }
    offset += ps;
  } while (p + offset < end && p[offset] != 0x80);

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CBitstreamParser::CBitstreamParser() = default;

void CBitstreamParser::Close()
{
}

bool CBitstreamParser::CanStartDecode(const uint8_t* buf, int buf_size)
{
  if (!buf)
    return false;

  bool rtn = false;
  uint32_t state = -1;
  const uint8_t *buf_begin, *buf_end = buf + buf_size;

  for (; rtn == false;)
  {
    buf = find_start_code(buf, buf_end, &state);
    if (buf >= buf_end)
    {
      break;
    }

    switch (state & 0x1f)
    {
      case AVC_NAL_SLICE:
        break;
      case AVC_NAL_IDR_SLICE:
        rtn = true;
        break;
      case AVC_NAL_SEI:
        buf_begin = buf - 1;
        buf = find_start_code(buf, buf_end, &state) - 4;
        if (has_sei_recovery_point(buf_begin, buf))
          rtn = true;
        break;
      case AVC_NAL_SPS:
        rtn = true;
        break;
      case AVC_NAL_PPS:
        break;
      default:
        break;
    }
  }

  return rtn;
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CBitstreamConverter::CBitstreamConverter()
{
  m_convert_bitstream = false;
  m_convertBuffer = NULL;
  m_convertSize = 0;
  m_start_decode = true;
  m_convert_dovi = false;
  m_removeDovi = false;
  m_removeHdr10Plus = false;
  m_setDoviZeroLevel5 = false;
}

CBitstreamConverter::~CBitstreamConverter()
{
  Close();
}

bool CBitstreamConverter::Open(enum AVCodecID codec, const uint8_t* in_extradata, int in_extrasize)
{
  m_codec = codec;
  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      if (in_extrasize < 7 || in_extradata == NULL)
      {
        CLog::Log(LOGERROR, "CBitstreamConverter::Open avcC data too small or missing");
        return false;
      }
      if (in_extradata[0] == 1)
      {
        CLog::Log(LOGINFO, "CBitstreamConverter::Open bitstream to annexb init");
        return BitstreamConvertInit("h264_mp4toannexb", in_extradata, in_extrasize);
      }
      CLog::Log(LOGINFO, "CBitstreamConverter::Open Invalid avcC");
      return false;
    case AV_CODEC_ID_HEVC:
      if (in_extrasize < 23 || in_extradata == NULL)
      {
        CLog::Log(LOGERROR, "CBitstreamConverter::Open hvcC data too small or missing");
        return false;
      }
      /**
       * It seems the extradata is encoded as hvcC format.
       * Temporarily, we support configurationVersion==0 until 14496-15 3rd
       * is finalized. When finalized, configurationVersion will be 1 and we
       * can recognize hvcC by checking if extradata[0]==1 or not.
       */
      if (in_extradata[0] || in_extradata[1] || in_extradata[2] > 1)
      {
        CLog::Log(LOGINFO, "CBitstreamConverter::Open bitstream to annexb init");
        return BitstreamConvertInit("hevc_mp4toannexb", in_extradata, in_extrasize);
      }
      CLog::Log(LOGINFO, "CBitstreamConverter::Open Invalid hvcC");
      return false;
    default:
      return false;
  }
}

void CBitstreamConverter::Close()
{
  av_bsf_free(&m_bitstreamFilter);

  if (m_convertBuffer)
    av_free(m_convertBuffer), m_convertBuffer = NULL;
  m_convertSize = 0;

  m_extraData = {};

  m_convert_bitstream = false;
}

bool CBitstreamConverter::Convert(uint8_t* pData, int iSize)
{
  if (m_convertBuffer)
  {
    av_free(m_convertBuffer);
    m_convertBuffer = NULL;
  }
  m_convertSize = 0;

  return pData && iSize > 0 && m_convert_bitstream && BitstreamConvert(pData, iSize);
}

uint8_t* CBitstreamConverter::GetConvertBuffer() const
{
  return m_convertBuffer;
}

int CBitstreamConverter::GetConvertSize() const
{
  return m_convertSize;
}

uint8_t* CBitstreamConverter::GetExtraData()
{
  return m_extraData.GetData();
}
const uint8_t* CBitstreamConverter::GetExtraData() const
{
  return m_extraData.GetData();
}
int CBitstreamConverter::GetExtraSize() const
{
  return m_extraData.GetSize();
}

void CBitstreamConverter::ResetStartDecode()
{
  m_start_decode = false;
}

bool CBitstreamConverter::CanStartDecode() const
{
  return m_start_decode;
}

bool CBitstreamConverter::BitstreamConvertInit(const char* filterName,
                                               const uint8_t* extraData,
                                               int extraDataSize)
{
  const AVBitStreamFilter* filter{av_bsf_get_by_name(filterName)};
  if (!filter)
  {
    CLog::LogF(LOGERROR, "Bitstream filter {} not found", filterName);
    return false;
  }

  int result{av_bsf_alloc(filter, &m_bitstreamFilter)};
  if (result < 0)
  {
    CLog::LogF(LOGERROR, "Failed to allocate {}: {}", filterName,
               FFMPEG_HELP_TOOLS::FFMpegErrorToString(result));
    return false;
  }

  AVCodecParameters* codecParameters{m_bitstreamFilter->par_in};
  codecParameters->codec_type = AVMEDIA_TYPE_VIDEO;
  codecParameters->codec_id = m_codec;
  codecParameters->extradata = static_cast<uint8_t*>(
      av_mallocz(static_cast<size_t>(extraDataSize) + AV_INPUT_BUFFER_PADDING_SIZE));
  if (!codecParameters->extradata)
  {
    av_bsf_free(&m_bitstreamFilter);
    return false;
  }

  std::copy_n(extraData, extraDataSize, codecParameters->extradata);
  codecParameters->extradata_size = extraDataSize;
  m_bitstreamFilter->time_base_in = {1, AV_TIME_BASE};

  result = av_bsf_init(m_bitstreamFilter);
  if (result < 0)
  {
    CLog::LogF(LOGERROR, "Failed to initialize {}: {}", filterName,
               FFMPEG_HELP_TOOLS::FFMpegErrorToString(result));
    av_bsf_free(&m_bitstreamFilter);
    return false;
  }

  if (m_bitstreamFilter->par_out->extradata && m_bitstreamFilter->par_out->extradata_size > 0)
  {
    m_extraData = FFmpegExtraData{m_bitstreamFilter->par_out->extradata,
                                  static_cast<size_t>(m_bitstreamFilter->par_out->extradata_size)};
  }

  m_convert_bitstream = true;
  return true;
}

bool CBitstreamConverter::IsIDR(uint8_t unit_type)
{
  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      return unit_type == AVC_NAL_IDR_SLICE;
    case AV_CODEC_ID_HEVC:
      return unit_type == HEVC_NAL_IDR_W_RADL || unit_type == HEVC_NAL_IDR_N_LP ||
             unit_type == HEVC_NAL_CRA_NUT;
    default:
      return false;
  }
}

bool CBitstreamConverter::BitstreamConvert(const uint8_t* pData, int iSize)
{
  const auto freePacket = [](AVPacket* packetToFree) { av_packet_free(&packetToFree); };
  std::unique_ptr<AVPacket, decltype(freePacket)> packet{av_packet_alloc(), freePacket};
  if (!packet)
    return false;

  int result{av_new_packet(packet.get(), iSize)};
  if (result >= 0)
  {
    std::copy_n(pData, iSize, packet->data);
    result = av_bsf_send_packet(m_bitstreamFilter, packet.get());
  }
  if (result >= 0)
    result = av_bsf_receive_packet(m_bitstreamFilter, packet.get());

  if (result < 0)
  {
    CLog::LogF(LOGERROR, "Error converting: {}", FFMPEG_HELP_TOOLS::FFMpegErrorToString(result));
    return false;
  }

  return ProcessAnnexB(packet->data, packet->size);
}

bool CBitstreamConverter::ProcessAnnexB(uint8_t* data, int size)
{
  const uint8_t* const bufferEnd{data + size};
  const uint8_t* nalStart{avc_find_startcode(data, bufferEnd)};

#ifdef HAVE_LIBDOVI
  const DoviData* rpuData{nullptr};
#endif

  std::vector<uint8_t> finalPrefixSeiNalu;
  std::vector<uint8_t> output;
  const bool rewrite{m_removeDovi || m_removeHdr10Plus || m_convert_dovi || m_setDoviZeroLevel5};
  while (nalStart < bufferEnd)
  {
    while (nalStart < bufferEnd && !*nalStart)
      ++nalStart;
    if (nalStart >= bufferEnd || *nalStart++ != 1 || nalStart >= bufferEnd)
      break;

    const uint8_t* nalEnd{avc_find_startcode(nalStart, bufferEnd)};
    const int nalSize{static_cast<int>(nalEnd - nalStart)};
    const uint8_t unitType{m_codec == AV_CODEC_ID_H264
                               ? static_cast<uint8_t>(*nalStart & 0x1f)
                               : static_cast<uint8_t>((*nalStart >> 1) & 0x3f)};
    const uint8_t nalSps{m_codec == AV_CODEC_ID_H264 ? static_cast<uint8_t>(AVC_NAL_SPS)
                                                     : static_cast<uint8_t>(HEVC_NAL_SPS)};
    const uint8_t nalSei{m_codec == AV_CODEC_ID_H264 ? static_cast<uint8_t>(AVC_NAL_SEI)
                                                     : static_cast<uint8_t>(HEVC_NAL_SEI_PREFIX)};

    if (!m_start_decode && (unitType == nalSps || IsIDR(unitType) ||
                            (unitType == nalSei && has_sei_recovery_point(nalStart, nalEnd))))
      m_start_decode = true;

    bool writeBuffer{true};
    const uint8_t* bufferToWrite{nalStart};
    int32_t finalNalSize{nalSize};
    bool containsHdr10Plus{false};

    if (m_removeDovi && (unitType == HEVC_NAL_UNSPEC62 || unitType == HEVC_NAL_UNSPEC63))
      writeBuffer = false;

    // Try removing HDR10+ only if the NAL is big enough, optimization
    if (m_removeHdr10Plus && unitType == HEVC_NAL_SEI_PREFIX && nalSize >= 7)
    {
      std::tie(containsHdr10Plus, finalPrefixSeiNalu) =
          CHevcSei::RemoveHdr10PlusFromSeiNalu(nalStart, nalSize);

      if (containsHdr10Plus)
      {
        if (!finalPrefixSeiNalu.empty())
        {
          bufferToWrite = finalPrefixSeiNalu.data();
          finalNalSize = static_cast<int32_t>(finalPrefixSeiNalu.size());
        }
        else
        {
          writeBuffer = false;
        }
      }
    }

    if (writeBuffer)
    {
      if (unitType == HEVC_NAL_UNSPEC62)
      {
#ifdef HAVE_LIBDOVI
        // Convert the RPU itself
        rpuData = processDoviRpu(const_cast<uint8_t*>(nalStart), nalSize);
        if (rpuData)
        {
          bufferToWrite = rpuData->data;
          finalNalSize = static_cast<int32_t>(rpuData->len);
        }
#endif
      }
      else if (m_convert_dovi && unitType == HEVC_NAL_UNSPEC63)
      {
        // Ignore the enhancement layer, may or may not help
        writeBuffer = false;
      }
    }

    if (rewrite && writeBuffer)
    {
      const size_t startCodeSize{output.empty() || unitType == HEVC_NAL_UNSPEC62 ? 4U : 3U};
      if (output.size() > static_cast<size_t>(std::numeric_limits<int>::max()) - startCodeSize ||
          static_cast<size_t>(finalNalSize) >
              static_cast<size_t>(std::numeric_limits<int>::max()) - output.size() - startCodeSize)
      {
#ifdef HAVE_LIBDOVI
        if (rpuData)
          dovi_data_free(rpuData);
#endif
        return false;
      }

      if (startCodeSize == 4)
        output.push_back(0);
      output.insert(output.end(), {0, 0, 1});
      output.insert(output.end(), bufferToWrite, bufferToWrite + finalNalSize);
    }

#ifdef HAVE_LIBDOVI
    if (rpuData)
    {
      dovi_data_free(rpuData);
      rpuData = nullptr;
    }
#endif

    if (containsHdr10Plus && !finalPrefixSeiNalu.empty())
      finalPrefixSeiNalu.clear();

    nalStart = nalEnd;
  }

  const uint8_t* outputData{rewrite ? output.data() : data};
  const size_t outputSize{rewrite ? output.size() : static_cast<size_t>(size)};
  if (outputSize == 0)
    return false;

  m_convertBuffer = static_cast<uint8_t*>(av_memdup(outputData, outputSize));
  if (!m_convertBuffer)
    return false;

  m_convertSize = static_cast<int>(outputSize);
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
bool CBitstreamConverter::mpeg2_sequence_header(const uint8_t* data,
                                                const uint32_t size,
                                                mpeg2_sequence* sequence)
{
  // parse nal's until a sequence_header_code is found
  // and return the width, height, aspect ratio and frame rate if changed.
  bool changed = false;

  if (!data)
    return changed;

  const uint8_t* p = data;
  const uint8_t* end = p + size;
  const uint8_t *nal_start, *nal_end;

  nal_start = avc_find_startcode(p, end);
  while (nal_start < end)
  {
    while (!*(nal_start++))
      ;
    nal_end = avc_find_startcode(nal_start, end);
    if (*nal_start == 0xB3)
    {
      nal_bitstream bs;
      nal_bs_init(&bs, nal_start, end - nal_start);

      // sequence_header_code
      nal_bs_read(&bs, 8);

      // width
      // nal_start + 12 bits == horizontal_size_value
      uint32_t width = nal_bs_read(&bs, 12);
      if (width != sequence->width)
      {
        changed = true;
        sequence->width = width;
      }
      // height
      // nal_start + 24 bits == vertical_size_value
      uint32_t height = nal_bs_read(&bs, 12);
      if (height != sequence->height)
      {
        changed = true;
        sequence->height = height;
      }

      // aspect ratio
      // nal_start + 28 bits == aspect_ratio_information
      float ratio = sequence->ratio;
      uint32_t ratio_info = nal_bs_read(&bs, 4);
      switch (ratio_info)
      {
        case 0x01:
          ratio = 1.0f;
          break;
        default:
        case 0x02:
          ratio = 4.0f / 3;
          break;
        case 0x03:
          ratio = 16.0f / 9;
          break;
        case 0x04:
          ratio = 2.21f;
          break;
      }
      if (ratio_info != sequence->ratio_info)
      {
        changed = true;
        sequence->ratio = ratio;
        sequence->ratio_info = ratio_info;
      }

      // frame rate
      // nal_start + 32 bits == frame_rate_code
      uint32_t fpsrate = sequence->fps_rate;
      uint32_t fpsscale = sequence->fps_scale;
      uint32_t rate_info = nal_bs_read(&bs, 4);

      switch (rate_info)
      {
        default:
        case 0x01:
          fpsrate = 24000;
          fpsscale = 1001;
          break;
        case 0x02:
          fpsrate = 24000;
          fpsscale = 1000;
          break;
        case 0x03:
          fpsrate = 25000;
          fpsscale = 1000;
          break;
        case 0x04:
          fpsrate = 30000;
          fpsscale = 1001;
          break;
        case 0x05:
          fpsrate = 30000;
          fpsscale = 1000;
          break;
        case 0x06:
          fpsrate = 50000;
          fpsscale = 1000;
          break;
        case 0x07:
          fpsrate = 60000;
          fpsscale = 1001;
          break;
        case 0x08:
          fpsrate = 60000;
          fpsscale = 1000;
          break;
      }

      if (fpsscale != sequence->fps_scale || fpsrate != sequence->fps_rate)
      {
        changed = true;
        sequence->fps_rate = fpsrate;
        sequence->fps_scale = fpsscale;
      }
    }
    nal_start = nal_end;
  }

  return changed;
}

#ifdef HAVE_LIBDOVI
// Processes Dolby Vision RPU
//   - Converts to profile 8.1 if `m_convert_dovi` is enabled
//   - Sets level 5 metadata to 0 offsets if `m_setDoviZeroLevel5` is enabled
//
// The returned data must be freed with `dovi_data_free`
// May be NULL if no processing was done or if parsing errored
const DoviData* CBitstreamConverter::processDoviRpu(uint8_t* buf, uint32_t nalSize)
{
  // early exit if no processing option is enabled
  if (!m_convert_dovi && !m_setDoviZeroLevel5)
    return NULL;

  DoviRpuOpaque* rpu = dovi_parse_unspec62_nalu(buf, nalSize);
  const DoviRpuDataHeader* header = dovi_rpu_get_header(rpu);
  const DoviData* rpuData = NULL;

  int ret = 0;
  bool processed = false;

  if (!header)
  {
    dovi_rpu_free(rpu);
    return rpuData;
  }

  if (m_convert_dovi && header->guessed_profile == 7)
  {
    ret = dovi_convert_rpu_with_mode(rpu, 2);
    processed = true;
  }

  if (ret == 0 && m_setDoviZeroLevel5)
  {
    ret = dovi_rpu_set_active_area_offsets(rpu, 0, 0, 0, 0);
    processed = true;
  }

  if (ret == 0 && processed)
    rpuData = dovi_write_unspec62_nalu(rpu);

  dovi_rpu_free_header(header);
  dovi_rpu_free(rpu);

  return rpuData;
}
#endif
