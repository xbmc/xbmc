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
 
#include "stdafx.h"
#include "RenderManager.h"
#include "utils/CriticalSection.h"

#ifndef HAS_SDL
#include "PixelShaderRenderer.h"
#include "ComboRenderer.h"
#include "RGBRenderer.h"
#include "RGBRendererV2.h"
#endif
#include "Application.h"
#include "Settings.h"

#ifdef _LINUX
#include "PlatformInclude.h"
#endif

#ifdef HAS_SDL_OPENGL
#include "Application.h"
#include "LinuxRendererGL.h"
#else 
#include "LinuxRenderer.h"
#endif

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif

/* to use the same as player */
#include "../dvdplayer/DVDClock.h"

CXBoxRenderManager g_renderManager;


#define MAXPRESENTDELAY 0.500

/* at any point we want an exclusive lock on rendermanager */
/* we must make sure we don't have a graphiccontext lock */
/* these two functions allow us to step out from that lock */
/* and reaquire it after having the exclusive lock */

#ifndef HAS_SDL
//VBlank information
HANDLE g_eventVBlank=NULL;
void VBlankCallback(D3DVBLANKDATA *pData)
{
  PulseEvent(g_eventVBlank);
}
#endif

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

CXBoxRenderManager::CXBoxRenderManager()
{
  m_pRenderer = NULL;
  m_bPauseDrawing = false;
  m_bIsStarted = false;

  m_presentfield = FS_NONE;
  m_presenttime = 0;
  m_rendermethod = 0;
}

CXBoxRenderManager::~CXBoxRenderManager()
{
  delete m_pRenderer;
  m_pRenderer = NULL;
}

/* These is based on QueryPerformanceCounter */
double CXBoxRenderManager::GetPresentTime()
{
  return CDVDClock::GetAbsoluteClock() / DVD_TIME_BASE;
}

void CXBoxRenderManager::WaitPresentTime(double presenttime)
{
  double now = GetPresentTime();
  while(now + 0.001 < presenttime)
  {
    Sleep(1); //this will be replaced with waiting for a vsync
    now = GetPresentTime();
  }
}
bool CXBoxRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  /* all frames before this should be rendered */
  WaitPresentTime(m_presenttime);

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
  }
  
  return result;
}

void CXBoxRenderManager::Update(bool bPauseDrawing)
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_bPauseDrawing = bPauseDrawing;
  if (m_pRenderer)
  {
    m_pRenderer->Update(bPauseDrawing);
  }
}

void CXBoxRenderManager::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  CSharedLock lock(m_sharedSection);

#ifdef HAS_SDL_OPENGL  
  if (m_pRenderer)
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_LAST, alpha);
#endif
}

unsigned int CXBoxRenderManager::PreInit()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

#ifndef HAS_SDL
  if(!g_eventVBlank)
  {
    //Only do this on first run
    g_eventVBlank = CreateEvent(NULL,FALSE,FALSE,NULL);
    D3DDevice::SetVerticalBlankCallback((D3DVBLANKCALLBACK)VBlankCallback);
  }
#endif

  m_bIsStarted = false;
  m_bPauseDrawing = false;
  if (!m_pRenderer)
  { 
#ifndef HAS_SDL
    // no renderer
    m_rendermethod = g_guiSettings.GetInt("videoplayer.rendermethod");
    if (m_rendermethod == RENDER_OVERLAYS)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected Overlay-Renderer");
      m_pRenderer = new CComboRenderer(g_graphicsContext.Get3DDevice());
    }
    else if (m_rendermethod == RENDER_HQ_RGB_SHADER)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected RGB-Renderer");
      m_pRenderer = new CRGBRenderer(g_graphicsContext.Get3DDevice());
    }
    else if (m_rendermethod == RENDER_HQ_RGB_SHADERV2)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected RGB-Renderer V2");
      m_pRenderer = new CRGBRendererV2(g_graphicsContext.Get3DDevice());
    }
    else // if (g_guiSettings.GetInt("videoplayer.rendermethod") == RENDER_LQ_RGB_SHADER)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected LQShader-Renderer");
      m_pRenderer = new CPixelShaderRenderer(g_graphicsContext.Get3DDevice());
    }
