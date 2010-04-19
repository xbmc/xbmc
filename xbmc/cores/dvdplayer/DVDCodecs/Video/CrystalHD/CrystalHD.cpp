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
  #include "WIN32Util.h"
  #include "util.h"

  // default Broadcom registy bits (setup when installing a CrystalHD card)
  #define BC_REG_PATH       "Software\\Broadcom\\MediaPC"
  #define BC_REG_PRODUCT    "CrystalHD" // 70012/70015
  #define BC_BCM_DLL        "bcmDIL.dll"
  #define BC_REG_INST_PATH  "InstallPath"
#endif

#if defined(HAVE_LIBCRYSTALHD)
#include "CrystalHD.h"
extern "C"
{
  #include "cpb.h"
  #include "h264_parser.h"
}

#include "DVDClock.h"
#include "DynamicDll.h"
#include "utils/Atomics.h"
#include "utils/Thread.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"

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

union pts_union
{
  double  pts_d;
  int64_t pts_i;
};

static int64_t pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  return u.pts_i;
}

static double pts_itod(int64_t pts)
{
  pts_union u;
  u.pts_i = pts;
  return u.pts_d;
}

////////////////////////////////////////////////////////////////////////////////////////////
class CMPCDecodeBuffer
{
public:
  CMPCDecodeBuffer(size_t size);
  CMPCDecodeBuffer(unsigned char* pBuffer, size_t size);
  virtual ~CMPCDecodeBuffer();
  size_t GetSize();
  unsigned char* GetPtr();
  void SetPts(uint64_t pts);
  uint64_t GetPts();
protected:
  size_t m_Size;
  unsigned char* m_pBuffer;
  uint64_t m_Pts;
};

////////////////////////////////////////////////////////////////////////////////////////////
#define USE_FFMPEG_ANNEXB

#ifdef USE_FFMPEG_ANNEXB
typedef struct H264BSFContext {
    uint8_t  length_size;
    uint8_t  first_idr;
    uint8_t *sps_pps_data;
    uint32_t size;
} H264BSFContext;
#endif

class CMPCInputThread : public CThread
{
public:
  CMPCInputThread(void *device, DllLibCrystalHD *dll, CRYSTALHD_CODEC_TYPE codec_type, int extradata_size, void *extradata);
  virtual ~CMPCInputThread();

  bool                AddInput(unsigned char* pData, size_t size, uint64_t pts);
  bool                WaitInput(unsigned int msec) { return m_PopEvent.WaitMSec(msec); }
  void                Flush(void);
  unsigned int        GetInputCount(void);

protected:
  CMPCDecodeBuffer*   AllocBuffer(size_t size);
  void                FreeBuffer(CMPCDecodeBuffer* pBuffer);
  CMPCDecodeBuffer*   GetNext(void);
  void                ProcessMPEG2(CMPCDecodeBuffer* pInput);
#ifdef USE_FFMPEG_ANNEXB
  bool                init_h264_mp4toannexb_filter(uint8_t *in_extradata, int in_extrasize);
  void                alloc_and_copy(uint8_t **poutbuf,     int *poutbuf_size,
                                const uint8_t *sps_pps, uint32_t sps_pps_size,
                                const uint8_t *in,      uint32_t in_size);
  bool                h264_mp4toannexb_filter(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size);
#endif
  void                ProcessH264(CMPCDecodeBuffer* pInput);
  void                ProcessVC1(CMPCDecodeBuffer* pInput);
  void                Process(void);

  CSyncPtrQueue<CMPCDecodeBuffer> m_InputList;
  CEvent              m_InputEvent;
  CEvent              m_PopEvent;

  DllLibCrystalHD     *m_dll;
  void                *m_Device;
  int                 m_SleepTime;
  CRYSTALHD_CODEC_TYPE m_codec_type;

  int                 m_start_decoding;
#ifdef USE_FFMPEG_ANNEXB
  bool                m_annexbfiltering;
  uint32_t            m_sps_pps_size;
  H264BSFContext      m_sps_pps_context;
#else
  struct h264_parser  *m_nal_parser;
	uint8_t             *m_extradata;
	int                 m_extradata_size;
#endif
};

////////////////////////////////////////////////////////////////////////////////////////////
class CMPCOutputThread : public CThread
{
public:
  CMPCOutputThread(void *device, DllLibCrystalHD *dll);
  virtual ~CMPCOutputThread();

  unsigned int        GetReadyCount(void);
  CPictureBuffer*     ReadyListPop(void);
  void                FreeListPush(CPictureBuffer* pBuffer);
  void                Flush(void);

protected:
  void                DoFrameRateTracking(double timestamp);
  void                SetFrameRate(uint32_t resolution);
  void                SetAspectRatio(BCM::BC_PIC_INFO_BLOCK *pic_info);
  void                CopyOutAsNV12(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride);
  void                CopyOutAsYV12(CPictureBuffer *pBuffer, BCM::BC_DTS_PROC_OUT *procOut, int w, int h, int stride);
  bool                GetDecoderOutput(void);
  virtual void        Process(void);

