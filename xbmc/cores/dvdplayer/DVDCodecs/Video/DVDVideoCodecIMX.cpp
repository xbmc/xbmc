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

#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "DVDClock.h"
#include "settings/AdvancedSettings.h"
#include "threads/Atomics.h"

#include <cassert>

#define FRAME_ALIGN 16
#define MEDIAINFO 1
#define _4CC(c1,c2,c3,c4) (((uint32_t)(c4)<<24)|((uint32_t)(c3)<<16)|((uint32_t)(c2)<<8)|(uint32_t)(c1))
#define Align(ptr,align)  (((unsigned int)ptr + (align) - 1)/(align)*(align))

// Experiments show that we need at least one more (+1) VPU buffer than the min value returned by the VPU
const int CDVDVideoCodecIMX::m_extraVpuBuffers = 6;
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

bool CDVDVideoCodecIMX::VpuFreeBuffers(void)
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
        CLog::Log(LOGERROR, "%s - Errror while trying to free physical memory (%d).\n", __FUNCTION__, ret);
        ret = false;
      }
    }
    free(m_decMemInfo.phyMem);
    m_decMemInfo.phyMem = NULL;
    m_decMemInfo.nPhyNum = 0;
  }

  return ret;
}


bool CDVDVideoCodecIMX::VpuOpen(void)
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
  m_decOpenParam.nChromaInterleave = 1;
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

