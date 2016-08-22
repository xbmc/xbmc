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

#include "system.h"
#include "RenderManager.h"
#include "RenderFlags.h"
#include "guilib/GraphicContext.h"
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "windowing/WindowingFactory.h"

#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"

#if defined(HAS_GL)
#include "LinuxRendererGL.h"
#include "HwDecRender/RendererVAAPI.h"
#include "HwDecRender/RendererVDPAU.h"
#if defined(TARGET_DARWIN_OSX)
#include "HwDecRender/RendererVTBGL.h"
#endif
#elif HAS_GLES == 2
  #include "LinuxRendererGLES.h"
#if defined(HAS_MMAL)
#include "HwDecRender/MMALRenderer.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#include "HwDecRender/RendererVTBGLES.h"
#endif
#if defined(HAS_IMXVPU)
#include "HwDecRender/RendererIMX.h"
#endif
#if defined(HAS_LIBAMCODEC)
#include "HwDecRender/RendererAML.h"
#endif
#if defined(HAVE_LIBOPENMAX)
#include "HwDecRender/RendererOpenMax.h"
#endif
#elif defined(HAS_DX)
  #include "WinRenderer.h"
#elif defined(HAS_SDL)
  #include "LinuxRenderer.h"
#endif

#if defined(TARGET_ANDROID)
#include "HwDecRender/RendererMediaCodec.h"
#include "HwDecRender/RendererMediaCodecSurface.h"
#endif

#if defined(TARGET_POSIX)
#include "linux/XTimeUtils.h"
#endif

#include "RenderCapture.h"

/* to use the same as player */
#include "../VideoPlayer/DVDClock.h"
#include "../VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "../VideoPlayer/DVDCodecs/DVDCodecUtils.h"

using namespace KODI::MESSAGING;

static void requeue(std::deque<int> &trg, std::deque<int> &src)
{
  trg.push_back(src.front());
  src.pop_front();
}

static std::string GetRenderFormatName(ERenderFormat format)
{
  switch(format)
  {
    case RENDER_FMT_YUV420P:   return "YV12";
    case RENDER_FMT_YUV420P16: return "YV12P16";
    case RENDER_FMT_YUV420P10: return "YV12P10";
    case RENDER_FMT_NV12:      return "NV12";
    case RENDER_FMT_UYVY422:   return "UYVY";
    case RENDER_FMT_YUYV422:   return "YUY2";
    case RENDER_FMT_VDPAU:     return "VDPAU";
    case RENDER_FMT_VDPAU_420: return "VDPAU_420";
    case RENDER_FMT_DXVA:      return "DXVA";
    case RENDER_FMT_VAAPI:     return "VAAPI";
    case RENDER_FMT_VAAPINV12: return "VAAPI_NV12";
    case RENDER_FMT_OMXEGL:    return "OMXEGL";
    case RENDER_FMT_CVBREF:    return "BGRA";
    case RENDER_FMT_BYPASS:    return "BYPASS";
    case RENDER_FMT_MEDIACODEC:return "MEDIACODEC";
    case RENDER_FMT_MEDIACODECSURFACE:return "MEDIACODECSURFACE";
    case RENDER_FMT_IMXMAP:    return "IMXMAP";
    case RENDER_FMT_MMAL:      return "MMAL";
    case RENDER_FMT_AML:       return "AMLCODEC";
    case RENDER_FMT_NONE:      return "NONE";
  }
  return "UNKNOWN";
}

void CRenderManager::CClockSync::Reset()
{
  m_error = 0;
  m_errCount = 0;
  m_syncOffset = 0;
  m_enabled = false;
}

unsigned int CRenderManager::m_nextCaptureId = 0;

CRenderManager::CRenderManager(CDVDClock &clock, IRenderMsg *player) :
  m_pRenderer(nullptr),
  m_bTriggerUpdateResolution(false),
  m_bRenderGUI(true),
  m_waitForBufferCount(0),
  m_rendermethod(0),
  m_renderedOverlay(false),
  m_renderDebug(false),
  m_renderState(STATE_UNCONFIGURED),
  m_displayLatency(0.0),
  m_videoDelay(0),
  m_QueueSize(2),
  m_QueueSkip(0),
  m_format(RENDER_FMT_NONE),
  m_width(0),
  m_height(0),
  m_dwidth(0),
  m_dheight(0),
  m_fps(0.0f),
  m_extended_format(0),
  m_orientation(0),
  m_NumberBuffers(0),
  m_lateframes(-1),
  m_presentpts(0.0),
  m_presentstep(PRESENT_IDLE),
  m_presentsource(0),
  m_dvdClock(clock),
  m_playerPort(player),
  m_captureWaitCounter(0),
  m_hasCaptures(false)
{
}

CRenderManager::~CRenderManager()
{
  delete m_pRenderer;
}

