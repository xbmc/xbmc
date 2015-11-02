/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 #include "DVDVideoCodecRK.h"
 #include "utils/SysfsUtils.h"
 #include "settings/Settings.h"
 #include <sys/system_properties.h>

 #ifndef RK_KODI_DEBUG
 #define RK_KODI_DEBUG
 #endif

CDVDVideoCodecRK::CDVDVideoCodecRK() :
    m_Codec(NULL),
    m_pFormatName("rkcodec"),
    m_opened(false),
    m_bitstream(NULL),
    m_bitparser(NULL)
{
   #ifdef RK_KODI_DEBUG
  CLog::Log(LOGDEBUG, "RK: DVDVideoCodecRK Constructed!");
  #endif
}

CDVDVideoCodecRK::~CDVDVideoCodecRK()
{
  Dispose();
}

bool CDVDVideoCodecRK::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  /* check rk support */
  if (!isRkHwSupport(hints))
    return false;
  
  m_hints.codec = hints.codec;
  m_hints.codec_tag = hints.codec_tag;
  m_hints.height = hints.height;
  m_hints.width = hints.width;
  m_hints.extrasize = hints.extrasize;
  m_hints.extradata = malloc(m_hints.extrasize);

  memcpy(m_hints.extradata, hints.extradata, m_hints.extrasize);

  if (hints.codec == AV_CODEC_ID_H264 || hints.codec == AV_CODEC_ID_AVS || hints.codec == AV_CODEC_ID_CAVS)
  {
    /* we needn't to convert h264-avcC to h264-annex-b as h264-avcC  */
  }
  else if (hints.codec == AV_CODEC_ID_HEVC)
  {
    CLog::Log(LOGDEBUG,"CodecRK Bit Create!");
    m_bitstream = new CBitstreamConverter();
    m_bitstream->Open(m_hints.codec, (uint8_t*)m_hints.extradata, m_hints.extrasize, true);
    free(m_hints.extradata);
    m_hints.extrasize = m_bitstream->GetExtraSize();
    m_hints.extradata = malloc(m_hints.extrasize);
    memcpy(m_hints.extradata, m_bitstream->GetExtraData(), m_hints.extrasize);
  }
  
  m_Codec = new CRKCodec();
  m_opened = false;
  return true;
}

#define ENABLE_RK_HEVC_HARDWARE_DEC;

bool CDVDVideoCodecRK::isRkHwSupport(CDVDStreamInfo &hints)
{
  AVCodecID codec_id = hints.codec;
  unsigned int codec_tag = hints.codec_tag;

  /*  here rk is not support divx */
  if ((codec_tag == MKTAG('3', 'I', 'V', 'D')) ||
      (codec_tag == MKTAG('D', 'I', 'V', 'X')) ||
      (codec_tag == MKTAG('X', 'V', 'I', 'D')) ||
      (codec_tag == MKTAG('3', 'I', 'V', '2')) )
  {
    CLog::Log(LOGDEBUG, "We not support DIVX!");
    return false;
  }
  
  char name[5] = {0};
  if (name != NULL) 
  {
    name[0] = (codec_tag&0xff);
    name[1] = (codec_tag&0xff00)>>8;
    name[2] = (codec_tag&0x00ff0000)>>16;
    name[3] = (codec_tag&0xff000000)>>24;
  }
  std::string codecName(name);
  std::transform(codecName.begin(), codecName.end(), codecName.begin(), ::tolower);
  if (strstr(codecName.c_str(),"div") != 0 || strstr(codecName.c_str(),"vid") != 0 ||
            strstr(codecName.c_str(),"dx50") != 0) 
  {
    CLog::Log(LOGDEBUG, "We not support DIVX!");
    return false;
  }

  /* check video codec */
  switch (codec_id) 
  {
    case AV_CODEC_ID_RV10:
    case AV_CODEC_ID_RV20:
    case AV_CODEC_ID_RV30:
    case AV_CODEC_ID_RV40:
      return false;
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
      return true;
    case AV_CODEC_ID_MPEG4:
      if (hints.width >= 3840 || hints.height >= 2160)
        return false;
    case AV_CODEC_ID_FLV1:
    case AV_CODEC_ID_H264:
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
    case AV_CODEC_ID_VP8:
#ifdef ENABLE_RK_HEVC_HARDWARE_DEC
    case AV_CODEC_ID_HEVC:
#endif
      CLog::Log(LOGDEBUG,"ISRKSupport true %d", codec_id);
      return true;
    default:
      CLog::Log(LOGDEBUG,"ISRKSupport false %d", codec_id);
      return false;
  }
  return false;
} 


