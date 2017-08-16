/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "DVDVideoCodecIMX.h"

#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "guilib/GraphicContext.h"
#include "utils/StringUtils.h"
#include "settings/MediaSettings.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

#include "linux/imx/IMX.h"
#include "libavcodec/avcodec.h"

#include "guilib/LocalizeStrings.h"

#include <cassert>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <algorithm>
#include <linux/fb.h>
#include <list>

#define FRAME_ALIGN             16
#define MEDIAINFO               1
#define RENDER_QUEUE_SIZE       NUM_BUFFERS
#define DECODE_OUTPUT_SIZE      15
#define IN_DECODER_SET          -1

#define _4CC(c1,c2,c3,c4) (((uint32_t)(c4)<<24)|((uint32_t)(c3)<<16)|((uint32_t)(c2)<<8)|(uint32_t)(c1))
#define Align(ptr,align)  (((unsigned int)ptr + (align) - 1)/(align)*(align))
#define Align2(ptr,align)  (((unsigned int)ptr)/(align)*(align))
#define ALIGN Align

#define BIT(nr) (1UL << (nr))
#define SZ_4K                   4*1024

#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS) || defined(TRACE_FRAMES)
unsigned char CDVDVideoCodecIMXBuffer::i = 0;
#endif

CIMXContext   g_IMXContext;
std::shared_ptr<CIMXCodec> g_IMXCodec;

std::list<VpuFrameBuffer*> m_recycleBuffers;

// Number of fb pages used for panning
const int CIMXContext::m_fbPages = 3;

// Experiments show that we need at least one more (+1) VPU buffer than the min value returned by the VPU
const unsigned int CIMXCodec::m_extraVpuBuffers = 1 + CIMXContext::m_fbPages + RENDER_QUEUE_SIZE;

CDVDVideoCodecIMX::~CDVDVideoCodecIMX()
{
  m_IMXCodec.reset();
  if (g_IMXCodec.use_count() == 1)
    g_IMXCodec.reset();
}

bool CDVDVideoCodecIMX::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!g_IMXCodec)
  {
    m_IMXCodec.reset(new CIMXCodec);
    g_IMXCodec = m_IMXCodec;
  }
  else
    m_IMXCodec = g_IMXCodec;

  return g_IMXCodec->Open(hints, options, m_pFormatName, &m_processInfo);
}

unsigned CDVDVideoCodecIMX::GetAllowedReferences()
{
  return RENDER_QUEUE_SIZE;
}

bool CIMXCodec::VpuAllocBuffers(VpuMemInfo *pMemBlock)
{
  int i, size;
  void* ptr;
  VpuMemDesc vpuMem;

  for(i=0; i<pMemBlock->nSubBlockNum; i++)
  {
    size = pMemBlock->MemSubBlock[i].nAlignment + pMemBlock->MemSubBlock[i].nSize;
    if (pMemBlock->MemSubBlock[i].MemType == VPU_MEM_VIRT)
    { // Allocate standard virtual memory
      ptr = malloc(size);
      if(ptr == NULL)
      {
        ExitError("%s - Unable to malloc %d bytes.\n", size);
        return false;
      }

      pMemBlock->MemSubBlock[i].pVirtAddr = (unsigned char*)Align(ptr, pMemBlock->MemSubBlock[i].nAlignment);

      m_decMemInfo.nVirtNum++;
      m_decMemInfo.virtMem = (void**)realloc(m_decMemInfo.virtMem, m_decMemInfo.nVirtNum*sizeof(void*));
      m_decMemInfo.virtMem[m_decMemInfo.nVirtNum-1] = ptr;
    }
    else
    { // Allocate contiguous mem for DMA
      vpuMem.nSize = size;
      if(!VpuAlloc(&vpuMem))
        return false;

      pMemBlock->MemSubBlock[i].pVirtAddr = (unsigned char*)Align(vpuMem.nVirtAddr, pMemBlock->MemSubBlock[i].nAlignment);
      pMemBlock->MemSubBlock[i].pPhyAddr = (unsigned char*)Align(vpuMem.nPhyAddr, pMemBlock->MemSubBlock[i].nAlignment);
    }
  }

  return true;
}

bool CIMXCodec::VpuFreeBuffers(bool dispose)
{
  VpuMemDesc vpuMem;
  VpuDecRetCode vpuRet;
  int freePhyNum = dispose ? m_decMemInfo.nPhyNum : m_vpuFrameBuffers.size();
  bool ret = true;

  m_decOutput.for_each(Release);

  if (m_decMemInfo.virtMem && dispose)
  {
    //free virtual mem
    for(int i=0; i<m_decMemInfo.nVirtNum; i++)
    {
      if (m_decMemInfo.virtMem[i])
        free((void*)m_decMemInfo.virtMem[i]);
    }
    free(m_decMemInfo.virtMem);
    m_decMemInfo.virtMem = NULL;
    m_decMemInfo.nVirtNum = 0;
  }

  if (m_decMemInfo.nPhyNum)
  {
    int released = 0;
    //free physical mem
    for(int i=m_decMemInfo.nPhyNum - 1; i>=m_decMemInfo.nPhyNum - freePhyNum; i--)
    {
      vpuMem.nPhyAddr = m_decMemInfo.phyMem[i].nPhyAddr;
      vpuMem.nVirtAddr = m_decMemInfo.phyMem[i].nVirtAddr;
      vpuMem.nCpuAddr = m_decMemInfo.phyMem[i].nCpuAddr;
      vpuMem.nSize = m_decMemInfo.phyMem[i].nSize;
      vpuRet = VPU_DecFreeMem(&vpuMem);
      if(vpuRet != VPU_DEC_RET_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s - Error while trying to free physical memory (%d).\n", __FUNCTION__, ret);
        ret = false;
        break;
      }
      else
        released++;
    }

    m_decMemInfo.nPhyNum -= released;
    if (!m_decMemInfo.nPhyNum)
    {
      free(m_decMemInfo.phyMem);
      m_decMemInfo.phyMem = NULL;
    }
  }

  m_vpuFrameBuffers.clear();
  return ret;
}


bool CIMXCodec::VpuOpen()
{
  VpuDecRetCode  ret;
  VpuVersionInfo vpuVersion;
  VpuMemInfo     memInfo;
  int            param;

  memset(&memInfo, 0, sizeof(VpuMemInfo));
  ret = VPU_DecLoad();
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - VPU load failed with error code %d.\n", __FUNCTION__, ret);
    goto VpuOpenError;
  }

  ret = VPU_DecGetVersionInfo(&vpuVersion);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - VPU version cannot be read (%d).\n", __FUNCTION__, ret);
    goto VpuOpenError;
  }
  else
    CLog::Log(LOGDEBUG, LOGVIDEO, "VPU Lib version : major.minor.rel=%d.%d.%d.\n", vpuVersion.nLibMajor, vpuVersion.nLibMinor, vpuVersion.nLibRelease);

  ret = VPU_DecQueryMem(&memInfo);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
          CLog::Log(LOGERROR, "%s - iMX VPU query mem error (%d).\n", __FUNCTION__, ret);
          goto VpuOpenError;
  }

  if (!VpuAllocBuffers(&memInfo))
    goto VpuOpenError;

  m_decOpenParam.nReorderEnable = 1;
#ifdef IMX_INPUT_FORMAT_I420
  m_decOpenParam.nChromaInterleave = 0;
#else
  m_decOpenParam.nChromaInterleave = 1;
#endif
  m_decOpenParam.nTiled2LinearEnable = 0;
  m_decOpenParam.nEnableFileMode = 0;

  ret = VPU_DecOpen(&m_vpuHandle, &m_decOpenParam, &memInfo);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - iMX VPU open failed (%d).\n", __FUNCTION__, ret);
    goto VpuOpenError;
  }

  param = 0;
  SetVPUParams(VPU_DEC_CONF_BUFDELAY, &param);

  return true;

VpuOpenError:
  Dispose();
  return false;
}

bool CIMXCodec::VpuAlloc(VpuMemDesc *vpuMem)
{
  VpuDecRetCode ret = VPU_DecGetMem(vpuMem);
  if (ret)
  {
    CLog::Log(LOGERROR, "%s: vpu malloc frame buf of size %d failure: ret=%d \r\n",__FUNCTION__, vpuMem->nSize, ret);
    return false;
  }

  m_decMemInfo.nPhyNum++;
  m_decMemInfo.phyMem = (VpuMemDesc*)realloc(m_decMemInfo.phyMem, m_decMemInfo.nPhyNum*sizeof(VpuMemDesc));
  m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nPhyAddr = vpuMem->nPhyAddr;
  m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nVirtAddr = vpuMem->nVirtAddr;
  m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nCpuAddr = vpuMem->nCpuAddr;
  m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nSize = vpuMem->nSize;

  return true;
}

