/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderManager.h"

/* to use the same as player */
#include "../VideoPlayer/DVDClock.h"
#include "RenderCapture.h"
#include "RenderFactory.h"
#include "RenderFlags.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "cores/DataCacheCore.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "guilib/GUIComponent.h"
#include "guilib/StereoscopicsManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/AMLUtils.h"
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <memory>
#include <mutex>

using namespace std::chrono_literals;

void CRenderManager::CClockSync::Reset()
{
  m_error = 0;
  m_errCount = 0;
  m_syncOffset = 0;
  m_enabled = false;
}

unsigned int CRenderManager::m_nextCaptureId = 0;

CRenderManager::CRenderManager(CDVDClock &clock, IRenderMsg *player) :
  m_dvdClock(clock),
  m_playerPort(player),
  m_dataCacheCore(CServiceBroker::GetDataCacheCore())
{
}

CRenderManager::~CRenderManager()
{
  delete m_pRenderer;
}

void CRenderManager::GetVideoRect(CRect& source, CRect& dest, CRect& view) const
{
  std::unique_lock<CCriticalSection> lock(m_statelock);
  if (m_pRenderer)
    m_pRenderer->GetVideoRect(source, dest, view);
}

float CRenderManager::GetAspectRatio() const
{
  std::unique_lock<CCriticalSection> lock(m_statelock);
  if (m_pRenderer)
    return m_pRenderer->GetAspectRatio();
  else
    return 1.0f;
}

void CRenderManager::SetVideoSettings(const CVideoSettings& settings) const {
  std::unique_lock<CCriticalSection> lock(m_statelock);

  if (m_pRenderer)
  {
    m_pRenderer->SetVideoSettings(settings);
  }
}

bool CRenderManager::Configure(const VideoPicture& picture, float fps, unsigned int orientation,
  StreamHdrType hdrType, int buffers)
{

  // check if something has changed
  {
    std::unique_lock<CCriticalSection> lock(m_statelock);

    if (!m_bRenderGUI)
      return true;

    if (m_picture.IsSameParams(picture) && m_fps == fps && m_orientation == orientation &&
        m_NumberBuffers == buffers && m_pRenderer != nullptr &&
        !m_pRenderer->ConfigChanged(picture))
    {
      return true;
    }
  }

  const std::string hdrStr = CStreamDetails::HdrTypeToString(picture.hdrType);
  CLog::Log(LOGDEBUG,
            "CRenderManager::Configure - change configuration. {}x{}. display: {}x{}. framerate: "
            "{:4.2f}. hdrType: {}.",
            picture.iWidth, picture.iHeight, picture.iDisplayWidth, picture.iDisplayHeight, fps,
            hdrStr.empty() ? "none" : hdrStr);

  // make sure any queued frame was fully presented
  {
    std::unique_lock<CCriticalSection> lock(m_presentlock);
    XbmcThreads::EndTime<> endtime(5000ms);
    m_forceNext = true;
    while (m_presentstep != PRESENT_IDLE)
    {
      if (endtime.IsTimePast())
      {
        CLog::Log(LOGWARNING, "CRenderManager::Configure - timeout waiting for state");
        m_forceNext = false;
        return false;
      }
      WaitPresent(lock, endtime.GetTimeLeft());
    }
    m_forceNext = false;
  }

  {
    std::unique_lock<CCriticalSection> lock(m_statelock);
    m_picture.SetParams(picture);
    m_fps = fps;
    m_orientation = orientation;
    m_NumberBuffers  = buffers;
    m_renderState = STATE_CONFIGURING;
    m_stateEvent.Reset();
    m_clockSync.Reset();
    m_dvdClock.SetVsyncAdjust(0);
    m_pConfigPicture = std::make_unique<VideoPicture>();
    m_pConfigPicture->CopyRef(picture);

    std::unique_lock<CCriticalSection> lock2(m_presentlock);
    m_presentstep = PRESENT_READY;
    NotifyPresentWaiters();
  }

  // Waiting on m_stateEvent returns immediately once the render thread finishes configuring.
  // Keep this per-attempt timeout short; higher-level code can retry for a bounded time window
  // during slow display mode switches (refresh rate / HDR / DV / AVR handshakes).
  auto configureWaitTimeout = 1000ms;

  if (!m_stateEvent.Wait(configureWaitTimeout))
  {
    CLog::Log(LOGWARNING, "CRenderManager::Configure - timeout waiting for configure");
    std::unique_lock<CCriticalSection> lock(m_statelock);

    return false;
  }

  std::unique_lock<CCriticalSection> lock(m_statelock);
  if (m_renderState != STATE_CONFIGURED)
  {
    CLog::Log(LOGWARNING, "CRenderManager::Configure - failed to configure");
    return false;
  }

  return true;
}

