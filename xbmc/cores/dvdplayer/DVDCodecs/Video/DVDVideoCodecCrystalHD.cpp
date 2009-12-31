/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#if defined(HAVE_LIBCRYSTALHD)
#include "GUISettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecCrystalHD.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#define __MODULE_NAME__ "DVDVideoCodecCrystalHD"

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(NULL),
  m_DropPictures(false),
  m_pFormatName("")
{
}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  int requestedMethod = g_guiSettings.GetInt("videoplayer.rendermethod");
  
  if ((requestedMethod == RENDER_METHOD_CRYSTALHD) && !hints.software)
  {
    BCM_CODEC_TYPE codec_type;
    BCM_STREAM_TYPE stream_type;
    
    codec_type = hints.codec;
    stream_type = BC_STREAM_TYPE_ES;
    
    if (hints.codec == CODEC_ID_H264)
      m_annexbfiltering = init_h264_mp4toannexb_filter(hints);
    else
      m_annexbfiltering = false;

    m_Device = CCrystalHD::GetInstance();
    if (!m_Device)
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }
    
    if (m_Device && !m_Device->Open(stream_type, codec_type))
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }

    m_DropPictures = false;
    
    CLog::Log(LOGINFO, "%s: Opened Broadcom Crystal HD Codec", __MODULE_NAME__);
    return true;
  }
  
  return false;
}

void CDVDVideoCodecCrystalHD::Dispose(void)
{
  if (m_Device)
  {
    m_Device->Close();
    m_Device = NULL;
  }
}

int CDVDVideoCodecCrystalHD::Decode(BYTE *pData, int iSize, double pts)
{
  int ret = 0;
  bool annexbfiltered = false;

  m_Device->BusyListPop();
  
  // in NULL is passed, DVDPlayer wants us to flush any internal picture frame.
  // we don't have internal picture frames so just return.
  if (!pData)
    return VC_BUFFER;

  if (m_annexbfiltering)
  {
    int outbuf_size = 0;
    uint8_t *outbuf = NULL;
    
    h264_mp4toannexb_filter(pData, iSize, &outbuf, &outbuf_size);
    if (outbuf)
    {
      annexbfiltered = true;
      pData = outbuf;
      iSize = outbuf_size;
    }
  }
  
  if (pData)
  {
    if ( m_Device->AddInput(pData, iSize, pts) )
    {
      if (annexbfiltered)
        free(pData);
      pData = NULL;
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s: m_pInputThread->AddInput full.", __MODULE_NAME__);
      Sleep(10);
    }
  }
    
  if (m_Device->GetInputCount() < 10)
    ret |= VC_BUFFER;

  if (!m_DropPictures)
    Sleep(20);
      
  // Handle Output
  if (m_Device->GetReadyCount())
    ret |= VC_PICTURE;
    
  if (!ret)
    ret = VC_ERROR;

  return ret;
}

void CDVDVideoCodecCrystalHD::Reset(void)
{
  CLog::Log(LOGDEBUG, "%s: Reset, flushing decoder.", __MODULE_NAME__);   
  m_Device->Flush();
}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return  m_Device->GetPicture(pDvdVideoPicture);
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
  m_Device->SetDropState(m_DropPictures);
}

////////////////////////////////////////////////////////////////////////////////////////////
bool CDVDVideoCodecCrystalHD::init_h264_mp4toannexb_filter(CDVDStreamInfo &hints)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater
  
  m_sps_pps_data = NULL;
  m_sps_pps_size = 0;

  // nothing to filter
  if (!hints.extradata || hints.extrasize < 6)
    return false;

  uint16_t unit_size;
  uint32_t total_size = 0;
  uint8_t *out = NULL, unit_nb, sps_done = 0;
  const uint8_t *extradata = (uint8_t*)hints.extradata + 4;
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
    if ( (extradata + 2 + unit_size) > ((uint8_t*)hints.extradata + hints.extrasize) )
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

void CDVDVideoCodecCrystalHD::alloc_and_copy(uint8_t **poutbuf,     int *poutbuf_size,
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
  {
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);
  }
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

bool CDVDVideoCodecCrystalHD::h264_mp4toannexb_filter(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater
  
  uint8_t   *buf = pData;
  int       buf_size = iSize;
  uint8_t   unit_type;
  uint32_t  nal_size, cumul_size = 0;

  do
  {
    if (m_sps_pps_context.length_size == 1)
      nal_size = buf[0];
    else if (m_sps_pps_context.length_size == 2)
      nal_size = buf[0] << 8 | buf[1];
    else
      nal_size = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];

    buf += m_sps_pps_context.length_size;
    unit_type = *buf & 0x1f;

    // prepend only to the first type 5 NAL unit of an IDR picture
    if (m_sps_pps_context.first_idr && unit_type == 5)
    {
      alloc_and_copy(poutbuf, poutbuf_size, 
        m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      alloc_and_copy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size);
      if (!m_sps_pps_context.first_idr && unit_type == 1)
          m_sps_pps_context.first_idr = 1;
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;
}

#endif
