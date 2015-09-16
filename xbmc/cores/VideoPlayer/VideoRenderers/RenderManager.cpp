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
#include "threads/CriticalSection.h"
#include "video/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "guilib/GUIFontManager.h"
#include "cores/DataCacheCore.h"

#if defined(HAS_GL)
#include "LinuxRendererGL.h"
#include "HwDecRender/RendererVAAPI.h"
#include "HwDecRender/RendererVDPAU.h"
#if defined(TARGET_DARWIN_OSX)
#include "HwDecRender/RendererVDA.h"
#endif
#elif defined(HAS_MMAL)
  #include "HwDecRender/MMALRenderer.h"
#elif HAS_GLES == 2
  #include "LinuxRendererGLES.h"
#if defined(HAVE_VIDEOTOOLBOXDECODER)
#include "HWDecRender/RendererVTB.h"
#endif
#if defined(HAS_IMXVPU)
#include "HWDecRender/RendererIMX.h"
#endif
#if defined(HAVE_LIBOPENMAX)
#include "HWDecRender/RendererOMX.h"
#endif
#elif defined(HAS_DX)
  #include "WinRenderer.h"
#elif defined(HAS_SDL)
  #include "LinuxRenderer.h"
#endif

#if defined(TARGET_ANDROID)
#include "HWDecRender/RendererMediaCodec.h"
#endif

#include "RenderCapture.h"

/* to use the same as player */
#include "../VideoPlayer/DVDClock.h"
#include "../VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "../VideoPlayer/DVDCodecs/DVDCodecUtils.h"

using namespace KODI::MESSAGING;

#define MAXPRESENTDELAY 0.500


/* at any point we want an exclusive lock on rendermanager */
/* we must make sure we don't have a graphiccontext lock */
/* these two functions allow us to step out from that lock */
/* and reaquire it after having the exclusive lock */

template<class T>
class CRetakeLock
{
public:
  CRetakeLock(CSharedSection &section, CCriticalSection &owned = g_graphicsContext)
    : m_count(owned.exit())
    , m_lock (section),
      m_owned(owned)
  {
    m_owned.restore(m_count);
  }

  void Leave() { m_lock.Leave(); }
  void Enter()
  {
    m_count = m_owned.exit();
    m_lock.Enter();
    m_owned.restore(m_count);
  }

private:
  DWORD             m_count;
  T                 m_lock;
  CCriticalSection &m_owned;
};

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
    case RENDER_FMT_EGLIMG:    return "EGLIMG";
    case RENDER_FMT_BYPASS:    return "BYPASS";
    case RENDER_FMT_MEDIACODEC:return "MEDIACODEC";
    case RENDER_FMT_IMXMAP:    return "IMXMAP";
    case RENDER_FMT_MMAL:      return "MMAL";
    case RENDER_FMT_NONE:      return "NONE";
  }
  return "UNKNOWN";
}

CRenderManager::CRenderManager() : m_overlays(this)
{
  m_pRenderer = NULL;
  m_renderState = STATE_UNCONFIGURED;

  m_presentstep = PRESENT_IDLE;
  m_rendermethod = 0;
  m_presentsource = 0;
  m_bReconfigured = false;
  m_hasCaptures = false;
  m_displayLatency = 0.0f;
  m_presentcorr = 0.0;
  m_presenterr = 0.0;
  memset(&m_errorbuff, 0, ERRORBUFFSIZE);
  m_errorindex = 0;
  m_QueueSize   = 2;
  m_QueueSkip   = 0;
  m_format      = RENDER_FMT_NONE;
  m_renderedOverlay = false;
}

CRenderManager::~CRenderManager()
{
  delete m_pRenderer;
}

void CRenderManager::GetVideoRect(CRect &source, CRect &dest, CRect &view)
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->GetVideoRect(source, dest, view);
}

float CRenderManager::GetAspectRatio()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->GetAspectRatio();
  else
    return 1.0f;
}

