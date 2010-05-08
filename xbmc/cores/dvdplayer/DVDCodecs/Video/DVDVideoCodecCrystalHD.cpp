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
#elif defined(_WIN32)
#include "system.h"
#include "libavcodec/avcodec.h"
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
  m_DecodeStarted(false),
  m_DropPictures(false),
  m_Duration(0.0),
  m_pFormatName("")
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
        m_codec_type = CRYSTALHD_CODEC_ID_MPEG2;
        m_pFormatName = "chd-mpeg2";
      break;
      case CODEC_ID_H264:
        if (hints.extrasize < 7 || hints.extradata == NULL)
        {
          CLog::Log(LOGNOTICE, "%s - avcC atom too data small or missing", __FUNCTION__);
          return false;
        }
        m_codec_type = CRYSTALHD_CODEC_ID_H264;
        m_pFormatName = "chd-h264";
      break;
      case CODEC_ID_VC1:
        m_codec_type = CRYSTALHD_CODEC_ID_VC1;
        m_pFormatName = "chd-vc1";
      break;
      case CODEC_ID_WMV3:
        m_codec_type = CRYSTALHD_CODEC_ID_WMV3;
        m_pFormatName = "chd-wmv3";
      break;
      default:
        return false;
      break;
    }

    m_Device = CCrystalHD::GetInstance();
    if (!m_Device)
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }

    if (m_Device && !m_Device->OpenDecoder(m_codec_type, hints.extrasize, hints.extradata))
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }

    // default duration to 23.976 fps, have to guess something.
    m_Duration = (DVD_TIME_BASE / (24.0 * 1000.0/1001.0));
    m_DropPictures = false;
    m_DecodeStarted = false;

    CLog::Log(LOGINFO, "%s: Opened Broadcom Crystal HD Codec", __MODULE_NAME__);
    return true;
  }

  return false;
}

void CDVDVideoCodecCrystalHD::Dispose(void)
{
  if (m_Device)
  {
    m_Device->CloseDecoder();
    m_Device = NULL;
  }
}

int CDVDVideoCodecCrystalHD::Decode(BYTE *pData, int iSize, double dts, double pts)
{
  int ret = 0;

  // We are running a picture queue, picture frames are allocated
  // in CrystalHD class if needed, then passed up. Need to return
  // them back to CrystalHD class for re-queuing. This way we keep
  // the memory alloc/free to a minimum and don't churn memory for
  // each picture frame.
  m_Device->BusyListFlush();

  // If NULL is passed, DVDPlayer wants to process any queued picture frames.
  if (!pData)
  {
    // Always return VC_PICTURE if we have one ready.
    if (m_Device->GetReadyCount() > 0)
      ret |= VC_PICTURE;

    // Returning VC_BUFFER only breaks out of DVDPlayerVideo's picture process loop
    // which it then grabs another demuxer packet and calls this routine again.
    // We only do this if the ready picture count drops to below 4 or
    // risk draining vqueue which then will cause DVDPlayer thrashing. 
    if ((m_Device->GetInputCount() < 2) && (m_Device->GetReadyCount() < 6))
      ret |= VC_BUFFER;

    // if no picture ready and input is full, we must wait
    if(!(ret & VC_PICTURE) && !(ret & VC_BUFFER))
      m_Device->WaitInput(100);
      
    return ret;
  }

  // Handle Input, add demuxer packet to input queue, we must accept it or
  // it will be discarded as DVDPlayerVideo has no concept of "try again".
  if ( !m_Device->AddInput(pData, iSize, dts, pts) )
  {
    // Deep crap error, this should never happen unless we run away pulling demuxer pkts.
    CLog::Log(LOGDEBUG, "%s: m_pInputThread->AddInput full.", __MODULE_NAME__);
    Sleep(10);
  }

#if 1
  // Fake a decoding delay of 1/2 the frame duration, this helps keep DVDPlayerVideo from
  // draining the demuxer queue. DVDPlayerVideo expects one picture frame for each demuxer
  // packet so we sleep a little here to give the decoder a chance to output a frame.
  if (!m_DropPictures)
  {
    if (m_Duration > 0.0)
      Sleep((DWORD)(m_Duration/2000.0));
    else
      Sleep(20);
  }
#endif

  // Handle Output, we delay passing back VC_PICTURE on startup until we have a few
  // decoded picture frames as DVDPlayerVideo might discard picture frames when it
  // tries to sync with audio.
  if (m_DecodeStarted)
  {
    if (m_Device->GetReadyCount() > 0)
      ret |= VC_PICTURE;
    if (m_Device->GetInputCount() < 2 && (m_Device->GetReadyCount() < 4))
      ret |= VC_BUFFER;
  }
  else
  {
    if (m_Device->GetReadyCount() > 4)
    {
      m_DecodeStarted = true;
      ret |= VC_PICTURE;
    }
    if (m_Device->GetInputCount() < 2)
      ret |= VC_BUFFER;
  }

  return ret;
}

void CDVDVideoCodecCrystalHD::Reset(void)
{
  // Decoder flush, reset started flag and dump all input and output.
  m_DecodeStarted = false;
  m_Device->Reset();
}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  bool  ret;
  
  ret = m_Device->GetPicture(pDvdVideoPicture);
  m_Duration = pDvdVideoPicture->iDuration;
  pDvdVideoPicture->iDuration = 0;
  return ret;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
  m_Device->SetDropState(m_DropPictures);
}

#endif
