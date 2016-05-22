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

#include "linux/imx/IMX.h"

#include "threads/CriticalSection.h"
#include "threads/Condition.h"
#include "threads/Thread.h"
#include "utils/BitstreamConverter.h"
#include "guilib/Geometry.h"
#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "guilib/DispResource.h"
#include "DVDClock.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"

#include <linux/ipu.h>
#include <linux/mxcfb.h>
#include <imx-mm/vpu/vpu_wrapper.h>
#include <g2d.h>

#include <unordered_map>
#include <cstring>
#include <stdlib.h>

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
#define IMX_PROFILE_BUFFERS

#define IMX_PROFILE
//#define TRACE_FRAMES

#define RENDER_USE_G2D 0

// If uncommented a file "stream.dump" will be created in the current
// directory whenever a new stream is started. This is only for debugging
// and performance tests. This define must never be active in distributions.
//#define DUMP_STREAM

inline
double recalcPts(double pts)
{
  return (double)(pts == DVD_NOPTS_VALUE ? 0.0 : pts*1e-6);
}

enum SIGNALS
{
  SIGNAL_RESET         = (1 << 0),
  SIGNAL_DISPOSE       = (1 << 1),
  SIGNAL_SIGNAL        = (1 << 2),
  SIGNAL_FLUSH         = (1 << 3),
};

enum RENDER_TASK
{
  RENDER_TASK_AUTOPAGE = -1,
  RENDER_TASK_CAPTURE  = -2,
};

#define CLASS_PICTURE   (VPU_DEC_OUTPUT_DIS     | VPU_DEC_OUTPUT_MOSAIC_DIS)
#define CLASS_NOBUF     (VPU_DEC_OUTPUT_NODIS   | VPU_DEC_NO_ENOUGH_BUF | VPU_DEC_OUTPUT_REPEAT)
#define CLASS_FORCEBUF  (VPU_DEC_OUTPUT_EOS     | VPU_DEC_NO_ENOUGH_INBUF)
#define CLASS_DROP      (VPU_DEC_OUTPUT_DROPPED | VPU_DEC_SKIP)

// iMX context class that handles all iMX hardware
// related stuff
class CIMXContext : private CThread, IDispResource
{
public:
  CIMXContext();
  ~CIMXContext();

  bool AdaptScreen(bool allocate = false);
  bool TaskRestart();
  void CloseDevices();
  void g2dCloseDevices();
  void g2dOpenDevices();
  bool OpenDevices();

  bool Blank();
  bool Unblank();
  bool SetVSync(bool enable);

  // Blitter configuration
  bool IsDoubleRate() const { return m_currentFieldFmt & IPU_DEINTERLACE_RATE_EN; }
  void SetVideoPixelFormat(CProcessInfo *m_pProcessInfo);

  void SetBlitRects(const CRect &srcRect, const CRect &dstRect);

  // Blits a buffer to a particular page (-1 for auto page)
  // source_p (previous buffer) is required for de-interlacing
  // modes LOW_MOTION and MED_MOTION.
  void Blit(CIMXBuffer *source_p, CIMXBuffer *source,
            uint8_t fieldFmt = 0, int targetPage = RENDER_TASK_AUTOPAGE);

  // Shows a page vsynced
  bool ShowPage();
  void WaitVSync();

  // Clears the pages or a single page with 'black'
  void Clear(int page = RENDER_TASK_AUTOPAGE);

  // Captures the current visible frame buffer page and blends it into
  // the passed overlay. The buffer format is BGRA (4 byte)
  bool CaptureDisplay(unsigned char *&buffer, int iWidth, int iHeight, bool blend = false);

  void OnResetDisplay();
  void OnLostDisplay();

  void create() { Create(); m_onStartup.Wait(); }

  static const int  m_fbPages;

private:
  struct IPUTask
  {
    void Assign(CIMXBuffer *buffer_p, CIMXBuffer *buffer)
    {
      previous = buffer_p;
      current = buffer;
    }

    void Zero()
    {
      current = previous = NULL;
      memset(&task, 0, sizeof(task));
    }

    // Kept for reference
    CIMXBuffer *previous;
    CIMXBuffer *current;

    // The actual task
    struct ipu_task task;

    unsigned int page;
    int shift = true;
  };

  typedef std::shared_ptr<struct IPUTask> IPUTaskPtr;

  bool GetFBInfo(const std::string &fbdev, struct fb_var_screeninfo *fbVar);

  void PrepareTask(IPUTaskPtr &ipu, CIMXBuffer *source_p, CIMXBuffer *source);
  bool DoTask(IPUTaskPtr &ipu, CRect *dest = nullptr);
  bool TileTask(IPUTaskPtr &ipu);

  void SetFieldData(uint8_t fieldFmt, double fps);

  void Dispose();
  void MemMap(struct fb_fix_screeninfo *fb_fix = NULL);
  void Stop(bool bWait = true);

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

private:
  lkFIFO<IPUTaskPtr>             m_input;
  std::vector<bool>              m_flip;