void CRenderManager::GetVideoRect(CRect &source, CRect &dest, CRect &view)
{
  CSingleLock lock(m_statelock);
  if (m_pRenderer)
    m_pRenderer->GetVideoRect(source, dest, view);
}

float CRenderManager::GetAspectRatio()
{
  CSingleLock lock(m_statelock);
  if (m_pRenderer)
    return m_pRenderer->GetAspectRatio();
  else
    return 1.0f;
}

bool CRenderManager::Configure(DVDVideoPicture& picture, float fps, unsigned flags, unsigned int orientation, int buffers)
{

  // check if something has changed
  {
    CSingleLock lock(m_statelock);

    if (m_width == picture.iWidth &&
        m_height == picture.iHeight &&
        m_dwidth == picture.iDisplayWidth &&
        m_dheight == picture.iDisplayHeight &&
        m_fps == fps &&
        (m_flags & ~CONF_FLAGS_FULLSCREEN) == (flags & ~CONF_FLAGS_FULLSCREEN) &&
        m_format == picture.format &&
        m_extended_format == picture.extended_format &&
        m_orientation == orientation &&
        m_NumberBuffers == buffers &&
        m_pRenderer != NULL)
    {
      return true;
    }
  }

  std::string formatstr = GetRenderFormatName(picture.format);
  CLog::Log(LOGDEBUG, "CRenderManager::Configure - change configuration. %dx%d. display: %dx%d. framerate: %4.2f. format: %s", picture.iWidth, picture.iHeight, picture.iDisplayWidth, picture.iDisplayHeight, fps, formatstr.c_str());

  // make sure any queued frame was fully presented
  {
    CSingleLock lock(m_presentlock);
    XbmcThreads::EndTime endtime(5000);
    while (m_presentstep != PRESENT_IDLE)
    {
      if(endtime.IsTimePast())
      {
        CLog::Log(LOGWARNING, "CRenderManager::Configure - timeout waiting for state");
        return false;
      }
      m_presentevent.wait(lock, endtime.MillisLeft());
    }
  }

  {
    CSingleLock lock(m_statelock);
    m_width = picture.iWidth;
    m_height = picture.iHeight,
    m_dwidth = picture.iDisplayWidth;
    m_dheight = picture.iDisplayHeight;
    m_fps = fps;
    m_flags = flags;
    m_format = picture.format;
    m_extended_format = picture.extended_format;
    m_orientation = orientation;
    m_NumberBuffers  = buffers;
    m_renderState = STATE_CONFIGURING;
    m_stateEvent.Reset();

    CheckEnableClockSync();

    CSingleLock lock2(m_presentlock);
    m_presentstep = PRESENT_READY;
    m_presentevent.notifyAll();
  }

  if (!m_stateEvent.WaitMSec(1000))
  {
    CLog::Log(LOGWARNING, "CRenderManager::Configure - timeout waiting for configure");
    return false;
  }

  CSingleLock lock(m_statelock);
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
  CSingleLock lock(m_statelock);
  CSingleLock lock2(m_presentlock);
  CSingleLock lock3(m_datalock);

  if (m_pRenderer && !m_pRenderer->HandlesRenderFormat(m_format))
  {
    DeleteRenderer();
  }

  if(!m_pRenderer)
  {
    CreateRenderer();
    if (!m_pRenderer)
      return false;
  }

  bool result = m_pRenderer->Configure(m_width, m_height, m_dwidth, m_dheight, m_fps, m_flags, m_format, m_extended_format, m_orientation);
  if (result)
  {
    CRenderInfo info = m_pRenderer->GetRenderInfo();
    int renderbuffers = info.optimal_buffer_size;
    m_QueueSize = renderbuffers;
    if (m_NumberBuffers > 0)
      m_QueueSize = std::min(m_NumberBuffers, renderbuffers);

    m_QueueSize = std::min(m_QueueSize, (int)info.max_buffer_size);
    m_QueueSize = std::min(m_QueueSize, NUM_BUFFERS);
    if(m_QueueSize < 2)
    {
      m_QueueSize = 2;
      CLog::Log(LOGWARNING, "CRenderManager::Configure - queue size too small (%d, %d, %d)", m_QueueSize, renderbuffers, m_NumberBuffers);
    }

    m_pRenderer->SetBufferSize(m_QueueSize);
    m_pRenderer->Update();

    m_queued.clear();
    m_discard.clear();
    m_free.clear();
    m_presentsource = 0;
    for (int i=1; i < m_QueueSize; i++)
      m_free.push_back(i);

    m_bRenderGUI = true;
    m_waitForBufferCount = 0;
    m_bTriggerUpdateResolution = true;
    m_presentstep = PRESENT_IDLE;
    m_presentpts = DVD_NOPTS_VALUE;
    m_lateframes = -1.0;
    m_presentevent.notifyAll();
    m_renderedOverlay = false;
    m_renderDebug = false;
    m_clockSync.Reset();

    m_renderState = STATE_CONFIGURED;

    CLog::Log(LOGDEBUG, "CRenderManager::Configure - %d", m_QueueSize);

    std::list<EINTERLACEMETHOD> deintmethods;
    for (int deint = EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE; deint != EINTERLACEMETHOD::VS_INTERLACEMETHOD_MAX; deint++)
    {
      if (m_pRenderer->Supports((EINTERLACEMETHOD)deint))
        deintmethods.push_back((EINTERLACEMETHOD)deint);
    }
    m_playerPort->UpdateDeinterlacingMethods(deintmethods);
  }
  else
    m_renderState = STATE_UNCONFIGURED;

  m_stateEvent.Set();
  m_playerPort->VideoParamsChange();
  return result;
}

