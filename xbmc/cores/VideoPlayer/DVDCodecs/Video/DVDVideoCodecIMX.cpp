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
#include "utils/StringUtils.h"
#include "settings/MediaSettings.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

#include "linux/imx/IMX.h"
#include "libavcodec/avcodec.h"

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

#ifdef TRACE_FRAMES
unsigned char CDVDVideoCodecIMXBuffer::i = 0;
#endif

CIMXContext   g_IMXContext;
std::shared_ptr<CIMXCodec> g_IMXCodec;

std::list<VpuFrameBuffer*> m_recycleBuffers;

// Number of fb pages used for paning
const int CIMXContext::m_fbPages = 3;

// Experiments show that we need at least one more (+1) VPU buffer than the min value returned by the VPU
const unsigned int CIMXCodec::m_extraVpuBuffers = 2 + RENDER_QUEUE_SIZE;

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

  return g_IMXCodec->Open(hints, options, m_pFormatName);
}

unsigned CDVDVideoCodecIMX::GetAllowedReferences()
{
  return RENDER_QUEUE_SIZE;
}

bool CDVDVideoCodecIMX::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (pDvdVideoPicture)
    SAFE_RELEASE(pDvdVideoPicture->IMXBuffer);

  return true;
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
    { // Allocate contigous mem for DMA
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
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "VPU Lib version : major.minor.rel=%d.%d.%d.\n", vpuVersion.nLibMajor, vpuVersion.nLibMinor, vpuVersion.nLibRelease);
  }

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
{
  m_vpuHandle = 0;
  m_converter = NULL;
#ifdef DUMP_STREAM
  m_dump = NULL;
#endif
  m_drainMode = VPU_DEC_IN_NORMAL;
  m_skipMode = VPU_DEC_SKIPNONE;

  m_decOutput.setquotasize(1);
  m_decInput.setquotasize(20);
  m_loaded.Reset();
}

CIMXCodec::~CIMXCodec()
{
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
}

void CIMXCodec::Reset()
{
  m_queuesLock.lock();
  DisposeDecQueues();
  ProcessSignals(SIGNAL_FLUSH);
  CLog::Log(LOGDEBUG, "iMX VPU : queues cleared ===== in/out %d/%d =====\n", m_decInput.size(), m_decOutput.size());
}

bool CIMXCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options, std::string &m_pFormatName)
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

  m_hints = hints;
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "Let's decode with iMX VPU\n");

  int param = 0;
  SetVPUParams(VPU_DEC_CONF_INPUTTYPE, &param);
  SetVPUParams(VPU_DEC_CONF_SKIPMODE, &param);

