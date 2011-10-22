/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "RenderManager.h"
#include "threads/CriticalSection.h"
#include "video/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "Application.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"

#ifdef _LINUX
#include "PlatformInclude.h"
#endif

#if defined(HAS_GL)
  #include "LinuxRendererGL.h"
#elif HAS_GLES == 2
  #include "LinuxRendererGLES.h"
#elif defined(HAS_DX)
  #include "WinRenderer.h"
#elif defined(HAS_SDL)
  #include "LinuxRenderer.h"
#endif

#include "RenderCapture.h"

/* to use the same as player */
#include "../dvdplayer/DVDClock.h"
#include "../dvdplayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "../dvdplayer/DVDCodecs/DVDCodecUtils.h"

#define MAXPRESENTDELAY 0.500

/* at any point we want an exclusive lock on rendermanager */
/* we must make sure we don't have a graphiccontext lock */
/* these two functions allow us to step out from that lock */
/* and reaquire it after having the exclusive lock */

template<class T>
class CRetakeLock
{
public:
  CRetakeLock(CSharedSection &section, bool immidiate = true, CCriticalSection &owned = g_graphicsContext)
    : m_owned(owned)
  {
    m_count = m_owned.exit();
    m_lock  = new T(section);
    if(immidiate)
    {
      m_owned.restore(m_count);
      m_count = 0;
    }
  }
  ~CRetakeLock()
  {
    delete m_lock;
    m_owned.restore(m_count);
  }
  void Leave() { m_lock->Leave(); }
  void Enter() { m_lock->Enter(); }

private:
  T*                m_lock;
  CCriticalSection &m_owned;
  DWORD             m_count;
};

CXBMCRenderManager::CXBMCRenderManager()
{
  m_pRenderer = NULL;
  m_bPauseDrawing = false;
  m_bIsStarted = false;

  m_presentfield = FS_NONE;
  m_presenttime = 0;
  m_presentstep = PRESENT_IDLE;
  m_rendermethod = 0;
  m_presentsource = 0;
  m_presentmethod = PRESENT_METHOD_SINGLE;
  m_bReconfigured = false;
  m_hasCaptures = false;
}

CXBMCRenderManager::~CXBMCRenderManager()
{
  delete m_pRenderer;
  m_pRenderer = NULL;
}

/* These is based on CurrentHostCounter() */
double CXBMCRenderManager::GetPresentTime()
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

void CXBMCRenderManager::WaitPresentTime(double presenttime)
{
  double frametime;
  int fps = g_VideoReferenceClock.GetRefreshRate(&frametime);
  if(fps <= 0)
  {
    /* smooth video not enabled */
    CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE);
    return;
  }

  bool ismaster = CDVDClock::IsMasterClock();

  //the videoreferenceclock updates its clock on every vertical blank
  //we want every frame's presenttime to end up in the middle of two vblanks
  //if CDVDPlayerAudio is the master clock, we add a correction to the presenttime
  if (ismaster)
    presenttime += m_presentcorr * frametime;

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


  //if CDVDPlayerAudio is not the master clock, we change the clock speed slightly
  //to make every frame's presenttime end up in the middle of two vblanks
  if (!ismaster)
  {
    //integral correction, clamp to -0.5:0.5 range
    m_presentcorr = std::max(std::min(m_presentcorr + avgerror * 0.01, 0.1), -0.1);
    g_VideoReferenceClock.SetFineAdjust(1.0 - avgerror * 0.01 - m_presentcorr * 0.01);
  }
  else
  {
    //integral correction, wrap to -0.5:0.5 range
    m_presentcorr = wrap(m_presentcorr + avgerror * 0.01, target - 1.0, target);
    g_VideoReferenceClock.SetFineAdjust(1.0);
  }

  //printf("%f %f % 2.0f%% % f % f\n", presenttime, clock, m_presentcorr * 100, error, error_org);
}

CStdString CXBMCRenderManager::GetVSyncState()
{
  double avgerror = 0.0;
  for (int i = 0; i < ERRORBUFFSIZE; i++)
    avgerror += m_errorbuff[i];
  avgerror /= ERRORBUFFSIZE;

  CStdString state;
  state.Format("sync:%+3d%% avg:%3d%% error:%2d%%"
              ,     MathUtils::round_int(m_presentcorr * 100)
              ,     MathUtils::round_int(avgerror      * 100)
              , abs(MathUtils::round_int(m_presenterr  * 100)));
  return state;
}

