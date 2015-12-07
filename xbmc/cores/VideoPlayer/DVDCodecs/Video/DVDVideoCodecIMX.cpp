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
#include "threads/Atomics.h"
#include "utils/log.h"
#include "DVDClock.h"

#include <cassert>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <algorithm>

#define IMX_VDI_MAX_WIDTH 968
#define FRAME_ALIGN 16
#define MEDIAINFO 1
#define RENDER_QUEUE_SIZE 3
#define _4CC(c1,c2,c3,c4) (((uint32_t)(c4)<<24)|((uint32_t)(c3)<<16)|((uint32_t)(c2)<<8)|(uint32_t)(c1))
#define Align(ptr,align)  (((unsigned int)ptr + (align) - 1)/(align)*(align))
#define Align2(ptr,align)  (((unsigned int)ptr)/(align)*(align))


// Global instance
CIMXContext g_IMXContext;

// Number of fb pages used for paning
const int CIMXContext::m_fbPages = 2;

// Experiments show that we need at least one more (+1) VPU buffer than the min value returned by the VPU
const int CDVDVideoCodecIMX::m_extraVpuBuffers = 1+RENDER_QUEUE_SIZE+2;
const int CDVDVideoCodecIMX::m_maxVpuDecodeLoops = 5;
CCriticalSection CDVDVideoCodecIMX::m_codecBufferLock;

bool CDVDVideoCodecIMX::VpuAllocBuffers(VpuMemInfo *pMemBlock)
{
  int i, size;
  void* ptr;
  VpuMemDesc vpuMem;
  VpuDecRetCode ret;

  for(i=0; i<pMemBlock->nSubBlockNum; i++)
  {
    size = pMemBlock->MemSubBlock[i].nAlignment + pMemBlock->MemSubBlock[i].nSize;
    if (pMemBlock->MemSubBlock[i].MemType == VPU_MEM_VIRT)
    { // Allocate standard virtual memory
      ptr = malloc(size);
      if(ptr == NULL)
      {
        CLog::Log(LOGERROR, "%s - Unable to malloc %d bytes.\n", __FUNCTION__, size);
        goto AllocFailure;
      }
      pMemBlock->MemSubBlock[i].pVirtAddr = (unsigned char*)Align(ptr, pMemBlock->MemSubBlock[i].nAlignment);

      m_decMemInfo.nVirtNum++;
      m_decMemInfo.virtMem = (void**)realloc(m_decMemInfo.virtMem, m_decMemInfo.nVirtNum*sizeof(void*));
      m_decMemInfo.virtMem[m_decMemInfo.nVirtNum-1] = ptr;
    }
    else
    { // Allocate contigous mem for DMA
      vpuMem.nSize = size;
      ret = VPU_DecGetMem(&vpuMem);
      if(ret != VPU_DEC_RET_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s - Unable alloc %d bytes of physical memory (%d).\n", __FUNCTION__, size, ret);
        goto AllocFailure;
      }
      pMemBlock->MemSubBlock[i].pVirtAddr = (unsigned char*)Align(vpuMem.nVirtAddr, pMemBlock->MemSubBlock[i].nAlignment);
      pMemBlock->MemSubBlock[i].pPhyAddr = (unsigned char*)Align(vpuMem.nPhyAddr, pMemBlock->MemSubBlock[i].nAlignment);

      m_decMemInfo.nPhyNum++;
      m_decMemInfo.phyMem = (VpuMemDesc*)realloc(m_decMemInfo.phyMem, m_decMemInfo.nPhyNum*sizeof(VpuMemDesc));
      m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nPhyAddr = vpuMem.nPhyAddr;
      m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nVirtAddr = vpuMem.nVirtAddr;
      m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nCpuAddr = vpuMem.nCpuAddr;
      m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nSize = size;
    }
  }

  return true;

AllocFailure:
  VpuFreeBuffers();
  return false;
}

int CDVDVideoCodecIMX::VpuFindBuffer(void *frameAddr)
{
  for (int i=0; i<m_vpuFrameBufferNum; i++)
  {
    if (m_vpuFrameBuffers[i].pbufY == frameAddr)
      return i;
  }
  return -1;
}

bool CDVDVideoCodecIMX::VpuFreeBuffers()
{
  VpuMemDesc vpuMem;
  VpuDecRetCode vpuRet;
  bool ret = true;

  if (m_decMemInfo.virtMem)
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

  if (m_decMemInfo.phyMem)
  {
    //free physical mem
    for(int i=0; i<m_decMemInfo.nPhyNum; i++)
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
      }
    }
    free(m_decMemInfo.phyMem);
    m_decMemInfo.phyMem = NULL;
    m_decMemInfo.nPhyNum = 0;
  }

  return ret;
}


bool CDVDVideoCodecIMX::VpuOpen()
{
  VpuDecRetCode  ret;
  VpuVersionInfo vpuVersion;
  VpuMemInfo     memInfo;
  VpuDecConfig config;
  int param;

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
  VpuAllocBuffers(&memInfo);

  m_decOpenParam.nReorderEnable = 1;
#ifdef IMX_INPUT_FORMAT_I420
  m_decOpenParam.nChromaInterleave = 0;
#else
  m_decOpenParam.nChromaInterleave = 1;
#endif
  m_decOpenParam.nMapType = 0;
  m_decOpenParam.nTiled2LinearEnable = 0;
  m_decOpenParam.nEnableFileMode = 0;

  ret = VPU_DecOpen(&m_vpuHandle, &m_decOpenParam, &memInfo);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - iMX VPU open failed (%d).\n", __FUNCTION__, ret);
    goto VpuOpenError;
  }

  config = VPU_DEC_CONF_SKIPMODE;
  param = VPU_DEC_SKIPNONE;
  ret = VPU_DecConfig(m_vpuHandle, config, &param);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - iMX VPU set skip mode failed  (%d).\n", __FUNCTION__, ret);
    goto VpuOpenError;
  }

  config = VPU_DEC_CONF_BUFDELAY;
  param = 0;
  ret = VPU_DecConfig(m_vpuHandle, config, &param);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - iMX VPU set buffer delay failed  (%d).\n", __FUNCTION__, ret);
    goto VpuOpenError;
  }

  config = VPU_DEC_CONF_INPUTTYPE;
  param = VPU_DEC_IN_NORMAL;
  ret = VPU_DecConfig(m_vpuHandle, config, &param);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - iMX VPU configure input type failed  (%d).\n", __FUNCTION__, ret);
    goto VpuOpenError;
  }

  // Note that libvpufsl (file vpu_wrapper.c) associates VPU_DEC_CAP_FRAMESIZE
  // capability to the value of nDecFrameRptEnabled which is in fact directly
  // related to the ability to generate VPU_DEC_ONE_FRM_CONSUMED even if the
  // naming is misleading...
  ret = VPU_DecGetCapability(m_vpuHandle, VPU_DEC_CAP_FRAMESIZE, &param);
  m_frameReported = (param != 0);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - iMX VPU get framesize capability failed (%d).\n", __FUNCTION__, ret);
    m_frameReported = false;
  }

  return true;

VpuOpenError:
  Dispose();
  return false;
}

