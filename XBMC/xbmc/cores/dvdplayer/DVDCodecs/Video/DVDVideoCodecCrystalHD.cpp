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

#define __MODULE_NAME__ "MPCLink"

/* We really don't want to include ffmpeg headers, so define these */
const int BC_CODEC_ID_MPEG2 = 2;
const int BC_CODEC_ID_VC1 = 73;
const int BC_CODEC_ID_H264 = 28;

class CExecTimer
{
public:
  CExecTimer() :
    m_StartTime(0),
    m_StopTime(0),
    m_PunchInTime(0),
    m_PunchOutTime(0),
    m_LastCallInterval(0)
  {
    
  }
  
  void Start()
  {
    m_StartTime = m_StopTime = GetTickCount();
  }
  
  void PunchIn()
  {
    m_PunchInTime = GetTickCount();
    if (m_PunchOutTime)
      m_LastCallInterval = m_PunchInTime - m_PunchOutTime;
    else
      m_LastCallInterval = 0;
    m_PunchOutTime = 0;
  }
  
  void PunchOut()
  {
    if (m_PunchInTime)
      m_PunchOutTime = GetTickCount();
  }
  
  void Reset()
  {
    m_StartTime = 0;
    m_StopTime = 0;
    m_PunchInTime = 0;
    m_PunchOutTime = 0;
    m_LastCallInterval = 0;
  }
  
  uint64_t GetElapsedTime()
  {
    if (m_StartTime)
      return GetTickCount() - m_StartTime;  
    else
      return 0;
  }
  
  uint64_t GetExecTime()
  {
    if (m_PunchOutTime && m_PunchInTime)
      return m_PunchOutTime - m_PunchInTime;  
    else
      return 0;
  }
  
  uint64_t GetIntervalTime()
  {
    return m_LastCallInterval;
  }
  
protected:
  uint64_t m_StartTime;
  uint64_t m_StopTime;
  uint64_t m_PunchInTime;
  uint64_t m_PunchOutTime;
  uint64_t m_LastCallInterval;
};

CExecTimer g_InputTimer;
CExecTimer g_OutputTimer;

unsigned int CMPCDecodeBuffer::m_NextId = 0;

CMPCDecodeBuffer::CMPCDecodeBuffer(size_t size) :
m_Size(size)
{
  m_Id = m_NextId++;
  m_pBuffer = (unsigned char*)_aligned_malloc(size, 4);
}

CMPCDecodeBuffer::~CMPCDecodeBuffer()
{
  _aligned_free(m_pBuffer);
}

size_t CMPCDecodeBuffer::GetSize()
{
  return m_Size;
}

unsigned char* CMPCDecodeBuffer::GetPtr()
{
  return m_pBuffer;
}

#ifdef __APPLE__
CMPCDecodeThread::CMPCDecodeThread(void* device) :
#else
CMPCDecodeThread::CMPCDecodeThread(HANDLE device) :
#endif
  CThread(),
  m_Device(device),
  m_OutputTimeout(0),
  m_BufferCount(0)
{
  m_OutputHeight = 1080;
  m_OutputWidth = 1920;
  
  InitializeCriticalSection(&m_FreeLock);
  InitializeCriticalSection(&m_ReadyLock);
  
//  lf_queue_init(&m_FreeBuffers);
//  lf_queue_init(&m_DecodedFrames);
}

CMPCDecodeThread::~CMPCDecodeThread()
{
  while(m_ReadyList.size())
  {
    delete m_ReadyList.front();
    m_ReadyList.pop_front();
  }
  while(m_FreeList.size())
  {
    delete m_FreeList.front();
    m_FreeList.pop_front();
  }
}

unsigned int CMPCDecodeThread::GetReadyCount()
{
//  return m_DecodedFrames.len;
  return m_ReadyList.size();
}

void CMPCDecodeThread::AddFrame(CMPCDecodeBuffer* pBuffer)
{
//  lf_queue_enqueue(&m_DecodedFrames, pBuffer);
  CSingleLock lock(m_ReadyLock);
  m_ReadyList.push_back(pBuffer);
}

