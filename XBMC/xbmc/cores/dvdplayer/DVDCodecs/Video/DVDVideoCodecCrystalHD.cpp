/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#define HAVE_MPCLINK // TODO: Remove this and define in configure/project
#if defined(HAVE_MPCLINK)
#include "DVDVideoCodecCrystalHD.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"

#if defined(WIN32)
#pragma comment(lib, "bcmDIL.lib")
#endif

/* We really don't want to include ffmpeg headers, so define these */
const int BC_CODEC_ID_MPEG2 = 2;
const int BC_CODEC_ID_VC1 = 73;
const int BC_CODEC_ID_H264 = 28;

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(0),
  m_DropPictures(false),
  m_PicturesDecoded(0),
  m_LastDecoded(0),
  m_pFormatName(""),
  m_FramesOut(0),
  m_OutputTimeout(0),
  m_LastPts(-1.0)
{
  memset(&m_Output, 0, sizeof(m_Output));
  memset(&m_CurrentFormat, 0, sizeof(m_CurrentFormat));
}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  BC_STATUS res;
  U32 mode = DTS_PLAYBACK_MODE | DTS_LOAD_FILE_PLAY_FW | DTS_SKIP_TX_CHK_CPB | DTS_DFLT_RESOLUTION(vdecRESOLUTION_720p23_976);
  
  U32 videoAlg = 0;
  switch (hints.codec)
  {
  case BC_CODEC_ID_VC1:
    videoAlg = BC_VID_ALGO_VC1;
    m_pFormatName = "bcm-vc1";
    break;
  case BC_CODEC_ID_H264:
    videoAlg = BC_VID_ALGO_H264;
    m_pFormatName = "bcm-h264";
    break;
  case BC_CODEC_ID_MPEG2:
    videoAlg = BC_VID_ALGO_MPEG2;
    m_pFormatName = "bcm-mpeg2";
    break;
  default:
    return false;
  }

  res = DtsDeviceOpen(&m_Device, mode);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __FUNCTION__);
    Dispose();
    Reset();
    return false;
  }

  res = DtsOpenDecoder(m_Device, BC_STREAM_TYPE_ES);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to open decoder", __FUNCTION__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsSetVideoParams(m_Device, videoAlg, FALSE, FALSE, TRUE, 0x80000000 | vdecFrameRate23_97);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to set video params", __FUNCTION__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsSet422Mode(m_Device, MODE420);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to set 422 mode", __FUNCTION__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsStartDecoder(m_Device);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to start decoder", __FUNCTION__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsStartCapture(m_Device);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to start capture", __FUNCTION__);
    Dispose();
    Reset();
    return false;
  }

  CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __FUNCTION__);
  return true;
}

void CDVDVideoCodecCrystalHD::Dispose()
{
  DtsStopDecoder(m_Device);
  DtsCloseDecoder(m_Device);
  DtsDeviceClose(m_Device);
  m_Device = 0;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
}

int CDVDVideoCodecCrystalHD::Decode(BYTE* pData, int iSize, double pts)
{
  if (!pData)
    return VC_BUFFER;

  //CLog::Log(LOGDEBUG, "%s: Tx %d bytes to decoder.", __FUNCTION__, iSize);
  BC_STATUS ret = DtsProcInput(m_Device, pData, iSize, (U64)(pts * (10000000.0 / DVD_TIME_BASE)), FALSE);
  if (ret != BC_STS_SUCCESS)
  {
    CLog::Log(LOGDEBUG, "%s: DtsProcInput returned %d.", __FUNCTION__, ret);
    return VC_ERROR;
  }

  if (IsPictureReady())
    return VC_PICTURE;

  return VC_BUFFER;
}

void CDVDVideoCodecCrystalHD::Reset()
{

}