  int                            m_fbHandle;
  std::atomic<int>               m_fbCurrentPage;
  int                            m_pg;
  int                            m_fbWidth;
  int                            m_fbHeight;
  int                            m_fbLineLength;
  int                            m_fbPageSize;
  int                            m_fbPhysSize;
  int                            m_fbPhysAddr;
  uint8_t                       *m_fbVirtAddr;
  struct fb_var_screeninfo       m_fbVar;
  int                            m_ipuHandle;
  uint8_t                        m_currentFieldFmt;
  bool                           m_vsync;
  CRect                          m_srcRect;
  CRect                          m_dstRect;
  CRectInt                      *m_pageCrops;
  bool                           m_bFbIsConfigured;
  CEvent                         m_waitVSync;
  CEvent                         m_onStartup;
  CEvent                         m_waitFlip;
  CProcessInfo                  *m_processInfo;

  CCriticalSection               m_pageSwapLock;
public:
  void                          *m_g2dHandle;
  struct g2d_buf                *m_bufferCapture;

  std::string                    m_deviceName;
  int                            m_speed;

  double                         m_fps;
};


extern CIMXContext g_IMXContext;

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

class CDVDVideoCodecIMX;
class CIMXCodec;

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
class CDVDVideoCodecIMXBuffer : public CIMXBuffer
{
friend class CIMXCodec;
friend class CIMXContext;
public:
  CDVDVideoCodecIMXBuffer(VpuDecOutFrameInfo *frameInfo, double fps, int map);
  virtual ~CDVDVideoCodecIMXBuffer();

  // reference counting
  virtual void Lock();
  virtual long Release();

  void                  SetPts(double pts)      { m_pts = pts; }
  double                GetPts() const          { return m_pts; }

  void                  SetDts(double dts)      { m_dts = dts; }
  double                GetDts() const          { return m_dts; }

  void                  SetFlags(int flags)     { m_iFlags = flags; }
  int                   GetFlags() const        { return m_iFlags; }

#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS) || defined(TRACE_FRAMES)
  int                   GetIdx()                { return m_idx; }
#endif
  VpuFieldType          GetFieldType() const    { return m_fieldType; }

protected:
  unsigned int             m_pctWidth;
  unsigned int             m_pctHeight;

private:
  double                   m_pts;
  double                   m_dts;
  VpuFieldType             m_fieldType;
  VpuFrameBuffer          *m_frameBuffer;
  int                      m_iFlags;
#if defined(IMX_PROFILE) || defined(IMX_PROFILE_BUFFERS) || defined(TRACE_FRAMES)
  unsigned char            m_idx;
  static unsigned char     i;
#endif

public:
  struct g2d_buf          *m_convBuffer;
};

class CIMXCodec : public CThread
{
public:
  CIMXCodec();
  ~CIMXCodec();

  bool                  Open(CDVDStreamInfo &hints, CDVDCodecOptions &options, std::string &m_pFormatName, CProcessInfo *m_pProcessInfo);
  int                   Decode(BYTE *pData, int iSize, double dts, double pts);

  void                  SetDropState(bool bDrop);

  void                  Reset();

  void                  SetSpeed(int iSpeed)                    { m_speed = iSpeed; }
  void                  WaitStartup()                           { m_loaded.Wait(); }

  bool                  GetPicture(DVDVideoPicture *pDvdVideoPicture);

  bool                  GetCodecStats(double &pts, int &droppedFrames, int &skippedPics);
  void                  SetCodecControl(int flags);

  virtual void Process() override;

  static void           ReleaseFramebuffer(VpuFrameBuffer* fb);

protected:
  class VPUTask
  {
  public:
    VPUTask(DemuxPacket pkg = { nullptr, 0, 0, 0, 0, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE, 0, 0 },
            CBitstreamConverter *cnv = nullptr) : demux(pkg)
    {
      if (IsEmpty())
        return;

      bool cok = false;
      if (cnv && (cok = cnv->Convert(pkg.pData, pkg.iSize)))
        demux.iSize = cnv->GetConvertSize();

      posix_memalign((void**)&demux.pData, 1024, demux.iSize);
      std::memcpy(demux.pData, cok ? cnv->GetConvertBuffer() : pkg.pData, demux.iSize);
    }

    void Release()
    {
      if (!IsEmpty())
        free(demux.pData);
      demux.pData = nullptr;
    }

    bool IsEmpty() { return !demux.pData; }

    DemuxPacket demux;
  };

  bool VpuOpen();
  bool VpuAllocBuffers(VpuMemInfo *);
  bool VpuFreeBuffers(bool dispose = true);
  bool VpuAllocFrameBuffers();

  void SetVPUParams(VpuDecConfig InDecConf, void* pInParam);
  void SetDrainMode(VpuDecInputType drop);
  void SetSkipMode(VpuDecSkipMode skip);

