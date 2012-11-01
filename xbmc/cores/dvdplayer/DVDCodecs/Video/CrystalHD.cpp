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

#include "system.h"
#if defined(_WIN32)
#include "WIN32Util.h"
#include "util.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#endif

#if defined(HAVE_LIBCRYSTALHD)
#include "CrystalHD.h"

#include "DVDClock.h"
#include "DynamicDll.h"
#include "utils/SystemInfo.h"
#include "threads/Atomics.h"
#include "threads/Thread.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"
#include "DllSwScale.h"
#include "utils/TimeUtils.h"
#include "windowing/WindowingFactory.h"

namespace BCM
{
  #if defined(WIN32)
    typedef void		*HANDLE;
  #else
    #ifndef __LINUX_USER__
      #define __LINUX_USER__
    #endif
  #endif

  #include <libcrystalhd/bc_dts_types.h>
  #include <libcrystalhd/bc_dts_defs.h>
  #include <libcrystalhd/libcrystalhd_if.h>
};

#define __MODULE_NAME__ "CrystalHD"
//#define USE_CHD_SINGLE_THREADED_API
class DllLibCrystalHDInterface
{
public:
  virtual ~DllLibCrystalHDInterface() {}
  virtual BCM::BC_STATUS DtsDeviceOpen(void *hDevice, uint32_t mode)=0;
  virtual BCM::BC_STATUS DtsDeviceClose(void *hDevice)=0;
  virtual BCM::BC_STATUS DtsOpenDecoder(void *hDevice, uint32_t StreamType)=0;
  virtual BCM::BC_STATUS DtsCloseDecoder(void *hDevice)=0;
  virtual BCM::BC_STATUS DtsStartDecoder(void *hDevice)=0;
  virtual BCM::BC_STATUS DtsSetVideoParams(void *hDevice, uint32_t videoAlg, int FGTEnable, int MetaDataEnable, int Progressive, uint32_t OptFlags)=0;
  virtual BCM::BC_STATUS DtsStartCapture(void *hDevice)=0;
  virtual BCM::BC_STATUS DtsFlushRxCapture(void *hDevice, int bDiscardOnly)=0;
  virtual BCM::BC_STATUS DtsSetFFRate(void *hDevice, uint32_t rate)=0;
  virtual BCM::BC_STATUS DtsGetDriverStatus(void *hDevice, BCM::BC_DTS_STATUS *pStatus)=0;
  virtual BCM::BC_STATUS DtsProcInput(void *hDevice, uint8_t *pUserData, uint32_t ulSizeInBytes, uint64_t timeStamp, int encrypted)=0;
  virtual BCM::BC_STATUS DtsProcOutput(void *hDevice, uint32_t milliSecWait, BCM::BC_DTS_PROC_OUT *pOut)=0;
  virtual BCM::BC_STATUS DtsProcOutputNoCopy(void *hDevice, uint32_t milliSecWait, BCM::BC_DTS_PROC_OUT *pOut)=0;
  virtual BCM::BC_STATUS DtsReleaseOutputBuffs(void *hDevice, void *Reserved, int fChange)=0;
  virtual BCM::BC_STATUS DtsSetSkipPictureMode(void *hDevice, uint32_t Mode)=0;
  virtual BCM::BC_STATUS DtsFlushInput(void *hDevice, uint32_t SkipMode)=0;

#if (HAVE_LIBCRYSTALHD == 2)
  // new function calls, only present in new driver/library so manually load them
  virtual BCM::BC_STATUS DtsGetVersion(void *hDevice, uint32_t *DrVer, uint32_t *DilVer)=0;
  virtual BCM::BC_STATUS DtsSetInputFormat(void *hDevice, BCM::BC_INPUT_FORMAT *pInputFormat)=0;
  virtual BCM::BC_STATUS DtsGetColorPrimaries(void *hDevice, uint32_t *colorPrimaries)=0;
  virtual BCM::BC_STATUS DtsSetColorSpace(void *hDevice, BCM::BC_OUTPUT_FORMAT Mode422)=0;
  virtual BCM::BC_STATUS DtsGetCapabilities(void *hDevice, BCM::BC_HW_CAPS *CapsBuffer)=0;
  virtual BCM::BC_STATUS DtsSetScaleParams(void *hDevice, BCM::BC_SCALING_PARAMS *ScaleParams)=0;
  virtual BCM::BC_STATUS DtsIsEndOfStream(void *hDevice, uint8_t* bEOS)=0;
  virtual BCM::BC_STATUS DtsCrystalHDVersion(void *hDevice, BCM::BC_INFO_CRYSTAL *CrystalInfo)=0;
#endif
};

class DllLibCrystalHD : public DllDynamic, DllLibCrystalHDInterface
{
  DECLARE_DLL_WRAPPER(DllLibCrystalHD, DLL_PATH_LIBCRYSTALHD)

