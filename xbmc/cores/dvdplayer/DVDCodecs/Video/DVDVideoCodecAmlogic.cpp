/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "DVDVideoCodecAmlogic.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "AMLCodec.h"
#include "video/VideoThumbLoader.h"
#include "utils/log.h"

#define __MODULE_NAME__ "DVDVideoCodecAmlogic"

CDVDVideoCodecAmlogic::CDVDVideoCodecAmlogic() :
  m_Codec(NULL),
  m_pFormatName("")
{
}

CDVDVideoCodecAmlogic::~CDVDVideoCodecAmlogic()
{
  Dispose();
}

bool CDVDVideoCodecAmlogic::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  switch(hints.codec)
  {
    case CODEC_ID_MPEG1VIDEO:
    case CODEC_ID_MPEG2VIDEO:
    case CODEC_ID_MPEG2VIDEO_XVMC:
      m_pFormatName = "am-mpeg2";
      break;
    case CODEC_ID_H264:
      m_pFormatName = "am-h264";
      break;
    case CODEC_ID_MPEG4:
    case CODEC_ID_MSMPEG4V2:
    case CODEC_ID_MSMPEG4V3:
      m_pFormatName = "am-mpeg4";
      break;
    case CODEC_ID_H263:
    case CODEC_ID_H263P:
    case CODEC_ID_H263I:
      m_pFormatName = "am-h263";
      break;
    case CODEC_ID_FLV1:
      m_pFormatName = "am-flv1";
      break;
    case CODEC_ID_RV10:
    case CODEC_ID_RV20:
    case CODEC_ID_RV30:
    case CODEC_ID_RV40:
      m_pFormatName = "am-rv";
      break;
    case CODEC_ID_VC1:
      m_pFormatName = "am-vc1";
      break;
    case CODEC_ID_WMV3:
      m_pFormatName = "am-wmv3";
      break;
    default:
      return false;
      break;
  }

  m_hints = hints;
  m_Codec = new CAMLCodec();
  if (!m_Codec)
  {
    CLog::Log(LOGERROR, "%s: Failed to create Amlogic Codec", __MODULE_NAME__);
    return false;
  }
  m_opened = false;

  // allocate a dummy DVDVideoPicture buffer.
  // first make sure all properties are reset.
  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));

  m_videobuffer.dts = DVD_NOPTS_VALUE;
  m_videobuffer.pts = DVD_NOPTS_VALUE;
  m_videobuffer.format = RENDER_FMT_BYPASS;
  m_videobuffer.color_range  = 0;
  m_videobuffer.color_matrix = 4;
  m_videobuffer.iFlags  = DVP_FLAG_ALLOCATED;
  m_videobuffer.iWidth  = hints.width;
  m_videobuffer.iHeight = hints.height;
  m_videobuffer.iDisplayWidth  = hints.width;
  m_videobuffer.iDisplayHeight = hints.height;

  CJobManager::GetInstance().Pause(kJobTypeMediaFlags);
  if (CJobManager::GetInstance().IsProcessing(kJobTypeMediaFlags) > 0)
  {
    int timeout_ms = 5000;
    // use m_bStop and Sleep so we can get canceled.
    while (timeout_ms > 0)
    {
      if (CJobManager::GetInstance().IsProcessing(kJobTypeMediaFlags) > 0)
      {
        Sleep(100);
        timeout_ms -= 100;
      }
      else
        break;
    }
  }

  CLog::Log(LOGINFO, "%s: Opened Amlogic Codec", __MODULE_NAME__);
  return true;
}

void CDVDVideoCodecAmlogic::Dispose(void)
{
  if (m_Codec)
  {
    m_Codec->CloseDecoder();
    m_Codec = NULL;
  }
  if (m_videobuffer.iFlags & DVP_FLAG_ALLOCATED)
  {
    m_videobuffer.iFlags = 0;
  }
  // let thumbgen jobs resume.
  CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);
}

int CDVDVideoCodecAmlogic::Decode(BYTE *pData, int iSize, double dts, double pts)
{
  // Handle Input, add demuxer packet to input queue, we must accept it or
  // it will be discarded as DVDPlayerVideo has no concept of "try again".
  if (!m_opened)
  {
    if (m_Codec && !m_Codec->OpenDecoder(m_hints))
      CLog::Log(LOGERROR, "%s: Failed to open Amlogic Codec", __MODULE_NAME__);
    m_opened = true;
  }

  return m_Codec->Decode(pData, iSize, dts, pts);
}

void CDVDVideoCodecAmlogic::Reset(void)
{
  m_Codec->Reset();
}

bool CDVDVideoCodecAmlogic::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  m_Codec->GetPicture(&m_videobuffer);
  *pDvdVideoPicture = m_videobuffer;

  return true;
}

void CDVDVideoCodecAmlogic::SetDropState(bool bDrop)
{
}

int CDVDVideoCodecAmlogic::GetDataSize(void)
{
  return m_Codec->GetDataSize();
}

double CDVDVideoCodecAmlogic::GetTimeSize(void)
{
  return m_Codec->GetTimeSize();
}