#elif defined(HAS_SDL_OPENGL)
    m_pRenderer = new CLinuxRendererGL();
#else
    m_pRenderer = new CLinuxRenderer();
#endif
  }

  return m_pRenderer->PreInit();
}

void CXBoxRenderManager::UnInit()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_bIsStarted = false;
  if (m_pRenderer)
  {
    m_pRenderer->UnInit();
#ifndef _LINUX
    delete m_pRenderer;
    m_pRenderer = NULL;
#endif
  }
}

void CXBoxRenderManager::SetupScreenshot()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->SetupScreenshot();
}

#ifndef HAS_SDL
void CXBoxRenderManager::CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height)
#else
void CXBoxRenderManager::CreateThumbnail(SDL_Surface * surface, unsigned int width, unsigned int height)
#endif
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->CreateThumbnail(surface, width, height);
}

void CXBoxRenderManager::FlipPage(volatile bool& bStop, double timestamp /* = 0LL*/, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  if(timestamp - GetPresentTime() > MAXPRESENTDELAY)
    timestamp =  GetPresentTime() + MAXPRESENTDELAY;

  /* make sure any queued frame was presented */
  if(g_graphicsContext.IsFullScreenVideo())
    while(!g_application.WaitFrame(100) && !bStop) {}
  else
    WaitPresentTime(timestamp);

  if(bStop)
    return;

  { CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    if(!m_pRenderer) return;

    m_presenttime  = timestamp;
    m_presentfield = sync;

    m_pRenderer->FlipPage(source);
  }

  g_application.NewFrame();
  g_application.WaitFrame(1); // we give the application thread 1ms to present
}

float CXBoxRenderManager::GetMaximumFPS()
{
  float fps;

  if (g_videoConfig.GetVSyncMode() == VSYNC_ALWAYS || g_videoConfig.GetVSyncMode() == VSYNC_VIDEO) 
    fps = g_graphicsContext.GetFPS();
  else
    fps = 1000.0f;

#ifndef HAS_SDL
  if( m_rendermethod == RENDER_HQ_RGB_SHADER
   || m_rendermethod == RENDER_HQ_RGB_SHADERV2)
  {
    EINTERLACEMETHOD method = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;

    if((method == VS_INTERLACEMETHOD_AUTO && m_presentfield != FS_NONE)
    ||  method == VS_INTERLACEMETHOD_RENDER_BOB )
      fps *= 0.5;
  }
#endif
  
  return fps;
}

bool CXBoxRenderManager::SupportsBrightness()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->SupportsBrightness();
  return false;
}

bool CXBoxRenderManager::SupportsContrast()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->SupportsContrast();
  return false;
}

bool CXBoxRenderManager::SupportsGamma()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->SupportsGamma();
  return false;
}

void CXBoxRenderManager::Present()
{
  CSharedLock lock(m_sharedSection);

  if (!m_pRenderer)
  {
    CLog::Log(LOGERROR, "%s called without valid Renderer object", __FUNCTION__);
    return;
  }
  
  EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;

  /* check for forced fields */
  if( mInt == VS_INTERLACEMETHOD_AUTO && m_presentfield != FS_NONE )
  {
    /* this is uggly to do on each frame, should only need be done once */
    int mResolution = g_graphicsContext.GetVideoResolution();
#if defined (HAS_SDL)
    if (1)
#else
    if( m_rendermethod == RENDER_HQ_RGB_SHADER 
     || m_rendermethod == RENDER_HQ_RGB_SHADERV2 )
#endif
      mInt = VS_INTERLACEMETHOD_RENDER_BOB;
    else if( mResolution == HDTV_480p_16x9 
          || mResolution == HDTV_480p_4x3 
          || mResolution == HDTV_720p 
          || mResolution == HDTV_1080i )
      mInt = VS_INTERLACEMETHOD_RENDER_BLEND;
    else
      mInt = VS_INTERLACEMETHOD_RENDER_BOB;
  }
  else if( mInt == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED )
  {
    /* all methods should default to odd if nothing is specified */
    if( m_presentfield == FS_EVEN )
      m_presentfield = FS_ODD;
    else
      m_presentfield = FS_EVEN;
  }

  if( mInt == VS_INTERLACEMETHOD_RENDER_BOB || mInt == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
    PresentBob();
  else if( mInt == VS_INTERLACEMETHOD_RENDER_WEAVE || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
    PresentWeave();
  else if( mInt == VS_INTERLACEMETHOD_RENDER_BLEND )
    PresentBlend();
  else
    PresentSingle();
  
  /* wait for this present to be valid */
  if(g_graphicsContext.IsFullScreenVideo())
    WaitPresentTime(m_presenttime);
}

/* simple present method */
void CXBoxRenderManager::PresentSingle()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, 0, 255);

#ifndef HAS_SDL
  D3DDevice::Present( NULL, NULL, NULL, NULL );
#endif
}