bool CDVDVideoCodecIMX::VpuAllocFrameBuffers()
{
  int totalSize = 0;
  int ySize     = 0;
  int uSize     = 0;
  int vSize     = 0;
  int mvSize    = 0;
  int yStride   = 0;
  int uvStride  = 0;

  VpuDecRetCode ret;
  VpuMemDesc vpuMem;
  unsigned char* ptr;
  unsigned char* ptrVirt;
  int nAlign;

  m_vpuFrameBufferNum = m_initInfo.nMinFrameBufferCount + m_extraVpuBuffers;
  m_vpuFrameBuffers = new VpuFrameBuffer[m_vpuFrameBufferNum];

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
    CLog::Log(LOGERROR, "%s: invalid source format in init info\n",__FUNCTION__,ret);
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

  m_outputBuffers = new CDVDVideoCodecIMXBuffer*[m_vpuFrameBufferNum];

  for (int i=0 ; i < m_vpuFrameBufferNum; i++)
  {
    totalSize = ySize + uSize + vSize + mvSize + nAlign;

    vpuMem.nSize = totalSize;
    ret = VPU_DecGetMem(&vpuMem);
    if(ret != VPU_DEC_RET_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s: vpu malloc frame buf failure: ret=%d \r\n",__FUNCTION__,ret);
      return false;
    }

    //record memory info for release
    m_decMemInfo.nPhyNum++;
    m_decMemInfo.phyMem = (VpuMemDesc*)realloc(m_decMemInfo.phyMem, m_decMemInfo.nPhyNum*sizeof(VpuMemDesc));
    m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nPhyAddr = vpuMem.nPhyAddr;
    m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nVirtAddr = vpuMem.nVirtAddr;
    m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nCpuAddr = vpuMem.nCpuAddr;
    m_decMemInfo.phyMem[m_decMemInfo.nPhyNum-1].nSize = vpuMem.nSize;

    //fill frameBuf
    ptr = (unsigned char*)vpuMem.nPhyAddr;
    ptrVirt = (unsigned char*)vpuMem.nVirtAddr;

    //align the base address
    if(nAlign>1)
    {
      ptr = (unsigned char*)Align(ptr,nAlign);
      ptrVirt = (unsigned char*)Align(ptrVirt,nAlign);
    }

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

#ifdef TRACE_FRAMES
    m_outputBuffers[i] = new CDVDVideoCodecIMXBuffer(i);
#else
    m_outputBuffers[i] = new CDVDVideoCodecIMXBuffer;
#endif
    // Those buffers are ours so lock them to prevent destruction
    m_outputBuffers[i]->Lock();
  }

  return true;
}

CDVDVideoCodecIMX::CDVDVideoCodecIMX()
{
  m_pFormatName = "iMX-xxx";
  m_vpuHandle = 0;
  m_vpuFrameBuffers = NULL;
  m_outputBuffers = NULL;
  m_lastBuffer = NULL;
  m_currentBuffer = NULL;
  m_extraMem = NULL;
  m_vpuFrameBufferNum = 0;
  m_dropState = false;
  m_convert_bitstream = false;
  m_frameCounter = 0;
  m_usePTS = true;
  if (getenv("IMX_NOPTS") != NULL)
  {
    m_usePTS = false;
  }
  m_converter = NULL;
  m_convert_bitstream = false;
  m_bytesToBeConsumed = 0;
  m_previousPts = DVD_NOPTS_VALUE;
  m_warnOnce = true;
#ifdef DUMP_STREAM
  m_dump = NULL;
#endif
}

CDVDVideoCodecIMX::~CDVDVideoCodecIMX()
{
  Dispose();
}

bool CDVDVideoCodecIMX::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
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

  g_IMXContext.RequireConfiguration();

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
  { uint8_t *pb = (uint8_t*)&m_hints.codec_tag;
    if ((isalnum(pb[0]) && isalnum(pb[1]) && isalnum(pb[2]) && isalnum(pb[3])) && g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "Decode: MEDIAINFO: Tag fourcc %c%c%c%c\n", pb[0], pb[1], pb[2], pb[3]);
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

  m_convert_bitstream = false;
  switch(m_hints.codec)
  {
  case CODEC_ID_MPEG1VIDEO:
    m_decOpenParam.CodecFormat = VPU_V_MPEG2;
    m_pFormatName = "iMX-mpeg1";
    break;
  case CODEC_ID_MPEG2VIDEO:
  case CODEC_ID_MPEG2VIDEO_XVMC:
    m_decOpenParam.CodecFormat = VPU_V_MPEG2;
    m_pFormatName = "iMX-mpeg2";
    break;
  case CODEC_ID_H263:
    m_decOpenParam.CodecFormat = VPU_V_H263;
    m_pFormatName = "iMX-h263";
    break;
  case CODEC_ID_H264:
  {
    // Test for VPU unsupported profiles to revert to sw decoding
    if ((m_hints.profile == 110) || //hi10p
        (m_hints.profile == 578 && m_hints.level == 30))   //quite uncommon h264 profile with Main 3.0
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
        m_converter         = new CBitstreamConverter();
        m_convert_bitstream = m_converter->Open(hints.codec, (uint8_t *)hints.extradata, hints.extrasize, true);
      }
    }
    break;
  }
  case CODEC_ID_VC1:
    m_decOpenParam.CodecFormat = VPU_V_VC1_AP;
    m_pFormatName = "iMX-vc1";
    break;
  case CODEC_ID_CAVS:
  case CODEC_ID_AVS:
    m_decOpenParam.CodecFormat = VPU_V_AVS;
    m_pFormatName = "iMX-AVS";
    break;
  case CODEC_ID_RV10:
  case CODEC_ID_RV20:
  case CODEC_ID_RV30:
  case CODEC_ID_RV40:
    m_decOpenParam.CodecFormat = VPU_V_RV;
    m_pFormatName = "iMX-RV";
    break;
  case CODEC_ID_KMVC:
    m_decOpenParam.CodecFormat = VPU_V_AVC_MVC;
    m_pFormatName = "iMX-MVC";
    break;
  case CODEC_ID_VP8:
    m_decOpenParam.CodecFormat = VPU_V_VP8;
    m_pFormatName = "iMX-vp8";
    break;
  case CODEC_ID_MPEG4:
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

void CDVDVideoCodecIMX::Dispose()
{
#ifdef DUMP_STREAM
  if (m_dump)
  {
    fclose(m_dump);
    m_dump = NULL;
  }
#endif

  g_IMXContext.Clear();

  VpuDecRetCode  ret;
  bool VPU_loaded = m_vpuHandle;

  // Release last buffer
  SAFE_RELEASE(m_lastBuffer);
  SAFE_RELEASE(m_currentBuffer);

  Enter();

  // Invalidate output buffers to prevent the renderer from mapping this memory
  for (int i=0; i<m_vpuFrameBufferNum; i++)
  {
    m_outputBuffers[i]->ReleaseFramebuffer(&m_vpuHandle);
    SAFE_RELEASE(m_outputBuffers[i]);
  }

  Leave();

  if (m_vpuHandle)
  {
    ret = VPU_DecFlushAll(m_vpuHandle);
    if (ret != VPU_DEC_RET_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s - VPU flush failed with error code %d.\n", __FUNCTION__, ret);
    }
    ret = VPU_DecClose(m_vpuHandle);
    if (ret != VPU_DEC_RET_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s - VPU close failed with error code %d.\n", __FUNCTION__, ret);
    }
    m_vpuHandle = 0;
  }

  m_frameCounter = 0;

  // Release memory
  if (m_outputBuffers != NULL)
  {
    delete m_outputBuffers;
    m_outputBuffers = NULL;
  }

  VpuFreeBuffers();
  m_vpuFrameBufferNum = 0;

  if (m_vpuFrameBuffers != NULL)
  {
    delete m_vpuFrameBuffers;
    m_vpuFrameBuffers = NULL;
  }

  if (VPU_loaded)
  {
    ret = VPU_DecUnLoad();
    if (ret != VPU_DEC_RET_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s - VPU unload failed with error code %d.\n", __FUNCTION__, ret);
    }
  }

  if (m_converter)
  {
    m_converter->Close();
    SAFE_DELETE(m_converter);
  }

  return;
}

