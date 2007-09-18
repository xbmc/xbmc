/*
* XBoxMediaCenter
* Copyright (c) 2003 Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#ifdef _LINUX
#include "PlatformInclude.h"
#endif

#ifdef HAS_SDL_OPENGL
#include "LinuxRendererGL.h"
#include "LinuxRendererATI.h"
#else 
#include "LinuxRenderer.h"
#endif

CXBoxRenderManager g_renderManager;


#define MAXPRESENTDELAY 500

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


CXBoxRenderManager::CXBoxRenderManager()
{
  m_pRenderer = NULL;
  m_bPauseDrawing = false;
  m_bIsStarted = false;

  m_presentdelay = 5; //Just a guess to what delay we have
  m_presentfield = FS_NONE;
  m_presenttime = 0;
  m_rendermethod = 0;
}

CXBoxRenderManager::~CXBoxRenderManager()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
    delete m_pRenderer;
  m_pRenderer = NULL;
}

bool CXBoxRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);      

  if(!m_pRenderer) 
  {
    RestoreCriticalSection(g_graphicsContext, locks);
    CLog::Log(LOGERROR, "%s called without a valid Renderer object", __FUNCTION__);
    return false;
  }

  bool result = m_pRenderer->Configure(width, height, d_width, d_height, fps, flags);
  if(result)
  {
    if( flags & CONF_FLAGS_FULLSCREEN )
    {
      lock.Leave();
      g_applicationMessenger.SwitchToFullscreen();
      lock.Enter();
    }
    m_pRenderer->Update(false);
    m_bIsStarted = true;
  }
  
  RestoreCriticalSection(g_graphicsContext, locks);
  return result;
}

void CXBoxRenderManager::Update(bool bPauseDrawing)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  m_bPauseDrawing = bPauseDrawing;
  if (m_pRenderer)
  {
    m_pRenderer->Update(bPauseDrawing);
  }
}

void CXBoxRenderManager::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CSharedLock lock(m_sharedSection); 
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
    m_pRenderer->RenderUpdate(clear, flags, alpha);
}

unsigned int CXBoxRenderManager::PreInit()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

#ifndef HAS_SDL
  if(!g_eventVBlank)
  {
    //Only do this on first run
    g_eventVBlank = CreateEvent(NULL,FALSE,FALSE,NULL);
    D3DDevice::SetVerticalBlankCallback((D3DVBLANKCALLBACK)VBlankCallback);
  }
#endif

  /* no pedning present */
  m_eventPresented.Set();

  m_bIsStarted = false;
  m_bPauseDrawing = false;
  m_presentdelay = 5;
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
#ifdef _WIN32
      m_pRenderer = new CLinuxRendererATI(true); // Win32 likes this renderer better (no context support yet)
#else
    if (g_graphicsContext.getScreenSurface()->GetGLVendor().find("ATI Technologies Inc.") != std::string::npos)
      m_pRenderer = new CLinuxRendererATI(); // We need a special ATI renderer since ATI drivers can't seem to handle multi-threaded rendering
    else
      m_pRenderer = new CLinuxRendererGL();
#endif
#else
    m_pRenderer = new CLinuxRenderer();
#endif
  }

  return m_pRenderer->PreInit();
}

void CXBoxRenderManager::UnInit()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);

  m_bStop = true;
  m_eventFrame.Set();
  StopThread();

  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  m_bIsStarted = false;
  if (m_pRenderer)
  {
    m_pRenderer->UnInit();
    delete m_pRenderer;
    m_pRenderer = NULL;
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
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
    m_pRenderer->CreateThumbnail(surface, width, height);
}