bool CRenderManager::Configure()
{
  // lock all interfaces
  std::unique_lock<CCriticalSection> lock(m_statelock);
  std::unique_lock<CCriticalSection> lock2(m_presentlock);
  std::unique_lock<CCriticalSection> lock3(m_datalock);

  if (m_pRenderer)
  {
    DeleteRenderer();
  }

  if (!m_pRenderer)
  {
    CreateRenderer();
    if (!m_pRenderer)
      return false;
  }

  m_pRenderer->SetVideoSettings(m_playerPort->GetVideoSettings());
  bool result = m_pRenderer->Configure(*m_pConfigPicture, m_fps, m_orientation);
  if (result)
  {
    CRenderInfo info = m_pRenderer->GetRenderInfo();
    int renderbuffers = info.max_buffer_size;
    m_QueueSize = renderbuffers;
    if (m_NumberBuffers > 0)
      m_QueueSize = std::min(m_NumberBuffers, renderbuffers);

    if (m_QueueSize < 2)
    {
      m_QueueSize = 2;
      CLog::Log(LOGWARNING, "CRenderManager::Configure - queue size too small ({}, {}, {})",
                m_QueueSize, renderbuffers, m_NumberBuffers);
    }

    m_pRenderer->SetBufferSize(m_QueueSize);
    m_pRenderer->Update();

    m_playerPort->UpdateRenderInfo(info);
    m_playerPort->UpdateGuiRender(true);
    m_playerPort->UpdateVideoRender(!m_pRenderer->IsGuiLayer());

    m_queued.clear();
    m_discard.clear();
    m_free.clear();
    m_presentstarted = false;
    m_presentsource = 0;
    m_presentsourcePast = -1;
    for (int i = 0; i < m_QueueSize; i++)
      m_free.push_back(i);

    m_bRenderGUI = true;
    m_bTriggerUpdateResolution = true;
    m_presentstep = PRESENT_IDLE;
    m_presentpts = DVD_NOPTS_VALUE;
    m_lateframes = -1;
    NotifyPresentWaiters();
    m_renderedOverlay = false;
    m_renderDebug = false;
    m_clockSync.Reset();
    m_dvdClock.SetVsyncAdjust(0);
    m_overlays.Reset();
    m_overlays.SetStereoMode(m_picture.stereoMode);

    m_renderState = STATE_CONFIGURED;

    UpdateResolution(true);

    CLog::Log(LOGDEBUG, "CRenderManager::Configure - {}", m_QueueSize);
  }
  else
    m_renderState = STATE_UNCONFIGURED;

  m_pConfigPicture.reset();

  m_stateEvent.Set();
  m_playerPort->VideoParamsChange();
  return result;
}

bool CRenderManager::IsConfigured() const
{
  std::unique_lock lock(m_statelock);
  return m_renderState == STATE_CONFIGURED;
}

void CRenderManager::ShowVideo(bool enable)
{
  m_showVideo = enable;
  if (!enable)
    DiscardBuffer();
}

void CRenderManager::FrameWait(std::chrono::milliseconds duration)
{
  XbmcThreads::EndTime<> timeout{duration};
  std::unique_lock lock(m_presentlock);
  while(m_presentstep == PRESENT_IDLE && !timeout.IsTimePast())
    WaitPresent(lock, timeout.GetTimeLeft());
}

bool CRenderManager::IsPresenting()
{
  if (!IsConfigured())
    return false;

  std::unique_lock lock(m_presentlock);
  return !m_presentTimer.IsTimePast();
}

void CRenderManager::FrameMove()
{
  bool firstFrame = false;
  UpdateResolution();

  {
    std::unique_lock<CCriticalSection> lock(m_statelock);

    if (m_renderState == STATE_UNCONFIGURED)
      return;
    else if (m_renderState == STATE_CONFIGURING)
    {
      lock.unlock();
      if (!Configure())
        return;

      firstFrame = true;
      FrameWait(50ms);
    }

    CheckEnableClockSync();
  }
  {
    std::unique_lock<CCriticalSection> lock2(m_presentlock);

    if (m_queued.empty())
    {
      m_presentstep = PRESENT_IDLE;
    }
    else
    {
      m_presentTimer.Set(1000ms);
    }

    if (m_presentstep == PRESENT_READY)
      PrepareNextRender();

    if (m_presentstep == PRESENT_FLIP)
    {
      m_presentstep = PRESENT_FRAME;
      NotifyPresentWaiters();
    }

    // release all previous
    for (std::deque<int>::iterator it = m_discard.begin(); it != m_discard.end(); )
    {
      // renderer may want to keep the frame for postprocessing
      if (!m_pRenderer->NeedBuffer(*it) || !m_bRenderGUI)
      {
        m_pRenderer->ReleaseBuffer(*it);
        m_overlays.Release(*it);
        m_free.push_back(*it);
        it = m_discard.erase(it);
      }
      else
        ++it;
    }

    m_playerPort->UpdateRenderBuffers(m_queued.size(), m_discard.size(), m_free.size());
    m_bRenderGUI = true;
  }

  m_playerPort->UpdateGuiRender(IsGuiLayer() || firstFrame);

  ManageCaptures();
}

void CRenderManager::PreInit()
{
  {
    std::unique_lock<CCriticalSection> lock(m_statelock);
    if (m_renderState != STATE_UNCONFIGURED)
      return;
  }

  if (!CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    m_initEvent.Reset();
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RENDERER_PREINIT);
    if (!m_initEvent.Wait(2000ms))
    {
      CLog::Log(LOGERROR, "{} - timed out waiting for renderer to preinit", __FUNCTION__);
    }
  }

  std::unique_lock<CCriticalSection> lock(m_statelock);

  if (!m_pRenderer)
  {
    CreateRenderer();
  }

  m_debugRenderer.Initialize();

  UpdateVideoLatencyTweak();

  m_QueueSize   = 2;
  m_QueueSkip   = 0;
  m_presentstep = PRESENT_IDLE;
  m_bRenderGUI = true;

  m_initEvent.Set();
}