  DEFINE_METHOD2(BCM::BC_STATUS, DtsDeviceOpen,      (void *p1, uint32_t p2))
  DEFINE_METHOD1(BCM::BC_STATUS, DtsDeviceClose,     (void *p1))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsOpenDecoder,     (void *p1, uint32_t p2))
  DEFINE_METHOD1(BCM::BC_STATUS, DtsCloseDecoder,    (void *p1))
  DEFINE_METHOD1(BCM::BC_STATUS, DtsStartDecoder,    (void *p1))
  DEFINE_METHOD1(BCM::BC_STATUS, DtsStopDecoder,     (void *p1))
  DEFINE_METHOD6(BCM::BC_STATUS, DtsSetVideoParams,  (void *p1, uint32_t p2, int p3, int p4, int p5, uint32_t p6))
  DEFINE_METHOD1(BCM::BC_STATUS, DtsStartCapture,    (void *p1))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsFlushRxCapture,  (void *p1, int p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsSetFFRate,       (void *p1, uint32_t p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsGetDriverStatus, (void *p1, BCM::BC_DTS_STATUS *p2))
  DEFINE_METHOD5(BCM::BC_STATUS, DtsProcInput,       (void *p1, uint8_t *p2, uint32_t p3, uint64_t p4, int p5))
  DEFINE_METHOD3(BCM::BC_STATUS, DtsProcOutput,      (void *p1, uint32_t p2, BCM::BC_DTS_PROC_OUT *p3))
  DEFINE_METHOD3(BCM::BC_STATUS, DtsProcOutputNoCopy,(void *p1, uint32_t p2, BCM::BC_DTS_PROC_OUT *p3))
  DEFINE_METHOD3(BCM::BC_STATUS, DtsReleaseOutputBuffs,(void *p1, void *p2, int p3))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsSetSkipPictureMode,(void *p1, uint32_t p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsFlushInput,      (void *p1, uint32_t p2))

#if (HAVE_LIBCRYSTALHD == 2)
  DEFINE_METHOD3(BCM::BC_STATUS, DtsGetVersion,      (void *p1, uint32_t *p2, uint32_t *p3))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsSetInputFormat,  (void *p1, BCM::BC_INPUT_FORMAT *p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsGetColorPrimaries,(void *p1, uint32_t *p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsSetColorSpace,   (void *p1, BCM::BC_OUTPUT_FORMAT p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsGetCapabilities, (void *p1, BCM::BC_HW_CAPS *p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsSetScaleParams,  (void *p1, BCM::BC_SCALING_PARAMS *p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsIsEndOfStream,   (void *p1, uint8_t *p2))
  DEFINE_METHOD2(BCM::BC_STATUS, DtsCrystalHDVersion,(void *p1, BCM::BC_INFO_CRYSTAL *p2))
#endif

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(DtsDeviceOpen,      DtsDeviceOpen)
    RESOLVE_METHOD_RENAME(DtsDeviceClose,     DtsDeviceClose)
    RESOLVE_METHOD_RENAME(DtsOpenDecoder,     DtsOpenDecoder)
    RESOLVE_METHOD_RENAME(DtsCloseDecoder,    DtsCloseDecoder)
    RESOLVE_METHOD_RENAME(DtsStartDecoder,    DtsStartDecoder)
    RESOLVE_METHOD_RENAME(DtsStopDecoder,     DtsStopDecoder)
    RESOLVE_METHOD_RENAME(DtsSetVideoParams,  DtsSetVideoParams)
    RESOLVE_METHOD_RENAME(DtsStartCapture,    DtsStartCapture)
    RESOLVE_METHOD_RENAME(DtsFlushRxCapture,  DtsFlushRxCapture)
    RESOLVE_METHOD_RENAME(DtsSetFFRate,       DtsSetFFRate)
    RESOLVE_METHOD_RENAME(DtsGetDriverStatus, DtsGetDriverStatus)
    RESOLVE_METHOD_RENAME(DtsProcInput,       DtsProcInput)
    RESOLVE_METHOD_RENAME(DtsProcOutput,      DtsProcOutput)
    RESOLVE_METHOD_RENAME(DtsProcOutputNoCopy,DtsProcOutputNoCopy)
    RESOLVE_METHOD_RENAME(DtsReleaseOutputBuffs,DtsReleaseOutputBuffs)
    RESOLVE_METHOD_RENAME(DtsSetSkipPictureMode,DtsSetSkipPictureMode)
    RESOLVE_METHOD_RENAME(DtsFlushInput,      DtsFlushInput)
  END_METHOD_RESOLVE()
  
public:
  bool LoadNewLibFunctions(void)
  {
#if (HAVE_LIBCRYSTALHD == 2)
    int rtn;
    rtn  = m_dll->ResolveExport("DtsGetVersion",       (void**)&m_DtsGetVersion_ptr, false);
    rtn &= m_dll->ResolveExport("DtsSetInputFormat",   (void**)&m_DtsSetInputFormat_ptr, false);
    rtn &= m_dll->ResolveExport("DtsGetColorPrimaries",(void**)&m_DtsGetColorPrimaries_ptr, false);
    rtn &= m_dll->ResolveExport("DtsSetColorSpace",    (void**)&m_DtsSetColorSpace_ptr, false);
    rtn &= m_dll->ResolveExport("DtsGetCapabilities",  (void**)&m_DtsGetCapabilities_ptr, false);
    rtn &= m_dll->ResolveExport("DtsSetScaleParams",   (void**)&m_DtsSetScaleParams_ptr, false);
    rtn &= m_dll->ResolveExport("DtsIsEndOfStream",    (void**)&m_DtsIsEndOfStream_ptr, false);
    rtn &= m_dll->ResolveExport("DtsCrystalHDVersion", (void**)&m_DtsCrystalHDVersion_ptr, false);
    rtn &= m_dll->ResolveExport("DtsSetInputFormat",   (void**)&m_DtsSetInputFormat_ptr, false);
    return(rtn == 1);
#else
    return false;
#endif
  };
};

void PrintFormat(BCM::BC_PIC_INFO_BLOCK &pib);
void BcmDebugLog( BCM::BC_STATUS lResult, CStdString strFuncName="");

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
class CMPCOutputThread : public CThread
{
public:
  CMPCOutputThread(void *device, DllLibCrystalHD *dll, bool has_bcm70015);
  virtual ~CMPCOutputThread();

  unsigned int        GetReadyCount(void);
  unsigned int        GetFreeCount(void);
  CPictureBuffer*     ReadyListPop(void);
  void                FreeListPush(CPictureBuffer* pBuffer);
  bool                WaitOutput(unsigned int msec);

protected:
  void                DoFrameRateTracking(double timestamp);
  void                SetFrameRate(uint32_t resolution);
  void                SetAspectRatio(BCM::BC_PIC_INFO_BLOCK *pic_info);
  void                CopyOutAsNV12(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride);
  void                CopyOutAsNV12DeInterlace(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride);
  void                CopyOutAsYV12(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride);
  void                CopyOutAsYV12DeInterlace(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride);
  void                CheckUpperLeftGreenPixelHack(CPictureBuffer *pBuffer);
  bool                GetDecoderOutput(void);
  virtual void        Process(void);

  CSyncPtrQueue<CPictureBuffer> m_FreeList;
  CSyncPtrQueue<CPictureBuffer> m_ReadyList;

  DllLibCrystalHD     *m_dll;
  void                *m_device;
  bool                m_has_bcm70015;
  unsigned int        m_timeout;
  bool                m_format_valid;
  bool                m_is_live_stream;
  int                 m_width;
  int                 m_height;
  uint64_t            m_timestamp;
  bool                m_output_YV12;
  uint64_t            m_PictureNumber;
  uint8_t             m_color_space;
  unsigned int        m_color_range;
  unsigned int        m_color_matrix;
  int                 m_interlace;
  bool                m_framerate_tracking;
  uint64_t            m_framerate_cnt;
  double              m_framerate_timestamp;
  double              m_framerate;
  int                 m_aspectratio_x;
  int                 m_aspectratio_y;
  CEvent              m_ready_event;
  DllSwScale          *m_dllSwScale;
  struct SwsContext   *m_sw_scale_ctx;
};

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(TARGET_DARWIN)
#pragma mark -
#endif
CPictureBuffer::CPictureBuffer(ERenderFormat format, int width, int height)
{
  m_width = width;
  m_height = height;
  m_field = CRYSTALHD_FIELD_FULL;
  m_interlace = false;
  m_timestamp = DVD_NOPTS_VALUE;
  m_PictureNumber = 0;
  m_color_space = BCM::MODE420;
  m_color_range = 0;
  m_color_matrix = 4;
  m_format = format;
  m_framerate = 0;
  
  switch(m_format)
  {
    default:
    case RENDER_FMT_NV12:
      // setup y plane
      m_y_buffer_size = m_width * m_height;
      m_y_buffer_ptr = (unsigned char*)_aligned_malloc(m_y_buffer_size, 16);
  
      m_u_buffer_size = 0;
      m_v_buffer_size = 0;
      m_u_buffer_ptr = NULL;
      m_v_buffer_ptr = NULL;
      m_uv_buffer_size = m_y_buffer_size / 2;
      m_uv_buffer_ptr = (unsigned char*)_aligned_malloc(m_uv_buffer_size, 16);
    break;
    case RENDER_FMT_YUYV422:
      // setup y plane
      m_y_buffer_size = (2 * m_width) * m_height;
      m_y_buffer_ptr = (unsigned char*)_aligned_malloc(m_y_buffer_size, 16);
  
      m_uv_buffer_size = 0;
      m_uv_buffer_ptr = NULL;
      m_u_buffer_size = 0;
      m_v_buffer_size = 0;
      m_u_buffer_ptr = NULL;
      m_v_buffer_ptr = NULL;
    break;
    case RENDER_FMT_YUV420P:
      // setup y plane
      m_y_buffer_size = m_width * m_height;
      m_y_buffer_ptr = (unsigned char*)_aligned_malloc(m_y_buffer_size, 16);
  
      m_uv_buffer_size = 0;
      m_uv_buffer_ptr = NULL;
      m_u_buffer_size = m_y_buffer_size / 4;
      m_v_buffer_size = m_y_buffer_size / 4;
      m_u_buffer_ptr = (unsigned char*)_aligned_malloc(m_u_buffer_size, 16);
      m_v_buffer_ptr = (unsigned char*)_aligned_malloc(m_v_buffer_size, 16);
    break;
  }
}

CPictureBuffer::~CPictureBuffer()
{
  if (m_y_buffer_ptr) _aligned_free(m_y_buffer_ptr);
  if (m_u_buffer_ptr) _aligned_free(m_u_buffer_ptr);
  if (m_v_buffer_ptr) _aligned_free(m_v_buffer_ptr);
  if (m_uv_buffer_ptr) _aligned_free(m_uv_buffer_ptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////
#if defined(TARGET_DARWIN)
#pragma mark -
#endif
CMPCOutputThread::CMPCOutputThread(void *device, DllLibCrystalHD *dll, bool has_bcm70015) :
  CThread("CMPCOutputThread"),
  m_dll(dll),
  m_device(device),
  m_has_bcm70015(has_bcm70015),
  m_timeout(20),
  m_format_valid(false),
  m_is_live_stream(false),
  m_width(0),
  m_height(0),
  m_timestamp(0),
  m_PictureNumber(0),
  m_color_space(0),
  m_color_range(0),
  m_color_matrix(0),
  m_interlace(0),
  m_framerate_tracking(false),
  m_framerate_cnt(0),
  m_framerate_timestamp(0.0),
  m_framerate(0.0),
  m_aspectratio_x(1),
  m_aspectratio_y(1)
{
  m_sw_scale_ctx = NULL;
  m_dllSwScale = new DllSwScale;
  m_dllSwScale->Load();

  
  if (g_Windowing.GetRenderQuirks() & RENDER_QUIRKS_YV12_PREFERED)
    m_output_YV12 = true;
  else
    m_output_YV12 = false;
}

CMPCOutputThread::~CMPCOutputThread()
{
  while(m_ReadyList.Count())
    delete m_ReadyList.Pop();
  while(m_FreeList.Count())
    delete m_FreeList.Pop();
    
  if (m_sw_scale_ctx)
    m_dllSwScale->sws_freeContext(m_sw_scale_ctx);
  delete m_dllSwScale;
}

unsigned int CMPCOutputThread::GetReadyCount(void)
{
  return m_ReadyList.Count();
}

unsigned int CMPCOutputThread::GetFreeCount(void)
{
  return m_FreeList.Count();
}

CPictureBuffer* CMPCOutputThread::ReadyListPop(void)
{
  CPictureBuffer *pBuffer = m_ReadyList.Pop();
  return pBuffer;
}

void CMPCOutputThread::FreeListPush(CPictureBuffer* pBuffer)
{
  m_FreeList.Push(pBuffer);
}

bool CMPCOutputThread::WaitOutput(unsigned int msec)
{
  return m_ready_event.WaitMSec(msec);
}

void CMPCOutputThread::DoFrameRateTracking(double timestamp)
{
  if (timestamp != DVD_NOPTS_VALUE)
  {
    double duration;
    // if timestamp does not start at a low value we 
    // came in the middle of an online live stream
    // 250 ms is a fourth of a 25fps source
    // if timestamp is larger than that at the beginning
    // we are much more out of sync than with the rough 
    // calculation. To cover these 250 ms we need
    // roughly 5 seconds of video stream to get back
    // in sync
    if (m_framerate_cnt == 0 && timestamp > 250000.0)
      m_is_live_stream = true;
    
    duration = timestamp - m_framerate_timestamp;
    if (duration > 0.0)
    {
      double framerate;
      // cnt count has to be done here, cause we miss frames
      // if framerate will not calculated correctly and
      // duration has to be > 0.0 so we do not calc images twice
      m_framerate_cnt++;

      m_framerate_timestamp += duration;
      framerate = DVD_TIME_BASE / duration;
      // qualify framerate, we don't care about absolute value, just
      // want to to verify range. Timestamp could be borked so ignore
      // anything that does not verify.
      // 60, 59.94 -> 60
      // 50, 49.95 -> 50
      // 30, 29.97 -> 30
      // 25, 24.975 -> 25
      // 24, 23.976 -> 24
      switch ((int)(0.5 + framerate))
      {
        case 60:
        case 50:
        case 30:
        case 25:
        case 24:
          // if we have such a live stream framerate is more exact than calculating
          // cause of m_framerate_cnt and timestamp do not match in any way
          m_framerate = m_is_live_stream ? framerate : DVD_TIME_BASE / (m_framerate_timestamp/m_framerate_cnt);
        break;
      }
    }
  }
}

void CMPCOutputThread::SetFrameRate(uint32_t resolution)
{
  m_interlace = FALSE;

  switch (resolution)
  {
    case BCM::vdecRESOLUTION_1080p30:
      m_framerate = 30.0;
    break;
    case BCM::vdecRESOLUTION_1080p29_97:
      m_framerate = 30.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_1080p25 :
      m_framerate = 25.0;
    break;
    case BCM::vdecRESOLUTION_1080p24:
      m_framerate = 24.0;
    break;
    case BCM::vdecRESOLUTION_1080p23_976:
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_1080p0:
      // 1080p0 is ambiguious, could be 23.976 or 29.97 fps, decoder
      // just does not know. 1080p@23_976 is more common but this
      // will mess up 1080p@29_97 playback. We really need to verify
      // which framerate with duration tracking.
      m_framerate_tracking = true;
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    
    case BCM::vdecRESOLUTION_1080i29_97:
      m_framerate = 30.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i0:
      m_framerate = 30.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i:
      m_framerate = 30.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_1080i25:
      m_framerate = 25.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    
    case BCM::vdecRESOLUTION_720p59_94:
      m_framerate = 60.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_720p:
      m_framerate = 60.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_720p50:
      m_framerate = 50.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_720p29_97:
      m_framerate = 30.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_720p24:
      m_framerate = 24.0;
    break;
    case BCM::vdecRESOLUTION_720p23_976:
      // some 720p/25 will be identifed as this, enable tracking.
      m_framerate_tracking = true;
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_720p0:
      // 720p0 is ambiguious, could be 23.976, 29.97 or 59.97 fps, decoder
      // just does not know. 720p@23_976 is more common but this
      // will mess up other playback. We really need to verify
      // which framerate with duration tracking.
      m_framerate_tracking = true;
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    
    case BCM::vdecRESOLUTION_576p25:
      m_framerate = 25.0;
    break;
    case BCM::vdecRESOLUTION_576p0:
      m_framerate = 25.0;
    break;
    case BCM::vdecRESOLUTION_PAL1:
      m_framerate = 25.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    
    case BCM::vdecRESOLUTION_480p0:
      m_framerate = 60.0;
    break;
    case BCM::vdecRESOLUTION_480p:
      m_framerate = 60.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_480p29_97:
      m_framerate = 30.0 * 1000.0 / 1001.0;
    break;
    case BCM::vdecRESOLUTION_480p23_976:
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
    
    case BCM::vdecRESOLUTION_480i0:
      m_framerate = 30.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_480i:
      m_framerate = 30.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    case BCM::vdecRESOLUTION_NTSC:
      m_framerate = 30.0 * 1000.0 / 1001.0;
      m_interlace = TRUE;
    break;
    
    default:
      m_framerate_tracking = true;
      m_framerate = 24.0 * 1000.0 / 1001.0;
    break;
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

void CMPCOutputThread::CopyOutAsYV12(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride)
{
  // copy y
  if (w == stride)
  {
    fast_memcpy(pBuffer->m_y_buffer_ptr, procOut->Ybuff, w * h);
  }
  else
  {
    uint8_t *s_y = procOut->Ybuff;
    uint8_t *d_y = pBuffer->m_y_buffer_ptr;
    for (int y = 0; y < h; y++, s_y += stride, d_y += w)
      fast_memcpy(d_y, s_y, w);
  }
  //copy chroma
  //copy uv packed to u,v planes (1/2 the width and 1/2 the height of y)
  uint8_t *s_uv;
  uint8_t *d_u = pBuffer->m_u_buffer_ptr;
  uint8_t *d_v = pBuffer->m_v_buffer_ptr;
  for (int y = 0; y < h/2; y++)
  {
    s_uv = procOut->UVbuff + (y * stride);
    for (int x = 0; x < w/2; x++)
    {
      *d_u++ = *s_uv++;
      *d_v++ = *s_uv++;
    }
  }
}

void CMPCOutputThread::CopyOutAsYV12DeInterlace(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride)
{
  // copy luma
  uint8_t *s_y = procOut->Ybuff;
  uint8_t *d_y = pBuffer->m_y_buffer_ptr;
  for (int y = 0; y < h/2; y++, s_y += stride)
  {
    fast_memcpy(d_y, s_y, w);
    d_y += w;
    fast_memcpy(d_y, s_y, w);
    d_y += w;
  }
  //copy chroma
  //copy uv packed to u,v planes (1/2 the width and 1/2 the height of y)
  uint8_t *s_uv;
  uint8_t *d_u = pBuffer->m_u_buffer_ptr;
  uint8_t *d_v = pBuffer->m_v_buffer_ptr;
  for (int y = 0; y < h/4; y++)
  {
    s_uv = procOut->UVbuff + (y * stride);
    for (int x = 0; x < w/2; x++)
    {
      *d_u++ = *s_uv++;
      *d_v++ = *s_uv++;
    }
    s_uv = procOut->UVbuff + (y * stride);
    for (int x = 0; x < w/2; x++)
    {
      *d_u++ = *s_uv++;
      *d_v++ = *s_uv++;
    }
  }

  pBuffer->m_interlace = false;
}

void CMPCOutputThread::CopyOutAsNV12(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride)
{
  if (w == stride)
  {
    int bytes = w * h;
    // copy y
    fast_memcpy(pBuffer->m_y_buffer_ptr, procOut->Ybuff, bytes);
    // copy uv
    fast_memcpy(pBuffer->m_uv_buffer_ptr, procOut->UVbuff, bytes/2 );
  }
  else
  {
    // copy y
    uint8_t *s = procOut->Ybuff;
    uint8_t *d = pBuffer->m_y_buffer_ptr;
    for (int y = 0; y < h; y++, s += stride, d += w)
      fast_memcpy(d, s, w);
    // copy uv
    s = procOut->UVbuff;
    d = pBuffer->m_uv_buffer_ptr;
    for (int y = 0; y < h/2; y++, s += stride, d += w)
      fast_memcpy(d, s, w);
  }
}

void CMPCOutputThread::CopyOutAsNV12DeInterlace(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride)
{
  // do simple line doubling de-interlacing.
  // copy luma
  uint8_t *s_y = procOut->Ybuff;
  uint8_t *d_y = pBuffer->m_y_buffer_ptr;
  for (int y = 0; y < h/2; y++, s_y += stride)
  {
    fast_memcpy(d_y, s_y, w);
    d_y += w;
    fast_memcpy(d_y, s_y, w);
    d_y += w;
  }
  //copy chroma
  uint8_t *s_uv = procOut->UVbuff;
  uint8_t *d_uv = pBuffer->m_uv_buffer_ptr;
  for (int y = 0; y < h/4; y++, s_uv += stride) {
    fast_memcpy(d_uv, s_uv, w);
    d_uv += w;
    fast_memcpy(d_uv, s_uv, w);
    d_uv += w;
  }
  pBuffer->m_interlace = false;
}

void CMPCOutputThread::CheckUpperLeftGreenPixelHack(CPictureBuffer *pBuffer)
{
  // crystalhd driver sends internal info in 1st pixel location, then restores
  // original pixel value but sometimes, the info is broked and the
  // driver cannot do the restore and zeros the pixel. This is wrong for
  // yuv color space, uv values should be set to 128 otherwise we get a
  // bright green pixel in upper left.
  // We fix this by replicating the 2nd pixel to the 1st.
  switch(pBuffer->m_format)
  {
    default:
    case RENDER_FMT_YUV420P:
    {
      uint8_t *d_y = pBuffer->m_y_buffer_ptr;
      uint8_t *d_u = pBuffer->m_u_buffer_ptr;
      uint8_t *d_v = pBuffer->m_v_buffer_ptr;
      d_y[0] = d_y[1];
      d_u[0] = d_u[1];
      d_v[0] = d_v[1];
    }
    break;

    case RENDER_FMT_NV12:
    {
      uint8_t  *d_y  = pBuffer->m_y_buffer_ptr;
      uint16_t *d_uv = (uint16_t*)pBuffer->m_uv_buffer_ptr;
      d_y[0] = d_y[1];
      d_uv[0] = d_uv[1];
    }
    break;

    case RENDER_FMT_YUYV422:
    {
      uint32_t *d_yuyv = (uint32_t*)pBuffer->m_y_buffer_ptr;
      d_yuyv[0] = d_yuyv[1];
    }
    break;
  }
}

bool CMPCOutputThread::GetDecoderOutput(void)
{
  BCM::BC_STATUS ret;
  BCM::BC_DTS_PROC_OUT procOut;
  CPictureBuffer *pBuffer = NULL;
  bool got_picture = false;

  // Setup output struct
  memset(&procOut, 0, sizeof(BCM::BC_DTS_PROC_OUT));

  // Fetch data from the decoder
  ret = m_dll->DtsProcOutputNoCopy(m_device, m_timeout, &procOut);

  switch (ret)
  {
    case BCM::BC_STS_SUCCESS:
      if (m_format_valid && (procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID))
      {
        if (procOut.PicInfo.timeStamp && 
          m_timestamp != procOut.PicInfo.timeStamp &&
          m_width == (int)procOut.PicInfo.width && 
          m_height == (int)procOut.PicInfo.height)
        {
          m_timestamp = procOut.PicInfo.timeStamp;
          m_PictureNumber = procOut.PicInfo.picture_number;

          if (m_framerate_tracking)
            DoFrameRateTracking((double)m_timestamp / 1000.0);

          // do not let FreeList to get greater than 10
          if (m_FreeList.Count() > 10)
            delete m_FreeList.Pop();

          // Get next output buffer from the free list
          pBuffer = m_FreeList.Pop();
          if (!pBuffer)
          {
            // No free pre-allocated buffers so make one
            if (m_output_YV12)
            {
              // output YV12, nouveau driver has slow NV12, YUY2 capability.
              pBuffer = new CPictureBuffer(RENDER_FMT_YUV420P, m_width, m_height);
            }
            else
            {
              if (m_color_space == BCM::MODE422_YUY2)
                pBuffer = new CPictureBuffer(RENDER_FMT_YUYV422, m_width, m_height);
              else
                pBuffer = new CPictureBuffer(RENDER_FMT_NV12, m_width, m_height);
            }

            CLog::Log(LOGDEBUG, "%s: Added a new Buffer, ReadyListCount: %d", __MODULE_NAME__, m_ReadyList.Count());
            while (!m_bStop && m_ReadyList.Count() > 10)
              Sleep(1);
          }

          pBuffer->m_width = m_width;
          pBuffer->m_height = m_height;
          pBuffer->m_field = CRYSTALHD_FIELD_FULL;
          pBuffer->m_interlace = m_interlace > 0 ? true : false;
          pBuffer->m_framerate = m_framerate;
          pBuffer->m_timestamp = m_timestamp;
          pBuffer->m_color_space = m_color_space;
          pBuffer->m_color_range = m_color_range;
          pBuffer->m_color_matrix = m_color_matrix;
          pBuffer->m_PictureNumber = m_PictureNumber;

          int w = m_width;
          int h = m_height;
          // frame that are not equal in width to 720, 1280 or 1920
          // need to be copied by a quantized stride (possible lib/driver bug) so force it.
          int stride = m_width;
          if (!m_has_bcm70015)
          {
            // bcm70012 uses quantized strides
            if (w <= 720)
              stride = 720;
            else if (w <= 1280)
              stride = 1280;
            else
              stride = 1920;
          }

          if (pBuffer->m_color_space == BCM::MODE420)
          {
            switch(pBuffer->m_format)
            {
              case RENDER_FMT_NV12:
                if (pBuffer->m_interlace)
                  CopyOutAsNV12DeInterlace(pBuffer, &procOut, w, h, stride);
                else
                  CopyOutAsNV12(pBuffer, &procOut, w, h, stride);
              break;
              case RENDER_FMT_YUV420P:
                if (pBuffer->m_interlace)
                  CopyOutAsYV12DeInterlace(pBuffer, &procOut, w, h, stride);
                else
                  CopyOutAsYV12(pBuffer, &procOut, w, h, stride);
              break;
              default:
              break;
            }
          }
          else
          {
            switch(pBuffer->m_format)
            {
              case RENDER_FMT_YUYV422:
                if (pBuffer->m_interlace)
                {
                  // do simple line doubling de-interlacing.
                  // copy luma
                  int yuy2_w = w * 2;
                  int yuy2_stride = stride*2;
                  uint8_t *s_y = procOut.Ybuff;
                  uint8_t *d_y = pBuffer->m_y_buffer_ptr;
                  for (int y = 0; y < h/2; y++, s_y += yuy2_stride)
                  {
                    fast_memcpy(d_y, s_y, yuy2_w);
                    d_y += yuy2_w;
                    fast_memcpy(d_y, s_y, yuy2_w);
                    d_y += yuy2_w;
                  }
                  pBuffer->m_interlace = false;
                }
                else
                {
                  fast_memcpy(pBuffer->m_y_buffer_ptr,  procOut.Ybuff, pBuffer->m_y_buffer_size);
                }
              break;
              case RENDER_FMT_YUV420P:
                // TODO: deinterlace for yuy2 -> yv12, icky
                {
                  // Perform the color space conversion.
                  uint8_t* src[] =       { procOut.Ybuff, NULL, NULL, NULL };
                  int      srcStride[] = { stride*2, 0, 0, 0 };
                  uint8_t* dst[] =       { pBuffer->m_y_buffer_ptr, pBuffer->m_u_buffer_ptr, pBuffer->m_v_buffer_ptr, NULL };
                  int      dstStride[] = { pBuffer->m_width, pBuffer->m_width/2, pBuffer->m_width/2, 0 };

                  m_sw_scale_ctx = m_dllSwScale->sws_getCachedContext(m_sw_scale_ctx,
                    pBuffer->m_width, pBuffer->m_height, PIX_FMT_YUYV422,
                    pBuffer->m_width, pBuffer->m_height, PIX_FMT_YUV420P,
                    SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);
                  m_dllSwScale->sws_scale(m_sw_scale_ctx, src, srcStride, 0, pBuffer->m_height, dst, dstStride);
                }
              break;
              default:
              break;
            }
          }

          CheckUpperLeftGreenPixelHack(pBuffer);
          m_ReadyList.Push(pBuffer);
          m_ready_event.Set();
          got_picture = true;
        }
        else
        {
          if (m_PictureNumber != procOut.PicInfo.picture_number)
            CLog::Log(LOGDEBUG, "%s: No timestamp detected: %"PRIu64, __MODULE_NAME__, procOut.PicInfo.timeStamp);
          m_PictureNumber = procOut.PicInfo.picture_number;
        }
      }

      m_dll->DtsReleaseOutputBuffs(m_device, NULL, FALSE);
    break;

    case BCM::BC_STS_FMT_CHANGE:
      CLog::Log(LOGDEBUG, "%s: Format Change Detected. Flags: 0x%08x", __MODULE_NAME__, procOut.PoutFlags);
      if ((procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID) && (procOut.PoutFlags & BCM::BC_POUT_FLAGS_FMT_CHANGE))
      {
        PrintFormat(procOut.PicInfo);

        if (procOut.PicInfo.height == 1088) {
          procOut.PicInfo.height = 1080;
        }
        m_width = procOut.PicInfo.width;
        m_height = procOut.PicInfo.height;
        m_timestamp = DVD_NOPTS_VALUE;
        m_color_space = procOut.b422Mode;
        m_color_range = 0;
        m_color_matrix = procOut.PicInfo.colour_primaries;
        SetAspectRatio(&procOut.PicInfo);
        SetFrameRate(procOut.PicInfo.frame_rate);
        if (procOut.PicInfo.flags & VDEC_FLAG_INTERLACED_SRC)
        {
          m_interlace = true;
        }
        m_timeout = 2000;
        m_format_valid = true;
        m_ready_event.Set();
      }
    break;

    case BCM::BC_STS_DEC_NOT_OPEN:
    break;

    case BCM::BC_STS_DEC_NOT_STARTED:
    break;

    case BCM::BC_STS_IO_USER_ABORT:
    break;

    case BCM::BC_STS_NO_DATA:
    break;

    case BCM::BC_STS_TIMEOUT:
    break;

    default:
      if (ret > 26)
        CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned %d.", __MODULE_NAME__, ret);
      else
        CLog::Log(LOGDEBUG, "%s: DtsProcOutput returned %s.", __MODULE_NAME__, g_DtsStatusText[ret]);
    break;
  }
  
  return got_picture;
}

void CMPCOutputThread::Process(void)
{
  BCM::BC_STATUS ret;
  BCM::BC_DTS_STATUS decoder_status;

  m_PictureNumber = 0;

  CLog::Log(LOGDEBUG, "%s: Output Thread Started...", __MODULE_NAME__);

  // wait for decoder startup, calls into DtsProcOutputXXCopy will
  // return immediately until decoder starts getting input packets. 
  while (!m_bStop)
  {
    memset(&decoder_status, 0, sizeof(decoder_status));
    ret = m_dll->DtsGetDriverStatus(m_device, &decoder_status);
    if (ret == BCM::BC_STS_SUCCESS && decoder_status.ReadyListCount)
    {
      GetDecoderOutput();
      break;
    }
    Sleep(10);
  }

  // decoder is primed so now calls in DtsProcOutputXXCopy will block
  while (!m_bStop)
  {
    memset(&decoder_status, 0, sizeof(decoder_status));
    ret = m_dll->DtsGetDriverStatus(m_device, &decoder_status);
    if (ret == BCM::BC_STS_SUCCESS && decoder_status.ReadyListCount != 0)
      GetDecoderOutput();
    else
      Sleep(1);

#ifdef USE_CHD_SINGLE_THREADED_API
    while (!m_bStop)
    {
      ret = m_dll->DtsGetDriverStatus(m_device, &decoder_status);
      if (ret == BCM::BC_STS_SUCCESS && decoder_status.ReadyListCount != 0)
      {
        double pts = (double)decoder_status.NextTimeStamp / 1000.0;
        fprintf(stdout, "cpbEmptySize(%d), NextTimeStamp(%f)\n", decoder_status.cpbEmptySize, pts);
        break;
      }
      Sleep(10);
    }
#endif
  }
  CLog::Log(LOGDEBUG, "%s: Output Thread Stopped...", __MODULE_NAME__);
}

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(TARGET_DARWIN)
#pragma mark -
#endif
CCrystalHD* CCrystalHD::m_pInstance = NULL;

CCrystalHD::CCrystalHD() :
  m_device(NULL),
  m_device_preset(false),
  m_new_lib(false),
  m_decoder_open(false),
  m_has_bcm70015(false),
  m_color_space(BCM::MODE420),
  m_drop_state(false),
  m_skip_state(false),
  m_timeout(0),
  m_wait_timeout(0),
  m_field(0),
  m_width(0),
  m_height(0),
  m_reset(0),
  m_last_pict_num(0),
  m_last_demuxer_pts(0.0),
  m_last_decoder_pts(0.0),
  m_pOutputThread(NULL),
  m_sps_pps_size(0),
  m_convert_bitstream(false)
{
#if (HAVE_LIBCRYSTALHD == 2)
  memset(&m_bc_info_crystal, 0, sizeof(m_bc_info_crystal));
#endif

  memset(&m_chd_params, 0, sizeof(m_chd_params));
  memset(&m_sps_pps_context, 0, sizeof(m_sps_pps_context));

  m_dll = new DllLibCrystalHD;
#ifdef _WIN32
  CStdString  strDll;
  if(CWIN32Util::GetCrystalHDLibraryPath(strDll) && m_dll->SetFile(strDll) && m_dll->Load() && m_dll->IsLoaded() )
#else
  if (m_dll->Load() && m_dll->IsLoaded() )
#endif
  {
#if (HAVE_LIBCRYSTALHD == 2)
    m_new_lib = m_dll->LoadNewLibFunctions();
#endif

    OpenDevice();
    
#if (HAVE_LIBCRYSTALHD == 2)
    if (m_device && m_new_lib)
    {
      m_dll->DtsCrystalHDVersion(m_device, (BCM::PBC_INFO_CRYSTAL)&m_bc_info_crystal);
      m_has_bcm70015 = (m_bc_info_crystal.device == 1);
      // bcm70012 can do nv12 (420), yuy2 (422) and uyvy (422)
      // bcm70015 can do only yuy2 (422)
      if (m_has_bcm70015)
        m_color_space = BCM::OUTPUT_MODE422_YUY2;
    }
#endif
  }

  // delete dll if device open fails, minimizes ram footprint
  if (!m_device)
  {
    delete m_dll;
    m_dll = NULL;
    CLog::Log(LOGDEBUG, "%s: broadcom crystal hd not found", __MODULE_NAME__);
  }
  else
  {
    // we know there's a device present now, close the device until doing playback
    CloseDevice();
  }
}


CCrystalHD::~CCrystalHD()
{
  if (m_decoder_open)
    CloseDecoder();

  if (m_device)
    CloseDevice();

  if (m_dll)
    delete m_dll;
}


bool CCrystalHD::DevicePresent(void)
{
  return m_device_preset;
}

void CCrystalHD::RemoveInstance(void)
{
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance = NULL;
  }
}

CCrystalHD* CCrystalHD::GetInstance(void)
{
  if (!m_pInstance)
  {
    m_pInstance = new CCrystalHD();
  }
  return m_pInstance;
}

void CCrystalHD::OpenDevice()
{
  uint32_t mode = BCM::DTS_PLAYBACK_MODE          |
                  BCM::DTS_LOAD_FILE_PLAY_FW      |
#ifdef USE_CHD_SINGLE_THREADED_API
                  BCM::DTS_SINGLE_THREADED_MODE   |
#endif
                  BCM::DTS_SKIP_TX_CHK_CPB        |
                  BCM::DTS_PLAYBACK_DROP_RPT_MODE |
                  DTS_DFLT_RESOLUTION(BCM::vdecRESOLUTION_720p23_976);

  BCM::BC_STATUS res= m_dll->DtsDeviceOpen(&m_device, mode);
  if (res != BCM::BC_STS_SUCCESS)
  {
    m_device = NULL;
    if( res == BCM::BC_STS_DEC_EXIST_OPEN )
      CLog::Log(LOGDEBUG, "%s: device owned by another application", __MODULE_NAME__);
    else
      CLog::Log(LOGDEBUG, "%s: device open failed , returning(0x%x)", __MODULE_NAME__, res);
    m_device_preset = false;
  }
  else
  {
    #if (HAVE_LIBCRYSTALHD == 2)
      if (m_new_lib)
        CLog::Log(LOGDEBUG, "%s(new API): device opened", __MODULE_NAME__);
      else
        CLog::Log(LOGDEBUG, "%s(old API): device opened", __MODULE_NAME__);
    #else
      CLog::Log(LOGDEBUG, "%s: device opened", __MODULE_NAME__);
    #endif
    m_device_preset = true;
  }
}

void CCrystalHD::CloseDevice()
{
  if (m_device)
  {
    m_dll->DtsDeviceClose(m_device);
    m_device = NULL;
    CLog::Log(LOGDEBUG, "%s: device closed", __MODULE_NAME__);
  }
}

bool CCrystalHD::OpenDecoder(CRYSTALHD_CODEC_TYPE codec_type, CDVDStreamInfo &hints)
{
  BCM::BC_STATUS res;
  uint32_t StreamType;
#if (HAVE_LIBCRYSTALHD == 2)
  BCM::BC_MEDIA_SUBTYPE Subtype;
#endif

  if (!m_device_preset)
    return false;

  if (m_decoder_open)
    CloseDecoder();
    
  OpenDevice();
  if (!m_device)
    return false;

#if (HAVE_LIBCRYSTALHD == 2) && defined(TARGET_WINDOWS)
  // Drivers prior to 3.6.9.32 don't have proper support for CRYSTALHD_CODEC_ID_AVC1
  // The internal version numbers are different for some reason...
  if (   (m_bc_info_crystal.dilVersion.dilRelease < 3)
      || (m_bc_info_crystal.dilVersion.dilRelease == 3 && m_bc_info_crystal.dilVersion.dilMajor < 22)
      || (m_bc_info_crystal.drvVersion.drvRelease < 3)
      || (m_bc_info_crystal.drvVersion.drvRelease == 3 && m_bc_info_crystal.drvVersion.drvMajor < 7) )
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, "CrystalHD", g_localizeStrings.Get(2101));
    CLog::Log(LOGWARNING, "CrystalHD drivers too old, please upgrade to 3.6.9 or later. Make sure to uninstall the old version first or the upgrade won't work.");

    if (codec_type == CRYSTALHD_CODEC_ID_AVC1)
      return false;
  }
#endif

  uint32_t videoAlg = 0;
  switch (codec_type)
  {
    case CRYSTALHD_CODEC_ID_VC1:
      videoAlg = BCM::BC_VID_ALGO_VC1;
      StreamType = BCM::BC_STREAM_TYPE_ES;
      m_convert_bitstream = false;
    break;
    case CRYSTALHD_CODEC_ID_WVC1:
      videoAlg = BCM::BC_VID_ALGO_VC1MP;
      StreamType = BCM::BC_STREAM_TYPE_ES;
      m_convert_bitstream = false;
    break;
    case CRYSTALHD_CODEC_ID_WMV3:
      if (!m_has_bcm70015)
        return false;
      videoAlg = BCM::BC_VID_ALGO_VC1MP;
      StreamType = BCM::BC_STREAM_TYPE_ES;
      m_convert_bitstream = false;
    break;
    case CRYSTALHD_CODEC_ID_H264:
      videoAlg = BCM::BC_VID_ALGO_H264;
      StreamType = BCM::BC_STREAM_TYPE_ES;
      m_convert_bitstream = false;
    break;
    case CRYSTALHD_CODEC_ID_AVC1:
      videoAlg = BCM::BC_VID_ALGO_H264;
      StreamType = BCM::BC_STREAM_TYPE_ES;
      if (!m_new_lib)
        m_convert_bitstream = bitstream_convert_init((uint8_t*)hints.extradata, hints.extrasize);
      else
        m_convert_bitstream = false;
    break;
    case CRYSTALHD_CODEC_ID_MPEG2:
      videoAlg = BCM::BC_VID_ALGO_MPEG2;
      StreamType = BCM::BC_STREAM_TYPE_ES;
      m_convert_bitstream = false;
    break;
    //BC_VID_ALGO_DIVX:
    //BC_VID_ALGO_VC1MP:
    default:
      return false;
    break;
  }
  
#if (HAVE_LIBCRYSTALHD == 2)
  uint8_t *pMetaData = NULL;
  uint32_t metaDataSz = 0;
  uint32_t startCodeSz = 4;
  m_chd_params.sps_pps_buf = NULL;
  switch (codec_type)
  {
    case CRYSTALHD_CODEC_ID_VC1:
      Subtype = BCM::BC_MSUBTYPE_VC1;
      pMetaData = (uint8_t*)hints.extradata;
      metaDataSz = hints.extrasize;
    break;
    case CRYSTALHD_CODEC_ID_WVC1:
      Subtype = BCM::BC_MSUBTYPE_WVC1;
    break;
    case CRYSTALHD_CODEC_ID_WMV3:
      Subtype = BCM::BC_MSUBTYPE_WMV3;
      pMetaData = (uint8_t*)hints.extradata;
      metaDataSz = hints.extrasize;
    break;
    case CRYSTALHD_CODEC_ID_H264:
      Subtype = BCM::BC_MSUBTYPE_H264;
      pMetaData = (uint8_t*)hints.extradata;
      metaDataSz = hints.extrasize;
    break;
    case CRYSTALHD_CODEC_ID_AVC1:
      Subtype = BCM::BC_MSUBTYPE_AVC1;
      m_chd_params.sps_pps_buf = (uint8_t*)malloc(1000);
			if (!extract_sps_pps_from_avcc(hints.extrasize, hints.extradata))
      {
        free(m_chd_params.sps_pps_buf);
        m_chd_params.sps_pps_buf = NULL;
			}
      else
      {
        pMetaData = m_chd_params.sps_pps_buf;
        metaDataSz = m_chd_params.sps_pps_size;
        startCodeSz = m_chd_params.nal_size_bytes;
      }
    break;
    case CRYSTALHD_CODEC_ID_MPEG2:
      Subtype = BCM::BC_MSUBTYPE_MPEG2VIDEO;
      pMetaData = (uint8_t*)hints.extradata;
      metaDataSz = hints.extrasize;
    break;
    //BC_VID_ALGO_DIVX:
    //BC_VID_ALGO_VC1MP:
  }
#endif

  do
  {
#if (HAVE_LIBCRYSTALHD == 2)
    if (m_new_lib)
    {
      BCM::BC_INPUT_FORMAT bcm_input_format;
      memset(&bcm_input_format, 0, sizeof(BCM::BC_INPUT_FORMAT));

      bcm_input_format.FGTEnable = FALSE;
      bcm_input_format.Progressive = TRUE;
      bcm_input_format.OptFlags = 0x80000000 | BCM::vdecFrameRate23_97;
      #ifdef USE_CHD_SINGLE_THREADED_API
        bcm_input_format.OptFlags |= 0x80;
      #endif
      
      bcm_input_format.width = hints.width;
      bcm_input_format.height = hints.height;
      bcm_input_format.mSubtype = Subtype;
      bcm_input_format.pMetaData = pMetaData;
      bcm_input_format.metaDataSz = metaDataSz;
      bcm_input_format.startCodeSz = startCodeSz;

      res = m_dll->DtsSetInputFormat(m_device, &bcm_input_format);
      if (res != BCM::BC_STS_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s: set input format failed", __MODULE_NAME__);
        break;
      }

      res = m_dll->DtsOpenDecoder(m_device, StreamType);
      if (res != BCM::BC_STS_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s: open decoder failed", __MODULE_NAME__);
        break;
      }

      if (m_has_bcm70015)
        res = m_dll->DtsSetColorSpace(m_device, BCM::OUTPUT_MODE422_YUY2); 
      else
        res = m_dll->DtsSetColorSpace(m_device, BCM::OUTPUT_MODE420); 
      if (res != BCM::BC_STS_SUCCESS)
      { 
        CLog::Log(LOGERROR, "%s: set color space failed", __MODULE_NAME__); 
        break; 
      }
    }
    else
#endif
    {
      res = m_dll->DtsOpenDecoder(m_device, StreamType);
      if (res != BCM::BC_STS_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s: open decoder failed", __MODULE_NAME__);
        break;
      }

      uint32_t OptFlags = 0x80000000 | BCM::vdecFrameRate23_97;
      res = m_dll->DtsSetVideoParams(m_device, videoAlg, FALSE, FALSE, TRUE, OptFlags);
      if (res != BCM::BC_STS_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s: set video params failed", __MODULE_NAME__);
        break;
      }
    }
    
    res = m_dll->DtsStartDecoder(m_device);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: start decoder failed", __MODULE_NAME__);
      break;
    }

    res = m_dll->DtsStartCapture(m_device);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: start capture failed", __MODULE_NAME__);
      break;
    }

    m_pOutputThread = new CMPCOutputThread(m_device, m_dll, m_has_bcm70015);
    m_pOutputThread->Create();

    m_drop_state = false;
    m_skip_state = false;
    m_decoder_open = true;
    // set output timeout to 1ms during startup,
    // this will get reset once we get a picture back.
    // the effect is to speed feeding demux packets during startup.
    m_wait_timeout = 1;
    m_reset = 0;
    m_last_pict_num = 0;
    m_last_demuxer_pts = DVD_NOPTS_VALUE;
    m_last_decoder_pts = DVD_NOPTS_VALUE;
  } while(false);

  return m_decoder_open;
}

void CCrystalHD::CloseDecoder(void)
{
  if (m_pOutputThread)
  {
    while(m_BusyList.Count())
      m_pOutputThread->FreeListPush( m_BusyList.Pop() );

    m_pOutputThread->StopThread();
    delete m_pOutputThread;
    m_pOutputThread = NULL;
  }

  if (m_convert_bitstream)
  {
    if (m_sps_pps_context.sps_pps_data)
    {
      free(m_sps_pps_context.sps_pps_data);
      m_sps_pps_context.sps_pps_data = NULL;
    }
  }
#if (HAVE_LIBCRYSTALHD == 2)
	if (m_chd_params.sps_pps_buf)
  {
		free(m_chd_params.sps_pps_buf);
		m_chd_params.sps_pps_buf = NULL;
	}
#endif

  if (m_decoder_open)
  {
    // DtsFlushRxCapture must release internal queues when
    // calling DtsStopDecoder/DtsCloseDecoder or the next
    // DtsStartCapture will fail. This is a driver/lib bug
    // with new chd driver. The existing driver ignores the
    // bDiscardOnly arg.
    if (!m_has_bcm70015)
      m_dll->DtsFlushRxCapture(m_device, false);
    m_dll->DtsStopDecoder(m_device);
    m_dll->DtsCloseDecoder(m_device);
    m_decoder_open = false;
  }
  
  CloseDevice();
}

void CCrystalHD::Reset(void)
{
  if (!m_has_bcm70015)
  {
    // Calling for non-error flush, Flushes all the decoder
    //  buffers, input, decoded and to be decoded. 
    m_reset = 10;
    m_wait_timeout = 1;
    m_dll->DtsFlushInput(m_device, 2);
  }

  while (m_BusyList.Count())
    m_pOutputThread->FreeListPush( m_BusyList.Pop() );

  while (m_pOutputThread->GetReadyCount())
  {
    ::Sleep(1);
    m_pOutputThread->FreeListPush( m_pOutputThread->ReadyListPop() );
  }
}

bool CCrystalHD::AddInput(unsigned char *pData, size_t size, double dts, double pts)
{
  if (pData)
  {
    BCM::BC_STATUS ret;
    uint64_t int_pts = pts * 1000;
    int demuxer_bytes = size;
    uint8_t *demuxer_content = pData;
    bool free_demuxer_content  = false;

    if (m_convert_bitstream)
    {
      // convert demuxer packet from bitstream (AVC1) to bytestream (AnnexB)
      int bytestream_size = 0;
      uint8_t *bytestream_buff = NULL;

      bitstream_convert(demuxer_content, demuxer_bytes, &bytestream_buff, &bytestream_size);
      if (bytestream_buff && (bytestream_size > 0))
      {
        if (bytestream_buff != demuxer_content)
          free_demuxer_content = true;
        demuxer_bytes = bytestream_size;
        demuxer_content = bytestream_buff;
      }
    }

    do
    {
      ret = m_dll->DtsProcInput(m_device, demuxer_content, demuxer_bytes, int_pts, 0);
      if (ret == BCM::BC_STS_SUCCESS)
      {
        m_last_demuxer_pts = pts;
      }
      else if (ret == BCM::BC_STS_BUSY)
      {
        CLog::Log(LOGDEBUG, "%s: DtsProcInput returned BC_STS_BUSY", __MODULE_NAME__);
        ::Sleep(1); // Buffer is full, sleep it empty
      }
    } while (ret != BCM::BC_STS_SUCCESS);

    if (free_demuxer_content)
      free(demuxer_content);

    if (!m_has_bcm70015)
    {
      if (m_reset)
      {
        m_reset--;
        if (!m_skip_state)
        {
          m_skip_state = true;
          m_dll->DtsSetSkipPictureMode(m_device, 1);
        }
      }
      else
      {
        if (m_skip_state)
        {
          m_skip_state = false;
          m_dll->DtsSetSkipPictureMode(m_device, 0);
        }
      }
    }

    if (m_pOutputThread->GetReadyCount() < 1)
      m_pOutputThread->WaitOutput(m_wait_timeout);
  }

  return true;
}

int CCrystalHD::GetReadyCount(void)
{
  if (m_pOutputThread)
    return m_pOutputThread->GetReadyCount();
  else
    return 0;
}

void CCrystalHD::BusyListFlush(void)
{
  if (m_pOutputThread)
  {
    while ( m_BusyList.Count())
      m_pOutputThread->FreeListPush( m_BusyList.Pop() );
  }
}

bool CCrystalHD::GetPicture(DVDVideoPicture *pDvdVideoPicture)
{
  CPictureBuffer* pBuffer = m_pOutputThread->ReadyListPop();

  if (!pBuffer)
    return false;

  // default both dts/pts to DVD_NOPTS_VALUE, if crystalhd drops a frame,
  // we can't tell so we can not track dts through the decoder or with
  // and external queue. pts will get set from m_timestamp.
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if (pBuffer->m_timestamp != 0)
    pDvdVideoPicture->pts = (double)pBuffer->m_timestamp / 1000.0;

  pDvdVideoPicture->iWidth = pBuffer->m_width;
  pDvdVideoPicture->iHeight = pBuffer->m_height;
  pDvdVideoPicture->iDisplayWidth = pBuffer->m_width;
  pDvdVideoPicture->iDisplayHeight = pBuffer->m_height;

  switch(pBuffer->m_format)
  {
    default:
    case RENDER_FMT_NV12:
      // Y plane
      pDvdVideoPicture->data[0] = (BYTE*)pBuffer->m_y_buffer_ptr;
      pDvdVideoPicture->iLineSize[0] = pBuffer->m_width;
      // UV packed plane
      pDvdVideoPicture->data[1] = (BYTE*)pBuffer->m_uv_buffer_ptr;
      pDvdVideoPicture->iLineSize[1] = pBuffer->m_width;
      // unused
      pDvdVideoPicture->data[2] = NULL;
      pDvdVideoPicture->iLineSize[2] = 0;
    break;
    case RENDER_FMT_YUYV422:
      // YUV packed plane
      pDvdVideoPicture->data[0] = (BYTE*)pBuffer->m_y_buffer_ptr;
      pDvdVideoPicture->iLineSize[0] = pBuffer->m_width * 2;
      // unused
      pDvdVideoPicture->data[1] = NULL;
      pDvdVideoPicture->iLineSize[1] = 0;
      // unused
      pDvdVideoPicture->data[2] = NULL;
      pDvdVideoPicture->iLineSize[2] = 0;
    break;
    case RENDER_FMT_YUV420P:
      // Y plane
      pDvdVideoPicture->data[0] = (BYTE*)pBuffer->m_y_buffer_ptr;
      pDvdVideoPicture->iLineSize[0] = pBuffer->m_width;
      // U plane
      pDvdVideoPicture->data[1] = (BYTE*)pBuffer->m_u_buffer_ptr;
      pDvdVideoPicture->iLineSize[1] = pBuffer->m_width / 2;
      // V plane
      pDvdVideoPicture->data[2] = (BYTE*)pBuffer->m_v_buffer_ptr;
      pDvdVideoPicture->iLineSize[2] = pBuffer->m_width / 2;
    break;
  }

  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = DVD_TIME_BASE / pBuffer->m_framerate;
  m_wait_timeout = pDvdVideoPicture->iDuration/2000;
  pDvdVideoPicture->color_range = pBuffer->m_color_range;
  pDvdVideoPicture->color_matrix = pBuffer->m_color_matrix;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_drop_state ? DVP_FLAG_DROPPED : 0;
  pDvdVideoPicture->format = pBuffer->m_format;

  m_last_pict_num = pBuffer->m_PictureNumber;
  m_last_decoder_pts = pDvdVideoPicture->pts;

  while( m_BusyList.Count())
    m_pOutputThread->FreeListPush( m_BusyList.Pop() );

  m_BusyList.Push(pBuffer);
  return true;
}

void CCrystalHD::SetDropState(bool bDrop)
{
  if (m_drop_state != bDrop)
  {
    m_drop_state = bDrop;
    
    if (!m_has_bcm70015)
    {
      if (!m_reset)
      {
        if (m_drop_state)
        {
          if (!m_skip_state)
          {
            m_skip_state = true;
            m_dll->DtsSetSkipPictureMode(m_device, 1);
            Sleep(1);
          }
        }
        else
        {
          if (m_skip_state)
          {
            m_skip_state = false;
            m_dll->DtsSetSkipPictureMode(m_device, 0);
            Sleep(1);
          }
        }
      }
    }
  }
  /*
  CLog::Log(LOGDEBUG, "%s: m_drop_state(%d), GetFreeCount(%d), GetReadyCount(%d)", __MODULE_NAME__, 
      m_drop_state, m_pOutputThread->GetFreeCount(), m_pOutputThread->GetReadyCount());
  */
}
////////////////////////////////////////////////////////////////////////////////////////////
bool CCrystalHD::extract_sps_pps_from_avcc(int extradata_size, void *extradata)
{
  // based on gstbcmdec.c (bcmdec_insert_sps_pps)
  // which is Copyright(c) 2008 Broadcom Corporation.
  // and Licensed LGPL 2.1

  uint8_t *data = (uint8_t*)extradata;
  uint32_t data_size = extradata_size;
  int profile;
  unsigned int nal_size;
  unsigned int num_sps, num_pps;

  m_chd_params.sps_pps_size = 0;

  profile = (data[1] << 16) | (data[2] << 8) | data[3];
  CLog::Log(LOGDEBUG, "%s: profile %06x", __MODULE_NAME__, profile);

  m_chd_params.nal_size_bytes = (data[4] & 0x03) + 1;

  CLog::Log(LOGDEBUG, "%s: nal size %d", __MODULE_NAME__, m_chd_params.nal_size_bytes);

  num_sps = data[5] & 0x1f;
  CLog::Log(LOGDEBUG, "%s: num sps %d", __MODULE_NAME__, num_sps);

  data += 6;
  data_size -= 6;

  for (unsigned int i = 0; i < num_sps; i++)
  {
    if (data_size < 2)
      return false;

    nal_size = (data[0] << 8) | data[1];
    data += 2;
    data_size -= 2;

    if (data_size < nal_size)
			return false;

    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size + 0] = 0;
    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size + 1] = 0;
    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size + 2] = 0;
    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size + 3] = 1;

    m_chd_params.sps_pps_size += 4;

    memcpy(m_chd_params.sps_pps_buf + m_chd_params.sps_pps_size, data, nal_size);
    m_chd_params.sps_pps_size += nal_size;

    data += nal_size;
    data_size -= nal_size;
  }

  if (data_size < 1)
    return false;

  num_pps = data[0];
  data += 1;
  data_size -= 1;

  for (unsigned int i = 0; i < num_pps; i++)
  {
    if (data_size < 2)
      return false;

    nal_size = (data[0] << 8) | data[1];
    data += 2;
    data_size -= 2;

    if (data_size < nal_size)
      return false;

    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size+0] = 0;
    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size+1] = 0;
    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size+2] = 0;
    m_chd_params.sps_pps_buf[m_chd_params.sps_pps_size+3] = 1;

    m_chd_params.sps_pps_size += 4;

    memcpy(m_chd_params.sps_pps_buf + m_chd_params.sps_pps_size, data, nal_size);
    m_chd_params.sps_pps_size += nal_size;

    data += nal_size;
    data_size -= nal_size;
  }

  CLog::Log(LOGDEBUG, "%s: data size at end = %d ", __MODULE_NAME__, data_size);

  return true;
}