bool CXBMCRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, unsigned int format)
{
  /* make sure any queued frame was fully presented */
  double timeout = m_presenttime + 0.1;
  while(m_presentstep != PRESENT_IDLE)
  {
    if(!m_presentevent.WaitMSec(100) && GetPresentTime() > timeout)
    {
      CLog::Log(LOGWARNING, "CRenderManager::Configure - timeout waiting for previous frame");
      break;
    }
  };

  CRetakeLock<CExclusiveLock> lock(m_sharedSection, false);
  if(!m_pRenderer)
  {
    CLog::Log(LOGERROR, "%s called without a valid Renderer object", __FUNCTION__);
    return false;
  }

  bool result = m_pRenderer->Configure(width, height, d_width, d_height, fps, flags, format);
  if(result)
  {
    if( flags & CONF_FLAGS_FULLSCREEN )
    {
      lock.Leave();
      g_application.getApplicationMessenger().SwitchToFullscreen();
      lock.Enter();
    }
    m_pRenderer->Update(false);
    m_bIsStarted = true;
    m_bReconfigured = true;
    m_presentstep = PRESENT_IDLE;
    m_presentevent.Set();
  }

  return result;
}

bool CXBMCRenderManager::IsConfigured()
{
  if (!m_pRenderer)
    return false;
  return m_pRenderer->IsConfigured();
}

void CXBMCRenderManager::Update(bool bPauseDrawing)
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_bPauseDrawing = bPauseDrawing;
  if (m_pRenderer)
  {
    m_pRenderer->Update(bPauseDrawing);
  }

  m_presentevent.Set();
}

void CXBMCRenderManager::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  { CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    if (!m_pRenderer)
      return;

    if(m_presentstep == PRESENT_FLIP)
    {
      m_overlays.Flip();
      m_pRenderer->FlipPage(m_presentsource);
      m_presentstep = PRESENT_FRAME;
      m_presentevent.Set();
    }
  }

  if (g_advancedSettings.m_videoDisableBackgroundDeinterlace)
  {
    CSharedLock lock(m_sharedSection);
    PresentSingle(clear, flags, alpha);
  }
  else
    Render(clear, flags, alpha);

  m_presentevent.Set();
}

unsigned int CXBMCRenderManager::PreInit()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_presentcorr = 0.0;
  m_presenterr  = 0.0;
  m_errorindex  = 0;
  memset(m_errorbuff, 0, sizeof(m_errorbuff));

  m_bIsStarted = false;
  m_bPauseDrawing = false;
  if (!m_pRenderer)
  {
#if defined(HAS_GL)
    m_pRenderer = new CLinuxRendererGL();
#elif HAS_GLES == 2
    m_pRenderer = new CLinuxRendererGLES();
#elif defined(HAS_DX)
    m_pRenderer = new CWinRenderer();
#elif defined(HAS_SDL)
    m_pRenderer = new CLinuxRenderer();
#endif
  }

  return m_pRenderer->PreInit();
}

void CXBMCRenderManager::UnInit()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_bIsStarted = false;

  m_overlays.Flush();

  // free renderer resources.
  // TODO: we may also want to release the renderer here.
  if (m_pRenderer)
    m_pRenderer->UnInit();
}

bool CXBMCRenderManager::Flush()
{
  if (!m_pRenderer)
    return true;

  if (g_application.IsCurrentThread())
  {
    CLog::Log(LOGDEBUG, "%s - flushing renderer", __FUNCTION__);

    CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    m_pRenderer->Flush();
    m_flushEvent.Set();
  }
  else
  {
    ThreadMessage msg = {TMSG_RENDERER_FLUSH};
    m_flushEvent.Reset();
    g_application.getApplicationMessenger().SendMessage(msg, false);
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

void CXBMCRenderManager::SetupScreenshot()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->SetupScreenshot();
}

CRenderCapture* CXBMCRenderManager::AllocRenderCapture()
{
  return new CRenderCapture;
}

void CXBMCRenderManager::ReleaseRenderCapture(CRenderCapture* capture)
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

void CXBMCRenderManager::Capture(CRenderCapture* capture, unsigned int width, unsigned int height, int flags)
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

void CXBMCRenderManager::ManageCaptures()
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

        it++;
      }
      else
      {
        it = m_captures.erase(it);
      }
    }
    else
    {
      it++;
    }
  }

  if (m_captures.empty())
    m_hasCaptures = false;
}

void CXBMCRenderManager::RenderCapture(CRenderCapture* capture)
{
  CSharedLock lock(m_sharedSection);
  if (!m_pRenderer || !m_pRenderer->RenderCapture(capture))
    capture->SetState(CAPTURESTATE_FAILED);
}

void CXBMCRenderManager::RemoveCapture(CRenderCapture* capture)
{
  //remove this CRenderCapture from the list
  std::list<CRenderCapture*>::iterator it;
  while ((it = find(m_captures.begin(), m_captures.end(), capture)) != m_captures.end())
    m_captures.erase(it);
}

