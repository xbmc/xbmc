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

#include "threads/CriticalSection.h"
#include "threads/Condition.h"
#include "threads/Thread.h"
#include "utils/BitstreamConverter.h"
#include "guilib/Geometry.h"
#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"

#include <vector>
#include <linux/ipu.h>
#include <linux/mxcfb.h>
#include <imx-mm/vpu/vpu_wrapper.h>
#include <g2d.h>


// The decoding format of the VPU buffer. Comment this to decode
// as NV12. The VPU works faster with NV12 in combination with
// deinterlacing.
// Progressive content seems to be handled faster with I420 whereas
// interlaced content is processed faster with NV12 as output format.
//#define IMX_INPUT_FORMAT_I420

// This enables logging of times for Decode, Render->Render,
// Deinterlace. It helps to profile several stages of
// processing with respect to changed kernels or other configurations.
// Since we utilize VPU, IPU and GPU at the same time different kernel
// priorities to those subsystems can result in a very different user
// experience. With that setting enabled we can build some statistics,
// as numbers are always better than "feelings"
//#define IMX_PROFILE_BUFFERS

//#define IMX_PROFILE
//#define TRACE_FRAMES

// If uncommented a file "stream.dump" will be created in the current
// directory whenever a new stream is started. This is only for debugging
// and performance tests. This define must never be active in distributions.
//#define DUMP_STREAM


/*>> TO BE MOVED TO A MORE CENTRAL PLACE IN THE SOURCE DIR >>>>>>>>>>>>>>>>>>>*/
// Generell description of a buffer used by
// the IMX context, e.g. for blitting
class CIMXBuffer {
public:
  CIMXBuffer() : m_iRefs(0) {}

  // Shared pointer interface
  virtual void Lock() = 0;
  virtual long Release() = 0;
  virtual bool IsValid() = 0;

  virtual void BeginRender() = 0;
  virtual void EndRender() = 0;

public:
  uint32_t     iWidth;
  uint32_t     iHeight;
  int          pPhysAddr;
  uint8_t     *pVirtAddr;
  int          iFormat;

protected:
  long         m_iRefs;
};


// iMX context class that handles all iMX hardware
// related stuff
class CIMXContext : private CThread
{
public:
  CIMXContext();
  ~CIMXContext();

  void RequireConfiguration() { m_checkConfigRequired = true; }
  bool Configure();
  bool Close();

  bool Blank();
  bool Unblank();
  bool SetVSync(bool enable);

  bool IsValid() const { return m_checkConfigRequired == false; }

  // Populates a CIMXBuffer with attributes of a page
  bool GetPageInfo(CIMXBuffer *info, int page);

  // Blitter configuration
  void SetDeInterlacing(bool flag);
  void SetDoubleRate(bool flag);
  bool DoubleRate() const;
  void SetInterpolatedFrame(bool flag);

  void SetBlitRects(const CRect &srcRect, const CRect &dstRect);

  // Blits a buffer to a particular page.
  // source_p (previous buffer) is required for de-interlacing
  // modes LOW_MOTION and MED_MOTION.
  bool Blit(int targetPage, CIMXBuffer *source_p,
            CIMXBuffer *source,
            bool topBottomFields = true);

  // Same as blit but runs in another thread and returns after the task has
  // been queued. BlitAsync renders always to the current backbuffer and
  // swaps the pages.
  bool BlitAsync(CIMXBuffer *source_p, CIMXBuffer *source,
                 bool topBottomFields = true, CRect *dest = NULL);

  // Shows a page vsynced
  bool ShowPage(int page);

  // Returns the visible page
  int  GetCurrentPage() const { return m_fbCurrentPage; }

  // Clears the pages or a single page with 'black'
  void Clear(int page = -1);

  // Captures the current visible frame buffer page and blends it into
  // the passed overlay. The buffer format is BGRA (4 byte)
  void CaptureDisplay(unsigned char *buffer, int iWidth, int iHeight);
  bool PushCaptureTask(CIMXBuffer *source, CRect *dest);
  void *GetCaptureBuffer() const { if (m_bufferCapture) return m_bufferCapture->buf_vaddr; else return NULL; }
  void WaitCapture();

private:
  struct IPUTask
  {
    void Zero()
    {
      current = previous = NULL;
      memset(&task, 0, sizeof(task));
    }

    void Done()
    {
      SAFE_RELEASE(previous);
      SAFE_RELEASE(current);
    }

    // Kept for reference
    CIMXBuffer *previous;
    CIMXBuffer *current;

    // The actual task
    struct ipu_task task;
  };

  bool PushTask(const IPUTask &);
  void PrepareTask(IPUTask &ipu, CIMXBuffer *source_p, CIMXBuffer *source,
                   bool topBottomFields, CRect *dest = NULL);
  bool DoTask(IPUTask &ipu, int targetPage);

  virtual void OnStartup();
  virtual void OnExit();
  virtual void StopThread(bool bWait = true);
  virtual void Process();

private:
  typedef std::vector<IPUTask> TaskQueue;

  int                            m_fbHandle;
  int                            m_fbCurrentPage;
  int                            m_fbWidth;
  int                            m_fbHeight;
  int                            m_fbLineLength;
  int                            m_fbPageSize;
  int                            m_fbPhysSize;
  int                            m_fbPhysAddr;
  uint8_t                       *m_fbVirtAddr;
  struct fb_var_screeninfo       m_fbVar;
  int                            m_ipuHandle;
  int                            m_currentFieldFmt;
  bool                           m_vsync;
  bool                           m_deInterlacing;
  CRect                          m_srcRect;
  CRect                          m_dstRect;
  CRectInt                       m_inputRect;
  CRectInt                       m_outputRect;
  CRectInt                      *m_pageCrops;