/* These is based on CurrentHostCounter() */
double CRenderManager::GetPresentTime()
{
  return CDVDClock::GetAbsoluteClock(false) / DVD_TIME_BASE;
}

static double wrap(double x, double minimum, double maximum)
{
  if(x >= minimum
  && x <= maximum)
    return x;
  x = fmod(x - minimum, maximum - minimum) + minimum;
  if(x < minimum)
    x += maximum - minimum;
  if(x > maximum)
    x -= maximum - minimum;
  return x;
}

void CRenderManager::WaitPresentTime(double presenttime)
{
  double frametime;
  double fps = g_VideoReferenceClock.GetRefreshRate(&frametime);
  if(fps <= 0)
  {
    /* smooth video not enabled */
    CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE);
    return;
  }

  CDVDClock *dvdclock = CDVDClock::GetMasterClock();
  if(dvdclock != NULL && dvdclock->GetSpeedAdjust() != 0.0)
  {
    CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE);
    m_presenterr = 0;
    m_presentcorr = 0;
    return;
  }

  double clock     = CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE) / DVD_TIME_BASE;
  double target    = 0.5;
  double error     = ( clock - presenttime ) / frametime - target;

  m_presenterr     = error;

  // correct error so it targets the closest vblank
  error = wrap(error, 0.0 - target, 1.0 - target);

  // scale the error used for correction,
  // based on how much buffer we have on
  // that side of the target
  if(error > 0)
    error /= 2.0 * (1.0 - target);
  if(error < 0)
    error /= 2.0 * (0.0 + target);

  //save error in the buffer
  m_errorindex = (m_errorindex + 1) % ERRORBUFFSIZE;
  m_errorbuff[m_errorindex] = error;

  //get the average error from the buffer
  double avgerror = 0.0;
  for (int i = 0; i < ERRORBUFFSIZE; i++)
    avgerror += m_errorbuff[i];

  avgerror /= ERRORBUFFSIZE;

  //we change the clock speed slightly
  //to make every frame's presenttime end up in the middle of two vblanks
  //integral correction, clamp to -0.5:0.5 range
  m_presentcorr = std::max(std::min(m_presentcorr + avgerror * 0.01, 0.1), -0.1);
  g_VideoReferenceClock.SetFineAdjust(1.0 - avgerror * 0.01 - m_presentcorr * 0.01);
}

std::string CRenderManager::GetVSyncState()
{
  double avgerror = 0.0;
  for (int i = 0; i < ERRORBUFFSIZE; i++)
    avgerror += m_errorbuff[i];
  avgerror /= ERRORBUFFSIZE;

  std::string state = StringUtils::Format("sync:%+3d%% avg:%3d%% error:%2d%%"
                                         ,     MathUtils::round_int(m_presentcorr * 100)
                                         ,     MathUtils::round_int(avgerror      * 100)
                                         , abs(MathUtils::round_int(m_presenterr  * 100)));
  return state;
}

bool CRenderManager::Configure(DVDVideoPicture& picture, float fps, unsigned flags, unsigned int orientation, int buffers)
{

  // check if something has changed
  {
    float config_framerate = fps;
    float render_framerate = g_graphicsContext.GetFPS();
    if (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) == ADJUST_REFRESHRATE_OFF)
      render_framerate = config_framerate;
    bool changerefresh = (fps != 0) &&
                         (m_fps == 0.0 || fmod(m_fps, fps) != 0.0) &&
                         (render_framerate != config_framerate);

    CSharedLock lock(m_sharedSection);
    if (m_width == picture.iWidth &&
        m_height == picture.iHeight &&
        m_dwidth == picture.iDisplayWidth &&
        m_dheight == picture.iDisplayHeight &&
        !changerefresh &&
        (m_flags & ~CONF_FLAGS_FULLSCREEN) == (flags & ~CONF_FLAGS_FULLSCREEN) &&
        m_format == picture.format &&
        m_extended_format == picture.extended_format &&
        m_orientation == orientation &&
        m_NumberBuffers == buffers &&
        m_pRenderer != NULL)
      return true;
  }

  std::string formatstr = GetRenderFormatName(picture.format);
  CLog::Log(LOGDEBUG, "CRenderManager::Configure - change configuration. %dx%d. display: %dx%d. framerate: %4.2f. format: %s", picture.iWidth, picture.iHeight, picture.iDisplayWidth, picture.iDisplayHeight, fps, formatstr.c_str());

  // make sure any queued frame was fully presented
  {
    CSingleLock lock(m_presentlock);
    XbmcThreads::EndTime endtime(5000);
    while(m_presentstep != PRESENT_IDLE && m_presentstep != PRESENT_READY)
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
    CExclusiveLock lock(m_sharedSection);
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
  }

  if (!m_stateEvent.WaitMSec(1000))
  {
    CLog::Log(LOGWARNING, "CRenderManager::Configure - timeout waiting for configure");
    return false;
  }

  CSharedLock lock(m_sharedSection);
  if (m_renderState != STATE_CONFIGURED)
  {
    CLog::Log(LOGWARNING, "CRenderManager::Configure - failed to configure");
    return false;
  }

  return true;
}

