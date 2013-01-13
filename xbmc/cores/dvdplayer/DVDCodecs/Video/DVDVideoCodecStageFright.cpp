/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
#include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBSTAGEFRIGHT)
#include "DVDClock.h"
#include "settings/GUISettings.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecStageFright.h"
#include "StageFrightVideo.h"
#include "utils/log.h"

#define CLASSNAME "CDVDVideoCodecStageFright"
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecStageFright::CDVDVideoCodecStageFright() : CDVDVideoCodec()
{
  m_stf_decoder = NULL;
  m_pFormatName = "stf-xxxx";

  m_convert_bitstream = false;
}

CDVDVideoCodecStageFright::~CDVDVideoCodecStageFright()
{
  Dispose();
}

bool CDVDVideoCodecStageFright::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{  
  // we always qualify even if DVDFactoryCodec does this too.
  if (g_guiSettings.GetBool("videoplayer.usestagefright") && !hints.software)
  {
    m_convert_bitstream = false;

    switch (hints.codec)
    {
      case CODEC_ID_H264:
        m_pFormatName = "stf-h264";
        if (hints.extrasize < 7 || hints.extradata == NULL)
        {
          CLog::Log(LOGNOTICE,
              "%s::%s - avcC data too small or missing", CLASSNAME, __func__);
          return false;
        }
        // valid avcC data (bitstream) always starts with the value 1 (version)
        if ( *(char*)hints.extradata == 1 )
          m_convert_bitstream = bitstream_convert_init(hints.extradata, hints.extrasize);
        break;
      case CODEC_ID_MPEG4:
        m_pFormatName = "stf-mpeg4";
        break;
      case CODEC_ID_MPEG2VIDEO:
        m_pFormatName = "stf-mpeg2";
        break;
      case CODEC_ID_VP8:
        m_pFormatName = "stf-vp8";
        break;
      case CODEC_ID_VC1:
        m_pFormatName = "stf-vc1";
        break;
      default:
        return false;
        break;
    }

    m_stf_decoder = new CStageFrightVideo;
    if (!m_stf_decoder->Open(hints))
    {
      CLog::Log(LOGERROR,
          "%s::%s - failed to open, codec(%d), profile(%d), level(%d)", 
          CLASSNAME, __func__, hints.codec, hints.profile, hints.level);
      delete m_stf_decoder;
      m_stf_decoder = NULL;
      return false;
    }

    return true;
  }

  return false;
}

void CDVDVideoCodecStageFright::Dispose()
{
  if (m_stf_decoder)
  {
    m_stf_decoder->Close();
    delete m_stf_decoder;
    m_stf_decoder = NULL;
  }
  if (m_convert_bitstream)
  {
    if (m_sps_pps_context.sps_pps_data)
    {
      free(m_sps_pps_context.sps_pps_data);
      m_sps_pps_context.sps_pps_data = NULL;
    }
  }
}

void CDVDVideoCodecStageFright::SetDropState(bool bDrop)
{
  m_stf_decoder->SetDropState(bDrop);
}

int CDVDVideoCodecStageFright::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  int rtn;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;
  bool bitstream_convered  = false;

  if (m_convert_bitstream && demuxer_content)
  {
    // convert demuxer packet from bitstream to bytestream (AnnexB)
    int bytestream_size = 0;
    uint8_t *bytestream_buff = NULL;

    bitstream_convert(demuxer_content, demuxer_bytes, &bytestream_buff, &bytestream_size);
    if (bytestream_buff && (bytestream_size > 0))
    {
      bitstream_convered = true;
      demuxer_bytes = bytestream_size;
      demuxer_content = bytestream_buff;
    } else
      CLog::Log(LOGERROR,"%s::%s - bitstream_convert error", CLASSNAME, __func__);
  }

  rtn = m_stf_decoder->Decode(demuxer_content, demuxer_bytes, dts, pts);

  if (bitstream_convered)
    free(demuxer_content);

  return rtn;
}

void CDVDVideoCodecStageFright::Reset(void)
{
  m_stf_decoder->Reset();
}

bool CDVDVideoCodecStageFright::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_decoder->GetPicture(pDvdVideoPicture);
}

bool CDVDVideoCodecStageFright::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return m_stf_decoder->ClearPicture(pDvdVideoPicture);
}


////////////////////////////////////////////////////////////////////////////////////////////
bool CDVDVideoCodecStageFright::bitstream_convert_init(void *in_extradata, int in_extrasize)
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

bool CDVDVideoCodecStageFright::bitstream_convert(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
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
      bitstream_alloc_and_copy(poutbuf, poutbuf_size,
          m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      bitstream_alloc_and_copy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size);
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

void CDVDVideoCodecStageFright::bitstream_alloc_and_copy(
    uint8_t **poutbuf,      int *poutbuf_size,
    const uint8_t *sps_pps, uint32_t sps_pps_size,
    const uint8_t *in,      uint32_t in_size)
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

#endif