CMPCDecodeBuffer* CMPCDecodeThread::GetNext()
{
//  return (CMPCDecodeBuffer*)lf_queue_dequeue(&m_DecodedFrames);
  CSingleLock lock(m_ReadyLock);
  CMPCDecodeBuffer* pBuf = NULL;
  if (m_ReadyList.size())
  {
    pBuf = m_ReadyList.front();
    m_ReadyList.pop_front();
  }
  return pBuf;
}

void CMPCDecodeThread::FreeBuffer(CMPCDecodeBuffer* pBuffer)
{
//  lf_queue_enqueue(&m_FreeBuffers, &pBuffer);
  CSingleLock lock(m_FreeLock);
  m_FreeList.push_back(pBuffer);
}

CMPCDecodeBuffer* CMPCDecodeThread::AllocBuffer()
{
//  return (CMPCDecodeBuffer*)lf_queue_dequeue(&m_FreeBuffers) ;
  CSingleLock lock(m_FreeLock);
  CMPCDecodeBuffer* pBuf = NULL;
  if (m_FreeList.size())
  {
    pBuf = m_FreeList.front();
    m_FreeList.pop_front();
  }
  return pBuf;
}


void CMPCDecodeThread::Process()
{
  CLog::Log(LOGDEBUG, "%s: Output Thread Started...", __MODULE_NAME__);
  while (!m_bStop)
  {
    CMPCDecodeBuffer* pBuffer = GetDecoderOutput();
    if (pBuffer)
    {
      CLog::Log(LOGDEBUG, "%s: Moving Buffer %d (Free -> Ready)", __MODULE_NAME__, pBuffer->GetId());    
      AddFrame(pBuffer);
    }
    else
      Sleep(1);
  }
  CLog::Log(LOGDEBUG, "%s: MPCLink Output Thread Stopped...", __MODULE_NAME__);
}

