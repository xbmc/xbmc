/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *
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

#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include "bitstream.h"
#include "receiver.h"

#include "demuxer_h264.h"

static const int h264_lev2cpbsize[][2] =
{
  {10, 175},
  {11, 500},
  {12, 1000},
  {13, 2000},
  {20, 2000},
  {21, 4000},
  {22, 4000},
  {30, 10000},
  {31, 14000},
  {32, 20000},
  {40, 25000},
  {41, 62500},
  {42, 62500},
  {50, 135000},
  {51, 240000},
  {-1, -1},
};

cParserH264::cParserH264(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID)
 : cParser(streamer, streamID)
{
  m_pictureBuffer     = NULL;
  m_pictureBufferSize = 0;
  m_pictureBufferPtr  = 0;
  m_StartCond         = 0;
  m_StartCode         = 0;
  m_StartCodeOffset   = 0;
  m_PrevDTS           = DVD_NOPTS_VALUE;
  m_Height            = 0;
  m_Width             = 0;
  m_FrameDuration     = 0;
  m_demuxer           = demuxer;
  m_vbvDelay          = -1;
  m_vbvSize           = 0;
  m_firstIFrame       = false;
  m_PixelAspect.den   = 1;
  m_PixelAspect.num   = 0;
  memset(&m_streamData, 0, sizeof(m_streamData));
}

cParserH264::~cParserH264()
{
  if (m_pictureBuffer)
    free(m_pictureBuffer);
}

void cParserH264::Parse(unsigned char *data, int size, bool pusi)
{
  uint32_t startcode = m_StartCond;

  if (m_pictureBuffer == NULL)
  {
    m_pictureBufferSize   = 80000;
    m_pictureBuffer       = (uint8_t*)malloc(m_pictureBufferSize);
  }

  if (m_pictureBufferPtr + size + 4 >= m_pictureBufferSize)
  {
    m_pictureBufferSize  += size * 4;
    m_pictureBuffer       = (uint8_t*)realloc(m_pictureBuffer, m_pictureBufferSize);
  }

  for (int i = 0; i < size; i++)
  {
    m_pictureBuffer[m_pictureBufferPtr++] = data[i];
    startcode = startcode << 8 | data[i];

    if ((startcode & 0xffffff00) != 0x00000100)
      continue;

    bool reset = true;
    if (m_pictureBufferPtr - 4 > 0 && m_StartCode != 0)
    {
      reset = Parse_H264(m_pictureBufferPtr - 4, startcode, m_StartCodeOffset);
    }

    if (reset)
    {
      /* Reset packet parser upon length error or if parser tells us so */
      m_pictureBufferPtr = 0;
      m_pictureBuffer[m_pictureBufferPtr++] = startcode >> 24;
      m_pictureBuffer[m_pictureBufferPtr++] = startcode >> 16;
      m_pictureBuffer[m_pictureBufferPtr++] = startcode >> 8;
      m_pictureBuffer[m_pictureBufferPtr++] = startcode >> 0;
    }
    m_StartCode = startcode;
    m_StartCodeOffset = m_pictureBufferPtr - 4;
  }
  m_StartCond = startcode;
}

bool cParserH264::Parse_H264(size_t len, uint32_t next_startcode, int sc_offset)
{
  uint8_t nal_data[len];
  int pkttype;
  uint8_t *buf = m_pictureBuffer + sc_offset;
  uint32_t startcode = m_StartCode;

  if (startcode == 0x10c)
  {
    /* RBSP padding, we don't want this */
    int length = len - sc_offset;
    memcpy(buf, buf + length, 4); /* Move down new start code */
    m_pictureBufferPtr -= length;  /* Drop buffer */
  }

  if (startcode >= 0x000001e0 && startcode <= 0x000001ef)
  {
    /* System start codes for video */
    if (len >= 9)
      ParsePESHeader(buf, len);

    if (m_PrevDTS != DVD_NOPTS_VALUE)
    {
      int64_t duration = (m_curDTS - m_PrevDTS) & 0x1ffffffffLL;

      if (duration < 90000)
        m_FrameDuration = duration;
    }
    m_PrevDTS = m_curDTS;
    return true;
  }

  switch(startcode & 0x1f)
  {
  case NAL_SPS:
  {
    int nal_len = nalUnescape(nal_data, buf + 4, len - 4);
    if (!Parse_SPS(nal_data, nal_len))
      return true;

    double PAR = (double)m_PixelAspect.num/(double)m_PixelAspect.den;
    double DAR = (PAR * m_Width) / m_Height;
    DEBUGLOG("H.264 SPS: PAR %i:%i", m_PixelAspect.num, m_PixelAspect.den);
    DEBUGLOG("H.264 SPS: DAR %.2f", DAR);

    m_demuxer->SetVideoInformation(0,0, m_Height, m_Width, DAR);
    break;
  }

  case NAL_PPS:
  {
    int nal_len = nalUnescape(nal_data, buf + 4, len - 4);
    if (!Parse_PPS(nal_data, nal_len))
      return true;

    break;
  }

  case 5: /* IDR+SLICE */
  case NAL_SLH:
  {
    if (m_FoundFrame || m_FrameDuration == 0 || m_curDTS == DVD_NOPTS_VALUE)
      break;

    int nal_len = nalUnescape(nal_data, buf + 4, len - 4 > 64 ? 64 : len - 3);
    if (!Parse_SLH(nal_data, nal_len, &pkttype))
      return true;

    m_StreamPacket.id         = m_streamID;
    m_StreamPacket.pts        = m_curPTS;
    m_StreamPacket.dts        = m_curDTS;
    m_StreamPacket.frametype  = pkttype;
    m_StreamPacket.duration   = m_FrameDuration;
    m_FoundFrame = true;

    if (pkttype == PKT_I_FRAME)
      m_firstIFrame = true;

    break;
  }

  default:
    break;
  }

  if (next_startcode >= 0x000001e0 && next_startcode <= 0x000001ef)
  {
    /* Complete frame */
    if (!m_FoundFrame)
      return true;

    /* Discard Packets until we have the picture size (XBMC can't enable VDPAU without it) */
    if (!m_firstIFrame || m_Width <= 0)
      return true;

    // send packet (will be cached if the stream isn't ready yet)
    m_FoundFrame        = false;
    m_StreamPacket.data = m_pictureBuffer;
    m_StreamPacket.size = m_pictureBufferPtr;
    SendPacket(&m_StreamPacket);

    // signal stream is ready
    m_Streamer->SetReady();
    return true;
  }

  return false;
}

