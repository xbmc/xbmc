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
#include "DVDVideoCodecCrystalHD.h"

#if defined(HAVE_CRYSTALHD)
#include "DVDClock.h"
#include "DVDStreamInfo.h"

#if defined(WIN32)
#pragma comment(lib, "bcmDIL.lib")
#endif

/* We really don't want to include ffmpeg headers, so define these */
const int CODEC_ID_MPEG2 = 2;
const int CODEC_ID_VC1 = 73;
const int CODEC_ID_H264 = 28;

#define OUTPUT_PROC_TIMEOUT 10

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(0),
  m_Height(0),
  m_Width(0),
  m_pBuffer(NULL),
  m_YSize(0),
  m_UVSize(0),
  m_DropPictures(false),
  m_PicturesDecoded(0),
  m_LastDecoded(0),
  m_pFormatName("")
{

}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  BC_STATUS res;
  U32 mode = DTS_PLAYBACK_MODE | DTS_LOAD_FILE_PLAY_FW | DTS_SKIP_TX_CHK_CPB | DTS_DFLT_RESOLUTION(vdecRESOLUTION_480i);
  
  U32 videoAlg = 0;
  switch (hints.codec)
  {
  case CODEC_ID_VC1:
    videoAlg = BC_VID_ALGO_VC1;
    m_pFormatName = "bcm-vc1";
    break;
  case CODEC_ID_H264:
    videoAlg = BC_VID_ALGO_H264;
    m_pFormatName = "bcm-h264";
    break;
  case CODEC_ID_MPEG2:
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

  m_Height = hints.height;
  m_Width = hints.width;

  // Create a 4-byte aligned m_Output buffer
  m_YSize = m_Height * m_Width;
  m_UVSize = m_YSize / 2;
  m_pBuffer =  (U8*)malloc(m_YSize + m_UVSize + 4);

  CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __FUNCTION__);
  return true;
}

void CDVDVideoCodecCrystalHD::SetSize(unsigned int height, unsigned int width)
{
  if (m_Height == height && m_Width == width)
    return;

  m_Height = height;
  m_Width = width;

  if (m_pBuffer)
    free(m_pBuffer);

  if (m_Height && m_Width)
  {
    // Create a 4-byte aligned m_Output buffer
    m_YSize = m_Height * m_Width;
    m_UVSize = m_YSize / 2;
    m_pBuffer =  (U8*)malloc(m_YSize + m_UVSize + 4);
  }
  else
  {
    m_YSize = 0;
    m_UVSize = 0;
    m_pBuffer = NULL;
  }
}

void CDVDVideoCodecCrystalHD::Dispose()
{
  DtsStopDecoder(m_Device);
  DtsCloseDecoder(m_Device);
  DtsDeviceClose(m_Device);
  m_Device = 0;
  free(m_pBuffer);
  m_pBuffer = NULL;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
}

int CDVDVideoCodecCrystalHD::Decode(BYTE* pData, int iSize, double pts)
{
  BC_STATUS ret = DtsProcInput(m_Device, pData, iSize, 0/*(U64)(pts * 1000000000)*/, FALSE);
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
  InitOutput(&m_Output);
  BC_STATUS ret = DtsProcOutput(m_Device, 0, &m_Output);  
  switch (ret)
  {
  case BC_STS_SUCCESS:
    if (m_Output.PoutFlags & ~BC_POUT_FLAGS_PIB_VALID)
      return true;
    CLog::Log(LOGDEBUG, "%s: Recieved picture with no PIB. Flags: 0x%08x", __FUNCTION__, m_Output.PoutFlags);
    break;
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
      SetSize(m_Output.PicInfo.height, m_Output.PicInfo.width);
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
  //pDvdVideoPicture->pts = (double)m_Output.PicInfo.timeStamp / (DVD_TIME_BASE * 1000);
  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->data[0] = m_Output.Ybuff; // Y plane
  pDvdVideoPicture->data[2] = m_Output.UVbuff; // U plane
  pDvdVideoPicture->data[1] = m_Output.UVbuff + (m_UVSize / 2); // V plane
  pDvdVideoPicture->iLineSize[0] = m_Width;
  pDvdVideoPicture->iLineSize[1] = m_Width/4;
  pDvdVideoPicture->iLineSize[2] = m_Width/4;

  // Flags
  pDvdVideoPicture->iFlags |= (m_Output.PicInfo.flags & VDEC_FLAG_BOTTOM_FIRST) ? 0 : DVP_FLAG_TOP_FIELD_FIRST;
  // DVP_FLAG_ALLOCATED == 0
  pDvdVideoPicture->iFlags |= (m_Output.PicInfo.flags & VDEC_FLAG_INTERLACED_SRC) ? DVP_FLAG_INTERLACED : 0;
  //pDvdVideoPicture->iFlags |= pDvdVideoPicture->data[0] ? 0 : DVP_FLAG_DROPPED; // use n_drop

  // pDvdVideoPicture->iRepeatPicture = ??
  pDvdVideoPicture->iDuration = 41711.111111;
  // pDvdVideoPicture->iFrameType
  // pDvdVideoPicture->color_matrix // colour_primaries

  pDvdVideoPicture->color_range = 0;
  pDvdVideoPicture->iWidth = m_Output.PicInfo.width;
  pDvdVideoPicture->iHeight = m_Output.PicInfo.height;
  pDvdVideoPicture->iDisplayWidth = m_Output.PicInfo.width;
  pDvdVideoPicture->iDisplayHeight = m_Output.PicInfo.height;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;

  return true;
}

void CDVDVideoCodecCrystalHD::InitOutput(BC_DTS_PROC_OUT* pOut)
{
  U8* alignedBuf = m_pBuffer;
  if(((unsigned int)m_pBuffer)%4)
  {
    // TODO: This will not work on x86_64. Use _aligned_malloc
    U8 oddBytes = 4 - ((U8)((DWORD)m_pBuffer % 4));
    alignedBuf = m_pBuffer + oddBytes;
  }

  memset(pOut, 0, sizeof(BC_DTS_PROC_OUT));
  pOut->PicInfo.width = m_Width;
  pOut->PicInfo.height = m_Height;
  pOut->Ybuff = alignedBuf;
  pOut->YbuffSz = m_YSize/4;
  // If UV is in use, it's data immediately follows Y
  if (m_UVSize)
    pOut->UVbuff = alignedBuf + m_YSize;
  else
    pOut->UVbuff = NULL;
  pOut->UVbuffSz = m_UVSize/4;
  pOut->PoutFlags = BC_POUT_FLAGS_SIZE;
}
#endif