bool CRenderManager::IsConfigured() const
{
  CSingleLock lock(m_statelock);
  if (m_renderState == STATE_CONFIGURED)
    return true;
  else
    return false;
}

void CRenderManager::FrameWait(int ms)
{
  XbmcThreads::EndTime timeout(ms);
  CSingleLock lock(m_presentlock);
  while(m_presentstep == PRESENT_IDLE && !timeout.IsTimePast())
    m_presentevent.wait(lock, timeout.MillisLeft());
}

bool CRenderManager::HasFrame()
{
  CSingleLock lock(m_presentlock);
  if (m_presentstep == PRESENT_READY ||
      m_presentstep == PRESENT_FRAME || m_presentstep == PRESENT_FRAME2)
    return true;
  else
    return false;
}

void CRenderManager::FrameMove()
{
  {
    CSingleLock lock(m_statelock);

    if (m_renderState == STATE_UNCONFIGURED)
      return;
    else if (m_renderState == STATE_CONFIGURING)
    {
      lock.Leave();
      if (!Configure())
        return;

      FrameWait(50);

      if (m_flags & CONF_FLAGS_FULLSCREEN)
      {
        CApplicationMessenger::GetInstance().PostMsg(TMSG_SWITCHTOFULLSCREEN);
      }
    }
  }
  {
    CSingleLock lock2(m_presentlock);

    if (m_presentstep == PRESENT_FRAME2)
    {
      if (!m_queued.empty())
      {
        m_presentstep = PRESENT_READY;
        m_presentevent.notifyAll();
      }
    }

    if (m_presentstep == PRESENT_READY)
      PrepareNextRender();

    if(m_presentstep == PRESENT_FLIP)
    {
      m_pRenderer->FlipPage(m_presentsource);
      m_presentstep = PRESENT_FRAME;
      m_presentevent.notifyAll();
    }

    /* release all previous */
    for (std::deque<int>::iterator it = m_discard.begin(); it != m_discard.end(); )
    {
      // renderer may want to keep the frame for postprocessing
      if (!m_pRenderer->NeedBufferForRef(*it) || !m_bRenderGUI)
      {
        m_pRenderer->ReleaseBuffer(*it);
        m_overlays.Release(*it);
        m_free.push_back(*it);
        it = m_discard.erase(it);
      }
      else
        ++it;
    }
    
    m_bRenderGUI = true;
  }

  UpdateResolution();
  ManageCaptures();
}

void CRenderManager::PreInit()
{
  if (!g_application.IsCurrentThread())
  {
    CLog::Log(LOGERROR, "CRenderManager::UnInit - not called from render thread");
    return;
  }

  CSingleLock lock(m_statelock);

  if (!m_pRenderer)
  {
    m_format = RENDER_FMT_YUV420P;
    CreateRenderer();
  }

  UpdateDisplayLatency();

  m_QueueSize   = 2;
  m_QueueSkip   = 0;
  m_presentstep = PRESENT_IDLE;
  m_format = RENDER_FMT_NONE;
}

void CRenderManager::UnInit()
{
  if (!g_application.IsCurrentThread())
  {
    CLog::Log(LOGERROR, "CRenderManager::UnInit - not called from render thread");
    return;
  }

  CSingleLock lock(m_statelock);

  m_overlays.Flush();
  m_debugRenderer.Flush();

  DeleteRenderer();

  m_renderState = STATE_UNCONFIGURED;
  RemoveCaptures();
}

