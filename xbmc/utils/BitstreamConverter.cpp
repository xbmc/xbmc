/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/log.h"

#include <assert.h>

#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif

#include "BitstreamConverter.h"
#include "BitstreamReader.h"
#include "BitstreamWriter.h"
#include "HevcSei.h"
#include "HDR10.h"
#include "HDR10Plus.h"
#include "HDR10PlusConvert.h"

#include "utils/StringUtils.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"

#include <algorithm>
#include <fmt/format.h>

extern "C"
{
#include <libavutil/mastering_display_metadata.h>
#ifdef HAVE_LIBDOVI
#include <libdovi/rpu_parser.h>
#endif
}

enum {
  AVC_NAL_SLICE=1,
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
  AVC_NAL_AUXILIARY_SLICE=19
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

enum {
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
static void nal_bs_init(nal_bitstream *bs, const uint8_t *data, size_t size)
{
  bs->data = data;
  bs->end  = data + size;
  bs->head = 0;
  // fill with something other than 0 to detect
  //  emulation prevention bytes
  bs->cache = 0xffffffff;
}

static uint32_t nal_bs_read(nal_bitstream *bs, int n)
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

static bool nal_bs_eos(nal_bitstream *bs)
{
  return (bs->data >= bs->end) && (bs->head == 0);
}

// read unsigned Exp-Golomb code
static int nal_bs_read_ue(nal_bitstream *bs)
{
  int i = 0;

  while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 31)
    i++;

  return ((1 << i) - 1 + nal_bs_read(bs, i));
}

// read signed Exp-Golomb code
static int nal_bs_read_se(nal_bitstream *bs)
{
  int i = 0;

  i = nal_bs_read_ue (bs);
  /* (-1)^(i+1) Ceil (i / 2) */
  i = (i + 1) / 2 * (i & 1 ? 1 : -1);

  return i;
}

static const uint8_t* avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *a = p + 4 - ((intptr_t)p & 3);

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
          return p+1;
      }
      if (p[3] == 0)
      {
        if (p[2] == 0 && p[4] == 1)
          return p+2;
        if (p[4] == 0 && p[5] == 1)
          return p+3;
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

static const uint8_t* avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *out = avc_find_startcode_internal(p, end);
  if (p<out && out<end && !out[-1])
    out--;
  return out;
}

