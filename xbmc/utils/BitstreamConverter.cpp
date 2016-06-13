/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/log.h"
#include "assert.h"

#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif

#include "BitstreamConverter.h"

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

enum {
    HEVC_NAL_TRAIL_N    = 0,
    HEVC_NAL_TRAIL_R    = 1,
    HEVC_NAL_TSA_N      = 2,
    HEVC_NAL_TSA_R      = 3,
    HEVC_NAL_STSA_N     = 4,
    HEVC_NAL_STSA_R     = 5,
    HEVC_NAL_RADL_N     = 6,
    HEVC_NAL_RADL_R     = 7,
    HEVC_NAL_RASL_N     = 8,
    HEVC_NAL_RASL_R     = 9,
    HEVC_NAL_BLA_W_LP   = 16,
    HEVC_NAL_BLA_W_RADL = 17,
    HEVC_NAL_BLA_N_LP   = 18,
    HEVC_NAL_IDR_W_RADL = 19,
    HEVC_NAL_IDR_N_LP   = 20,
    HEVC_NAL_CRA_NUT    = 21,
    HEVC_NAL_VPS        = 32,
    HEVC_NAL_SPS        = 33,
    HEVC_NAL_PPS        = 34,
    HEVC_NAL_AUD        = 35,
    HEVC_NAL_EOS_NUT    = 36,
    HEVC_NAL_EOB_NUT    = 37,
    HEVC_NAL_FD_NUT     = 38,
    HEVC_NAL_SEI_PREFIX = 39,
    HEVC_NAL_SEI_SUFFIX = 40
};

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// GStreamer h264 parser
// Copyright (C) 2005 Michal Benes <michal.benes@itonis.tv>
//           (C) 2008 Wim Taymans <wim.taymans@gmail.com>
// gsth264parse.c:
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
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
    res = bs->cache >> shift;
  else
    res = bs->cache;

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

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CBitstreamParser::CBitstreamParser()
{
}

CBitstreamParser::~CBitstreamParser()
{
  Close();
}

bool CBitstreamParser::Open()
{
  return true;
}

void CBitstreamParser::Close()
{
}

const uint8_t* CBitstreamParser::find_start_code(const uint8_t *p,
  const uint8_t *end, uint32_t *state)
{
  assert(p <= end);
  if (p >= end)
    return end;

  for (int i = 0; i < 3; i++) {
    uint32_t tmp = *state << 8;
    *state = tmp + *(p++);
    if (tmp == 0x100 || p == end)
      return p;
  }

  while (p < end) {
    if      (p[-1] > 1      ) p += 3;
    else if (p[-2]          ) p += 2;
    else if (p[-3]|(p[-1]-1)) p++;
    else {
      p++;
      break;
    }
  }

  p = FFMIN(p, end) - 4;
  *state = BS_RB32(p);

  return p + 4;
}