bool CIMXCodec::VpuAllocFrameBuffers()
{
  int totalSize = 0;
  int ySize     = 0;
  int uSize     = 0;
  int vSize     = 0;
  int mvSize    = 0;
  int yStride   = 0;
  int uvStride  = 0;

  VpuMemDesc vpuMem;
  unsigned char* ptr;
  unsigned char* ptrVirt;
  int nAlign;

  int nrBuf = std::min(m_initInfo.nMinFrameBufferCount + m_extraVpuBuffers + DECODE_OUTPUT_SIZE, (unsigned int)30);

  yStride = Align(m_initInfo.nPicWidth,FRAME_ALIGN);
  if(m_initInfo.nInterlace)
  {
    ySize = Align(m_initInfo.nPicWidth,FRAME_ALIGN)*Align(m_initInfo.nPicHeight,(2*FRAME_ALIGN));
  }
  else
  {
    ySize = Align(m_initInfo.nPicWidth,FRAME_ALIGN)*Align(m_initInfo.nPicHeight,FRAME_ALIGN);
  }

#ifdef IMX_INPUT_FORMAT_I420
  switch (m_initInfo.nMjpgSourceFormat)
  {
  case 0: // I420 (4:2:0)
    uvStride = yStride / 2;
    uSize = vSize = mvSize = ySize / 4;
    break;
  case 1: // Y42B (4:2:2 horizontal)
    uvStride = yStride / 2;
    uSize = vSize = mvSize = ySize / 2;
    break;
  case 3: // Y444 (4:4:4)
    uvStride = yStride;
    uSize = vSize = mvSize = ySize;
    break;
  default:
    CLog::Log(LOGERROR, "%s: invalid source format in init info\n",__FUNCTION__);
    return false;
  }

#else
  // NV12
  uvStride = yStride;
  uSize    = ySize/2;
  mvSize   = uSize/2;
#endif

  nAlign = m_initInfo.nAddressAlignment;
  if(nAlign>1)
  {
    ySize = Align(ySize, nAlign);
    uSize = Align(uSize, nAlign);
    vSize = Align(vSize, nAlign);
    mvSize = Align(mvSize, nAlign);
  }

  totalSize = ySize + uSize + vSize + mvSize + nAlign;
  for (int i=0 ; i < nrBuf; i++)
  {
    vpuMem.nSize = totalSize;
    if(!VpuAlloc(&vpuMem))
    {
      if (m_vpuFrameBuffers.size() < m_initInfo.nMinFrameBufferCount + m_extraVpuBuffers)
        return false;

      CLog::Log(LOGWARNING, "%s: vpu can't allocate sufficient extra buffers. specify bigger CMA e.g. cma=320M.\r\n",__FUNCTION__);
      break;
    }

    //fill frameBuf
    ptr = (unsigned char*)vpuMem.nPhyAddr;
    ptrVirt = (unsigned char*)vpuMem.nVirtAddr;

    //align the base address
    if(nAlign>1)
    {
      ptr = (unsigned char*)Align(ptr,nAlign);
      ptrVirt = (unsigned char*)Align(ptrVirt,nAlign);
    }

    VpuFrameBuffer vpuFrameBuffer;
    m_vpuFrameBuffers.push_back(vpuFrameBuffer);

    // fill stride info
    m_vpuFrameBuffers[i].nStrideY           = yStride;
    m_vpuFrameBuffers[i].nStrideC           = uvStride;

    // fill phy addr
    m_vpuFrameBuffers[i].pbufY              = ptr;
    m_vpuFrameBuffers[i].pbufCb             = ptr + ySize;
#ifdef IMX_INPUT_FORMAT_I420
    m_vpuFrameBuffers[i].pbufCr             = ptr + ySize + uSize;
#else
    m_vpuFrameBuffers[i].pbufCr             = 0;
#endif
    m_vpuFrameBuffers[i].pbufMvCol          = ptr + ySize + uSize + vSize;

    // fill virt addr
    m_vpuFrameBuffers[i].pbufVirtY          = ptrVirt;
    m_vpuFrameBuffers[i].pbufVirtCb         = ptrVirt + ySize;
#ifdef IMX_INPUT_FORMAT_I420
    m_vpuFrameBuffers[i].pbufVirtCr         = ptrVirt + ySize + uSize;
#else
    m_vpuFrameBuffers[i].pbufVirtCr         = 0;
#endif
    m_vpuFrameBuffers[i].pbufVirtMvCol      = ptrVirt + ySize + uSize + vSize;

    m_vpuFrameBuffers[i].pbufY_tilebot      = 0;
    m_vpuFrameBuffers[i].pbufCb_tilebot     = 0;
    m_vpuFrameBuffers[i].pbufVirtY_tilebot  = 0;
    m_vpuFrameBuffers[i].pbufVirtCb_tilebot = 0;
  }

  if (VPU_DEC_RET_SUCCESS != VPU_DecRegisterFrameBuffer(m_vpuHandle, &m_vpuFrameBuffers[0], m_vpuFrameBuffers.size()))
    return false;

  m_processInfo->SetVideoDAR((double)m_initInfo.nQ16ShiftWidthDivHeightRatio/0x10000);

  m_decOutput.setquotasize(m_vpuFrameBuffers.size() - m_initInfo.nMinFrameBufferCount - m_extraVpuBuffers);
  return true;
}

CIMXCodec::CIMXCodec()
  : CThread("iMX VPU")
  , m_dropped(0)
  , m_lastPTS(DVD_NOPTS_VALUE)
  , m_codecControlFlags(0)
  , m_decSignal(0)
  , m_threadID(0)
  , m_decRet(VPU_DEC_INPUT_NOT_USED)
  , m_fps(-1)
  , m_burst(0)
{
  m_nrOut.store(0);

  m_vpuHandle = 0;
  m_converter = NULL;
#ifdef DUMP_STREAM
  m_dump = NULL;
#endif
  m_drainMode = VPU_DEC_IN_NORMAL;
  m_skipMode = VPU_DEC_SKIPNONE;

  m_decOutput.setquotasize(1);
  m_decInput.setquotasize(30);
  m_loaded.Reset();
}

CIMXCodec::~CIMXCodec()
{
  g_IMXContext.SetProcessInfo(nullptr);
  StopThread(false);
  ProcessSignals(SIGNAL_SIGNAL);
  SetDrainMode(VPU_DEC_IN_DRAIN);
  StopThread();
}

void CIMXCodec::DisposeDecQueues()
{
  m_decInput.signal();
  m_decInput.for_each(Release);
  m_decOutput.signal();
  m_decOutput.for_each(Release);
  m_rebuffer = true;
}

void CIMXCodec::Reset()
{
  m_queuesLock.lock();
  DisposeDecQueues();
  ProcessSignals(SIGNAL_FLUSH);
  CLog::Log(LOGDEBUG, "iMX VPU : queues cleared ===== in/out %d/%d =====\n", m_decInput.size(), m_decOutput.size());
}

bool CIMXCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options, std::string &m_pFormatName, CProcessInfo *m_pProcessInfo)
{
  CSingleLock lk(m_openLock);

  if (hints.software)
  {
    CLog::Log(LOGNOTICE, "iMX VPU : software decoding requested.\n");
    return false;
  }
  else if (hints.width > 1920)
  {
    CLog::Log(LOGNOTICE, "iMX VPU : software decoding forced - video dimensions out of spec: %d %d.", hints.width, hints.height);
    return false;
  }
  else if (hints.stills && hints.dvd)
    return false;

#ifdef DUMP_STREAM
  m_dump = fopen("stream.dump", "wb");
  if (m_dump != NULL)
  {
    fwrite(&hints.software, sizeof(hints.software), 1, m_dump);
    fwrite(&hints.codec, sizeof(hints.codec), 1, m_dump);
    fwrite(&hints.profile, sizeof(hints.profile), 1, m_dump);
    fwrite(&hints.codec_tag, sizeof(hints.codec_tag), 1, m_dump);
    fwrite(&hints.extrasize, sizeof(hints.extrasize), 1, m_dump);
    CLog::Log(LOGNOTICE, "Dump: HEADER: %d  %d  %d  %d  %d\n",
              hints.software, hints.codec, hints.profile,
              hints.codec_tag, hints.extrasize);
    if (hints.extrasize > 0)
      fwrite(hints.extradata, 1, hints.extrasize, m_dump);
  }
#endif

  if (m_hints != hints && g_IMXCodec->IsRunning())
  {
    StopThread(false);
    ProcessSignals(SIGNAL_FLUSH);
  }

  m_hints = hints;
  CLog::Log(LOGDEBUG, LOGVIDEO, "Let's decode with iMX VPU\n");

  int param = 0;
  SetVPUParams(VPU_DEC_CONF_INPUTTYPE, &param);
  SetVPUParams(VPU_DEC_CONF_SKIPMODE, &param);

#ifdef MEDIAINFO
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: fpsrate %d / fpsscale %d\n", m_hints.fpsrate, m_hints.fpsscale);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: CodecID %d \n", m_hints.codec);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: StreamType %d \n", m_hints.type);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: Level %d \n", m_hints.level);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: Profile %d \n", m_hints.profile);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: PTS_invalid %d \n", m_hints.ptsinvalid);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: Tag %d \n", m_hints.codec_tag);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: %dx%d \n", m_hints.width,  m_hints.height);
  { char str_tag[128]; av_get_codec_tag_string(str_tag, sizeof(str_tag), m_hints.codec_tag);
      CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: Tag fourcc %s\n", str_tag);
  }
  if (m_hints.extrasize)
  {
    char buf[4096];

    for (unsigned int i=0; i < m_hints.extrasize; i++)
      sprintf(buf+i*2, "%02x", ((uint8_t*)m_hints.extradata)[i]);

    CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: %s extradata %d %s\n", *(char*)m_hints.extradata == 1 ? "AnnexB" : "avcC", m_hints.extrasize, buf);
  }
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: MEDIAINFO: %d / %d \n", m_hints.width,  m_hints.height);
  CLog::Log(LOGDEBUG, LOGVIDEO, "Decode: aspect %f - forced aspect %d\n", m_hints.aspect, m_hints.forced_aspect);