////////////////////////////////////////////////////////////////////////////////////////////
bool CCrystalHD::bitstream_convert_init(void *in_extradata, int in_extrasize)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  m_sps_pps_size = 0;
  m_sps_pps_context.sps_pps_data = NULL;
  
  // nothing to filter
  if (!in_extradata || in_extrasize < 6)
    return false;

  uint16_t unit_size;
  uint32_t total_size = 0;
  uint8_t *out = NULL, unit_nb, sps_done = 0;
  const uint8_t *extradata = (uint8_t*)in_extradata + 4;
  static const uint8_t nalu_header[4] = {0, 0, 0, 1};

  // retrieve length coded size
  m_sps_pps_context.length_size = (*extradata++ & 0x3) + 1;
  if (m_sps_pps_context.length_size == 3)
    return false;

  // retrieve sps and pps unit(s)
  unit_nb = *extradata++ & 0x1f;  // number of sps unit(s)
  if (!unit_nb)
  {
    unit_nb = *extradata++;       // number of pps unit(s)
    sps_done++;
  }
  while (unit_nb--)
  {
    unit_size = extradata[0] << 8 | extradata[1];
    total_size += unit_size + 4;
    if ( (extradata + 2 + unit_size) > ((uint8_t*)in_extradata + in_extrasize) )
    {
      free(out);
      return false;
    }
    out = (uint8_t*)realloc(out, total_size);
    if (!out)
      return false;

    memcpy(out + total_size - unit_size - 4, nalu_header, 4);
    memcpy(out + total_size - unit_size, extradata + 2, unit_size);
    extradata += 2 + unit_size;

    if (!unit_nb && !sps_done++)
      unit_nb = *extradata++;     // number of pps unit(s)
  }

  m_sps_pps_context.sps_pps_data = out;
  m_sps_pps_context.size = total_size;
  m_sps_pps_context.first_idr = 1;

  return true;
}

