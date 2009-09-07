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

#include "stdafx.h"
#if defined(HAVE_LIBCRYSTALHD)
#include "CrystalHD.h"
#include "DVDClock.h"
#include <utils/Thread.h>
#include "utils/Atomics.h"

namespace BCM
{
#if defined(WIN32)
  typedef void		*HANDLE;
  #include "lib/crystalhd/include/bc_dts_defs.h"
  #include "lib/crystalhd/include/windows/bc_drv_if.h"
#else
  #ifndef __LINUX_USER__
  #define __LINUX_USER__
  #endif

  #include "crystalhd/bc_dts_types.h"
  #include "crystalhd/bc_dts_defs.h"
  #include "crystalhd/bc_ldil_if.h"
#endif //defined(WIN32)
};


#if defined(WIN32)
#pragma comment(lib, "bcmDIL.lib")
#endif

#define __MODULE_NAME__ "CrystalHD"

CCrystalHD*          g_CrystalHD = NULL;

extern void* fast_memcpy(void * to, const void * from, size_t len);
void PrintFormat(BCM::BC_PIC_INFO_BLOCK &pib);

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

////////////////////////////////////////////////////////////////////////////////////////////
class CMPCInputThread : public CThread
{
public:
  CMPCInputThread(BCM::HANDLE device);
  virtual ~CMPCInputThread();
  
  bool                AddInput(unsigned char* pData, size_t size, uint64_t pts);
  void                Flush(void);
  unsigned int        GetQueueLen();
  
protected:
  CMPCDecodeBuffer*   AllocBuffer(size_t size);
  void                FreeBuffer(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer*   GetNext(void);
  void                Process(void);

  CSyncPtrQueue<CMPCDecodeBuffer> m_InputList;
  
  BCM::HANDLE         m_Device;
};

////////////////////////////////////////////////////////////////////////////////////////////
class CMPCOutputThread : public CThread
{
public:
  CMPCOutputThread(BCM::HANDLE device);
  virtual ~CMPCOutputThread();
  
  unsigned int        GetReadyCount();
  CMPCDecodeBuffer*   GetNext();
  void                FreeBuffer(CMPCDecodeBuffer* pBuffer);
  void                Flush(void);
  double              GetWidth(void);
  double              GetHeight(void);
  bool                GetInterlace(void);
  double              GetFrameRate(void);
  double              GetAspectRatio(void);
  
protected:
  void                SetFrameRate(uint32_t resolution);
  void                SetAspectRatio(BCM::BC_PIC_INFO_BLOCK *pic_info);
  CMPCDecodeBuffer*   AllocBuffer();
  void                AddFrame(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer*   GetDecoderOutput();
  virtual void        Process();
  
  CSyncPtrQueue<CMPCDecodeBuffer> m_FreeList;
  CSyncPtrQueue<CMPCDecodeBuffer> m_ReadyList;

  BCM::HANDLE         m_Device;
  unsigned int        m_OutputTimeout;
  long                m_BufferCount;
  unsigned int        m_PictureCount;
  unsigned int        m_width;
  unsigned int        m_height;
  bool                m_interlace;
  double              m_framerate;
	unsigned int        m_aspectratio_x;
	unsigned int        m_aspectratio_y;
};

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
#pragma mark -
#endif
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
#if defined(__APPLE__)
#pragma mark -
#endif
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
  if (m_InputList.Count() > 75)
    return false;