#endif

  m_warnOnce = true;
  switch(m_hints.codec)
  {
  case AV_CODEC_ID_MPEG1VIDEO:
    m_decOpenParam.CodecFormat = VPU_V_MPEG2;
    m_pFormatName = "iMX-mpeg1";
    break;
  case AV_CODEC_ID_MPEG2VIDEO:
  case AV_CODEC_ID_MPEG2VIDEO_XVMC:
    m_decOpenParam.CodecFormat = VPU_V_MPEG2;
    m_pFormatName = "iMX-mpeg2";
    break;
  case AV_CODEC_ID_H263:
    m_decOpenParam.CodecFormat = VPU_V_H263;
    m_pFormatName = "iMX-h263";
    break;
  case AV_CODEC_ID_H264:
  {
    // Test for VPU unsupported profiles to revert to sw decoding
    if (m_hints.profile == 110)
    {
      CLog::Log(LOGNOTICE, "i.MX6 VPU is not able to decode AVC profile %d level %d", m_hints.profile, m_hints.level);
      return false;
    }
    m_decOpenParam.CodecFormat = VPU_V_AVC;
    m_pFormatName = "iMX-h264";
    break;
  }
  case AV_CODEC_ID_VC1:
    m_decOpenParam.CodecFormat = VPU_V_VC1_AP;
    m_pFormatName = "iMX-vc1";
    break;
  case AV_CODEC_ID_CAVS:
  case AV_CODEC_ID_AVS:
    m_decOpenParam.CodecFormat = VPU_V_AVS;
    m_pFormatName = "iMX-AVS";
    break;
  case AV_CODEC_ID_RV10:
  case AV_CODEC_ID_RV20:
  case AV_CODEC_ID_RV30:
  case AV_CODEC_ID_RV40:
    m_decOpenParam.CodecFormat = VPU_V_RV;
    m_pFormatName = "iMX-RV";
    break;
  case AV_CODEC_ID_KMVC:
    m_decOpenParam.CodecFormat = VPU_V_AVC_MVC;
    m_pFormatName = "iMX-MVC";
    break;
  case AV_CODEC_ID_VP8:
    m_decOpenParam.CodecFormat = VPU_V_VP8;
    m_pFormatName = "iMX-vp8";
    break;
  case AV_CODEC_ID_MPEG4:
    switch(m_hints.codec_tag)
    {
    case _4CC('D','I','V','X'):
      // Test for VPU unsupported profiles to revert to sw decoding
      if (m_hints.profile == -99 && m_hints.level == -99)
      {
        CLog::Log(LOGNOTICE, "i.MX6 iMX-divx4 profile %d level %d - sw decoding", m_hints.profile, m_hints.level);
        return false;
      }
      m_decOpenParam.CodecFormat = VPU_V_XVID; // VPU_V_DIVX4
      m_pFormatName = "iMX-divx4";
      break;
    case _4CC('D','X','5','0'):
    case _4CC('D','I','V','5'):
      m_decOpenParam.CodecFormat = VPU_V_XVID; // VPU_V_DIVX56
      m_pFormatName = "iMX-divx5";
      break;
    case _4CC('X','V','I','D'):
    case _4CC('M','P','4','V'):
    case _4CC('P','M','P','4'):
    case _4CC('F','M','P','4'):
      m_decOpenParam.CodecFormat = VPU_V_XVID;
      m_pFormatName = "iMX-xvid";
      break;
    default:
      CLog::Log(LOGERROR, "iMX VPU : MPEG4 codec tag %d is not (yet) handled.\n", m_hints.codec_tag);
      return false;
    }
    break;
  default:
    CLog::Log(LOGERROR, "iMX VPU : codecid %d is not (yet) handled.\n", m_hints.codec);
    return false;
  }

  std::list<EINTERLACEMETHOD> deintMethods({ EINTERLACEMETHOD::VS_INTERLACEMETHOD_AUTO,
                                             EINTERLACEMETHOD::VS_INTERLACEMETHOD_RENDER_BOB });

  for(int i = EINTERLACEMETHOD::VS_INTERLACEMETHOD_IMX_FASTMOTION; i <= VS_INTERLACEMETHOD_IMX_ADVMOTION_HALF; ++i)
    deintMethods.push_back(static_cast<EINTERLACEMETHOD>(i));

  m_processInfo = m_pProcessInfo;
  m_processInfo->SetVideoDecoderName(m_pFormatName, true);
  m_processInfo->SetVideoDimensions(m_hints.width, m_hints.height);
  m_processInfo->SetVideoDeintMethod("none");
  m_processInfo->SetVideoPixelFormat("Y/CbCr 4:2:0");
  m_processInfo->UpdateDeinterlacingMethods(deintMethods);
  g_IMXContext.SetProcessInfo(m_processInfo);

  return true;
}

void CIMXCodec::Dispose()
{
#ifdef DUMP_STREAM
  if (m_dump)
  {
    fclose(m_dump);
    m_dump = NULL;
  }
#endif

  VpuDecRetCode  ret;
  bool VPU_loaded = m_vpuHandle;

  RecycleFrameBuffers();

  if (m_vpuHandle)
  {
    ret = VPU_DecClose(m_vpuHandle);
    if (ret != VPU_DEC_RET_SUCCESS)
      CLog::Log(LOGERROR, "%s - VPU close failed with error code %d.\n", __FUNCTION__, ret);
    else
      CLog::Log(LOGDEBUG, "%s - VPU closed.", __FUNCTION__);

    m_vpuHandle = 0;
  }

  VpuFreeBuffers();

  if (VPU_loaded)
  {
    ret = VPU_DecUnLoad();
    if (ret != VPU_DEC_RET_SUCCESS)
      CLog::Log(LOGERROR, "%s - VPU unload failed with error code %d.\n", __FUNCTION__, ret);
  }

  if (m_converter)
  {
    m_converter->Close();
    SAFE_DELETE(m_converter);
  }
}

void CIMXCodec::SetVPUParams(VpuDecConfig InDecConf, void* pInParam)
{
  if (m_vpuHandle)
    if (VPU_DEC_RET_SUCCESS != VPU_DecConfig(m_vpuHandle, InDecConf, pInParam))
      CLog::Log(LOGERROR, "%s - iMX VPU set dec param failed (%d).\n", __FUNCTION__, (int)InDecConf);
}

void CIMXCodec::SetDrainMode(VpuDecInputType drain)
{
  if (m_drainMode == drain)
    return;

  m_drainMode = drain;
  VpuDecInputType config = drain == IN_DECODER_SET ? VPU_DEC_IN_DRAIN : drain;
  SetVPUParams(VPU_DEC_CONF_INPUTTYPE, &config);
  if (drain == VPU_DEC_IN_DRAIN && !EOS())
    ProcessSignals(SIGNAL_SIGNAL);
}

void CIMXCodec::SetSkipMode(VpuDecSkipMode skip)
{
  if (m_skipMode == skip)
    return;

  m_skipMode = skip;
  VpuDecSkipMode config = skip == IN_DECODER_SET ? VPU_DEC_SKIPB : skip;
  SetVPUParams(VPU_DEC_CONF_SKIPMODE, &config);
}

bool CIMXCodec::GetCodecStats(double &pts, int &droppedFrames, int &skippedPics)
{
  droppedFrames = m_dropped;
  skippedPics = -1;
  m_dropped = 0;
  pts = m_lastPTS;
  return true;
}

void CIMXCodec::SetCodecControl(int flags)
{
  if (!FBRegistered())
    return;

  m_codecControlFlags = flags;
  if (m_codecControlFlags & DVD_CODEC_CTRL_HURRY && !m_burst)
    m_burst = std::max(0, RENDER_QUEUE_SIZE - 2);

  SetDrainMode(m_codecControlFlags & DVD_CODEC_CTRL_DRAIN && !m_decInput.size() ? VPU_DEC_IN_DRAIN : VPU_DEC_IN_NORMAL);
}

bool CIMXCodec::getOutputFrame(VpuDecOutFrameInfo *frm)
{
  VpuDecRetCode ret = VPU_DecGetOutputFrame(m_vpuHandle, frm);
  if(VPU_DEC_RET_SUCCESS != ret)
    CLog::Log(LOGERROR, "%s - VPU Cannot get output frame(%d).\n", __FUNCTION__, ret);
  return ret == VPU_DEC_RET_SUCCESS;
}

