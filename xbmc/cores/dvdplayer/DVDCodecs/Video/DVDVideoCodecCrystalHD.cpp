/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "DllAvCodec.h"
#endif

#if defined(HAVE_LIBCRYSTALHD)
#include "settings/GUISettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecCrystalHD.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#define __MODULE_NAME__ "DVDVideoCodecCrystalHD"

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Codec(NULL),
  m_DropPictures(false),
  m_Duration(0.0),
  m_pFormatName(""),
  m_CodecType(CRYSTALHD_CODEC_ID_MPEG2)
{
}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (g_guiSettings.GetBool("videoplayer.usechd") && !hints.software)
  {
    switch (hints.codec)
    {
      case CODEC_ID_MPEG2VIDEO:
        m_CodecType = CRYSTALHD_CODEC_ID_MPEG2;
        m_pFormatName = "chd-mpeg2";
      break;
      case CODEC_ID_H264:
        switch(hints.profile)
        {
          case FF_PROFILE_H264_HIGH_10:
          case FF_PROFILE_H264_HIGH_10_INTRA:
          case FF_PROFILE_H264_HIGH_422:
          case FF_PROFILE_H264_HIGH_422_INTRA:
          case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
          case FF_PROFILE_H264_HIGH_444_INTRA:
          case FF_PROFILE_H264_CAVLC_444:
            CLog::Log(LOGNOTICE, "%s - unsupported h264 profile(%d)", __FUNCTION__, hints.profile);
            return false;
            break;
        }
        if (hints.extrasize < 7 || hints.extradata == NULL)
        {
          CLog::Log(LOGNOTICE, "%s - avcC atom too data small or missing", __FUNCTION__);
          return false;
        }
        // valid avcC data (bitstream) always starts with the value 1 (version)
        if ( *(char*)hints.extradata == 1 )
          m_CodecType = CRYSTALHD_CODEC_ID_AVC1;
        else
          m_CodecType = CRYSTALHD_CODEC_ID_H264;

        m_pFormatName = "chd-h264";
      break;
      case CODEC_ID_VC1:
        m_CodecType = CRYSTALHD_CODEC_ID_VC1;
        m_pFormatName = "chd-vc1";
      break;
      case CODEC_ID_WMV3:
        m_CodecType = CRYSTALHD_CODEC_ID_WMV3;
        m_pFormatName = "chd-wmv3";
      break;
      default:
        return false;
      break;
    }

    m_Codec = CCrystalHD::GetInstance();
    if (!m_Codec)
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }

    if (m_Codec && !m_Codec->OpenDecoder(m_CodecType, hints))
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }

    // default duration to 23.976 fps, have to guess something.
    m_Duration = (DVD_TIME_BASE / (24.0 * 1000.0/1001.0));
    m_DropPictures = false;

    CLog::Log(LOGINFO, "%s: Opened Broadcom Crystal HD Codec", __MODULE_NAME__);
    return true;
  }

  return false;
}

void CDVDVideoCodecCrystalHD::Dispose(void)
{
  if (m_Codec)
  {
    m_Codec->CloseDecoder();
    m_Codec = NULL;
  }
}

int CDVDVideoCodecCrystalHD::Decode(BYTE *pData, int iSize, double dts, double pts)
{
  if (!pData)
  {
    // if pData is nil, we are in dvdplayervideo's special loop
    // where it checks for more picture frames, you must pass
    // VC_BUFFER to get it to break out of this loop.
    int ready_cnt = m_Codec->GetReadyCount();
    if (ready_cnt == 1)
      return VC_PICTURE | VC_BUFFER;
    if (ready_cnt > 2)
      return VC_PICTURE;
    else
      return VC_BUFFER;
  }

  // We are running a picture queue, picture frames are allocated
  // in CrystalHD class if needed, then passed up. Need to return
  // them back to CrystalHD class for re-queuing. This way we keep
  // the memory alloc/free to a minimum and don't churn memory for
  // each picture frame.
  m_Codec->BusyListFlush();

  if (pData)
  {
    // Handle Input, add demuxer packet to input queue, we must accept it or
    // it will be discarded as DVDPlayerVideo has no concept of "try again".
    if ( !m_Codec->AddInput(pData, iSize, dts, pts) )
    {
      // Deep crap error, this should never happen unless we run away pulling demuxer pkts.
      CLog::Log(LOGDEBUG, "%s: m_pInputThread->AddInput full.", __MODULE_NAME__);
      Sleep(10);
    }
  }

  // if we have more than one frame ready, just return VC_PICTURE so 
  // dvdplayervideo will loop and drain them before sending another demuxer packet.
  if (m_Codec->GetReadyCount() > 2)
    return VC_PICTURE;
  
  int rtn = 0;
  if (m_Codec->GetReadyCount())
    rtn = VC_PICTURE;

  return rtn | VC_BUFFER;
}

void CDVDVideoCodecCrystalHD::Reset(void)
{
  m_Codec->Reset();
}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  bool  ret;
  
  ret = m_Codec->GetPicture(pDvdVideoPicture);
  m_Duration = pDvdVideoPicture->iDuration;
  return ret;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
  m_Codec->SetDropState(m_DropPictures);
}

#endif