int CDVDVideoCodecRK::Decode(uint8_t *pData, int iSize, double dts, double pts)
{
  if (pData)
  {
    /* convert h264-avcC to h264-annex-b as h264-avcC */
    if (m_bitstream)
    {
      if (!m_bitstream->Convert(pData, iSize))
        return VC_ERROR;
      pData = m_bitstream->GetConvertBuffer();
      iSize = m_bitstream->GetConvertSize();
    }
  }
  if(!m_opened)
  { 
    if (m_Codec && !m_Codec->OpenDecoder(m_hints))
      CLog::Log(LOGDEBUG, " Failed to open Rockchip Codec!");
    m_opened = true;
    /* support for 23.976 match */ 
//    m_Codec->Set23976Match(CSettings::GetInstance().GetBool("videoplayer.userkfpsmatch"));
    CLog::Log(LOGDEBUG,"CDVDVideoCodecRK openDecoder Success");
  }
  return m_Codec->Decode(pData, iSize, dts, pts);
}


void CDVDVideoCodecRK::Dispose(void)
{
  #ifdef RK_KODI_DEBUG
  CLog::Log(LOGDEBUG, "CDVDVideoCodecRK::Dispose()!");
  #endif
  
  if (m_Codec)
    m_Codec->CloseDecoder(), delete m_Codec, m_Codec = NULL;
  
  if (m_bitstream)
    delete m_bitstream, m_bitstream = NULL;

  if (m_bitparser)
    delete m_bitparser, m_bitparser = NULL;
}

void CDVDVideoCodecRK::Reset(void)
{
  m_Codec->Reset();
}


void CDVDVideoCodecRK::Flush(void)
{
  m_Codec->Flush();
}

bool CDVDVideoCodecRK::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (m_Codec)
    m_Codec->GetPicture(&m_videobuffer);
  
  *pDvdVideoPicture = m_videobuffer;
  pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth = m_hints.width;
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight = m_hints.height;
  
  return true;
}

void CDVDVideoCodecRK::SetDropState(bool bDrop)
{
  if(m_Codec)
    m_Codec->SetDropState(bDrop);
}

void CDVDVideoCodecRK::SetSpeed(int iSpeed)
{
  if(m_Codec)
    m_Codec->SetSpeed(iSpeed);
}

int CDVDVideoCodecRK::GetDataSize(void)
{
  if(m_Codec)
    return m_Codec->GetDataSize();
  return 0;
}

double CDVDVideoCodecRK::GetTimeSize(void)
{
  if(m_Codec)
    return m_Codec->GetTimeSize();
  return 0.0;
}

/* C Functions */
void rk_set_audio_passthrough(bool passthrough)
{
  CLog::Log(LOGDEBUG,"RKTEST rk_set_audio_passthrough %d",passthrough);
  if (!SysfsUtils::HasRW("/sdcard/passthrough"))
  {
    FILE* fd = fopen("/sdcard/passthrough","wb+");
    fclose(fd);
  }
  if (SysfsUtils::HasRW("/sdcard/passthrough"))
  {
    SysfsUtils::SetInt("/sdcard/passthrough", passthrough ? 1:0);
  }
}

int rk_get_audio_setting()
{
  char buf[PROP_VALUE_MAX];  
  __system_property_get("persist.audio.currentplayback", buf);
  return atoi(buf);
}

int64_t rk_get_adjust_latency()
{
  return RKAdjustLatency;
}