void CRenderManager::UnInit()
{
  if (!CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    m_initEvent.Reset();
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RENDERER_UNINIT);
    if (!m_initEvent.Wait(2000ms))
    {
      CLog::Log(LOGERROR, "{} - timed out waiting for renderer to uninit", __FUNCTION__);
    }
  }

  std::unique_lock<CCriticalSection> lock(m_statelock);

  m_overlays.UnInit();
  m_debugRenderer.Dispose();

  DeleteRenderer();

  m_renderState = STATE_UNCONFIGURED;
  m_picture.Reset();
  m_bRenderGUI = false;
  CServiceBroker::GetWinSystem()->GetGfxContext().SetHDRType(m_picture.hdrType);
  RemoveCaptures();

  m_initEvent.Set();
}

bool CRenderManager::Flush(bool wait, bool saveBuffers)
{
  if (!m_pRenderer)
    return true;

  if (CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    CLog::Log(LOGDEBUG, "{} - flushing renderer", __FUNCTION__);

// fix deadlock on Windows only when is enabled 'Sync playback to display'
#ifndef TARGET_WINDOWS
    CSingleExit exitlock(CServiceBroker::GetWinSystem()->GetGfxContext());
#endif

    std::unique_lock<CCriticalSection> lock(m_statelock);
    std::unique_lock<CCriticalSection> lock2(m_presentlock);
    std::unique_lock<CCriticalSection> lock3(m_datalock);

    if (m_pRenderer)
    {
      m_overlays.Flush();
      m_debugRenderer.Flush();

      if (!m_pRenderer->Flush(saveBuffers))
      {
        m_queued.clear();
        m_discard.clear();
        m_free.clear();
        m_presentstarted = false;
        m_presentsource = 0;
        m_presentsourcePast = -1;
        m_presentstep = PRESENT_IDLE;
        for (int i = 0; i < m_QueueSize; i++)
          m_free.push_back(i);
      }

      m_flushEvent.Set();
    }
  }
  else
  {
    m_flushEvent.Reset();
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RENDERER_FLUSH);
    if (wait)
    {
      if (!m_flushEvent.Wait(1000ms))
      {
        CLog::Log(LOGERROR, "{} - timed out waiting for renderer to flush", __FUNCTION__);
        return false;
      }
      else
        return true;
    }
  }
  return true;
}

void CRenderManager::CreateRenderer()
{
  if (!m_pRenderer)
  {
    CVideoBuffer *buffer = nullptr;
    if (m_pConfigPicture)
      buffer = m_pConfigPicture->videoBuffer;

    auto renderers = VIDEOPLAYER::CRendererFactory::GetRenderers();
    for (auto &id : renderers)
    {
      if (id == "default")
        continue;

      m_pRenderer = VIDEOPLAYER::CRendererFactory::CreateRenderer(id, buffer);
      if (m_pRenderer)
      {
        return;
      }
    }
    m_pRenderer = VIDEOPLAYER::CRendererFactory::CreateRenderer("default", buffer);
  }
}

void CRenderManager::DeleteRenderer()
{
  if (m_pRenderer)
  {
    CLog::Log(LOGDEBUG, "{} - deleting renderer", __FUNCTION__);

    delete m_pRenderer;
    m_pRenderer = nullptr;
  }
}

unsigned int CRenderManager::AllocRenderCapture()
{
  if (m_pRenderer)
  {
    CRenderCapture* capture = m_pRenderer->GetRenderCapture();
    if (capture)
    {
      m_captures[m_nextCaptureId] = capture;
      return m_nextCaptureId++;
    }
  }

  return m_nextCaptureId;
}

void CRenderManager::ReleaseRenderCapture(unsigned int captureId)
{
  std::unique_lock<CCriticalSection> lock(m_captCritSect);

  std::map<unsigned int, CRenderCapture*>::iterator it;
  it = m_captures.find(captureId);

  if (it != m_captures.end())
    it->second->SetState(CAPTURESTATE_NEEDSDELETE);
}

void CRenderManager::StartRenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags)
{
  std::unique_lock<CCriticalSection> lock(m_captCritSect);

  std::map<unsigned int, CRenderCapture*>::iterator it;
  it = m_captures.find(captureId);
  if (it == m_captures.end())
  {
    CLog::Log(LOGERROR, "CRenderManager::Capture - unknown capture id: {}", captureId);
    return;
  }

  CRenderCapture *capture = it->second;

  capture->SetState(CAPTURESTATE_NEEDSRENDER);
  capture->SetUserState(CAPTURESTATE_WORKING);
  capture->SetWidth(width);
  capture->SetHeight(height);
  capture->SetFlags(flags);
  capture->GetEvent().Reset();

  if (CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    if (flags & CAPTUREFLAG_IMMEDIATELY)
    {
      // render capture and read out immediately
      RenderCapture(capture);
      capture->SetUserState(capture->GetState());
      capture->GetEvent().Set();
    }
  }

  if (!m_captures.empty())
    m_hasCaptures = true;
}