int CIMXCodec::Decode(BYTE *pData, int iSize, double dts, double pts)
{
  static class CIMXFps ptrn;

  if (EOS() && m_drainMode && !m_decOutput.size())
    return VC_BUFFER;

  int ret = 0;
  if (!g_IMXCodec->IsRunning())
  {
    if (!m_decInput.full())
    {
      if (pts != DVD_NOPTS_VALUE)
        ptrn.Add(pts);
      else if (dts != DVD_NOPTS_VALUE)
        ptrn.Add(dts);

      ret |= VC_BUFFER;
    }
    else
    {
      double fd = ptrn.GetFrameDuration(true);
      if (fd > 80000 && m_hints.fpsscale != 0)
        m_fps = (double)m_hints.fpsrate / m_hints.fpsscale;
      else if (fd != 0)
        m_fps = (double)DVD_TIME_BASE / fd;
      else
        m_fps = 60;

      m_decOpenParam.nMapType = 1;

      ptrn.Flush();
      g_IMXCodec->Create();
      g_IMXCodec->WaitStartup();
    }
  }

  if (pData)
    m_decInput.push(new VPUTask({ pData, iSize, 0, 0, 0, pts, dts, 0, 0 }, m_converter));

  if (!IsDraining() &&
      (m_decInput.size() < m_decInput.getquotasize() -1))
  {
    ret |= VC_BUFFER;
    if (!m_burst && m_decInput.size() < m_decInput.getquotasize() /2)
      return ret;
  }

  if (m_rebuffer && m_decInput.size() > m_decInput.getquotasize() /2)
    m_rebuffer = false;

  if ((m_decOutput.size() >= m_decOutput.getquotasize() /2 ||
       m_drainMode || m_burst || !ret) && m_decOutput.size() && !m_rebuffer)
    ret |= VC_PICTURE;

  if (m_burst)
  {
    if (m_decInput.size() >= RENDER_QUEUE_SIZE * 2)
      ret &= ~VC_BUFFER;
    --m_burst;
  }

#ifdef IMX_PROFILE
  CLog::Log(LOGDEBUG, "%s - demux size: %d  dts : %f - pts : %f - addr : 0x%x, return %d ===== in/out %d/%d =====\n",
                       __FUNCTION__, iSize, recalcPts(dts), recalcPts(pts), (uint)pData, ret, m_decInput.size(), m_decOutput.size());
#endif

  if (!(ret & VC_PICTURE) || !ret || m_drainMode)
    Sleep(10);

  return ret;
}

void CIMXCodec::ReleaseFramebuffer(VpuFrameBuffer* fb)
{
  m_recycleBuffers.push_back(fb);
}

void CIMXCodec::RecycleFrameBuffers()
{
  while(!m_recycleBuffers.empty())
  {
    m_pts[m_recycleBuffers.front()] = DVD_NOPTS_VALUE;
    VPU_DecOutFrameDisplayed(m_vpuHandle, m_recycleBuffers.front());
    m_recycleBuffers.pop_front();
  --m_nrOut;
  }
}

inline
void CIMXCodec::AddExtraData(VpuBufferNode *bn, bool force)
{
  if ((m_decOpenParam.CodecFormat == VPU_V_MPEG2) ||
      (m_decOpenParam.CodecFormat == VPU_V_VC1_AP)||
      (m_decOpenParam.CodecFormat == VPU_V_XVID)  ||
      (force))
    bn->sCodecData = { (unsigned char *)m_hints.extradata, m_hints.extrasize };
  else
    bn->sCodecData = { nullptr, 0 };
}

void CIMXCodec::Process()
{
  VpuDecFrameLengthInfo         frameLengthInfo;
  VpuBufferNode                 inData;
  VpuBufferNode                 dummy;
  VpuDecRetCode                 ret;
  int                           retStatus;
  VPUTask                      *task = nullptr;
#ifdef IMX_PROFILE
  static unsigned long long     previous, current;
  int                           freeInfo;
#endif

  m_threadID = GetCurrentThreadId();
  SetPriority(GetPriority()+1);

  m_recycleBuffers.clear();
  m_pts.clear();
  m_loaded.Set();

  m_dropped = 0;
  m_dropRequest = false;
  m_lastPTS = DVD_NOPTS_VALUE;

  memset(&dummy, 0, sizeof(dummy));
  AddExtraData(&dummy, (m_decOpenParam.CodecFormat == VPU_V_AVC && m_hints.extrasize));
  inData = dummy;
  if (dummy.sCodecData.pData)
    CLog::Log(LOGVIDEO, "Decode: MEDIAINFO: adding extra codec data\n");

  VpuOpen();

  while (!m_bStop && m_vpuHandle)
  {
    RecycleFrameBuffers();
    SAFE_DELETE(task);
    if (!(task = m_decInput.pop()))
      task = new VPUTask();

#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS)
    unsigned long long before_dec;
#ifdef IMX_PROFILE
    current = XbmcThreads::SystemClockMillis();
    CLog::Log(LOGDEBUG, "%s - delta time decode : %llu - demux size : %d  dts : %f - pts : %f - addr : 0x%x\n",
                                      __FUNCTION__, current - previous, task->demux.iSize, recalcPts(task->demux.dts), recalcPts(task->demux.pts), (uint)task->demux.pData);
    previous = current;
#endif
#endif

    inData.nSize = task->demux.iSize;
    inData.pPhyAddr = NULL;
    inData.pVirAddr = task->demux.pData;

    // some streams have problem with getting initial info after seek into (during playback start).
    // feeding VPU with extra data helps
    if (!m_vpuFrameBuffers.size() && !task->IsEmpty() && m_decRet & VPU_DEC_NO_ENOUGH_INBUF)
      AddExtraData(&inData, true);

#ifdef IMX_PROFILE_BUFFERS
    static unsigned long long dec_time = 0;
#endif

    while (!m_bStop) // Decode as long as the VPU consumes data
    {
      RecycleFrameBuffers();
      ProcessSignals();

      retStatus = m_decRet & VPU_DEC_SKIP ? VC_USERDATA : 0;

#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS)
      before_dec = XbmcThreads::SystemClockMillis();
#endif
      ret = VPU_DecDecodeBuf(m_vpuHandle, &inData, &m_decRet);
#ifdef IMX_PROFILE_BUFFERS
      unsigned long long dec_single_call = XbmcThreads::SystemClockMillis()-before_dec;
      dec_time += dec_single_call;
#endif
#ifdef IMX_PROFILE
      VPU_DecGetNumAvailableFrameBuffers(m_vpuHandle, &freeInfo);
      CLog::Log(LOGDEBUG, "%s - VPU ret %d dec 0x%x decode takes : %lld free: %d\n\n", __FUNCTION__, ret, m_decRet,  XbmcThreads::SystemClockMillis() - before_dec, freeInfo);
#endif

      if (m_skipMode == IN_DECODER_SET)
        SetSkipMode(VPU_DEC_SKIPNONE);

      if (m_drainMode == IN_DECODER_SET)
      {
        AddExtraData(&inData);
        SetDrainMode(VPU_DEC_IN_NORMAL);
      }

      if (m_decRet & VPU_DEC_OUTPUT_EOS)
        break;

      if (ret != VPU_DEC_RET_SUCCESS && ret != VPU_DEC_RET_FAILURE_TIMEOUT)
        ExitError("VPU decode failed with error code %d (0x%x).\n", ret, m_decRet);

      if (m_decRet & VPU_DEC_INIT_OK || m_decRet & VPU_DEC_RESOLUTION_CHANGED)
      // VPU decoding init OK : We can retrieve stream info
      {
        if (m_decRet & VPU_DEC_RESOLUTION_CHANGED)
        {
          while (m_nrOut > 2 && !m_bStop)
          {
            RecycleFrameBuffers();
            std::this_thread::yield();
          }
        }

        if (VPU_DecGetInitialInfo(m_vpuHandle, &m_initInfo) != VPU_DEC_RET_SUCCESS)
          ExitError("VPU get initial info failed");

        if (!VpuFreeBuffers(false) || !VpuAllocFrameBuffers())
          ExitError("VPU error while registering frame buffers");

        if (m_initInfo.nInterlace && m_fps >= 49 && m_decOpenParam.nMapType == 1)
        {
          m_decOpenParam.nMapType = 0;
          Dispose();
          VpuOpen();
          continue;
        }

        if (m_initInfo.nInterlace && m_fps <= 30)
          m_fps *= 2;

        m_processInfo->SetVideoFps(m_fps);

        CLog::Log(LOGDEBUG, "%s - VPU Init Stream Info : %dx%d (interlaced : %d - Minframe : %d)"\
                  " - Align : %d bytes - crop : %d %d %d %d - Q16Ratio : %x, fps: %.3f\n", __FUNCTION__,
          m_initInfo.nPicWidth, m_initInfo.nPicHeight, m_initInfo.nInterlace, m_initInfo.nMinFrameBufferCount,
          m_initInfo.nAddressAlignment, m_initInfo.PicCropRect.nLeft, m_initInfo.PicCropRect.nTop,
          m_initInfo.PicCropRect.nRight, m_initInfo.PicCropRect.nBottom, m_initInfo.nQ16ShiftWidthDivHeightRatio, m_fps);

        m_decInput.setquotasize(m_fps / 2);

        bool getFrame = !(m_decOpenParam.CodecFormat == VPU_V_AVC && *(char*)m_hints.extradata == 1);
        getFrame &= m_decOpenParam.CodecFormat != VPU_V_MPEG2 && m_decOpenParam.CodecFormat != VPU_V_XVID;
        if (getFrame || m_decRet & VPU_DEC_RESOLUTION_CHANGED)
        {
          SetDrainMode((VpuDecInputType)IN_DECODER_SET);
          inData = dummy;
          continue;
        }
      }

      if (m_decRet & VPU_DEC_ONE_FRM_CONSUMED)
        if (!VPU_DecGetConsumedFrameInfo(m_vpuHandle, &frameLengthInfo) && frameLengthInfo.pFrame)
          m_pts[frameLengthInfo.pFrame] = task->demux.pts;

      if (m_decRet & CLASS_PICTURE && getOutputFrame(&m_frameInfo))
      {
        ++m_nrOut;
        CDVDVideoCodecIMXBuffer *buffer = new CDVDVideoCodecIMXBuffer(&m_frameInfo, m_fps, m_decOpenParam.nMapType);

        /* quick & dirty fix to get proper timestamping for VP8 codec */
        if (m_decOpenParam.CodecFormat == VPU_V_VP8)
          buffer->SetPts(task->demux.pts);
        else
          buffer->SetPts(m_pts[m_frameInfo.pDisplayFrameBuf]);

        buffer->SetDts(task->demux.dts);

#ifdef IMX_PROFILE_BUFFERS
        CLog::Log(LOGNOTICE, "+D  %f  %lld\n", recalcPts(buffer->GetPts()), dec_time);
        dec_time = 0;
#endif
#ifdef TRACE_FRAMES
        CLog::Log(LOGDEBUG, "+  0x%x dts %f pts %f  (VPU)\n", buffer->GetIdx(), recalcPts(task->demux.dts), recalcPts(buffer->GetPts()));
#endif

#ifdef IMX_PROFILE_BUFFERS
        static unsigned long long lastD = 0;
        unsigned long long current = XbmcThreads::SystemClockMillis();
        CLog::Log(LOGNOTICE, "+V  %f  %lld\n", recalcPts(buffer->GetPts()), current-lastD);
        lastD = current;
#endif

        if (m_decRet & VPU_DEC_OUTPUT_DIS)
          buffer->SetFlags(DVP_FLAG_ALLOCATED);

        if (!m_decOutput.push(buffer))
          SAFE_RELEASE(buffer);
        else
          m_lastPTS = buffer->GetPts();

      }
      else if (m_decRet & CLASS_DROP)
        ++m_dropped;

      if (m_decRet & VPU_DEC_SKIP)
        ++m_dropped;

      if (m_decRet & VPU_DEC_NO_ENOUGH_BUF && m_decOutput.size())
      {
        m_decOutput.pop()->Release();
        FlushVPU();
        continue;
      }

      ProcessSignals();

      if (retStatus & VC_USERDATA)
        continue;

      if (m_decRet & CLASS_FORCEBUF)
        break;

      if (!(m_drainMode ||
            m_decRet & (CLASS_NOBUF | CLASS_DROP)))
        break;

      inData = dummy;
    } // Decode loop

    task->Release();
  } // Process() main loop

  ProcessSignals(SIGNAL_RESET | SIGNAL_DISPOSE);
}

