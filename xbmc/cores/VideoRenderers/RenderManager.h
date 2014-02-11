#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <list>

#include "cores/VideoRenderers/BaseRenderer.h"
#include "guilib/Geometry.h"
#include "guilib/Resolution.h"
#include "threads/SharedSection.h"
#include "threads/Thread.h"
#include "settings/VideoSettings.h"
#include "OverlayRenderer.h"
#include <deque>
#include "PlatformDefs.h"

class CRenderCapture;

namespace DXVA { class CProcessor; }
namespace VAAPI { class CSurfaceHolder; }
namespace VDPAU { class CVdpauRenderPicture; }
struct DVDVideoPicture;

#define ERRORBUFFSIZE 30

class CWinRenderer;
class CLinuxRenderer;
class CLinuxRendererGL;
class CLinuxRendererGLES;

class CXBMCRenderManager
{
public:
  CXBMCRenderManager();
  ~CXBMCRenderManager();

  // Functions called from the GUI
  void GetVideoRect(CRect &source, CRect &dest);
  float GetAspectRatio();
  void Update();
  void FrameMove();
  void FrameFinish();
  bool FrameWait(int ms);
  void Render(bool clear, DWORD flags = 0, DWORD alpha = 255);
  void SetupScreenshot();

  CRenderCapture* AllocRenderCapture();
  void ReleaseRenderCapture(CRenderCapture* capture);
  void Capture(CRenderCapture *capture, unsigned int width, unsigned int height, int flags);
  void ManageCaptures();

  void SetViewMode(int iViewMode);

  // Functions called from mplayer
  /**
   * Called by video player to configure renderer
   * @param width width of decoded frame
   * @param height height of decoded frame
   * @param d_width displayed width of frame (aspect ratio)
   * @param d_height displayed height of frame
   * @param fps frames per second of video
   * @param flags see RenderFlags.h
   * @param format see RenderFormats.h
   * @param extended_format used by DXVA
   * @param orientation
   * @param numbers of kept buffer references
   */
  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format,  unsigned int orientation, int buffers = 0);
  bool IsConfigured() const;

  int AddVideoPicture(DVDVideoPicture& picture);

  /**
   * Called by video player to flip render buffers
   * If buffering is enabled this method does not block. In case of disabled buffering
   * this method blocks waiting for the render thread to pass by.
   * When buffering is used there might be no free buffer available after the call to
   * this method. Player has to call WaitForBuffer. A free buffer will become
   * available after the main thread has flipped front / back buffers.
   *
   * @param bStop reference to stop flag of calling thread
   * @param timestamp of frame delivered with AddVideoPicture
   * @param pts used for lateness detection
   * @param source depreciated
   * @param sync signals frame, top, or bottom field
   */
  void FlipPage(volatile bool& bStop, double timestamp = 0.0, double pts = 0.0, int source = -1, EFIELDSYNC sync = FS_NONE);
  unsigned int PreInit();
  void UnInit();
  bool Flush();

  void AddOverlay(CDVDOverlay* o, double pts)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddOverlay(o, pts, m_free.front());
  }

  void AddCleanup(OVERLAY::COverlay* o)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddCleanup(o);
  }

  void Reset();

  RESOLUTION GetResolution();

  static float GetMaximumFPS();
  inline bool IsStarted() { return m_bIsStarted;}
  double GetDisplayLatency() { return m_displayLatency; }
  int    GetSkippedFrames()  { return m_QueueSkip; }

  bool Supports(ERENDERFEATURE feature);
  bool Supports(EDEINTERLACEMODE method);
  bool Supports(EINTERLACEMETHOD method);
  bool Supports(ESCALINGMETHOD method);

  EINTERLACEMETHOD AutoInterlaceMethod(EINTERLACEMETHOD mInt);

  static double GetPresentTime();
  void  WaitPresentTime(double presenttime);

  CStdString GetVSyncState();

  void UpdateResolution();

  bool RendererHandlesPresent() const;

#ifdef HAS_GL
  CLinuxRendererGL    *m_pRenderer;