  void RecycleFrameBuffers();

  static void Release(VPUTask *&t)                     { SAFE_RELEASE(t); }
  static void Release(CDVDVideoCodecIMXBuffer *&t)     { SAFE_RELEASE(t); }
  static bool noDTS(VPUTask *&t)                       { return t->demux.dts == 0.0; }

  lkFIFO<VPUTask*>             m_decInput;
  lkFIFO<CDVDVideoCodecIMXBuffer*>
                               m_decOutput;

  static const unsigned int    m_extraVpuBuffers;   // Number of additional buffers for VPU
                                                    // by both decoding and rendering threads
  CDVDStreamInfo               m_hints;             // Hints from demuxer at stream opening

  VpuDecOpenParam              m_decOpenParam;      // Parameters required to call VPU_DecOpen
  CDecMemInfo                  m_decMemInfo;        // VPU dedicated memory description
  VpuDecHandle                 m_vpuHandle;         // Handle for VPU library calls
  VpuDecInitInfo               m_initInfo;          // Initial info returned from VPU at decoding start
  VpuDecSkipMode               m_skipMode;          // Current drop state
  VpuDecInputType              m_drainMode;
  int                          m_dropped;
  bool                         m_dropRequest;

  std::vector<VpuFrameBuffer>  m_vpuFrameBuffers;   // Table of VPU frame buffers description
  std::unordered_map<VpuFrameBuffer*,double>
                               m_pts;
  double                       m_lastPTS;
  VpuDecOutFrameInfo           m_frameInfo;         // Store last VPU output frame info
  CBitstreamConverter         *m_converter;         // H264 annex B converter
  bool                         m_warnOnce;          // Track warning messages to only warn once
  int                          m_codecControlFlags;
  int                          m_speed;
  CCriticalSection             m_signalLock;
  CCriticalSection             m_queuesLock;
#ifdef DUMP_STREAM
  FILE                        *m_dump;
#endif

private:
  bool                         IsDraining()             { return m_drainMode || m_codecControlFlags & DVD_CODEC_CTRL_DRAIN; }
  bool                         EOS()                    { return m_decRet & VPU_DEC_OUTPUT_EOS; }
  bool                         FBRegistered()           { return m_vpuFrameBuffers.size(); }

  bool                         getOutputFrame(VpuDecOutFrameInfo *frm);
  void                         ProcessSignals(int signal = 0);
  void                         AddExtraData(VpuBufferNode *bn, bool force = false);

  bool                         VpuAlloc(VpuMemDesc *vpuMem);

  void                         DisposeDecQueues();
  void                         FlushVPU();

  void                         Dispose();

  unsigned int                 m_decSignal;
  ThreadIdentifier             m_threadID;
  CEvent                       m_loaded;
  int                          m_decRet;
  double                       m_fps;
  unsigned int                 m_burst;
  bool                         m_requestDrop;
  CProcessInfo                *m_processInfo;

private:
  void                         ExitError(const char *msg, ...);
  bool                         IsCurrentThread() const;

  CCriticalSection             m_openLock;
};


/*
 *
 *  CDVDVideoCodec only wraps IMXCodec class
 *
 */
class CDVDVideoCodecIMX : public CDVDVideoCodec
{
public:
  CDVDVideoCodecIMX(CProcessInfo &processInfo) : CDVDVideoCodec(processInfo), m_pFormatName("iMX-xxx") {}
  virtual ~CDVDVideoCodecIMX();

  // Methods from CDVDVideoCodec which require overrides
  virtual bool          Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual bool          ClearPicture(DVDVideoPicture *pDvdVideoPicture);

  virtual int           Decode(BYTE *pData, int iSize, double dts, double pts)  { return m_IMXCodec->Decode(pData, iSize, dts, pts); }

  virtual void          Reset()                                                 { m_IMXCodec->Reset(); }
  virtual const char*   GetName()                                               { return (const char*)m_pFormatName.c_str(); }

  virtual bool          GetPicture(DVDVideoPicture *pDvdVideoPicture)           { return m_IMXCodec->GetPicture(pDvdVideoPicture); }
  virtual void          SetDropState(bool bDrop)                                { m_IMXCodec->SetDropState(bDrop); }
  virtual unsigned      GetAllowedReferences();

  virtual bool          GetCodecStats(double &pts, int &droppedFrames, int &skippedPics) override
                                                                                { return m_IMXCodec->GetCodecStats(pts, droppedFrames, skippedPics); }
  virtual void          SetCodecControl(int flags) override                     { m_IMXCodec->SetCodecControl(flags); }
  virtual void          SetSpeed(int iSpeed)                                    { m_IMXCodec->SetSpeed(iSpeed); }

private:
  std::shared_ptr<CIMXCodec> m_IMXCodec;

  std::string           m_pFormatName;       // Current decoder format name
};