bool CBitstreamParser::FindIdrSlice(const uint8_t *buf, int buf_size)
{
  if (!buf)
    return false;

  bool rtn = false;
  uint32_t state = -1;
  const uint8_t *buf_end = buf + buf_size;

  for(;;)
  {
    buf = find_start_code(buf, buf_end, &state);
    if (buf >= buf_end)
    {
      //CLog::Log(LOGDEBUG, "FindIdrSlice: buf(%p), buf_end(%p)", buf, buf_end);
      break;
    }

    --buf;
    int src_length = buf_end - buf;
    switch (state & 0x1f)
    {
      default:
        CLog::Log(LOGDEBUG, "FindIdrSlice: found nal_type(%d)", state & 0x1f);
        break;
      case AVC_NAL_SLICE:
        CLog::Log(LOGDEBUG, "FindIdrSlice: found NAL_SLICE");
        break;
      case AVC_NAL_IDR_SLICE:
        CLog::Log(LOGDEBUG, "FindIdrSlice: found NAL_IDR_SLICE");
        rtn = true;
        break;
      case AVC_NAL_SEI:
        CLog::Log(LOGDEBUG, "FindIdrSlice: found NAL_SEI");
        break;
      case AVC_NAL_SPS:
        CLog::Log(LOGDEBUG, "FindIdrSlice: found NAL_SPS");
        break;
      case AVC_NAL_PPS:
        CLog::Log(LOGDEBUG, "FindIdrSlice: found NAL_PPS");
        break;
    }
    buf += src_length;
  }

  return rtn;
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CBitstreamConverter::CBitstreamConverter()
{
  m_convert_bitstream = false;
  m_convertBuffer     = NULL;
  m_convertSize       = 0;
  m_inputBuffer       = NULL;
  m_inputSize         = 0;
  m_to_annexb         = false;
  m_extradata         = NULL;
  m_extrasize         = 0;
  m_convert_3byteTo4byteNALSize = false;
  m_convert_bytestream = false;
  m_sps_pps_context.sps_pps_data = NULL;
}

CBitstreamConverter::~CBitstreamConverter()
{
  Close();
}

bool CBitstreamConverter::Open(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize, bool to_annexb)
{
  m_to_annexb = to_annexb;

  m_codec = codec;
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
          m_extrasize = in_extrasize;
          m_extradata = (uint8_t*)av_malloc(in_extrasize);
          memcpy(m_extradata, in_extradata, in_extrasize);
          m_convert_bitstream = BitstreamConvertInitAVC(m_extradata, m_extrasize);
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
            // NAL reformating to bitstream format needed
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
            m_extradata = (uint8_t *)av_malloc(in_extrasize);
            memcpy(m_extradata, in_extradata, in_extrasize);
            m_extrasize = in_extrasize;
            // done with the converted extradata, we MUST free using av_free
            av_free(in_extradata);
            return true;
          }
          else
          {
            CLog::Log(LOGNOTICE, "CBitstreamConverter::Open invalid avcC atom data");
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
           
            m_extradata = (uint8_t *)av_malloc(in_extrasize);
            memcpy(m_extradata, in_extradata, in_extrasize);
            m_extrasize = in_extrasize;
            return true;
          }
        }
        // valid avcC atom 
        m_extradata = (uint8_t*)av_malloc(in_extrasize);
        memcpy(m_extradata, in_extradata, in_extrasize);
        m_extrasize = in_extrasize;
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
          m_extrasize = in_extrasize;
          m_extradata = (uint8_t*)av_malloc(in_extrasize);
          memcpy(m_extradata, in_extradata, in_extrasize);
          m_convert_bitstream = BitstreamConvertInitHEVC(m_extradata, m_extrasize);
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
            CLog::Log(LOGNOTICE, "CBitstreamConverter::Open invalid hvcC atom data");
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
        m_extradata = (uint8_t*)av_malloc(in_extrasize);
        memcpy(m_extradata, in_extradata, in_extrasize);
        m_extrasize = in_extrasize;
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

  if (m_extradata)
    av_free(m_extradata), m_extradata = NULL;
  m_extrasize = 0;

  m_inputSize = 0;
  m_inputBuffer = NULL;

  m_convert_bitstream = false;
  m_convert_bytestream = false;
  m_convert_3byteTo4byteNALSize = false;
}

bool CBitstreamConverter::Convert(uint8_t *pData, int iSize)
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

          BitstreamConvert(demuxer_content, demuxer_bytes, &bytestream_buff, &bytestream_size);
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


uint8_t *CBitstreamConverter::GetConvertBuffer() const
{
  if((m_convert_bitstream || m_convert_bytestream || m_convert_3byteTo4byteNALSize) && m_convertBuffer != NULL)
    return m_convertBuffer;
  else
    return m_inputBuffer;
}

int CBitstreamConverter::GetConvertSize() const
{
  if((m_convert_bitstream || m_convert_bytestream || m_convert_3byteTo4byteNALSize) && m_convertBuffer != NULL)
    return m_convertSize;
  else
    return m_inputSize;
}