  CSyncPtrQueue<CPictureBuffer> m_FreeList;
  CSyncPtrQueue<CPictureBuffer> m_ReadyList;

  DllLibCrystalHD     *m_dll;
  void                *m_Device;
  unsigned int        m_OutputTimeout;
  int                 m_width;
  int                 m_height;
  uint64_t            m_timestamp;
  uint64_t            m_PictureNumber;
  unsigned int        m_color_range;
  unsigned int        m_color_matrix;
  int                 m_interlace;
  bool                m_framerate_tracking;
  uint64_t            m_framerate_cnt;
  double              m_framerate_timestamp;
  double              m_framerate;
  int                 m_aspectratio_x;
  int                 m_aspectratio_y;
  CPictureBuffer      *m_interlace_buf;
  CEvent              m_ReadyEvent;
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

size_t CMPCDecodeBuffer::GetSize(void)
{
  return m_Size;
}

unsigned char* CMPCDecodeBuffer::GetPtr(void)
{
  return m_pBuffer;
}

void CMPCDecodeBuffer::SetPts(uint64_t pts)
{
  m_Pts = pts;
}

uint64_t CMPCDecodeBuffer::GetPts(void)
{
  return m_Pts;
}

///////////////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
#pragma mark -
#endif
CMPCInputThread::CMPCInputThread(void *device, DllLibCrystalHD *dll, CRYSTALHD_CODEC_TYPE codec_type,
  int extradata_size, void *extradata) :