int cParserH264::nalUnescape(uint8_t *dst, const uint8_t *src, int len)
{
  int s = 0, d = 0;

  while (s < len)
  {
    if (!src[s] && !src[s + 1])
    {
      // hit 00 00 xx
      dst[d] = dst[d + 1] = 0;
      s += 2;
      d += 2;
      if (src[s] == 3)
      {
        s++; // 00 00 03 xx --> 00 00 xx
        if (s >= len)
          return d;
      }
    }
    dst[d++] = src[s++];
  }

  return d;
}

bool cParserH264::Parse_PPS(uint8_t *buf, int len)
{
  cBitstream bs(buf, len*8);

  int pps_id = bs.readGolombUE();
  int sps_id = bs.readGolombUE();
  m_streamData.pps[pps_id].sps = sps_id;
  return true;
}

bool cParserH264::Parse_SLH(uint8_t *buf, int len, int *pkttype)
{
  cBitstream bs(buf, len*8);

  bs.readGolombUE(); /* first_mb_in_slice */
  int slice_type = bs.readGolombUE();

  if (slice_type > 4)
    slice_type -= 5;  /* Fixed slice type per frame */

  switch (slice_type)
  {
  case 0:
    *pkttype = PKT_P_FRAME;
    break;
  case 1:
    *pkttype = PKT_B_FRAME;
    break;
  case 2:
    *pkttype = PKT_I_FRAME;
    break;
  default:
    return false;
  }

  int pps_id = bs.readGolombUE();
  int sps_id = m_streamData.pps[pps_id].sps;
  if (m_streamData.sps[sps_id].cbpsize == 0)
    return false;

  m_vbvSize = m_streamData.sps[sps_id].cbpsize;
  m_vbvDelay = -1;
  return true;
}