void CIMXCodec::ProcessSignals(int signal)
{
  if (signal & SIGNAL_SIGNAL)
  {
    m_decInput.signal();
    m_decOutput.signal();
  }
  if (!(m_decSignal | signal))
    return;

  CSingleLock lk(m_signalLock);
  m_decSignal |= signal & ~SIGNAL_SIGNAL;

  if (!IsCurrentThread())
    return;

  int process = m_decSignal;
  m_decSignal = 0;

  if (process & SIGNAL_FLUSH)
  {
    FlushVPU();
    m_queuesLock.unlock();
  }
  if (process & SIGNAL_RESET)
    DisposeDecQueues();
  if (process & SIGNAL_DISPOSE)
    Dispose();
}

void CIMXCodec::FlushVPU()
{
  int ret = VPU_DecFlushAll(m_vpuHandle);
  if (ret != VPU_DEC_RET_SUCCESS && ret != VPU_DEC_RET_INVALID_HANDLE)
    CLog::Log(LOGERROR, "%s: VPU flush failed with error code %d.\n", __FUNCTION__, ret);
}

inline
void CIMXCodec::ExitError(const char *msg, ...)
{
  va_list va;
  va_start(va, msg);
  CLog::Log(LOGERROR, "%s: %s", __FUNCTION__, StringUtils::FormatV(msg, va).c_str());
  va_end(va);

  StopThread(false);
}

bool CIMXCodec::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  pDvdVideoPicture->IMXBuffer = m_decOutput.pop();
  assert(pDvdVideoPicture->IMXBuffer);

#ifdef IMX_PROFILE
  static unsigned int previous = 0;
  unsigned int current;

  current = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "+G 0x%x %f/%f tm:%03d : Interlaced 0x%x\n", pDvdVideoPicture->IMXBuffer->GetIdx(),
                            recalcPts(pDvdVideoPicture->IMXBuffer->GetDts()), recalcPts(pDvdVideoPicture->IMXBuffer->GetPts()), current - previous,
                            m_initInfo.nInterlace ? pDvdVideoPicture->IMXBuffer->GetFieldType() : 0);
  previous = current;
#endif

  if (m_dropRequest && !m_burst)
  {
    pDvdVideoPicture->iFlags = DVP_FLAG_DROPPED;
    ++m_dropped;
  }
  else
    pDvdVideoPicture->iFlags = pDvdVideoPicture->IMXBuffer->GetFlags();

  if (m_initInfo.nInterlace)
  {
    if (pDvdVideoPicture->IMXBuffer->GetFieldType() == VPU_FIELD_NONE && m_warnOnce)
    {
      m_warnOnce = false;
      CLog::Log(LOGWARNING, "Interlaced content reported by VPU, but full frames detected - Please turn off deinterlacing manually.");
    }
    else if (pDvdVideoPicture->IMXBuffer->GetFieldType() == VPU_FIELD_TB || pDvdVideoPicture->IMXBuffer->GetFieldType() == VPU_FIELD_TOP)
      pDvdVideoPicture->iFlags |= DVP_FLAG_TOP_FIELD_FIRST;

    pDvdVideoPicture->iFlags |= DVP_FLAG_INTERLACED;
  }

  pDvdVideoPicture->format = RENDER_FMT_IMXMAP;
  pDvdVideoPicture->iWidth = pDvdVideoPicture->IMXBuffer->m_pctWidth;
  pDvdVideoPicture->iHeight = pDvdVideoPicture->IMXBuffer->m_pctHeight;

  pDvdVideoPicture->iDisplayWidth = ((pDvdVideoPicture->iWidth * m_frameInfo.pExtInfo->nQ16ShiftWidthDivHeightRatio) + 32767) >> 16;
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;

  pDvdVideoPicture->pts = pDvdVideoPicture->IMXBuffer->GetPts();
  pDvdVideoPicture->dts = pDvdVideoPicture->IMXBuffer->GetDts();

  if (pDvdVideoPicture->iFlags & DVP_FLAG_DROPPED)
    SAFE_RELEASE(pDvdVideoPicture->IMXBuffer);

  return true;
}

void CIMXCodec::SetDropState(bool bDrop)
{
  m_dropRequest = bDrop;
}

bool CIMXCodec::IsCurrentThread() const
{
  return CThread::IsCurrentThread(m_threadID);
}

/*******************************************/
CDVDVideoCodecIMXBuffer::CDVDVideoCodecIMXBuffer(VpuDecOutFrameInfo *frameInfo, double fps, int map)
  : m_dts(DVD_NOPTS_VALUE)
  , m_fieldType(frameInfo->eFieldType)
  , m_frameBuffer(frameInfo->pDisplayFrameBuf)
  , m_iFlags(DVP_FLAG_DROPPED)
  , m_convBuffer(nullptr)
{
  m_pctWidth  = frameInfo->pExtInfo->FrmCropRect.nRight - frameInfo->pExtInfo->FrmCropRect.nLeft;
  m_pctHeight = frameInfo->pExtInfo->FrmCropRect.nBottom - frameInfo->pExtInfo->FrmCropRect.nTop;

  // Some codecs (VC1?) lie about their frame size (mod 16). Adjust...
  iWidth      = (((frameInfo->pExtInfo->nFrmWidth) + 15) & ~15);
  iHeight     = (((frameInfo->pExtInfo->nFrmHeight) + 15) & ~15);

  pVirtAddr   = m_frameBuffer->pbufVirtY;
  pPhysAddr   = (int)m_frameBuffer->pbufY;

#ifdef IMX_INPUT_FORMAT_I420
  iFormat     = _4CC('I', '4', '2', '0');
#else
  iFormat     = map == 1 ? _4CC('T', 'N', 'V', 'P'):
                map == 0 ? _4CC('N', 'V', '1', '2'):
                           _4CC('T', 'N', 'V', 'F');
#endif
  m_fps       = fps;
#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS) || defined(TRACE_FRAMES)
  m_idx       = i++;
#endif
  Lock();
}

void CDVDVideoCodecIMXBuffer::Lock()
{
  long count = ++m_iRefs;
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "R+ 0x%x  -  ref : %ld  (VPU)\n", m_idx, count);
#endif
}