CMPCDecodeBuffer* CMPCDecodeThread::GetDecoderOutput()
{
    // Get next output buffer from the free list
  CMPCDecodeBuffer* pBuffer = AllocBuffer();
  if (!pBuffer) // No free buffers
  {
    if (m_BufferCount >= 3)
      return NULL;
    pBuffer = new CMPCDecodeBuffer(m_OutputWidth * m_OutputHeight * 2); // Allocate a new buffer
    CLog::Log(LOGDEBUG, "%s: Added a new Buffer. Id: %d, Size: %d", __MODULE_NAME__, pBuffer->GetId(), pBuffer->GetSize());    
    m_BufferCount++;
  }
  else
    CLog::Log(LOGDEBUG, "%s: Got Buffer. Id: %d, Size: %d", __MODULE_NAME__, pBuffer->GetId(), pBuffer->GetSize());    

// Static picture testing
//  char luma = GetTickCount() & 0xFF;
//  memset(pBuffer->GetPtr(), luma, pBuffer->GetSize());
//  g_OutputTimer.PunchIn();
//  g_OutputTimer.PunchOut();
//  CLog::Log(LOGDEBUG, "%s: OutputTimer - %llu exec / %llu interval. CPU: %0.04f%%", __MODULE_NAME__, g_OutputTimer.GetExecTime(), g_OutputTimer.GetIntervalTime(), GetRelativeUsage() * 100);
//  return pBuffer;
  
  // Set-up output struct
  BC_DTS_PROC_OUT procOut;
  memset(&procOut, 0, sizeof(BC_DTS_PROC_OUT));
  procOut.PoutFlags = BC_POUT_FLAGS_SIZE | BC_POUT_FLAGS_YV12;
  procOut.PicInfo.width = m_OutputWidth;
  procOut.PicInfo.height = m_OutputHeight;
  procOut.YbuffSz = m_OutputWidth * m_OutputHeight;
  procOut.UVbuffSz = procOut.YbuffSz / 2;
  if ((procOut.YbuffSz + procOut.UVbuffSz) > pBuffer->GetSize())
  {
    CLog::Log(LOGDEBUG, "%s: Buffer %d was too small (%d bytes)...reallocating (%d bytes)", __MODULE_NAME__, pBuffer->GetId(), pBuffer->GetSize(), procOut.YbuffSz + procOut.UVbuffSz);    
    delete pBuffer;
    pBuffer = new CMPCDecodeBuffer(procOut.YbuffSz + procOut.UVbuffSz); // Allocate a new buffer
  }
  procOut.Ybuff = (U8*)pBuffer->GetPtr();
  procOut.UVbuff =  procOut.Ybuff + procOut.YbuffSz;
  
  // Fetch data from the decoder
  g_OutputTimer.PunchIn();
  BC_STATUS ret = DtsProcOutput(m_Device, m_OutputTimeout, &procOut);
  g_OutputTimer.PunchOut();
  CLog::Log(LOGDEBUG, "%s: OutputTimer - %llu exec / %llu interval. (Status: %02x Timeout: %d)", __MODULE_NAME__, g_OutputTimer.GetExecTime(), g_OutputTimer.GetIntervalTime(), ret, m_OutputTimeout);
  
  switch (ret)
  {
    case BC_STS_SUCCESS:
      return pBuffer;
    case BC_STS_NO_DATA:
      break;
    case BC_STS_FMT_CHANGE:
      CLog::Log(LOGDEBUG, "%s: Format Change Detected. Flags: 0x%08x", __MODULE_NAME__, procOut.PoutFlags); 
      
      if ((procOut.PoutFlags & BC_POUT_FLAGS_PIB_VALID) && (procOut.PoutFlags & BC_POUT_FLAGS_FMT_CHANGE))
      {
        // Read format data from driver
        CLog::Log(LOGDEBUG, "%s: New Format", __MODULE_NAME__);
        CLog::Log(LOGDEBUG, "%s: \t----------------------------------", __MODULE_NAME__);
        CLog::Log(LOGDEBUG, "%s: \tTimeStamp: %llu", __MODULE_NAME__, procOut.PicInfo.timeStamp);
        CLog::Log(LOGDEBUG, "%s: \tPicture Number: %lu", __MODULE_NAME__, procOut.PicInfo.picture_number);
        CLog::Log(LOGDEBUG, "%s: \tWidth: %lu", __MODULE_NAME__, procOut.PicInfo.width);
        CLog::Log(LOGDEBUG, "%s: \tHeight: %lu", __MODULE_NAME__, procOut.PicInfo.height);
        CLog::Log(LOGDEBUG, "%s: \tChroma: 0x%03x", __MODULE_NAME__, procOut.PicInfo.chroma_format);
        CLog::Log(LOGDEBUG, "%s: \tPulldown: %lu", __MODULE_NAME__, procOut.PicInfo.pulldown);         
        CLog::Log(LOGDEBUG, "%s: \tFlags: 0x%08x", __MODULE_NAME__, procOut.PicInfo.flags);        
        CLog::Log(LOGDEBUG, "%s: \tFrame Rate/Res: %lu", __MODULE_NAME__, procOut.PicInfo.frame_rate);       
        CLog::Log(LOGDEBUG, "%s: \tAspect Ratio: %lu", __MODULE_NAME__, procOut.PicInfo.aspect_ratio);     
        CLog::Log(LOGDEBUG, "%s: \tColor Primaries: %lu", __MODULE_NAME__, procOut.PicInfo.colour_primaries);
        CLog::Log(LOGDEBUG, "%s: \tMetaData: %lu\n", __MODULE_NAME__, procOut.PicInfo.picture_meta_payload);
        CLog::Log(LOGDEBUG, "%s: \tSession Number: %lu", __MODULE_NAME__, procOut.PicInfo.sess_num);
        CLog::Log(LOGDEBUG, "%s: \tTimeStamp: %lu", __MODULE_NAME__, procOut.PicInfo.ycom);
        CLog::Log(LOGDEBUG, "%s: \tCustom Aspect: %lu", __MODULE_NAME__, procOut.PicInfo.custom_aspect_ratio_width_height);
        CLog::Log(LOGDEBUG, "%s: \tFrames to Drop: %lu", __MODULE_NAME__, procOut.PicInfo.n_drop);
        CLog::Log(LOGDEBUG, "%s: \tH264 Valid Fields: 0x%08x", __MODULE_NAME__, procOut.PicInfo.other.h264.valid);   
        m_OutputTimeout = 15000;
      }
      break;
    case BC_STS_IO_XFR_ERROR:
      CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned BC_STS_IO_XFR_ERROR.", __MODULE_NAME__);
      break;
    case BC_STS_BUSY:
      CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned BC_STS_BUSY.", __MODULE_NAME__);
      break;
    default:
      CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned %lu.", __MODULE_NAME__, ret);
      break;
  }  
  FreeBuffer(pBuffer); // Use it again later
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(0),
  m_DropPictures(false),
  m_PicturesDecoded(0),
  m_LastDecoded(0),
  m_pFormatName(""),
  m_FramesOut(0),
  m_PacketsIn(0),
  m_OutputTimeout(0),
  m_LastPts(-1.0)
{
  memset(&m_Output, 0, sizeof(m_Output));
  memset(&m_CurrentFormat, 0, sizeof(m_CurrentFormat));
    
  m_OutputTimeout = 15000;
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
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }

  res = DtsOpenDecoder(m_Device, BC_STREAM_TYPE_ES);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to open decoder", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsSetVideoParams(m_Device, videoAlg, FALSE, FALSE, TRUE, 0x80000000 | vdecFrameRate23_97);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to set video params", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsSet422Mode(m_Device, MODE420);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to set 422 mode", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsStartDecoder(m_Device);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to start decoder", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = DtsStartCapture(m_Device);
  if (res != BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to start capture", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }

  g_InputTimer.Reset();
  g_OutputTimer.Reset();
  g_InputTimer.Start();
  g_OutputTimer.Start();
    
  m_pDecodeThread = new CMPCDecodeThread(m_Device);
  m_pDecodeThread->Create();

  CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __MODULE_NAME__);
  return true;
}