bool CRenderManager::Flush()
{
  if (!m_pRenderer)
    return true;

  if (g_application.IsCurrentThread())
  {
    CLog::Log(LOGDEBUG, "%s - flushing renderer", __FUNCTION__);

    CSingleExit exitlock(g_graphicsContext);

    CSingleLock lock(m_statelock);
    CSingleLock lock2(m_presentlock);
    CSingleLock lock3(m_datalock);

    if (m_pRenderer)
    {
      m_pRenderer->Flush();
      m_overlays.Flush();
      m_debugRenderer.Flush();

      m_queued.clear();
      m_discard.clear();
      m_free.clear();
      m_presentsource = 0;
      m_presentstep = PRESENT_IDLE;
      for (int i = 1; i < m_QueueSize; i++)
        m_free.push_back(i);

      m_flushEvent.Set();
    }
  }
  else
  {
    m_flushEvent.Reset();
    CApplicationMessenger::GetInstance().PostMsg(TMSG_RENDERER_FLUSH);
    if (!m_flushEvent.WaitMSec(1000))
    {
      CLog::Log(LOGERROR, "%s - timed out waiting for renderer to flush", __FUNCTION__);
      return false;
    }
    else
      return true;
  }
  return true;
}

void CRenderManager::CreateRenderer()
{
  if (!m_pRenderer)
  {
    if (m_format == RENDER_FMT_VAAPI || m_format == RENDER_FMT_VAAPINV12)
    {
#if defined(HAVE_LIBVA)
      m_pRenderer = new CRendererVAAPI;
#endif
    }
    else if (m_format == RENDER_FMT_VDPAU || m_format == RENDER_FMT_VDPAU_420)
    {
#if defined(HAVE_LIBVDPAU)
      m_pRenderer = new CRendererVDPAU;
#endif
    }
    else if (m_format == RENDER_FMT_CVBREF)
    {
#if defined(TARGET_DARWIN)
      m_pRenderer = new CRendererVTB;
#endif
    }
    else if (m_format == RENDER_FMT_MEDIACODEC)
    {
#if defined(TARGET_ANDROID)
      m_pRenderer = new CRendererMediaCodec;
#endif
    }
    else if (m_format == RENDER_FMT_MEDIACODECSURFACE)
    {
#if defined(TARGET_ANDROID)
      m_pRenderer = new CRendererMediaCodecSurface;
#endif
    }
    else if (m_format == RENDER_FMT_MMAL)
    {
#if defined(HAS_MMAL)
      m_pRenderer = new CMMALRenderer;
#endif
    }
    else if (m_format == RENDER_FMT_IMXMAP)
    {
#if defined(HAS_IMXVPU)
      m_pRenderer = new CRendererIMX;
#endif
    }
    else if (m_format == RENDER_FMT_OMXEGL)
    {
#if defined(HAVE_LIBOPENMAX)
      m_pRenderer = new CRendererOMX;
#endif
    }
    else if (m_format == RENDER_FMT_DXVA)
    {
#if defined(HAS_DX)
      m_pRenderer = new CWinRenderer();
#endif
    }
    else if (m_format == RENDER_FMT_AML)
    {
#if defined(HAS_LIBAMCODEC)
      m_pRenderer = new CRendererAML;
#endif
    }
    else if (m_format != RENDER_FMT_NONE)
    {
#if defined(HAS_MMAL)
      m_pRenderer = new CMMALRenderer;
#elif defined(HAS_GL)
      m_pRenderer = new CLinuxRendererGL;
#elif HAS_GLES == 2
      m_pRenderer = new CLinuxRendererGLES;
#elif defined(HAS_DX)
      m_pRenderer = new CWinRenderer();
#endif
    }
#if defined(HAS_MMAL)
    if (!m_pRenderer)
      m_pRenderer = new CMMALRenderer;
#endif
    if (m_pRenderer)
      m_pRenderer->PreInit();
    else
      CLog::Log(LOGERROR, "RenderManager::CreateRenderer: failed to create renderer");
  }
}

void CRenderManager::DeleteRenderer()
{
  if (m_pRenderer)
  {
    CLog::Log(LOGDEBUG, "%s - deleting renderer", __FUNCTION__);

    delete m_pRenderer;
    m_pRenderer = NULL;
  }
}

unsigned int CRenderManager::AllocRenderCapture()
{
  CRenderCapture *capture = new CRenderCapture;
  m_captures[m_nextCaptureId] = capture;
  return m_nextCaptureId++;
}

void CRenderManager::ReleaseRenderCapture(unsigned int captureId)
{
  CSingleLock lock(m_captCritSect);

  std::map<unsigned int, CRenderCapture*>::iterator it;
  it = m_captures.find(captureId);

  if (it != m_captures.end())
    it->second->SetState(CAPTURESTATE_NEEDSDELETE);
}