long CDVDVideoCodecIMXBuffer::Release()
{
  long count = --m_iRefs;
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "R- 0x%x  -  ref : %ld  (VPU)\n", m_idx, count);
#endif

  if (count)
    return count;

  CIMXCodec::ReleaseFramebuffer(m_frameBuffer);
  if (m_convBuffer)
    g2d_free(m_convBuffer);

  delete this;
  return 0;
}

CDVDVideoCodecIMXBuffer::~CDVDVideoCodecIMXBuffer()
{
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "~  0x%x  (VPU)\n", m_idx);
#endif
}

CIMXContext::CIMXContext()
  : CThread("iMX IPU")
  , m_fbHandle(0)
  , m_fbCurrentPage(0)
  , m_fbPhysAddr(0)
  , m_fbVirtAddr(NULL)
  , m_ipuHandle(0)
  , m_vsync(true)
  , m_pageCrops(NULL)
  , m_bFbIsConfigured(false)
  , m_g2dHandle(NULL)
  , m_bufferCapture(NULL)
  , m_deviceName("/dev/fb1")
{
  m_pageCrops = new CRectInt[m_fbPages];
  OpenDevices();
}

CIMXContext::~CIMXContext()
{
  Stop(false);
  Dispose();
  CloseDevices();
}

bool CIMXContext::AdaptScreen(bool allocate)
{
  MemMap();

  struct fb_var_screeninfo fbVar;
  if (!GetFBInfo("/dev/fb0", &fbVar))
    goto Err;

  m_fbWidth = allocate ? 1920 : fbVar.xres;
  m_fbHeight = allocate ? 1080 : fbVar.yres;

  if (!GetFBInfo(m_deviceName, &m_fbVar))
    goto Err;

  m_fbVar.xoffset = 0;
  m_fbVar.yoffset = 0;

  if (!allocate && (fbVar.bits_per_pixel == 16 || m_fps >= 49 && m_fbHeight >= 1080))
  {
    m_fbVar.nonstd = _4CC('Y', 'U', 'Y', 'V');
    m_fbVar.bits_per_pixel = 16;
  }
  else
  {
    m_fbVar.nonstd = _4CC('R', 'G', 'B', '4');
    m_fbVar.bits_per_pixel = 32;
  }

  m_fbVar.activate = FB_ACTIVATE_NOW;
  m_fbVar.xres = m_fbWidth;
  m_fbVar.yres = m_fbHeight;

  m_fbVar.yres_virtual = (m_fbVar.yres + 1) * m_fbPages;
  m_fbVar.xres_virtual = m_fbVar.xres;

  CloseIPU();
  Blank();

  struct fb_fix_screeninfo fb_fix;

  if (ioctl(m_fbHandle, FBIOPUT_VSCREENINFO, &m_fbVar) == -1)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to setup %s (%s)\n", m_deviceName.c_str(), strerror(errno));
    goto Err;
  }
  else if (ioctl(m_fbHandle, FBIOGET_FSCREENINFO, &fb_fix) == -1)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to query fixed screen info at %s (%s)\n", m_deviceName.c_str(), strerror(errno));
    goto Err;
  }

  MemMap(&fb_fix);
  Unblank();

  OpenIPU();
  m_bFbIsConfigured = true;

  return true;

Err:
  TaskRestart();
  return false;
}

bool CIMXContext::GetFBInfo(const std::string &fbdev, struct fb_var_screeninfo *fbVar)
{
  int fb = open(fbdev.c_str(), O_RDONLY, 0);
  if (fb < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to open /dev/fb0\n");
    return false;
  }

  int err = ioctl(fb, FBIOGET_VSCREENINFO, fbVar);
  if (err < 0)
    CLog::Log(LOGWARNING, "iMX : Failed to query variable screen info at %s\n", fbdev.c_str());

  close(fb);
  return err >= 0;
}

void CIMXContext::MemMap(struct fb_fix_screeninfo *fb_fix)
{
  if (m_fbVirtAddr && m_fbPhysSize)
  {
    munmap(m_fbVirtAddr, m_fbPhysSize);
    m_fbVirtAddr = NULL;
    m_fbPhysAddr = 0;
  }
  else if (fb_fix)
  {
    m_fbLineLength = fb_fix->line_length;
    m_fbPhysSize = fb_fix->smem_len;
    m_fbPageSize = m_fbLineLength * m_fbVar.yres_virtual / m_fbPages;
    m_fbPhysAddr = fb_fix->smem_start;
    m_fbVirtAddr = (uint8_t*)mmap(0, m_fbPhysSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fbHandle, 0);
    Clear();
  }
}

void CIMXContext::OnLostDisplay()
{
  CSingleLock lk(m_pageSwapLock);
  if (IsRunning())
    m_bFbIsConfigured = false;
}

void CIMXContext::OnResetDisplay()
{
  CSingleLock lk(m_pageSwapLock);
  if (m_bFbIsConfigured)
    return;

  CLog::Log(LOGDEBUG, "iMX : %s - going to change screen parameters\n", __FUNCTION__);
  AdaptScreen();
}

bool CIMXContext::TaskRestart()
{
  CLog::Log(LOGINFO, "iMX : %s - restarting IMX renderer\n", __FUNCTION__);
  // Stop the ipu thread
  Stop();
  MemMap();
  CloseDevices();

  Create();
  return true;
}

void CIMXContext::Dispose()
{
  if (!m_pageCrops)
    return;

  delete[] m_pageCrops;
  m_pageCrops = NULL;
}

void CIMXContext::OpenIPU()
{
  m_ipuHandle = open("/dev/mxc_ipu", O_RDWR, 0);
}

bool CIMXContext::OpenDevices()
{
  m_fbHandle = open(m_deviceName.c_str(), O_RDWR, 0);
  OpenIPU();

  bool opened = m_fbHandle > 0 && m_ipuHandle > 0;
  if (!opened)
    CLog::Log(LOGWARNING, "iMX : Failed to open framebuffer: %s\n", m_deviceName.c_str());

  return opened;
}

void CIMXContext::CloseIPU()
{
  if (m_ipuHandle)
  {
    close(m_ipuHandle);
    m_ipuHandle = 0;
  }
}

void CIMXContext::CloseDevices()
{
  CLog::Log(LOGINFO, "iMX : Closing devices\n");

  if (m_fbHandle)
  {
    close(m_fbHandle);
    m_fbHandle = 0;
  }

  CloseIPU();
}

bool CIMXContext::Blank()
{
  if (!m_fbHandle) return false;

  m_bFbIsConfigured = false;
  return ioctl(m_fbHandle, FBIOBLANK, 1) == 0;
}

bool CIMXContext::Unblank()
{
  if (!m_fbHandle) return false;

  int ret = ioctl(m_fbHandle, FBIOBLANK, FB_BLANK_UNBLANK);
  m_bFbIsConfigured = true;
  return ret == 0;
}

bool CIMXContext::SetVSync(bool enable)
{
  m_vsync = enable;
  return true;
}

inline
void CIMXContext::SetFieldData(uint8_t fieldFmt, double fps)
{
  if (m_bStop || !IsRunning() || !m_bFbIsConfigured)
    return;

  static EINTERLACEMETHOD imPrev;
  bool dr = IsDoubleRate();
  bool deint = !!m_currentFieldFmt;
  m_currentFieldFmt = fieldFmt;

  if (!!fieldFmt != deint ||
      dr != IsDoubleRate()||
      fps != m_fps        ||
      imPrev != CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod)
    m_bFbIsConfigured = false;

  if (m_bFbIsConfigured)
    return;

  m_fps = fps;
  imPrev = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod;
  CLog::Log(LOGDEBUG, "iMX : Output parameters changed - deinterlace %s%s, fps: %.3f\n", !!fieldFmt ? "active" : "not active", IsDoubleRate() ? " DR" : "", m_fps);
  SetIPUMotion(imPrev);

  CSingleLock lk(m_pageSwapLock);
  AdaptScreen();
}

#define MASK1 (IPU_DEINTERLACE_RATE_FRAME1 | RENDER_FLAG_TOP)
#define MASK2 (IPU_DEINTERLACE_RATE_FRAME1 | RENDER_FLAG_BOT)
#define VAL1  MASK1
#define VAL2  RENDER_FLAG_BOT

inline
bool checkIPUStrideOffset(struct ipu_deinterlace *d, bool DR)
{
  switch (d->motion)
  {
  case HIGH_MOTION:
    return ((d->field_fmt & MASK1) == VAL1) || ((d->field_fmt & MASK2) == VAL2);
  case MED_MOTION:
    return DR && !(((d->field_fmt & MASK1) == VAL1) || ((d->field_fmt & MASK2) == VAL2));
  default:
    return true;
  }
}

