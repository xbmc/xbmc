/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif

#include "BitstreamConverter.h"

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
// GStreamer h264 parser
// Copyright (C) 2005 Michal Benes <michal.benes@itonis.tv>
//           (C) 2008 Wim Taymans <wim.taymans@gmail.com>
// gsth264parse.c:
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
void CBitstreamConverter::nal_bs_init(nal_bitstream *bs, const uint8_t *data, size_t size)
{
  bs->data = data;
  bs->end  = data + size;
  bs->head = 0;
  // fill with something other than 0 to detect
  //  emulation prevention bytes
  bs->cache = 0xffffffff;
}

uint32_t CBitstreamConverter::nal_bs_read(nal_bitstream *bs, int n)
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

bool CBitstreamConverter::nal_bs_eos(nal_bitstream *bs)
{
  return (bs->data >= bs->end) && (bs->head == 0);
}

// read unsigned Exp-Golomb code
int CBitstreamConverter::nal_bs_read_ue(nal_bitstream *bs)
{
  int i = 0;

  while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 32)
    i++;

  return ((1 << i) - 1 + nal_bs_read(bs, i));
}

void CBitstreamConverter::parseh264_sps(uint8_t *sps, uint32_t sps_size, bool *interlaced, int32_t *max_ref_frames)
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
      /* TODO: unfinished */
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
  { // TODO: unfinished
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

const uint8_t *CBitstreamConverter::avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
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

const uint8_t *CBitstreamConverter::avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *out= avc_find_startcode_internal(p, end);
  if (p<out && out<end && !out[-1])
    out--;
  return out;
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
    m_dllAvFormat->avio_wb32(pb, nal_end - nal_start);
    m_dllAvFormat->avio_write(pb, nal_start, nal_end - nal_start);
    size += 4 + nal_end - nal_start;
    nal_start = nal_end;
  }
  return size;
}

const int CBitstreamConverter::avc_parse_nal_units_buf(const uint8_t *buf_in, uint8_t **buf, int *size)
{
  AVIOContext *pb;
  int ret = m_dllAvFormat->avio_open_dyn_buf(&pb);
  if (ret < 0)
    return ret;

  avc_parse_nal_units(pb, buf_in, *size);

  m_dllAvUtil->av_freep(buf);
  *size = m_dllAvFormat->avio_close_dyn_buf(pb, buf);
  return 0;
}

const int CBitstreamConverter::isom_write_avcc(AVIOContext *pb, const uint8_t *data, int len)
{
  // extradata from bytestream h264, convert to avcC atom data for bitstream
  if (len > 6)
  {
    /* check for h264 start code */
    if (OMX_RB32(data) == 0x00000001 || OMX_RB24(data) == 0x000001)
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
        size = FFMIN(OMX_RB32(buf), end - buf - 4);
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

      m_dllAvFormat->avio_w8(pb, 1); /* version */
      m_dllAvFormat->avio_w8(pb, sps[1]); /* profile */
      m_dllAvFormat->avio_w8(pb, sps[2]); /* profile compat */
      m_dllAvFormat->avio_w8(pb, sps[3]); /* level */
      m_dllAvFormat->avio_w8(pb, 0xff); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
      m_dllAvFormat->avio_w8(pb, 0xe1); /* 3 bits reserved (111) + 5 bits number of sps (00001) */

      m_dllAvFormat->avio_wb16(pb, sps_size);
      m_dllAvFormat->avio_write(pb, sps, sps_size);
      if (pps)
      {
        m_dllAvFormat->avio_w8(pb, 1); /* number of pps */
        m_dllAvFormat->avio_wb16(pb, pps_size);
        m_dllAvFormat->avio_write(pb, pps, pps_size);
      }
      m_dllAvUtil->av_free(start);
    }
    else
    {
      m_dllAvFormat->avio_write(pb, data, len);
    }
  }
  return 0;
}

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
  m_dllAvUtil         = NULL;
  m_dllAvFormat       = NULL;
  m_convert_bytestream = false;
}

CBitstreamConverter::~CBitstreamConverter()
{
  Close();
}