/* new simpler method of handling interlaced material, *
 * we just render the two fields right after eachother */
void CXBoxRenderManager::PresentBob()
{
  CSingleLock lock(g_graphicsContext);

  if( m_presentfield == FS_EVEN )
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN | RENDER_FLAG_NOUNLOCK , 255);
  else
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD | RENDER_FLAG_NOUNLOCK, 255);

#ifndef HAS_SDL
  if( m_presenttime )
  {
    D3DDevice::Present( NULL, NULL, NULL, NULL );
  }
  else
  {
    /* if no present time, assume we are in a hurry */
    /* try to present first field directly          */
    DWORD interval;
    D3DDevice::GetRenderState(D3DRS_PRESENTATIONINTERVAL, &interval);
    D3DDevice::SetRenderState(D3DRS_PRESENTATIONINTERVAL, D3DPRESENT_INTERVAL_IMMEDIATE);
    D3DDevice::Present( NULL, NULL, NULL, NULL );
    D3DDevice::SetRenderState(D3DRS_PRESENTATIONINTERVAL, interval);
  }
#elif defined (HAS_SDL_OPENGL)

  return; // hack for now untill bob rendering is corrected.

#endif

  /* render second field */
  if( m_presentfield == FS_EVEN )
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD | RENDER_FLAG_NOLOCK, 255);
  else
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN | RENDER_FLAG_NOLOCK, 255);
#ifndef HAS_SDL
  D3DDevice::Present( NULL, NULL, NULL, NULL );
#endif
}

void CXBoxRenderManager::PresentBlend()
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

#ifndef HAS_SDL
  D3DDevice::Present( NULL, NULL, NULL, NULL );
#endif
}

/* renders the two fields as one, but doing fieldbased *
 * scaling then reinterlaceing resulting image         */
void CXBoxRenderManager::PresentWeave()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, RENDER_FLAG_BOTH, 255);

#ifndef HAS_SDL
  //If we have interlaced video, we have to sync to only render on even fields
  D3DFIELD_STATUS mStatus;
  D3DDevice::GetDisplayFieldStatus(&mStatus);

#ifdef PROFILE
  m_presentfield = FS_NONE;
#endif

  //If this was not the correct field. we have to wait for the next one.. damn
  if( (mStatus.Field == D3DFIELD_EVEN && m_presentfield == FS_EVEN) ||
      (mStatus.Field == D3DFIELD_ODD && m_presentfield == FS_ODD) )
  {
    if( WaitForSingleObject(g_eventVBlank, 500) == WAIT_TIMEOUT )
      CLog::Log(LOGERROR, __FUNCTION__" - Waiting for vertical-blank timed out");
  }
  D3DDevice::Present( NULL, NULL, NULL, NULL );
#endif
}

void CXBoxRenderManager::Recover()
{
#ifdef HAVE_LIBVDPAU
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);
  if (g_VDPAU)
  {
    glFlush(); // attempt to have gpu done with pixmap
    g_VDPAU->CheckRecover(true);
  }
#endif
}