inline
void CIMXContext::SetIPUMotion(EINTERLACEMETHOD imethod)
{
  std::string strImethod;

  if (m_processInfo)
    m_processInfo->SetVideoDeintMethod("none");

  if (!m_currentFieldFmt || imethod == VS_INTERLACEMETHOD_NONE)
    return;

  switch (imethod)
  {
  case VS_INTERLACEMETHOD_IMX_ADVMOTION:
    strImethod = g_localizeStrings.Get(16337);
    m_motion   = MED_MOTION;
    break;

  case VS_INTERLACEMETHOD_AUTO:
  case VS_INTERLACEMETHOD_IMX_ADVMOTION_HALF:
    strImethod = g_localizeStrings.Get(16335);
    m_motion   = MED_MOTION;
    break;

  case VS_INTERLACEMETHOD_IMX_FASTMOTION:
    strImethod = g_localizeStrings.Get(16334);
    m_motion   = HIGH_MOTION;
    break;

  default:
    strImethod = g_localizeStrings.Get(16021);
    m_motion   = HIGH_MOTION;
    break;
  }

  if (m_processInfo)
    m_processInfo->SetVideoDeintMethod(strImethod);
}

void CIMXContext::Blit(CIMXBuffer *source_p, CIMXBuffer *source, const CRect &srcRect,
                       const CRect &dstRect, uint8_t fieldFmt, int page)
{
  static unsigned char pg;

  if (page == RENDER_TASK_AUTOPAGE)
    page = pg;
  else if (page < 0 && page >= m_fbPages)
    return;

  IPUTaskPtr ipu(new IPUTask(source_p, source, page));
  pg = ++pg % m_fbPages;

#ifdef IMX_PROFILE_BUFFERS
  unsigned long long before = XbmcThreads::SystemClockMillis();
#endif
  SetFieldData(fieldFmt, source->m_fps);
  PrepareTask(ipu, srcRect, dstRect);

  if (DoTask(ipu))
    m_fbCurrentPage = ipu->page | checkIPUStrideOffset(&ipu->task.input.deinterlace, IsDoubleRate()) << 4;

  m_pingFlip.Set();

#ifdef IMX_PROFILE_BUFFERS
  unsigned long long after = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "+P 0x%x@%d  %d\n", ((CDVDVideoCodecIMXBuffer*)ipu->current)->GetIdx(), ipu->page, (int)(after-before));
#endif
}

void CIMXContext::WaitVSync()
{
  m_waitVSync.WaitMSec(1300 / g_graphicsContext.GetFPS());
}

inline
bool CIMXContext::ShowPage()
{
  m_pingFlip.Wait();

  {
    CSingleLock lk(m_pageSwapLock);
    if (!m_bFbIsConfigured)
      return false;
  }

  m_fbVar.yoffset = (m_fbVar.yres + 1) * (m_fbCurrentPage & 0xf) + !(m_fbCurrentPage >> 4);
  if (ioctl(m_fbHandle, FBIOPAN_DISPLAY, &m_fbVar) < 0)
    CLog::Log(LOGWARNING, "Panning failed: %s\n", strerror(errno));

  m_waitVSync.Set();

  if (ioctl(m_fbHandle, MXCFB_WAIT_FOR_VSYNC, nullptr) < 0 && !CIMX::IsBlank())
    CLog::Log(LOGWARNING, "Vsync failed: %s\n", strerror(errno));
}

void CIMXContext::SetProcessInfo(CProcessInfo *m_pProcessInfo)
{
  m_processInfo = m_pProcessInfo;
  if (!m_processInfo)
    return;

  SetIPUMotion(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod);
}

void CIMXContext::Clear(int page)
{
  if (!m_fbVirtAddr) return;

  uint16_t clr = 128 << 8 | 16;
  uint8_t *tmp_buf;
  int bytes;

  if (page < 0)
  {
    tmp_buf = m_fbVirtAddr;
    bytes = m_fbPageSize*m_fbPages;
  }
  else if (page < m_fbPages)
  {
    tmp_buf = m_fbVirtAddr + page*m_fbPageSize;
    bytes = m_fbPageSize;
  }
  else
    // out of range
    return;

  if (m_fbVar.nonstd == _4CC('R', 'G', 'B', '4'))
    memset(tmp_buf, 0, bytes);
  else if (m_fbVar.nonstd == _4CC('Y', 'U', 'Y', 'V'))
    for (int i = 0; i < bytes / 2; ++i, tmp_buf += 2)
      memcpy(tmp_buf, &clr, 2);
  else
    CLog::Log(LOGERROR, "iMX Clear fb error : Unexpected format");

  SetProcessInfo(m_processInfo);
}

void CIMXContext::PrepareTask(IPUTaskPtr &ipu, CRect srcRect, CRect dstRect)
{
  CRectInt iSrcRect, iDstRect;

  float srcWidth = srcRect.Width();
  float srcHeight = srcRect.Height();
  float dstWidth = dstRect.Width();
  float dstHeight = dstRect.Height();

  // Project coordinates outside the target buffer rect to
  // the source rect otherwise the IPU task will fail
  // This is under the assumption that the srcRect is always
  // inside the input buffer rect. If that is not the case
  // it needs to be projected to the output buffer rect as well
  if (dstRect.x1 < 0)
  {
    srcRect.x1 -= dstRect.x1*srcWidth / dstWidth;
    dstRect.x1 = 0;
  }
  if (dstRect.x2 > m_fbWidth)
  {
    srcRect.x2 -= (dstRect.x2-m_fbWidth)*srcWidth / dstWidth;
    dstRect.x2 = m_fbWidth;
  }
  if (dstRect.y1 < 0)
  {
    srcRect.y1 -= dstRect.y1*srcHeight / dstHeight;
    dstRect.y1 = 0;
  }
  if (dstRect.y2 > m_fbHeight)
  {
    srcRect.y2 -= (dstRect.y2-m_fbHeight)*srcHeight / dstHeight;
    dstRect.y2 = m_fbHeight;
  }

  iSrcRect.x1 = Align((int)srcRect.x1,8);
  iSrcRect.y1 = Align((int)srcRect.y1,8);
  iSrcRect.x2 = Align2((int)srcRect.x2,8);
  iSrcRect.y2 = Align2((int)srcRect.y2,16);

  iDstRect.x1 = Align((int)dstRect.x1,8);
  iDstRect.y1 = Align((int)dstRect.y1,8);
  iDstRect.x2 = Align2((int)dstRect.x2,8);
  iDstRect.y2 = Align2((int)dstRect.y2,8);

  ipu->task.input.crop.pos.x  = iSrcRect.x1;
  ipu->task.input.crop.pos.y  = iSrcRect.y1;
  ipu->task.input.crop.w      = iSrcRect.Width();
  ipu->task.input.crop.h      = iSrcRect.Height();

  ipu->task.output.crop.pos.x = iDstRect.x1;
  ipu->task.output.crop.pos.y = iDstRect.y1;
  ipu->task.output.crop.w     = iDstRect.Width();
  ipu->task.output.crop.h     = iDstRect.Height();

  // Setup deinterlacing if enabled
  if (m_currentFieldFmt)
  {
    ipu->task.input.deinterlace.enable = 1;
    ipu->task.input.deinterlace.motion = ipu->previous ? m_motion : HIGH_MOTION;
    ipu->task.input.deinterlace.field_fmt = m_currentFieldFmt;
  }
}