static bool has_sei_recovery_point(const uint8_t *p, const uint8_t *end)
{
  int pt(0), ps(0), offset(1);

  do
  {
    pt = 0;
    do {
      pt += p[offset];
    } while (p[offset++] == 0xFF);

    ps = 0;
    do {
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
////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_LIBDOVI

// The returned data must be freed with `dovi_data_free`
// May be NULL if no conversion was done
static const DoviData* convert_dovi_rpu_nal(uint8_t* nal_buf, uint32_t nal_size, int mode, bool first_frame, DOVIELType& dovi_el_type)
{
  DoviRpuOpaque* rpuOpaque = dovi_parse_unspec62_nalu(nal_buf, nal_size);
  const DoviRpuDataHeader* header = dovi_rpu_get_header(rpuOpaque);
  const DoviData* rpu_data = NULL;

  if (header && header->guessed_profile == 7)
  {
    if (first_frame)
    {
      dovi_el_type = DOVIELType::TYPE_NONE;
      if (header->el_type)
      {
        if (StringUtils::EqualsNoCase(header->el_type, "FEL"))
          dovi_el_type = DOVIELType::TYPE_FEL;
        else if (StringUtils::EqualsNoCase(header->el_type, "MEL"))
          dovi_el_type = DOVIELType::TYPE_MEL;
      }
    }

    if (dovi_convert_rpu_with_mode(rpuOpaque, mode) >= 0)
      rpu_data = dovi_write_unspec62_nalu(rpuOpaque);
  }

  dovi_rpu_free_header(header);
  dovi_rpu_free(rpuOpaque);

  return rpu_data;
}

static void get_dovi_rpu_info(uint8_t* nal_buf, uint32_t nal_size, bool first_frame, DOVIELType& dovi_el_type, AVDOVIDecoderConfigurationRecord& dovi, double pts, CDataCacheCore& dataCacheCore)
{
  // https://professionalsupport.dolby.com/s/article/Dolby-Vision-Metadata-Levels?language=en_US

  DoviRpuOpaque* rpuOpaque = dovi_parse_unspec62_nalu(nal_buf, nal_size);

  const DoviVdrDmData* vdr_dm_data = dovi_rpu_get_vdr_dm_data(rpuOpaque);

  if (vdr_dm_data)
  {

    DOVIFrameMetadata dovi_frame_metadata;

    if (vdr_dm_data->dm_data.level1)
    {
      dovi_frame_metadata.level1_min_pq = vdr_dm_data->dm_data.level1->min_pq;
      dovi_frame_metadata.level1_max_pq = vdr_dm_data->dm_data.level1->max_pq;
      dovi_frame_metadata.level1_avg_pq = vdr_dm_data->dm_data.level1->avg_pq;
      dovi_frame_metadata.pts = pts;
    }

    if (vdr_dm_data->dm_data.level5)
    {
      dovi_frame_metadata.has_level5_metadata = true;
      dovi_frame_metadata.level5_active_area_left_offset = vdr_dm_data->dm_data.level5->active_area_left_offset;
      dovi_frame_metadata.level5_active_area_right_offset = vdr_dm_data->dm_data.level5->active_area_right_offset;
      dovi_frame_metadata.level5_active_area_top_offset = vdr_dm_data->dm_data.level5->active_area_top_offset;
      dovi_frame_metadata.level5_active_area_bottom_offset = vdr_dm_data->dm_data.level5->active_area_bottom_offset;
    }

    dataCacheCore.SetVideoDoViFrameMetadata(dovi_frame_metadata);
  }

  if (first_frame) {

    DOVIStreamMetadata dovi_stream_metadata;

    if (vdr_dm_data)
    {
      dovi_stream_metadata.source_min_pq = vdr_dm_data->source_min_pq;
      dovi_stream_metadata.source_max_pq = vdr_dm_data->source_max_pq;
    }

    if (vdr_dm_data && vdr_dm_data->dm_data.level6)
    {
      dovi_stream_metadata.has_level6_metadata = true;

      dovi_stream_metadata.level6_max_lum = vdr_dm_data->dm_data.level6->max_display_mastering_luminance;
      dovi_stream_metadata.level6_min_lum = vdr_dm_data->dm_data.level6->min_display_mastering_luminance;

      dovi_stream_metadata.level6_max_cll = vdr_dm_data->dm_data.level6->max_content_light_level;
      dovi_stream_metadata.level6_max_fall = vdr_dm_data->dm_data.level6->max_frame_average_light_level;
    }

    std::string meta_version = "";
    if (vdr_dm_data && vdr_dm_data->dm_data.level254)
    {
      unsigned int noL8 = vdr_dm_data->dm_data.level8.len;
      if (noL8 > 0)
        meta_version = fmt::format("CMv4.0 {}-{} {}-L8",
                                  vdr_dm_data->dm_data.level254->dm_version_index,
                                  vdr_dm_data->dm_data.level254->dm_mode,
                                  noL8);
      else
        meta_version = fmt::format("CMv4.0 {}-{}",
                                  vdr_dm_data->dm_data.level254->dm_version_index,
                                  vdr_dm_data->dm_data.level254->dm_mode);
    }
    else if (vdr_dm_data && vdr_dm_data->dm_data.level1)
    {
      unsigned int noL2 = vdr_dm_data->dm_data.level2.len;
      if (noL2 > 0)
        meta_version = fmt::format("CMv2.9 {}-L2", noL2);
      else
        meta_version = "CMv2.9";
    }
    dovi_stream_metadata.meta_version = meta_version;
    dataCacheCore.SetVideoDoViStreamMetadata(dovi_stream_metadata);

    DOVIStreamInfo dovi_stream_info;
    const DoviRpuDataHeader* header = dovi_rpu_get_header(rpuOpaque);
    dovi_el_type = DOVIELType::TYPE_NONE;

    if (header && ((header->guessed_profile == 4) || (header->guessed_profile == 7)) && header->el_type)
    {
      if (StringUtils::EqualsNoCase(header->el_type, "FEL"))
        dovi_el_type = DOVIELType::TYPE_FEL;
      else if (StringUtils::EqualsNoCase(header->el_type, "MEL"))
        dovi_el_type = DOVIELType::TYPE_MEL;
    }

    dovi_stream_info.dovi_el_type = dovi_el_type;
    dovi_stream_info.dovi = dovi;

    dovi_stream_info.has_config = (memcmp(&dovi, &CDVDStreamInfo::empty_dovi, sizeof(AVDOVIDecoderConfigurationRecord)) != 0);
    dovi_stream_info.has_header = (header != 0);

    dataCacheCore.SetVideoDoViStreamInfo(dovi_stream_info);
    dovi_rpu_free_header(header);
  }

  dovi_rpu_free_vdr_dm_data(vdr_dm_data);
  dovi_rpu_free(rpuOpaque);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CBitstreamParser::CBitstreamParser() = default;

void CBitstreamParser::Close()
{
}

bool CBitstreamParser::CanStartDecode(const uint8_t *buf, int buf_size)
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
CBitstreamConverter::CBitstreamConverter(CDVDStreamInfo& hints)
                        : m_hints(hints)
                        , m_dataCacheCore(CServiceBroker::GetDataCacheCore())
{
  m_convert_bitstream = false;
  m_convertBuffer     = NULL;
  m_convertSize       = 0;
  m_inputBuffer       = NULL;
  m_inputSize         = 0;
  m_to_annexb = false;
  m_convert_3byteTo4byteNALSize = false;
  m_convert_bytestream = false;
  m_sps_pps_context.sps_pps_data = NULL;
  m_start_decode = false;
  m_convert_dovi = DOVIMode::MODE_NONE;
  m_convert_Hdr10Plus = false;
  m_prefer_Hdr10Plus_conversion = false;
  m_dual_priority_Hdr10Plus = false;
  m_removeDovi = false;
  m_removeHdr10Plus = false;
  m_combine = false;
  m_first_frame = true;
  m_hdrStaticMetadataInfo = {};
  m_dataCacheCore.SetVideoSourceHdrType(m_hints.hdrType);
}

CBitstreamConverter::~CBitstreamConverter()
{
  Close();
}

bool CBitstreamConverter::Open(bool to_annexb)
{
  m_to_annexb = to_annexb;
  m_codec = m_hints.codec;
  m_intial_hdrType = m_hints.hdrType;
  uint8_t *in_extradata = m_hints.extradata.GetData();
  int in_extrasize = m_hints.extradata.GetSize();

  switch(m_codec)
  {
    case AV_CODEC_ID_H264:
      if (in_extrasize < 7 || in_extradata == NULL)
      {
        CLog::Log(LOGERROR, "CBitstreamConverter::Open avcC data too small or missing");
        return false;
      }
      // valid avcC data (bitstream) always starts with the value 1 (version)
      if(m_to_annexb)
      {
        if ( in_extradata[0] == 1 )
        {
          CLog::Log(LOGINFO, "CBitstreamConverter::Open bitstream to annexb init");
          m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
          m_convert_bitstream =
              BitstreamConvertInitAVC(m_extraData.GetData(), m_extraData.GetSize());
          return true;
        }
        else
          CLog::Log(LOGINFO, "CBitstreamConverter::Open Invalid avcC");
      }
      else
      {
        // valid avcC atom data always starts with the value 1 (version)
        if ( in_extradata[0] != 1 )
        {
          if ( (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 0 && in_extradata[3] == 1) ||
               (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 1) )
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init");
            // video content is from x264 or from bytestream h264 (AnnexB format)
            // NAL reformatting to bitstream format needed
            AVIOContext *pb;
            if (avio_open_dyn_buf(&pb) < 0)
              return false;
            m_convert_bytestream = true;
            // create a valid avcC atom data from ffmpeg's extradata
            isom_write_avcc(pb, in_extradata, in_extrasize);
            // unhook from ffmpeg's extradata
            in_extradata = NULL;
            // extract the avcC atom data into extradata then write it into avcCData for VDADecoder
            in_extrasize = avio_close_dyn_buf(pb, &in_extradata);
            // make a copy of extradata contents
            m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
            // done with the converted extradata, we MUST free using av_free
            av_free(in_extradata);
            return true;
          }
          else
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open invalid avcC atom data");
            return false;
          }
        }
        else
        {
          if (in_extradata[4] == 0xFE)
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init 3 byte to 4 byte nal");
            // video content is from so silly encoder that think 3 byte NAL sizes
            // are valid, setup to convert 3 byte NAL sizes to 4 byte.
            in_extradata[4] = 0xFF;
            m_convert_3byteTo4byteNALSize = true;

            m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
            return true;
          }
        }
        // valid avcC atom
        m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
        return true;
      }
      return false;
      break;
    case AV_CODEC_ID_HEVC:
      if (in_extrasize < 23 || in_extradata == NULL)
      {
        CLog::Log(LOGERROR, "CBitstreamConverter::Open hvcC data too small or missing");
        return false;
      }
      // valid hvcC data (bitstream) always starts with the value 1 (version)
      if(m_to_annexb)
      {
       /** @todo from Amlogic
        * It seems the extradata is encoded as hvcC format.
        * Temporarily, we support configurationVersion==0 until 14496-15 3rd
        * is finalized. When finalized, configurationVersion will be 1 and we
        * can recognize hvcC by checking if extradata[0]==1 or not.
        */

        if (in_extradata[0] || in_extradata[1] || in_extradata[2] > 1)
        {
          CLog::Log(LOGINFO, "CBitstreamConverter::Open bitstream to annexb init");
          m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
          m_convert_bitstream =
              BitstreamConvertInitHEVC(m_extraData.GetData(), m_extraData.GetSize());
          return true;
        }
        else
          CLog::Log(LOGINFO, "CBitstreamConverter::Open Invalid hvcC");
      }
      else
      {
        // valid hvcC atom data always starts with the value 1 (version)
        if ( in_extradata[0] != 1 )
        {
          if ( (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 0 && in_extradata[3] == 1) ||
               (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 1) )
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init");
            //! @todo convert annexb to bitstream format
            return false;
          }
          else
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open invalid hvcC atom data");
            return false;
          }
        }
        else
        {
          if ((in_extradata[4] & 0x3) == 2)
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init 3 byte to 4 byte nal");
            // video content is from so silly encoder that think 3 byte NAL sizes
            // are valid, setup to convert 3 byte NAL sizes to 4 byte.
            in_extradata[4] |= 0x03;
            m_convert_3byteTo4byteNALSize = true;
          }
        }
        // valid hvcC atom
        m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
        return true;
      }
      return false;
      break;
    default:
      return false;
      break;
  }
  return false;
}

void CBitstreamConverter::Close(void)
{
  if (m_sps_pps_context.sps_pps_data)
    av_free(m_sps_pps_context.sps_pps_data), m_sps_pps_context.sps_pps_data = NULL;

  if (m_convertBuffer)
    av_free(m_convertBuffer), m_convertBuffer = NULL;
  m_convertSize = 0;

  m_extraData = {};

  m_inputSize = 0;
  m_inputBuffer = NULL;

  m_convert_bitstream = false;
  m_convert_bytestream = false;
  m_convert_3byteTo4byteNALSize = false;
  m_combine = false;
}