#ifdef MEDIAINFO
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
  {
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: fpsrate %d / fpsscale %d\n", m_hints.fpsrate, m_hints.fpsscale);
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: CodecID %d \n", m_hints.codec);
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: StreamType %d \n", m_hints.type);
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: Level %d \n", m_hints.level);
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: Profile %d \n", m_hints.profile);
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: PTS_invalid %d \n", m_hints.ptsinvalid);
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: Tag %d \n", m_hints.codec_tag);
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: %dx%d \n", m_hints.width,  m_hints.height);
  }
  { char str_tag[128]; av_get_codec_tag_string(str_tag, sizeof(str_tag), m_hints.codec_tag);
      CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: Tag fourcc %s\n", str_tag);
  }
  if (m_hints.extrasize)
  {
    char buf[4096];

    for (unsigned int i=0; i < m_hints.extrasize; i++)
      sprintf(buf+i*2, "%02x", ((uint8_t*)m_hints.extradata)[i]);

    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: extradata %d %s\n", m_hints.extrasize, buf);
  }
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
  {
    CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: %d / %d \n", m_hints.width,  m_hints.height);
    CLog::Log(LOGDEBUG, "Decode: aspect %f - forced aspect %d\n", m_hints.aspect, m_hints.forced_aspect);
  }
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
    if (hints.extradata)
    {
      if ( *(char*)hints.extradata == 1 )
      {
        m_converter = new CBitstreamConverter();
        if (!m_converter->Open(hints.codec, (uint8_t *)m_hints.extradata, m_hints.extrasize, true))
        {
          SAFE_DELETE(m_converter);
          break;
        }
        std::free(m_hints.extradata);
        m_hints.extrasize = m_converter->GetExtraSize();
        m_hints.extradata = std::malloc(m_hints.extrasize);
        std::memcpy(m_hints.extradata, m_converter->GetExtraData(), m_hints.extrasize);
      }
    }
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
      CLog::Log(LOGVIDEO, "%s - VPU closed.", __FUNCTION__);

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

  //SetSkipMode(m_codecControlFlags & DVD_CODEC_CTRL_DROP ? VPU_DEC_ISEARCH : VPU_DEC_SKIPNONE);
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
  if (EOS() && m_drainMode && !m_decOutput.size())
    return VC_BUFFER;

  int ret = 0;
  if (!g_IMXCodec->IsRunning())
  {
    static double pattern;
    if (!m_decInput.full())
    {
      ret |= VC_BUFFER;
      if (dts != DVD_NOPTS_VALUE)
      {
        static double last;
        if (last != 0.0)
          pattern = (pattern + dts - last) / 2;
        last = dts;
      }
    }
    else
    {
      if (pattern > 0.01)
        m_fps = (double)DVD_TIME_BASE/pattern;
      else
        m_fps = m_hints.fpsscale ? (double)m_hints.fpsrate / m_hints.fpsscale : 60;
      m_decOpenParam.nMapType = 1;

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
    if (!(m_codecControlFlags & DVD_CODEC_CTRL_HURRY) && (m_decInput.size() < m_decInput.getquotasize() /2))
      return ret;
  }

  if ((m_decOutput.size() >= m_decOutput.getquotasize() /2 ||
       m_drainMode || (m_codecControlFlags & DVD_CODEC_CTRL_HURRY)) && m_decOutput.size())
    ret |= VC_PICTURE;

#ifdef IMX_PROFILE
  CLog::Log(LOGVIDEO, "%s - demux size: %d  dts : %f - pts : %f - addr : 0x%x, return %d ===== in/out %d/%d =====\n",
                       __FUNCTION__, iSize, recalcPts(dts), recalcPts(pts), (uint)pData, ret, m_decInput.size(), m_decOutput.size());
#endif

  if (!ret || m_drainMode)
    Sleep(5);

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

  m_recycleBuffers.clear();
  m_pts.clear();
  m_loaded.Set();

  memset(&dummy, 0, sizeof(dummy));
  AddExtraData(&dummy);
  inData = dummy;

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
    CLog::Log(LOGVIDEO, "%s - delta time decode : %llu - demux size : %d  dts : %f - pts : %f - addr : 0x%x\n",
                                      __FUNCTION__, current - previous, task->demux.iSize, recalcPts(task->demux.dts), recalcPts(task->demux.pts), (uint)task->demux.pData);
    previous = current;
#endif
#endif

    inData.nSize = task->demux.iSize;
    inData.pPhyAddr = NULL;
    inData.pVirAddr = task->demux.pData;

    // some streams have problem with getting intial info after seek into (during playback start).
    // feeding VPU with extra data helps
    if (!m_vpuFrameBuffers.size() && m_converter && !task->IsEmpty() && m_decRet & VPU_DEC_NO_ENOUGH_INBUF)
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
        m_decOutput.setquotasize(1);
        if (m_decRet & VPU_DEC_RESOLUTION_CHANGED && m_decOutput.size())
        {
          int returning = m_decOutput.size() + RENDER_QUEUE_SIZE;
          while (m_recycleBuffers.size() < returning)
            std::this_thread::yield();
        }

        if (VPU_DecGetInitialInfo(m_vpuHandle, &m_initInfo) != VPU_DEC_RET_SUCCESS)
          ExitError("VPU get initial info failed");

        CLog::Log(LOGDEBUG, "%s - VPU Init Stream Info : %dx%d (interlaced : %d - Minframe : %d)"\
                  " - Align : %d bytes - crop : %d %d %d %d - Q16Ratio : %x\n", __FUNCTION__,
          m_initInfo.nPicWidth, m_initInfo.nPicHeight, m_initInfo.nInterlace, m_initInfo.nMinFrameBufferCount,
          m_initInfo.nAddressAlignment, m_initInfo.PicCropRect.nLeft, m_initInfo.PicCropRect.nTop,
          m_initInfo.PicCropRect.nRight, m_initInfo.PicCropRect.nBottom, m_initInfo.nQ16ShiftWidthDivHeightRatio);

        if (!VpuFreeBuffers(false) || !VpuAllocFrameBuffers())
          ExitError("VPU error while registering frame buffers");

        if (m_initInfo.nInterlace && m_fps >= 50 && !m_converter && m_decOpenParam.nMapType == 1)
        {
          m_decOpenParam.nMapType = 0;
          Dispose();
          VpuOpen();
          continue;
        }

        m_decInput.setquotasize(m_initInfo.nMinFrameBufferCount*7);

        if (m_decOpenParam.CodecFormat != VPU_V_MPEG2)
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
        // Some codecs (VC1?) lie about their frame size (mod 16). Adjust...
        m_frameInfo.pExtInfo->nFrmWidth  = (((m_frameInfo.pExtInfo->nFrmWidth) + 15) & ~15);
        m_frameInfo.pExtInfo->nFrmHeight = (((m_frameInfo.pExtInfo->nFrmHeight) + 15) & ~15);

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
        m_dropped++;

      if (m_decRet & VPU_DEC_SKIP)
        m_dropped++;

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
  return;
  if (bDrop)
  {
    if (m_decOutput.size() && !m_bStop)
    {
      m_decOutput.pop()->Release();
      ++m_dropped;
    }
    else if (m_skipMode < 1 && m_speed != DVD_PLAYSPEED_PAUSE)
      SetSkipMode((VpuDecSkipMode)IN_DECODER_SET);
  }
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
  iWidth      = frameInfo->pExtInfo->nFrmWidth;
  iHeight     = frameInfo->pExtInfo->nFrmHeight;
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
#ifdef TRACE_FRAMES
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
  m_input.clear();
  m_input.setquotasize(m_fbPages);
  m_pageCrops = new CRectInt[m_fbPages];
  CLog::Log(LOGDEBUG, "iMX : Allocated %d render buffers\n", m_fbPages);

  SetBlitRects(CRectInt(), CRectInt());

  g2dOpenDevices();
  Create();
}