void CRenderManager::StartRenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags)
{
  CSingleLock lock(m_captCritSect);

  std::map<unsigned int, CRenderCapture*>::iterator it;
  it = m_captures.find(captureId);
  if (it == m_captures.end())
  {
    CLog::Log(LOGERROR, "CRenderManager::Capture - unknown capture id: %d", captureId);
    return;
  }

  CRenderCapture *capture = it->second;

  capture->SetState(CAPTURESTATE_NEEDSRENDER);
  capture->SetUserState(CAPTURESTATE_WORKING);
  capture->SetWidth(width);
  capture->SetHeight(height);
  capture->SetFlags(flags);
  capture->GetEvent().Reset();

  if (g_application.IsCurrentThread())
  {
    if (flags & CAPTUREFLAG_IMMEDIATELY)
    {
      //render capture and read out immediately
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
  CSingleLock lock(m_captCritSect);

  std::map<unsigned int, CRenderCapture*>::iterator it;
  it = m_captures.find(captureId);
  if (it == m_captures.end())
    return false;

  m_captureWaitCounter++;

  {
    if (!millis)
      millis = 1000;
    
    CSingleExit exitlock(m_captCritSect);
    if (!it->second->GetEvent().WaitMSec(millis))
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
  //no captures, return here so we don't do an unnecessary lock
  if (!m_hasCaptures)
    return;

  CSingleLock lock(m_captCritSect);

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
      //tell the thread that the capture is done or has failed
      capture->SetUserState(capture->GetState());
      capture->GetEvent().Set();

      if (capture->GetFlags() & CAPTUREFLAG_CONTINUOUS)
      {
        capture->SetState(CAPTURESTATE_NEEDSRENDER);

        //if rendering this capture continuously, and readout is async, render a new capture immediately
        if (capture->IsAsync() && !(capture->GetFlags() & CAPTUREFLAG_IMMEDIATELY))
          RenderCapture(capture);

        ++it;
      }
    }
    else
    {
      ++it;
    }
  }

  if (m_captures.empty())
    m_hasCaptures = false;
}

void CRenderManager::RenderCapture(CRenderCapture* capture)
{
  if (!m_pRenderer || !m_pRenderer->RenderCapture(capture))
    capture->SetState(CAPTURESTATE_FAILED);
}

void CRenderManager::RemoveCaptures()
{
  CSingleLock lock(m_captCritSect);

  while (m_captureWaitCounter > 0)
  {
    for (auto entry : m_captures)
    {
      entry.second->GetEvent().Set();
    }
    CSingleExit lockexit(m_captCritSect);
    Sleep(10);
  }

  for (auto entry : m_captures)
  {
    delete entry.second;
  }
  m_captures.clear();
}

void CRenderManager::SetViewMode(int iViewMode)
{
  CSingleLock lock(m_statelock);
  if (m_pRenderer)
    m_pRenderer->SetViewMode(iViewMode);
  m_playerPort->VideoParamsChange();
}

void CRenderManager::FlipPage(volatile std::atomic_bool& bStop, double pts /* = 0 */, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  { CSingleLock lock(m_statelock);

    if (bStop)
      return;

    if(!m_pRenderer)
      return;
  }

  EPRESENTMETHOD presentmethod;

  EINTERLACEMETHOD interlacemethod = AutoInterlaceMethodInternal(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod);

  if (interlacemethod == VS_INTERLACEMETHOD_NONE)
  {
    presentmethod = PRESENT_METHOD_SINGLE;
    sync = FS_NONE;
  }
  else
  {
    if (sync == FS_NONE)
      presentmethod = PRESENT_METHOD_SINGLE;
    else
    {
      bool invert = false;
      if (interlacemethod == VS_INTERLACEMETHOD_RENDER_BLEND)
        presentmethod = PRESENT_METHOD_BLEND;
      else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_WEAVE)
        presentmethod = PRESENT_METHOD_WEAVE;
      else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
      {
        presentmethod = PRESENT_METHOD_WEAVE;
        invert = true;
      }
      else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_BOB)
        presentmethod = PRESENT_METHOD_BOB;
      else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
      {
        presentmethod = PRESENT_METHOD_BOB;
        invert = true;
      }
      else
      {
        if (!m_pRenderer->WantsDoublePass())
          presentmethod = PRESENT_METHOD_SINGLE;
        else
          presentmethod = PRESENT_METHOD_BOB;
      }

      if (presentmethod != PRESENT_METHOD_SINGLE)
      {
        /* invert present field */
        if (invert)
        {
          if (sync == FS_BOT)
            sync = FS_TOP;
          else
            sync = FS_BOT;
        }
      }
    }
  }

  CSingleLock lock(m_presentlock);

  if(m_free.empty())
    return;

  if(source < 0)
    source = m_free.front();

  SPresent& m = m_Queue[source];
  m.presentfield  = sync;
  m.presentmethod = presentmethod;
  m.pts           = pts;
  requeue(m_queued, m_free);

  /* signal to any waiters to check state */
  if(m_presentstep == PRESENT_IDLE)
  {
    m_presentstep = PRESENT_READY;
    m_presentevent.notifyAll();
  }
}