bool CBitstreamConverter::Convert(uint8_t *pData, int iSize, double pts)
{
  if (m_convertBuffer)
  {
    av_free(m_convertBuffer);
    m_convertBuffer = NULL;
  }
  m_inputSize = 0;
  m_convertSize = 0;
  m_inputBuffer = NULL;

  if (pData)
  {
    if (m_codec == AV_CODEC_ID_H264 ||
        m_codec == AV_CODEC_ID_HEVC)
    {
      if (m_to_annexb)
      {
        int demuxer_bytes = iSize;
        uint8_t *demuxer_content = pData;

        if (m_convert_bitstream)
        {
          // convert demuxer packet from bitstream to bytestream (AnnexB)
          int bytestream_size = 0;
          uint8_t *bytestream_buff = NULL;

          BitstreamConvert(demuxer_content, demuxer_bytes, &bytestream_buff, &bytestream_size, pts);
          if (bytestream_buff && (bytestream_size > 0))
          {
            m_convertSize   = bytestream_size;
            m_convertBuffer = bytestream_buff;
            return true;
          }
          else
          {
            m_convertSize = 0;
            m_convertBuffer = NULL;
            CLog::Log(LOGERROR, "CBitstreamConverter::Convert: error converting.");
            return false;
          }
        }
        else
        {
          m_inputSize = iSize;
          m_inputBuffer = pData;
          m_start_decode = true; // TODO: should really wait for IDR even though not converting.
          return true;
        }
      }
      else
      {
        m_inputSize = iSize;
        m_inputBuffer = pData;

        if (m_convert_bytestream)
        {
          if(m_convertBuffer)
          {
            av_free(m_convertBuffer);
            m_convertBuffer = NULL;
          }
          m_convertSize = 0;

          // convert demuxer packet from bytestream (AnnexB) to bitstream
          AVIOContext *pb;

          if(avio_open_dyn_buf(&pb) < 0)
          {
            return false;
          }
          m_convertSize = avc_parse_nal_units(pb, pData, iSize);
          m_convertSize = avio_close_dyn_buf(pb, &m_convertBuffer);
        }
        else if (m_convert_3byteTo4byteNALSize)
        {
          if(m_convertBuffer)
          {
            av_free(m_convertBuffer);
            m_convertBuffer = NULL;
          }
          m_convertSize = 0;

          // convert demuxer packet from 3 byte NAL sizes to 4 byte
          AVIOContext *pb;
          if (avio_open_dyn_buf(&pb) < 0)
            return false;

          uint32_t nal_size;
          uint8_t *end = pData + iSize;
          uint8_t *nal_start = pData;
          while (nal_start < end)
          {
            nal_size = BS_RB24(nal_start);
            avio_wb32(pb, nal_size);
            nal_start += 3;
            avio_write(pb, nal_start, nal_size);
            nal_start += nal_size;
          }

          m_convertSize = avio_close_dyn_buf(pb, &m_convertBuffer);
        }
        return true;
      }
    }
  }

  return false;
}

bool CBitstreamConverter::Convert(uint8_t *pData_bl, int iSize_bl, uint8_t *pData_el, int iSize_el, double pts)
{
  if (m_convertBuffer)
  {
    av_free(m_convertBuffer);
    m_convertBuffer = NULL;
  }
  m_inputSize = 0;
  m_convertSize = 0;
  m_inputBuffer = NULL;

  if (pData_bl && pData_el)
  {
    uint32_t offset = 0, size_eos;
    uint8_t *buf=NULL, *end, *start, *buf_eos=NULL;

    uint32_t bl_frame_nal_buf_size = iSize_bl;
    uint32_t el_frame_nal_buf_size = iSize_el;
    if (!m_convert_bitstream)
    {
      AVIOContext *pb;

      if (avio_open_dyn_buf(&pb) < 0)
        return false;

      bl_frame_nal_buf_size = avc_parse_nal_units(pb, pData_bl, iSize_bl);
      el_frame_nal_buf_size = avc_parse_nal_units(pb, pData_el, iSize_el);
      avio_close_dyn_buf(pb, &buf);
    }
    else
      buf = pData_bl;

    Hdr10PlusMetadata hdr10plus_meta;
    bool convert_hdr10plus_meta = false;

    // process bl frame data
    start = buf;
    end = buf + bl_frame_nal_buf_size;
    while (end - buf > 4)
    {
      uint32_t size;
      uint8_t nal_type;
      size = std::min<uint32_t>(BS_RB32(buf), end - buf - 4);
      buf += 4;
      nal_type = (buf[0] >> 1) & 0x3f;

      switch (nal_type) {

        case HEVC_NAL_SEI_PREFIX:
          ProcessSeiPrefixWrap(buf, size, &m_convertBuffer, offset, hdr10plus_meta, convert_hdr10plus_meta);
          break;

        case AVC_NAL_END_SEQUENCE:
          buf_eos = buf;
          size_eos = size;
          break;

        default:
          if (!m_start_decode && IsIDR(nal_type)) m_start_decode = true;
          BitstreamAllocAndCopy(&m_convertBuffer, &offset, buf, size, nal_type);
          break;
      }

      // Make sure bl_present_flag is set.
      m_hints.dovi.bl_present_flag = true;

      CLog::Log(LOGDEBUG, LOGVIDEO, "CBitstreamConverter::Convert: DT-DL BL nal_type: [{}], size: [{}]", nal_type, size);

      buf += size;
    }

    if (m_convert_bitstream)
      buf = pData_el;

    // process el frame data
    end = buf + el_frame_nal_buf_size;
    while (end - buf > 4)
    {
      uint32_t size;
      uint8_t nal_type;
      size = std::min<uint32_t>(BS_RB32(buf), end - buf - 4);
      buf += 4;
      nal_type = (buf[0] >> 1) & 0x3f;

      switch (nal_type) {

        case HEVC_NAL_UNSPEC62: // DoVi RPU
          if (!m_removeDovi && !convert_hdr10plus_meta)
            ProcessDoViRpuWrap(buf, size, &m_convertBuffer, offset, pts);
          break;

        default: // Package other data into HEVC_NAL_UNSPEC63 DoVi EL
          if (!m_removeDovi && !convert_hdr10plus_meta && (m_convert_dovi == DOVIMode::MODE_NONE))
            BitstreamAllocAndCopy(&m_convertBuffer, &offset, buf, size, HEVC_NAL_UNSPEC63);
          break;
      }

      // Make sure el_present_flag is set.
      m_hints.dovi.el_present_flag = true;

      CLog::Log(LOGDEBUG, LOGVIDEO, "CBitstreamConverter::Convert: DT-DL EL nal_type: [{}], size: [{}]", nal_type, size);

      buf += size;
    }

    // If converting hdr10plus - add the DoVi RPU as the last NALU in the access unit.
    if (convert_hdr10plus_meta)
      AddDoViRpuNaluWrap(hdr10plus_meta, &m_convertBuffer, offset, pts);

    // append end of sequence if exist
    if (buf_eos)
      BitstreamAllocAndCopy(&m_convertBuffer, &offset, buf_eos, size_eos, AVC_NAL_END_SEQUENCE);

    if (!m_convert_bitstream)
      av_free(start);

    m_convertSize = offset;
    m_combine = true;
  }

  m_first_frame = false;
  return true;
}

uint8_t *CBitstreamConverter::GetConvertBuffer() const
{
  if((m_convert_bitstream || m_convert_bytestream || m_convert_3byteTo4byteNALSize || m_combine) && m_convertBuffer != NULL)
    return m_convertBuffer;
  else
    return m_inputBuffer;
}

