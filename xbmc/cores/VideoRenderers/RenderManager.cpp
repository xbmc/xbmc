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
#include "utils/CriticalSection.h"
#include "VideoReferenceClock.h"
#include "MathUtils.h"
#include "utils/SingleLock.h"
#include "utils/log.h"

#include "Application.h"
#include "Settings.h"
#include "GUISettings.h"
#include "SystemGlobals.h"

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

/* to use the same as player */
#include "../dvdplayer/DVDClock.h"

CXBMCRenderManager& g_renderManager = g_SystemGlobals.m_renderManager;

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
    m_count = ExitCriticalSection(m_owned);
    m_lock  = new T(section);
    if(immidiate)
    {
      RestoreCriticalSection(m_owned, m_count);
      m_count = 0;
    }
  }
  ~CRetakeLock()
  {
    delete m_lock;
    RestoreCriticalSection(m_owned, m_count);
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
  m_presentmethod = VS_INTERLACEMETHOD_NONE;
  m_bReconfigured = false;
}

CXBMCRenderManager::~CXBMCRenderManager()
{
  delete m_pRenderer;
  m_pRenderer = NULL;
}

/* These is based on CurrentHostCounter() */
double CXBMCRenderManager::GetPresentTime()
{
  return CDVDClock::GetAbsoluteClock() / DVD_TIME_BASE;
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
  int fps = g_VideoReferenceClock.GetRefreshRate();
  if(fps <= 0)
  {
    /* smooth video not enabled */
    CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE);
    return;
  }

  double frametime = g_VideoReferenceClock.GetSpeed() / fps;

  presenttime     += m_presentcorr * frametime;

  double clock     = CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE) / DVD_TIME_BASE;
  double target    = 0.5;
  double error     = ( clock - presenttime ) / frametime - target;

  m_presenterr   = error;

  // correct error so it targets the closest vblank
  error = wrap(error, 0.0 - target, 1.0 - target);

  // scale the error used for correction,
  // based on how much buffer we have on
  // that side of the target
  if(error > 0)
    error /= 2.0 * (1.0 - target);
  if(error < 0)
    error /= 2.0 * (0.0 + target);

  m_presentcorr = wrap(m_presentcorr + error * 0.02, target - 1.0, target);
  //printf("%f %f % 2.0f%% % f % f\n", presenttime, clock, m_presentcorr * 100, error, error_org);
}

CStdString CXBMCRenderManager::GetVSyncState()
{
  CStdString state;
  state.Format("sync:%+3d%% error:%2d%%"
              ,     MathUtils::round_int(m_presentcorr * 100)
              , abs(MathUtils::round_int(m_presenterr  * 100)));
  return state;
}

bool CXBMCRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
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

  bool result = m_pRenderer->Configure(width, height, d_width, d_height, fps, flags);
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

  CSharedLock lock(m_sharedSection);

  if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE
   || m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOTH, alpha);
  else
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_LAST, alpha);

  m_overlays.Render();

  m_presentstep = PRESENT_IDLE;
  m_presentevent.Set();
}

unsigned int CXBMCRenderManager::PreInit()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_presentcorr = 0.0;
  m_presenterr  = 0.0;

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

void CXBMCRenderManager::SetupScreenshot()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->SetupScreenshot();
}

void CXBMCRenderManager::CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height)
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->CreateThumbnail(texture, width, height);
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
    m_presentmethod = g_settings.m_currentVideoSettings.m_InterlaceMethod;

    /* select render method for auto */
    if(m_presentmethod == VS_INTERLACEMETHOD_AUTO)
    {
      if(m_presentfield == FS_NONE)
        m_presentmethod = VS_INTERLACEMETHOD_NONE;
      else if(m_pRenderer->Supports(VS_INTERLACEMETHOD_RENDER_BOB))
        m_presentmethod = VS_INTERLACEMETHOD_RENDER_BOB;
      else
        m_presentmethod = VS_INTERLACEMETHOD_NONE;
    }

    /* default to odd field if we want to deinterlace and don't know better */
    if(m_presentfield == FS_NONE && m_presentmethod != VS_INTERLACEMETHOD_NONE)
      m_presentfield = FS_ODD;

    /* invert present field if we have one of those methods */
    if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED
     || m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED )
    {
      if( m_presentfield == FS_EVEN )
        m_presentfield = FS_ODD;
      else
        m_presentfield = FS_EVEN;
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

  CSharedLock lock(m_sharedSection);

  if     ( m_presentmethod == VS_INTERLACEMETHOD_RENDER_BOB
        || m_presentmethod == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
    PresentBob();
  else if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE
        || m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
    PresentWeave();
  else if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_BLEND )
    PresentBlend();
  else
    PresentSingle();

  m_overlays.Render();

  /* wait for this present to be valid */
  if(g_graphicsContext.IsFullScreenVideo())
    WaitPresentTime(m_presenttime);

  m_presentevent.Set();
}

/* simple present method */
void CXBMCRenderManager::PresentSingle()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, 0, 255);
  m_presentstep = PRESENT_IDLE;
}

/* new simpler method of handling interlaced material, *
 * we just render the two fields right after eachother */
void CXBMCRenderManager::PresentBob()
{
  CSingleLock lock(g_graphicsContext);

  if(m_presentstep == PRESENT_FRAME)
  {
    if( m_presentfield == FS_EVEN)
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN, 255);
    else
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD, 255);
    m_presentstep = PRESENT_FRAME2;
    g_application.NewFrame();
  }
  else
  {
    if( m_presentfield == FS_ODD)
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN, 255);
    else
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD, 255);
    m_presentstep = PRESENT_IDLE;
  }
}

void CXBMCRenderManager::PresentBlend()
{
  CSingleLock lock(g_graphicsContext);

  if( m_presentfield == FS_EVEN )
  {
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN | RENDER_FLAG_NOOSD, 255);
    m_pRenderer->RenderUpdate(false, RENDER_FLAG_ODD, 128);
  }
  else
  {
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD | RENDER_FLAG_NOOSD, 255);
    m_pRenderer->RenderUpdate(false, RENDER_FLAG_EVEN, 128);
  }
  m_presentstep = PRESENT_IDLE;
}

/* renders the two fields as one, but doing fieldbased *
 * scaling then reinterlaceing resulting image         */
void CXBMCRenderManager::PresentWeave()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, RENDER_FLAG_BOTH, 255);
  m_presentstep = PRESENT_IDLE;
}

void CXBMCRenderManager::Recover()
{
#ifdef HAS_GL
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