bool CDVDVideoCodecIMX::VpuAllocFrameBuffers(void)
{
  VpuDecRetCode ret;
  VpuMemDesc vpuMem;
  int totalSize=0;
  int mvSize=0;
  int ySize=0;
  int uvSize=0;
  int yStride=0;
  int uvStride=0;
  unsigned char* ptr;
  unsigned char* ptrVirt;
  int nAlign;

  m_vpuFrameBufferNum =  m_initInfo.nMinFrameBufferCount + m_extraVpuBuffers;
  m_vpuFrameBuffers = new VpuFrameBuffer[m_vpuFrameBufferNum];

  yStride=Align(m_initInfo.nPicWidth,FRAME_ALIGN);
  if(m_initInfo.nInterlace)
  {
    ySize=Align(m_initInfo.nPicWidth,FRAME_ALIGN)*Align(m_initInfo.nPicHeight,(2*FRAME_ALIGN));
  }
  else
  {
    ySize=Align(m_initInfo.nPicWidth,FRAME_ALIGN)*Align(m_initInfo.nPicHeight,FRAME_ALIGN);
  }

  //NV12 for all video
  uvStride=yStride;
  uvSize=ySize/2;
  mvSize=uvSize/2;

  nAlign=m_initInfo.nAddressAlignment;
  if(nAlign>1)
  {
    ySize=Align(ySize,nAlign);
    uvSize=Align(uvSize,nAlign);
  }

  m_outputBuffers = new CDVDVideoCodecIMXBuffer*[m_vpuFrameBufferNum];

  for (int i=0 ; i < m_vpuFrameBufferNum; i++)
  {
    totalSize=(ySize+uvSize+mvSize+nAlign)*1;

    vpuMem.nSize=totalSize;
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
    ptr=(unsigned char*)vpuMem.nPhyAddr;
    ptrVirt=(unsigned char*)vpuMem.nVirtAddr;

    //align the base address
    if(nAlign>1)
    {
      ptr=(unsigned char*)Align(ptr,nAlign);
      ptrVirt=(unsigned char*)Align(ptrVirt,nAlign);
    }

    // fill stride info
    m_vpuFrameBuffers[i].nStrideY=yStride;
    m_vpuFrameBuffers[i].nStrideC=uvStride;

    // fill phy addr
    m_vpuFrameBuffers[i].pbufY=ptr;
    m_vpuFrameBuffers[i].pbufCb=ptr+ySize;
    m_vpuFrameBuffers[i].pbufCr=0;
    m_vpuFrameBuffers[i].pbufMvCol=ptr+ySize+uvSize;

    // fill virt addr
    m_vpuFrameBuffers[i].pbufVirtY=ptrVirt;
    m_vpuFrameBuffers[i].pbufVirtCb=ptrVirt+ySize;
    m_vpuFrameBuffers[i].pbufVirtCr=0;
    m_vpuFrameBuffers[i].pbufVirtMvCol=ptrVirt+ySize+uvSize;

    m_vpuFrameBuffers[i].pbufY_tilebot=0;
    m_vpuFrameBuffers[i].pbufCb_tilebot=0;
    m_vpuFrameBuffers[i].pbufVirtY_tilebot=0;
    m_vpuFrameBuffers[i].pbufVirtCb_tilebot=0;

#ifdef TRACE_FRAMES
    m_outputBuffers[i] = new CDVDVideoCodecIMXBuffer(i);
#else
    m_outputBuffers[i] = new CDVDVideoCodecIMXBuffer();
#endif
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
        (m_hints.profile == 578))   //quite uncommon h264 profile
    {
      CLog::Log(LOGNOTICE, "i.MX6 VPU is not able to decode AVC profile %d", m_hints.profile);
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

void CDVDVideoCodecIMX::Dispose(void)
{
  VpuDecRetCode  ret;
  bool VPU_loaded = m_vpuHandle;

  // Prevent rendering thread from using frame buffers
  Enter();

  // Release last buffer
  if(m_lastBuffer)
    SAFE_RELEASE(m_lastBuffer);

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
  unsigned long long before_dec;
#endif

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

    while (true) // Decode as long as the VPU consumes data
    {
#ifdef IMX_PROFILE
      before_dec = XbmcThreads::SystemClockMillis();
#endif
      if (m_frameReported)
        m_bytesToBeConsumed += inData.nSize;
      ret = VPU_DecDecodeBuf(m_vpuHandle, &inData, &decRet);
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

        /* quick & dirty fix to get proper timestamping for VP8 codec */
        if (m_decOpenParam.CodecFormat == VPU_V_VP8)
        {
          idx = VpuFindBuffer(m_frameInfo.pDisplayFrameBuf->pbufY);
          m_outputBuffers[idx]->SetPts(pts);
        }

        retStatus |= VC_PICTURE;
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

  if (retStatus == 0)
  {
    retStatus |= VC_BUFFER;
  }

  if (m_bytesToBeConsumed > 0)
  {
    // Remember the current pts because the data which has just
    // been sent to the VPU has not yet been consumed.
    // This pts is related to the frame that will be consumed
    // at next call...
    m_previousPts = pts;
  }
  // Store current dts (will be used only if VC_PICTURE is set)
  m_dts = dts;

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
  if(m_lastBuffer)
    SAFE_RELEASE(m_lastBuffer);

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
  return 3;
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

  pDvdVideoPicture->format = RENDER_FMT_IMXMAP;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->iWidth = m_frameInfo.pExtInfo->FrmCropRect.nRight - m_frameInfo.pExtInfo->FrmCropRect.nLeft;
  pDvdVideoPicture->iHeight = m_frameInfo.pExtInfo->FrmCropRect.nBottom - m_frameInfo.pExtInfo->FrmCropRect.nTop;

  pDvdVideoPicture->iDisplayWidth = ((pDvdVideoPicture->iWidth * m_frameInfo.pExtInfo->nQ16ShiftWidthDivHeightRatio) + 32767) >> 16;
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;

  int idx = VpuFindBuffer(m_frameInfo.pDisplayFrameBuf->pbufY);
  if (idx != -1)
  {
    CDVDVideoCodecIMXBuffer *buffer = m_outputBuffers[idx];

    pDvdVideoPicture->pts = buffer->GetPts();
    pDvdVideoPicture->dts = m_dts;
    if (!m_usePTS)
    {
      pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
      pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
    }

    buffer->Queue(&m_frameInfo, m_lastBuffer);

#ifdef TRACE_FRAMES
    CLog::Log(LOGDEBUG, "+  %02d dts %f pts %f  (VPU)\n", idx, pDvdVideoPicture->dts, pDvdVideoPicture->pts);
#endif

    pDvdVideoPicture->IMXBuffer = buffer;
    pDvdVideoPicture->IMXBuffer->Lock();

    // Save last buffer
    if (m_lastBuffer)
      SAFE_RELEASE(m_lastBuffer);
    m_lastBuffer = buffer;
    m_lastBuffer->Lock();
  }
  else
  {
    CLog::Log(LOGERROR, "%s - could not find frame buffer\n", __FUNCTION__);
  }

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
  : m_refs(1)
  , m_idx(idx)
#else
CDVDVideoCodecIMXBuffer::CDVDVideoCodecIMXBuffer()
  : m_refs(1)
#endif
  , m_frameBuffer(NULL)
  , m_rendered(false)
  , m_pts(DVD_NOPTS_VALUE)
  , m_previousBuffer(NULL)
{
}

void CDVDVideoCodecIMXBuffer::Lock()
{
#ifdef TRACE_FRAMES
  long count = AtomicIncrement(&m_refs);
  CLog::Log(LOGDEBUG, "R+ %02d  -  ref : %d  (VPU)\n", m_idx, count);
#else
  AtomicIncrement(&m_refs);
#endif
}

long CDVDVideoCodecIMXBuffer::Release()
{
  long count = AtomicDecrement(&m_refs);
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "R- %02d  -  ref : %d  (VPU)\n", m_idx, count);
#endif
  if (count == 2)
  {
    // Only referenced by the coded and its next frame, release the previous
    SAFE_RELEASE(m_previousBuffer);
  }
  if (count == 1)
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

  m_iWidth  = frameInfo->pExtInfo->nFrmWidth;
  m_iHeight = frameInfo->pExtInfo->nFrmHeight;
  m_VirtAddr = m_frameBuffer->pbufVirtY;
  m_phyAddr = m_frameBuffer->pbufY;
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
  m_pts = DVD_NOPTS_VALUE;
  SAFE_RELEASE(m_previousBuffer);

  return ret;
}

void CDVDVideoCodecIMXBuffer::SetPts(double pts)
{
  m_pts = pts;
}

double CDVDVideoCodecIMXBuffer::GetPts(void) const
{
  return m_pts;
}

CDVDVideoCodecIMXBuffer *CDVDVideoCodecIMXBuffer::GetPreviousBuffer() const
{
  return m_previousBuffer;
}

CDVDVideoCodecIMXBuffer::~CDVDVideoCodecIMXBuffer()
{
  assert(m_refs == 0);
#ifdef TRACE_FRAMES
  CLog::Log(LOGDEBUG, "~  %02d  (VPU)\n", m_idx);
#endif
}