int CBitstreamConverter::GetConvertSize() const
{
  if((m_convert_bitstream || m_convert_bytestream || m_convert_3byteTo4byteNALSize || m_combine) && m_convertBuffer != NULL)
    return m_convertSize;
  else
    return m_inputSize;
}

uint8_t* CBitstreamConverter::GetExtraData()
{
  if (m_convert_bitstream)
    return m_sps_pps_context.sps_pps_data;
  else
    return m_extraData.GetData();
}
const uint8_t* CBitstreamConverter::GetExtraData() const
{
  if(m_convert_bitstream)
    return m_sps_pps_context.sps_pps_data;
  else
    return m_extraData.GetData();
}
int CBitstreamConverter::GetExtraSize() const
{
  if(m_convert_bitstream)
    return m_sps_pps_context.size;
  else
    return m_extraData.GetSize();
}

void CBitstreamConverter::ResetStartDecode(void)
{
  m_start_decode = false;
}

bool CBitstreamConverter::CanStartDecode() const
{
  return m_start_decode;
}

bool CBitstreamConverter::BitstreamConvertInitAVC(void *in_extradata, int in_extrasize)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  m_sps_pps_size = 0;
  m_sps_pps_context.sps_pps_data = NULL;

  // nothing to filter
  if (!in_extradata || in_extrasize < 6)
    return false;

  uint16_t unit_size;
  uint32_t total_size = 0;
  uint8_t *out = NULL, unit_nb, sps_done = 0, sps_seen = 0, pps_seen = 0;
  uint8_t mvc_done = 0;
  const uint8_t *extradata = (uint8_t*)in_extradata + 4;
  static const uint8_t nalu_header[4] = {0, 0, 0, 1};

  // retrieve length coded size
  m_sps_pps_context.length_size = (*extradata++ & 0x3) + 1;

  // retrieve sps and pps unit(s)
  unit_nb = *extradata++ & 0x1f;  // number of sps unit(s)
  if (!unit_nb)
  {
    goto pps;
  }
  else
  {
    sps_seen = 1;
  }

  while (unit_nb--)
  {
    void *tmp;

    unit_size = extradata[0] << 8 | extradata[1];
    total_size += unit_size + 4;

    if (total_size > INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE ||
      (extradata + 2 + unit_size) > ((uint8_t*)in_extradata + in_extrasize))
    {
      av_free(out);
      return false;
    }
    tmp = av_realloc(out, total_size + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!tmp)
    {
      av_free(out);
      return false;
    }
    out = (uint8_t*)tmp;
    memcpy(out + total_size - unit_size - 4, nalu_header, 4);
    memcpy(out + total_size - unit_size, extradata + 2, unit_size);
    extradata += 2 + unit_size;

pps:
    if (!unit_nb && !sps_done++)
    {
      unit_nb = *extradata++;   // number of pps unit(s)
      if (unit_nb)
        pps_seen = 1;
    }

    if (!unit_nb && !mvc_done++)
    {
      if (in_extrasize - total_size > 14 && memcmp(extradata + 8, "mvcC", 4) == 0)
      {
        // start over; take SPS and PPS from the mvcC atom
        extradata += 12 + 5; // skip over mvcC atom header
        unit_nb = *extradata++ & 0x1f;  // number of sps unit(s)
        sps_done = 0;
        pps_seen = 0;
      }
    }
  }

  if (out)
    memset(out + total_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

  if (!sps_seen)
      CLog::Log(LOGDEBUG, "SPS NALU missing or invalid. The resulting stream may not play");
  if (!pps_seen)
      CLog::Log(LOGDEBUG, "PPS NALU missing or invalid. The resulting stream may not play");

  m_sps_pps_context.sps_pps_data = out;
  m_sps_pps_context.size = total_size;
  m_sps_pps_context.first_idr = 1;
  m_sps_pps_context.idr_sps_pps_seen = 0;

  return true;
}

bool CBitstreamConverter::BitstreamConvertInitHEVC(void *in_extradata, int in_extrasize)
{
  m_sps_pps_size = 0;
  m_sps_pps_context.sps_pps_data = NULL;

  // nothing to filter
  if (!in_extradata || in_extrasize < 23)
    return false;

  uint16_t unit_nb, unit_size;
  uint32_t total_size = 0;
  uint8_t *out = NULL, array_nb, nal_type, sps_seen = 0, pps_seen = 0;
  const uint8_t *extradata = (uint8_t*)in_extradata + 21;
  static const uint8_t nalu_header[4] = {0, 0, 0, 1};

  // retrieve length coded size
  m_sps_pps_context.length_size = (*extradata++ & 0x3) + 1;

  array_nb = *extradata++;
  while (array_nb--)
  {
    nal_type = *extradata++ & 0x3f;
    unit_nb  = extradata[0] << 8 | extradata[1];
    extradata += 2;

    if (nal_type == HEVC_NAL_SPS && unit_nb)
    {
        sps_seen = 1;
    }
    else if (nal_type == HEVC_NAL_PPS && unit_nb)
    {
        pps_seen = 1;
    }
    while (unit_nb--)
    {
      void *tmp;

      unit_size = extradata[0] << 8 | extradata[1];
      extradata += 2;
      if (nal_type != HEVC_NAL_SPS &&
          nal_type != HEVC_NAL_PPS &&
          nal_type != HEVC_NAL_VPS)
      {
        extradata += unit_size;
        continue;
      }
      total_size += unit_size + 4;

      if (total_size > INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE ||
        (extradata + unit_size) > ((uint8_t*)in_extradata + in_extrasize))
      {
        av_free(out);
        return false;
      }
      tmp = av_realloc(out, total_size + AV_INPUT_BUFFER_PADDING_SIZE);
      if (!tmp)
      {
        av_free(out);
        return false;
      }
      out = (uint8_t*)tmp;
      memcpy(out + total_size - unit_size - 4, nalu_header, 4);
      memcpy(out + total_size - unit_size, extradata, unit_size);
      extradata += unit_size;
    }
  }

  if (out)
    memset(out + total_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

  if (!sps_seen)
      CLog::Log(LOGDEBUG, "SPS NALU missing or invalid. The resulting stream may not play");
  if (!pps_seen)
      CLog::Log(LOGDEBUG, "PPS NALU missing or invalid. The resulting stream may not play");

  m_sps_pps_context.sps_pps_data = out;
  m_sps_pps_context.size = total_size;
  m_sps_pps_context.first_idr = 1;
  m_sps_pps_context.idr_sps_pps_seen = 0;

  return true;
}

bool CBitstreamConverter::IsIDR(uint8_t unit_type)
{
  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      return unit_type == AVC_NAL_IDR_SLICE;
    case AV_CODEC_ID_HEVC:
      return unit_type == HEVC_NAL_IDR_W_RADL ||
             unit_type == HEVC_NAL_IDR_N_LP ||
             unit_type == HEVC_NAL_CRA_NUT;
    default:
      return false;
  }
}

bool CBitstreamConverter::IsSlice(uint8_t unit_type)
{
  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      return unit_type == AVC_NAL_SLICE;
    case AV_CODEC_ID_HEVC:
      return unit_type == HEVC_NAL_TRAIL_R ||
             unit_type == HEVC_NAL_TRAIL_N ||
             unit_type == HEVC_NAL_TSA_N ||
             unit_type == HEVC_NAL_TSA_R ||
             unit_type == HEVC_NAL_STSA_N ||
             unit_type == HEVC_NAL_STSA_R ||
             unit_type == HEVC_NAL_BLA_W_LP ||
             unit_type == HEVC_NAL_BLA_W_RADL ||
             unit_type == HEVC_NAL_BLA_N_LP ||
             unit_type == HEVC_NAL_CRA_NUT ||
             unit_type == HEVC_NAL_RADL_N ||
             unit_type == HEVC_NAL_RADL_R ||
             unit_type == HEVC_NAL_RASL_N ||
             unit_type == HEVC_NAL_RASL_R;
    default:
      return false;
  }
}