bool CRenderManager::RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size)
{
  std::unique_lock<CCriticalSection> lock(m_captCritSect);

  std::map<unsigned int, CRenderCapture*>::iterator it;
  it = m_captures.find(captureId);
  if (it == m_captures.end())
    return false;

  m_captureWaitCounter++;

  {
    if (!millis)
      millis = 1000;

    CSingleExit exitlock(m_captCritSect);
    if (!it->second->GetEvent().Wait(std::chrono::milliseconds(millis)))
    {
      m_captureWaitCounter--;
      return false;
    }
  }

  m_captureWaitCounter--;

  if (it->second->GetUserState() != CAPTURESTATE_DONE)
    return false;

  unsigned int srcSize = it->second->GetWidth() * it->second->GetHeight() * 4;
  unsigned int bytes = std::min(srcSize, size);

  memcpy(buffer, it->second->GetPixels(), bytes);
  return true;
}

void CRenderManager::ManageCaptures()
{
  // no captures, return here so we don't do an unnecessary lock
  if (!m_hasCaptures)
    return;

  std::unique_lock<CCriticalSection> lock(m_captCritSect);

  std::map<unsigned int, CRenderCapture*>::iterator it = m_captures.begin();
  while (it != m_captures.end())
  {
    CRenderCapture* capture = it->second;

    if (capture->GetState() == CAPTURESTATE_NEEDSDELETE)
    {
      delete capture;
      it = m_captures.erase(it);
      continue;
    }

    if (capture->GetState() == CAPTURESTATE_NEEDSRENDER)
      RenderCapture(capture);
    else if (capture->GetState() == CAPTURESTATE_NEEDSREADOUT)
      capture->ReadOut();

    if (capture->GetState() == CAPTURESTATE_DONE || capture->GetState() == CAPTURESTATE_FAILED)
    {
      // tell the thread that the capture is done or has failed
      capture->SetUserState(capture->GetState());
      capture->GetEvent().Set();

      if (capture->GetFlags() & CAPTUREFLAG_CONTINUOUS)
      {
        capture->SetState(CAPTURESTATE_NEEDSRENDER);

        // if rendering this capture continuously, and readout is async, render a new capture immediately
        if (capture->IsAsync() && !(capture->GetFlags() & CAPTUREFLAG_IMMEDIATELY))
          RenderCapture(capture);
      }
      ++it;
    }
    else
    {
      ++it;
    }
  }

  if (m_captures.empty())
    m_hasCaptures = false;
}

void CRenderManager::RenderCapture(CRenderCapture* capture) const {
  if (!m_pRenderer || !m_pRenderer->RenderCapture(m_presentsource, capture))
    capture->SetState(CAPTURESTATE_FAILED);
}

void CRenderManager::RemoveCaptures()
{
  std::unique_lock<CCriticalSection> lock(m_captCritSect);

  while (m_captureWaitCounter > 0)
  {
    for (auto entry : m_captures)
    {
      entry.second->GetEvent().Set();
    }
    CSingleExit lockexit(m_captCritSect);
    KODI::TIME::Sleep(10ms);
  }

  for (auto entry : m_captures)
  {
    delete entry.second;
  }
  m_captures.clear();
}

void CRenderManager::SetViewMode(int iViewMode) const {
  std::unique_lock<CCriticalSection> lock(m_statelock);

  if (m_pRenderer)
    m_pRenderer->SetViewMode(iViewMode);
  m_playerPort->VideoParamsChange();
}

RESOLUTION CRenderManager::GetResolution() const {
  RESOLUTION res = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();

  std::unique_lock<CCriticalSection> lock(m_statelock);
  if (m_renderState == STATE_UNCONFIGURED)
    return res;

  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
    res = CResolutionUtils::ChooseBestResolution(m_fps, m_picture.iWidth, m_picture.iHeight,
                                                 !m_picture.stereoMode.empty());

  return res;
}