  CCriticalSection               m_pageSwapLock;
  TaskQueue                      m_input;
  volatile int                   m_beginInput, m_endInput;
  volatile size_t                m_bufferedInput;
  XbmcThreads::ConditionVariable m_inputNotEmpty;
  XbmcThreads::ConditionVariable m_inputNotFull;
  mutable CCriticalSection       m_monitor;

  void                           *m_g2dHandle;
  struct g2d_buf                 *m_bufferCapture;
  bool                           m_CaptureDone;
  bool                           m_checkConfigRequired;
  static const int               m_fbPages;
};


extern CIMXContext g_IMXContext;
/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/


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
  int         nVirtNum;
  void      **virtMem;

  //phy mem info
  int         nPhyNum;
  VpuMemDesc *phyMem;
};


// Base class of IMXVPU and IMXIPU buffer
class CDVDVideoCodecIMXBuffer : public CIMXBuffer {
public:
#ifdef TRACE_FRAMES
  CDVDVideoCodecIMXBuffer(int idx);
#else
  CDVDVideoCodecIMXBuffer();
#endif

  // reference counting
  virtual void Lock();
  virtual long Release();
  virtual bool IsValid();

  virtual void BeginRender();
  virtual void EndRender();

  void SetPts(double pts);
  double GetPts() const { return m_pts; }

  void SetDts(double dts);
  double GetDts() const { return m_dts; }

  bool Rendered() const;
  void Queue(VpuDecOutFrameInfo *frameInfo,
             CDVDVideoCodecIMXBuffer *previous);
  VpuDecRetCode ReleaseFramebuffer(VpuDecHandle *handle);
  CDVDVideoCodecIMXBuffer *GetPreviousBuffer() const { return m_previousBuffer; }
  VpuFieldType GetFieldType() const { return m_fieldType; }

private:
  // private because we are reference counted
  virtual ~CDVDVideoCodecIMXBuffer();

protected:
#ifdef TRACE_FRAMES
  int                      m_idx;
#endif

private:
  double                   m_pts;
  double                   m_dts;
  VpuFieldType             m_fieldType;
  VpuFrameBuffer          *m_frameBuffer;
  bool                     m_rendered;
  CDVDVideoCodecIMXBuffer *m_previousBuffer; // Holds the reference counted previous buffer
};


class CDVDVideoCodecIMX : public CDVDVideoCodec
{
public:
  CDVDVideoCodecIMX();
  virtual ~CDVDVideoCodecIMX();

  // Methods from CDVDVideoCodec which require overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int  Decode(BYTE *pData, int iSize, double dts, double pts);
  virtual void Reset();
  virtual bool ClearPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return (const char*)m_pFormatName; }
  virtual unsigned GetAllowedReferences();

  static void Enter();
  static void Leave();

protected:
  bool VpuOpen();
  bool VpuAllocBuffers(VpuMemInfo *);
  bool VpuFreeBuffers();
  bool VpuAllocFrameBuffers();
  int  VpuFindBuffer(void *frameAddr);

  static const int             m_extraVpuBuffers;   // Number of additional buffers for VPU
  static const int             m_maxVpuDecodeLoops; // Maximum iterations in VPU decoding loop
                                                    // by both decoding and rendering threads
  static CCriticalSection      m_codecBufferLock;   // Lock to protect buffers handled
  CDVDStreamInfo               m_hints;             // Hints from demuxer at stream opening
  const char                  *m_pFormatName;       // Current decoder format name
  VpuDecOpenParam              m_decOpenParam;      // Parameters required to call VPU_DecOpen
  CDecMemInfo                  m_decMemInfo;        // VPU dedicated memory description
  VpuDecHandle                 m_vpuHandle;         // Handle for VPU library calls
  VpuDecInitInfo               m_initInfo;          // Initial info returned from VPU at decoding start
  bool                         m_dropState;         // Current drop state
  int                          m_vpuFrameBufferNum; // Total number of allocated frame buffers
  VpuFrameBuffer              *m_vpuFrameBuffers;   // Table of VPU frame buffers description
  CDVDVideoCodecIMXBuffer    **m_outputBuffers;     // Table of VPU output buffers
  CDVDVideoCodecIMXBuffer     *m_lastBuffer;        // Keep track of previous VPU output buffer (needed by deinterlacing motion engin)
  CDVDVideoCodecIMXBuffer     *m_currentBuffer;
  VpuMemDesc                  *m_extraMem;          // Table of allocated extra Memory
  int                          m_frameCounter;      // Decoded frames counter
  bool                         m_usePTS;            // State whether pts out of decoding process should be used
  VpuDecOutFrameInfo           m_frameInfo;         // Store last VPU output frame info
  CBitstreamConverter         *m_converter;         // H264 annex B converter
  bool                         m_convert_bitstream; // State whether bitstream conversion is required
  int                          m_bytesToBeConsumed; // Remaining bytes in VPU
  double                       m_previousPts;       // Enable to keep pts when needed
  bool                         m_frameReported;     // State whether the frame consumed event will be reported by libfslvpu
  bool                         m_warnOnce;          // Track warning messages to only warn once
#ifdef DUMP_STREAM
  FILE                        *m_dump;
#endif
};