void CBitstreamConverter::ApplyMasteringDisplayColourVolume(const MasteringDisplayColourVolume& metadata, bool& update) {

  if ((m_hdrStaticMetadataInfo.max_lum != metadata.maxLuminance) ||
      (m_hdrStaticMetadataInfo.min_lum != metadata.minLuminance) ||
      !m_hdrStaticMetadataInfo.has_mdcv_metadata)
  {
    m_hdrStaticMetadataInfo.has_mdcv_metadata = true;
    m_hdrStaticMetadataInfo.max_lum = metadata.maxLuminance;
    m_hdrStaticMetadataInfo.min_lum = metadata.minLuminance;
    m_hdrStaticMetadataInfo.colour_primaries = MasteringDisplayColourVolumeText(metadata);
    update = true;

    CLog::Log(LOGINFO, "CBitstreamConverter::ApplyMasteringDisplayColourVolume [{}] [{}]",  m_hdrStaticMetadataInfo.max_lum, m_hdrStaticMetadataInfo.min_lum);
  }
}

void CBitstreamConverter::ApplyContentLightLevel(const ContentLightLevel& metadata, bool& update) {

  if ((m_hdrStaticMetadataInfo.max_cll != metadata.maxContentLightLevel) ||
      (m_hdrStaticMetadataInfo.max_fall != metadata.maxFrameAverageLightLevel) ||
      !m_hdrStaticMetadataInfo.has_cll_metadata)
  {
    m_hdrStaticMetadataInfo.has_cll_metadata = true;
    m_hdrStaticMetadataInfo.max_cll = metadata.maxContentLightLevel;
    m_hdrStaticMetadataInfo.max_fall = metadata.maxFrameAverageLightLevel;
    update = true;

    CLog::Log(LOGINFO, "CBitstreamConverter::ApplyContentLightLevel [{}] [{}]", m_hdrStaticMetadataInfo.max_cll, m_hdrStaticMetadataInfo.max_fall);
  }
}

void CBitstreamConverter::UpdateHdrStaticMetadata() {

  HDRStaticMetadataInfo hdrStaticMetadataInfo;

  hdrStaticMetadataInfo.has_mdcv_metadata = m_hdrStaticMetadataInfo.has_mdcv_metadata;
  hdrStaticMetadataInfo.max_lum = m_hdrStaticMetadataInfo.max_lum;
  hdrStaticMetadataInfo.min_lum = m_hdrStaticMetadataInfo.min_lum;
  hdrStaticMetadataInfo.colour_primaries = m_hdrStaticMetadataInfo.colour_primaries;

  hdrStaticMetadataInfo.has_cll_metadata = m_hdrStaticMetadataInfo.has_cll_metadata;
  hdrStaticMetadataInfo.max_cll = m_hdrStaticMetadataInfo.max_cll;
  hdrStaticMetadataInfo.max_fall = m_hdrStaticMetadataInfo.max_fall;

  m_dataCacheCore.SetVideoHDRStaticMetadataInfo(hdrStaticMetadataInfo);
}

void CBitstreamConverter::AddDoViRpuNaluWrap(const Hdr10PlusMetadata& meta, uint8_t **poutbuf, uint32_t& poutbuf_size, double pts) {

  int int_poutbuf_size = poutbuf_size;
  AddDoViRpuNalu(meta, poutbuf, &int_poutbuf_size, pts);
  poutbuf_size = static_cast<uint32_t>(int_poutbuf_size);
}

void CBitstreamConverter::AddDoViRpuNalu(const Hdr10PlusMetadata& meta, uint8_t **poutbuf, int *poutbuf_size, double pts) {

  auto nalu = create_rpu_nalu_for_hdr10plus(
    meta,
    m_convert_Hdr10Plus_peak_brightness_source,
    m_hdrStaticMetadataInfo);

  if (!nalu.empty())
  {
    if (m_first_frame) {
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

#ifdef HAVE_LIBDOVI
    get_dovi_rpu_info(nalu.data(), nalu.size(), m_first_frame, m_hints.dovi_el_type, m_hints.dovi, pts, m_dataCacheCore);
#endif

    BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, nalu.data(), nalu.size(), HEVC_NAL_UNSPEC62);
    nalu.clear();
  }
}

void CBitstreamConverter::ProcessSeiPrefixWrap(uint8_t *buf, int32_t nal_size, uint8_t **poutbuf, uint32_t& poutbuf_size, Hdr10PlusMetadata& meta, bool& convert_hdr10plus_meta) {

  int int_poutbuf_size = poutbuf_size;
  ProcessSeiPrefix(buf, nal_size, poutbuf, &int_poutbuf_size, meta, convert_hdr10plus_meta);
  poutbuf_size = static_cast<uint32_t>(int_poutbuf_size);
}

void CBitstreamConverter::ProcessSeiPrefix(uint8_t *buf, int32_t nal_size, uint8_t **poutbuf, int *poutbuf_size, Hdr10PlusMetadata& meta, bool& convert_hdr10plus_meta) {

  bool copy = true;

  std::vector<uint8_t> clearBuf;
  auto messages = CHevcSei::ParseSeiRbspUnclearedEmulation(buf, nal_size, clearBuf);

  bool updateMetadata = false;

  if (auto colourVolume = CHevcSei::ExtractMasteringDisplayColourVolume(messages, clearBuf))
    ApplyMasteringDisplayColourVolume(colourVolume.value(), updateMetadata);

  if (auto lightLevel = CHevcSei::ExtractContentLightLevel(messages, clearBuf))
    ApplyContentLightLevel(lightLevel.value(), updateMetadata);

  if (updateMetadata) UpdateHdrStaticMetadata();

  if (auto res = CHevcSei::ExtractHdr10Plus(messages, clearBuf)) {

    bool isDual = (m_intial_hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION); // Original is DV and now also found HDR10+ so is dual.
    bool considerAsHdr10Plus = (!isDual || m_dual_priority_Hdr10Plus || m_prefer_Hdr10Plus_conversion);

    if (m_first_frame) {
      if (considerAsHdr10Plus) {
        m_hints.hdrType = StreamHdrType::HDR_TYPE_HDR10PLUS;
        m_dataCacheCore.SetVideoSourceHdrType(StreamHdrType::HDR_TYPE_HDR10PLUS);
        if (isDual) m_dataCacheCore.SetVideoSourceAdditionalHdrType(StreamHdrType::HDR_TYPE_DOLBYVISION);
      } else {
        if (isDual) m_dataCacheCore.SetVideoSourceAdditionalHdrType(StreamHdrType::HDR_TYPE_HDR10PLUS);
      }
    }

    bool convert = (considerAsHdr10Plus && m_convert_Hdr10Plus && !m_dual_priority_Hdr10Plus);

    if (convert) {
      meta = res.value();
      convert_hdr10plus_meta = true;
    }

    if (convert || m_removeHdr10Plus) {
      // Remove and carry forward remaining sei in nalu.
      auto nalu = CHevcSei::RemoveHdr10PlusFromSeiNalu(buf, nal_size);
      if (!nalu.empty())
      {
        BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, nalu.data(), nalu.size(), HEVC_NAL_SEI_PREFIX);
        nalu.clear();
      }
      copy = false;
    }
  }

  if (copy) BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size, HEVC_NAL_SEI_PREFIX);
}