void CRenderManager::Render(bool clear, DWORD flags, DWORD alpha, bool gui)
{
  CSingleExit exitLock(CServiceBroker::GetWinSystem()->GetGfxContext());

  {
    std::unique_lock<CCriticalSection> lock(m_statelock);

    if (!m_presentstarted || (m_renderState != STATE_CONFIGURED))
      return;
  }

  if (!gui && m_pRenderer->IsGuiLayer())
    return;

  const SPresent& present = m_Queue[m_presentsource];

  if (!gui || m_pRenderer->IsGuiLayer())
  {
    if (present.presentmethod == PRESENT_METHOD_BOB)
      PresentFields(clear, flags, alpha);
    else if (present.presentmethod == PRESENT_METHOD_BLEND)
      PresentBlend(clear, flags, alpha);
    else
      PresentSingle(clear, flags, alpha);
  }

  if (gui)
  {
    if (!m_pRenderer->IsGuiLayer())
      m_pRenderer->Update();

    const bool hasOverlay = m_overlays.HasOverlay(m_presentsource);
    m_renderedOverlay = hasOverlay;

    if (hasOverlay || m_renderDebug)
    {
      CRect src, dst, view;
      m_pRenderer->GetVideoRect(src, dst, view);

      if (hasOverlay)
      {
        m_overlays.SetVideoRect(src, dst, view);
        m_overlays.Render(m_presentsource);
      }

      if (m_renderDebug)
      {
        if (m_renderDebugVideo)
        {
          DEBUG_INFO_VIDEO video = m_pRenderer->GetDebugInfo(m_presentsource);
          DEBUG_INFO_RENDER render = CServiceBroker::GetWinSystem()->GetDebugInfo();

          m_debugRenderer.SetInfo(video, render);
        }
        else
        {
          DEBUG_INFO_PLAYER info;

          m_playerPort->GetDebugInfo(info.audio, info.video, info.player);

          double refreshrate, clockspeed;
          int missedvblanks;

          info.vsync = StringUtils::Format("VSync Off:{:.1f}", (m_clockSync.m_syncOffset / 1000));

          if (m_dvdClock.GetClockInfo(missedvblanks, clockspeed, refreshrate))
            info.vsync += StringUtils::Format("VSync: refresh:{:.3f} missed:{} speed:{:.3f}%",
                                              refreshrate, missedvblanks, (clockspeed * 100));

          double videoLatency = (m_videoLatencyTweak / 1000.0);
          double audioLatency = (m_audioLatencyTweak / 1000.0);
          double videoDelay = (-m_videoDelay / 1000.0);
          double totalLatency = videoLatency + audioLatency + videoDelay;

          info.latency = StringUtils::Format(
              "Latency: video:{:.3f} audio:{:.3f} user:{:.3f} total:{:.3f}", videoLatency, audioLatency,
              videoDelay, totalLatency);

          m_debugRenderer.SetInfo(info);
        }

        m_debugRenderer.Render(src, dst, view);

        m_debugTimer.Set(1000ms);
        m_renderedOverlay = true;
      }
    }
  }

  {
    std::unique_lock<CCriticalSection> lock(m_presentlock);

    if (m_presentstep == PRESENT_FRAME)
    {
      if (present.presentmethod == PRESENT_METHOD_BOB)
        m_presentstep = PRESENT_FRAME2;
      else
        m_presentstep = PRESENT_IDLE;
    }
    else if (m_presentstep == PRESENT_FRAME2)
      m_presentstep = PRESENT_IDLE;

    if (m_presentstep == PRESENT_IDLE)
    {
      if (!m_queued.empty())
        m_presentstep = PRESENT_READY;
    }

    NotifyPresentWaiters();
  }
}

bool CRenderManager::IsGuiLayer()
{
  std::unique_lock<CCriticalSection> lock(m_statelock);

  if (!m_pRenderer)
    return false;

  // Inline the IsPresenting() check to avoid:
  //  - recursive re-lock of m_statelock (via IsConfigured())
  //  - an extra m_presentlock acquisition when we'd return true via overlay/debug paths anyway
  if (m_pRenderer->IsGuiLayer() && m_renderState == STATE_CONFIGURED)
  {
    std::unique_lock<CCriticalSection> plock(m_presentlock);
    if (!m_presentTimer.IsTimePast())
      return true;
  }

  if (m_renderedOverlay || m_overlays.HasOverlay(m_presentsource))
    return true;

  if (m_renderDebug && m_debugTimer.IsTimePast())
    return true;

  return false;
}

bool CRenderManager::IsVideoLayer() const {
  {
    std::unique_lock<CCriticalSection> lock(m_statelock);

    if (!m_pRenderer)
      return false;

    if (!m_pRenderer->IsGuiLayer())
      return true;
  }
  return false;
}

/* simple present method */
void CRenderManager::PresentSingle(bool clear, DWORD flags, DWORD alpha)
{
  const SPresent& present = m_Queue[m_presentsource];

  if (present.presentfield == FS_BOT)
    m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_BOT, alpha);
  else if (present.presentfield == FS_TOP)
    m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_TOP, alpha);
  else
    m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags, alpha);
}

/* new simpler method of handling interlaced material, *
 * we just render the two fields right after each other */
void CRenderManager::PresentFields(bool clear, DWORD flags, DWORD alpha)
{
  const SPresent& present = m_Queue[m_presentsource];

  if (m_presentstep == PRESENT_FRAME)
  {
    if (present.presentfield == FS_BOT)
      m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_FIELD0, alpha);
    else
      m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_FIELD0, alpha);
  }
  else
  {
    if (present.presentfield == FS_TOP)
      m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_FIELD1, alpha);
    else
      m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_FIELD1, alpha);
  }
}

void CRenderManager::PresentBlend(bool clear, DWORD flags, DWORD alpha)
{
  const SPresent& present = m_Queue[m_presentsource];

  if (present.presentfield == FS_BOT)
  {
    m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_NOOSD, alpha);
    m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, false, flags | RENDER_FLAG_TOP, alpha / 2);
  }
  else
  {
    m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_NOOSD, alpha);
    m_pRenderer->RenderUpdate(m_presentsource, m_presentsourcePast, false, flags | RENDER_FLAG_BOT, alpha / 2);
  }
}