  CMPCDecodeBuffer* pBuffer = AllocBuffer(size);
  fast_memcpy(pBuffer->GetPtr(), pData, size);
  pBuffer->SetPts(pts);
  m_InputList.Push(pBuffer);
  return true;
}

void CMPCInputThread::Flush(void)
{
  while (m_InputList.Count())
    delete m_InputList.Pop();
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
  CLog::Log(LOGDEBUG, "%s: Input Thread Started...", __MODULE_NAME__);
  CMPCDecodeBuffer* pInput = NULL;
  while (!m_bStop)
  {
    if (!pInput)
      pInput = GetNext();

    if (pInput)
    {
      BCM::BC_STATUS ret = BCM::DtsProcInput(m_Device, pInput->GetPtr(), pInput->GetSize(), pInput->GetPts(), FALSE);
      if (ret == BCM::BC_STS_SUCCESS)
      {
        delete pInput;
        pInput = NULL;
      }
      else if (ret == BCM::BC_STS_BUSY)
        Sleep(40); // Buffer is full
    }
    else
    {
      Sleep(5);
    }
  }

  CLog::Log(LOGDEBUG, "%s: Input Thread Stopped...", __MODULE_NAME__);
}

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
#pragma mark -
#endif
CMPCOutputThread::CMPCOutputThread(BCM::HANDLE device) :
  CThread(),
  m_Device(device),
  m_OutputTimeout(20),
  m_BufferCount(0),
  m_PictureCount(0)
{
  m_width = 1920;
  m_height = 1080;
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

CMPCDecodeBuffer* CMPCOutputThread::GetNext()
{
  CMPCDecodeBuffer *pBuffer = m_ReadyList.Pop();
  if (pBuffer)
    AtomicDecrement(&m_BufferCount);
  //CLog::Log(LOGDEBUG, "%s: CMPCOutputThread::GetNext:m_BufferCount %d", __MODULE_NAME__, m_BufferCount);
  return pBuffer;
}

void CMPCOutputThread::FreeBuffer(CMPCDecodeBuffer* pBuffer)
{
  m_FreeList.Push(pBuffer);
}

void CMPCOutputThread::Flush(void)
{
  while(m_ReadyList.Count()) {
    AtomicDecrement(&m_BufferCount);
    delete m_ReadyList.Pop();
  }
  while(m_FreeList.Count())
    delete m_FreeList.Pop();
}

double CMPCOutputThread::GetWidth(void)
{
  return(m_width);
}

double CMPCOutputThread::GetHeight(void)
{
  return(m_height);
}

bool CMPCOutputThread::GetInterlace(void)
{
  return(m_interlace);
}

double CMPCOutputThread::GetFrameRate(void)
{
  return(m_framerate);
}

double CMPCOutputThread::GetAspectRatio(void)
{
  return(m_aspectratio_x/m_aspectratio_y);
}

void CMPCOutputThread::AddFrame(CMPCDecodeBuffer* pBuffer)
{
  m_ReadyList.Push(pBuffer);
  AtomicIncrement(&m_BufferCount);
  //CLog::Log(LOGDEBUG, "%s: CMPCOutputThread::AddFrame:m_BufferCount %d", __MODULE_NAME__, m_BufferCount);
}

void CMPCOutputThread::SetFrameRate(uint32_t resolution)
{
	m_interlace = FALSE;
  
	switch (resolution) {
    case BCM::vdecRESOLUTION_480p0:
      m_framerate = 60;
    break;
    case BCM::vdecRESOLUTION_576p0:
      m_framerate = 25;
    break;
    case BCM::vdecRESOLUTION_720p0:
      m_framerate = 60;
    break;
    case BCM::vdecRESOLUTION_1080p0:
      m_framerate = 23.976;
    break;
    case BCM::vdecRESOLUTION_480i0:
      m_framerate = 59.94;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i0:
      m_framerate = 59.94;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080p23_976:
      m_framerate = 23.976;
    break;
    case BCM::vdecRESOLUTION_1080p29_97 :
      m_framerate = 29.97;
    break;
    case BCM::vdecRESOLUTION_1080p30  :
      m_framerate = 30;
    break;
    case BCM::vdecRESOLUTION_1080p24  :
      m_framerate = 24;
    break;
    case BCM::vdecRESOLUTION_1080p25 :
      m_framerate = 25;
    break;
    case BCM::vdecRESOLUTION_1080i29_97:
      m_framerate = 59.94;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i25:
      m_framerate = 50;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i:
      m_framerate = 59.94;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_720p59_94:
      m_framerate = 59.94;
    break;
    case BCM::vdecRESOLUTION_720p50:
      m_framerate = 50;
    break;
    case BCM::vdecRESOLUTION_720p:
      m_framerate = 60;
    break;
    case BCM::vdecRESOLUTION_720p23_976:
      m_framerate = 23.976;
    break;
    case BCM::vdecRESOLUTION_720p24:
      m_framerate = 25;
      break;
    case BCM::vdecRESOLUTION_720p29_97:
      m_framerate = 29.97;
    break;
    case BCM::vdecRESOLUTION_480i:
      m_framerate = 59.94;    
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_NTSC:
      m_framerate = 60;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_480p:
      m_framerate = 60;
    break;
    case BCM::vdecRESOLUTION_PAL1:
      m_framerate = 50;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_480p23_976:
      m_framerate = 23.976; 
    break;
    case BCM::vdecRESOLUTION_480p29_97:
      m_framerate = 29.97;
    break;
    case BCM::vdecRESOLUTION_576p25:
      m_framerate = 25;
    break;          
    default:
      m_framerate = 23.976;
    break;
	}
  
	if(m_interlace)  {
		m_framerate /= 2;
	}
  CLog::Log(LOGDEBUG, "%s: resolution = %x  interlace = %d", __MODULE_NAME__, resolution, m_interlace);
  
	return;
}

void CMPCOutputThread::SetAspectRatio(BCM::BC_PIC_INFO_BLOCK *pic_info)
{
	switch(pic_info->aspect_ratio) {
	case BCM::vdecAspectRatioSquare:
		m_aspectratio_x = 1;
		m_aspectratio_y = 1;
		break;
	case BCM::vdecAspectRatio12_11:
		m_aspectratio_x = 12;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio10_11:
		m_aspectratio_x = 10;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio16_11:
		m_aspectratio_x = 16;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio40_33:
		m_aspectratio_x = 40;
		m_aspectratio_y = 33;
		break;
	case BCM::vdecAspectRatio24_11:
		m_aspectratio_x = 24;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio20_11:
		m_aspectratio_x = 20;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio32_11:
		m_aspectratio_x = 32;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio80_33:
		m_aspectratio_x = 80;
		m_aspectratio_y = 33;
		break;
	case BCM::vdecAspectRatio18_11:
		m_aspectratio_x = 18;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio15_11:
		m_aspectratio_x = 15;
		m_aspectratio_y = 11;
		break;
	case BCM::vdecAspectRatio64_33:
		m_aspectratio_x = 64;
		m_aspectratio_y = 33;
		break;
	case BCM::vdecAspectRatio160_99:
		m_aspectratio_x = 160;
		m_aspectratio_y = 99;
		break;
	case BCM::vdecAspectRatio4_3:
		m_aspectratio_x = 4;
		m_aspectratio_y = 3;
		break;
	case BCM::vdecAspectRatio16_9:
		m_aspectratio_x = 16;
		m_aspectratio_y = 9;
		break;
	case BCM::vdecAspectRatio221_1:
		m_aspectratio_x = 221;
		m_aspectratio_y = 1;
		break;
	case BCM::vdecAspectRatioUnknown:
		m_aspectratio_x = 0;
		m_aspectratio_y = 0;

		break;
	case BCM::vdecAspectRatioOther:
		m_aspectratio_x = pic_info->custom_aspect_ratio_width_height & 0x0000ffff;
		m_aspectratio_y = pic_info->custom_aspect_ratio_width_height >> 16;
		break;
	default:
		break;
	}
	if(m_aspectratio_x == 0)
  {
    m_aspectratio_x = 1;
    m_aspectratio_y = 1;
	}

  CLog::Log(LOGDEBUG, "%s: dec_par x = %d, dec_par y = %d", __MODULE_NAME__, m_aspectratio_x, m_aspectratio_y);
}

CMPCDecodeBuffer* CMPCOutputThread::AllocBuffer()
{
  return m_FreeList.Pop();
}

CMPCDecodeBuffer* CMPCOutputThread::GetDecoderOutput()
{
  BCM::BC_STATUS ret;
  
  // Get next output buffer from the free list
  CMPCDecodeBuffer* pBuffer = AllocBuffer();
  if (!pBuffer) // No free pre-allocated buffers so make one
  {
    // why are we getting buffer acqumulation?
    // maybe should throttle demuxer feed into DtsProcInput?
    // don't let them get more than 6 deep less we overrun DIL DMA buffers (8).
    while(m_ReadyList.Count() > 6) {
      AtomicDecrement(&m_BufferCount);
      delete m_ReadyList.Pop();
    }
    pBuffer = AllocBuffer();
    if (!pBuffer) // No free pre-allocated buffers so make one
    {
      pBuffer = new CMPCDecodeBuffer( sizeof(BCM::BC_DTS_PROC_OUT) ); // Allocate a new buffer
      CLog::Log(LOGDEBUG, "%s: Added a new Buffer (count=%d). Size: %d", __MODULE_NAME__, m_BufferCount, pBuffer->GetSize());    
    }
  }

  // Set-up output struct
  BCM::BC_DTS_PROC_OUT procOut;
  memset(&procOut, 0, sizeof(BCM::BC_DTS_PROC_OUT));
  procOut.PoutFlags = BCM::BC_POUT_FLAGS_SIZE | BCM::BC_POUT_FLAGS_YV12;
  procOut.PicInfo.width = m_width;
  procOut.PicInfo.height = m_height;
  procOut.YbuffSz = m_width * m_height;
  procOut.UVbuffSz = procOut.YbuffSz / 2;

  // Fetch data from the decoder
  BCM::DtsReleaseOutputBuffs(m_Device, NULL, FALSE);
  ret = BCM::DtsProcOutputNoCopy(m_Device, m_OutputTimeout, &procOut);

/*
  // Read format data from driver
  printf("New Format\n----------------------------------\n");
  printf("\tTimeStamp: %llu\n", procOut.PicInfo.timeStamp);
  printf("\tPicture Number: %d\n", procOut.PicInfo.picture_number);
  printf("\tWidth: %d\n", procOut.PicInfo.width);
  printf("\tHeight: %d\n", procOut.PicInfo.height);
  printf("\tChroma: 0x%03x\n", procOut.PicInfo.chroma_format);
  printf("\tPulldown: %d\n", procOut.PicInfo.pulldown);         
  printf("\tFlags: 0x%08x\n", procOut.PicInfo.flags);        
  printf("\tFrame Rate/Res: %d\n", procOut.PicInfo.frame_rate);       
  printf("\tAspect Ratio: %d\n", procOut.PicInfo.aspect_ratio);     
  printf("\tColor Primaries: %d\n", procOut.PicInfo.colour_primaries);
  printf("\tMetaData: %d\n", procOut.PicInfo.picture_meta_payload);
  printf("\tSession Number: %d\n", procOut.PicInfo.sess_num);
  printf("\tTimeStamp: %d\n", procOut.PicInfo.ycom);
  printf("\tCustom Aspect: %d\n", procOut.PicInfo.custom_aspect_ratio_width_height);
  printf("\tFrames to Drop: %d\n", procOut.PicInfo.n_drop);
  printf("\tH264 Valid Fields: 0x%08x\n", procOut.PicInfo.other.h264.valid);
*/
  switch (ret)
  {
    case BCM::BC_STS_SUCCESS:
      if (procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID)
      {
        pBuffer->SetPts(procOut.PicInfo.timeStamp);
        memcpy(pBuffer->GetPtr(), &procOut, sizeof(BCM::BC_DTS_PROC_OUT));
        m_PictureCount++;
        return pBuffer;
      }
    break;
      
    case BCM::BC_STS_NO_DATA:
    break;

    case BCM::BC_STS_FMT_CHANGE:
      CLog::Log(LOGDEBUG, "%s: Format Change Detected. Flags: 0x%08x", __MODULE_NAME__, procOut.PoutFlags); 
      if ((procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID) && (procOut.PoutFlags & BCM::BC_POUT_FLAGS_FMT_CHANGE))
      {
        PrintFormat(procOut.PicInfo);
        
        SetAspectRatio(&procOut.PicInfo);
        SetFrameRate(procOut.PicInfo.frame_rate);
        if (procOut.PicInfo.height == 1088) {
          procOut.PicInfo.height = 1080;
        }
        m_width = procOut.PicInfo.width;
        m_height = procOut.PicInfo.height;

        m_OutputTimeout = 10000;
        m_PictureCount = 0;
      }
    break;
    
    default:
      if (ret > 26)
        CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned %d.", __MODULE_NAME__, ret);
      else
        CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned %s.", __MODULE_NAME__, g_DtsStatusText[ret]);
    break;
  }  
  FreeBuffer(pBuffer); // Use it again later
  
  return NULL;
}

void CMPCOutputThread::Process()
{
  CLog::Log(LOGDEBUG, "%s: Output Thread Started...", __MODULE_NAME__);
  while (!m_bStop)
  {
    CMPCDecodeBuffer* pBuffer = GetDecoderOutput(); // Check for output frames
    if (pBuffer)
      AddFrame(pBuffer);
  }
  CLog::Log(LOGDEBUG, "%s: Output Thread Stopped...", __MODULE_NAME__);
}

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
#pragma mark -
#endif
CCrystalHD::CCrystalHD()
{
  m_Inited = false;
  m_IsConfigured = false;
  m_drop_state = false;
  
  InitHardware();
}

CCrystalHD::~CCrystalHD()
{
}

bool CCrystalHD::InitHardware(void)
{
  BCM::BC_STATUS res;
  BCM::U32 mode = BCM::DTS_LOAD_FILE_PLAY_FW;
  
  m_Inited = false;
  do 
  {
    res = BCM::DtsDeviceOpen(&m_Device, mode);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
      break;
    }
    res = BCM::DtsDeviceClose(m_Device);
    m_Device = NULL;
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to close Broadcom Crystal HD", __MODULE_NAME__);
      break;
    }
  } while(false);
  