CIMXContext::~CIMXContext()
{
  Stop(false);
  Dispose();
  CloseDevices();
  g2dCloseDevices();
}


bool CIMXContext::AdaptScreen(bool allocate)
{

  if(m_ipuHandle)
  {
    close(m_ipuHandle);
    m_ipuHandle = 0;
  }

  MemMap();

  if(!m_fbHandle && !OpenDevices())
    goto Err;

  struct fb_var_screeninfo fbVar;
  if (!GetFBInfo("/dev/fb0", &fbVar))
    goto Err;

  CLog::Log(LOGNOTICE, "iMX : Changing framebuffer parameters\n");

  m_fbWidth = allocate ? 1920 : fbVar.xres;
  m_fbHeight = allocate ? 1080 : fbVar.yres;

  if (!GetFBInfo(m_deviceName, &m_fbVar))
    goto Err;

  m_fbVar.xoffset = 0;
  m_fbVar.yoffset = 0;

  if (!allocate && (fbVar.bits_per_pixel == 16 || m_currentFieldFmt || m_fbHeight >= 1080 && m_fps >= 49))
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

  if (m_fbVar.bits_per_pixel == 16 || !RENDER_USE_G2D)
    m_ipuHandle = open("/dev/mxc_ipu", O_RDWR, 0);

  Unblank();

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
    m_fbCurrentPage = 0;
    Clear();
  }
}

void CIMXContext::OnResetDisplay()
{
  CSingleLock lk(m_pageSwapLock);

  CLog::Log(LOGDEBUG, "iMX : %s - going to change screen parameters\n", __FUNCTION__);
  m_bFbIsConfigured = false;
  AdaptScreen();
}