#elif HAS_GLES == 2
  CLinuxRendererGLES  *m_pRenderer;
#elif defined(HAS_DX)
  CWinRenderer        *m_pRenderer;
#elif defined(HAS_SDL)
  CLinuxRenderer      *m_pRenderer;
#endif

  unsigned int GetProcessorSize();

  // Supported pixel formats, can be called before configure
  std::vector<ERenderFormat> SupportedFormats();

  void Recover(); // called after resolution switch if something special is needed

  CSharedSection& GetSection() { return m_sharedSection; };

  void RegisterRenderUpdateCallBack(const void *ctx, RenderUpdateCallBackFn fn);
  void RegisterRenderFeaturesCallBack(const void *ctx, RenderFeaturesCallBackFn fn);

  /**
   * If player uses buffering it has to wait for a buffer before it calls
   * AddVideoPicture and AddOverlay. It waits for max 50 ms before it returns -1
   * in case no buffer is available. Player may call this in a loop and decides
   * by itself when it wants to drop a frame.
   * If no buffering is requested in Configure, player does not need to call this,
   * because FlipPage will block.
   */
  int WaitForBuffer(volatile bool& bStop, int timeout = 100);

  /**
   * Can be called by player for lateness detection. This is done best by
   * looking at the end of the queue.
   */
  bool GetStats(double &sleeptime, double &pts, int &bufferLevel);

  /**
   * Video player call this on flush in oder to discard any queued frames
   */
  void DiscardBuffer();

protected:

  void PresentSingle(bool clear, DWORD flags, DWORD alpha);
  void PresentFields(bool clear, DWORD flags, DWORD alpha);
  void PresentBlend(bool clear, DWORD flags, DWORD alpha);

  void PrepareNextRender();

  EINTERLACEMETHOD AutoInterlaceMethodInternal(EINTERLACEMETHOD mInt);

  bool m_bIsStarted;
  CSharedSection m_sharedSection;

  bool m_bReconfigured;

  int m_rendermethod;

  enum EPRESENTSTEP
  {
    PRESENT_IDLE     = 0
  , PRESENT_FLIP
  , PRESENT_FRAME
  , PRESENT_FRAME2
  , PRESENT_READY
  };

  enum EPRESENTMETHOD
  {
    PRESENT_METHOD_SINGLE = 0,
    PRESENT_METHOD_BLEND,
    PRESENT_METHOD_WEAVE,
    PRESENT_METHOD_BOB,
  };

  double m_displayLatency;
  void UpdateDisplayLatency();

  int m_QueueSize;
  int m_QueueSkip;

  struct SPresent
  {
    double         pts;
    double         timestamp;
    EFIELDSYNC     presentfield;
    EPRESENTMETHOD presentmethod;
  } m_Queue[NUM_BUFFERS];

  std::deque<int> m_free;
  std::deque<int> m_queued;
  std::deque<int> m_discard;

  ERenderFormat   m_format;

  double     m_sleeptime;
  double     m_presentpts;
  double     m_presentcorr;
  double     m_presenterr;
  double     m_errorbuff[ERRORBUFFSIZE];
  int        m_errorindex;
  EPRESENTSTEP     m_presentstep;
  int        m_presentsource;
  XbmcThreads::ConditionVariable  m_presentevent;
  CCriticalSection m_presentlock;
  CEvent     m_flushEvent;
  double     m_clock_framefinish;


  OVERLAY::CRenderer m_overlays;

  void RenderCapture(CRenderCapture* capture);
  void RemoveCapture(CRenderCapture* capture);
  CCriticalSection           m_captCritSect;
  std::list<CRenderCapture*> m_captures;
  //set to true when adding something to m_captures, set to false when m_captures is made empty
  //std::list::empty() isn't thread safe, using an extra bool will save a lock per render when no captures are requested
  bool                       m_hasCaptures; 

  // temporary fix for RendererHandlesPresent after #2811
  bool m_firstFlipPage;
};

extern CXBMCRenderManager g_renderManager;