  m_Inited = true;

  return(m_Inited);
}

bool CCrystalHD::Open(BCM_STREAM_TYPE stream_type, BCM_CODEC_TYPE codec_type)
{
  BCM::BC_STATUS res;
  BCM::U32 mode = BCM::DTS_PLAYBACK_MODE | DTS_DFLT_RESOLUTION(BCM::vdecRESOLUTION_720p23_976);
  
  if (m_IsConfigured)
  {
    Close();
  }

  BCM::U32 videoAlg = 0;
  switch (codec_type)
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

  do 
  {
    res = BCM::DtsDeviceOpen(&m_Device, mode);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
      break;
    }
      
    res = BCM::DtsOpenDecoder(m_Device, stream_type);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to open decoder", __MODULE_NAME__);
      break;
    }
    res = BCM::DtsSetVideoParams(m_Device, videoAlg, FALSE, FALSE, TRUE, 0x80000000 | BCM::vdecFrameRate23_97);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to set video params", __MODULE_NAME__);
      break;
    }
    res = BCM::DtsSet422Mode(m_Device, BCM::MODE420);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to set 422 mode", __MODULE_NAME__);
      break;
    }
    res = BCM::DtsStartDecoder(m_Device);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to start decoder", __MODULE_NAME__);
      break;
    }
    res = BCM::DtsStartCapture(m_Device);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: Failed to start capture", __MODULE_NAME__);
      break;
    }
    
    //m_pInputThread = new CMPCInputThread(m_Device);
    //m_pInputThread->Create();
    m_pOutputThread = new CMPCOutputThread(m_Device);
    m_pOutputThread->Create();

    m_IsConfigured = true;
    CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __MODULE_NAME__);
  } while(false);
  
  return(m_IsConfigured);
}