bool CIMXContext::TaskRestart()
{
  CLog::Log(LOGINFO, "iMX : %s - restarting IMX rendererer\n", __FUNCTION__);
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

bool CIMXContext::OpenDevices()
{
  m_fbHandle = open(m_deviceName.c_str(), O_RDWR, 0);
  if (m_fbHandle < 0)
  {
    m_fbHandle = 0;
    CLog::Log(LOGWARNING, "iMX : Failed to open framebuffer: %s\n", m_deviceName.c_str());
  }

  return m_fbHandle > 0;
}

void CIMXContext::g2dOpenDevices()
{
  // open g2d here to ensure all g2d fucntions are called from the same thread
  if (!g2d_open(&m_g2dHandle))
    return;

  m_g2dHandle = NULL;
  CLog::Log(LOGERROR, "%s - Error while trying open G2D\n", __FUNCTION__);
}

void CIMXContext::g2dCloseDevices()
{
  // close g2d here to ensure all g2d fucntions are called from the same thread
  if (m_bufferCapture && !g2d_free(m_bufferCapture))
    m_bufferCapture = NULL;

  if (m_g2dHandle && !g2d_close(m_g2dHandle))
    m_g2dHandle = NULL;
}

void CIMXContext::CloseDevices()
{
  CLog::Log(LOGINFO, "iMX : Closing devices\n");

  if (m_fbHandle)
  {
    close(m_fbHandle);
    m_fbHandle = 0;
  }

  if (m_ipuHandle)
  {
    close(m_ipuHandle);
    m_ipuHandle = 0;
  }
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

void CIMXContext::SetBlitRects(const CRect &srcRect, const CRect &dstRect)
{
  m_srcRect = srcRect;
  m_dstRect = dstRect;
}

inline
void CIMXContext::SetFieldData(uint8_t fieldFmt, double fps)
{
  if (m_bStop || !IsRunning())
    return;

  bool dr = IsDoubleRate();
  bool deint = !!m_currentFieldFmt;
  m_currentFieldFmt = fieldFmt;

  if (!!fieldFmt != deint ||
      dr != IsDoubleRate()||
      fps != m_fps)
    m_bFbIsConfigured = false;

  if (m_bFbIsConfigured)
    return;

  m_fps = fps;
  CLog::Log(LOGDEBUG, "iMX : Output parameters changed - deinterlace %s%s, fps: %.3f\n", !!fieldFmt ? "active" : "not active", IsDoubleRate() ? " DR" : "", m_fps);

  CSingleLock lk(m_pageSwapLock);
  AdaptScreen();
}

#define MASK1 (IPU_DEINTERLACE_RATE_FRAME1 | RENDER_FLAG_TOP)
#define MASK2 (IPU_DEINTERLACE_RATE_FRAME1 | RENDER_FLAG_BOT)
#define VAL1  MASK1
#define VAL2  RENDER_FLAG_BOT

inline
bool checkIPUStrideOffset(struct ipu_deinterlace *d)
{
  return ((d->field_fmt & MASK1) == VAL1) ||
         ((d->field_fmt & MASK2) == VAL2);
}

void CIMXContext::Blit(CIMXBuffer *source_p, CIMXBuffer *source, uint8_t fieldFmt, int page, CRect *dest)
{
  static int pg;

  if (page == RENDER_TASK_AUTOPAGE)
    page = pg;
  else if (page == RENDER_TASK_CAPTURE)
    m_CaptureDone = false;
  else if (page < 0 && page >= m_fbPages)
    return;

  pg = ++pg % m_fbPages;

  IPUTask *ipu = new IPUTask;

  SetFieldData(fieldFmt, source->m_fps);
  PrepareTask(ipu, source_p, source, dest);

  ipu->page = page;
#ifdef IMX_PROFILE_BUFFERS
  unsigned long long before = XbmcThreads::SystemClockMillis();
#endif
  if (!DoTask(ipu))
  {
    delete ipu;
    return;
  }
#ifdef IMX_PROFILE_BUFFERS
  unsigned long long after = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGVIDEO, "+P 0x%x@%d  %d\n", ((CDVDVideoCodecIMXBuffer*)ipu->current)->GetIdx(), ipu->page, (int)(after-before));
#endif

  CSingleLock lk(m_pageSwapLock);
  if (ipu->task.output.width)
    m_input.push(ipu);
  else
    delete ipu;
}

bool CIMXContext::PushCaptureTask(CIMXBuffer *source, CRect *dest)
{
  Blit(NULL, source, RENDER_TASK_CAPTURE, 0, dest);
  return true;
}

bool CIMXContext::ShowPage(int page, bool shift)
{
  if (!m_fbHandle || !m_bFbIsConfigured) return false;

  // Protect page swapping from screen capturing that does read the current
  // front buffer. This is actually not done very frequently so the lock
  // does not hurt.

  CSingleLock lk(m_pageSwapLock);

  m_fbCurrentPage = page;
  m_fbVar.activate = FB_ACTIVATE_VBL;
  m_fbVar.yoffset = (m_fbVar.yres + 1) * page + !shift;
  if (ioctl(m_fbHandle, FBIOPAN_DISPLAY, &m_fbVar) < 0)
  {
    CLog::Log(LOGWARNING, "Panning failed: %s\n", strerror(errno));
    return false;
  }

  // Wait for flip
  if (m_vsync && ioctl(m_fbHandle, FBIO_WAITFORVSYNC, 0) < 0)
  {
    CLog::Log(LOGWARNING, "Vsync failed: %s\n", strerror(errno));
    return false;
  }

  return true;
}

void CIMXContext::Clear(int page)
{
  if (!m_fbVirtAddr) return;

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
  {
    uint16_t clr = 128 << 8 | 16;
    int pixels = bytes / 2;
    for (int i = 0; i < pixels; ++i, tmp_buf += 2)
      memcpy(tmp_buf, &clr, 2);
  }
  else
    CLog::Log(LOGERROR, "iMX Clear fb error : Unexpected format");
}

#define clamp_byte(x) (x<0?0:(x>255?255:x))

void CIMXContext::CaptureDisplay(unsigned char *buffer, int iWidth, int iHeight)
{
  if ((m_fbVar.nonstd != _4CC('R', 'G', 'B', '4')) &&
      (m_fbVar.nonstd != _4CC('U', 'Y', 'V', 'Y')))
  {
    CLog::Log(LOGWARNING, "iMX : Unknown screen capture format\n");
    return;
  }

  // Prevent page swaps
  CSingleLock lk(m_pageSwapLock);
  if (m_fbCurrentPage < 0 || m_fbCurrentPage >= m_fbPages)
  {
    CLog::Log(LOGWARNING, "iMX : Invalid page to capture\n");
    return;
  }
  unsigned char *display = m_fbVirtAddr + m_fbCurrentPage*m_fbPageSize;

  if (m_fbVar.nonstd == _4CC('R', 'G', 'B', '4'))
  {
    memcpy(buffer, display, iWidth * iHeight * 4);
    // BGRA is needed RGBA we get
    unsigned int size = iWidth * iHeight * 4;
    for (unsigned int i = 0; i < size; i += 4)
    {
       std::swap(buffer[i], buffer[i + 2]);
    }
  }
  else //_4CC('U', 'Y', 'V', 'Y')))
  {
    int r,g,b,a;
    int u, y0, v, y1;
    int iStride = m_fbWidth*2;
    int oStride = iWidth*4;

    int cy  =  1*(1 << 16);
    int cr1 =  1.40200*(1 << 16);
    int cr2 = -0.71414*(1 << 16);
    int cr3 =  0*(1 << 16);
    int cb1 =  0*(1 << 16);
    int cb2 = -0.34414*(1 << 16);
    int cb3 =  1.77200*(1 << 16);

    iWidth = std::min(iWidth/2, m_fbWidth/2);
    iHeight = std::min(iHeight, m_fbHeight);

    for (int y = 0; y < iHeight; ++y, display += iStride, buffer += oStride)
    {
      unsigned char *iLine = display;
      unsigned char *oLine = buffer;

      for (int x = 0; x < iWidth; ++x, iLine += 4, oLine += 8 )
      {
        u  = iLine[0]-128;
        y0 = iLine[1]-16;
        v  = iLine[2]-128;
        y1 = iLine[3]-16;

        a = 255-oLine[3];
        r = (cy*y0 + cb1*u + cr1*v) >> 16;
        g = (cy*y0 + cb2*u + cr2*v) >> 16;
        b = (cy*y0 + cb3*u + cr3*v) >> 16;

        oLine[0] = (clamp_byte(b)*a + oLine[0]*oLine[3])/255;
        oLine[1] = (clamp_byte(g)*a + oLine[1]*oLine[3])/255;
        oLine[2] = (clamp_byte(r)*a + oLine[2]*oLine[3])/255;
        oLine[3] = 255;

        a = 255-oLine[7];
        r = (cy*y0 + cb1*u + cr1*v) >> 16;
        g = (cy*y0 + cb2*u + cr2*v) >> 16;
        b = (cy*y0 + cb3*u + cr3*v) >> 16;

        oLine[4] = (clamp_byte(b)*a + oLine[4]*oLine[7])/255;
        oLine[5] = (clamp_byte(g)*a + oLine[5]*oLine[7])/255;
        oLine[6] = (clamp_byte(r)*a + oLine[6]*oLine[7])/255;
        oLine[7] = 255;
      }
    }
  }
}

void CIMXContext::WaitCapture()
{
}

void CIMXContext::PrepareTask(IPUTask *ipu, CIMXBuffer *source_p, CIMXBuffer *source,
                              CRect *dest)
{
  // Fill with zeros
  ipu->Zero();
  ipu->previous = source_p;
  ipu->current = source;

  CRect srcRect = m_srcRect;
  CRect dstRect;
  if (dest == NULL)
    dstRect = m_dstRect;
  else
    dstRect = *dest;

  CRectInt iSrcRect, iDstRect;

  float srcWidth = srcRect.Width();
  float srcHeight = srcRect.Height();
  float dstWidth = dstRect.Width();
  float dstHeight = dstRect.Height();

  // Project coordinates outside the target buffer rect to
  // the source rect otherwise the IPU task will fail
  // This is under the assumption that the srcRect is always
  // inside the input buffer rect. If that is not the case
  // it needs to be projected to the ouput buffer rect as well
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
  iSrcRect.y2 = Align2((int)srcRect.y2,8);

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

  // If dest is set it means we do not want to blit to frame buffer
  // but to a capture buffer and we state this capture buffer dimensions
  if (dest)
  {
    // Populate partly output block
    ipu->task.output.crop.pos.x = 0;
    ipu->task.output.crop.pos.y = 0;
    ipu->task.output.crop.w     = iDstRect.Width();
    ipu->task.output.crop.h     = iDstRect.Height();
    ipu->task.output.width  = iDstRect.Width();
    ipu->task.output.height = iDstRect.Height();
  }
  else
  {
  // Setup deinterlacing if enabled
  if (m_currentFieldFmt)
  {
    ipu->task.input.deinterlace.enable = 1;
    ipu->task.input.deinterlace.motion = setIPUMotion(ipu->previous, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod);
    ipu->task.input.deinterlace.field_fmt = m_currentFieldFmt;
  }
  }
}

bool CIMXContext::TileTask(IPUTask *ipu)
{
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
  if (m_fps >= 49 && m_fbWidth == 1920 && ipu->task.input.width == 1920 && !ipu->task.input.deinterlace.enable && m_CaptureDone)
  {
    ipu->task.output.crop.pos.x = ipu->task.input.crop.pos.x = 0;
    ipu->task.output.crop.pos.y = ipu->task.input.crop.pos.y = 0;
    ipu->task.output.crop.h     = ipu->task.input.crop.h = ipu->current->iHeight;
    ipu->task.output.paddr     += m_fbLineLength * (m_fbHeight - ipu->task.input.crop.h)/2;

    return true;
  }

  // rasterize from tile (frame)
  struct ipu_task    vdoa;

  memset(&vdoa, 0, sizeof(ipu->task));
  vdoa.input.width   = vdoa.output.width  = ipu->current->iWidth;
  vdoa.input.height  = vdoa.output.height = ipu->current->iHeight;
  vdoa.input.format  = ipu->current->iFormat;

  // check for 3-field deinterlace (no HIGH_MOTION allowed) from tile field format
  if (ipu->previous && ipu->current->iFormat == _4CC('T', 'N', 'V', 'F'))
  {
    memcpy(&vdoa.input.deinterlace, &ipu->task.input.deinterlace, sizeof(ipu->task.input.deinterlace));
    memset(&ipu->task.input.deinterlace, 0, sizeof(ipu->task.input.deinterlace));
    vdoa.input.paddr_n = ipu->current->pPhysAddr;
  }

  struct g2d_buf *conv = g2d_alloc(ipu->current->iWidth *ipu->current->iHeight * 2, 0);
  if (!conv)
  {
    CLog::Log(LOGERROR, "iMX: can't allocate crop buffer");
    return false;
  }

  ((CDVDVideoCodecIMXBuffer*)ipu->current)->m_convBuffer = conv;

  vdoa.input.paddr   = vdoa.input.paddr_n ? ipu->previous->pPhysAddr : ipu->current->pPhysAddr;
  vdoa.output.format = m_fbVar.bits_per_pixel == 16 && m_CaptureDone ? _4CC('Y', 'U', 'Y', 'V') : _4CC('N', 'V', '1', '2');
  vdoa.output.paddr  = conv->buf_paddr;

  if (int ret = ioctl(m_ipuHandle, IPU_CHECK_TASK, &vdoa))
  {
    CLog::Log(LOGERROR, "IPU conversion from tiled failed %d at #%d", ret, __LINE__);
    return false;
  }
  if (ioctl(m_ipuHandle, IPU_QUEUE_TASK, &vdoa) < 0)
    return false;

  ipu->task.input.paddr  = vdoa.output.paddr;
  ipu->task.input.format = vdoa.output.format;
  if (ipu->task.input.deinterlace.enable && ipu->task.input.deinterlace.motion != HIGH_MOTION && ipu->previous)
  {
    ipu->task.input.paddr_n = ipu->task.input.paddr;
    ipu->task.input.paddr   = ipu->previous->pPhysAddr;
  }
  ipu->current->iFormat   = vdoa.output.format;
  ipu->current->pPhysAddr = vdoa.output.paddr;

  return true;
}

bool CIMXContext::DoTask(IPUTask *ipu)
{
  bool swapColors = false;

  // Clear page if cropping changes
  CRectInt dstRect(ipu->task.output.crop.pos.x, ipu->task.output.crop.pos.y,
                   ipu->task.output.crop.pos.x + ipu->task.output.crop.w,
                   ipu->task.output.crop.pos.y + ipu->task.output.crop.h);

  // Populate input block
  ipu->task.input.width   = ipu->current->iWidth;
  ipu->task.input.height  = ipu->current->iHeight;
  ipu->task.input.format  = ipu->current->iFormat;
  ipu->task.input.paddr   = ipu->current->pPhysAddr;

  // Populate output block if it has not already been filled
  if (ipu->task.output.width == 0)
  {
    ipu->task.output.width  = m_fbWidth;
    ipu->task.output.height = m_fbHeight;
    ipu->task.output.format = m_fbVar.nonstd;
    ipu->task.output.paddr  = m_fbPhysAddr + ipu->page*m_fbPageSize;

    if (m_pageCrops[ipu->page] != dstRect)
    {
      m_pageCrops[ipu->page] = dstRect;
      Clear(ipu->page);
    }
  }
  else
  {
    // If we have already set dest dimensions we want to use capture buffer
    // Note we allocate this capture buffer as late as this function because
    // all g2d functions have to be called from the same thread
    int size = ipu->task.output.width * ipu->task.output.height * 4;
    if ((m_bufferCapture) && (size != m_bufferCapture->buf_size))
    {
      if (g2d_free(m_bufferCapture))
        CLog::Log(LOGERROR, "iMX : Error while freeing capture buuffer\n");
      m_bufferCapture = NULL;
    }

    if (m_bufferCapture == NULL)
    {
      m_bufferCapture = g2d_alloc(size, 0);
      if (m_bufferCapture == NULL)
        CLog::Log(LOGERROR, "iMX : Error allocating capture buffer\n");
    }
    ipu->task.output.paddr = m_bufferCapture->buf_paddr;
    swapColors = true;
  }

  if ((ipu->task.input.crop.w <= 0) || (ipu->task.input.crop.h <= 0)
  ||  (ipu->task.output.crop.w <= 0) || (ipu->task.output.crop.h <= 0))
    return false;

  if (!TileTask(ipu))
    return false;

  if (m_CaptureDone && (m_fbVar.bits_per_pixel == 16 || !RENDER_USE_G2D))
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
            ipu->task.output.crop.w -= 8;
            break;
        case IPU_CHECK_ERR_SPLIT_OUTPUTH_OVER:
            ipu->task.output.crop.h -= 8;
            break;
        // deinterlacing setup changing, m_ipuHandle is closed
        case -1:
            return true;
        default:
            CLog::Log(LOGWARNING, "iMX : unhandled IPU check error: %d\n", ret);
            return false;
        }
    }

    ret = ioctl(m_ipuHandle, IPU_QUEUE_TASK, &ipu->task);
    if (ret < 0)
    {
        CLog::Log(LOGERROR, "IPU task failed: %s at #%d\n", strerror(errno), __LINE__);
        return false;
    }
  }
  else
  {
    // deinterlacing is not required, let's use g2d instead of IPU

    struct g2d_surface src, dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    {
      if (ipu->current->iFormat == _4CC('I', '4', '2', '0'))
      {
        src.format = G2D_I420;
        src.planes[0] = ipu->current->pPhysAddr;
        src.planes[1] = src.planes[0] + Align(ipu->current->iWidth * ipu->current->iHeight, 64);
        src.planes[2] = src.planes[1] + Align((ipu->current->iWidth * ipu->current->iHeight) / 2, 64);
      }
      else //_4CC('N', 'V', '1', '2');
      {
        src.format = G2D_NV12;
        src.planes[0] = ipu->current->pPhysAddr;
        src.planes[1] =  src.planes[0] + Align(ipu->current->iWidth * ipu->current->iHeight, 64);
      }

      src.left = ipu->task.input.crop.pos.x;
      src.right = ipu->task.input.crop.w + src.left ;
      src.top  = ipu->task.input.crop.pos.y;
      src.bottom = ipu->task.input.crop.h + src.top;
      src.stride = ipu->current->iWidth;
      src.width  = ipu->current->iWidth;
      src.height = ipu->current->iHeight;
      src.rot = G2D_ROTATION_0;

      dst.planes[0] = ipu->task.output.paddr;
      dst.left = ipu->task.output.crop.pos.x;
      dst.top = ipu->task.output.crop.pos.y;
      dst.right = ipu->task.output.crop.w + dst.left;
      dst.bottom = ipu->task.output.crop.h + dst.top;

      dst.stride = ipu->task.output.width;
      dst.width = ipu->task.output.width;
      dst.height = ipu->task.output.height;
      dst.rot = G2D_ROTATION_0;
      dst.format = swapColors ? G2D_BGRA8888 : G2D_RGBA8888;

      // Launch synchronous blit
      g2d_blit(m_g2dHandle, &src, &dst);
      g2d_finish(m_g2dHandle);
      if ((m_bufferCapture) && (ipu->task.output.paddr == m_bufferCapture->buf_paddr))
        m_CaptureDone = true;
    }
  }

  return true;
}

void CIMXContext::OnStartup()
{
  OpenDevices();

  AdaptScreen(true);
  AdaptScreen();

  g_Windowing.Register(this);
  CLog::Log(LOGNOTICE, "iMX : IPU thread started");
}

void CIMXContext::OnExit()
{
  g_Windowing.Unregister(this);
  CLog::Log(LOGNOTICE, "iMX : IPU thread terminated");
}

void CIMXContext::Stop(bool bWait /*= true*/)
{
  if (!IsRunning())
    return;

  CThread::StopThread(false);
  m_input.signal();
  Blank();
  if (bWait && IsRunning())
    CThread::StopThread(true);
}

void CIMXContext::Process()
{
  while (!m_bStop)
  {
    IPUTask *ipu = m_input.pop();

    if (!ipu)
      continue;

    ipu->shift = checkIPUStrideOffset(&ipu->task.input.deinterlace);

    // Show back buffer
    ShowPage(ipu->page, ipu->shift);

    delete ipu;
  }
}