bool CCrystalHD::bitstream_convert(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  uint8_t *buf = pData;
  uint32_t buf_size = iSize;
  uint8_t  unit_type;
  int32_t  nal_size;
  uint32_t cumul_size = 0;
  const uint8_t *buf_end = buf + buf_size;

  do
  {
    if (buf + m_sps_pps_context.length_size > buf_end)
      goto fail;

    if (m_sps_pps_context.length_size == 1)
      nal_size = buf[0];
    else if (m_sps_pps_context.length_size == 2)
      nal_size = buf[0] << 8 | buf[1];
    else
      nal_size = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];

    buf += m_sps_pps_context.length_size;
    unit_type = *buf & 0x1f;

    if (buf + nal_size > buf_end || nal_size < 0)
      goto fail;

    // prepend only to the first type 5 NAL unit of an IDR picture
    if (m_sps_pps_context.first_idr && unit_type == 5)
    {
      bitstream_alloc_and_copy(poutbuf, poutbuf_size,
        m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      bitstream_alloc_and_copy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size);
      if (!m_sps_pps_context.first_idr && unit_type == 1)
          m_sps_pps_context.first_idr = 1;
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;

fail:
  free(*poutbuf);
  *poutbuf = NULL;
  *poutbuf_size = 0;
  return false;
}

void CCrystalHD::bitstream_alloc_and_copy(
  uint8_t **poutbuf,      int *poutbuf_size,
  const uint8_t *sps_pps, uint32_t sps_pps_size,
  const uint8_t *in,      uint32_t in_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  #define CHD_WB32(p, d) { \
    ((uint8_t*)(p))[3] = (d); \
    ((uint8_t*)(p))[2] = (d) >> 8; \
    ((uint8_t*)(p))[1] = (d) >> 16; \
    ((uint8_t*)(p))[0] = (d) >> 24; }

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;

  *poutbuf_size += sps_pps_size + in_size + nal_header_size;
  *poutbuf = (uint8_t*)realloc(*poutbuf, *poutbuf_size);
  if (sps_pps)
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);

  memcpy(*poutbuf + sps_pps_size + nal_header_size + offset, in, in_size);
  if (!offset)
  {
    CHD_WB32(*poutbuf + sps_pps_size, 1);
  }
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
void PrintFormat(BCM::BC_PIC_INFO_BLOCK &pib)
{
  CLog::Log(LOGDEBUG, "----------------------------------\n%s","");
  CLog::Log(LOGDEBUG, "\tTimeStamp: %"PRIu64"\n", pib.timeStamp);
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