void CRenderManager::UpdateVideoLatencyTweak()
{
  float fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  const RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();

  float refresh = fps;
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution() == RES_WINDOW)
    refresh = 0; // No idea about refresh rate when windowed, just get the default latency

  m_videoLatencyTweak = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->GetVideoLatencyTweak(refresh, res.iScreenHeight);
}

void CRenderManager::UpdateResolution(bool force)
{
  std::unique_lock<CCriticalSection> lock(m_resolutionlock);
  if (m_bTriggerUpdateResolution)
  {
    if (force ||
        (CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() &&
         CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot()))
    {
      RENDER_STEREO_MODE user_stereo_mode =
        CServiceBroker::GetGUI()->GetStereoscopicsManager().GetStereoModeByUser();
      STEREOSCOPIC_PLAYBACK_MODE playbackMode =
        static_cast<STEREOSCOPIC_PLAYBACK_MODE>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE));
      if (!m_picture.stereoMode.empty() &&
          playbackMode == STEREOSCOPIC_PLAYBACK_MODE_ASK &&
          user_stereo_mode == RENDER_STEREO_MODE_UNDEFINED)
        m_bTriggerUpdateResolution = false;
      if (m_bTriggerUpdateResolution &&
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF && m_fps > 0.0f)
      {
        auto& gfxContext = CServiceBroker::GetWinSystem()->GetGfxContext();

        // Some platforms send a follow-up "reassert" trigger with no params (fps/width/height = 0)
        // after a mode switch/reset. In that case, prefer keeping the currently applied HDR type
        // (if any) to avoid an unnecessary second mode switch (e.g. VS10 HDR10->DV mapping).
        StreamHdrType desiredHdrType = (m_hdrType_override != StreamHdrType::HDR_TYPE_NONE)
                                           ? m_hdrType_override
                                           : m_picture.hdrType;
        if (m_hdrType_override == StreamHdrType::HDR_TYPE_NONE && m_bTriggerUpdateResolutionNoParams)
        {
          const auto currentHdrType = gfxContext.GetHDRType();
          if (currentHdrType != StreamHdrType::HDR_TYPE_NONE)
            desiredHdrType = currentHdrType;
        }

        const RESOLUTION desiredRes = CResolutionUtils::ChooseBestResolution(
            m_fps, m_picture.iWidth, m_picture.iHeight, !m_picture.stereoMode.empty());

        const bool needsApply = force || gfxContext.GetHDRType() != desiredHdrType ||
                                gfxContext.GetVideoResolution() != desiredRes;

        if (needsApply)
        {
          gfxContext.SetHDRType(desiredHdrType);
          gfxContext.SetVideoResolution(desiredRes, false);
          UpdateVideoLatencyTweak();

          if (m_pRenderer)
            m_pRenderer->Update();
        }
      }
      m_bTriggerUpdateResolution = false;
      m_bTriggerUpdateResolutionNoParams = false;
      m_hdrType_override = StreamHdrType::HDR_TYPE_NONE;
      m_playerPort->VideoParamsChange();
    }
  }
}

void CRenderManager::TriggerUpdateResolutionHdr(StreamHdrType hdrType)
{
  m_hdrType_override = hdrType;
  m_bTriggerUpdateResolution = true;
}

void CRenderManager::TriggerUpdateResolution(float fps, int width, int height, std::string &stereomode)
{
  m_bTriggerUpdateResolutionNoParams = (width == 0);
  if (width)
  {
    m_fps = fps;
    m_picture.iWidth = width;
    m_picture.iHeight = height;
    m_picture.stereoMode = stereomode;
  }
  m_bTriggerUpdateResolution = true;
}

void CRenderManager::ToggleDebug()
{
  m_renderDebug = !m_renderDebug;
  m_debugTimer.SetExpired();
  m_renderDebugVideo = false;
}

void CRenderManager::ToggleDebugVideo()
{
  m_renderDebug = !m_renderDebug;
  m_debugTimer.SetExpired();
  m_renderDebugVideo = true;
}

void CRenderManager::SetSubtitleVerticalPosition(int value, bool save)
{
  m_overlays.SetSubtitleVerticalPosition(value, save);
}