void CXBMCRenderManager::FlipPage(volatile bool& bStop, double timestamp /* = 0LL*/, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  if(timestamp - GetPresentTime() > MAXPRESENTDELAY)
    timestamp =  GetPresentTime() + MAXPRESENTDELAY;

  /* can't flip, untill timestamp */
  if(!g_graphicsContext.IsFullScreenVideo())
    WaitPresentTime(timestamp);

  /* make sure any queued frame was fully presented */
  double timeout = m_presenttime + 1.0;
  while(m_presentstep != PRESENT_IDLE && !bStop)
  {
    if(!m_presentevent.WaitMSec(100) && GetPresentTime() > timeout && !bStop)
    {
      CLog::Log(LOGWARNING, "CRenderManager::FlipPage - timeout waiting for previous frame");
      return;
    }
  };

  if(bStop)
    return;

  { CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    if(!m_pRenderer) return;

    m_presenttime  = timestamp;
    m_presentfield = sync;
    m_presentstep  = PRESENT_FLIP;
    m_presentsource = source;
    EDEINTERLACEMODE deinterlacemode = g_settings.m_currentVideoSettings.m_DeinterlaceMode;
    EINTERLACEMETHOD interlacemethod = g_settings.m_currentVideoSettings.m_InterlaceMethod;
    if (interlacemethod == VS_INTERLACEMETHOD_AUTO)
      interlacemethod = m_pRenderer->AutoInterlaceMethod();

    bool invert = false;

    if (deinterlacemode == VS_DEINTERLACEMODE_OFF)
      m_presentmethod = PRESENT_METHOD_SINGLE;
    else
    {
      if (deinterlacemode == VS_DEINTERLACEMODE_AUTO && m_presentfield == FS_NONE)
        m_presentmethod = PRESENT_METHOD_SINGLE;
      else
      {
        if      (interlacemethod == VS_INTERLACEMETHOD_RENDER_BLEND)            m_presentmethod = PRESENT_METHOD_BLEND;
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_WEAVE)            m_presentmethod = PRESENT_METHOD_WEAVE;
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED) { m_presentmethod = PRESENT_METHOD_WEAVE ; invert = true; }
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_BOB)              m_presentmethod = PRESENT_METHOD_BOB;
        else if (interlacemethod == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)   { m_presentmethod = PRESENT_METHOD_BOB; invert = true; }
        else if (interlacemethod == VS_INTERLACEMETHOD_DXVA_BOB)                m_presentmethod = PRESENT_METHOD_BOB;
        else if (interlacemethod == VS_INTERLACEMETHOD_DXVA_BEST)               m_presentmethod = PRESENT_METHOD_BOB;
        else                                                                    m_presentmethod = PRESENT_METHOD_SINGLE;

        /* default to odd field if we want to deinterlace and don't know better */
        if (deinterlacemode == VS_DEINTERLACEMODE_FORCE && m_presentfield == FS_NONE)
          m_presentfield = FS_TOP;

        /* invert present field */
        if(invert)
        {
          if( m_presentfield == FS_BOT )
            m_presentfield = FS_TOP;
          else
            m_presentfield = FS_BOT;
        }
      }
    }

  }

  g_application.NewFrame();
  /* wait untill render thread have flipped buffers */
  timeout = m_presenttime + 1.0;
  while(m_presentstep == PRESENT_FLIP && !bStop)
  {
    if(!m_presentevent.WaitMSec(100) && GetPresentTime() > timeout && !bStop)
    {
      CLog::Log(LOGWARNING, "CRenderManager::FlipPage - timeout waiting for flip to complete");
      return;
    }
  }
}

float CXBMCRenderManager::GetMaximumFPS()
{
  float fps;

  if (g_guiSettings.GetInt("videoscreen.vsync") != VSYNC_DISABLED)
  {
    fps = (float)g_VideoReferenceClock.GetRefreshRate();
    if (fps <= 0) fps = g_graphicsContext.GetFPS();
  }
  else
    fps = 1000.0f;

  return fps;
}

void CXBMCRenderManager::Render(bool clear, DWORD flags, DWORD alpha)
{
  CSharedLock lock(m_sharedSection);

  if( m_presentmethod == PRESENT_METHOD_BOB )
    PresentBob(clear, flags, alpha);
  else if( m_presentmethod == PRESENT_METHOD_WEAVE )
    PresentWeave(clear, flags, alpha);
  else if( m_presentmethod == PRESENT_METHOD_BLEND )
    PresentBlend(clear, flags, alpha);
  else
    PresentSingle(clear, flags, alpha);

  m_overlays.Render();
}

