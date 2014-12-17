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

class CDVDVideoCodecIMXBuffer
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
  void                     Queue(VpuDecOutFrameInfo *frameInfo,
                                 CDVDVideoCodecIMXBuffer *previous);
  VpuDecRetCode            ReleaseFramebuffer(VpuDecHandle *handle);
  void                     SetPts(double pts);
  double                   GetPts(void) const;
  CDVDVideoCodecIMXBuffer *GetPreviousBuffer() const;

  uint32_t       m_iWidth;
  uint32_t       m_iHeight;
  uint8_t       *m_phyAddr;
  uint8_t       *m_VirtAddr;

private:
  // private because we are reference counted
  virtual                  ~CDVDVideoCodecIMXBuffer();

private:
#ifdef TRACE_FRAMES
  int                      m_idx;
#endif
  long                     m_refs;
  VpuFrameBuffer          *m_frameBuffer;
  bool                     m_rendered;
  double                   m_pts;
  CDVDVideoCodecIMXBuffer *m_previousBuffer; // Holds a the reference counted
                                             // previous buffer
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

  static void Enter();
  static void Leave();

protected:

  bool VpuOpen();
  bool VpuAllocBuffers(VpuMemInfo *);
  bool VpuFreeBuffers();
  bool VpuAllocFrameBuffers();
  int  VpuFindBuffer(void *frameAddr);

  static const int          m_extraVpuBuffers;   // Number of additional buffers for VPU
  static const int          m_maxVpuDecodeLoops; // Maximum iterations in VPU decoding loop
  static CCriticalSection   m_codecBufferLock;   // Lock to protect buffers handled
                                                 // by both decoding and rendering threads

  CDVDStreamInfo            m_hints;             // Hints from demuxer at stream opening
  const char               *m_pFormatName;       // Current decoder format name
  VpuDecOpenParam           m_decOpenParam;      // Parameters required to call VPU_DecOpen
  CDecMemInfo               m_decMemInfo;        // VPU dedicated memory description
  VpuDecHandle              m_vpuHandle;         // Handle for VPU library calls
  VpuDecInitInfo            m_initInfo;          // Initial info returned from VPU at decoding start
  bool                      m_dropState;         // Current drop state
  int                       m_vpuFrameBufferNum; // Total number of allocated frame buffers
  VpuFrameBuffer           *m_vpuFrameBuffers;   // Table of VPU frame buffers description
  CDVDVideoCodecIMXBuffer **m_outputBuffers;     // Table of VPU output buffers
  CDVDVideoCodecIMXBuffer  *m_lastBuffer;        // Keep track of previous VPU output buffer (needed by deinterlacing motion engin)
  VpuMemDesc               *m_extraMem;          // Table of allocated extra Memory
  int                       m_frameCounter;      // Decoded frames counter
  bool                      m_usePTS;            // State whether pts out of decoding process should be used
  VpuDecOutFrameInfo        m_frameInfo;         // Store last VPU output frame info
  CBitstreamConverter      *m_converter;         // H264 annex B converter
  bool                      m_convert_bitstream; // State whether bitstream conversion is required
  int                       m_bytesToBeConsumed; // Remaining bytes in VPU
  double                    m_previousPts;       // Enable to keep pts when needed
  bool                      m_frameReported;     // State whether the frame consumed event will be reported by libfslvpu
  double                    m_dts;               // Current dts
};