void CBitstreamConverter::ProcessDoViRpuWrap(uint8_t *nal_buf, int32_t nal_size, uint8_t **poutbuf, uint32_t& poutbuf_size, double pts) {

  int int_poutbuf_size = poutbuf_size;
  ProcessDoViRpu(nal_buf, nal_size, poutbuf, &int_poutbuf_size, pts);
  poutbuf_size = static_cast<uint32_t>(int_poutbuf_size);
}

void CBitstreamConverter::ProcessDoViRpu(uint8_t *nal_buf, int32_t nal_size, uint8_t **poutbuf, int *poutbuf_size, double pts) {

#ifdef HAVE_LIBDOVI
  const DoviData* rpu_data = NULL;
  if (m_convert_dovi != DOVIMode::MODE_NONE) {
    DOVIELType dovi_el_type = DOVIELType::TYPE_NONE;
    rpu_data = convert_dovi_rpu_nal(nal_buf, nal_size, m_convert_dovi, m_first_frame, dovi_el_type);
    if (rpu_data)
    {
      nal_buf = const_cast<uint8_t*>(rpu_data->data);
      nal_size = rpu_data->len;

      // Capture the DOVI source details - about to be replaced.
      if (m_first_frame)
      {
        DOVIStreamInfo dovi_stream_info;
        dovi_stream_info.dovi_el_type = dovi_el_type;
        dovi_stream_info.dovi = m_hints.dovi;
        m_dataCacheCore.SetVideoSourceDoViStreamInfo(dovi_stream_info);
      }

      m_hints.dovi.el_present_flag = 0; // EL removed in both converstion cases - to MEL and to P8.1
      if (m_convert_dovi == DOVIMode::MODE_TO81) {
        m_hints.dovi.dv_profile = 8;
        m_hints.dovi.dv_bl_signal_compatibility_id = 1;
      }
    }
  }
  get_dovi_rpu_info(nal_buf, nal_size, m_first_frame, m_hints.dovi_el_type, m_hints.dovi, pts, m_dataCacheCore);
#endif

  BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, nal_buf, nal_size, HEVC_NAL_UNSPEC62);

#ifdef HAVE_LIBDOVI
  if (rpu_data) dovi_data_free(rpu_data);
#endif
}

bool CBitstreamConverter::BitstreamConvert(uint8_t* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size, double pts)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  int i;
  uint8_t *buf = pData;
  uint32_t buf_size = iSize;
  uint8_t  unit_type, nal_sps, nal_pps, nal_sei;
  int32_t  nal_size;
  uint32_t cumul_size = 0;
  const uint8_t *buf_end = buf + buf_size;

  Hdr10PlusMetadata hdr10plus_meta;
  bool convert_hdr10plus_meta = false;

  std::vector<uint8_t> finalPrefixSeiNalu;

  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      nal_sps = AVC_NAL_SPS;
      nal_pps = AVC_NAL_PPS;
      nal_sei = AVC_NAL_SEI;
      break;
    case AV_CODEC_ID_HEVC:
      nal_sps = HEVC_NAL_SPS;
      nal_pps = HEVC_NAL_PPS;
      nal_sei = HEVC_NAL_SEI_PREFIX;
      break;
    default:
      return false;
  }

  do
  {
    if (buf + m_sps_pps_context.length_size > buf_end)
      goto fail;

    for (nal_size = 0, i = 0; i < m_sps_pps_context.length_size; i++)
      nal_size = (nal_size << 8) | buf[i];

    buf += m_sps_pps_context.length_size;
    if (m_codec == AV_CODEC_ID_H264)
    {
        unit_type = *buf & 0x1f;
    }
    else
    {
        unit_type = (*buf >> 1) & 0x3f;
    }

    if (buf + nal_size > buf_end || nal_size <= 0)
      goto fail;

    // Don't add sps/pps if the unit already contain them
    if (m_sps_pps_context.first_idr && (unit_type == nal_sps || unit_type == nal_pps))
      m_sps_pps_context.idr_sps_pps_seen = 1;

    if (!m_start_decode && IsIDR(unit_type))
      m_start_decode = true;

    // prepend only to the first access unit of an IDR picture, if no sps/pps already present
    if (m_sps_pps_context.first_idr && IsIDR(unit_type) && !m_sps_pps_context.idr_sps_pps_seen)
    {
      BitstreamAllocAndCopy(poutbuf, poutbuf_size, m_sps_pps_context.sps_pps_data,
                            m_sps_pps_context.size, buf, nal_size, unit_type);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {

      if (!m_sps_pps_context.first_idr && IsSlice(unit_type))
      {
          m_sps_pps_context.first_idr = 1;
          m_sps_pps_context.idr_sps_pps_seen = 0;
      }

      switch (unit_type) {

        case HEVC_NAL_SEI_PREFIX:
          ProcessSeiPrefix(buf, nal_size, poutbuf, poutbuf_size, hdr10plus_meta, convert_hdr10plus_meta);
          break;

        case HEVC_NAL_UNSPEC62: // DoVi RPU
          if (!m_removeDovi && !convert_hdr10plus_meta)
            ProcessDoViRpu(buf, nal_size, poutbuf, poutbuf_size, pts);
          break;

        case HEVC_NAL_UNSPEC63: // DoVi EL
          if (!m_removeDovi && !convert_hdr10plus_meta && (m_convert_dovi == DOVIMode::MODE_NONE))
            BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size, unit_type);
          break;

        default: // Other
          BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size, unit_type);
          break;
      }
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  // If converting hdr10plus - add the DoVi RPU as the last NALU in the access unit.
  if (convert_hdr10plus_meta)
    AddDoViRpuNalu(hdr10plus_meta, poutbuf, poutbuf_size, pts);

  m_first_frame = false;

  return true;

fail:
  av_free(*poutbuf), *poutbuf = NULL;
  *poutbuf_size = 0;
  return false;
}

void CBitstreamConverter::BitstreamAllocAndCopy(uint8_t** poutbuf,
                                                int* poutbuf_size,
                                                const uint8_t* sps_pps,
                                                uint32_t sps_pps_size,
                                                const uint8_t* in,
                                                uint32_t in_size,
                                                uint8_t nal_type)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;
  void *tmp;

  // According to x265, this type is always encoded with four-sized header
  // https://bitbucket.org/multicoreware/x265_git/src/4bf31dc15fb6d1f93d12ecf21fad5e695f0db5c0/source/encoder/nal.cpp#lines-100
  if (nal_type == HEVC_NAL_UNSPEC62)
    nal_header_size = 4;

  *poutbuf_size += sps_pps_size + in_size + nal_header_size;
  tmp = av_realloc(*poutbuf, *poutbuf_size);
  if (!tmp)
    return;
  *poutbuf = (uint8_t*)tmp;
  if (sps_pps)
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);

  memcpy(*poutbuf + sps_pps_size + nal_header_size + offset, in, in_size);
  if (!offset)
  {
    BS_WB32(*poutbuf + sps_pps_size, 1);
  }
  else if (nal_header_size == 4)
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 0;
    (*poutbuf + offset + sps_pps_size)[3] = 1;
  }
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}