RESOLUTION CRenderManager::GetResolution()
{
  RESOLUTION res = g_graphicsContext.GetVideoResolution();

  CSingleLock lock(m_statelock);
  if (m_renderState == STATE_UNCONFIGURED)
    return res;

  if (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
    res = CResolutionUtils::ChooseBestResolution(m_fps, m_width, CONF_FLAGS_STEREO_MODE_MASK(m_flags));

  return res;
}

void CRenderManager::Render(bool clear, DWORD flags, DWORD alpha, bool gui)
{
  CSingleExit exitLock(g_graphicsContext);

  {
    CSingleLock lock(m_statelock);
    if (m_renderState != STATE_CONFIGURED)
      return;
  }

  if (!gui && m_pRenderer->IsGuiLayer())
    return;

  if (!gui || m_pRenderer->IsGuiLayer())
  {
    SPresent& m = m_Queue[m_presentsource];

    if( m.presentmethod == PRESENT_METHOD_BOB )
      PresentFields(clear, flags, alpha);
    else if( m.presentmethod == PRESENT_METHOD_WEAVE )
      PresentFields(clear, flags | RENDER_FLAG_WEAVE, alpha);
    else if( m.presentmethod == PRESENT_METHOD_BLEND )
      PresentBlend(clear, flags, alpha);
    else
      PresentSingle(clear, flags, alpha);
  }

  if (gui)
  {
    if (!m_pRenderer->IsGuiLayer())
      m_pRenderer->Update();

    m_renderedOverlay = m_overlays.HasOverlay(m_presentsource);
    CRect src, dst, view;
    m_pRenderer->GetVideoRect(src, dst, view);
    m_overlays.SetVideoRect(src, dst, view);
    m_overlays.Render(m_presentsource);

    if (m_renderDebug)
    {
      std::string audio, video, player, vsync;

      m_playerPort->GetDebugInfo(audio, video, player);

      double refreshrate, clockspeed;
      int missedvblanks;
      vsync = StringUtils::Format("VSyncOff: %.1f  ", m_clockSync.m_syncOffset / 1000);
      if (m_dvdClock.GetClockInfo(missedvblanks, clockspeed, refreshrate))
      {
        vsync += StringUtils::Format("VSync: refresh:%.3f missed:%i speed:%.3f%%",
                                     refreshrate,
                                     missedvblanks,
                                     clockspeed * 100);
      }

      m_debugRenderer.SetInfo(audio, video, player, vsync);
      m_debugRenderer.Render(src, dst, view);

      m_debugTimer.Set(1000);
      m_renderedOverlay = true;
    }
  }


  SPresent& m = m_Queue[m_presentsource];

  { CSingleLock lock(m_presentlock);

    if(m_presentstep == PRESENT_FRAME)
    {
      if( m.presentmethod == PRESENT_METHOD_BOB
         ||  m.presentmethod == PRESENT_METHOD_WEAVE)
        m_presentstep = PRESENT_FRAME2;
      else
        m_presentstep = PRESENT_IDLE;
    }
    else if(m_presentstep == PRESENT_FRAME2)
      m_presentstep = PRESENT_IDLE;

    if(m_presentstep == PRESENT_IDLE)
    {
      if(!m_queued.empty())
        m_presentstep = PRESENT_READY;
    }

    m_presentevent.notifyAll();
  }
}

bool CRenderManager::IsGuiLayer()
{
  { CSingleLock lock(m_statelock);

    if (!m_pRenderer)
      return false;

    if (m_pRenderer->IsGuiLayer() || m_renderedOverlay || m_overlays.HasOverlay(m_presentsource))
      return true;

    if (m_renderDebug && m_debugTimer.IsTimePast())
      return true;
  }
  return false;
}

bool CRenderManager::IsVideoLayer()
{
  { CSingleLock lock(m_statelock);

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
  SPresent& m = m_Queue[m_presentsource];

  if (m.presentfield == FS_BOT)
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOT, alpha);
  else if (m.presentfield == FS_TOP)
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_TOP, alpha);
  else
    m_pRenderer->RenderUpdate(clear, flags, alpha);
}

/* new simpler method of handling interlaced material, *
 * we just render the two fields right after eachother */
void CRenderManager::PresentFields(bool clear, DWORD flags, DWORD alpha)
{
  SPresent& m = m_Queue[m_presentsource];

  if(m_presentstep == PRESENT_FRAME)
  {
    if( m.presentfield == FS_BOT)
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_FIELD0, alpha);
    else
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_FIELD0, alpha);
  }
  else
  {
    if( m.presentfield == FS_TOP)
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_FIELD1, alpha);
    else
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_FIELD1, alpha);
  }
}

void CRenderManager::PresentBlend(bool clear, DWORD flags, DWORD alpha)
{
  SPresent& m = m_Queue[m_presentsource];

  if( m.presentfield == FS_BOT )
  {
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_NOOSD, alpha);
    m_pRenderer->RenderUpdate(false, flags | RENDER_FLAG_TOP, alpha / 2);
  }
  else
  {
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_NOOSD, alpha);
    m_pRenderer->RenderUpdate(false, flags | RENDER_FLAG_BOT, alpha / 2);
  }
}

