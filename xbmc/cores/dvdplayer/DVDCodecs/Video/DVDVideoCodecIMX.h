#pragma once
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
#include <queue>
#include <imx-mm/vpu/vpu_wrapper.h>
#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecInfo.h"
#include "threads/CriticalSection.h"
#include "utils/BitstreamConverter.h"


//#define IMX_PROFILE
//#define TRACE_FRAMES

class CDecMemInfo
{
public:
  CDecMemInfo()
    : nVirtNum(0)
    , virtMem(NULL)
    , nPhyNum(0)
    , phyMem(NULL)
  {}

  //virtual mem info
  int nVirtNum;
  void** virtMem;

  //phy mem info
  int nPhyNum;
  VpuMemDesc* phyMem;
};

class CDVDVideoCodecIPUBuffer;

class CDVDVideoCodecIMXBuffer : public CDVDVideoCodecBuffer
{
public:
#ifdef TRACE_FRAMES
  CDVDVideoCodecIMXBuffer(int idx);
#else
  CDVDVideoCodecIMXBuffer();
#endif

  // reference counting
  virtual void             Lock();
  virtual long             Release();
  virtual bool             IsValid();

  bool                     Rendered() const;
  void                     Queue(VpuDecOutFrameInfo *frameInfo);
  VpuDecRetCode            ReleaseFramebuffer(VpuDecHandle *handle);
  void                     SetPts(double pts);
  double                   GetPts(void) const;

protected:
  // private because we are reference counted
  virtual                  ~CDVDVideoCodecIMXBuffer();

#ifdef TRACE_FRAMES
  int                      m_idx;
#endif
  long                     m_refs;
  VpuFrameBuffer          *m_frameBuffer;
  bool                     m_rendered;
  double                   m_pts;
};

// Shared buffer that holds an IPU allocated memory block and serves as target
// for IPU operations such as deinterlacing, rotation or color conversion.
class CDVDVideoCodecIPUBuffer : public CDVDVideoCodecBuffer
{
public:
#ifdef TRACE_FRAMES
  CDVDVideoCodecIPUBuffer(int idx);
#else
  CDVDVideoCodecIPUBuffer();
#endif

  // reference counting
  virtual void             Lock();
  virtual long             Release();
  virtual bool             IsValid();

  // Returns whether the buffer is ready to be used
  bool                     Rendered() const { return m_source == NULL; }
  bool                     Process(int fd, CDVDVideoCodecBuffer *currentBuffer,
                                   CDVDVideoCodecBuffer *previousBuffer, VpuFieldType fieldType);
  void                     ReleaseFrameBuffer();

  bool                     Allocate(int fd, int width, int height, int nAlign);
  bool                     Free(int fd);

private:
  virtual                  ~CDVDVideoCodecIPUBuffer();

private:
#ifdef TRACE_FRAMES
  int                      m_idx;
#endif
  long                     m_refs;
  CDVDVideoCodecBuffer    *m_source;
  int                      m_pPhyAddr;
  uint8_t                 *m_pVirtAddr;
  int                      m_iWidth;
  int                      m_iHeight;
  int                      m_nSize;
};

// Collection class that manages a pool of IPU buffers that are used for
// deinterlacing. In future they can also serve rotation or color conversion
// buffers.
class CDVDVideoCodecIPUBuffers
{
  public:
    CDVDVideoCodecIPUBuffers();
    ~CDVDVideoCodecIPUBuffers();

    bool Init(int width, int height, int numBuffers, int nAlign);
    bool Reset();
    bool Close();

    CDVDVideoCodecIPUBuffer *Process(CDVDVideoCodecBuffer *sourceBuffer,
                                     VpuFieldType fieldType, bool lowMotion);

  private:
    int                       m_ipuHandle;
    int                       m_bufferNum;
    CDVDVideoCodecIPUBuffer **m_buffers;
    CDVDVideoCodecBuffer     *m_lastBuffer;
};


class CDVDVideoCodecIMX : public CDVDVideoCodec
{
  friend class CDVDVideoCodecIMXBuffer;
  friend class CDVDVideoCodecIPUBuffer;

public:
  CDVDVideoCodecIMX();
  virtual ~CDVDVideoCodecIMX();

  // Methods from CDVDVideoCodec which require overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose(void);
  virtual int  Decode(BYTE *pData, int iSize, double dts, double pts);
  virtual void Reset(void);
  virtual bool ClearPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }
  virtual unsigned GetAllowedReferences();

protected:

  bool VpuOpen();
  bool VpuAllocBuffers(VpuMemInfo *);
  bool VpuFreeBuffers();
  bool VpuAllocFrameBuffers();
  int  VpuFindBuffer(void *frameAddr);

  static const int    m_extraVpuBuffers;   // Number of additional buffers for VPU
  static const int    m_maxVpuDecodeLoops; // Maximum iterations in VPU decoding loop
  static CCriticalSection m_codecBufferLock;

  CDVDStreamInfo      m_hints;             // Hints from demuxer at stream opening
  const char         *m_pFormatName;       // Current decoder format name
  VpuDecOpenParam     m_decOpenParam;      // Parameters required to call VPU_DecOpen
  CDecMemInfo         m_decMemInfo;        // VPU dedicated memory description
  VpuDecHandle        m_vpuHandle;         // Handle for VPU library calls
  VpuDecInitInfo      m_initInfo;          // Initial info returned from VPU at decoding start
  bool                m_dropState;         // Current drop state
  int                 m_vpuFrameBufferNum; // Total number of allocated frame buffers
  VpuFrameBuffer     *m_vpuFrameBuffers;   // Table of VPU frame buffers description
  CDVDVideoCodecIPUBuffers  m_deinterlacer;
  CDVDVideoCodecIMXBuffer **m_outputBuffers;
  VpuMemDesc         *m_extraMem;          // Table of allocated extra Memory
//  VpuMemDesc         *m_outputBuffers;     // Table of buffers out of VPU (used to call properly VPU_DecOutFrameDisplayed)
  int                 m_frameCounter;      // Decoded frames counter
  bool                m_usePTS;            // State whether pts out of decoding process should be used
  int                 m_modeDeinterlace;   // Deinterlacer mode: 0=off, 1=high, 2..=low
  VpuDecOutFrameInfo  m_frameInfo;
  CBitstreamConverter *m_converter;
  bool                m_convert_bitstream;
  int                 m_bytesToBeConsumed; // Remaining bytes in VPU
  double              m_previousPts;       // Enable to keep pts when needed
  bool                m_frameReported;     // State whether the frame consumed event will be reported by libfslvpu
  double              m_dts;               // Current dts
};