void CBitstreamConverter::BitstreamAllocAndCopy(uint8_t** poutbuf,
                                                uint32_t* poutbuf_size,
                                                const uint8_t* in,
                                                uint32_t in_size,
                                                uint8_t nal_type)
{
  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;
  void *tmp;

  if (nal_type == HEVC_NAL_UNSPEC62)
    nal_header_size = 4;
  else if (nal_type == HEVC_NAL_UNSPEC63)
    nal_header_size = 5;

  *poutbuf_size += in_size + nal_header_size;
  tmp = av_realloc(*poutbuf, *poutbuf_size);
  if (!tmp)
    return;
  *poutbuf = (uint8_t*)tmp;

  memcpy(*poutbuf + nal_header_size + offset, in, in_size);

  if (nal_header_size == 5)
  {
    (*poutbuf + offset)[0] = 0;
    (*poutbuf + offset)[1] = 0;
    (*poutbuf + offset)[2] = 1;
    (*poutbuf + offset)[3] = HEVC_NAL_UNSPEC63 << 1;
    (*poutbuf + offset)[4] = 1;
  }
  else if (nal_header_size == 4)
  {
    (*poutbuf + offset)[0] = 0;
    (*poutbuf + offset)[1] = 0;
    (*poutbuf + offset)[2] = 0;
    (*poutbuf + offset)[3] = 1;
  }
  else
  {
    (*poutbuf + offset)[0] = 0;
    (*poutbuf + offset)[1] = 0;
    (*poutbuf + offset)[2] = 1;
  }
}

int CBitstreamConverter::avc_parse_nal_units(AVIOContext *pb, const uint8_t *buf_in, int size)
{
  const uint8_t *p = buf_in;
  const uint8_t *end = p + size;
  const uint8_t *nal_start, *nal_end;

  size = 0;
  nal_start = avc_find_startcode(p, end);

  for (;;) {
    while (nal_start < end && !*(nal_start++));
    if (nal_start == end)
      break;

    nal_end = avc_find_startcode(nal_start, end);
    avio_wb32(pb, nal_end - nal_start);
    avio_write(pb, nal_start, nal_end - nal_start);
    size += 4 + nal_end - nal_start;
    nal_start = nal_end;
  }
  return size;
}

int CBitstreamConverter::avc_parse_nal_units_buf(const uint8_t *buf_in, uint8_t **buf, int *size)
{
  AVIOContext *pb;
  int ret = avio_open_dyn_buf(&pb);
  if (ret < 0)
    return ret;

  avc_parse_nal_units(pb, buf_in, *size);

  av_freep(buf);
  *size = avio_close_dyn_buf(pb, buf);
  return 0;
}

