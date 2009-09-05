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

//#if (defined HAVE_CONFIG_H) && (!defined WIN32)
//  #include "config.h"
//#endif

#include "stdafx.h"
#if defined(HAVE_LIBCRYSTALHD)

#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecCrystalHD.h"

extern void* fast_memcpy(void * to, const void * from, size_t len);

#if defined(WIN32)
#pragma comment(lib, "bcmDIL.lib")
#endif

#define LOG_LEVEL 0
#define LOG_ERROR(x, ...) CLog::Log(LOGERROR, x, __VA_ARGS__)
#define LOG_WARN(x, ...) if(LOG_LEVEL > 0) CLog::Log(LOGWARNING, x, __VA_ARGS__)
#define LOG_INFO(x, ...) if(LOG_LEVEL > 1) CLog::Log(LOGINFO, x, __VA_ARGS__)
#define LOG_DEBUG(x, ...) if(LOG_LEVEL > 2) CLog::Log(LOGDEBUG, x, __VA_ARGS__)

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

  uint64_t GetTimeSincePunchIn()
  {
    if (m_PunchInTime)
    {
      uint64_t now;
      QueryPerformanceCounter((LARGE_INTEGER*)&now);
      return (now - m_PunchInTime)/m_CounterFreq;  
    }
    else
      return 0;
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

CMPCDecodeBuffer::CMPCDecodeBuffer(size_t size) :
m_Size(size)
{
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

void CMPCDecodeBuffer::SetPts(BCM::U64 pts)
{
  m_Pts = pts;
}

BCM::U64 CMPCDecodeBuffer::GetPts()
{
  return m_Pts;
}

////////////////////////////////////////////////////////////////////////////////////////////

CMPCInputThread::CMPCInputThread(BCM::HANDLE device) :
  CThread(),
  m_Device(device)
{
  
}
  
CMPCInputThread::~CMPCInputThread()
{
  while (m_InputList.Count())
    delete m_InputList.Pop();
}

bool CMPCInputThread::AddInput(unsigned char* pData, size_t size, uint64_t pts)
{
  if (m_InputList.Count() > 10)
    return false;

  CMPCDecodeBuffer* pBuffer = AllocBuffer(size);
  fast_memcpy(pBuffer->GetPtr(), pData, size);
  pBuffer->SetPts(pts);
  m_InputList.Push(pBuffer);
  return true;
}

unsigned int CMPCInputThread::GetQueueLen()
{
  return m_InputList.Count();
}

CMPCDecodeBuffer* CMPCInputThread::AllocBuffer(size_t size)
{
  return new CMPCDecodeBuffer(size);
}

void CMPCInputThread::FreeBuffer(CMPCDecodeBuffer* pBuffer)
{
  delete pBuffer;
}

CMPCDecodeBuffer* CMPCInputThread::GetNext()
{
  return m_InputList.Pop();
}

void CMPCInputThread::Process()
{
  LOG_DEBUG("%s: Input Thread Started...", __MODULE_NAME__);
  CMPCDecodeBuffer* pInput = NULL;
  while (!m_bStop)
  {
    if (!pInput)
      pInput = GetNext();

    if (pInput)
    {
      g_InputTimer.PunchIn();
      BCM::BC_STATUS ret = BCM::DtsProcInput(m_Device, pInput->GetPtr(), pInput->GetSize(), pInput->GetPts(), FALSE);
      g_InputTimer.PunchOut();
      //LOG_DEBUG("%s: InputTimer (%d bytes) - %llu exec / %llu interval. %d in queue. (Status: %s CPU: %0.02f%%)", 
      //  __MODULE_NAME__, pInput->GetSize(), g_InputTimer.GetExecTime(), g_InputTimer.GetIntervalTime(), GetQueueLen(), g_DtsStatusText[ret],
      //  GetRelativeUsage());
      if (ret == BCM::BC_STS_SUCCESS)
      {
        delete pInput;
        pInput = NULL;
      }
      else if (ret == BCM::BC_STS_BUSY)
        Sleep(40); // Buffer is full
    }
    else
      Sleep(10);
  }

  LOG_DEBUG("%s: Input Thread Stopped...", __MODULE_NAME__);
}

////////////////////////////////////////////////////////////////////////////////////////////

CMPCOutputThread::CMPCOutputThread(BCM::HANDLE device) :
  CThread(),
  m_Device(device),
  m_OutputTimeout(20),
  m_BufferCount(0),
  m_PictureCount(0)
{
  m_OutputHeight = 1080;
  m_OutputWidth = 1920;
}

CMPCOutputThread::~CMPCOutputThread()
{
  while(m_ReadyList.Count())
    delete m_ReadyList.Pop();
  while(m_FreeList.Count())
    delete m_FreeList.Pop();
}

unsigned int CMPCOutputThread::GetReadyCount()
{
  return m_ReadyList.Count();
}

void CMPCOutputThread::AddFrame(CMPCDecodeBuffer* pBuffer)
{
  m_ReadyList.Push(pBuffer);
}

CMPCDecodeBuffer* CMPCOutputThread::GetNext()
{
  return m_ReadyList.Pop();
}

void CMPCOutputThread::FreeBuffer(CMPCDecodeBuffer* pBuffer)
{
  m_FreeList.Push(pBuffer);
}

CMPCDecodeBuffer* CMPCOutputThread::AllocBuffer()
{
  return m_FreeList.Pop();
}

void CMPCOutputThread::Process()
{
  LOG_DEBUG("%s: Output Thread Started...", __MODULE_NAME__);
  while (!m_bStop)
  {
    //if (GetReadyCount() < 2) // Need more frames in ready list
    //{
      CMPCDecodeBuffer* pBuffer = GetDecoderOutput(); // Check for output frames
      if (pBuffer)
        AddFrame(pBuffer);
      else
        Sleep(20);
    //}
    //else
    //  Sleep(20);
  }
  LOG_DEBUG("%s: Output Thread Stopped...", __MODULE_NAME__);
}

CMPCDecodeBuffer* CMPCOutputThread::GetDecoderOutput()
{
  BCM::BC_STATUS ret;
  
  // Get next output buffer from the free list
  CMPCDecodeBuffer* pBuffer = AllocBuffer();
  if (!pBuffer) // No free pre-allocated buffers so make one
  {
    pBuffer = new CMPCDecodeBuffer( sizeof(BCM::BC_DTS_PROC_OUT) ); // Allocate a new buffer
    m_BufferCount++;
    LOG_DEBUG("%s: Added a new Buffer (count=%d). Size: %d", __MODULE_NAME__, m_BufferCount, pBuffer->GetSize());    
  }

  // Set-up output struct
  BCM::BC_DTS_PROC_OUT procOut;
  memset(&procOut, 0, sizeof(BCM::BC_DTS_PROC_OUT));
  procOut.PoutFlags = BCM::BC_POUT_FLAGS_SIZE | BCM::BC_POUT_FLAGS_YV12;
  procOut.PicInfo.width = m_OutputWidth;
  procOut.PicInfo.height = m_OutputHeight;
  procOut.YbuffSz = m_OutputWidth * m_OutputHeight;
  procOut.UVbuffSz = procOut.YbuffSz / 2;

  // Fetch data from the decoder
  g_OutputTimer.PunchIn();
  
  BCM::DtsReleaseOutputBuffs(m_Device, NULL, FALSE);
  ret = BCM::DtsProcOutputNoCopy(m_Device, m_OutputTimeout, &procOut);
  g_OutputTimer.PunchOut();
  
  BCM::U64 time = g_OutputTimer.GetElapsedTime();
  LOG_DEBUG("%s: OutputTimer - %llu exec / %llu interval. %d frames ready (Count: %d Time: %llu PTS: %llu Gap: %d FPS: %0.02f Status: %02x CPU: %0.02f%%)", 
    __MODULE_NAME__, g_OutputTimer.GetExecTime(), g_OutputTimer.GetIntervalTime(), m_ReadyList.Count(), m_PictureCount, time, 
    procOut.PicInfo.timeStamp/10000, (int32_t)((int64_t)(procOut.PicInfo.timeStamp/10000) - (int64_t)time), (double)m_PictureCount/((double)time/1000.0), 
    ret, GetRelativeUsage() * 100.0);

  switch (ret)
  {
    case BCM::BC_STS_SUCCESS:
      if (procOut.PicInfo.timeStamp)
      {
        m_PictureCount++;
        pBuffer->SetPts(procOut.PicInfo.timeStamp);
        memcpy(pBuffer->GetPtr(), &procOut, sizeof(BCM::BC_DTS_PROC_OUT));
        return pBuffer;
      }
      else
      {
        break;
      }
    case BCM::BC_STS_NO_DATA:
      break;
    case BCM::BC_STS_FMT_CHANGE:
      LOG_DEBUG("%s: Format Change Detected. Flags: 0x%08x", __MODULE_NAME__, procOut.PoutFlags); 
      if ((procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID) && (procOut.PoutFlags & BCM::BC_POUT_FLAGS_FMT_CHANGE))
      {
        PrintFormat(procOut.PicInfo); 
        m_OutputTimeout = 10000;
        m_PictureCount = 0;
        g_OutputTimer.Start();
      }
      break;
    default:
      if (ret > 26)
        LOG_DEBUG("%s: DtsProcOutput returned %d.", __MODULE_NAME__, ret);
      else
        LOG_DEBUG("%s: DtsProcOutput returned %s.", __MODULE_NAME__, g_DtsStatusText[ret]);
      break;
  }  
  FreeBuffer(pBuffer); // Use it again later
  return NULL;
}

const char* g_DtsStatusText[] = {
	"BC_STS_SUCCESS",
	"BC_STS_INV_ARG",
	"BC_STS_BUSY",		
	"BC_STS_NOT_IMPL",		
	"BC_STS_PGM_QUIT",		
	"BC_STS_NO_ACCESS",	
	"BC_STS_INSUFF_RES",	
	"BC_STS_IO_ERROR",		
	"BC_STS_NO_DATA",		
	"BC_STS_VER_MISMATCH",
	"BC_STS_TIMEOUT",		
	"BC_STS_FW_CMD_ERR",	
	"BC_STS_DEC_NOT_OPEN",
	"BC_STS_ERR_USAGE",
	"BC_STS_IO_USER_ABORT",
	"BC_STS_IO_XFR_ERROR",
	"BC_STS_DEC_NOT_STARTED",
	"BC_STS_FWHEX_NOT_FOUND",
	"BC_STS_FMT_CHANGE",
	"BC_STS_HIF_ACCESS",
	"BC_STS_CMD_CANCELLED",
	"BC_STS_FW_AUTH_FAILED",
	"BC_STS_BOOTLOADER_FAILED",
	"BC_STS_CERT_VERIFY_ERROR",
	"BC_STS_DEC_EXIST_OPEN",
	"BC_STS_PENDING",
	"BC_STS_CLK_NOCHG"
};

void PrintFormat(BCM::BC_PIC_INFO_BLOCK& pib)
{
  LOG_INFO("----------------------------------\n%s","");
  LOG_INFO("\tTimeStamp: %llu\n", pib.timeStamp);
  LOG_INFO("\tPicture Number: %d\n", pib.picture_number);
  LOG_INFO("\tWidth: %d\n", pib.width);
  LOG_INFO("\tHeight: %d\n", pib.height);
  LOG_INFO("\tChroma: 0x%03x\n", pib.chroma_format);
  LOG_INFO("\tPulldown: %d\n", pib.pulldown);         
  LOG_INFO("\tFlags: 0x%08x\n", pib.flags);        
  LOG_INFO("\tFrame Rate/Res: %d\n", pib.frame_rate);       
  LOG_INFO("\tAspect Ratio: %d\n", pib.aspect_ratio);     
  LOG_INFO("\tColor Primaries: %d\n", pib.colour_primaries);
  LOG_INFO("\tMetaData: %d\n", pib.picture_meta_payload);
  LOG_INFO("\tSession Number: %d\n", pib.sess_num);
  LOG_INFO("\tTimeStamp: %d\n", pib.ycom);
  LOG_INFO("\tCustom Aspect: %d\n", pib.custom_aspect_ratio_width_height);
  LOG_INFO("\tFrames to Drop: %d\n", pib.n_drop);
  LOG_INFO("\tH264 Valid Fields: 0x%08x\n", pib.other.h264.valid);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CExecTimer g_ClientTimer;

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(0),
  m_DropPictures(false),
  m_PicturesDecoded(0),
  m_pFormatName(""),
  m_PacketsIn(0),
  m_FramesOut(0),
  m_pOutputThread(NULL),
  m_pInputThread(NULL)
{
}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  BCM::BC_STATUS res;
  BCM::U32 mode = BCM::DTS_PLAYBACK_MODE | BCM::DTS_LOAD_FILE_PLAY_FW | DTS_DFLT_RESOLUTION(BCM::vdecRESOLUTION_720p23_976);
  
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

  g_OutputTimer.Reset();
  g_OutputTimer.Start();
  g_InputTimer.Reset();
  g_InputTimer.Start();
  g_ClientTimer.Reset();
  g_ClientTimer.Start();

  m_pInputThread = new CMPCInputThread(m_Device);
  m_pInputThread->Create();
  m_pOutputThread = new CMPCOutputThread(m_Device);
  m_pOutputThread->Create();

  CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __MODULE_NAME__);
  return true;
}

void CDVDVideoCodecCrystalHD::Dispose()
{
  if (m_pInputThread)
  {
    m_pInputThread->StopThread();
    delete m_pInputThread;
    m_pInputThread = NULL;
  }

  if (m_pOutputThread)
  {
    m_pOutputThread->StopThread();
    delete m_pOutputThread;
    m_pOutputThread = NULL;
  }

  BCM::DtsStopDecoder(m_Device);
  BCM::DtsCloseDecoder(m_Device);
  BCM::DtsDeviceClose(m_Device);
  m_Device = 0;
  
  while(m_BusyList.Count())
    delete m_BusyList.Pop();
}

int CDVDVideoCodecCrystalHD::Decode(BYTE* pData, int iSize, double pts)
{
  int ret = VC_BUFFER;
  g_ClientTimer.PunchIn();

  // Handle Input
  if (pData && iSize > 100)
  {
    // Try to push data to the decoder input thread. We cannot return the data to the caller. 
    // It will be lost if we do not send it.
    int maxWait = 40;
    int waitTime = 0;
    int waitInterval = 5;
    for (waitTime = 0; waitTime < maxWait; waitTime += waitInterval)
    {
      if (m_pInputThread->AddInput(pData, iSize, (BCM::U64)(pts * 10)))
      {
        pData = NULL;
        //CLog::Log(LOGDEBUG, "%s: Added %d bytes to decoder input (Call Time: %llu, Interval %llu, Input Queue Len: %d)", __MODULE_NAME__, iSize, g_ClientTimer.GetTimeSincePunchIn(), g_ClientTimer.GetIntervalTime(), m_pInputThread->GetQueueLen());
        break;
      }
      else
        Sleep(waitInterval);
    }
  }

  // Handle Output
  for (;;)
  {
    if (!m_pInputThread->GetQueueLen())
    {
      ret = VC_BUFFER;
    }
    if (m_pOutputThread->GetReadyCount())
    {
      //CLog::Log(LOGDEBUG, "%s: Got a picture (Call Time: %llu, Interval %llu, Input Queue Len: %d)", __MODULE_NAME__, g_ClientTimer.GetTimeSincePunchIn(), g_ClientTimer.GetIntervalTime(), m_pInputThread->GetQueueLen());
      ret |= VC_PICTURE;
    }

    // If we have taken too long, return...
    if (g_ClientTimer.GetTimeSincePunchIn() > 35)
    {
      CLog::Log(LOGDEBUG, "%s: Timed-out waiting for picture (Call Time: %llu, Interval %llu, Input Queue Len: %d)", __MODULE_NAME__, g_ClientTimer.GetTimeSincePunchIn(), g_ClientTimer.GetIntervalTime(), m_pInputThread->GetQueueLen());
      //ret = VC_ERROR;
    }
    if (ret)
      break;
      
    Sleep(5);
  }

  g_ClientTimer.PunchOut();
  return ret;
}

void CDVDVideoCodecCrystalHD::Reset()
{

}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  //CLog::Log(LOGDEBUG, "%s: Fetching next decoded picture", __MODULE_NAME__);   
  CMPCDecodeBuffer* pBuffer = m_pOutputThread->GetNext();
  if (!pBuffer)
    return false;

  BCM::BC_DTS_PROC_OUT *procOut;
  
  procOut = (BCM::BC_DTS_PROC_OUT*)pBuffer->GetPtr();
 
  pDvdVideoPicture->pts = pBuffer->GetPts() / 10.0;
  pDvdVideoPicture->data[0] = procOut->Ybuff;  // Y plane
  pDvdVideoPicture->data[1] = procOut->UVbuff; // UV packed plane
  pDvdVideoPicture->iLineSize[0] = procOut->PicInfo.width;
  pDvdVideoPicture->iLineSize[1] = procOut->PicInfo.width;
  CLog::Log(LOGDEBUG, "%s: procOut->Ybuff %p", __MODULE_NAME__, procOut->Ybuff);   

  
  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = 41711.111111;
  pDvdVideoPicture->color_range = 0;
  pDvdVideoPicture->iWidth = procOut->PicInfo.width;
  pDvdVideoPicture->iHeight = procOut->PicInfo.height;
  pDvdVideoPicture->iDisplayWidth = procOut->PicInfo.width;
  pDvdVideoPicture->iDisplayHeight = procOut->PicInfo.height;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_NV12;
  //pDvdVideoPicture->format = DVDVideoPicture::FMT_YUV420P;  
  
  //CLog::Log(LOGDEBUG, "%s: Moving Buffer %d (Ready -> Busy)", __MODULE_NAME__, pBuffer->GetId());   
  m_BusyList.Push(pBuffer);

  m_FramesOut++;
  return true;
}

bool CDVDVideoCodecCrystalHD::ReleasePicture(DVDVideoPicture* pDvdVideoPicture)
{
  // Free the last provided picture reference
  if (m_BusyList.Count())
  {
    m_pOutputThread->FreeBuffer(m_BusyList.Pop());
  }
  
  return true;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
  if (m_DropPictures)
  {
    BCM::DtsDropPictures(m_Device, 1);
  }
  else
  {
    BCM::DtsDropPictures(m_Device, 0);
  }
}


/////////////////////////////////////////////////////////////////////


#endif
