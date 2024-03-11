/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDClock.h"
#include "DebugRenderer.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "cores/VideoPlayer/VideoRenderers/OverlayRenderer.h"
#include "cores/VideoSettings.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/SystemClock.h"
#include "utils/Geometry.h"
#include "windowing/Resolution.h"

#include <atomic>
#include <deque>
#include <list>
#include <map>

#include "PlatformDefs.h"

class CRenderCapture;
struct VideoPicture;

class CWinRenderer;
class CLinuxRenderer;
class CLinuxRendererGL;
class CLinuxRendererGLES;
class CRenderManager;

class IRenderMsg
{
  friend CRenderManager;
public:
  virtual ~IRenderMsg() = default;
protected:
  virtual void VideoParamsChange() = 0;
  virtual void GetDebugInfo(std::string &audio, std::string &video, std::string &general) = 0;
  virtual void UpdateClockSync(bool enabled) = 0;
  virtual void UpdateRenderInfo(CRenderInfo &info) = 0;
  virtual void UpdateRenderBuffers(int queued, int discard, int free) = 0;
  virtual void UpdateGuiRender(bool gui) = 0;
  virtual void UpdateVideoRender(bool video) = 0;
  virtual CVideoSettings GetVideoSettings() const = 0;
};

class CRenderManager
{
public:
  CRenderManager(CDVDClock &clock, IRenderMsg *player);
  virtual ~CRenderManager();

  // Functions called from render thread
  void GetVideoRect(CRect& source, CRect& dest, CRect& view) const;
  float GetAspectRatio() const;
  unsigned int GetOrientation() const;
  void FrameMove();
  void FrameWait(std::chrono::milliseconds duration);
  void Render(bool clear, DWORD flags = 0, DWORD alpha = 255, bool gui = true);
  bool IsVideoLayer();
  RESOLUTION GetResolution();
  void UpdateResolution();
  void TriggerUpdateResolution(float fps, int width, int height, std::string &stereomode);
  void SetViewMode(int iViewMode);
  void PreInit();
  void UnInit();
  bool Flush(bool wait, bool saveBuffers);
  bool IsConfigured() const;
  void ToggleDebug();
  void ToggleDebugVideo();

  /*!
   * \brief Set the subtitle vertical position,
   * it depends on current screen resolution
   * \param value The subtitle position in pixels
   * \param save If true, the value will be saved to resolution info
   */
  void SetSubtitleVerticalPosition(const int value, bool save);

  unsigned int AllocRenderCapture();
  void ReleaseRenderCapture(unsigned int captureId);
  void StartRenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags);
  bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size);

  // Functions called from GUI
  bool Supports(ERENDERFEATURE feature) const;
  bool Supports(ESCALINGMETHOD method) const;

  int GetSkippedFrames()  { return m_QueueSkip; }

  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation, int buffers = 0);
  bool AddVideoPicture(const VideoPicture& picture, volatile std::atomic_bool& bStop, EINTERLACEMETHOD deintMethod, bool wait);
  void AddOverlay(std::shared_ptr<CDVDOverlay> o, double pts);
  void ShowVideo(bool enable);

  /**
   * If player uses buffering it has to wait for a buffer before it calls
   * AddVideoPicture and AddOverlay. It waits for max 50 ms before it returns -1
   * in case no buffer is available. Player may call this in a loop and decides
   * by itself when it wants to drop a frame.
   */
  int WaitForBuffer(volatile std::atomic_bool& bStop,
                    std::chrono::milliseconds timeout = std::chrono::milliseconds(100));

  /**
   * Can be called by player for lateness detection. This is done best by
   * looking at the end of the queue.
   */
  bool GetStats(int &lateframes, double &pts, int &queued, int &discard);

  /**
   * Video player call this on flush in oder to discard any queued frames
   */
  void DiscardBuffer();

  void SetDelay(int delay) { m_videoDelay = delay; }
  int GetDelay() { return m_videoDelay; }

  void SetVideoSettings(const CVideoSettings& settings);

protected:

  void PresentSingle(bool clear, DWORD flags, DWORD alpha);
  void PresentFields(bool clear, DWORD flags, DWORD alpha);
  void PresentBlend(bool clear, DWORD flags, DWORD alpha);

  void PrepareNextRender();
  bool IsPresenting();
  bool IsGuiLayer();

  bool Configure();
  void CreateRenderer();
  void DeleteRenderer();
  void ManageCaptures();

  void UpdateLatencyTweak();
  void CheckEnableClockSync();

  CBaseRenderer *m_pRenderer = nullptr;
  OVERLAY::CRenderer m_overlays;
  CDebugRenderer m_debugRenderer;
  mutable CCriticalSection m_statelock;
  CCriticalSection m_presentlock;
  CCriticalSection m_datalock;
  bool m_bTriggerUpdateResolution = false;
  bool m_bRenderGUI = true;
  bool m_renderedOverlay = false;
  bool m_renderDebug = false;
  bool m_renderDebugVideo = false;
  XbmcThreads::EndTime<> m_debugTimer;
  std::atomic_bool m_showVideo = {false};

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
    PRESENT_METHOD_BOB,
  };

  enum ERENDERSTATE
  {
    STATE_UNCONFIGURED = 0,
    STATE_CONFIGURING,
    STATE_CONFIGURED,
  };
  ERENDERSTATE m_renderState = STATE_UNCONFIGURED;
  CEvent m_stateEvent;

  /// Display latency tweak value from AdvancedSettings for the current refresh rate
  /// in milliseconds
  double m_latencyTweak = 0.0;
  /// Display latency updated in PrepareNextRender in DVD clock units, includes m_latencyTweak
  double m_displayLatency = 0.0;
  std::atomic_int m_videoDelay = {};

  int m_QueueSize = 2;
  int m_QueueSkip = 0;

  struct SPresent
  {
    double         pts;
    EFIELDSYNC     presentfield;
    EPRESENTMETHOD presentmethod;
  } m_Queue[NUM_BUFFERS]{};

  std::deque<int> m_free;
  std::deque<int> m_queued;
  std::deque<int> m_discard;

  std::unique_ptr<VideoPicture> m_pConfigPicture;
  unsigned int m_width = 0;
  unsigned int m_height = 0;
  unsigned int m_dwidth = 0;
  unsigned int m_dheight = 0;
  float m_fps = 0.0;
  unsigned int m_orientation = 0;
  int m_NumberBuffers = 0;
  std::string m_stereomode;

  int m_lateframes = -1;
  double m_presentpts = 0.0;
  EPRESENTSTEP m_presentstep = PRESENT_IDLE;
  XbmcThreads::EndTime<> m_presentTimer;
  bool m_forceNext = false;
  int m_presentsource = 0;
  int m_presentsourcePast = -1;
  XbmcThreads::ConditionVariable m_presentevent;
  CEvent m_flushEvent;
  CEvent m_initEvent;
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
  unsigned int m_captureWaitCounter = 0;
  //set to true when adding something to m_captures, set to false when m_captures is made empty
  //std::list::empty() isn't thread safe, using an extra bool will save a lock per render when no captures are requested
  bool m_hasCaptures = false;
};