int CBitstreamConverter::isom_write_avcc(AVIOContext *pb, const uint8_t *data, int len)
{
  // extradata from bytestream h264, convert to avcC atom data for bitstream
  if (len > 6)
  {
    /* check for h264 start code */
    if (BS_RB32(data) == 0x00000001 || BS_RB24(data) == 0x000001)
    {
      uint8_t *buf=NULL, *end, *start;
      uint32_t sps_size=0, pps_size=0;
      uint8_t *sps=0, *pps=0;

      int ret = avc_parse_nal_units_buf(data, &buf, &len);
      if (ret < 0)
        return ret;
      start = buf;
      end = buf + len;

      /* look for sps and pps */
      while (end - buf > 4)
      {
        uint32_t size;
        uint8_t  nal_type;
        size = std::min<uint32_t>(BS_RB32(buf), end - buf - 4);
        buf += 4;
        nal_type = buf[0] & 0x1f;
        if (nal_type == 7) /* SPS */
        {
          sps = buf;
          sps_size = size;
        }
        else if (nal_type == 8) /* PPS */
        {
          pps = buf;
          pps_size = size;
        }
        buf += size;
      }
      if (!sps || !pps || sps_size < 4 || sps_size > UINT16_MAX || pps_size > UINT16_MAX)
        assert(0);

      avio_w8(pb, 1); /* version */
      avio_w8(pb, sps[1]); /* profile */
      avio_w8(pb, sps[2]); /* profile compat */
      avio_w8(pb, sps[3]); /* level */
      avio_w8(pb, 0xff); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
      avio_w8(pb, 0xe1); /* 3 bits reserved (111) + 5 bits number of sps (00001) */

      avio_wb16(pb, sps_size);
      avio_write(pb, sps, sps_size);
      if (pps)
      {
        avio_w8(pb, 1); /* number of pps */
        avio_wb16(pb, pps_size);
        avio_write(pb, pps, pps_size);
      }
      av_free(start);
    }
    else
    {
      avio_write(pb, data, len);
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
bool CBitstreamConverter::mpeg2_sequence_header(const uint8_t *data, const uint32_t size, mpeg2_sequence *sequence)
{
  // parse nal's until a sequence_header_code is found
  // and return the width, height, aspect ratio and frame rate if changed.
  bool changed = false;

  if (!data)
    return changed;

  const uint8_t *p = data;
  const uint8_t *end = p + size;
  const uint8_t *nal_start, *nal_end;

  nal_start = avc_find_startcode(p, end);
  while (nal_start < end)
  {
    while (!*(nal_start++));
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
      switch(ratio_info)
      {
        case 0x01:
          ratio = 1.0f;
          break;
        default:
        case 0x02:
          ratio = 4.0f/3;
          break;
        case 0x03:
          ratio = 16.0f/9;
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

      switch(rate_info)
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

bool CBitstreamConverter::h264_sequence_header(const uint8_t *data, const uint32_t size, h264_sequence *sequence)
{
    // parse nal units until SPS is found
    // and return the width, height and aspect ratio if changed.
    bool changed = false;

    if (!data)
        return changed;

    const uint8_t *p = data;
    const uint8_t *end = p + size;
    const uint8_t *nal_start, *nal_end;

    int profile_idc;
    int chroma_format_idc = 1;
    uint8_t pic_order_cnt_type;
    uint8_t aspect_ratio_idc = 0;
    uint8_t separate_colour_plane_flag = 0;
    int8_t frame_mbs_only_flag = -1;
    unsigned int pic_width, pic_width_cropped;
    unsigned int pic_height, pic_height_cropped;
    unsigned int frame_crop_right_offset = 0;
    unsigned int frame_crop_bottom_offset = 0;
    unsigned int sar_width = 0;
    unsigned int sar_height = 0;

    int lastScale;
    int nextScale;
    int deltaScale;

    nal_start = avc_find_startcode(p, end);

    while (nal_start < end)
    {
        while (!*(nal_start++));

        nal_end = avc_find_startcode(nal_start, end);

        if ((*nal_start & 0x1f) == 7) // SPS
        {
            nal_bitstream bs;
            nal_bs_init(&bs, nal_start, end - nal_start);

            nal_bs_read(&bs, 8); // NAL unit type

            profile_idc = nal_bs_read(&bs, 8);  // profile_idc

            nal_bs_read(&bs, 1);  // constraint_set0_flag
            nal_bs_read(&bs, 1);  // constraint_set1_flag
            nal_bs_read(&bs, 1);  // constraint_set2_flag
            nal_bs_read(&bs, 1);  // constraint_set3_flag
            nal_bs_read(&bs, 4);  // reserved
            nal_bs_read(&bs, 8);  // level_idc
            nal_bs_read_ue(&bs);  // sps_id

            if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 ||
                profile_idc == 244 || profile_idc == 44  || profile_idc == 83  ||
                profile_idc == 86  || profile_idc == 118 || profile_idc == 128 )
            {

                chroma_format_idc = nal_bs_read_ue(&bs); // chroma_format_idc
                // high_profile
                if (chroma_format_idc == 3)
                {
                    separate_colour_plane_flag = nal_bs_read(&bs, 1); // separate_colour_plane_flag
                }

                nal_bs_read_ue(&bs); // bit_depth_luma_minus8
                nal_bs_read_ue(&bs); // bit_depth_chroma_minus8
                nal_bs_read(&bs, 1); // qpprime_y_zero_transform_bypass_flag

                if (nal_bs_read (&bs, 1)) // seq_scaling_matrix_present_flag
                {

                    for (int idx = 0; idx < ((chroma_format_idc != 3) ? 8 : 12); ++idx)
                    {
                        if (nal_bs_read(&bs, 1)) // scaling list present
                        {
                            lastScale = nextScale = 8;
                            int sl_n = ((idx < 6) ? 16 : 64);

                            for(int sl_i = 0; sl_i < sl_n; ++sl_i)
                            {
                                if (nextScale != 0)
                                {
                                    deltaScale = nal_bs_read_se(&bs);
                                    nextScale = (lastScale + deltaScale + 256) % 256;

                                }
                                lastScale = (nextScale == 0) ? lastScale : nextScale;
                            }
                        }
                    }
                }
            }

            nal_bs_read_ue(&bs); // log2_max_frame_num_minus4

            pic_order_cnt_type = nal_bs_read_ue(&bs); // pic_order_cnt_type

            if (pic_order_cnt_type == 0)
                nal_bs_read_ue(&bs); //  log2_max_pic_order_cnt_lsb_minus4
            else if (pic_order_cnt_type == 1)
            {
                nal_bs_read(&bs, 1); // delta_pic_order_always_zero_flag
                nal_bs_read_se(&bs); // offset_for_non_ref_pic
                nal_bs_read_se(&bs); // offset_for_top_to_bottom_field

                unsigned int tmp, idx;
                tmp =  nal_bs_read_ue(&bs);
                for (idx = 0; idx < tmp; ++idx)
                    nal_bs_read_se(&bs); // offset_for_ref_frame[i]
            }

            nal_bs_read_ue(&bs); // num_ref_frames
            nal_bs_read(&bs, 1); // gaps_in_frame_num_allowed_flag

            pic_width = (nal_bs_read_ue(&bs) + 1) * 16 ; // pic_width
            pic_height = (nal_bs_read_ue(&bs) + 1) * 16; // pic_height

            frame_mbs_only_flag = nal_bs_read(&bs, 1); // frame_mbs_only_flag
            if (!frame_mbs_only_flag)
            {
                pic_height *= 2;
                nal_bs_read(&bs, 1); // mb_adaptive_frame_field_flag
            }

            nal_bs_read(&bs, 1); // direct_8x8_inference_flag

            if (nal_bs_read(&bs, 1)) // frame_cropping_flag
            {
                nal_bs_read_ue(&bs); // frame_crop_left_offset
                frame_crop_right_offset = nal_bs_read_ue(&bs); // frame_crop_right_offset
                nal_bs_read_ue(&bs); // frame_crop_top_offset
                frame_crop_bottom_offset = nal_bs_read_ue(&bs); // frame_crop_bottom_offset
            }

            if (nal_bs_read(&bs, 1)) // vui_parameters_present_flag
            {
                if (nal_bs_read(&bs, 1)) //aspect_ratio_info_present_flag
                {
                    aspect_ratio_idc = nal_bs_read(&bs, 8); // aspect_ratio_idc

                    if (aspect_ratio_idc == 255) // EXTENDED_SAR
                    {
                        sar_width  = nal_bs_read(&bs, 16);
                        sar_height = nal_bs_read(&bs, 16);

                    }
                }

                if (nal_bs_read(&bs, 1)) //overscan_info_present_flag
                    nal_bs_read(&bs, 1); //overscan_appropriate_flag

                if (nal_bs_read(&bs, 1))  //video_signal_type_present_flag
                {
                    nal_bs_read(&bs, 3); //video_format
                    nal_bs_read(&bs, 1); //video_full_range_flag
                    if (nal_bs_read(&bs, 1)) // colour_description_present_flag
                    {
                        nal_bs_read(&bs, 8); // colour_primaries
                        nal_bs_read(&bs, 8); // transfer_characteristics
                        nal_bs_read(&bs, 8); // matrix_coefficients
                    }
                }

                if (nal_bs_read(&bs, 1)) //chroma_loc_info_present_flag
                {
                    nal_bs_read_ue(&bs); //chroma_sample_loc_type_top_field ue(v)
                    nal_bs_read_ue(&bs); //chroma_sample_loc_type_bottom_field ue(v)
                }

                if (nal_bs_read(&bs, 1)) //timing_info_present_flag
                {
                    nal_bs_read(&bs, 32); //num_units_in_tick
                    nal_bs_read(&bs, 32); //time_scale
                    nal_bs_read(&bs, 1); // fixed rate
                }
            }

            unsigned int ChromaArrayType, crop;
            ChromaArrayType = separate_colour_plane_flag ? 0 : chroma_format_idc;

            // cropped width
            unsigned int CropUnitX, SubWidthC;
            CropUnitX = 1;
            SubWidthC = chroma_format_idc == 3 ? 1 : 2;
            if (ChromaArrayType != 0)
                CropUnitX = SubWidthC;
            crop = CropUnitX * frame_crop_right_offset;
            pic_width_cropped = pic_width - crop;

            if (pic_width_cropped != sequence->width)
            {
                changed = true;
                sequence->width = pic_width_cropped;
            }

            // cropped height
            unsigned int CropUnitY, SubHeightC;
            CropUnitY = 2 - frame_mbs_only_flag;
            SubHeightC = chroma_format_idc <= 1 ? 2 : 1;
            if (ChromaArrayType != 0)
                CropUnitY *= SubHeightC;
            crop = CropUnitY * frame_crop_bottom_offset;
            pic_height_cropped = pic_height - crop;

            if (pic_height_cropped != sequence->height)
            {
                changed = true;
                sequence->height = pic_height_cropped;
            }

            // aspect ratio
            float ratio = sequence->ratio;
            if (pic_height_cropped)
                ratio = pic_width_cropped / (double) pic_height_cropped;
            switch (aspect_ratio_idc)
            {
                case 0:
                    // Unspecified
                    break;
                case 1:
                    // 1:1
                    break;
                case 2:
                    // 12:11
                    ratio *= 1.0909090909090908f;
                    break;
                case 3:
                    // 10:11
                    ratio *= 0.90909090909090906f;
                    break;
                case 4:
                    // 16:11
                    ratio *= 1.4222222222222222f;
                    break;
                case 5:
                    // 40:33
                    ratio *= 1.2121212121212122f;
                    break;
                case 6:
                    // 24:11
                    ratio *= 2.1818181818181817f;
                    break;
                case 7:
                    // 20:11
                    ratio *= 1.8181818181818181f;
                    break;
                case 8:
                    // 32:11
                    ratio *= 2.9090909090909092f;
                    break;
                case 9:
                    // 80:33
                    ratio *= 2.4242424242424243f;
                    break;
                case 10:
                    // 18:11
                    ratio *= 1.6363636363636365f;
                    break;
                case 11:
                    // 15:11
                    ratio *= 1.3636363636363635f;
                    break;
                case 12:
                    // 64:33
                    ratio *= 1.9393939393939394f;
                    break;
                case 13:
                    // 160:99
                    ratio *= 1.6161616161616161f;
                    break;
                case 14:
                    // 4:3
                    ratio *= 1.3333333333333333f;
                    break;
                case 15:
                    // 3:2
                    ratio *= 1.5f;
                    break;
                case 16:
                    // 2:1
                    ratio *= 2.0f;
                    break;
                case 255:
                    // EXTENDED_SAR
                    if (sar_height)
                        ratio *= sar_width / (float)sar_height;
                    else
                        ratio = 0.0f;
                    break;
            } // switch
            if (aspect_ratio_idc != sequence->ratio_info)
            {
                changed = true;
                sequence->ratio = ratio;
                sequence->ratio_info = aspect_ratio_idc;
            }
            if (changed)
            {
              CLog::Log(LOGDEBUG, "CBitstreamConverter::h264_sequence_header: "
                "width({:d}), height({:d}), ratio({:f}), {:d}x{:d}", pic_width_cropped, pic_height_cropped, ratio, sar_width, sar_height);
            }

            break;
        } // SPS
        nal_start = nal_end;
    }

    return changed;
}