bool CRenderManager::AddVideoPicture(const VideoPicture& picture, volatile std::atomic_bool& bStop, EINTERLACEMETHOD deintMethod, bool wait)
{
  int index;
  {
    std::unique_lock lock(m_presentlock);
    if (m_free.empty())
      return false;
    index = m_free.front();
    m_free.pop_front();
  }

  bool wantsDoublePass = false;
  {
    std::lock_guard lock2(m_datalock);
    if (!m_pRenderer)
    {
      std::unique_lock lock(m_presentlock);
      m_free.push_front(index);
      return false;
    }

    m_pRenderer->AddVideoPicture(picture, index);
    wantsDoublePass = m_pRenderer->WantsDoublePass();
  }

  // set fieldsync if picture is interlaced
  EFIELDSYNC displayField = FS_NONE;
  if (picture.iFlags & DVP_FLAG_INTERLACED)
  {
    if (deintMethod != EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE)
    {
      if (picture.iFlags & DVP_FLAG_TOP_FIELD_FIRST)
        displayField = FS_TOP;
      else
        displayField = FS_BOT;
    }
  }

  EPRESENTMETHOD presentmethod = PRESENT_METHOD_SINGLE;
  if (deintMethod == VS_INTERLACEMETHOD_NONE)
  {
    presentmethod = PRESENT_METHOD_SINGLE;
    displayField = FS_NONE;
  }
  else
  {
    if (displayField == FS_NONE)
      presentmethod = PRESENT_METHOD_SINGLE;
    else
    {
      if (deintMethod == VS_INTERLACEMETHOD_RENDER_BLEND)
        presentmethod = PRESENT_METHOD_BLEND;
      else if (deintMethod == VS_INTERLACEMETHOD_RENDER_BOB)
        presentmethod = PRESENT_METHOD_BOB;
      else
      {
        if (!wantsDoublePass)
          presentmethod = PRESENT_METHOD_SINGLE;
        else
          presentmethod = PRESENT_METHOD_BOB;
      }
    }
  }

  std::unique_lock lock(m_presentlock);

  SPresent& present = m_Queue[index];
  present.presentfield = displayField;
  present.presentmethod = presentmethod;
  present.pts = picture.pts;

  // Keep the queue sorted by pts (and avoid duplicate indices) so the render tick doesn't
  // have to scan/sort.
  if (std::find(m_queued.begin(), m_queued.end(), index) == m_queued.end())
  {
    const double pts = present.pts;
    if (m_queued.empty() || m_Queue[m_queued.back()].pts <= pts)
    {
      m_queued.push_back(index);
    }
    else
    {
      auto insertPos = std::upper_bound(
          m_queued.begin(), m_queued.end(), pts,
          [this](double ptsValue, int queuedIndex) { return ptsValue < m_Queue[queuedIndex].pts; });
      m_queued.insert(insertPos, index);
    }
  }

  m_playerPort->UpdateRenderBuffers(m_queued.size(), m_discard.size(), m_free.size());

  // signal to any waiters to check state
  if (m_presentstep == PRESENT_IDLE)
  {
    m_presentstep = PRESENT_READY;
    NotifyPresentWaiters();
  }

  if (wait)
  {
    m_forceNext = true;
    XbmcThreads::EndTime<> endtime(200ms);
    while (m_presentstep == PRESENT_READY)
    {
      WaitPresent(lock, 20ms);
      if (endtime.IsTimePast() || bStop)
      {
        if (!bStop)
        {
          CLog::Log(LOGWARNING, "CRenderManager::AddVideoPicture - timeout waiting for render");
        }
        break;
      }
    }
    m_forceNext = false;
  }

  return true;
}

void CRenderManager::AddOverlay(std::shared_ptr<CDVDOverlay> o, double pts)
{
  int idx;
  {
    std::unique_lock<CCriticalSection> lock(m_presentlock);
    if (m_free.empty())
      return;
    idx = m_free.front();
  }
  std::unique_lock<CCriticalSection> lock(m_datalock);
  m_overlays.AddOverlay(std::move(o), pts, idx);
}

bool CRenderManager::Supports(ERENDERFEATURE feature) const
{
  std::unique_lock<CCriticalSection> lock(m_statelock);
  if (m_pRenderer)
    return m_pRenderer->Supports(feature);
  else
    return false;
}

bool CRenderManager::Supports(ESCALINGMETHOD method) const
{
  std::unique_lock<CCriticalSection> lock(m_statelock);
  if (m_pRenderer)
    return m_pRenderer->Supports(method);
  else
    return false;
}

int CRenderManager::WaitForBuffer(volatile std::atomic_bool& bStop,
                                  std::chrono::milliseconds timeout)
{
  std::unique_lock<CCriticalSection> lock(m_presentlock);

  // check if gui is active and discard buffer if not
  // this keeps videoplayer going
  if (!m_bRenderGUI || !g_application.GetRenderGUI())
  {
    m_bRenderGUI = false;
    double presenttime = 0;
    double clock = m_dvdClock.GetClock();
    if (!m_queued.empty())
    {
      int idx = m_queued.front();
      presenttime = m_Queue[idx].pts;
    }
    else
      presenttime = clock + 0.02;

    auto sleeptime = std::chrono::milliseconds(static_cast<int>((presenttime - clock) * 1000));
    if (sleeptime < 0ms)
      sleeptime = 0ms;
    sleeptime = std::min(sleeptime, 20ms);
    WaitPresent(lock, sleeptime);
    DiscardBufferLocked();
    return 0;
  }

  XbmcThreads::EndTime<> endtime{timeout};
  while(m_free.empty())
  {
    WaitPresent(lock, std::min(50ms, timeout));
    if (endtime.IsTimePast() || bStop)
    {
      return -1;
    }
  }

  // make sure overlay buffer is released, this won't happen on AddOverlay
  m_overlays.Release(m_free.front());

  // return buffer level
  return m_queued.size() + m_discard.size();
}