void CDVDVideoCodecCrystalHD::Dispose()
{
  if (m_pDecodeThread)
    m_pDecodeThread->StopThread();
  delete m_pDecodeThread;
  m_pDecodeThread = NULL;
  
  DtsStopDecoder(m_Device);
  DtsCloseDecoder(m_Device);
  DtsDeviceClose(m_Device);
  m_Device = 0;
  
  while(m_BusyList.size())
  {
    delete m_BusyList.front();
    m_BusyList.pop_front();
  }  
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
}

int CDVDVideoCodecCrystalHD::Decode(BYTE* pData, int iSize, double pts)
{
//  return VC_PICTURE;

  if (m_BusyList.size())
  {
    CMPCDecodeBuffer* pBuffer = m_BusyList.front();
    m_pDecodeThread->FreeBuffer(pBuffer);
    CLog::Log(LOGDEBUG, "%s: Moving Buffer %d (Busy -> Free)", __MODULE_NAME__, pBuffer->GetId());    
    m_BusyList.pop_front();
  }
  
  if (!pData)
    return VC_BUFFER;

  //CLog::Log(LOGDEBUG, "%s: Tx %d bytes to mpclink.", __MODULE_NAME__, iSize);
  g_InputTimer.PunchIn();
  BC_STATUS ret = DtsProcInput(m_Device, pData, iSize, 0/*(U64)(pts * (10000000.0 / DVD_TIME_BASE))*/, FALSE);
  g_InputTimer.PunchOut();
  if (ret != BC_STS_SUCCESS)
  {
    CLog::Log(LOGDEBUG, "%s: DtsProcInput returned %d.", __MODULE_NAME__, ret);
    return VC_ERROR;
  }
  m_PacketsIn++;
  CLog::Log(LOGDEBUG, "%s: InputTimer - %llu exec / %llu interval.", __MODULE_NAME__, g_InputTimer.GetExecTime(), g_InputTimer.GetIntervalTime());

  for (unsigned int waitTime = 0; waitTime < 100; waitTime += 10)
  {    
    if (m_pDecodeThread->GetReadyCount())
    {
      CLog::Log(LOGDEBUG, "%s: Decoded Picture is Ready. %d In, %d Out, %d Ready", __MODULE_NAME__, m_PacketsIn, m_FramesOut, m_pDecodeThread->GetReadyCount());
      return VC_PICTURE;
    }
    Sleep(5);
  }  
  return VC_BUFFER;
}