int CDVDVideoCodecIMX::Decode(BYTE *pData, int iSize, double dts, double pts)
{
  VpuDecFrameLengthInfo frameLengthInfo;
  VpuBufferNode inData;
  VpuDecRetCode ret;
  int decRet = 0;
  int retStatus = 0;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;
  int retries = 0;
  int idx;

#ifdef IMX_PROFILE
  static unsigned long long previous, current;
#endif
#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS)
  unsigned long long before_dec;
#endif

#ifdef DUMP_STREAM
  if (m_dump != NULL)
  {
    if (pData)
    {
      fwrite(&dts, sizeof(double), 1, m_dump);
      fwrite(&pts, sizeof(double), 1, m_dump);
      fwrite(&iSize, sizeof(int), 1, m_dump);
      fwrite(pData, 1, iSize, m_dump);
    }
  }
#endif

  SAFE_RELEASE(m_currentBuffer);

  if (!m_vpuHandle)
  {
    VpuOpen();
    if (!m_vpuHandle)
      return VC_ERROR;
  }

  for (int i=0; i < m_vpuFrameBufferNum; i++)
  {
    if (m_outputBuffers[i]->Rendered())
    {
      ret = m_outputBuffers[i]->ReleaseFramebuffer(&m_vpuHandle);
      if(ret != VPU_DEC_RET_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s: vpu clear frame display failure: ret=%d \r\n",__FUNCTION__,ret);
      }
    }
  }

#ifdef IMX_PROFILE
  current = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s - delta time decode : %llu - demux size : %d  dts : %f - pts : %f\n", __FUNCTION__, current - previous, iSize, dts, pts);
  previous = current;
#endif

  if ((pData && iSize) ||
     (m_bytesToBeConsumed))
  {
    //printf("D   %f  %d\n", pts, iSize);
    if ((m_convert_bitstream) && (iSize))
    {
      // convert demuxer packet from bitstream to bytestream (AnnexB)
      if (m_converter->Convert(demuxer_content, demuxer_bytes))
      {
        demuxer_content = m_converter->GetConvertBuffer();
        demuxer_bytes = m_converter->GetConvertSize();
      }
      else
        CLog::Log(LOGERROR,"%s - bitstream_convert error", __FUNCTION__);
    }

    inData.nSize = demuxer_bytes;
    inData.pPhyAddr = NULL;
    inData.pVirAddr = demuxer_content;
    if ((m_decOpenParam.CodecFormat == VPU_V_MPEG2) ||
        (m_decOpenParam.CodecFormat == VPU_V_VC1_AP)||
        (m_decOpenParam.CodecFormat == VPU_V_XVID))
    {
      inData.sCodecData.pData = (unsigned char *)m_hints.extradata;
      inData.sCodecData.nSize = m_hints.extrasize;
    }
    else
    {
      inData.sCodecData.pData = NULL;
      inData.sCodecData.nSize = 0;
    }

#ifdef IMX_PROFILE_BUFFERS
    static unsigned long long dec_time = 0;
#endif

    while (true) // Decode as long as the VPU consumes data
    {
#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS)
      before_dec = XbmcThreads::SystemClockMillis();
#endif
      if (m_frameReported)
        m_bytesToBeConsumed += inData.nSize;
      ret = VPU_DecDecodeBuf(m_vpuHandle, &inData, &decRet);
#ifdef IMX_PROFILE_BUFFERS
      unsigned long long dec_single_call = XbmcThreads::SystemClockMillis()-before_dec;
      dec_time += dec_single_call;
#endif
#ifdef IMX_PROFILE
      CLog::Log(LOGDEBUG, "%s - VPU dec 0x%x decode takes : %lld\n\n", __FUNCTION__, decRet,  XbmcThreads::SystemClockMillis() - before_dec);
#endif

      if (ret != VPU_DEC_RET_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s - VPU decode failed with error code %d.\n", __FUNCTION__, ret);
        goto out_error;
      }

      if (decRet & VPU_DEC_INIT_OK)
      // VPU decoding init OK : We can retrieve stream info
      {
        ret = VPU_DecGetInitialInfo(m_vpuHandle, &m_initInfo);
        if (ret == VPU_DEC_RET_SUCCESS)
        {
          if (g_advancedSettings.CanLogComponent(LOGVIDEO))
          {
            CLog::Log(LOGDEBUG, "%s - VPU Init Stream Info : %dx%d (interlaced : %d - Minframe : %d)"\
                      " - Align : %d bytes - crop : %d %d %d %d - Q16Ratio : %x\n", __FUNCTION__,
              m_initInfo.nPicWidth, m_initInfo.nPicHeight, m_initInfo.nInterlace, m_initInfo.nMinFrameBufferCount,
              m_initInfo.nAddressAlignment, m_initInfo.PicCropRect.nLeft, m_initInfo.PicCropRect.nTop,
              m_initInfo.PicCropRect.nRight, m_initInfo.PicCropRect.nBottom, m_initInfo.nQ16ShiftWidthDivHeightRatio);
          }
          if (VpuAllocFrameBuffers())
          {
            ret = VPU_DecRegisterFrameBuffer(m_vpuHandle, m_vpuFrameBuffers, m_vpuFrameBufferNum);
            if (ret != VPU_DEC_RET_SUCCESS)
            {
              CLog::Log(LOGERROR, "%s - VPU error while registering frame buffers (%d).\n", __FUNCTION__, ret);
              goto out_error;
            }
          }
          else
          {
            goto out_error;
          }

        }
        else
        {
          CLog::Log(LOGERROR, "%s - VPU get initial info failed (%d).\n", __FUNCTION__, ret);
          goto out_error;
        }
      } //VPU_DEC_INIT_OK

      if (decRet & VPU_DEC_ONE_FRM_CONSUMED)
      {
        ret = VPU_DecGetConsumedFrameInfo(m_vpuHandle, &frameLengthInfo);
        if (ret != VPU_DEC_RET_SUCCESS)
        {
          CLog::Log(LOGERROR, "%s - VPU error retireving info about consummed frame (%d).\n", __FUNCTION__, ret);
        }
        m_bytesToBeConsumed -= (frameLengthInfo.nFrameLength + frameLengthInfo.nStuffLength);
        if (frameLengthInfo.pFrame)
        {
          idx = VpuFindBuffer(frameLengthInfo.pFrame->pbufY);
          if (m_bytesToBeConsumed < 50)
            m_bytesToBeConsumed = 0;
          if (idx != -1)
          {
            if (m_previousPts != DVD_NOPTS_VALUE)
            {
              m_outputBuffers[idx]->SetPts(m_previousPts);
              m_previousPts = DVD_NOPTS_VALUE;
            }
            else
              m_outputBuffers[idx]->SetPts(pts);
          }
          else
            CLog::Log(LOGERROR, "%s - could not find frame buffer\n", __FUNCTION__);
        }
      } //VPU_DEC_ONE_FRM_CONSUMED

      if (decRet & VPU_DEC_OUTPUT_DIS)
      // Frame ready to be displayed
      {
        if (retStatus & VC_PICTURE)
            CLog::Log(LOGERROR, "%s - Second picture in the same decode call !\n", __FUNCTION__);

        ret = VPU_DecGetOutputFrame(m_vpuHandle, &m_frameInfo);
        if(ret != VPU_DEC_RET_SUCCESS)
        {
          CLog::Log(LOGERROR, "%s - VPU Cannot get output frame(%d).\n", __FUNCTION__, ret);
          goto out_error;
        }

        // Some codecs (VC1?) lie about their frame size (mod 16). Adjust...
        m_frameInfo.pExtInfo->nFrmWidth  = (((m_frameInfo.pExtInfo->nFrmWidth) + 15) & ~15);
        m_frameInfo.pExtInfo->nFrmHeight = (((m_frameInfo.pExtInfo->nFrmHeight) + 15) & ~15);

        idx = VpuFindBuffer(m_frameInfo.pDisplayFrameBuf->pbufY);
        if (idx != -1)
        {
          CDVDVideoCodecIMXBuffer *buffer = m_outputBuffers[idx];

          /* quick & dirty fix to get proper timestamping for VP8 codec */
          if (m_decOpenParam.CodecFormat == VPU_V_VP8)
            buffer->SetPts(pts);

          buffer->Lock();
          buffer->SetDts(dts);
          buffer->Queue(&m_frameInfo, m_lastBuffer);

#ifdef IMX_PROFILE_BUFFERS
          CLog::Log(LOGNOTICE, "+D  %f  %lld\n", buffer->GetPts(), dec_time);
          dec_time = 0;
#endif

#ifdef TRACE_FRAMES
          CLog::Log(LOGDEBUG, "+  %02d dts %f pts %f  (VPU)\n", idx, dts, pts);
          CLog::Log(LOGDEBUG, "+  %02d dts %f pts %f  (VPU)\n", idx, buffer->GetDts(), buffer->GetPts());
#endif

          if (!m_usePTS)
          {
            buffer->SetPts(DVD_NOPTS_VALUE);
            buffer->SetDts(DVD_NOPTS_VALUE);
          }

          // Save last buffer
          SAFE_RELEASE(m_lastBuffer);
          m_lastBuffer = buffer;
          m_lastBuffer->Lock();

#ifdef IMX_PROFILE_BUFFERS
          static unsigned long long lastD = 0;
          unsigned long long current = XbmcThreads::SystemClockMillis(), tmp;
          CLog::Log(LOGNOTICE, "+V  %f  %lld\n", buffer->GetPts(), current-lastD);
          lastD = current;
#endif

          m_currentBuffer = buffer;

          if (m_currentBuffer)
          {
            retStatus |= VC_PICTURE;
          }
        }
      } //VPU_DEC_OUTPUT_DIS

      // According to libfslvpuwrap: If this flag is set then the frame should
      // be dropped. It is just returned to gather decoder information but not
      // for display.
      if (decRet & VPU_DEC_OUTPUT_MOSAIC_DIS)
      {
        ret = VPU_DecGetOutputFrame(m_vpuHandle, &m_frameInfo);
        if(ret != VPU_DEC_RET_SUCCESS)
        {
          CLog::Log(LOGERROR, "%s - VPU Cannot get output frame(%d).\n", __FUNCTION__, ret);
          goto out_error;
        }

        // Display frame
        ret = VPU_DecOutFrameDisplayed(m_vpuHandle, m_frameInfo.pDisplayFrameBuf);
        if(ret != VPU_DEC_RET_SUCCESS)
        {
          CLog::Log(LOGERROR, "%s: VPU Clear frame display failure(%d)\n",__FUNCTION__,ret);
          goto out_error;
        }
      } //VPU_DEC_OUTPUT_MOSAIC_DIS

      if (decRet & VPU_DEC_OUTPUT_REPEAT)
      {
        if (g_advancedSettings.CanLogComponent(LOGVIDEO))
          CLog::Log(LOGDEBUG, "%s - Frame repeat.\n", __FUNCTION__);
      }
      if (decRet & VPU_DEC_OUTPUT_DROPPED)
      {
        if (g_advancedSettings.CanLogComponent(LOGVIDEO))
          CLog::Log(LOGDEBUG, "%s - Frame dropped.\n", __FUNCTION__);
      }
      if (decRet & VPU_DEC_NO_ENOUGH_BUF)
      {
          CLog::Log(LOGERROR, "%s - No frame buffer available.\n", __FUNCTION__);
      }
      if (decRet & VPU_DEC_SKIP)
      {
        if (g_advancedSettings.CanLogComponent(LOGVIDEO))
          CLog::Log(LOGDEBUG, "%s - Frame skipped.\n", __FUNCTION__);
      }
      if (decRet & VPU_DEC_FLUSH)
      {
        CLog::Log(LOGNOTICE, "%s - VPU requires a flush.\n", __FUNCTION__);
        Reset();
        retStatus = VC_FLUSHED;
      }
      if (decRet & VPU_DEC_OUTPUT_EOS)
      {
        CLog::Log(LOGNOTICE, "%s - EOS encountered.\n", __FUNCTION__);
      }
      if ((decRet & VPU_DEC_NO_ENOUGH_INBUF) ||
          (decRet & VPU_DEC_OUTPUT_DIS))
      {
        // We are done with VPU decoder that time
        break;
      }

      retries++;
      if (retries >= m_maxVpuDecodeLoops)
      {
        CLog::Log(LOGERROR, "%s - Leaving VPU decoding loop after %d iterations\n", __FUNCTION__, m_maxVpuDecodeLoops);
        break;
      }

      if (!(decRet & VPU_DEC_INPUT_USED))
      {
        CLog::Log(LOGERROR, "%s - input not used : addr %p  size :%d!\n", __FUNCTION__, inData.pVirAddr, inData.nSize);
      }

      // Let's process again as VPU_DEC_NO_ENOUGH_INBUF was not set
      // and we don't have an image ready if we reach that point
      inData.pVirAddr = NULL;
      inData.nSize = 0;
    } // Decode loop
  } //(pData && iSize)

  if (!retStatus)
    retStatus |= VC_BUFFER;

  if (m_bytesToBeConsumed > 0)
  {
    // Remember the current pts because the data which has just
    // been sent to the VPU has not yet been consumed.
    // This pts is related to the frame that will be consumed
    // at next call...
    m_previousPts = pts;
  }

#ifdef IMX_PROFILE
  CLog::Log(LOGDEBUG, "%s - returns %x - duration %lld\n", __FUNCTION__, retStatus, XbmcThreads::SystemClockMillis() - previous);
#endif
  return retStatus;

out_error:
  return VC_ERROR;
}