void CCrystalHD::Close(void)
{
  /*
  if (m_pInputThread)
  {
    m_pInputThread->StopThread();
    delete m_pInputThread;
    m_pInputThread = NULL;
  }
  */

  if (m_pOutputThread)
  {
    m_pOutputThread->StopThread();
    delete m_pOutputThread;
    m_pOutputThread = NULL;
  }

  if (m_Device)
  {
    BCM::DtsFlushRxCapture(m_Device, TRUE);
    BCM::DtsStopDecoder(m_Device);
    BCM::DtsCloseDecoder(m_Device);
    BCM::DtsDeviceClose(m_Device);
    m_Device = NULL;
  }
  m_IsConfigured = false;
  
  //while(m_BusyList.Count())
  //  delete m_BusyList.Pop();
}

void CCrystalHD::Flush(void)
{
  //m_pInputThread->Flush();
  m_pOutputThread->Flush();

  // Flush all the decoder buffers, input, decoded and to be decoded.
  BCM::DtsFlushInput(m_Device, 2);

  //while(m_BusyList.Count())
  //  delete m_BusyList.Pop();

  CLog::Log(LOGDEBUG, "%s: Flush...", __MODULE_NAME__);
}

bool CCrystalHD::AddInput(bool *got_picture_ptr, unsigned char *pData, size_t size, double pts)
{
  *got_picture_ptr = false;

  if (pData)
  {
    BCM::BC_STATUS ret = BCM::DtsProcInput(m_Device, pData, size, (uint64_t)(pts * 10), FALSE);
    if (ret == BCM::BC_STS_BUSY)
    {
      //Sleep(1); // Buffer is full
    }
  }
  // Handle Input
  /*
  if (pData)
  {
    // Try to push data to the decoder input thread. We cannot return the data to the caller. 
    // It will be lost if we do not send it.
    int maxWait = 40;
    int waitTime = 0;
    int waitInterval = 10;
    for (waitTime = 0; waitTime < maxWait; waitTime += waitInterval)
    {
      if (m_pInputThread->AddInput(pData, size, (uint64_t)(pts * 10) ))
      {
        //CLog::Log(LOGDEBUG, "%s: Added %d bytes to decoder input (Call Time: %llu, Interval %llu, Input Queue Len: %d)", __MODULE_NAME__, iSize, g_ClientTimer.GetTimeSincePunchIn(), g_ClientTimer.GetIntervalTime(), m_pInputThread->GetQueueLen());
        break;
      }
      else
      {
        CLog::Log(LOGDEBUG, "%s: m_pInputThread->AddInput full", __MODULE_NAME__);
        Sleep(waitInterval);
      }
    }
  }
  */

  // Handle Output
  if (m_pOutputThread->GetReadyCount())
  {
    *got_picture_ptr = true;
    //CLog::Log(LOGDEBUG, "%s: Got a picture (Call Time: %llu, Interval %llu, Input Queue Len: %d)", __MODULE_NAME__, g_ClientTimer.GetTimeSincePunchIn(), g_ClientTimer.GetIntervalTime(), m_pInputThread->GetQueueLen());
    //ret |= VC_PICTURE;
  }

  return(true);
}