bool CRenderManager::Configure()
{
  CExclusiveLock lock(m_sharedSection);
  CSingleLock lock2(m_presentlock);

  if (m_pRenderer && m_pRenderer->GetRenderFormat() != m_format)
  {
    DeleteRenderer();
  }

  if(!m_pRenderer)
  {
    CreateRenderer();
    if (!m_pRenderer)
      return false;
    else
      m_pRenderer->PreInit();
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
    m_bReconfigured = true;
    m_presentstep = PRESENT_IDLE;
    m_presentpts = DVD_NOPTS_VALUE;
    m_sleeptime = 1.0;
    m_presentevent.notifyAll();
    m_renderedOverlay = false;

    m_renderState = STATE_CONFIGURED;

    CLog::Log(LOGDEBUG, "CRenderManager::Configure - %d", m_QueueSize);
  }
  else
    m_renderState = STATE_UNCONFIGURED;

  m_stateEvent.Set();
  return result;
}

bool CRenderManager::IsConfigured() const
{
  CSharedLock lock(m_sharedSection);
  if (m_renderState == STATE_CONFIGURED)
    return true;
  else
    return false;
}

void CRenderManager::Update()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  if (m_pRenderer)
    m_pRenderer->Update();
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
  CSharedLock lock(m_sharedSection);

  if (m_renderState == STATE_UNCONFIGURED)
    return;
  else if (m_renderState == STATE_CONFIGURING)
  {
    lock.Leave();
    if (!Configure())
      return;

    if (m_flags & CONF_FLAGS_FULLSCREEN)
    {
      CApplicationMessenger::GetInstance().PostMsg(TMSG_SWITCHTOFULLSCREEN);
    }
    lock.Enter();
  }

  CSingleLock lock2(m_presentlock);

  if (m_presentstep == PRESENT_FRAME2)
  {
    if (!m_queued.empty())
    {
      double timestamp = GetPresentTime();
      SPresent& m = m_Queue[m_presentsource];
      SPresent& q = m_Queue[m_queued.front()];
      if(timestamp > m.timestamp + (q.timestamp - m.timestamp) * 0.5)
      {
        m_presentstep = PRESENT_READY;
        m_presentevent.notifyAll();
      }
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

void CRenderManager::FrameFinish()
{
  /* wait for this present to be valid */
  SPresent& m = m_Queue[m_presentsource];

  if(g_graphicsContext.IsFullScreenVideo())
  {
    CSingleExit lock(g_graphicsContext);
    WaitPresentTime(m.timestamp);
  }

  m_clock_framefinish = GetPresentTime();

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

void CRenderManager::PreInit()
{
  if (!g_application.IsCurrentThread())
  {
    CLog::Log(LOGERROR, "CRenderManager::UnInit - not called from render thread");
    return;
  }

  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_presentcorr = 0.0;
  m_presenterr  = 0.0;
  m_errorindex  = 0;
  memset(m_errorbuff, 0, sizeof(m_errorbuff));

  if (!m_pRenderer)
  {
    m_format = RENDER_FMT_NONE;
    CreateRenderer();
  }

  UpdateDisplayLatency();

  m_QueueSize   = 2;
  m_QueueSkip   = 0;
}

void CRenderManager::UnInit()
{
  if (!g_application.IsCurrentThread())
  {
    CLog::Log(LOGERROR, "CRenderManager::UnInit - not called from render thread");
    return;
  }

  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_overlays.Flush();
  g_fontManager.Unload("__subtitle__");
  g_fontManager.Unload("__subtitleborder__");

  DeleteRenderer();

  m_renderState = STATE_UNCONFIGURED;
}

bool CRenderManager::Flush()
{
  if (!m_pRenderer)
    return true;

  if (g_application.IsCurrentThread())
  {
    CLog::Log(LOGDEBUG, "%s - flushing renderer", __FUNCTION__);

    CRetakeLock<CExclusiveLock> lock(m_sharedSection);

    if (m_pRenderer)
    {
      m_pRenderer->Flush();
      m_overlays.Flush();
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

  CSingleLock lock(m_presentlock);
  m_queued.clear();
  m_discard.clear();
  m_free.clear();
  m_presentsource = 0;
  for (int i = 1; i < m_QueueSize; i++)
    m_free.push_back(i);

  return true;
}
void CRenderManager::CreateRenderer()
{
  if (!m_pRenderer)
  {
    if (m_format == RENDER_FMT_MEDIACODEC)
#if defined (TARGET_ANDROID)
    {
      m_pRenderer = new CRendererMediaCodec;
    }
#elif defined(HAS_MMAL)
    m_pRenderer = new CMMALRenderer
#elif HAS_GLES == 2
    if (m_format == RENDER_FMT_CVBREF)
    {
#if defined(HAVE_VIDEOTOOLBOXDECODER)
      m_pRenderer = new CRendererVTB;
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
    else if (m_format != RENDER_FMT_NONE)
    {
      m_pRenderer = new CLinuxRendererGLES;
    }
#elif defined(HAS_DX)
    m_pRenderer = new CWinRenderer();
#endif
#if defined(HAS_GL)
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
#if defined(TARGET_DARWIN_OSX)
      m_pRenderer = new CRendererVDA;
#endif
    }
    else if (m_format != RENDER_FMT_NONE)
    {
      m_pRenderer = new CLinuxRendererGL;
    }
#endif
    if (m_pRenderer)
      m_pRenderer->PreInit();
  }
}

void CRenderManager::DeleteRenderer()
{
  CLog::Log(LOGDEBUG, "%s - deleting renderer", __FUNCTION__);

  if (m_pRenderer)
  {
    delete m_pRenderer;
    m_pRenderer = NULL;
  }
}

CRenderCapture* CRenderManager::AllocRenderCapture()
{
  return new CRenderCapture;
}

void CRenderManager::ReleaseRenderCapture(CRenderCapture* capture)
{
  CSingleLock lock(m_captCritSect);

  RemoveCapture(capture);

  //because a CRenderCapture might have some gl things allocated, it can only be deleted from app thread
  if (g_application.IsCurrentThread())
  {
    delete capture;
  }
  else
  {
    capture->SetState(CAPTURESTATE_NEEDSDELETE);
    m_captures.push_back(capture);
  }

  if (!m_captures.empty())
    m_hasCaptures = true;
}

void CRenderManager::Capture(CRenderCapture* capture, unsigned int width, unsigned int height, int flags)
{
  CSingleLock lock(m_captCritSect);

  RemoveCapture(capture);

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

    if ((flags & CAPTUREFLAG_CONTINUOUS) || !(flags & CAPTUREFLAG_IMMEDIATELY))
    {
      //schedule this capture for a render and readout
      m_captures.push_back(capture);
    }
  }
  else
  {
    //schedule this capture for a render and readout
    m_captures.push_back(capture);
  }

  if (!m_captures.empty())
    m_hasCaptures = true;
}

void CRenderManager::ManageCaptures()
{
  //no captures, return here so we don't do an unnecessary lock
  if (!m_hasCaptures)
    return;

  CSingleLock lock(m_captCritSect);

  std::list<CRenderCapture*>::iterator it = m_captures.begin();
  while (it != m_captures.end())
  {
    CRenderCapture* capture = *it;

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
      else
      {
        it = m_captures.erase(it);
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
  CSharedLock lock(m_sharedSection);
  if (!m_pRenderer || !m_pRenderer->RenderCapture(capture))
    capture->SetState(CAPTURESTATE_FAILED);
}

void CRenderManager::RemoveCapture(CRenderCapture* capture)
{
  //remove this CRenderCapture from the list
  std::list<CRenderCapture*>::iterator it;
  while ((it = find(m_captures.begin(), m_captures.end(), capture)) != m_captures.end())
    m_captures.erase(it);
}

void CRenderManager::SetViewMode(int iViewMode)
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->SetViewMode(iViewMode);
  g_dataCacheCore.SignalVideoInfoChange();
}

void CRenderManager::FlipPage(volatile bool& bStop, double timestamp /* = 0LL*/, double pts /* = 0 */, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  { CSharedLock lock(m_sharedSection);

    if(bStop)
      return;

    if(!m_pRenderer) return;

    EPRESENTMETHOD presentmethod;

    EDEINTERLACEMODE deinterlacemode = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_DeinterlaceMode;
    EINTERLACEMETHOD interlacemethod = AutoInterlaceMethodInternal(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod);

    if(g_advancedSettings.m_videoDisableBackgroundDeinterlace && !g_graphicsContext.IsFullScreenVideo())
      deinterlacemode = VS_DEINTERLACEMODE_OFF;

    if (deinterlacemode == VS_DEINTERLACEMODE_OFF)
    {
      presentmethod = PRESENT_METHOD_SINGLE;
      sync = FS_NONE;
    }
    else
    {
      if (deinterlacemode == VS_DEINTERLACEMODE_AUTO && sync == FS_NONE)
        presentmethod = PRESENT_METHOD_SINGLE;
      else
      {
        bool invert = false;
        if      (interlacemethod == VS_INTERLACEMETHOD_RENDER_BLEND)            presentmethod = PRESENT_METHOD_BLEND;
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_WEAVE)            presentmethod = PRESENT_METHOD_WEAVE;
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED) { presentmethod = PRESENT_METHOD_WEAVE ; invert = true; }
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_BOB)              presentmethod = PRESENT_METHOD_BOB;
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)   { presentmethod = PRESENT_METHOD_BOB; invert = true; }
        else if (interlacemethod == VS_INTERLACEMETHOD_DXVA_BOB)                presentmethod = PRESENT_METHOD_BOB;
        else if (interlacemethod == VS_INTERLACEMETHOD_DXVA_BEST)               presentmethod = PRESENT_METHOD_BOB;
        else if (interlacemethod == VS_INTERLACEMETHOD_IMX_FASTMOTION_DOUBLE)   presentmethod = PRESENT_METHOD_BOB;
        else                                                                    presentmethod = PRESENT_METHOD_SINGLE;

        if (presentmethod != PRESENT_METHOD_SINGLE)
        {
          /* default to odd field if we want to deinterlace and don't know better */
          if (deinterlacemode == VS_DEINTERLACEMODE_FORCE && sync == FS_NONE)
            sync = FS_TOP;

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

    /* failsafe for invalid timestamps, to make sure queue always empties */
    if(timestamp > GetPresentTime() + 5.0)
      timestamp = GetPresentTime() + 5.0;

    CSingleLock lock2(m_presentlock);

    if(m_free.empty())
      return;

    if(source < 0)
      source = m_free.front();

    SPresent& m = m_Queue[source];
    m.timestamp     = timestamp;
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
}

RESOLUTION CRenderManager::GetResolution()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->GetResolution();
  else
    return RES_INVALID;
}

float CRenderManager::GetMaximumFPS()
{
  float fps;

  if (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOSCREEN_VSYNC) != VSYNC_DISABLED)
  {
    fps = (float)g_VideoReferenceClock.GetRefreshRate();
    if (fps <= 0) fps = g_graphicsContext.GetFPS();
  }
  else
    fps = 1000.0f;

  return fps;
}

void CRenderManager::RegisterRenderUpdateCallBack(const void *ctx, RenderUpdateCallBackFn fn)
{
  if (m_pRenderer)
    m_pRenderer->RegisterRenderUpdateCallBack(ctx, fn);
}

void CRenderManager::RegisterRenderFeaturesCallBack(const void *ctx, RenderFeaturesCallBackFn fn)
{
  if (m_pRenderer)
    m_pRenderer->RegisterRenderFeaturesCallBack(ctx, fn);
}

void CRenderManager::Render(bool clear, DWORD flags, DWORD alpha, bool gui)
{
  CSharedLock lock(m_sharedSection);

  if (m_renderState != STATE_CONFIGURED)
    return;

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
    m_overlays.Render(m_presentsource);
  }
}

bool CRenderManager::IsGuiLayer()
{
  { CSharedLock lock(m_sharedSection);

    if (!m_pRenderer)
      return false;

    if (m_pRenderer->IsGuiLayer() || m_renderedOverlay || m_overlays.HasOverlay(m_presentsource))
      return true;
  }
  return false;
}

bool CRenderManager::IsVideoLayer()
{
  { CSharedLock lock(m_sharedSection);

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
  CSingleLock lock(g_graphicsContext);
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
  CSingleLock lock(g_graphicsContext);
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
  CSingleLock lock(g_graphicsContext);
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
  float refresh = g_graphicsContext.GetFPS();
  if (g_graphicsContext.GetVideoResolution() == RES_WINDOW)
    refresh = 0; // No idea about refresh rate when windowed, just get the default latency
  m_displayLatency = (double) g_advancedSettings.GetDisplayLatency(refresh);
  //CLog::Log(LOGDEBUG, "CRenderManager::UpdateDisplayLatency - Latency set to %1.0f msec", m_displayLatency * 1000.0f);
}

void CRenderManager::UpdateResolution()
{
  if (m_bReconfigured)
  {
    CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    if (g_graphicsContext.IsFullScreenVideo() && g_graphicsContext.IsFullScreenRoot())
    {
      RESOLUTION res = GetResolution();
      g_graphicsContext.SetVideoResolution(res);
      UpdateDisplayLatency();
    }
    m_bReconfigured = false;
    g_dataCacheCore.SignalVideoInfoChange();
  }
}

// Get renderer info, can be called before configure
CRenderInfo CRenderManager::GetRenderInfo()
{
  CSharedLock lock(m_sharedSection);
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
  CSharedLock lock(m_sharedSection);
  if (!m_pRenderer)
    return -1;

  int index;
  {
    CSingleLock lock(m_presentlock);
    if (m_free.empty())
      return -1;
    index = m_free.front();
  }

  // TODO: this is a Windows onl thing and should go away
  if(m_pRenderer->AddVideoPicture(&pic, index))
    return 1;

  YV12Image image;
  if (m_pRenderer->GetImage(&image, index) < 0)
    return -1;

  if(pic.format == RENDER_FMT_YUV420P
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
  else if(pic.format == RENDER_FMT_VDPAU
       || pic.format == RENDER_FMT_VDPAU_420
       || pic.format == RENDER_FMT_OMXEGL
       || pic.format == RENDER_FMT_CVBREF
       || pic.format == RENDER_FMT_VAAPI
       || pic.format == RENDER_FMT_VAAPINV12
       || pic.format == RENDER_FMT_EGLIMG
       || pic.format == RENDER_FMT_MEDIACODEC
       || pic.format == RENDER_FMT_IMXMAP
       || pic.format == RENDER_FMT_MMAL)
    m_pRenderer->AddVideoPictureHW(pic, index);

  m_pRenderer->ReleaseImage(index, false);

  return index;
}

bool CRenderManager::Supports(ERENDERFEATURE feature)
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->Supports(feature);
  else
    return false;
}

bool CRenderManager::Supports(EDEINTERLACEMODE method)
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->Supports(method);
  else
    return false;
}

bool CRenderManager::Supports(EINTERLACEMETHOD method)
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->Supports(method);
  else
    return false;
}

bool CRenderManager::Supports(ESCALINGMETHOD method)
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->Supports(method);
  else
    return false;
}