bool CIMXContext::TileTask(IPUTaskPtr &ipu)
{
  m_zoomAllowed = true;

  // on double rate deinterlacing this is reusing previous already rasterised frame
  if (ipu->current->iFormat != _4CC('T', 'N', 'V', 'F') && ipu->current->iFormat != _4CC('T', 'N', 'V', 'P'))
  {
    if (ipu->task.input.deinterlace.enable && ipu->task.input.deinterlace.motion != HIGH_MOTION)
    {
      ipu->task.input.paddr_n = ipu->task.input.paddr;
      ipu->task.input.paddr   = ipu->previous->pPhysAddr;
    }
    return true;
  }

  // Use band mode directly to FB, as no transformations needed (eg cropping)
  if (m_fps >= 49 && m_fbWidth == 1920 && ipu->task.input.width == 1920 && !ipu->task.input.deinterlace.enable)
  {
    m_zoomAllowed = false;
    ipu->task.output.crop.pos.x = ipu->task.input.crop.pos.x = 0;
    ipu->task.output.crop.pos.y = ipu->task.input.crop.pos.y = 0;
    ipu->task.output.crop.h     = ipu->task.input.height     = ipu->task.input.crop.h = ipu->current->iHeight;
    ipu->task.output.crop.w     = ipu->task.input.width      = ipu->task.input.crop.w = ipu->current->iWidth;
    if (ipu->task.input.crop.h < m_fbHeight)
      ipu->task.output.paddr     += m_fbLineLength * (m_fbHeight - ipu->task.input.crop.h)/2;
    return true;
  }

  // check for 3-field deinterlace (no HIGH_MOTION allowed) from tile field format
  if (ipu->previous && ipu->current->iFormat == _4CC('T', 'N', 'V', 'F'))
  {
    ipu->task.input.paddr     = ipu->previous->pPhysAddr;
    ipu->task.input.paddr_n   = ipu->current->pPhysAddr;
    ipu->task.input.deinterlace.field_fmt = IPU_DEINTERLACE_FIELD_TOP;
    ipu->task.input.deinterlace.enable = true;

    ipu->task.output.crop.pos.x = ipu->task.input.crop.pos.x = 0;
    ipu->task.output.crop.pos.y = ipu->task.input.crop.pos.y = 0;
    ipu->task.output.crop.h     = ipu->task.input.height     = ipu->task.input.crop.h = ipu->current->iHeight;
    ipu->task.output.crop.w     = ipu->task.input.width      = ipu->task.input.crop.w = ipu->current->iWidth;

    return CheckTask(ipu) == 0;
  }

  // rasterize from tile (frame)
  struct ipu_task    vdoa;

  memset(&vdoa, 0, sizeof(ipu->task));
  vdoa.input.width   = vdoa.output.width  = ipu->current->iWidth;
  vdoa.input.height  = vdoa.output.height = ipu->current->iHeight;
  vdoa.input.format  = ipu->current->iFormat;

  vdoa.input.paddr   = vdoa.input.paddr_n ? ipu->previous->pPhysAddr : ipu->current->pPhysAddr;
  vdoa.output.format = ipu->task.input.format = m_fbVar.bits_per_pixel == 16 ? _4CC('Y', 'U', 'Y', 'V') : _4CC('N', 'V', '1', '2');

  int check = CheckTask(ipu);
  if (check == IPU_CHECK_ERR_PROC_NO_NEED)
  {
    vdoa.output.paddr = ipu->task.output.paddr;
  }
  else
  {
    struct g2d_buf *conv = g2d_alloc(ipu->current->iWidth *ipu->current->iHeight * 3, 0);
    if (!conv)
    {
      CLog::Log(LOGERROR, "iMX: can't allocate crop buffer");
      return false;
    }

    ((CDVDVideoCodecIMXBuffer*)ipu->current)->m_convBuffer = conv;
    vdoa.output.paddr = conv->buf_paddr;
  }

  if (ioctl(m_ipuHandle, IPU_QUEUE_TASK, &vdoa) < 0)
  {
    int ret = ioctl(m_ipuHandle, IPU_CHECK_TASK, &vdoa);
    CLog::Log(LOGERROR, "IPU conversion from tiled failed %d at #%d", ret, __LINE__);
    return false;
  }

  ipu->task.input.paddr   = vdoa.output.paddr;

  if (ipu->current->iFormat == _4CC('T', 'N', 'V', 'F'))
    return true;

  // output of VDOA task was sent directly to FB. no more processing needed.
  if (check == IPU_CHECK_ERR_PROC_NO_NEED)
    return true;

  if (ipu->task.input.deinterlace.enable && ipu->task.input.deinterlace.motion != HIGH_MOTION)
  {
    ipu->task.input.paddr_n = ipu->task.input.paddr;
    ipu->task.input.paddr   = ipu->previous->pPhysAddr;
  }
  ipu->current->iFormat   = vdoa.output.format;
  ipu->current->pPhysAddr = vdoa.output.paddr;

  return true;
}

int CIMXContext::CheckTask(IPUTaskPtr &ipu)
{
  //We really use IPU only if we have to deinterlace (using VDIC)
  int ret = IPU_CHECK_ERR_INPUT_CROP;
  while (ret > IPU_CHECK_ERR_MIN)
  {
    ret = ioctl(m_ipuHandle, IPU_CHECK_TASK, &ipu->task);
    switch (ret)
    {
      case IPU_CHECK_OK:
        break;
      case IPU_CHECK_ERR_SPLIT_INPUTW_OVER:
        ipu->task.input.crop.w -= 8;
        break;
      case IPU_CHECK_ERR_SPLIT_INPUTH_OVER:
        ipu->task.input.crop.h -= 8;
        break;
      case IPU_CHECK_ERR_SPLIT_OUTPUTW_OVER:
        ipu->task.output.width -= 8;
        ipu->task.output.crop.w = ipu->task.output.width;
        break;
      case IPU_CHECK_ERR_SPLIT_OUTPUTH_OVER:
        ipu->task.output.height -= 8;
        ipu->task.output.crop.h = ipu->task.output.height;
        break;
      // deinterlacing setup changing, m_ipuHandle is closed
      case -1:
      // no IPU processing needed
      case IPU_CHECK_ERR_PROC_NO_NEED:
        return ret;
      default:
        CLog::Log(LOGWARNING, "iMX : unhandled IPU check error: %d", ret);
        return ret;
      }
  }

  return 0;
}

bool CIMXContext::DoTask(IPUTaskPtr &ipu, CRect *dest)
{
  // Clear page if cropping changes
  CRectInt dstRect(ipu->task.output.crop.pos.x, ipu->task.output.crop.pos.y,
                   ipu->task.output.crop.pos.x + ipu->task.output.crop.w,
                   ipu->task.output.crop.pos.y + ipu->task.output.crop.h);

  // Populate input block
  ipu->task.input.width   = ipu->current->iWidth;
  ipu->task.input.height  = ipu->current->iHeight;
  ipu->task.input.format  = ipu->current->iFormat;
  ipu->task.input.paddr   = ipu->current->pPhysAddr;

  ipu->task.output.width  = m_fbWidth;
  ipu->task.output.height = m_fbHeight;
  ipu->task.output.format = m_fbVar.nonstd;
  ipu->task.output.paddr  = m_fbPhysAddr + ipu->page*m_fbPageSize;

  if (m_pageCrops[ipu->page] != dstRect)
  {
    m_pageCrops[ipu->page] = dstRect;
    Clear(ipu->page);
  }

  if ((ipu->task.input.crop.w <= 0) || (ipu->task.input.crop.h <= 0)
  ||  (ipu->task.output.crop.w <= 0) || (ipu->task.output.crop.h <= 0))
    return false;

  if (!TileTask(ipu))
    return false;

  if (CheckTask(ipu) == IPU_CHECK_ERR_PROC_NO_NEED)
    return true;

  int ret = ioctl(m_ipuHandle, IPU_QUEUE_TASK, &ipu->task);
  if (ret < 0)
    CLog::Log(LOGERROR, "IPU task failed: %s at #%d (ret %d)\n", strerror(errno), __LINE__, ret);

  return ret == 0;
}

bool CIMXContext::CaptureDisplay(unsigned char *&buffer, int iWidth, int iHeight, bool blend)
{
  int size = iWidth * iHeight * 4;
  m_bufferCapture = g2d_alloc(size, 0);

  if (!buffer)
    buffer = new uint8_t[size];
  else if (blend)
    std::memcpy(m_bufferCapture->buf_vaddr, buffer, m_bufferCapture->buf_size);

  if (g2d_open(&m_g2dHandle))
  {
    CLog::Log(LOGERROR, "%s : Error while trying open G2D\n", __FUNCTION__);
    return false;
  }

  CSingleLock lk(m_pageSwapLock);
  if (m_bufferCapture && buffer)
  {
    struct g2d_surface src, dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    {
      src.planes[0] = m_fbPhysAddr + (m_fbCurrentPage & 0xf) * m_fbPageSize;
      dst.planes[0] = m_bufferCapture->buf_paddr;
      if (m_fbVar.bits_per_pixel == 16)
      {
        src.format = G2D_YUYV;
        src.planes[1] = src.planes[0] + Align(m_fbWidth * m_fbHeight, 64);
        src.planes[2] = src.planes[1] + Align((m_fbWidth * m_fbHeight) / 2, 64);
      }
      else
      {
        src.format = G2D_RGBX8888;
      }

      dst.left = dst.top = src.top = src.left = 0;
      src.stride = src.right = src.width = m_fbWidth;
      src.bottom = src.height = m_fbHeight;

      dst.right = dst.stride = dst.width = iWidth;
      dst.bottom = dst.height = iHeight;

      dst.rot = src.rot = G2D_ROTATION_0;
      dst.format = G2D_BGRA8888;

      if (blend)
      {
        src.blendfunc = G2D_ONE_MINUS_DST_ALPHA;
        dst.blendfunc = G2D_ONE;
        g2d_enable(m_g2dHandle, G2D_BLEND);
      }

      g2d_blit(m_g2dHandle, &src, &dst);
      g2d_finish(m_g2dHandle);
      g2d_disable(m_g2dHandle, G2D_BLEND);
    }

    std::memcpy(buffer, m_bufferCapture->buf_vaddr, m_bufferCapture->buf_size);
  }
  else
    CLog::Log(LOGERROR, "iMX : Error allocating capture buffer\n");

  if (m_bufferCapture && g2d_free(m_bufferCapture))
    CLog::Log(LOGERROR, "iMX : Error while freeing capture buffer\n");

  if (m_g2dHandle && !g2d_close(m_g2dHandle))
    m_g2dHandle = NULL;
  return true;
}

void CIMXContext::Allocate()
{
  CSingleLock lk(m_pageSwapLock);
  AdaptScreen(true);
  AdaptScreen();
  Create();
}

void CIMXContext::OnStartup()
{
  CSingleLock lk(m_pageSwapLock);
  g_Windowing.Register(this);
  CLog::Log(LOGNOTICE, "iMX : IPU thread started");
}

void CIMXContext::OnExit()
{
  CSingleLock lk(m_pageSwapLock);
  g_Windowing.Unregister(this);
  CLog::Log(LOGNOTICE, "iMX : IPU thread terminated");
}

void CIMXContext::Stop(bool bWait /*= true*/)
{
  if (!IsRunning())
    return;

  CThread::StopThread(false);
  m_pingFlip.Set();
  if (bWait && IsRunning())
    CThread::StopThread(true);
}

void CIMXContext::Process()
{
  while (!m_bStop)
    ShowPage();
}