bool CDVDVideoCodecCrystalHD::IsPictureReady()
{
  //memset(&m_Output, 0, sizeof(m_Output));
  //DtsReleaseOutputBuffs(m_Device, FALSE, FALSE); // Previous output is no longer valid

  m_Output.PoutFlags = BC_POUT_FLAGS_SIZE | BC_POUT_FLAGS_YV12;
  m_Output.PicInfo.width = m_CurrentFormat.width;
  m_Output.PicInfo.height = m_CurrentFormat.height;
  BC_STATUS ret = DtsProcOutput(m_Device, m_OutputTimeout, &m_Output);  
  switch (ret)
  {
  case BC_STS_SUCCESS:
    m_FramesOut++;
    return true;
  case BC_STS_NO_DATA:
    break;
  case BC_STS_FMT_CHANGE:
    CLog::Log(LOGDEBUG, "%s: Format Change Detected. Flags: 0x%08x", __FUNCTION__, m_Output.PoutFlags); 

    if ((m_Output.PoutFlags & BC_POUT_FLAGS_PIB_VALID) && (m_Output.PoutFlags & BC_POUT_FLAGS_FMT_CHANGE))
    {
      // Read format data from driver
      CLog::Log(LOGDEBUG, "%s: New Format", __FUNCTION__);
      CLog::Log(LOGDEBUG, "\t----------------------------------", __FUNCTION__);
      CLog::Log(LOGDEBUG, "\tTimeStamp: %llu", m_Output.PicInfo.timeStamp);
      CLog::Log(LOGDEBUG, "\tPicture Number: %lu", m_Output.PicInfo.picture_number);
      CLog::Log(LOGDEBUG, "\tWidth: %lu", m_Output.PicInfo.width);
      CLog::Log(LOGDEBUG, "\tHeight: %lu", m_Output.PicInfo.height);
      CLog::Log(LOGDEBUG, "\tChroma: 0x%03x", m_Output.PicInfo.chroma_format);
      CLog::Log(LOGDEBUG, "\tPulldown: %lu", m_Output.PicInfo.pulldown);         
      CLog::Log(LOGDEBUG, "\tFlags: 0x%08x", m_Output.PicInfo.flags);        
      CLog::Log(LOGDEBUG, "\tFrame Rate/Res: %lu", m_Output.PicInfo.frame_rate);       
      CLog::Log(LOGDEBUG, "\tAspect Ratio: %lu", m_Output.PicInfo.aspect_ratio);     
      CLog::Log(LOGDEBUG, "\tColor Primaries: %lu", m_Output.PicInfo.colour_primaries);
      CLog::Log(LOGDEBUG, "\tMetaData: %lu\n", m_Output.PicInfo.picture_meta_payload);
      CLog::Log(LOGDEBUG, "\tSession Number: %lu", m_Output.PicInfo.sess_num);
      CLog::Log(LOGDEBUG, "\tTimeStamp: %lu", m_Output.PicInfo.ycom);
      CLog::Log(LOGDEBUG, "\tCustom Aspect: %lu", m_Output.PicInfo.custom_aspect_ratio_width_height);
      CLog::Log(LOGDEBUG, "\tFrames to Drop: %lu", m_Output.PicInfo.n_drop);
      CLog::Log(LOGDEBUG, "\tH264 Valid Fields: 0x%08x", m_Output.PicInfo.other.h264.valid);   
      memcpy(&m_CurrentFormat, &m_Output.PicInfo, sizeof(BC_PIC_INFO_BLOCK));

      m_Output.YbuffSz = m_CurrentFormat.width * m_CurrentFormat.height;
      m_Output.UVbuffSz = m_Output.YbuffSz / 2;
      m_Output.Ybuff = (U8*)_aligned_malloc(m_Output.YbuffSz + m_Output.UVbuffSz, 4);
      m_Output.UVbuff =  m_Output.Ybuff + m_Output.YbuffSz;

      m_OutputTimeout = 15000;
    }
    break;
  case BC_STS_IO_XFR_ERROR:
    CLog::Log(LOGDEBUG, "%s: DtsProcOutput (Peek) returned BC_STS_IO_XFR_ERROR.", __FUNCTION__);
    break;
  case BC_STS_BUSY:
    CLog::Log(LOGDEBUG, "%s: DtsProcOutput (Peek) returned BC_STS_BUSY.", __FUNCTION__);
    break;
  default:
    CLog::Log(LOGDEBUG, "%s: DtsProcOutput (Peek) returned %lu.", __FUNCTION__, ret);
    break;
  }

  return false;
}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (m_Output.PicInfo.timeStamp == 0)
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  else
    pDvdVideoPicture->pts = (double)m_Output.PicInfo.timeStamp / (10000000.0 / DVD_TIME_BASE);

  pDvdVideoPicture->data[0] = m_Output.Ybuff; // Y plane
  pDvdVideoPicture->data[2] = m_Output.UVbuff; // U plane
  pDvdVideoPicture->data[1] = m_Output.UVbuff + (m_Output.UVbuffSz / 2); // V plane
  pDvdVideoPicture->iLineSize[0] = m_CurrentFormat.width;
  pDvdVideoPicture->iLineSize[1] = m_CurrentFormat.width/2;
  pDvdVideoPicture->iLineSize[2] = m_CurrentFormat.width/2;

  // Flags
  pDvdVideoPicture->iFlags |= (m_CurrentFormat.flags & VDEC_FLAG_BOTTOM_FIRST) ? 0 : DVP_FLAG_TOP_FIELD_FIRST;
  pDvdVideoPicture->iFlags |= (m_CurrentFormat.flags & VDEC_FLAG_INTERLACED_SRC) ? DVP_FLAG_INTERLACED : 0;
  //pDvdVideoPicture->iFlags |= pDvdVideoPicture->data[0] ? 0 : DVP_FLAG_DROPPED; // use n_drop

  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = 41711.111111;
  // pDvdVideoPicture->iFrameType
  // pDvdVideoPicture->color_matrix // colour_primaries

  pDvdVideoPicture->color_range = 0;
  pDvdVideoPicture->iWidth = m_CurrentFormat.width;
  pDvdVideoPicture->iHeight = m_CurrentFormat.height;
  pDvdVideoPicture->iDisplayWidth = m_CurrentFormat.width;
  pDvdVideoPicture->iDisplayHeight = m_CurrentFormat.height;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;

  return true;
}
#endif