  CThread(),
  m_dll(dll),
  m_Device(device),
  m_SleepTime(1),
  m_codec_type(codec_type),
  m_start_decoding(0)
{
  m_PopEvent.Set();
#ifdef USE_FFMPEG_ANNEXB
  m_annexbfiltering = init_h264_mp4toannexb_filter((uint8_t*)extradata, extradata_size);
#else
  m_extradata = NULL;
  m_extradata_size = 0;
  m_nal_parser = init_parser();

  if (extradata_size > 0)
  {
    m_extradata_size = extradata_size;
    m_extradata = (uint8_t*)malloc(extradata_size);
    memcpy(m_extradata, extradata, extradata_size);
    if (parse_codec_private(m_nal_parser, m_extradata, m_extradata_size) != 0)
    {
      free(m_extradata);
      m_extradata = NULL;
      m_extradata_size = 0;
    }
  }
#endif
}

CMPCInputThread::~CMPCInputThread()
{
  while (m_InputList.Count())
    delete m_InputList.Pop();

#ifdef USE_FFMPEG_ANNEXB
	if (m_sps_pps_context.sps_pps_data)
		free(m_sps_pps_context.sps_pps_data);
#else
	if (m_extradata)
		free(m_extradata);

  free_parser(m_nal_parser);
#endif
}

bool CMPCInputThread::AddInput(unsigned char* pData, size_t size, uint64_t pts)
{
  if (m_InputList.Count() > 1024)
    return false;

  CMPCDecodeBuffer* pBuffer = AllocBuffer(size);
  fast_memcpy(pBuffer->GetPtr(), pData, size);
  pBuffer->SetPts(pts);
  m_InputList.Push(pBuffer);
  m_InputEvent.Set();

  return true;
}

void CMPCInputThread::Flush(void)
{
  while (m_InputList.Count())
    delete m_InputList.Pop();

  m_PopEvent.Set();
  m_start_decoding = 0;
#ifndef USE_FFMPEG_ANNEXB
  // Doing a full parser reinit here, works more reliable than resetting
  free_parser(m_nal_parser);
  m_nal_parser = init_parser();
	if (m_extradata_size > 0)
		parse_codec_private(m_nal_parser, m_extradata, m_extradata_size);
#endif
}

unsigned int CMPCInputThread::GetInputCount(void)
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

CMPCDecodeBuffer* CMPCInputThread::GetNext(void)
{
  CMPCDecodeBuffer* buf = m_InputList.Pop();
  m_PopEvent.Set();
  return buf;
}

void CMPCInputThread::ProcessMPEG2(CMPCDecodeBuffer* pInput)
{
  BCM::BC_STATUS ret;
  int demuxer_bytes = pInput->GetSize();
  int64_t demuxer_pts = pInput->GetPts();
  uint8_t *demuxer_content = pInput->GetPtr();

  do
  {
    ret = m_dll->DtsProcInput(m_Device, demuxer_content, demuxer_bytes, demuxer_pts, 0);
    if (ret == BCM::BC_STS_BUSY)
    {
      CLog::Log(LOGDEBUG, "%s: DtsProcInput returned BC_STS_BUSY", __MODULE_NAME__);
      Sleep(m_SleepTime); // Buffer is full, sleep it empty
    }
  } while (!m_bStop && ret != BCM::BC_STS_SUCCESS);
}

#ifdef USE_FFMPEG_ANNEXB
bool CMPCInputThread::init_h264_mp4toannexb_filter(uint8_t *in_extradata, int in_extrasize)
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

void CMPCInputThread::alloc_and_copy(uint8_t **poutbuf,     int *poutbuf_size,
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
  {
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);
  }
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

bool CMPCInputThread::h264_mp4toannexb_filter(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  uint8_t   *buf = pData;
  uint32_t  buf_size = iSize;
  uint8_t   unit_type;
  uint32_t  nal_size, cumul_size = 0;

  do
  {
    if (m_sps_pps_context.length_size == 1)
      nal_size = buf[0];
    else if (m_sps_pps_context.length_size == 2)
      nal_size = buf[0] << 8 | buf[1];
    else
      nal_size = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];

    buf += m_sps_pps_context.length_size;
    unit_type = *buf & 0x1f;

    // prepend only to the first type 5 NAL unit of an IDR picture
    if (m_sps_pps_context.first_idr && unit_type == 5)
    {
      alloc_and_copy(poutbuf, poutbuf_size,
        m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      alloc_and_copy(poutbuf, poutbuf_size, NULL, 0, buf, nal_size);
      if (!m_sps_pps_context.first_idr && unit_type == 1)
          m_sps_pps_context.first_idr = 1;
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;
}

void CMPCInputThread::ProcessH264(CMPCDecodeBuffer* pInput)
{
  BCM::BC_STATUS ret;
  bool annexbfiltered = false;
  int demuxer_bytes = pInput->GetSize();
  int64_t demuxer_pts = pInput->GetPts();
  uint8_t *demuxer_content = pInput->GetPtr();

  if (m_annexbfiltering)
  {
    int outbuf_size = 0;
    uint8_t *outbuf = NULL;

    h264_mp4toannexb_filter(demuxer_content, demuxer_bytes, &outbuf, &outbuf_size);
    if (outbuf)
    {
      annexbfiltered = true;
      demuxer_content = outbuf;
      demuxer_bytes = outbuf_size;
    }
  }

  do
  {
    ret = m_dll->DtsProcInput(m_Device, demuxer_content, demuxer_bytes, demuxer_pts, 0);
    if (ret == BCM::BC_STS_BUSY)
    {
      CLog::Log(LOGDEBUG, "%s: DtsProcInput returned BC_STS_BUSY", __MODULE_NAME__);
      Sleep(m_SleepTime); // Buffer is full, sleep it empty
    }
  } while (!m_bStop && ret != BCM::BC_STS_SUCCESS);

  if (annexbfiltered)
    free(demuxer_content);

}
#else
void CMPCInputThread::ProcessH264(CMPCDecodeBuffer* pInput)
{
  int bytes = 0;
  int demuxer_bytes = pInput->GetSize();
  int64_t demuxer_pts = pInput->GetPts();
  uint8_t *demuxer_content = pInput->GetPtr();

  while (bytes < demuxer_bytes)
  {
    uint8_t *decode_bytestream;
    uint32_t decode_bytestream_bytes;
    coded_picture *completed_pic = NULL;
    
    bytes += parse_frame(m_nal_parser, demuxer_content + bytes, demuxer_bytes - bytes, demuxer_pts,
      &decode_bytestream, &decode_bytestream_bytes, &completed_pic);

    if (completed_pic && completed_pic->sps_nal != NULL &&
        completed_pic->sps_nal->sps.pic_width > 0 &&
        completed_pic->sps_nal->sps.pic_height > 0)
    {
      m_start_decoding = 1;
    }

    if (decode_bytestream_bytes > 0 && m_start_decoding &&
        completed_pic->slc_nal != NULL && completed_pic->pps_nal != NULL)
    {
      BCM::BC_STATUS ret;

      do
      {
        ret = m_dll->DtsProcInput(m_Device, decode_bytestream, decode_bytestream_bytes, completed_pic->pts, 0);
        if (ret == BCM::BC_STS_BUSY)
        {
          CLog::Log(LOGDEBUG, "%s: DtsProcInput returned BC_STS_BUSY", __MODULE_NAME__);
          Sleep(m_SleepTime); // Buffer is full, sleep it empty
        }
      } while (!m_bStop && ret != BCM::BC_STS_SUCCESS);

      free(decode_bytestream);
    }

    if (completed_pic)
      free_coded_picture(completed_pic);

    if (m_nal_parser->last_nal_res == 3)
      m_dll->DtsFlushInput(m_Device, 2);
  }
}
#endif

void CMPCInputThread::ProcessVC1(CMPCDecodeBuffer* pInput)
{
  BCM::BC_STATUS ret;
  int demuxer_bytes = pInput->GetSize();
  int64_t demuxer_pts = pInput->GetPts();
  uint8_t *demuxer_content = pInput->GetPtr();

  do
  {
    ret = m_dll->DtsProcInput(m_Device, demuxer_content, demuxer_bytes, demuxer_pts, 0);
    if (ret == BCM::BC_STS_BUSY)
    {
      CLog::Log(LOGDEBUG, "%s: DtsProcInput returned BC_STS_BUSY", __MODULE_NAME__);
      Sleep(m_SleepTime); // Buffer is full, sleep it empty
    }
  } while (!m_bStop && ret != BCM::BC_STS_SUCCESS);
}

void CMPCInputThread::Process(void)
{
  CLog::Log(LOGDEBUG, "%s: Input Thread Started...", __MODULE_NAME__);
  CMPCDecodeBuffer* pInput = NULL;
  while (!m_bStop)
  {
    if (!pInput)
      pInput = GetNext();

    if (pInput)
    {
      switch (m_codec_type)
      {
        case CRYSTALHD_CODEC_ID_VC1:
        case CRYSTALHD_CODEC_ID_WMV3:
          ProcessVC1(pInput);
        break;
        default:
        case CRYSTALHD_CODEC_ID_H264:
          ProcessH264(pInput);
        break;
        case CRYSTALHD_CODEC_ID_MPEG2:
          ProcessMPEG2(pInput);
        break;
      }

      delete pInput;
      pInput = NULL;
    }
    else
    {
      m_InputEvent.WaitMSec(m_SleepTime);
    }
  }

  CLog::Log(LOGDEBUG, "%s: Input Thread Stopped...", __MODULE_NAME__);
}

////////////////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__)
#pragma mark -
#endif
CPictureBuffer::CPictureBuffer(DVDVideoPicture::EFormat format, int width, int height)
{
  m_width = width;
  m_height = height;
  m_field = CRYSTALHD_FIELD_FULL;
  m_interlace = false;
  m_timestamp = DVD_NOPTS_VALUE;
  m_PictureNumber = 0;
  m_color_range = 0;
  m_color_matrix = 4;
  m_format = format;
  
  // setup y plane
  m_y_buffer_size = m_width * m_height;
  m_y_buffer_ptr = (unsigned char*)_aligned_malloc(m_y_buffer_size, 16);
  
  switch(m_format)
  {
    default:
    case DVDVideoPicture::FMT_NV12:
      m_u_buffer_size = 0;
      m_v_buffer_size = 0;
      m_u_buffer_ptr = NULL;
      m_v_buffer_ptr = NULL;
      m_uv_buffer_size = m_y_buffer_size / 2;
      m_uv_buffer_ptr = (unsigned char*)_aligned_malloc(m_uv_buffer_size, 16);
    break;
    case DVDVideoPicture::FMT_YUV420P:
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
#if defined(__APPLE__)
#pragma mark -
#endif
CMPCOutputThread::CMPCOutputThread(void *device, DllLibCrystalHD *dll) :
  CThread(),
  m_dll(dll),
  m_Device(device),
  m_OutputTimeout(20),
  m_interlace_buf(NULL),
  m_framerate_tracking(false),
  m_framerate_cnt(0),
  m_framerate_timestamp(0.0),
  m_framerate(0.0)
{
}

CMPCOutputThread::~CMPCOutputThread()
{
  while(m_ReadyList.Count())
    delete m_ReadyList.Pop();
  while(m_FreeList.Count())
    delete m_FreeList.Pop();
    
  if (m_interlace_buf)
    delete m_interlace_buf;
}

unsigned int CMPCOutputThread::GetReadyCount(void)
{
  return m_ReadyList.Count();
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

void CMPCOutputThread::Flush(void)
{
  while(m_ReadyList.Count())
  {
    m_FreeList.Push( m_ReadyList.Pop() );
  }
}

void CMPCOutputThread::DoFrameRateTracking(double timestamp)
{
  if (timestamp != DVD_NOPTS_VALUE)
  {
    double duration;
    duration = timestamp - m_framerate_timestamp;
    if (duration > 0.0)
    {
      double framerate;

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
          m_framerate_timestamp += duration;
          m_framerate_cnt++;
          m_framerate = DVD_TIME_BASE / (m_framerate_timestamp/m_framerate_cnt);
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

bool CMPCOutputThread::GetDecoderOutput(void)
{
  BCM::BC_STATUS ret;
  BCM::BC_DTS_PROC_OUT procOut;
  CPictureBuffer *pBuffer = NULL;
  bool got_picture = false;

  // Setup output struct
  memset(&procOut, 0, sizeof(BCM::BC_DTS_PROC_OUT));

  // Fetch data from the decoder
  ret = m_dll->DtsProcOutputNoCopy(m_Device, m_OutputTimeout, &procOut);

  switch (ret)
  {
    case BCM::BC_STS_SUCCESS:
      if (procOut.PoutFlags & BCM::BC_POUT_FLAGS_PIB_VALID)
      {
        if (procOut.PicInfo.timeStamp)
        {
          m_timestamp = procOut.PicInfo.timeStamp;
          m_PictureNumber = procOut.PicInfo.picture_number;

          if (m_framerate_tracking)
            DoFrameRateTracking(pts_itod(m_timestamp));

          // Get next output buffer from the free list
          pBuffer = m_FreeList.Pop();
          if (!pBuffer)
          {
#ifdef _WIN32
            // force Windows to use YV12 until DX renderer gets fixed.
            if (TRUE)
#else
            // No free pre-allocated buffers so make one
            if (m_interlace)
#endif
              // Something wrong with NV12 rendering of interlaced so copy out as YV12.
              // Setting the picture format will determine the copy out method.
              pBuffer = new CPictureBuffer(DVDVideoPicture::FMT_YUV420P, m_width, m_height);
            else
              pBuffer = new CPictureBuffer(DVDVideoPicture::FMT_NV12, m_width, m_height);
              
            CLog::Log(LOGDEBUG, "%s: Added a new Buffer, ReadyListCount: %d", __MODULE_NAME__, m_ReadyList.Count());
          }

          pBuffer->m_width = m_width;
          pBuffer->m_height = m_height;
          pBuffer->m_field = CRYSTALHD_FIELD_FULL;
          pBuffer->m_interlace = m_interlace > 0 ? true : false;
          pBuffer->m_framerate = m_framerate;
          pBuffer->m_timestamp = m_timestamp;
          pBuffer->m_color_range = m_color_range;
          pBuffer->m_color_matrix = m_color_matrix;
          pBuffer->m_PictureNumber = m_PictureNumber;

          int w = procOut.PicInfo.width;
          int h = procOut.PicInfo.height;
          // frame that are not equal in width to 720, 1280 or 1920
          // need to be copied by a quantized stride (possible lib/driver bug) so force it.
          int stride;
          if (w <= 720)
            stride = 720;
          else if (w <= 1280)
            stride = 1280;
          else
            stride = 1920;
            
          if (pBuffer->m_format == DVDVideoPicture::FMT_NV12)
          {
            CopyOutAsNV12(pBuffer, &procOut, w, h, stride);
          }
          else
          {
            if (pBuffer->m_interlace)
            {
              // we get a 1/2 height frame (field) from hw, not seeing the odd/even flags so
              // can't tell which frames are odd, which are even so use picture number.
              int line_width = w*2;

              if (pBuffer->m_PictureNumber & 1)
                m_interlace_buf->m_field = CRYSTALHD_FIELD_ODD;
              else
                m_interlace_buf->m_field = CRYSTALHD_FIELD_EVEN;

              // copy luma
              uint8_t *s_y = procOut.Ybuff;
              uint8_t *d_y = m_interlace_buf->m_y_buffer_ptr;
              if (m_interlace_buf->m_field == CRYSTALHD_FIELD_ODD)
                d_y += line_width;
              for (int y = 0; y < h/2; y++, s_y += stride, d_y += line_width)
              {
                fast_memcpy(d_y, s_y, w);
              }

              //copy chroma
              line_width = w/2;
              uint8_t *s_uv;
              uint8_t *d_u = m_interlace_buf->m_u_buffer_ptr;
              uint8_t *d_v = m_interlace_buf->m_v_buffer_ptr;
              if (m_interlace_buf->m_field == CRYSTALHD_FIELD_ODD)
              {
                d_u += line_width;
                d_v += line_width;
              }
              for (int y = 0; y < h/4; y++, d_u += line_width, d_v += line_width) {
                s_uv = procOut.UVbuff + (y * stride);
                for (int x = 0; x < w/2; x++) {
                  *d_u++ = *s_uv++;
                  *d_v++ = *s_uv++;
                }
              }

              pBuffer->m_field = m_interlace_buf->m_field;
              // copy y
              fast_memcpy(pBuffer->m_y_buffer_ptr,  m_interlace_buf->m_y_buffer_ptr,  pBuffer->m_y_buffer_size);
              // copy u
              fast_memcpy(pBuffer->m_u_buffer_ptr, m_interlace_buf->m_u_buffer_ptr, pBuffer->m_u_buffer_size);
              // copy v
              fast_memcpy(pBuffer->m_v_buffer_ptr, m_interlace_buf->m_v_buffer_ptr, pBuffer->m_v_buffer_size);
            }
            else
            {
              CopyOutAsYV12(pBuffer, &procOut, w, h, stride);
            }
          }

          m_ReadyList.Push(pBuffer);
          got_picture = true;
        }
        else
        {
          if (m_PictureNumber != procOut.PicInfo.picture_number)
            CLog::Log(LOGDEBUG, "%s: No timestamp detected: %llu", __MODULE_NAME__, procOut.PicInfo.timeStamp);
          m_PictureNumber = procOut.PicInfo.picture_number;
        }
      }

      m_dll->DtsReleaseOutputBuffs(m_Device, NULL, FALSE);
    break;

    case BCM::BC_STS_NO_DATA:
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
        m_color_range = 0;
        m_color_matrix = procOut.PicInfo.colour_primaries;
        SetAspectRatio(&procOut.PicInfo);
        SetFrameRate(procOut.PicInfo.frame_rate);
        if (procOut.PicInfo.flags & VDEC_FLAG_INTERLACED_SRC)
        {
          m_interlace = true;
          m_interlace_buf = new CPictureBuffer(DVDVideoPicture::FMT_YUV420P, m_width, m_height);
        }
        m_OutputTimeout = 2000;
      }
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
    ret = m_dll->DtsGetDriverStatus(m_Device, &decoder_status);
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
    if (!GetDecoderOutput())
      Sleep(1);

#ifdef USE_CHD_SINGLE_THREADED_API
    while (!m_bStop)
    {
      ret = m_dll->DtsGetDriverStatus(m_Device, &decoder_status);
      if (ret == BCM::BC_STS_SUCCESS && decoder_status.ReadyListCount != 0)
      {
        double pts = pts_itod(decoder_status.NextTimeStamp);
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
#if defined(__APPLE__)
#pragma mark -
#endif
CCrystalHD* CCrystalHD::m_pInstance = NULL;

CCrystalHD::CCrystalHD() :
  m_Device(NULL),
  m_IsConfigured(false),
  m_drop_state(false),
  m_pInputThread(NULL),
  m_pOutputThread(NULL)
{

  m_dll = new DllLibCrystalHD;
  CheckCrystalHDLibraryPath();
  if (m_dll->Load() && m_dll->IsLoaded() )
  {
    uint32_t mode = BCM::DTS_PLAYBACK_MODE          |
                    BCM::DTS_LOAD_FILE_PLAY_FW      |
#ifdef USE_CHD_SINGLE_THREADED_API
                    BCM::DTS_SINGLE_THREADED_MODE   |
#endif
                    BCM::DTS_PLAYBACK_DROP_RPT_MODE |
                    DTS_DFLT_RESOLUTION(BCM::vdecRESOLUTION_720p23_976);

    BCM::BC_STATUS res= m_dll->DtsDeviceOpen(&m_Device, mode);
    if (res != BCM::BC_STS_SUCCESS)
    {
      m_Device = NULL;
      if( res == BCM::BC_STS_DEC_EXIST_OPEN )
        CLog::Log(LOGERROR, "%s: device owned by another application", __MODULE_NAME__);
      else
        CLog::Log(LOGERROR, "%s: device open failed", __MODULE_NAME__);
    }
    else
    {
      CLog::Log(LOGINFO, "%s: device opened", __MODULE_NAME__);
    }
  }

  // delete dll if device open fails, minimizes ram footprint
  if (!m_Device)
  {
    delete m_dll;
    m_dll = NULL;
    CLog::Log(LOGINFO, "%s: broadcom crystal hd not found", __MODULE_NAME__);
  }
}


CCrystalHD::~CCrystalHD()
{
  if (m_IsConfigured)
    CloseDecoder();

  if (m_Device)
  {
    m_dll->DtsDeviceClose(m_Device);
    m_Device = NULL;
  }
  CLog::Log(LOGINFO, "%s: device closed", __MODULE_NAME__);

  if (m_dll)
    delete m_dll;
}


bool CCrystalHD::DevicePresent(void)
{
  return m_Device != NULL;
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

void CCrystalHD::CheckCrystalHDLibraryPath(void)
{
  // support finding library by windows registry
#if defined _WIN32
  HKEY hKey;
  CStdString strRegKey;

  CLog::Log(LOGDEBUG, "%s: detecting CrystalHD installation path", __MODULE_NAME__);
  strRegKey.Format("%s\\%s", BC_REG_PATH, BC_REG_PRODUCT );

  if( CWIN32Util::UtilRegOpenKeyEx( HKEY_LOCAL_MACHINE, strRegKey.c_str(), KEY_READ, &hKey ))
  {
    DWORD dwType;
    char *pcPath= NULL;
    if( CWIN32Util::UtilRegGetValue( hKey, BC_REG_INST_PATH, &dwType, &pcPath, NULL, sizeof( pcPath ) ) == ERROR_SUCCESS )
    {
      CStdString strDll = CUtil::AddFileToFolder(pcPath, BC_BCM_DLL);
      CLog::Log(LOGDEBUG, "%s: got CrystalHD installation path (%s)", __MODULE_NAME__, strDll.c_str());
      m_dll->SetFile(strDll);
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s: getting CrystalHD installation path faild", __MODULE_NAME__);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s: CrystalHD software seems to be not installed.", __MODULE_NAME__);
  }
#endif
}

bool CCrystalHD::OpenDecoder(CRYSTALHD_CODEC_TYPE codec_type, int extradata_size, void *extradata)
{
  BCM::BC_STATUS res;
  uint32_t StreamType;

  if (!m_Device)
    return false;

  if (m_IsConfigured)
    CloseDecoder();

  uint32_t videoAlg = 0;
  switch (codec_type)
  {
    case CRYSTALHD_CODEC_ID_VC1:
      videoAlg = BCM::BC_VID_ALGO_VC1;
      StreamType = BCM::BC_STREAM_TYPE_ES;
    break;
    case CRYSTALHD_CODEC_ID_WMV3:
      videoAlg = BCM::BC_VID_ALGO_VC1MP;
      StreamType = BCM::BC_STREAM_TYPE_PES;
    break;
    case CRYSTALHD_CODEC_ID_H264:
      videoAlg = BCM::BC_VID_ALGO_H264;
      StreamType = BCM::BC_STREAM_TYPE_ES;
    break;
    case CRYSTALHD_CODEC_ID_MPEG2:
      videoAlg = BCM::BC_VID_ALGO_MPEG2;
      StreamType = BCM::BC_STREAM_TYPE_ES;
    break;
    default:
      return false;
    break;
  }

  do
  {
    res = m_dll->DtsOpenDecoder(m_Device, StreamType);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: open decoder failed", __MODULE_NAME__);
      break;
    }
#ifdef USE_CHD_SINGLE_THREADED_API
    res = m_dll->DtsSetVideoParams(m_Device, videoAlg, FALSE, FALSE, TRUE, 0x80 | 0x80000000 | BCM::vdecFrameRate23_97);
#else
    res = m_dll->DtsSetVideoParams(m_Device, videoAlg, FALSE, FALSE, TRUE, 0x80000000 | BCM::vdecFrameRate23_97);
#endif
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGDEBUG, "%s: set video params failed", __MODULE_NAME__);
      break;
    }
    res = m_dll->DtsStartDecoder(m_Device);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGDEBUG, "%s: start decoder failed", __MODULE_NAME__);
      break;
    }
    res = m_dll->DtsStartCapture(m_Device);
    if (res != BCM::BC_STS_SUCCESS)
    {
      CLog::Log(LOGDEBUG, "%s: start capture failed", __MODULE_NAME__);
      break;
    }

    if (videoAlg == BCM::BC_VID_ALGO_H264)
      m_pInputThread = new CMPCInputThread(m_Device, m_dll, codec_type, extradata_size, extradata);
    else
      m_pInputThread = new CMPCInputThread(m_Device, m_dll, codec_type, 0, extradata);
    m_pInputThread->Create();

    m_pOutputThread = new CMPCOutputThread(m_Device, m_dll);
    m_pOutputThread->Create();

    m_drop_state = false;
    m_IsConfigured = true;

    CLog::Log(LOGDEBUG, "%s: codec opened", __MODULE_NAME__);
  } while(false);

  return m_IsConfigured;
}

void CCrystalHD::CloseDecoder(void)
{
  if (m_pInputThread)
  {
    m_pInputThread->StopThread();
    delete m_pInputThread;
    m_pInputThread = NULL;
  }
  if (m_pOutputThread)
  {
    while(m_BusyList.Count())
      m_pOutputThread->FreeListPush( m_BusyList.Pop() );

    m_pOutputThread->StopThread();
    delete m_pOutputThread;
    m_pOutputThread = NULL;
  }

  if (m_Device)
  {
    m_dll->DtsFlushRxCapture(m_Device, TRUE);
    m_dll->DtsStopDecoder(m_Device);
    m_dll->DtsCloseDecoder(m_Device);
  }
  m_IsConfigured = false;

  CLog::Log(LOGDEBUG, "%s: codec closed", __MODULE_NAME__);
}

void CCrystalHD::Reset(void)
{
  // Calling for non-error flush, flush all 
  m_dll->DtsFlushInput(m_Device, 2);
  m_pInputThread->Flush();
  m_pOutputThread->Flush();

  while( m_BusyList.Count())
    m_pOutputThread->FreeListPush( m_BusyList.Pop() );

  m_timestamps.clear();

  CLog::Log(LOGDEBUG, "%s: codec flushed", __MODULE_NAME__);
}

bool CCrystalHD::WaitInput(unsigned int msec)
{
  if(m_pInputThread)
    return m_pInputThread->WaitInput(msec);
  else
    return true;
}

bool CCrystalHD::AddInput(unsigned char *pData, size_t size, double dts, double pts)
{
  if (m_pInputThread)
  {
    CHD_TIMESTAMP timestamp;
    
    timestamp.dts = dts;
    timestamp.pts = pts;
    m_timestamps.push_back(timestamp);

    return m_pInputThread->AddInput(pData, size, pts_dtoi(timestamp.pts) );
  }
  else
    return false;
}

int CCrystalHD::GetInputCount(void)
{
  if (m_pInputThread)
    return m_pInputThread->GetInputCount();
  else
    return 0;
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
    // leave one around, DVDPlayer expects it
    while( m_BusyList.Count() > 1)
      m_pOutputThread->FreeListPush( m_BusyList.Pop() );
  }
}

void CCrystalHD::ReadyListPop(void)
{
  if (m_pOutputThread)
  {
    // leave one around, DVDPlayer expects it
    if ( m_pOutputThread->GetReadyCount() )
      m_pOutputThread->FreeListPush( m_pOutputThread->ReadyListPop() );
  }
}

bool CCrystalHD::GetPicture(DVDVideoPicture *pDvdVideoPicture)
{
  CPictureBuffer* pBuffer = m_pOutputThread->ReadyListPop();

  if (pBuffer->m_timestamp == 0)
  {
    // All timestamps that we pass to hardware have a value != 0
    // so this a picture frame came from a demxer packet with more than one
    // picture frames encoded inside. Set DVD_NOPTS_VALUE both dts/pts and
    // let DVDPlayerVideo sort it out.
    pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  }
  else
  {
    if (!m_timestamps.empty())
    {
      CHD_TIMESTAMP timestamp;
      
      timestamp = m_timestamps.front();
      m_timestamps.pop_front();
      pDvdVideoPicture->dts = timestamp.dts;
      pDvdVideoPicture->pts = pts_itod(pBuffer->m_timestamp);
    }
    else
    {
      pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
      pDvdVideoPicture->pts = pts_itod(pBuffer->m_timestamp);
    }
  }

  pDvdVideoPicture->iWidth = pBuffer->m_width;
  pDvdVideoPicture->iHeight = pBuffer->m_height;
  pDvdVideoPicture->iDisplayWidth = pBuffer->m_width;
  pDvdVideoPicture->iDisplayHeight = pBuffer->m_height;

  // Y plane
  pDvdVideoPicture->data[0] = (BYTE*)pBuffer->m_y_buffer_ptr;
  pDvdVideoPicture->iLineSize[0] = pBuffer->m_width;
  switch(pBuffer->m_format)
  {
    default:
    case DVDVideoPicture::FMT_NV12:
      // UV packed plane
      pDvdVideoPicture->data[1] = (BYTE*)pBuffer->m_uv_buffer_ptr;
      pDvdVideoPicture->iLineSize[1] = pBuffer->m_width;
      // unused
      pDvdVideoPicture->data[2] = NULL;
      pDvdVideoPicture->iLineSize[2] = 0;
    break;
    case DVDVideoPicture::FMT_YUV420P:
      // U plane
      pDvdVideoPicture->data[1] = (BYTE*)pBuffer->m_u_buffer_ptr;
      pDvdVideoPicture->iLineSize[1] = pBuffer->m_width / 2;
      // V plane
      pDvdVideoPicture->data[2] = (BYTE*)pBuffer->m_v_buffer_ptr;
      pDvdVideoPicture->iLineSize[2] = pBuffer->m_width / 2;
    break;
  }

  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = (DVD_TIME_BASE / pBuffer->m_framerate);
  pDvdVideoPicture->color_range = pBuffer->m_color_range;
  pDvdVideoPicture->color_matrix = pBuffer->m_color_matrix;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= pBuffer->m_interlace ? DVP_FLAG_INTERLACED : 0;
  //if (pBuffer->m_interlace)
  //  pDvdVideoPicture->iFlags |= DVP_FLAG_TOP_FIELD_FIRST;
  pDvdVideoPicture->iFlags |= m_drop_state ? DVP_FLAG_DROPPED : 0;
  pDvdVideoPicture->format = pBuffer->m_format;

  m_BusyList.Push(pBuffer);
  return true;
}

void CCrystalHD::SetDropState(bool bDrop)
{
  if (m_drop_state != bDrop)
  {
    m_drop_state = bDrop;
    CLog::Log(LOGDEBUG, "%s: SetDropState... %d", __MODULE_NAME__, m_drop_state);
  }
/*
  if (m_drop_state)
    m_dll->DtsSetSkipPictureMode(m_Device, 1);
  else
    m_dll->DtsSetSkipPictureMode(m_Device, 0);
*/
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