void CDVDVideoCodecCrystalHD::Reset()
{

}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  //CLog::Log(LOGDEBUG, "%s: Fetching next decoded picture", __MODULE_NAME__);   
  CMPCDecodeBuffer* pBuffer = m_pDecodeThread->GetNext();
  if (!pBuffer)
    return false;
 
  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->data[0] = pBuffer->GetPtr(); // Y plane
  pDvdVideoPicture->data[2] = pBuffer->GetPtr() + 2073600; // U plane
  pDvdVideoPicture->data[1] = pBuffer->GetPtr() + 2073600 + 518400; // V plane
  pDvdVideoPicture->iLineSize[0] = 1920;
  pDvdVideoPicture->iLineSize[1] = 960;
  pDvdVideoPicture->iLineSize[2] = 960;
  
  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = 41711.111111;
  pDvdVideoPicture->color_range = 0;
  pDvdVideoPicture->iWidth = 1920;
  pDvdVideoPicture->iHeight = 1080;
  pDvdVideoPicture->iDisplayWidth = 1920;
  pDvdVideoPicture->iDisplayHeight = 1080;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;  
  
  CLog::Log(LOGDEBUG, "%s: Moving Buffer %d (Ready -> Busy)", __MODULE_NAME__, pBuffer->GetId());   
  m_BusyList.push_back(pBuffer);
  
//  if (m_Output.PicInfo.timeStamp == 0)
//    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
//  else
//    pDvdVideoPicture->pts = (double)m_Output.PicInfo.timeStamp / (10000000.0 / DVD_TIME_BASE);
//
//  
//  pDvdVideoPicture->data[0] = m_Output.Ybuff; // Y plane
//  pDvdVideoPicture->data[2] = m_Output.UVbuff; // U plane
//  pDvdVideoPicture->data[1] = m_Output.UVbuff + (m_Output.UVbuffSz / 2); // V plane
//  pDvdVideoPicture->iLineSize[0] = m_CurrentFormat.width;
//  pDvdVideoPicture->iLineSize[1] = m_CurrentFormat.width/2;
//  pDvdVideoPicture->iLineSize[2] = m_CurrentFormat.width/2;
//
//  // Flags
//  pDvdVideoPicture->iFlags |= (m_CurrentFormat.flags & VDEC_FLAG_BOTTOM_FIRST) ? 0 : DVP_FLAG_TOP_FIELD_FIRST;
//  pDvdVideoPicture->iFlags |= (m_CurrentFormat.flags & VDEC_FLAG_INTERLACED_SRC) ? DVP_FLAG_INTERLACED : 0;
//  //pDvdVideoPicture->iFlags |= pDvdVideoPicture->data[0] ? 0 : DVP_FLAG_DROPPED; // use n_drop
//
//  pDvdVideoPicture->iRepeatPicture = 0;
//  pDvdVideoPicture->iDuration = 41711.111111;
//  // pDvdVideoPicture->iFrameType
//  // pDvdVideoPicture->color_matrix // colour_primaries
//
//  pDvdVideoPicture->color_range = 0;
//  pDvdVideoPicture->iWidth = m_CurrentFormat.width;
//  pDvdVideoPicture->iHeight = m_CurrentFormat.height;
//  pDvdVideoPicture->iDisplayWidth = m_CurrentFormat.width;
//  pDvdVideoPicture->iDisplayHeight = m_CurrentFormat.height;
//  pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;

  m_FramesOut++;
  return true;
}

/////////////////////////////////////////////////////////////////////


#endif