EINTERLACEMETHOD CRenderManager::AutoInterlaceMethod(EINTERLACEMETHOD mInt)
{
  CSharedLock lock(m_sharedSection);
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

int CRenderManager::WaitForBuffer(volatile bool& bStop, int timeout)
{
  CSingleLock lock(m_presentlock);

  // check if gui is active and discard buffer if not
  // this keeps videoplayer going
  if (!m_bRenderGUI || !g_application.GetRenderGUI())
  {
    m_bRenderGUI = false;
    double presenttime = 0;
    double clock = GetPresentTime();
    if (!m_queued.empty())
    {
      int idx = m_queued.front();
      presenttime = m_Queue[idx].timestamp;
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
  CSingleLock lock(m_presentlock);

  if (m_queued.empty())
  {
    CLog::Log(LOGERROR, "CRenderManager::PrepareNextRender - asked to prepare with nothing available");
    m_presentstep = PRESENT_IDLE;
    m_presentevent.notifyAll();
    return;
  }

  double clocktime = GetPresentTime();
  double frametime = 1.0 / GetMaximumFPS();
  double correction = 0.0;
  int fps = g_VideoReferenceClock.GetRefreshRate();
  if((fps > 0) && g_graphicsContext.IsFullScreenVideo() && (clocktime != m_clock_framefinish))
  {
    correction = frametime;
  }

  /* see if any future queued frames are already due */
  std::deque<int>::reverse_iterator curr, prev;
  int idx;
  curr = prev = m_queued.rbegin();
  ++prev;
  while (prev != m_queued.rend())
  {
    if(clocktime > m_Queue[*prev].timestamp + correction                 /* previous frame is late */
    && clocktime > m_Queue[*curr].timestamp - frametime + correction)    /* selected frame is close to it's display time */
      break;
    ++curr;
    ++prev;
  }
  idx = *curr;

  /* in fullscreen we will block after render, but only for MAXPRESENTDELAY */
  bool next;
  if(g_graphicsContext.IsFullScreenVideo())
    next = (m_Queue[idx].timestamp <= clocktime + MAXPRESENTDELAY);
  else
    next = (m_Queue[idx].timestamp <= clocktime + frametime);

  if (next)
  {
    /* skip late frames */
    while(m_queued.front() != idx)
    {
      requeue(m_discard, m_queued);
      m_QueueSkip++;
    }

    m_presentstep   = PRESENT_FLIP;
    m_discard.push_back(m_presentsource);
    m_presentsource = idx;
    m_queued.pop_front();
    m_sleeptime = m_Queue[idx].timestamp - clocktime;
    m_presentpts = m_Queue[idx].pts;
    m_presentevent.notifyAll();
  }
}

void CRenderManager::DiscardBuffer()
{
  CSharedLock lock(m_sharedSection);
  CSingleLock lock2(m_presentlock);

  while(!m_queued.empty())
    requeue(m_discard, m_queued);

  m_Queue[m_presentsource].timestamp = GetPresentTime();

  if(m_presentstep == PRESENT_READY)
    m_presentstep = PRESENT_IDLE;
  m_presentevent.notifyAll();
}

bool CRenderManager::GetStats(double &sleeptime, double &pts, int &queued, int &discard)
{
  CSingleLock lock(m_presentlock);
  sleeptime = m_sleeptime;
  pts = m_presentpts;
  queued = m_queued.size();
  discard  = m_discard.size();
  return true;
}