void CRenderManager::UpdateDisplayLatency()
{
  float fps = g_graphicsContext.GetFPS();
  float refresh = fps;
  if (g_graphicsContext.GetVideoResolution() == RES_WINDOW)
    refresh = 0; // No idea about refresh rate when windowed, just get the default latency
  m_displayLatency = (double) g_advancedSettings.GetDisplayLatency(refresh);

  int buffers = g_Windowing.NoOfBuffers();
  m_displayLatency += (buffers - 1) / fps;

}

void CRenderManager::UpdateResolution()
{
  if (m_bTriggerUpdateResolution)
  {
    if (g_graphicsContext.IsFullScreenVideo() && g_graphicsContext.IsFullScreenRoot())
    {
      if (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF && m_fps > 0.0f)
      {
        RESOLUTION res = CResolutionUtils::ChooseBestResolution(m_fps, m_width, CONF_FLAGS_STEREO_MODE_MASK(m_flags));
        g_graphicsContext.SetVideoResolution(res);
        UpdateDisplayLatency();

        CheckEnableClockSync();
      }
      m_bTriggerUpdateResolution = false;
      m_playerPort->VideoParamsChange();
    }
  }
}

void CRenderManager::TriggerUpdateResolution(float fps, int width, int flags)
{
  if (width)
  {
    m_fps = fps;
    m_width = width;
    m_flags = flags;
  }
  m_bTriggerUpdateResolution = true;
}

void CRenderManager::ToggleDebug()
{
  m_renderDebug = !m_renderDebug;
  m_debugTimer.SetExpired();
}

// Get renderer info, can be called before configure
CRenderInfo CRenderManager::GetRenderInfo()
{
  CSingleLock lock(m_statelock);
  CRenderInfo info;
  if (!m_pRenderer)
  {
    info.max_buffer_size = NUM_BUFFERS;
    return info;;
  }
  return m_pRenderer->GetRenderInfo();
}

int CRenderManager::AddVideoPicture(DVDVideoPicture& pic)
{
  int index;
  {
    CSingleLock lock(m_presentlock);
    if (m_free.empty())
      return -1;
    index = m_free.front();
  }

  CSingleLock lock(m_datalock);
  if (!m_pRenderer)
    return -1;

  YV12Image image;
  if (m_pRenderer->GetImage(&image, index) < 0)
    return -1;

  if(pic.format == RENDER_FMT_VDPAU
  || pic.format == RENDER_FMT_VDPAU_420
  || pic.format == RENDER_FMT_OMXEGL
  || pic.format == RENDER_FMT_CVBREF
  || pic.format == RENDER_FMT_VAAPI
  || pic.format == RENDER_FMT_VAAPINV12
  || pic.format == RENDER_FMT_MEDIACODEC
  || pic.format == RENDER_FMT_MEDIACODECSURFACE
  || pic.format == RENDER_FMT_AML
  || pic.format == RENDER_FMT_IMXMAP
  || pic.format == RENDER_FMT_MMAL
  || m_pRenderer->IsPictureHW(pic))
  {
    m_pRenderer->AddVideoPictureHW(pic, index);
  }
  else if(pic.format == RENDER_FMT_YUV420P
       || pic.format == RENDER_FMT_YUV420P10
       || pic.format == RENDER_FMT_YUV420P16)
  {
    CDVDCodecUtils::CopyPicture(&image, &pic);
  }
  else if(pic.format == RENDER_FMT_NV12)
  {
    CDVDCodecUtils::CopyNV12Picture(&image, &pic);
  }
  else if(pic.format == RENDER_FMT_YUYV422
       || pic.format == RENDER_FMT_UYVY422)
  {
    CDVDCodecUtils::CopyYUV422PackedPicture(&image, &pic);
  }

  m_pRenderer->ReleaseImage(index, false);

  return index;
}

void CRenderManager::AddOverlay(CDVDOverlay* o, double pts)
{
  int idx;
  { CSingleLock lock(m_presentlock);
    if (m_free.empty())
      return;
    idx = m_free.front();
  }
  CSingleLock lock(m_datalock);
  m_overlays.AddOverlay(o, pts, idx);
}

bool CRenderManager::Supports(ERENDERFEATURE feature)
{
  CSingleLock lock(m_statelock);
  if (m_pRenderer)
    return m_pRenderer->Supports(feature);
  else
    return false;
}

bool CRenderManager::Supports(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_NONE)
    return true;
  
  CSingleLock lock(m_statelock);
  if (m_pRenderer)
    return m_pRenderer->Supports(method);
  else
    return false;
}

bool CRenderManager::Supports(ESCALINGMETHOD method)
{
  CSingleLock lock(m_statelock);
  if (m_pRenderer)
    return m_pRenderer->Supports(method);
  else
    return false;
}

