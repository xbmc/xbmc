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
    m_PunchInTime(0),
    m_PunchOutTime(0),
    m_LastCallInterval(0)
  {
    QueryPerformanceFrequency((LARGE_INTEGER*)&m_CounterFreq);
    m_CounterFreq /= 1000; // Scale to ms
  }
  
  void Start()
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&m_StartTime);
  }
  
  void PunchIn()
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&m_PunchInTime);
    if (m_PunchOutTime)
      m_LastCallInterval = m_PunchInTime - m_PunchOutTime;
    else
      m_LastCallInterval = 0;
    m_PunchOutTime = 0;
  }
  
  void PunchOut()
  {
    if (m_PunchInTime)
      QueryPerformanceCounter((LARGE_INTEGER*)&m_PunchOutTime);
  }
  
  void Reset()
  {
    m_StartTime = 0;
    m_PunchInTime = 0;
    m_PunchOutTime = 0;
    m_LastCallInterval = 0;
  }
  
  uint64_t GetElapsedTime()
  {
    if (m_StartTime)
    {
      uint64_t now;
      QueryPerformanceCounter((LARGE_INTEGER*)&now);
      return (now - m_StartTime)/m_CounterFreq;  
    }
    else
      return 0;
  }
  
  uint64_t GetExecTime()
  {
    if (m_PunchOutTime && m_PunchInTime)
      return (m_PunchOutTime - m_PunchInTime)/m_CounterFreq;  
    else
      return 0;
  }
  
  uint64_t GetIntervalTime()
  {
    return m_LastCallInterval/m_CounterFreq;
  }
  
protected:
  uint64_t m_StartTime;
  uint64_t m_PunchInTime;
  uint64_t m_PunchOutTime;
  uint64_t m_LastCallInterval;
  uint64_t m_CounterFreq;
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

CMPCDecodeThread::CMPCDecodeThread(BCM::HANDLE device) :
  CThread(),
  m_Device(device),
  m_OutputTimeout(0),
  m_BufferCount(0)
{
  m_OutputHeight = 1080;
  m_OutputWidth = 1920;
  
  InitializeCriticalSection(&m_FreeLock);
  InitializeCriticalSection(&m_ReadyLock);
  InitializeCriticalSection(&m_InputLock);
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
  return m_ReadyList.size();
}

void CMPCDecodeThread::AddFrame(CMPCDecodeBuffer* pBuffer)
{
  CSingleLock lock(m_ReadyLock);
  m_ReadyList.push_back(pBuffer);
}

CMPCDecodeBuffer* CMPCDecodeThread::GetNext()
{
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
  CSingleLock lock(m_FreeLock);
  m_FreeList.push_back(pBuffer);
}

CMPCDecodeBuffer* CMPCDecodeThread::AllocBuffer()
{
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
    if (GetReadyCount() < 3) // Need more frames in ready list
    {
      if (m_InputList.size()) // Read from the input queue and push to decoder
      {
        CSingleLock lock(m_InputLock);
        CMPCDecodeBuffer* pInput = m_InputList.front();
        m_InputList.pop_front();
        lock.Leave();

        g_InputTimer.PunchIn();
        BCM::BC_STATUS ret = BCM::DtsProcInput(m_Device, pInput->GetPtr(), pInput->GetSize(), 0, FALSE);
        g_InputTimer.PunchOut();
        if (ret != BCM::BC_STS_SUCCESS)
          CLog::Log(LOGDEBUG, "%s: DtsProcInput returned %d.", __MODULE_NAME__, ret);
        CLog::Log(LOGDEBUG, "%s: InputTimer (%d bytes) - %llu exec / %llu interval. %d in queue", __MODULE_NAME__, pInput->GetSize(), g_InputTimer.GetExecTime(), g_InputTimer.GetIntervalTime(), m_InputList.size());
        delete pInput;
      }
      CMPCDecodeBuffer* pBuffer = GetDecoderOutput(); // Check for output frames
      if (pBuffer)
      {
        CLog::Log(LOGDEBUG, "%s: Moving Buffer %d (Free -> Ready)", __MODULE_NAME__, pBuffer->GetId());    
        AddFrame(pBuffer);
      }
    }
    else
      Sleep(20);
  }
  CLog::Log(LOGDEBUG, "%s: MPCLink Output Thread Stopped...", __MODULE_NAME__);
}

