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

#if defined(HAVE_LIBCRYSTALHD)
#include "CrystalHD.h"
#include "DVDClock.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "utils/Thread.h"
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

#if !defined(WIN32)
extern void* fast_memcpy(void * to, const void * from, size_t len);
#else
#define fast_memcpy memcpy
#endif

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
  bool                UpdateNV12Pointers(DVDVideoPicture* pDest);
  
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
  uint64_t            m_old_timestamp;
  int                 m_y_buffer_size;
  int                 m_uv_buffer_size;
};

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
#pragma mark -
#endif
CMPCDecodeBuffer::CMPCDecodeBuffer(size_t size) :
m_Size(size)
{
  m_pBuffer = (unsigned char*)_aligned_malloc(size, 16);
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
  m_PictureCount(0),
  m_old_timestamp(0)
{
  m_width = 1920;
  m_height = 1080;
  m_y_buffer_size = m_width * m_height;
  m_uv_buffer_size = m_y_buffer_size / 2;
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
  
  switch (resolution) 
  {
    case BCM::vdecRESOLUTION_480p0:
      m_framerate = 60.0;
    break;
    case BCM::vdecRESOLUTION_576p0:
      m_framerate = 25.0;
    break;
    case BCM::vdecRESOLUTION_720p0:
      m_framerate = 60.0;
    break;
    case BCM::vdecRESOLUTION_1080p0:
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_480i0:
      m_framerate = 60.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i0:
      m_framerate = 60.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080p23_976:
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_1080p29_97 :
      m_framerate = 30.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_1080p30  :
      m_framerate = 30.0;
    break;
    case BCM::vdecRESOLUTION_1080p24  :
      m_framerate = 24.0;
    break;
    case BCM::vdecRESOLUTION_1080p25 :
      m_framerate = 25.0;
    break;
    case BCM::vdecRESOLUTION_1080i29_97:
      m_framerate = 60.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i25:
      m_framerate = 50.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i:
      m_framerate = 60.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_720p59_94:
      m_framerate = 60.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_720p50:
      m_framerate = 50.0;
    break;
    case BCM::vdecRESOLUTION_720p:
      m_framerate = 60.0;
    break;
    case BCM::vdecRESOLUTION_720p23_976:
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_720p24:
      m_framerate = 25.0;
      break;
    case BCM::vdecRESOLUTION_720p29_97:
      m_framerate = 30.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_480i:
      m_framerate = 60.0 * 1000.0 / 1001.0;    
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_NTSC:
      m_framerate = 60.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_480p:
      m_framerate = 60.0;
    break;
    case BCM::vdecRESOLUTION_PAL1:
      m_framerate = 50.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_480p23_976:
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_480p29_97:
      m_framerate = 30.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_576p25:
      m_framerate = 25.0;
    break;          
    default:
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
  }
  
  if(m_interlace)
  {
    m_framerate /= 2;
  }
  CLog::Log(LOGDEBUG, "%s: resolution = %x  interlace = %d", __MODULE_NAME__, resolution, m_interlace);

}

void CMPCOutputThread::SetAspectRatio(BCM::BC_PIC_INFO_BLOCK *pic_info)
{
  switch(pic_info->aspect_ratio)
  {
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
  BCM::BC_DTS_PROC_OUT procOut;
  BCM::BC_DTS_STATUS decoder_status;
  CMPCDecodeBuffer *pBuffer = NULL;
  bool got_picture = false;
  
  do
  {
    ret = BCM::DtsGetDriverStatus(m_Device, &decoder_status);
    if (ret == BCM::BC_STS_SUCCESS)
    {
      int ready_count;
      
      ready_count = decoder_status.ReadyListCount;
      /*
      CLog::Log(LOGDEBUG, "%s: ReadyListCount %d FreeListCount %d PIBMissCount %d\n", __MODULE_NAME__,
        decoder_status.ReadyListCount, decoder_status.FreeListCount, decoder_status.PIBMissCount);
      CLog::Log(LOGDEBUG, "%s: FramesDropped %d FramesCaptured %d FramesRepeated %d\n", __MODULE_NAME__,
        decoder_status.FramesDropped, decoder_status.FramesCaptured, decoder_status.FramesRepeated);
      */
    }

    // Setup output struct
    memset(&procOut, 0, sizeof(BCM::BC_DTS_PROC_OUT));

    // Fetch data from the decoder
    ret = BCM::DtsProcOutputNoCopy(m_Device, m_OutputTimeout, &procOut);

    switch (ret)
    {
      case BCM::BC_STS_SUCCESS:
        if (procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID)
        {
          if (procOut.PicInfo.timeStamp && (procOut.PicInfo.timeStamp != m_old_timestamp))
          {
            // Get next output buffer from the free list
            pBuffer = AllocBuffer();
            if (!pBuffer)
            {
              // No free pre-allocated buffers so make one
              pBuffer = new CMPCDecodeBuffer( m_y_buffer_size + m_uv_buffer_size ); 
              CLog::Log(LOGDEBUG, "%s: Added a new Buffer (count=%d). Size: %d", __MODULE_NAME__, (int)m_BufferCount, pBuffer->GetSize());    
            }

            pBuffer->SetPts(procOut.PicInfo.timeStamp);
            m_old_timestamp = procOut.PicInfo.timeStamp;
            m_PictureCount++;

            unsigned char *y_buffer_ptr = (BYTE*)pBuffer->GetPtr();
            unsigned char *uv_buffer_ptr = (BYTE*)(y_buffer_ptr + m_y_buffer_size);
            
            switch(procOut.PicInfo.width)
            {
              case 720:
              case 1280:
              case 1920:
              {
                if (m_interlace)
                {
                  // we get a 1/2 height frame from hw, not seeing the old/even flags so
                  // can't tell which frames are odd, which are even so just replicate
                  // given lines into odd/even pairs.
                  int w = procOut.PicInfo.width;
                  int h = procOut.PicInfo.height;
                  // copy y
                  unsigned char *s = procOut.Ybuff;
                  unsigned char *d = y_buffer_ptr;
                  for (int y = 0; y < h/2; y++, s += w, d += w)
                  {
                    fast_memcpy(d, s, w);
                    d += w;
                    fast_memcpy(d, s, w);
                  }
                  // copy uv
                  s = procOut.UVbuff;
                  d = uv_buffer_ptr;
                  for (int y = 0; y < h/4; y++, s += w, d += w)
                  {
                    fast_memcpy(d, s, w);
                    d += w;
                    fast_memcpy(d, s, w);
                  }
                }
                else
                {
                  // copy y
                  fast_memcpy(y_buffer_ptr, procOut.Ybuff, m_y_buffer_size);
                  // copy uv
                  fast_memcpy(uv_buffer_ptr, procOut.UVbuff, m_uv_buffer_size);
                }
              }
              break;
              
              // frame that are not equal in width to 720, 1280 or 1920
              // need to be copied by a quantized stride.
              default:
              {
                int w = procOut.PicInfo.width;
                int h = procOut.PicInfo.height;
                
                if (w < 720)
                {
                  // copy y
                  unsigned char *s = procOut.Ybuff;
                  unsigned char *d = y_buffer_ptr;
                  for (int y = 0; y < h; y++, s += 720, d += w)
                    fast_memcpy(d, s, w);
                  // copy uv
                  s = procOut.UVbuff;
                  d = uv_buffer_ptr;
                  for (int y = 0; y < h/2; y++, s += 720, d += w)
                    fast_memcpy(d, s, w);
                }
                else if (w < 1280)
                {
                  // copy y
                  unsigned char *s = procOut.Ybuff;
                  unsigned char *d = y_buffer_ptr;
                  for (int y = 0; y < h; y++, s += 1280, d += w)
                    fast_memcpy(d, s, w);
                  // copy uv
                  s = procOut.UVbuff;
                  d = uv_buffer_ptr;
                  for (int y = 0; y < h/2; y++, s += 1280, d += w)
                    fast_memcpy(d, s, w);
                }
                else
                {
                  // copy y
                  unsigned char *s = procOut.Ybuff;
                  unsigned char *d = y_buffer_ptr;
                  for (int y = 0; y < h; y++, s += 1920, d += w)
                    fast_memcpy(d, s, w);
                  // copy uv
                  s = procOut.UVbuff;
                  d = uv_buffer_ptr;
                  for (int y = 0; y < h/2; y++, s += 1920, d += w)
                    fast_memcpy(d, s, w);
                }
              }
              break;
            }

            AddFrame(pBuffer);
            got_picture = true;
          }
          else
          {
            //CLog::Log(LOGDEBUG, "%s: Duplicate or no timestamp detected: %llu", __MODULE_NAME__, procOut.PicInfo.timeStamp); 
          }
        }

        BCM::DtsReleaseOutputBuffs(m_Device, NULL, FALSE);
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
          m_y_buffer_size = m_width * m_height;
          m_uv_buffer_size = m_y_buffer_size / 2;

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
  } while (!m_bStop && !got_picture);
  
  return pBuffer;
}

void CMPCOutputThread::Process()
{
  CLog::Log(LOGDEBUG, "%s: Output Thread Started...", __MODULE_NAME__);
  while (!m_bStop)
  {
    // Crystal HD likes this structure, video video starts to glitch if removed.
    if (GetReadyCount() < 2)
    {  
      GetDecoderOutput(); // Check for output frames
    }
    else
    {
      Sleep(10);
    }
  }
  CLog::Log(LOGDEBUG, "%s: Output Thread Stopped...", __MODULE_NAME__);
}

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
#pragma mark -
#endif
CCrystalHD* CCrystalHD::m_pInstance = NULL;

CCrystalHD::CCrystalHD() :
  m_Inited(false),
  m_IsConfigured(false),
  m_drop_state(false),
  m_pInputThread(NULL),
  m_pOutputThread(NULL)
{
  BCM::BC_STATUS res;
  //BCM::U32 mode = BCM::DTS_PLAYBACK_MODE | BCM::DTS_LOAD_FILE_PLAY_FW | BCM::DTS_PLAYBACK_DROP_RPT_MODE | BCM::DTS_SKIP_TX_CHK_CPB | DTS_DFLT_RESOLUTION(BCM::vdecRESOLUTION_720p23_976);
  BCM::U32 mode = BCM::DTS_PLAYBACK_MODE | BCM::DTS_LOAD_FILE_PLAY_FW | BCM::DTS_PLAYBACK_DROP_RPT_MODE | DTS_DFLT_RESOLUTION(BCM::vdecRESOLUTION_720p23_976);
  
  m_Inited = false;
  res = BCM::DtsDeviceOpen(&m_Device, mode);
  if (res == BCM::BC_STS_SUCCESS)
  {
    m_Inited = true;
  }
  else
  {
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
  }
}

CCrystalHD::~CCrystalHD()
{
  if (m_Device)
  {
    BCM::DtsDeviceClose(m_Device);
    m_Device = NULL;
  }
  g_CrystalHD = NULL;
}

void CCrystalHD::RemoveInstance()
{
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance = NULL;
  }
}

CCrystalHD* CCrystalHD::GetInstance()
{
  if (!m_pInstance)
  {
    m_pInstance = new CCrystalHD();
  }
  return m_pInstance;
}

bool CCrystalHD::Open(BCM_STREAM_TYPE stream_type, BCM_CODEC_TYPE codec_type)
{
  BCM::BC_STATUS res;
  
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
    
    m_pInputThread = new CMPCInputThread(m_Device);
    m_pInputThread->Create();
    m_pOutputThread = new CMPCOutputThread(m_Device);
    m_pOutputThread->Create();

    m_IsConfigured = true;
    CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __MODULE_NAME__);
  } while(false);
  
  return(m_IsConfigured);
}