bool cParserH264::Parse_SPS(uint8_t *buf, int len)
{
  cBitstream bs(buf, len*8);
  unsigned int tmp, frame_mbs_only;
  int cbpsize = -1;

  int profile_idc = bs.readBits(8);
  /* constraint_set0_flag = bs.readBits1();    */
  /* constraint_set1_flag = bs.readBits1();    */
  /* constraint_set2_flag = bs.readBits1();    */
  /* constraint_set3_flag = bs.readBits1();    */
  /* reserved             = bs.readBits(4);    */
  bs.skipBits(8);
  int level_idc = bs.readBits(8);
  unsigned int seq_parameter_set_id = bs.readGolombUE();

  unsigned int i = 0;
  while (h264_lev2cpbsize[i][0] != -1)
  {
    if (h264_lev2cpbsize[i][0] >= level_idc)
    {
      cbpsize = h264_lev2cpbsize[i][1];
      break;
    }
    i++;
  }
  if (cbpsize < 0)
    return false;

  m_streamData.sps[seq_parameter_set_id].cbpsize = cbpsize * 125; /* Convert from kbit to bytes */

  if (profile_idc >= 100)       /* high profile                   */
  {
    if(bs.readGolombUE() == 3)  /* chroma_format_idc              */
      bs.skipBits(1);           /* residual_colour_transform_flag */
    bs.readGolombUE();          /* bit_depth_luma - 8             */
    bs.readGolombUE();          /* bit_depth_chroma - 8           */
    bs.skipBits(1);             /* transform_bypass               */
    if (bs.readBits1())         /* seq_scaling_matrix_present     */
    {
      for (int i = 0; i < 8; i++)
      {
        if (bs.readBits1())     /* seq_scaling_list_present       */
        {
          int last = 8, next = 8, size = (i<6) ? 16 : 64;
          for (int j = 0; j < size; j++)
          {
            if (next)
              next = (last + bs.readGolombSE()) & 0xff;
            last = next ?: last;
          }
        }
      }
    }
  }

  bs.readGolombUE();           /* log2_max_frame_num - 4 */
  int pic_order_cnt_type = bs.readGolombUE();
  if (pic_order_cnt_type == 0)
    bs.readGolombUE();         /* log2_max_poc_lsb - 4 */
  else if (pic_order_cnt_type == 1)
  {
    bs.skipBits(1);            /* delta_pic_order_always_zero     */
    bs.readGolombSE();         /* offset_for_non_ref_pic          */
    bs.readGolombSE();         /* offset_for_top_to_bottom_field  */
    tmp = bs.readGolombUE();   /* num_ref_frames_in_pic_order_cnt_cycle */
    for (unsigned int i = 0; i < tmp; i++)
      bs.readGolombSE();       /* offset_for_ref_frame[i]         */
  }
  else if(pic_order_cnt_type != 2)
  {
    /* Illegal poc */
    return false;
  }

  bs.readGolombUE();          /* ref_frames                      */
  bs.skipBits(1);             /* gaps_in_frame_num_allowed       */
  m_Width  /* mbs */ = bs.readGolombUE() + 1;
  m_Height /* mbs */ = bs.readGolombUE() + 1;
  frame_mbs_only     = bs.readBits1();
  DEBUGLOG("H.264 SPS: pic_width:  %u mbs", (unsigned) m_Width);
  DEBUGLOG("H.264 SPS: pic_height: %u mbs", (unsigned) m_Height);
  DEBUGLOG("H.264 SPS: frame only flag: %d", frame_mbs_only);

  m_Width  *= 16;
  m_Height *= 16 * (2-frame_mbs_only);

  if (!frame_mbs_only)
  {
    if (bs.readBits1())     /* mb_adaptive_frame_field_flag */
      DEBUGLOG("H.264 SPS: MBAFF");
  }
  bs.skipBits(1);           /* direct_8x8_inference_flag    */
  if (bs.readBits1())       /* frame_cropping_flag */
  {
    uint32_t crop_left   = bs.readGolombUE();
    uint32_t crop_right  = bs.readGolombUE();
    uint32_t crop_top    = bs.readGolombUE();
    uint32_t crop_bottom = bs.readGolombUE();
    DEBUGLOG("H.264 SPS: cropping %d %d %d %d", crop_left, crop_top, crop_right, crop_bottom);

    m_Width -= 2*(crop_left + crop_right);
    if (frame_mbs_only)
      m_Height -= 2*(crop_top + crop_bottom);
    else
      m_Height -= 4*(crop_top + crop_bottom);
  }

  /* VUI parameters */
  m_PixelAspect.num = 0;
  if (bs.readBits1())    /* vui_parameters_present flag */
  {
    if (bs.readBits1())  /* aspect_ratio_info_present */
    {
      uint32_t aspect_ratio_idc = bs.readBits(8);
      DEBUGLOG("H.264 SPS: aspect_ratio_idc %d", aspect_ratio_idc);

      if (aspect_ratio_idc == 255 /* Extended_SAR */)
      {
        m_PixelAspect.num = bs.readBits(16); /* sar_width */
        m_PixelAspect.den = bs.readBits(16); /* sar_height */
        DEBUGLOG("H.264 SPS: -> sar %dx%d", m_PixelAspect.num, m_PixelAspect.den);
      }
      else
      {
        static const mpeg_rational_t aspect_ratios[] =
        { /* page 213: */
          /* 0: unknown */
          {0, 1},
          /* 1...16: */
          { 1,  1}, {12, 11}, {10, 11}, {16, 11}, { 40, 33}, {24, 11}, {20, 11}, {32, 11},
          {80, 33}, {18, 11}, {15, 11}, {64, 33}, {160, 99}, { 4,  3}, { 3,  2}, { 2,  1}
        };

        if (aspect_ratio_idc < sizeof(aspect_ratios)/sizeof(aspect_ratios[0]))
        {
          memcpy(&m_PixelAspect, &aspect_ratios[aspect_ratio_idc], sizeof(mpeg_rational_t));
          DEBUGLOG("H.264 SPS: PAR %d / %d", m_PixelAspect.num, m_PixelAspect.den);
        }
        else
        {
          DEBUGLOG("H.264 SPS: aspect_ratio_idc out of range !");
        }
      }
    }
  }

  DEBUGLOG("H.264 SPS: -> video size %dx%d, aspect %d:%d", m_Width, m_Height, m_PixelAspect.num, m_PixelAspect.den);
  return true;
}

