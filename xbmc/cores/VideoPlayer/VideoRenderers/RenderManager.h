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

#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "cores/VideoPlayer/VideoRenderers/OverlayRenderer.h"
#include "guilib/Geometry.h"
#include "guilib/Resolution.h"
#include "threads/CriticalSection.h"
#include "settings/VideoSettings.h"
#include "OverlayRenderer.h"
#include "DebugRenderer.h"
#include <deque>
#include <map>
#include <atomic>
#include "PlatformDefs.h"
#include "threads/Event.h"
#include "DVDClock.h"

class CRenderCapture;

namespace DXVA { class CProcessor; }
namespace VAAPI { class CSurfaceHolder; }
namespace VDPAU { class CVdpauRenderPicture; }
struct DVDVideoPicture;

class CWinRenderer;
class CMMALRenderer;
class CLinuxRenderer;
class CLinuxRendererGL;
class CLinuxRendererGLES;
class CRenderManager;

class IRenderMsg
{
  friend CRenderManager;
protected:
  virtual void VideoParamsChange() = 0;
  virtual void GetDebugInfo(std::string &audio, std::string &video, std::string &general) = 0;
  virtual void UpdateClockSync(bool enabled) = 0;
  virtual void UpdateDeinterlacingMethods(std::list<EINTERLACEMETHOD> &methods) = 0;
};

class CRenderManager
{
public:
  CRenderManager(CDVDClock &clock, IRenderMsg *player);
  ~CRenderManager();

  // Functions called from render thread
  void GetVideoRect(CRect &source, CRect &dest, CRect &view);
  float GetAspectRatio();
  void FrameMove();
  void FrameWait(int ms);
  bool HasFrame();
  void Render(bool clear, DWORD flags = 0, DWORD alpha = 255, bool gui = true);
  bool IsGuiLayer();
  bool IsVideoLayer();
  RESOLUTION GetResolution();
  void UpdateResolution();
  void TriggerUpdateResolution(float fps, int width, int flags);
  void SetViewMode(int iViewMode);
  void PreInit();
  void UnInit();
  bool Flush();
  bool IsConfigured() const;
  void ToggleDebug();

  unsigned int AllocRenderCapture();
  void ReleaseRenderCapture(unsigned int captureId);
  void StartRenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags);
  bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size);

  // Functions called from GUI
  bool Supports(ERENDERFEATURE feature);
  bool Supports(EINTERLACEMETHOD method);
  bool Supports(ESCALINGMETHOD method);
  EINTERLACEMETHOD AutoInterlaceMethod(EINTERLACEMETHOD mInt);

  int GetSkippedFrames()  { return m_QueueSkip; }

  // Functions called from mplayer
  /**
   * Called by video player to configure renderer
   * @param picture
   * @param fps frames per second of video
   * @param flags see RenderFlags.h
   * @param orientation
   * @param numbers of kept buffer references
   */
  bool Configure(DVDVideoPicture& picture, float fps, unsigned flags, unsigned int orientation, int buffers = 0);

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
  void FlipPage(volatile std::atomic_bool& bStop, double pts = 0.0, int source = -1, EFIELDSYNC sync = FS_NONE);

  void AddOverlay(CDVDOverlay* o, double pts);

  // Get renderer info, can be called before configure
  CRenderInfo GetRenderInfo();

  /**
   * If player uses buffering it has to wait for a buffer before it calls
   * AddVideoPicture and AddOverlay. It waits for max 50 ms before it returns -1
   * in case no buffer is available. Player may call this in a loop and decides
   * by itself when it wants to drop a frame.
   * If no buffering is requested in Configure, player does not need to call this,
   * because FlipPage will block.
   */
  int WaitForBuffer(volatile std::atomic_bool& bStop, int timeout = 100);

  /**
   * Can be called by player for lateness detection. This is done best by
   * looking at the end of the queue.
   */
  bool GetStats(int &lateframes, double &pts, int &queued, int &discard);

  /**
   * Video player call this on flush in oder to discard any queued frames
   */
  void DiscardBuffer();

  void SetDelay(int delay) { m_videoDelay = delay; };
  int GetDelay() { return m_videoDelay; };

protected:

  void PresentSingle(bool clear, DWORD flags, DWORD alpha);
  void PresentFields(bool clear, DWORD flags, DWORD alpha);
  void PresentBlend(bool clear, DWORD flags, DWORD alpha);

  void PrepareNextRender();

  EINTERLACEMETHOD AutoInterlaceMethodInternal(EINTERLACEMETHOD mInt);
  bool Configure();
  void CreateRenderer();
  void DeleteRenderer();
  void ManageCaptures();

  void UpdateDisplayLatency();
  void CheckEnableClockSync();

  CBaseRenderer *m_pRenderer;
  OVERLAY::CRenderer m_overlays;
  CDebugRenderer m_debugRenderer;
  CCriticalSection m_statelock;
  CCriticalSection m_presentlock;
  CCriticalSection m_datalock;
  bool m_bTriggerUpdateResolution;
  bool m_bRenderGUI;
  int m_waitForBufferCount;
  int m_rendermethod;
  bool m_renderedOverlay;
  bool m_renderDebug;
  XbmcThreads::EndTime m_debugTimer;


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

  enum ERENDERSTATE
  {
    STATE_UNCONFIGURED = 0,
    STATE_CONFIGURING,
    STATE_CONFIGURED,
  };
  ERENDERSTATE m_renderState;
  CEvent m_stateEvent;

  double m_displayLatency;
  std::atomic_int m_videoDelay;

  int m_QueueSize;
  int m_QueueSkip;

  struct SPresent
  {
    double         pts;
    EFIELDSYNC     presentfield;
    EPRESENTMETHOD presentmethod;
  } m_Queue[NUM_BUFFERS];

  std::deque<int> m_free;
  std::deque<int> m_queued;
  std::deque<int> m_discard;

  ERenderFormat m_format;
  unsigned int m_width, m_height, m_dwidth, m_dheight;
  unsigned int m_flags;
  float m_fps;
  unsigned int m_extended_format;
  unsigned int m_orientation;
  int m_NumberBuffers;

  int m_lateframes;
  double m_presentpts;
  EPRESENTSTEP m_presentstep;
  int m_presentsource;
  XbmcThreads::ConditionVariable  m_presentevent;
  CEvent m_flushEvent;
  CDVDClock &m_dvdClock;
  IRenderMsg *m_playerPort;

  struct CClockSync
  {
    void Reset();
    double m_error;
    int m_errCount;
    double m_syncOffset;
    bool m_enabled;
  };
  CClockSync m_clockSync;

  void RenderCapture(CRenderCapture* capture);
  void RemoveCaptures();
  CCriticalSection m_captCritSect;
  std::map<unsigned int, CRenderCapture*> m_captures;
  static unsigned int m_nextCaptureId;
  unsigned int m_captureWaitCounter;
  //set to true when adding something to m_captures, set to false when m_captures is made empty
  //std::list::empty() isn't thread safe, using an extra bool will save a lock per render when no captures are requested
  bool m_hasCaptures;
};