void CDVDVideoCodecIMX::Reset()
{
  int ret;

  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "%s - called\n", __FUNCTION__);

  // Release last buffer
  SAFE_RELEASE(m_lastBuffer);
  SAFE_RELEASE(m_currentBuffer);

  // Invalidate all buffers
  for(int i=0; i < m_vpuFrameBufferNum; i++)
    m_outputBuffers[i]->ReleaseFramebuffer(&m_vpuHandle);

  m_frameCounter = 0;
  m_bytesToBeConsumed = 0;
  m_previousPts = DVD_NOPTS_VALUE;

  // Flush VPU
  ret = VPU_DecFlushAll(m_vpuHandle);
  if (ret != VPU_DEC_RET_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - VPU flush failed with error code %d.\n", __FUNCTION__, ret);
  }
}

unsigned CDVDVideoCodecIMX::GetAllowedReferences()
{
  return RENDER_QUEUE_SIZE;
}

bool CDVDVideoCodecIMX::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (pDvdVideoPicture)
  {
    SAFE_RELEASE(pDvdVideoPicture->IMXBuffer);
  }

  return true;
}

bool CDVDVideoCodecIMX::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
#ifdef IMX_PROFILE
  static unsigned int previous = 0;
  unsigned int current;

  current = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s  tm:%03d\n", __FUNCTION__, current - previous);
  previous = current;