void CRenderManager::PrepareNextRender()
{
  if (m_queued.empty())
  {
    CLog::Log(LOGERROR, "CRenderManager::PrepareNextRender - asked to prepare with nothing available");
    m_presentstep = PRESENT_IDLE;
    m_presentevent.notifyAll();
    return;
  }

  if (!m_showVideo && !m_forceNext)
    return;

  double frametime = 1.0 /
                     static_cast<double>(CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS()) *
                     DVD_TIME_BASE;


  int nextFrameIndex = m_queued.front();
  double nextFramePts = m_Queue[m_queued.front()].pts;

  double renderPts = m_dvdClock.GetClock();
  if (m_dvdClock.GetClockSpeed() < 0)
    nextFramePts = renderPts;

  if (m_clockSync.m_enabled)
  {
    double err = fmod(renderPts - nextFramePts, frametime);
    m_clockSync.m_error += err;
    m_clockSync.m_errCount++;
    if (m_clockSync.m_errCount > 30)
    {
      double average = m_clockSync.m_error / m_clockSync.m_errCount;
      m_clockSync.m_syncOffset = average;
      m_clockSync.m_error = 0;
      m_clockSync.m_errCount = 0;

      m_dvdClock.SetVsyncAdjust(-average);
    }
    renderPts += frametime / 2 - m_clockSync.m_syncOffset;
  }
  else
  {
    m_dvdClock.SetVsyncAdjust(0);
  }

  CLog::LogFC(LOGDEBUG, LOGAVTIMING,
              "renderPts: {:.3f} renderPts: {:.3f} nextFramePts: {:.3f} -> diff: {:.3f}  render: {:d} "
              "forceNext: {:d}",
              renderPts / DVD_TIME_BASE, renderPts / DVD_TIME_BASE, nextFramePts / DVD_TIME_BASE,
              (renderPts - nextFramePts) / DVD_TIME_BASE, renderPts >= nextFramePts, m_forceNext);

  bool combined = false;
  if (m_presentsourcePast >= 0)
  {
    m_discard.push_back(m_presentsourcePast);
    m_presentsourcePast = -1;
    combined = true;
  }

  if ((renderPts >= nextFramePts) || m_forceNext)
  {
    // push back present source index before other lates to keep order
    if (m_presentstarted) m_discard.push_back(m_presentsource);
    
    double diff = (renderPts - nextFramePts);
    bool skip = ((m_dataCacheCore.GetSpeed() == 1.0f) && (diff > 79000.0));

    // skip late frames
    while ((diff > 62000.0) && (m_queued.size() > 2))
    {
      int late = m_queued.front();
      m_queued.pop_front();
      m_discard.push_back(late);
      if (skip) m_QueueSkip++;
      diff = (renderPts - m_Queue[m_queued.front()].pts);
    }

    if (m_displayReset)
    {
      m_QueueSkip = 0;
      m_lateframes = 0;
      m_displayReset = false;
    }

    int idx = m_queued.front();
    m_lateframes = static_cast<int>(std::max(0.0, diff / frametime));

    m_presentstep = PRESENT_FLIP;
    m_presentsource = idx;
    m_presentstarted = true;
    m_queued.pop_front();
    m_presentpts = m_Queue[m_presentsource].pts;
    NotifyPresentWaiters();

    m_playerPort->UpdateRenderBuffers(m_queued.size(), m_discard.size(), m_free.size());
  }
  else if (!combined && renderPts > (nextFramePts - frametime))
  {
    m_lateframes = 0;
    m_presentstep = PRESENT_FLIP;
    m_presentsourcePast = m_presentsource;
    m_presentsource = m_queued.front();
    m_presentstarted = true;
    m_queued.pop_front();
    m_presentpts = m_Queue[m_presentsource].pts - frametime / 2;
    m_presentevent.notifyAll();
  }
}

void CRenderManager::DiscardBuffer()
{
  std::unique_lock<CCriticalSection> lock2(m_presentlock);

  DiscardBufferLocked();
}

void CRenderManager::DiscardBufferLocked()
{
  while (!m_queued.empty())
  {
    m_discard.push_back(m_queued.front());
    m_queued.pop_front();
  }

  if (m_presentstep == PRESENT_READY)
    m_presentstep = PRESENT_IDLE;
  NotifyPresentWaiters();
}

bool CRenderManager::GetStats(int &lateframes, double &pts, int &queued, int &discard)
{
  std::unique_lock<CCriticalSection> lock(m_presentlock);
  lateframes = m_lateframes / 10;
  pts = m_presentpts;
  queued = m_queued.size();
  discard  = m_discard.size();
  return true;
}

void CRenderManager::CheckEnableClockSync()
{
  // refresh rate can be a multiple of video fps
  double diff = 1.0;

  if (m_fps != 0)
  {
    double fps = static_cast<double>(m_fps);
    double refreshrate, clockspeed;
    int missedvblanks;
    if (m_dvdClock.GetClockInfo(missedvblanks, clockspeed, refreshrate))
    {
      fps *= clockspeed;
    }

    diff = static_cast<double>(CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS()) / fps;
    if (diff < 1.0)
      diff = 1.0 / diff;

    // Calculate distance from nearest integer proportion
    diff = std::abs(std::round(diff) - diff);
  }

  if (diff && (diff > 0.0005))
  {
    m_clockSync.m_enabled = true;
  }
  else
  {
    m_clockSync.m_enabled = false;
    m_dvdClock.SetVsyncAdjust(0);
  }

  m_playerPort->UpdateClockSync(m_clockSync.m_enabled);
}