EINTERLACEMETHOD CRenderManager::AutoInterlaceMethod(EINTERLACEMETHOD mInt)
{
  CSingleLock lock(m_statelock);
  return AutoInterlaceMethodInternal(mInt);
}

EINTERLACEMETHOD CRenderManager::AutoInterlaceMethodInternal(EINTERLACEMETHOD mInt)
{
  if (mInt == VS_INTERLACEMETHOD_NONE)
    return VS_INTERLACEMETHOD_NONE;

  if(m_pRenderer && !m_pRenderer->Supports(mInt))
    mInt = VS_INTERLACEMETHOD_AUTO;

  if (m_pRenderer && mInt == VS_INTERLACEMETHOD_AUTO)
    return m_pRenderer->AutoInterlaceMethod();

  return mInt;
}

int CRenderManager::WaitForBuffer(volatile std::atomic_bool&bStop, int timeout)
{
  CSingleLock lock(m_presentlock);

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

    int sleeptime = (presenttime - clock) * 1000;
    if (sleeptime < 0)
      sleeptime = 0;
    sleeptime = std::min(sleeptime, 20);
    m_presentevent.wait(lock, sleeptime);
    DiscardBuffer();
    return 0;
  }

  XbmcThreads::EndTime endtime(timeout);
  while(m_free.empty())
  {
    m_presentevent.wait(lock, std::min(50, timeout));
    if(endtime.IsTimePast() || bStop)
    {
      if (timeout != 0 && !bStop)
      {
        CLog::Log(LOGWARNING, "CRenderManager::WaitForBuffer - timeout waiting for buffer");
        m_waitForBufferCount++;
        if (m_waitForBufferCount > 2)
        {
          m_bRenderGUI = false;
        }
      }
      return -1;
    }
  }

  m_waitForBufferCount = 0;

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

  double frameOnScreen = m_dvdClock.GetClock();
  double frametime = 1.0 / g_graphicsContext.GetFPS() * DVD_TIME_BASE;

  // correct display latency
  // internal buffers of driver, assume that driver lets us go one frame in advance
  double totalLatency = DVD_SEC_TO_TIME(m_displayLatency) - DVD_MSEC_TO_TIME(m_videoDelay) + 2* frametime;

  double renderPts = frameOnScreen + totalLatency;

  double nextFramePts = m_Queue[m_queued.front()].pts;
  if (m_dvdClock.GetClockSpeed() < 0)
    nextFramePts = renderPts;

  if (m_clockSync.m_enabled)
  {
    double err = fmod(renderPts - nextFramePts, frametime);
    m_clockSync.m_error += err;
    m_clockSync.m_errCount ++;
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

  if (renderPts >= nextFramePts)
  {
    // see if any future queued frames are already due
    auto iter = m_queued.begin();
    int idx = *iter;
    ++iter;
    while (iter != m_queued.end())
    {
      // the slot for rendering in time is [pts .. (pts +  x * frametime)]
      // renderer/drivers have internal queues, being slightliy late here does not mean that
      // we are really late. The likelihood that we recover decreases the greater m_lateframes
      // get. Skipping a frame is easier than having decoder dropping one (lateframes > 10)
      double x = (m_lateframes <= 6) ? 0.98 : 0;
      if (renderPts < m_Queue[*iter].pts + x * frametime)
        break;
      idx = *iter;
      ++iter;
    }

    // skip late frames
    while (m_queued.front() != idx)
    {
      requeue(m_discard, m_queued);
      m_QueueSkip++;
    }

    int lateframes = (renderPts - m_Queue[idx].pts) / frametime;
    if (lateframes)
      m_lateframes += lateframes;
    else
      m_lateframes = 0;
    
    m_presentstep = PRESENT_FLIP;
    m_discard.push_back(m_presentsource);
    m_presentsource = idx;
    m_queued.pop_front();
    m_presentpts = m_Queue[idx].pts - totalLatency;
    m_presentevent.notifyAll();
  }
}

void CRenderManager::DiscardBuffer()
{
  CSingleLock lock2(m_presentlock);

  while(!m_queued.empty())
    requeue(m_discard, m_queued);

  if(m_presentstep == PRESENT_READY)
    m_presentstep = PRESENT_IDLE;
  m_presentevent.notifyAll();
}

bool CRenderManager::GetStats(int &lateframes, double &pts, int &queued, int &discard)
{
  CSingleLock lock(m_presentlock);
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
    float fps = m_fps;
    double refreshrate, clockspeed;
    int missedvblanks;
    if (m_dvdClock.GetClockInfo(missedvblanks, clockspeed, refreshrate))
    {
      fps *= clockspeed;
    }

    if (g_graphicsContext.GetFPS() >= fps)
      diff = fmod(g_graphicsContext.GetFPS(), fps);
    else
      diff = fps - g_graphicsContext.GetFPS();
  }

  if (diff < 0.01)
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