bool CMPCDecodeThread::AddInput(CMPCDecodeBuffer* pBuffer)
{
  CSingleLock lock(m_InputLock);
  m_InputList.push_back(pBuffer);
  return true;
}

unsigned int CMPCDecodeThread::GetInputCount()
{
  return m_InputList.size();
}

CMPCDecodeBuffer* CMPCDecodeThread::GetDecoderOutput()
{
    // Get next output buffer from the free list
  CMPCDecodeBuffer* pBuffer = AllocBuffer();
  if (!pBuffer) // No free buffers
  {
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
  BCM::BC_DTS_PROC_OUT procOut;
  memset(&procOut, 0, sizeof(BCM::BC_DTS_PROC_OUT));
  procOut.PoutFlags = BCM::BC_POUT_FLAGS_SIZE | BCM::BC_POUT_FLAGS_YV12;
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
  procOut.Ybuff = (BCM::U8*)pBuffer->GetPtr();
  procOut.UVbuff =  procOut.Ybuff + procOut.YbuffSz;
  
  // Fetch data from the decoder
  g_OutputTimer.PunchIn();
  BCM::BC_STATUS ret = DtsProcOutput(m_Device, m_OutputTimeout, &procOut);
  g_OutputTimer.PunchOut();
  CLog::Log(LOGDEBUG, "%s: OutputTimer - %llu exec / %llu interval. (Status: %02x Timeout: %d CPU %0.02f)", __MODULE_NAME__, g_OutputTimer.GetExecTime(), g_OutputTimer.GetIntervalTime(), ret, m_OutputTimeout, GetRelativeUsage() * 100.0);
  
  switch (ret)
  {
    case BCM::BC_STS_SUCCESS:
      return pBuffer;
    case BCM::BC_STS_NO_DATA:
      Sleep(41);
      break;
    case BCM::BC_STS_FMT_CHANGE:
      CLog::Log(LOGDEBUG, "%s: Format Change Detected. Flags: 0x%08x", __MODULE_NAME__, procOut.PoutFlags); 
      
      if ((procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID) && (procOut.PoutFlags & BCM::BC_POUT_FLAGS_FMT_CHANGE))
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
        m_OutputTimeout = 20;
      }
      break;
    case BCM::BC_STS_IO_XFR_ERROR:
      CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned BC_STS_IO_XFR_ERROR.", __MODULE_NAME__);
      break;
    case BCM::BC_STS_BUSY:
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
  BCM::BC_STATUS res;
  BCM::U32 mode = BCM::DTS_PLAYBACK_MODE | BCM::DTS_LOAD_FILE_PLAY_FW | BCM::DTS_SKIP_TX_CHK_CPB | DTS_DFLT_RESOLUTION(BCM::vdecRESOLUTION_720p23_976);
  
  BCM::U32 videoAlg = 0;
  switch (hints.codec)
  {
  case BC_CODEC_ID_VC1:
    videoAlg = BCM::BC_VID_ALGO_VC1;
    m_pFormatName = "bcm-vc1";
    break;
  case BC_CODEC_ID_H264:
    videoAlg = BCM::BC_VID_ALGO_H264;
    m_pFormatName = "bcm-h264";
    break;
  case BC_CODEC_ID_MPEG2:
    videoAlg = BCM::BC_VID_ALGO_MPEG2;
    m_pFormatName = "bcm-mpeg2";
    break;
  default:
    return false;
  }

  res = BCM::DtsDeviceOpen(&m_Device, mode);
  if (res != BCM::BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }

  res = BCM::DtsOpenDecoder(m_Device, BCM::BC_STREAM_TYPE_ES);
  if (res != BCM::BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to open decoder", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = BCM::DtsSetVideoParams(m_Device, videoAlg, FALSE, FALSE, TRUE, 0x80000000 | BCM::vdecFrameRate23_97);
  if (res != BCM::BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to set video params", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = BCM::DtsSet422Mode(m_Device, BCM::MODE420);
  if (res != BCM::BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to set 422 mode", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = BCM::DtsStartDecoder(m_Device);
  if (res != BCM::BC_STS_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s: Failed to start decoder", __MODULE_NAME__);
    Dispose();
    Reset();
    return false;
  }
  res = BCM::DtsStartCapture(m_Device);
  if (res != BCM::BC_STS_SUCCESS)
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
  
  BCM::DtsStopDecoder(m_Device);
  BCM::DtsCloseDecoder(m_Device);
  BCM::DtsDeviceClose(m_Device);
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
  if (m_BusyList.size())
  {
    CMPCDecodeBuffer* pBuffer = m_BusyList.front();
    m_pDecodeThread->FreeBuffer(pBuffer);
    CLog::Log(LOGDEBUG, "%s: Moving Buffer %d (Busy -> Free)", __MODULE_NAME__, pBuffer->GetId());    
    m_BusyList.pop_front();
  }
  
  if (!pData)
    return VC_BUFFER;

  CMPCDecodeBuffer* pBuffer = new CMPCDecodeBuffer(iSize);
  memcpy(pBuffer->GetPtr(), pData, iSize);
  m_pDecodeThread->AddInput(pBuffer);

  for (unsigned int waitTime = 0; waitTime < 40; waitTime += 5)
  {    
    if (m_pDecodeThread->GetReadyCount())
    {
      CLog::Log(LOGDEBUG, "%s: Decoded Picture is Ready (%d wait). %d In, %d Out, %d Ready", __MODULE_NAME__, waitTime, m_PacketsIn, m_FramesOut, m_pDecodeThread->GetReadyCount());
      return VC_PICTURE;
    }
    if (m_pDecodeThread->GetInputCount() > 4)
      Sleep(5);
  }  
  return VC_BUFFER;

  //g_InputTimer.PunchIn();
  //BCM::BC_STATUS ret = BCM::DtsProcInput(m_Device, pData, iSize, 0/*(U64)(pts * (10000000.0 / DVD_TIME_BASE))*/, FALSE);
  //g_InputTimer.PunchOut();
  //if (ret != BCM::BC_STS_SUCCESS)
  //{
  //  CLog::Log(LOGDEBUG, "%s: DtsProcInput returned %d.", __MODULE_NAME__, ret);
  //  return VC_ERROR;
  //}
  //m_PacketsIn++;

  //CLog::Log(LOGDEBUG, "%s: InputTimer (%d bytes) - %llu exec / %llu interval.\t%d %llu %llu", __MODULE_NAME__, iSize, g_InputTimer.GetExecTime(), g_InputTimer.GetIntervalTime(), iSize, g_InputTimer.GetExecTime(), g_InputTimer.GetIntervalTime());
  //if (g_InputTimer.GetExecTime() > 40)
  //{
  //  CLog::Log(LOGERROR, "%s: **INPUT PROCESSING TIME EXCEEDED FRAME DURATION**", __MODULE_NAME__);
  //}
  //for (unsigned int waitTime = 0; waitTime < 100; waitTime += 10)
  //{    
  //if (m_pDecodeThread->GetReadyCount())
  //{
  //  CLog::Log(LOGDEBUG, "%s: Decoded Picture is Ready. %d In, %d Out, %d Ready", __MODULE_NAME__, m_PacketsIn, m_FramesOut, m_pDecodeThread->GetReadyCount());
  //  return VC_PICTURE;
  //}
  //  Sleep(5);
  //}  
  //return VC_BUFFER;
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