void CXBMCRenderManager::Present()
{
  { CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    if (!m_pRenderer)
      return;

    if(m_presentstep == PRESENT_FLIP)
    {
      m_overlays.Flip();
      m_pRenderer->FlipPage(m_presentsource);
      m_presentstep = PRESENT_FRAME;
      m_presentevent.Set();
    }
  }

  Render(true, 0, 255);

  /* wait for this present to be valid */
  if(g_graphicsContext.IsFullScreenVideo())
    WaitPresentTime(m_presenttime);

  m_presentevent.Set();
}

/* simple present method */
void CXBMCRenderManager::PresentSingle(bool clear, DWORD flags, DWORD alpha)
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(clear, flags, alpha);
  m_presentstep = PRESENT_IDLE;
}

/* new simpler method of handling interlaced material, *
 * we just render the two fields right after eachother */
void CXBMCRenderManager::PresentBob(bool clear, DWORD flags, DWORD alpha)
{
  CSingleLock lock(g_graphicsContext);

  if(m_presentstep == PRESENT_FRAME)
  {
    if( m_presentfield == FS_BOT)
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_FIELD0, alpha);
    else
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_FIELD0, alpha);
    m_presentstep = PRESENT_FRAME2;
    g_application.NewFrame();
  }
  else
  {
    if( m_presentfield == FS_TOP)
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_FIELD1, alpha);
    else
      m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_FIELD1, alpha);
    m_presentstep = PRESENT_IDLE;
  }
}

void CXBMCRenderManager::PresentBlend(bool clear, DWORD flags, DWORD alpha)
{
  CSingleLock lock(g_graphicsContext);

  if( m_presentfield == FS_BOT )
  {
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOT | RENDER_FLAG_NOOSD, alpha);
    m_pRenderer->RenderUpdate(false, flags | RENDER_FLAG_TOP, alpha / 2);
  }
  else
  {
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_TOP | RENDER_FLAG_NOOSD, alpha);
    m_pRenderer->RenderUpdate(false, flags | RENDER_FLAG_BOT, alpha / 2);
  }
  m_presentstep = PRESENT_IDLE;
}

/* renders the two fields as one, but doing fieldbased *
 * scaling then reinterlaceing resulting image         */
void CXBMCRenderManager::PresentWeave(bool clear, DWORD flags, DWORD alpha)
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOTH, alpha);
  m_presentstep = PRESENT_IDLE;
}

void CXBMCRenderManager::Recover()
{
#if defined(HAS_GL) && !defined(TARGET_DARWIN)
  glFlush(); // attempt to have gpu done with pixmap and vdpau
#endif
}

void CXBMCRenderManager::UpdateResolution()
{
  if (m_bReconfigured)
  {
    CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    if (g_graphicsContext.IsFullScreenVideo() && g_graphicsContext.IsFullScreenRoot())
    {
      RESOLUTION res = GetResolution();
      g_graphicsContext.SetVideoResolution(res);
    }
    m_bReconfigured = false;
  }
}


int CXBMCRenderManager::AddVideoPicture(DVDVideoPicture& pic)
{
  CSharedLock lock(m_sharedSection);
  if (!m_pRenderer)
    return -1;

  if(m_pRenderer->AddVideoPicture(&pic))
    return 1;

  YV12Image image;
  int index = m_pRenderer->GetImage(&image);

  if(index < 0)
    return index;

  if(pic.format == DVDVideoPicture::FMT_YUV420P)
  {
    CDVDCodecUtils::CopyPicture(&image, &pic);
  }
  else if(pic.format == DVDVideoPicture::FMT_NV12)
  {
    CDVDCodecUtils::CopyNV12Picture(&image, &pic);
  }
  else if(pic.format == DVDVideoPicture::FMT_YUY2
       || pic.format == DVDVideoPicture::FMT_UYVY)
  {
    CDVDCodecUtils::CopyYUV422PackedPicture(&image, &pic);
  }
  else if(pic.format == DVDVideoPicture::FMT_DXVA)
  {
    CDVDCodecUtils::CopyDXVA2Picture(&image, &pic);
  }
#ifdef HAVE_LIBVDPAU
  else if(pic.format == DVDVideoPicture::FMT_VDPAU)
    m_pRenderer->AddProcessor(pic.vdpau);
#endif
#ifdef HAVE_LIBOPENMAX
  else if(pic.format == DVDVideoPicture::FMT_OMXEGL)
    m_pRenderer->AddProcessor(pic.openMax, &pic);
#endif
#ifdef HAVE_VIDEOTOOLBOXDECODER
  else if(pic.format == DVDVideoPicture::FMT_CVBREF)
    m_pRenderer->AddProcessor(pic.vtb, &pic);
#endif
#ifdef HAVE_LIBVA
  else if(pic.format == DVDVideoPicture::FMT_VAAPI)
    m_pRenderer->AddProcessor(*pic.vaapi);
#endif
  m_pRenderer->ReleaseImage(index, false);

  return index;
}