bool CCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  BCM::BC_DTS_PROC_OUT *procOut;
  
  //CLog::Log(LOGDEBUG, "%s: Fetching next decoded picture", __MODULE_NAME__);   

  CMPCDecodeBuffer* pBuffer = m_pOutputThread->GetNext();
  if (!pBuffer)
    return false;
    
  procOut = (BCM::BC_DTS_PROC_OUT*)pBuffer->GetPtr();
 
  m_interlace = m_pOutputThread->GetInterlace();
  m_framerate = m_pOutputThread->GetFrameRate();

  pDvdVideoPicture->pts = pBuffer->GetPts() / 10.0;
  pDvdVideoPicture->data[0] = procOut->Ybuff;  // Y plane
  pDvdVideoPicture->data[1] = procOut->UVbuff; // UV packed plane
  pDvdVideoPicture->iLineSize[0] = procOut->PicInfo.width;
  pDvdVideoPicture->iLineSize[1] = procOut->PicInfo.width*2;
  //CLog::Log(LOGDEBUG, "%s: procOut->Ybuff %p", __MODULE_NAME__, procOut->Ybuff);   

  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = (DVD_TIME_BASE / m_framerate);
  pDvdVideoPicture->color_range = 0;
  pDvdVideoPicture->iWidth = procOut->PicInfo.width;
  pDvdVideoPicture->iHeight = procOut->PicInfo.height;
  pDvdVideoPicture->iDisplayWidth = procOut->PicInfo.width;
  pDvdVideoPicture->iDisplayHeight = procOut->PicInfo.height;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_interlace ? DVP_FLAG_INTERLACED : 0;
  //pDvdVideoPicture->iFlags |= frame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;
  pDvdVideoPicture->iFlags |= m_drop_state ? DVP_FLAG_DROPPED : 0;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_NV12;
  
  //CLog::Log(LOGDEBUG, "%s: Moving Buffer %d (Ready -> Busy)", __MODULE_NAME__, pBuffer->GetId());   
  //m_BusyList.Push(pBuffer);
  m_pOutputThread->FreeBuffer(pBuffer);
  
  return true;
}