#endif

  m_frameCounter++;

  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  if (m_dropState)
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;
  else
    pDvdVideoPicture->iFlags &= ~DVP_FLAG_DROPPED;

  if (m_initInfo.nInterlace)
    pDvdVideoPicture->iFlags |= DVP_FLAG_INTERLACED;
  else
    pDvdVideoPicture->iFlags &= ~DVP_FLAG_INTERLACED;

  // do a sanity check to not deinterlace progressive content
  if ((pDvdVideoPicture->iFlags & DVP_FLAG_INTERLACED) && (m_currentBuffer->GetFieldType() == VPU_FIELD_NONE))
  {
    if (m_warnOnce)
    {
      m_warnOnce = false;
      CLog::Log(LOGWARNING, "Interlaced content reported by VPU, but full frames detected - Please turn off deinterlacing manually.");
    }
  }

  if (pDvdVideoPicture->iFlags & DVP_FLAG_INTERLACED)
  {
    if (m_currentBuffer->GetFieldType() != VPU_FIELD_BOTTOM && m_currentBuffer->GetFieldType() != VPU_FIELD_BT)
      pDvdVideoPicture->iFlags |= DVP_FLAG_TOP_FIELD_FIRST;
  }
  else
    pDvdVideoPicture->iFlags &= ~DVP_FLAG_TOP_FIELD_FIRST;

  pDvdVideoPicture->format = RENDER_FMT_IMXMAP;
  pDvdVideoPicture->iWidth = m_frameInfo.pExtInfo->FrmCropRect.nRight - m_frameInfo.pExtInfo->FrmCropRect.nLeft;
  pDvdVideoPicture->iHeight = m_frameInfo.pExtInfo->FrmCropRect.nBottom - m_frameInfo.pExtInfo->FrmCropRect.nTop;

  pDvdVideoPicture->iDisplayWidth = ((pDvdVideoPicture->iWidth * m_frameInfo.pExtInfo->nQ16ShiftWidthDivHeightRatio) + 32767) >> 16;
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;

  // Current buffer is locked already -> hot potato
  pDvdVideoPicture->pts = m_currentBuffer->GetPts();
  pDvdVideoPicture->dts = m_currentBuffer->GetDts();

  pDvdVideoPicture->IMXBuffer = m_currentBuffer;
  m_currentBuffer = NULL;

  return true;
}

void CDVDVideoCodecIMX::SetDropState(bool bDrop)
{

  // We are fast enough to continue to really decode every frames
  // and avoid artefacts...
  // (Of course these frames won't be rendered but only decoded)

  if (m_dropState != bDrop)
  {
    m_dropState = bDrop;
#ifdef TRACE_FRAMES
    CLog::Log(LOGDEBUG, "%s : %d\n", __FUNCTION__, bDrop);
#endif
  }
}

void CDVDVideoCodecIMX::Enter()
{
  m_codecBufferLock.lock();
}

void CDVDVideoCodecIMX::Leave()
{
  m_codecBufferLock.unlock();
}

/*******************************************/
#ifdef TRACE_FRAMES
CDVDVideoCodecIMXBuffer::CDVDVideoCodecIMXBuffer(int idx)
  : m_idx(idx)
  ,
#else
CDVDVideoCodecIMXBuffer::CDVDVideoCodecIMXBuffer()
  :
#endif
    m_pts(DVD_NOPTS_VALUE)
  , m_dts(DVD_NOPTS_VALUE)
  , m_frameBuffer(NULL)
  , m_rendered(false)
  , m_previousBuffer(NULL)
{
}

void CDVDVideoCodecIMXBuffer::SetPts(double pts)
{
  m_pts = pts;
}

void CDVDVideoCodecIMXBuffer::SetDts(double dts)
{
  m_dts = dts;
}

void CDVDVideoCodecIMXBuffer::Lock()
{
#ifdef TRACE_FRAMES
  long count = AtomicIncrement(&m_iRefs);
  CLog::Log(LOGDEBUG, "R+ %02d  -  ref : %d  (VPU)\n", m_idx, count);
#else
  AtomicIncrement(&m_iRefs);
#endif
}

long CDVDVideoCodecIMXBuffer::Release()
{
  long count = AtomicDecrement(&m_iRefs);
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "R- %02d  -  ref : %d  (VPU)\n", m_idx, count);
#endif
  if (count == 2)
  {
    // Only referenced by the codec and its next frame, release the previous
    SAFE_RELEASE(m_previousBuffer);
  }
  else if (count == 1)
  {
    // If count drops to 1 then the only reference is being held by the codec
    // that it can be released in the next Decode call.
    if(m_frameBuffer != NULL)
    {
      m_rendered = true;
      SAFE_RELEASE(m_previousBuffer);
#ifdef TRACE_FRAMES
      CLog::Log(LOGDEBUG, "R  %02d  (VPU)\n", m_idx);
#endif
    }
  }
  else if (count == 0)
  {
    delete this;
  }

  return count;
}

bool CDVDVideoCodecIMXBuffer::IsValid()
{
  return m_frameBuffer != NULL;
}

void CDVDVideoCodecIMXBuffer::BeginRender()
{
  CDVDVideoCodecIMX::Enter();
}

void CDVDVideoCodecIMXBuffer::EndRender()
{
  CDVDVideoCodecIMX::Leave();
}

bool CDVDVideoCodecIMXBuffer::Rendered() const
{
  return m_rendered;
}

void CDVDVideoCodecIMXBuffer::Queue(VpuDecOutFrameInfo *frameInfo,
                                    CDVDVideoCodecIMXBuffer *previous)
{
  // No lock necessary because at this time there is definitely no
  // thread still holding a reference
  m_frameBuffer = frameInfo->pDisplayFrameBuf;
  m_rendered = false;
  m_previousBuffer = previous;
  if (m_previousBuffer)
    m_previousBuffer->Lock();

#ifdef IMX_INPUT_FORMAT_I420
  iFormat     = _4CC('I', '4', '2', '0');
#else
  iFormat     = _4CC('N', 'V', '1', '2');
#endif
  iWidth      = frameInfo->pExtInfo->nFrmWidth;
  iHeight     = frameInfo->pExtInfo->nFrmHeight;
  pVirtAddr   = m_frameBuffer->pbufVirtY;
  pPhysAddr   = (int)m_frameBuffer->pbufY;
  m_fieldType = frameInfo->eFieldType;
}

VpuDecRetCode CDVDVideoCodecIMXBuffer::ReleaseFramebuffer(VpuDecHandle *handle)
{
  // Again no lock required because this is only issued after the last
  // external reference was released
  VpuDecRetCode ret = VPU_DEC_RET_FAILURE;

  if((m_frameBuffer != NULL) && *handle)
  {
    ret = VPU_DecOutFrameDisplayed(*handle, m_frameBuffer);
    if(ret != VPU_DEC_RET_SUCCESS)
      CLog::Log(LOGERROR, "%s: vpu clear frame display failure: ret=%d \r\n",__FUNCTION__,ret);
  }
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "-  %02d  (VPU)\n", m_idx);
#endif
  m_rendered = false;
  m_frameBuffer = NULL;
  SetPts(DVD_NOPTS_VALUE);
  SAFE_RELEASE(m_previousBuffer);

  return ret;
}