void CCrystalHD::Close(void)
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

  if (m_Device)
  {
    BCM::DtsFlushRxCapture(m_Device, TRUE);
    BCM::DtsStopDecoder(m_Device);
    BCM::DtsCloseDecoder(m_Device);
  }
  m_IsConfigured = false;
}

void CCrystalHD::Flush(void)
{
  m_pInputThread->Flush();
  m_pOutputThread->Flush();

  // Flush all the decoder buffers, input, decoded and to be decoded.
  BCM::DtsFlushInput(m_Device, 2);

  CLog::Log(LOGDEBUG, "%s: Flush...", __MODULE_NAME__);
}

unsigned int CCrystalHD::GetInputCount()
{
  if (m_pInputThread)
    return m_pInputThread->GetQueueLen();
  else
    return false;  
}

bool CCrystalHD::AddInput(unsigned char *pData, size_t size, double pts)
{
  if (m_pInputThread)
    return m_pInputThread->AddInput(pData, size, (uint64_t)(pts * 10));
  else
    return false;
}


unsigned int CCrystalHD::GetReadyCount()
{
  if (m_pOutputThread)
    return m_pOutputThread->GetReadyCount();
  else
    return 0;
}

bool CCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  //CLog::Log(LOGDEBUG, "%s: Fetching next decoded picture", __MODULE_NAME__);   

  CMPCDecodeBuffer* pBuffer = m_pOutputThread->GetNext();
  if (!pBuffer)
  {
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;
    pDvdVideoPicture->format = DVDVideoPicture::FMT_NV12;
    pDvdVideoPicture->private_data = NULL;
    return false;
  }

  m_interlace = m_pOutputThread->GetInterlace();
  m_framerate = m_pOutputThread->GetFrameRate();

  pDvdVideoPicture->pts = pBuffer->GetPts() / 10.0;
  pDvdVideoPicture->iWidth = m_pOutputThread->GetWidth();
  pDvdVideoPicture->iHeight = m_pOutputThread->GetHeight();
  pDvdVideoPicture->iDisplayWidth = pDvdVideoPicture->iWidth;
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;
  
  pDvdVideoPicture->private_data = pBuffer;
  // Y plane
  pDvdVideoPicture->data[0] = (BYTE*)pBuffer->GetPtr();
  // UV packed plane
  pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] +
    (pDvdVideoPicture->iWidth * pDvdVideoPicture->iHeight);
  pDvdVideoPicture->data[2] = NULL;
  
  pDvdVideoPicture->iLineSize[0] = pDvdVideoPicture->iWidth;
  pDvdVideoPicture->iLineSize[1] = pDvdVideoPicture->iWidth;
  pDvdVideoPicture->iLineSize[2] = 0;

  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = (DVD_TIME_BASE / m_framerate);
  //if (m_interlace)
  //  pDvdVideoPicture->iDuration /= 2;
  pDvdVideoPicture->color_range = 1;
  // todo 
  //pDvdVideoPicture->color_matrix = 1;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_interlace ? DVP_FLAG_INTERLACED : 0;
  //if (m_interlace)
  //  pDvdVideoPicture->iFlags |= DVP_FLAG_TOP_FIELD_FIRST;
  pDvdVideoPicture->iFlags |= m_drop_state ? DVP_FLAG_DROPPED : 0;
  pDvdVideoPicture->format = DVDVideoPicture::FMT_NV12;

  return true;
}


bool CCrystalHD::ResetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (pDvdVideoPicture->private_data)
  {
    m_pOutputThread->FreeBuffer((CMPCDecodeBuffer*)pDvdVideoPicture->private_data);
  }
  memset(pDvdVideoPicture, 0, sizeof(DVDVideoPicture));
  
  return true;
}


void CCrystalHD::SetDropState(bool bDrop)
{
  if (m_drop_state != bDrop)
  {
    m_drop_state = bDrop;
    if (m_drop_state)
    {
      //BCM::DtsSetFFRate(m_Device, 2);
      //BCM::DtsDropPictures(m_Device, 1);
    }
    else
    {
      //BCM::DtsSetFFRate(m_Device, 1);
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