bool CCrystalHD::ReleasePicture(DVDVideoPicture* pDvdVideoPicture)
{
  //CLog::Log(LOGDEBUG, "%s: Release decoded picture", __MODULE_NAME__);   
  // Free the last provided picture reference
  /*
  if (m_BusyList.Count())
  {
    m_pOutputThread->FreeBuffer(m_BusyList.Pop());
  }
  */
  
  return true;
}

void CCrystalHD::SetDropState(bool bDrop)
{
  if (m_drop_state != bDrop)
  {
    m_drop_state = bDrop;
    if (m_drop_state)
    {
      BCM::DtsSetFFRate(m_Device, 2);
      //BCM::DtsDropPictures(m_Device, 1);
    }
    else
    {
      BCM::DtsSetFFRate(m_Device, 1);
      //BCM::DtsDropPictures(m_Device, 0);
    }
    
    CLog::Log(LOGDEBUG, "%s: SetDropState... %d", __MODULE_NAME__, m_drop_state);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
void PrintFormat(BCM::BC_PIC_INFO_BLOCK &pib)
{
  CLog::Log(LOGDEBUG, "----------------------------------\n%s","");
  CLog::Log(LOGDEBUG, "\tTimeStamp: %llu\n", pib.timeStamp);
  CLog::Log(LOGDEBUG, "\tPicture Number: %d\n", pib.picture_number);
  CLog::Log(LOGDEBUG, "\tWidth: %d\n", pib.width);
  CLog::Log(LOGDEBUG, "\tHeight: %d\n", pib.height);
  CLog::Log(LOGDEBUG, "\tChroma: 0x%03x\n", pib.chroma_format);
  CLog::Log(LOGDEBUG, "\tPulldown: %d\n", pib.pulldown);         
  CLog::Log(LOGDEBUG, "\tFlags: 0x%08x\n", pib.flags);        
  CLog::Log(LOGDEBUG, "\tFrame Rate/Res: %d\n", pib.frame_rate);       
  CLog::Log(LOGDEBUG, "\tAspect Ratio: %d\n", pib.aspect_ratio);     
  CLog::Log(LOGDEBUG, "\tColor Primaries: %d\n", pib.colour_primaries);
  CLog::Log(LOGDEBUG, "\tMetaData: %d\n", pib.picture_meta_payload);
  CLog::Log(LOGDEBUG, "\tSession Number: %d\n", pib.sess_num);
  CLog::Log(LOGDEBUG, "\tTimeStamp: %d\n", pib.ycom);
  CLog::Log(LOGDEBUG, "\tCustom Aspect: %d\n", pib.custom_aspect_ratio_width_height);
  CLog::Log(LOGDEBUG, "\tFrames to Drop: %d\n", pib.n_drop);
  CLog::Log(LOGDEBUG, "\tH264 Valid Fields: 0x%08x\n", pib.other.h264.valid);
}


#endif