CDVDVideoCodecIMXBuffer::~CDVDVideoCodecIMXBuffer()
{
  assert(m_iRefs == 0);
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "~  %02d  (VPU)\n", m_idx);
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
  , m_deInterlacing(false)
  , m_pageCrops(NULL)
  , m_g2dHandle(NULL)
  , m_bufferCapture(NULL)
  , m_checkConfigRequired(true)
{
  // Limit queue to 2
  m_input.resize(2);
  m_beginInput = m_endInput = m_bufferedInput = 0;
}

CIMXContext::~CIMXContext()
{
  Close();
}


bool CIMXContext::Configure()
{

  if (!m_checkConfigRequired)
    return false;

  SetBlitRects(CRectInt(), CRectInt());
  m_fbCurrentPage = 0;

  int fb0 = open("/dev/fb0", O_RDWR, 0);

  if (fb0 < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to open /dev/fb0\n");
    return false;
  }

  struct fb_var_screeninfo fbVar;
  if (ioctl(fb0, FBIOGET_VSCREENINFO, &fbVar) < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to read primary screen resolution\n");
    close(fb0);
    return false;
  }

  close(fb0);

  if (m_fbHandle)
    Close();

  CLog::Log(LOGNOTICE, "iMX : Initialize render buffers\n");

  memcpy(&m_fbVar, &fbVar, sizeof(fbVar));

  const char *deviceName = "/dev/fb1";

  m_fbHandle = open(deviceName, O_RDWR | O_NONBLOCK, 0);
  if (m_fbHandle < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to open framebuffer: %s\n", deviceName);
    return false;
  }

  m_fbWidth = m_fbVar.xres;
  m_fbHeight = m_fbVar.yres;

  if (ioctl(m_fbHandle, FBIOGET_VSCREENINFO, &m_fbVar) < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to query variable screen info at %s\n", deviceName);
    return false;
  }

  m_pageCrops = new CRectInt[m_fbPages];

  m_fbVar.xoffset = 0;
  m_fbVar.yoffset = 0;
  if (m_deInterlacing)
  {
    m_fbVar.nonstd = _4CC('U', 'Y', 'V', 'Y');
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
  // One additional line that is required for deinterlacing
  m_fbVar.yres_virtual = (m_fbVar.yres+1) * m_fbPages;
  m_fbVar.xres_virtual = m_fbVar.xres;

  if (ioctl(m_fbHandle, FBIOPUT_VSCREENINFO, &m_fbVar) < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to setup %s\n", deviceName);
    close(m_fbHandle);
    m_fbHandle = 0;
    return false;
  }

  struct fb_fix_screeninfo fb_fix;
  if (ioctl(m_fbHandle, FBIOGET_FSCREENINFO, &fb_fix) < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to query fixed screen info at %s\n", deviceName);
    close(m_fbHandle);
    m_fbHandle = 0;
    return false;
  }

  // Final setup
  m_fbLineLength = fb_fix.line_length;
  m_fbPhysSize = fb_fix.smem_len;
  m_fbPageSize = m_fbLineLength * m_fbVar.yres_virtual / m_fbPages;
  m_fbPhysAddr = fb_fix.smem_start;
  m_fbVirtAddr = (uint8_t*)mmap(0, m_fbPhysSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fbHandle, 0);

  CLog::Log(LOGDEBUG, "iMX : Allocated %d render buffers\n", m_fbPages);

  m_ipuHandle = open("/dev/mxc_ipu", O_RDWR, 0);
  if (m_ipuHandle < 0)
  {
    CLog::Log(LOGWARNING, "iMX : Failed to initialize IPU: %s\n", strerror(errno));
    m_ipuHandle = 0;
    Close();
    return false;
  }

  Clear();
  Unblank();

  // Start the ipu thread
  Create();
  m_checkConfigRequired = false;
  return true;
}

bool CIMXContext::Close()
{
  CLog::Log(LOGINFO, "iMX : Closing context\n");

  // Stop the ipu thread
  StopThread();

  if (m_pageCrops)
  {
    delete[] m_pageCrops;
    m_pageCrops = NULL;
  }

  if (m_fbVirtAddr)
  {
    Clear();
    munmap(m_fbVirtAddr, m_fbPhysSize);
    m_fbVirtAddr = NULL;
  }

  if (m_fbHandle)
  {
    Blank();
    close(m_fbHandle);
    m_fbHandle = 0;
    m_fbPhysAddr = 0;
  }

  if (m_ipuHandle)
  {
    // Close IPU device
    if (close(m_ipuHandle))
      CLog::Log(LOGERROR, "iMX : Failed to close IPU: %s\n", strerror(errno));

    m_ipuHandle = 0;
  }

  m_checkConfigRequired = true;
  CLog::Log(LOGNOTICE, "iMX : Deinitialized render context\n");

  return true;
}

bool CIMXContext::GetPageInfo(CIMXBuffer *info, int page)
{
  if (page < 0 || page >= m_fbPages)
    return false;

  info->iWidth    = m_fbWidth;
  info->iHeight   = m_fbHeight;
  info->iFormat   = m_fbVar.nonstd;
  info->pPhysAddr = m_fbPhysAddr + page*m_fbPageSize;
  info->pVirtAddr = m_fbVirtAddr + page*m_fbPageSize;

  return true;
}

bool CIMXContext::Blank()
{
  if (!m_fbHandle) return false;
  return ioctl(m_fbHandle, FBIOBLANK, 1) == 0;
}

bool CIMXContext::Unblank()
{
  if (!m_fbHandle) return false;
  return ioctl(m_fbHandle, FBIOBLANK, FB_BLANK_UNBLANK) == 0;
}

bool CIMXContext::SetVSync(bool enable)
{
  m_vsync = enable;
  return true;
}

void CIMXContext::SetDoubleRate(bool flag)
{
  if (flag)
    m_currentFieldFmt |= IPU_DEINTERLACE_RATE_EN;
  else
    m_currentFieldFmt &= ~IPU_DEINTERLACE_RATE_EN;

  m_currentFieldFmt &= ~IPU_DEINTERLACE_RATE_FRAME1;
}

bool CIMXContext::DoubleRate() const
{
  return m_currentFieldFmt & IPU_DEINTERLACE_RATE_EN;
}

void CIMXContext::SetInterpolatedFrame(bool flag)
{
  if (flag)
    m_currentFieldFmt &= ~IPU_DEINTERLACE_RATE_FRAME1;
  else
    m_currentFieldFmt |= IPU_DEINTERLACE_RATE_FRAME1;
}

void CIMXContext::SetDeInterlacing(bool flag)
{
  bool sav_deInt = m_deInterlacing;
  m_deInterlacing = flag;
  // If deinterlacing configuration changes then fb has to be reconfigured
  if (sav_deInt != m_deInterlacing)
  {
    m_checkConfigRequired = true;
    Configure();
  }
}

void CIMXContext::SetBlitRects(const CRect &srcRect, const CRect &dstRect)
{
  m_srcRect = srcRect;
  m_dstRect = dstRect;
}

bool CIMXContext::Blit(int page, CIMXBuffer *source_p, CIMXBuffer *source, bool topBottomFields)
{
  if (page < 0 || page >= m_fbPages)
    return false;

  IPUTask ipu;
  PrepareTask(ipu, source_p, source, topBottomFields);
  return DoTask(ipu, page);
}

bool CIMXContext::BlitAsync(CIMXBuffer *source_p, CIMXBuffer *source, bool topBottomFields, CRect *dest)
{
  IPUTask ipu;
  PrepareTask(ipu, source_p, source, topBottomFields, dest);
  return PushTask(ipu);
}

bool CIMXContext::PushCaptureTask(CIMXBuffer *source, CRect *dest)
{
  IPUTask ipu;
  m_CaptureDone = false;
  PrepareTask(ipu, NULL, source, false, dest);
  return PushTask(ipu);
}

bool CIMXContext::ShowPage(int page)
{
  int ret;

  if (!m_fbHandle) return false;
  if (page < 0 || page >= m_fbPages) return false;

  // Protect page swapping from screen capturing that does read the current
  // front buffer. This is actually not done very frequently so the lock
  // does not hurt.
  CSingleLock lk(m_pageSwapLock);

  m_fbCurrentPage = page;
  m_fbVar.activate = FB_ACTIVATE_VBL;
  m_fbVar.yoffset = (m_fbVar.yres+1)*page;
  if ((ret = ioctl(m_fbHandle, FBIOPAN_DISPLAY, &m_fbVar)) < 0)
    CLog::Log(LOGWARNING, "Panning failed: %s\n", strerror(errno));

  // Wait for sync
  if (m_vsync)
  {
    if (ioctl(m_fbHandle, FBIO_WAITFORVSYNC, 0) < 0)
      CLog::Log(LOGWARNING, "Vsync failed: %s\n", strerror(errno));
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
  else if (m_fbVar.nonstd == _4CC('U', 'Y', 'V', 'Y'))
  {
    int pixels = bytes / 2;
    for (int i = 0; i < pixels; ++i, tmp_buf += 2)
    {
      tmp_buf[0] = 128;
      tmp_buf[1] = 16;
    }
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

bool CIMXContext::PushTask(const IPUTask &task)
{
  if (!task.current)
    return false;

  CSingleLock lk(m_monitor);

  if (m_bStop)
  {
    m_inputNotEmpty.notifyAll();
    return false;
  }

  // If the input queue is full, wait for a free slot
  while ((m_bufferedInput == m_input.size()) && !m_bStop)
    m_inputNotFull.wait(lk);

  if (m_bStop)
  {
    m_inputNotEmpty.notifyAll();
    return false;
  }

  // Store the value
  if (task.previous) task.previous->Lock();
  task.current->Lock();

  memcpy(&m_input[m_endInput], &task, sizeof(IPUTask));
  m_endInput = (m_endInput+1) % m_input.size();
  ++m_bufferedInput;
  m_inputNotEmpty.notifyAll();

  return true;
}

void CIMXContext::WaitCapture()
{
  CSingleLock lk(m_monitor);
  while (!m_CaptureDone)
    m_inputNotFull.wait(lk);
}

void CIMXContext::PrepareTask(IPUTask &ipu, CIMXBuffer *source_p, CIMXBuffer *source,
                              bool topBottomFields, CRect *dest)
{
  Configure();
  // Fill with zeros
  ipu.Zero();
  ipu.previous = source_p;
  ipu.current = source;

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

  iSrcRect.x1 = (int)srcRect.x1;
  iSrcRect.y1 = (int)srcRect.y1;
  iSrcRect.x2 = (int)srcRect.x2;
  iSrcRect.y2 = (int)srcRect.y2;

  iDstRect.x1 = Align((int)dstRect.x1,8);
  iDstRect.y1 = Align((int)dstRect.y1,8);
  iDstRect.x2 = Align2((int)dstRect.x2,8);
  iDstRect.y2 = Align2((int)dstRect.y2,8);

  ipu.task.input.crop.pos.x  = iSrcRect.x1;
  ipu.task.input.crop.pos.y  = iSrcRect.y1;
  ipu.task.input.crop.w      = iSrcRect.Width();
  ipu.task.input.crop.h      = iSrcRect.Height();

  ipu.task.output.crop.pos.x = iDstRect.x1;
  ipu.task.output.crop.pos.y = iDstRect.y1;
  ipu.task.output.crop.w     = iDstRect.Width();
  ipu.task.output.crop.h     = iDstRect.Height();

  // If dest is set it means we do not want to blit to frame buffer
  // but to a capture buffer and we state this capture buffer dimensions
  if (dest)
  {
    // Populate partly output block
    ipu.task.output.crop.pos.x = 0;
    ipu.task.output.crop.pos.y = 0;
    ipu.task.output.crop.w     = iDstRect.Width();
    ipu.task.output.crop.h     = iDstRect.Height();
    ipu.task.output.width  = iDstRect.Width();
    ipu.task.output.height = iDstRect.Height();
  }
  else
  {
  // Setup deinterlacing if enabled
  if (m_deInterlacing)
  {
    ipu.task.input.deinterlace.enable = 1;
    /*
    if (source_p)
    {
      task.input.deinterlace.motion = MED_MOTION;
      task.input.paddr   = source_p->pPhysAddr;
      task.input.paddr_n = source->pPhysAddr;
    }
    else
    */
      ipu.task.input.deinterlace.motion = HIGH_MOTION;
    ipu.task.input.deinterlace.field_fmt = m_currentFieldFmt;

    if (topBottomFields)
      ipu.task.input.deinterlace.field_fmt |= IPU_DEINTERLACE_FIELD_TOP;
    else
      ipu.task.input.deinterlace.field_fmt |= IPU_DEINTERLACE_FIELD_BOTTOM;
  }
  }
}

bool CIMXContext::DoTask(IPUTask &ipu, int targetPage)
{
  bool swapColors = false;

  // Clear page if cropping changes
  CRectInt dstRect(ipu.task.output.crop.pos.x, ipu.task.output.crop.pos.y,
                   ipu.task.output.crop.pos.x + ipu.task.output.crop.w,
                   ipu.task.output.crop.pos.y + ipu.task.output.crop.h);

  // Populate input block
  ipu.task.input.width   = ipu.current->iWidth;
  ipu.task.input.height  = ipu.current->iHeight;
  ipu.task.input.format  = ipu.current->iFormat;
  ipu.task.input.paddr   = ipu.current->pPhysAddr;

  // Populate output block if it has not already been filled
  if (ipu.task.output.width == 0)
  {
    ipu.task.output.width  = m_fbWidth;
    ipu.task.output.height = m_fbHeight;
    ipu.task.output.format = m_fbVar.nonstd;
    ipu.task.output.paddr  = m_fbPhysAddr + targetPage*m_fbPageSize;

    if (m_pageCrops[targetPage] != dstRect)
    {
      m_pageCrops[targetPage] = dstRect;
      Clear(targetPage);
    }
  }
  else
  {
    // If we have already set dest dimensions we want to use capture buffer
    // Note we allocate this capture buffer as late as this function because
    // all g2d functions have to be called from the same thread
    int size = ipu.task.output.width * ipu.task.output.height * 4;
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
    ipu.task.output.paddr = m_bufferCapture->buf_paddr;
    swapColors = true;
  }

  if ((ipu.task.input.crop.w <= 0) || (ipu.task.input.crop.h <= 0)
  ||  (ipu.task.output.crop.w <= 0) || (ipu.task.output.crop.h <= 0))
    return false;

#ifdef IMX_PROFILE_BUFFERS
  unsigned long long before = XbmcThreads::SystemClockMillis();
#endif

  if (ipu.task.input.deinterlace.enable)
  {
    //We really use IPU only if we have to deinterlace (using VDIC)
    int ret = IPU_CHECK_ERR_INPUT_CROP;
    while (ret != IPU_CHECK_OK && ret > IPU_CHECK_ERR_MIN)
    {
        ret = ioctl(m_ipuHandle, IPU_CHECK_TASK, &ipu.task);
        switch (ret)
        {
        case IPU_CHECK_OK:
            break;
        case IPU_CHECK_ERR_SPLIT_INPUTW_OVER:
            ipu.task.input.crop.w -= 8;
            break;
        case IPU_CHECK_ERR_SPLIT_INPUTH_OVER:
            ipu.task.input.crop.h -= 8;
            break;
        case IPU_CHECK_ERR_SPLIT_OUTPUTW_OVER:
            ipu.task.output.crop.w -= 8;
            break;
        case IPU_CHECK_ERR_SPLIT_OUTPUTH_OVER:
            ipu.task.output.crop.h -= 8;
            break;
        default:
            CLog::Log(LOGWARNING, "iMX : unhandled IPU check error: %d\n", ret);
            return false;
        }
    }

    // Need to find another interface to protect ipu.current from disposing
    // in CDVDVideoCodecIMX::Dispose. CIMXContext must not have knowledge
    // about CDVDVideoCodecIMX.
    ipu.current->BeginRender();
    if (ipu.current->IsValid())
        ret = ioctl(m_ipuHandle, IPU_QUEUE_TASK, &ipu.task);
    else
        ret = 0;
    ipu.current->EndRender();

    if (ret < 0)
    {
        CLog::Log(LOGERROR, "IPU task failed: %s\n", strerror(errno));
        return false;
    }

    // Duplicate 2nd scandline if double rate is active
    if (ipu.task.input.deinterlace.field_fmt & IPU_DEINTERLACE_RATE_EN)
    {
        uint8_t *pageAddr = m_fbVirtAddr + targetPage*m_fbPageSize;
        memcpy(pageAddr, pageAddr+m_fbLineLength, m_fbLineLength);
    }
  }
  else
  {
    // deinterlacing is not required, let's use g2d instead of IPU

    struct g2d_surface src, dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    ipu.current->BeginRender();
    if (ipu.current->IsValid())
    {
      if (ipu.current->iFormat == _4CC('I', '4', '2', '0'))
      {
        src.format = G2D_I420;
        src.planes[0] = ipu.current->pPhysAddr;
        src.planes[1] = src.planes[0] + Align(ipu.current->iWidth * ipu.current->iHeight, 64);
        src.planes[2] = src.planes[1] + Align((ipu.current->iWidth * ipu.current->iHeight) / 2, 64);
      }
      else //_4CC('N', 'V', '1', '2');
      {
        src.format = G2D_NV12;
        src.planes[0] = ipu.current->pPhysAddr;
        src.planes[1] =  src.planes[0] + Align(ipu.current->iWidth * ipu.current->iHeight, 64);
      }

      src.left = ipu.task.input.crop.pos.x;
      src.right = ipu.task.input.crop.w + src.left ;
      src.top  = ipu.task.input.crop.pos.y;
      src.bottom = ipu.task.input.crop.h + src.top;
      src.stride = ipu.current->iWidth;
      src.width  = ipu.current->iWidth;
      src.height = ipu.current->iHeight;
      src.rot = G2D_ROTATION_0;
      /*printf("src planes :%x -%x -%x \n",src.planes[0], src.planes[1], src.planes[2] );
      printf("src left %d right %d top %d bottom %d stride %d w : %d h %d rot : %d\n",
           src.left, src.right, src.top, src.bottom, src.stride, src.width, src.height, src.rot);*/

      dst.planes[0] = ipu.task.output.paddr;
      dst.left = ipu.task.output.crop.pos.x;
      dst.top = ipu.task.output.crop.pos.y;
      dst.right = ipu.task.output.crop.w + dst.left;
      dst.bottom = ipu.task.output.crop.h + dst.top;

      dst.stride = ipu.task.output.width;
      dst.width = ipu.task.output.width;
      dst.height = ipu.task.output.height;
      dst.rot = G2D_ROTATION_0;
      dst.format = swapColors ? G2D_BGRA8888 : G2D_RGBA8888;
      /*printf("dst planes :%x -%x -%x \n",dst.planes[0], dst.planes[1], dst.planes[2] );
      printf("dst left %d right %d top %d bottom %d stride %d w : %d h %d rot : %d format %d\n",
           dst.left, dst.right, dst.top, dst.bottom, dst.stride, dst.width, dst.height, dst.rot, dst.format);*/

      // Launch synchronous blit
      g2d_blit(m_g2dHandle, &src, &dst);
      g2d_finish(m_g2dHandle);
      if ((m_bufferCapture) && (ipu.task.output.paddr == m_bufferCapture->buf_paddr))
        m_CaptureDone = true;
    }
    ipu.current->EndRender();
  }

#ifdef IMX_PROFILE_BUFFERS
  unsigned long long after = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGNOTICE, "+P  %f  %d\n", ((CDVDVideoCodecIMXBuffer*)ipu.current)->GetPts(), (int)(after-before));
#endif

  return true;
}

void CIMXContext::OnStartup()
{
  CLog::Log(LOGNOTICE, "iMX : IPU thread started");
}

void CIMXContext::OnExit()
{
  CLog::Log(LOGNOTICE, "iMX : IPU thread terminated");
}

void CIMXContext::StopThread(bool bWait /*= true*/)
{
  CThread::StopThread(false);
  m_inputNotFull.notifyAll();
  m_inputNotEmpty.notifyAll();
  if (bWait)
    CThread::StopThread(true);
}

void CIMXContext::Process()
{
  bool ret, useBackBuffer;
  int backBuffer;

  // open g2d here to ensure all g2d fucntions are called from the same thread
  if (m_g2dHandle == NULL)
  {
    if (g2d_open(&m_g2dHandle) != 0)
    {
      m_g2dHandle = NULL;
      CLog::Log(LOGERROR, "%s - Error while trying open G2D\n", __FUNCTION__);
    }
  }

  while (!m_bStop)
  {
    {
      CSingleLock lk(m_monitor);
      while (!m_bufferedInput && !m_bStop)
        m_inputNotEmpty.wait(lk);

      if (m_bStop) break;

      useBackBuffer = m_vsync;
      IPUTask &task = m_input[m_beginInput];
      backBuffer = useBackBuffer?1-m_fbCurrentPage:m_fbCurrentPage;

      // Hack to detect we deal with capture buffer
      if (task.task.output.width != 0)
          useBackBuffer = false;
      ret = DoTask(task, backBuffer);

      // Free resources
      task.Done();

      m_beginInput = (m_beginInput+1) % m_input.size();
      --m_bufferedInput;
      m_inputNotFull.notifyAll();
    }

    // Show back buffer
    if (useBackBuffer && ret) ShowPage(backBuffer);
  }

  // Mark all pending jobs as done
  CSingleLock lk(m_monitor);
  while (m_bufferedInput > 0)
  {
    m_input[m_beginInput].Done();
    m_beginInput = (m_beginInput+1) % m_input.size();
    --m_bufferedInput;
  }

  // close g2d here to ensure all g2d fucntions are called from the same thread
  if (m_bufferCapture)
  {
    if (g2d_free(m_bufferCapture))
      CLog::Log(LOGERROR, "iMX : Failed to free capture buffers\n");
    m_bufferCapture = NULL;
  }
  if (m_g2dHandle)
  {
    if (g2d_close(m_g2dHandle))
      CLog::Log(LOGERROR, "iMX : Error while closing G2D\n");
    m_g2dHandle = NULL;
  }

  return;
}