bool CBitstreamConverter::Open(enum CodecID codec, uint8_t *in_extradata, int in_extrasize, bool to_annexb)
{
  m_to_annexb = to_annexb;

  m_codec = codec;

  switch(codec)
  {
    case CODEC_ID_H264:
      if (in_extrasize < 7 || in_extradata == NULL)
      {
        CLog::Log(LOGERROR, "CBitstreamConverter::Open avcC data too small or missing\n");
        return false;
      }
      // valid avcC data (bitstream) always starts with the value 1 (version)
      if(m_to_annexb)
      {
        if ( *(char*)in_extradata == 1 )
        {
          CLog::Log(LOGINFO, "CBitstreamConverter::Open bitstream to annexb init\n");
          m_convert_bitstream = BitstreamConvertInit(in_extradata, in_extrasize);
          return true;
        }
      }
      else
      {
        // valid avcC atom data always starts with the value 1 (version)
        if ( *in_extradata != 1 )
        {
          if (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 0 && in_extradata[3] == 1)
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init\n");
            // video content is from x264 or from bytestream h264 (AnnexB format)
            // NAL reformating to bitstream format needed
            m_dllAvUtil = new DllAvUtil;
            m_dllAvFormat = new DllAvFormat;
            if (!m_dllAvUtil->Load() || !m_dllAvFormat->Load())
              return false;

            AVIOContext *pb;
            if (m_dllAvFormat->avio_open_dyn_buf(&pb) < 0)
              return false;
            m_convert_bytestream = true;
            // create a valid avcC atom data from ffmpeg's extradata
            isom_write_avcc(pb, in_extradata, in_extrasize);
            // unhook from ffmpeg's extradata
            in_extradata = NULL;
            // extract the avcC atom data into extradata then write it into avcCData for VDADecoder
            in_extrasize = m_dllAvFormat->avio_close_dyn_buf(pb, &in_extradata);
            // make a copy of extradata contents
            m_extradata = (uint8_t *)malloc(in_extrasize);
            memcpy(m_extradata, in_extradata, in_extrasize);
            m_extrasize = in_extrasize;
            // done with the converted extradata, we MUST free using av_free
            m_dllAvUtil->av_free(in_extradata);
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
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init 3 byte to 4 byte nal\n");
            // video content is from so silly encoder that think 3 byte NAL sizes
            // are valid, setup to convert 3 byte NAL sizes to 4 byte.
            m_dllAvUtil = new DllAvUtil;
            m_dllAvFormat = new DllAvFormat;
            if (!m_dllAvUtil->Load() || !m_dllAvFormat->Load())
              return false;

            in_extradata[4] = 0xFF;
            m_convert_3byteTo4byteNALSize = true;
           
            m_extradata = (uint8_t *)malloc(in_extrasize);
            memcpy(m_extradata, in_extradata, in_extrasize);
            m_extrasize = in_extrasize;
            return true;
          }
        }
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
  if (m_convert_bitstream)
  {
    if (m_sps_pps_context.sps_pps_data)
    {
      free(m_sps_pps_context.sps_pps_data);
      m_sps_pps_context.sps_pps_data = NULL;
    }
    if(m_convertBuffer)
      free(m_convertBuffer);
    m_convertSize       = 0;
  }

  if (m_convert_bytestream)
  {
    if(m_convertBuffer)
    {
      m_dllAvUtil->av_free(m_convertBuffer);
      m_convertBuffer = NULL;
    }
    m_convertSize = 0;
  }

  if(m_extradata)
    free(m_extradata);
  m_extradata = NULL;
  m_extrasize = 0;

  m_inputBuffer       = NULL;
  m_inputSize         = 0;
  m_convert_3byteTo4byteNALSize = false;

  m_convert_bitstream = false;

  if (m_dllAvUtil)
  {
    delete m_dllAvUtil;
    m_dllAvUtil = NULL;
  }
  if (m_dllAvFormat)
  {
    delete m_dllAvFormat;
    m_dllAvFormat = NULL;
  }
}

bool CBitstreamConverter::Convert(uint8_t *pData, int iSize)
{
  if(m_convertBuffer)
    free(m_convertBuffer);
  m_convertBuffer = NULL;
  m_convertSize   = 0;
  m_inputBuffer   = NULL;
  m_inputSize     = 0;

  if (pData)
  {
    if(m_codec == CODEC_ID_H264)
    {
      if(m_to_annexb)
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
          }
          else
          {
            Close();
            m_inputBuffer = pData;
            m_inputSize   = iSize;
            CLog::Log(LOGERROR, "CBitstreamConverter::Convert error converting. disable converter\n");
          }
        }
        else
        {
          m_inputBuffer = pData;
          m_inputSize   = iSize;
        }

        return true;
      }
      else
      {
        m_inputBuffer = pData;
        m_inputSize   = iSize;
  
        if (m_convert_bytestream)
        {
          if(m_convertBuffer)
          {
            m_dllAvUtil->av_free(m_convertBuffer);
            m_convertBuffer = NULL;
          }
          m_convertSize = 0;

          // convert demuxer packet from bytestream (AnnexB) to bitstream
          AVIOContext *pb;
  
          if(m_dllAvFormat->avio_open_dyn_buf(&pb) < 0)
          {
            return false;
          }
          m_convertSize = avc_parse_nal_units(pb, pData, iSize);
          m_convertSize = m_dllAvFormat->avio_close_dyn_buf(pb, &m_convertBuffer);
        }
        else if (m_convert_3byteTo4byteNALSize)
        {
          if(m_convertBuffer)
          {
            m_dllAvUtil->av_free(m_convertBuffer);
            m_convertBuffer = NULL;
          }
          m_convertSize = 0;

          // convert demuxer packet from 3 byte NAL sizes to 4 byte
          AVIOContext *pb;
          if (m_dllAvFormat->avio_open_dyn_buf(&pb) < 0)
            return false;

          uint32_t nal_size;
          uint8_t *end = pData + iSize;
          uint8_t *nal_start = pData;
          while (nal_start < end)
          {
            nal_size = OMX_RB24(nal_start);
            m_dllAvFormat->avio_wb16(pb, nal_size);
            nal_start += 3;
            m_dllAvFormat->avio_write(pb, nal_start, nal_size);
            nal_start += nal_size;
          }
  
          m_convertSize = m_dllAvFormat->avio_close_dyn_buf(pb, &m_convertBuffer);
        }
        return true;
      }
    }
  }

  return false;
}