uint8_t *CBitstreamConverter::GetExtraData() const
{
  if(m_convert_bitstream)
    return m_sps_pps_context.sps_pps_data;
  else
    return m_extradata;
}
int CBitstreamConverter::GetExtraSize() const
{
  if(m_convert_bitstream)
    return m_sps_pps_context.size;
  else
    return m_extrasize;
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

    if (total_size > INT_MAX - FF_INPUT_BUFFER_PADDING_SIZE ||
      (extradata + 2 + unit_size) > ((uint8_t*)in_extradata + in_extrasize))
    {
      av_free(out);
      return false;
    }
    tmp = av_realloc(out, total_size + FF_INPUT_BUFFER_PADDING_SIZE);
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
  }

  if (out)
    memset(out + total_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

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

      if (total_size > INT_MAX - FF_INPUT_BUFFER_PADDING_SIZE ||
        (extradata + unit_size) > ((uint8_t*)in_extradata + in_extrasize))
      {
        av_free(out);
        return false;
      }
      tmp = av_realloc(out, total_size + FF_INPUT_BUFFER_PADDING_SIZE);
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
    memset(out + total_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

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
             unit_type == HEVC_NAL_IDR_N_LP;
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

bool CBitstreamConverter::BitstreamConvert(uint8_t* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  int i;
  uint8_t *buf = pData;
  uint32_t buf_size = iSize;
  uint8_t  unit_type, nal_sps, nal_pps;
  int32_t  nal_size;
  uint32_t cumul_size = 0;
  const uint8_t *buf_end = buf + buf_size;

  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      nal_sps = AVC_NAL_SPS;
      nal_pps = AVC_NAL_PPS;
      break;
    case AV_CODEC_ID_HEVC:
      nal_sps = HEVC_NAL_SPS;
      nal_pps = HEVC_NAL_PPS;
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

      // prepend only to the first access unit of an IDR picture, if no sps/pps already present
    if (m_sps_pps_context.first_idr && IsIDR(unit_type) && !m_sps_pps_context.idr_sps_pps_seen)
    {
      BitstreamAllocAndCopy(poutbuf, poutbuf_size,
        m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size);
      if (!m_sps_pps_context.first_idr && IsSlice(unit_type))
      {
          m_sps_pps_context.first_idr = 1;
          m_sps_pps_context.idr_sps_pps_seen = 0;
      }
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;

fail:
  av_free(*poutbuf), *poutbuf = NULL;
  *poutbuf_size = 0;
  return false;
}

void CBitstreamConverter::BitstreamAllocAndCopy( uint8_t **poutbuf, int *poutbuf_size,
    const uint8_t *sps_pps, uint32_t sps_pps_size, const uint8_t *in, uint32_t in_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;
  void *tmp;

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
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}

const int CBitstreamConverter::avc_parse_nal_units(AVIOContext *pb, const uint8_t *buf_in, int size)
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

const int CBitstreamConverter::avc_parse_nal_units_buf(const uint8_t *buf_in, uint8_t **buf, int *size)
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

const int CBitstreamConverter::isom_write_avcc(AVIOContext *pb, const uint8_t *data, int len)
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
        size = FFMIN(BS_RB32(buf), end - buf - 4);
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

void CBitstreamConverter::bits_reader_set( bits_reader_t *br, uint8_t *buf, int len )
{
  br->buffer = br->start = buf;
  br->offbits = 0;
  br->length = len;
  br->oflow = 0;
}

uint32_t CBitstreamConverter::read_bits( bits_reader_t *br, int nbits )
{
  int i, nbytes;
  uint32_t ret = 0;
  uint8_t *buf;

  buf = br->buffer;
  nbytes = (br->offbits + nbits)/8;
  if ( ((br->offbits + nbits) %8 ) > 0 )
    nbytes++;
  if ( (buf + nbytes) > (br->start + br->length) ) {
    br->oflow = 1;
    return 0;
  }
  for ( i=0; i<nbytes; i++ )
    ret += buf[i]<<((nbytes-i-1)*8);
  i = (4-nbytes)*8+br->offbits;
  ret = ((ret<<i)>>i)>>((nbytes*8)-nbits-br->offbits);

  br->offbits += nbits;
  br->buffer += br->offbits / 8;
  br->offbits %= 8;

  return ret;
}

void CBitstreamConverter::skip_bits( bits_reader_t *br, int nbits )
{
  br->offbits += nbits;
  br->buffer += br->offbits / 8;
  br->offbits %= 8;
  if ( br->buffer > (br->start + br->length) ) {
    br->oflow = 1;
  }
}

uint32_t CBitstreamConverter::get_bits( bits_reader_t *br, int nbits )
{
  int i, nbytes;
  uint32_t ret = 0;
  uint8_t *buf;

  buf = br->buffer;
  nbytes = (br->offbits + nbits)/8;
  if ( ((br->offbits + nbits) %8 ) > 0 )
    nbytes++;
  if ( (buf + nbytes) > (br->start + br->length) ) {
    br->oflow = 1;
    return 0;
  }
  for ( i=0; i<nbytes; i++ )
    ret += buf[i]<<((nbytes-i-1)*8);
  i = (4-nbytes)*8+br->offbits;
  ret = ((ret<<i)>>i)>>((nbytes*8)-nbits-br->offbits);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void CBitstreamConverter::init_bits_writer(bits_writer_t *s, uint8_t *buffer, int buffer_size, int writer_le)
{
  if (buffer_size < 0)
  {
    buffer_size = 0;
    buffer      = NULL;
  }

  s->size_in_bits = 8 * buffer_size;
  s->buf          = buffer;
  s->buf_end      = s->buf + buffer_size;
  s->buf_ptr      = s->buf;
  s->bit_left     = 32;
  s->bit_buf      = 0;
  s->writer_le    = writer_le;
}

void CBitstreamConverter::write_bits(bits_writer_t *s, int n, unsigned int value)
{
  // Write up to 32 bits into a bitstream.
  unsigned int bit_buf;
  int bit_left;

  if (n == 32)
  {
    // Write exactly 32 bits into a bitstream.
    // danger, recursion in play.
    int lo = value & 0xffff;
    int hi = value >> 16;
    if (s->writer_le)
    {
      write_bits(s, 16, lo);
      write_bits(s, 16, hi);
    }
    else
    {
      write_bits(s, 16, hi);
      write_bits(s, 16, lo);
    }
    return;
  }

  bit_buf  = s->bit_buf;
  bit_left = s->bit_left;

  if (s->writer_le)
  {
    bit_buf |= value << (32 - bit_left);
    if (n >= bit_left) {
      BS_WL32(s->buf_ptr, bit_buf);
      s->buf_ptr += 4;
      bit_buf     = (bit_left == 32) ? 0 : value >> bit_left;
      bit_left   += 32;
    }
    bit_left -= n;
  }
  else
  {
    if (n < bit_left) {
      bit_buf     = (bit_buf << n) | value;
      bit_left   -= n;
    } else {
      bit_buf   <<= bit_left;
      bit_buf    |= value >> (n - bit_left);
      BS_WB32(s->buf_ptr, bit_buf);
      s->buf_ptr += 4;
      bit_left   += 32 - n;
      bit_buf     = value;
    }
  }

  s->bit_buf  = bit_buf;
  s->bit_left = bit_left;
}

void CBitstreamConverter::skip_bits(bits_writer_t *s, int n)
{
  // Skip the given number of bits.
  // Must only be used if the actual values in the bitstream do not matter.
  // If n is 0 the behavior is undefined.
  s->bit_left -= n;
  s->buf_ptr  -= 4 * (s->bit_left >> 5);
  s->bit_left &= 31;
}

void CBitstreamConverter::flush_bits(bits_writer_t *s)
{
  if (!s->writer_le)
  {
    if (s->bit_left < 32)
      s->bit_buf <<= s->bit_left;
  }
  while (s->bit_left < 32)
  {

    if (s->writer_le)
    {
      *s->buf_ptr++ = s->bit_buf;
      s->bit_buf  >>= 8;
    }
    else
    {
      *s->buf_ptr++ = s->bit_buf >> 24;
      s->bit_buf  <<= 8;
    }
    s->bit_left  += 8;
  }
  s->bit_left = 32;
  s->bit_buf  = 0;
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
          ratio = 1.0;
          break;
        default:
        case 0x02:
          ratio = 4.0/3.0;
          break;
        case 0x03:
          ratio = 16.0/9.0;
          break;
        case 0x04:
          ratio = 2.21;
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
      float rate = sequence->rate;
      uint32_t rate_info = nal_bs_read(&bs, 4);
      switch(rate_info)
      {
        default:
        case 0x01:
          rate = 24000.0 / 1001.0;
          break;
        case 0x02:
          rate = 24000.0 / 1000.0;
          break;
        case 0x03:
          rate = 25000.0 / 1000.0;
          break;
        case 0x04:
          rate = 30000.0 / 1001.0;
          break;
        case 0x05:
          rate = 30000.0 / 1000.0;
          break;
        case 0x06:
          rate = 50000.0 / 1000.0;
          break;
        case 0x07:
          rate = 60000.0 / 1001.0;
          break;
        case 0x08:
          rate = 60000.0 / 1000.0;
          break;
      }
      if (rate_info != sequence->rate_info)
      {
        changed = true;
        sequence->rate = rate;
        sequence->rate_info = rate_info;
      }
      /*
      if (changed)
      {
        CLog::Log(LOGDEBUG, "CBitstreamConverter::mpeg2_sequence_header: "
          "width(%d), height(%d), ratio(%f), rate(%f)", width, height, ratio, rate);
      }
      */
    }
    nal_start = nal_end;
  }

  return changed;
}

void CBitstreamConverter::parseh264_sps(const uint8_t *sps, const uint32_t sps_size, bool *interlaced, int32_t *max_ref_frames)
{
  nal_bitstream bs;
  sps_info_struct sps_info;

  nal_bs_init(&bs, sps, sps_size);

  sps_info.profile_idc  = nal_bs_read(&bs, 8);
  nal_bs_read(&bs, 1);  // constraint_set0_flag
  nal_bs_read(&bs, 1);  // constraint_set1_flag
  nal_bs_read(&bs, 1);  // constraint_set2_flag
  nal_bs_read(&bs, 1);  // constraint_set3_flag
  nal_bs_read(&bs, 4);  // reserved
  sps_info.level_idc    = nal_bs_read(&bs, 8);
  sps_info.sps_id       = nal_bs_read_ue(&bs);

  if (sps_info.profile_idc == 100 ||
      sps_info.profile_idc == 110 ||
      sps_info.profile_idc == 122 ||
      sps_info.profile_idc == 244 ||
      sps_info.profile_idc == 44  ||
      sps_info.profile_idc == 83  ||
      sps_info.profile_idc == 86)
  {
    sps_info.chroma_format_idc                    = nal_bs_read_ue(&bs);
    if (sps_info.chroma_format_idc == 3)
      sps_info.separate_colour_plane_flag         = nal_bs_read(&bs, 1);
    sps_info.bit_depth_luma_minus8                = nal_bs_read_ue(&bs);
    sps_info.bit_depth_chroma_minus8              = nal_bs_read_ue(&bs);
    sps_info.qpprime_y_zero_transform_bypass_flag = nal_bs_read(&bs, 1);

    sps_info.seq_scaling_matrix_present_flag = nal_bs_read (&bs, 1);
    if (sps_info.seq_scaling_matrix_present_flag)
    {
      //! @todo unfinished
    }
  }
  sps_info.log2_max_frame_num_minus4 = nal_bs_read_ue(&bs);
  if (sps_info.log2_max_frame_num_minus4 > 12)
  { // must be between 0 and 12
    return;
  }
  sps_info.pic_order_cnt_type = nal_bs_read_ue(&bs);
  if (sps_info.pic_order_cnt_type == 0)
  {
    sps_info.log2_max_pic_order_cnt_lsb_minus4 = nal_bs_read_ue(&bs);
  }
  else if (sps_info.pic_order_cnt_type == 1)
  { //! @todo unfinished
    /*
    delta_pic_order_always_zero_flag = gst_nal_bs_read (bs, 1);
    offset_for_non_ref_pic = gst_nal_bs_read_se (bs);
    offset_for_top_to_bottom_field = gst_nal_bs_read_se (bs);

    num_ref_frames_in_pic_order_cnt_cycle = gst_nal_bs_read_ue (bs);
    for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
    offset_for_ref_frame[i] = gst_nal_bs_read_se (bs);
    */
  }

  sps_info.max_num_ref_frames             = nal_bs_read_ue(&bs);
  sps_info.gaps_in_frame_num_value_allowed_flag = nal_bs_read(&bs, 1);
  sps_info.pic_width_in_mbs_minus1        = nal_bs_read_ue(&bs);
  sps_info.pic_height_in_map_units_minus1 = nal_bs_read_ue(&bs);

  sps_info.frame_mbs_only_flag            = nal_bs_read(&bs, 1);
  if (!sps_info.frame_mbs_only_flag)
    sps_info.mb_adaptive_frame_field_flag = nal_bs_read(&bs, 1);

  sps_info.direct_8x8_inference_flag      = nal_bs_read(&bs, 1);

  sps_info.frame_cropping_flag            = nal_bs_read(&bs, 1);
  if (sps_info.frame_cropping_flag)
  {
    sps_info.frame_crop_left_offset       = nal_bs_read_ue(&bs);
    sps_info.frame_crop_right_offset      = nal_bs_read_ue(&bs);
    sps_info.frame_crop_top_offset        = nal_bs_read_ue(&bs);
    sps_info.frame_crop_bottom_offset     = nal_bs_read_ue(&bs);
  }

  *interlaced = !sps_info.frame_mbs_only_flag;
  *max_ref_frames = sps_info.max_num_ref_frames;
}