void CXBoxRenderManager::FlipPage(DWORD delay /* = 0LL*/, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  DWORD timestamp = 0;

  if(delay > MAXPRESENTDELAY) delay = MAXPRESENTDELAY;
  if(delay > 0)
    timestamp = GetTickCount() + delay;

  CSharedLock lock(m_sharedSection);
  if(!m_pRenderer) return;

  /* make sure any previous frame was presented */
  if( !m_eventPresented.WaitMSec(MAXPRESENTDELAY*2) )
    CLog::Log(LOGERROR, " - Timeout waiting for previous frame to be presented");

  m_presenttime = timestamp;
  m_presentfield = sync;

  CSingleLock lock2(g_graphicsContext);
#ifdef HAS_SDL_OPENGL
  if( 0 ) /*disable async renderer*/
#else
  if( g_graphicsContext.IsFullScreenVideo() )
#endif
  {
    lock2.Leave();

    g_graphicsContext.AcquireCurrentContext();
    m_pRenderer->FlipPage(source);
    g_graphicsContext.ReleaseCurrentContext();
    if( CThread::ThreadHandle() == NULL ) CThread::Create();
    m_eventFrame.Set();
  }
  else
  {
    //g_graphicsContext.ReleaseCurrentContext();
    lock2.Leave();

    /* if we are not in fullscreen, we don't control when we render */
    /* so we must await the time and flip then */
#ifdef HAS_SDL_OPENGL
    // In OpenGL, we shouldn't be waiting for CThread::m_bStop since rendering is
    // happening from the main thread.
    while( timestamp > GetTickCount() ) Sleep(1);
#else
    while( timestamp > GetTickCount() && !CThread::m_bStop) Sleep(1);
#endif
    //m_pRenderer->FlipPage(source);
    m_eventPresented.Set();
  }
}

void CXBoxRenderManager::Present()
{
#ifdef HAS_SDL_OPENGL
  if (!m_pRenderer)
  {
    CLog::Log(LOGERROR, "%s called without valid Renderer object", __FUNCTION__);
    return;
  }
#endif

  EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;

  /* check for forced fields */
  if( mInt == VS_INTERLACEMETHOD_AUTO && m_presentfield != FS_NONE )
  {
    /* this is uggly to do on each frame, should only need be done once */
    int mResolution = g_graphicsContext.GetVideoResolution();
    if( m_rendermethod == RENDER_HQ_RGB_SHADERV2 )
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

  /* if we have a field, we will have a presentation delay */
  if( m_presentfield == FS_NONE )
    m_presentdelay = 20;
  else
    m_presentdelay = 40;

  if( m_presenttime >= m_presentdelay )
    m_presenttime -=  m_presentdelay;
  else
    m_presenttime = 0;

  if( mInt == VS_INTERLACEMETHOD_RENDER_BOB || mInt == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
    PresentBob();
  else if( mInt == VS_INTERLACEMETHOD_RENDER_WEAVE || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
    PresentWeave();
  else if( mInt == VS_INTERLACEMETHOD_RENDER_BLEND )
    PresentBlend();
  else
    PresentSingle();
}

/* simple present method */
void CXBoxRenderManager::PresentSingle()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, 0, 255);

  int nTicks = GetTickCount();
  if (m_presenttime > nTicks) Sleep(m_presenttime - nTicks);
  //while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);

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
    /* wait for timestamp */
    while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);
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


  /* wait for timestamp */
  while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);

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

  /* wait for timestamp */
  while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);

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

void CXBoxRenderManager::Process()
{
  CLog::Log(LOGINFO, "Starting async renderer thread");
  float actualdelay = (float)m_presentdelay;

  SetPriority(THREAD_PRIORITY_TIME_CRITICAL);
  SetName("AsyncRenderer");
  while( !m_bStop )
  {
    //Wait for new frame or an stop event
    g_graphicsContext.ReleaseCurrentContext();
    m_eventFrame.Wait();
    if( m_bStop )
    { 
      return;
    }

    DWORD dwTimeStamp = GetTickCount();
    try
    {
      CSharedLock lock(m_sharedSection);
      CSingleLock lock2(g_graphicsContext);

      if( m_pRenderer && g_graphicsContext.IsFullScreenVideo() )
      {
	// If we need to render acquire the context.
	// Here, we are sharing the main thread's rendering context
	g_graphicsContext.AcquireCurrentContext();
        Present();
	g_graphicsContext.ReleaseCurrentContext();
      }
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "CLinuxRendererGL::Process() - Exception thrown in flippage");
    }
    m_eventPresented.Set();
    const int TC = 100; /* time (frame) constant for convergence */
    actualdelay = ( actualdelay * (TC-1) + (GetTickCount() - dwTimeStamp) ) / TC;
  }
  //g_graphicsContext.ReleaseCurrentContext();
}