uint8_t *CBitstreamConverter::GetConvertBuffer()
{
  if((m_convert_bitstream || m_convert_bytestream || m_convert_3byteTo4byteNALSize) && m_convertBuffer != NULL)
    return m_convertBuffer;
  else
    return m_inputBuffer;
}

int CBitstreamConverter::GetConvertSize()
{
  if((m_convert_bitstream || m_convert_bytestream || m_convert_3byteTo4byteNALSize) && m_convertBuffer != NULL)
    return m_convertSize;
  else
    return m_inputSize; 
}

uint8_t *CBitstreamConverter::GetExtraData()
{
  return m_extradata;
}
int CBitstreamConverter::GetExtraSize()
{
  return m_extrasize;
}

bool CBitstreamConverter::BitstreamConvertInit(void *in_extradata, int in_extrasize)
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
  uint8_t *out = NULL, unit_nb, sps_done = 0;
  const uint8_t *extradata = (uint8_t*)in_extradata + 4;
  static const uint8_t nalu_header[4] = {0, 0, 0, 1};

  // retrieve length coded size
  m_sps_pps_context.length_size = (*extradata++ & 0x3) + 1;
  if (m_sps_pps_context.length_size == 3)
    return false;

  // retrieve sps and pps unit(s)
  unit_nb = *extradata++ & 0x1f;  // number of sps unit(s)
  if (!unit_nb)
  {
    unit_nb = *extradata++;       // number of pps unit(s)
    sps_done++;
  }
  while (unit_nb--)
  {
    unit_size = extradata[0] << 8 | extradata[1];
    total_size += unit_size + 4;
    if ( (extradata + 2 + unit_size) > ((uint8_t*)in_extradata + in_extrasize) )
    {
      free(out);
      return false;
    }
    out = (uint8_t*)realloc(out, total_size);
    if (!out)
      return false;

    memcpy(out + total_size - unit_size - 4, nalu_header, 4);
    memcpy(out + total_size - unit_size, extradata + 2, unit_size);
    extradata += 2 + unit_size;

    if (!unit_nb && !sps_done++)
      unit_nb = *extradata++;     // number of pps unit(s)
  }

  m_sps_pps_context.sps_pps_data = out;
  m_sps_pps_context.size = total_size;
  m_sps_pps_context.first_idr = 1;

  return true;
}

bool CBitstreamConverter::BitstreamConvert(uint8_t* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater


  uint8_t *buf = pData;
  uint32_t buf_size = iSize;
  uint8_t  unit_type;
  int32_t  nal_size;
  uint32_t cumul_size = 0;
  const uint8_t *buf_end = buf + buf_size;

  do
  {
    if (buf + m_sps_pps_context.length_size > buf_end)
      goto fail;

    if (m_sps_pps_context.length_size == 1)
      nal_size = buf[0];
    else if (m_sps_pps_context.length_size == 2)
      nal_size = buf[0] << 8 | buf[1];
    else
      nal_size = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];

    buf += m_sps_pps_context.length_size;
    unit_type = *buf & 0x1f;

    if (buf + nal_size > buf_end || nal_size < 0)
      goto fail;

    // prepend only to the first type 5 NAL unit of an IDR picture
    if (m_sps_pps_context.first_idr && unit_type == 5)
    {
      BitstreamAllocAndCopy(poutbuf, poutbuf_size,
        m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      BitstreamAllocAndCopy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size);
      if (!m_sps_pps_context.first_idr && unit_type == 1)
          m_sps_pps_context.first_idr = 1;
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;

fail:
  free(*poutbuf);
  *poutbuf = NULL;
  *poutbuf_size = 0;
  return false;
}

void CBitstreamConverter::BitstreamAllocAndCopy( uint8_t **poutbuf, int *poutbuf_size,
    const uint8_t *sps_pps, uint32_t sps_pps_size, const uint8_t *in, uint32_t in_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  #define CHD_WB32(p, d) { \
    ((uint8_t*)(p))[3] = (d); \
    ((uint8_t*)(p))[2] = (d) >> 8; \
    ((uint8_t*)(p))[1] = (d) >> 16; \
    ((uint8_t*)(p))[0] = (d) >> 24; }

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;

  *poutbuf_size += sps_pps_size + in_size + nal_header_size;
  *poutbuf = (uint8_t*)realloc(*poutbuf, *poutbuf_size);
  if (sps_pps)
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);

  memcpy(*poutbuf + sps_pps_size + nal_header_size + offset, in, in_size);
  if (!offset)
  {
    CHD_WB32(*poutbuf + sps_pps_size, 1);
  }
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}


