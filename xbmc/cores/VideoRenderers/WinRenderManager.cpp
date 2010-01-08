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
#include "WinRenderManager.h"


CWinRenderManager g_renderManager;


#define MAXPRESENTDELAY 500

/* at any point we want an exclusive lock on rendermanager */
/* we must make sure we don't have a graphiccontext lock */

CWinRenderManager::CWinRenderManager()
{
  m_pRenderer = NULL;
  m_bPauseDrawing = false;
  m_bIsStarted = false;
}

CWinRenderManager::~CWinRenderManager()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  delete m_pRenderer;
  m_pRenderer = NULL;
}

bool CWinRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);

  if(!m_pRenderer)
  {
    RestoreCriticalSection(g_graphicsContext, locks);
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

void CWinRenderManager::Update(bool bPauseDrawing)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  m_bPauseDrawing = bPauseDrawing;
  if (m_pRenderer)
    m_pRenderer->Update(bPauseDrawing);
}

void CWinRenderManager::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CSharedLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
    m_pRenderer->RenderUpdate(clear, flags, alpha);
}

unsigned int CWinRenderManager::PreInit()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  m_bIsStarted = false;
  m_bPauseDrawing = false;

  if (!m_pRenderer)
  { // no renderer
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected Win RGB-Renderer ");
      m_pRenderer = new CPixelShaderRenderer(g_graphicsContext.Get3DDevice());
  }
  return m_pRenderer->PreInit();
}

void CWinRenderManager::UnInit()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
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

void CWinRenderManager::SetupScreenshot()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->SetupScreenshot();
}

void CWinRenderManager::CreateThumbnail(SurfacePtr surface, unsigned int width, unsigned int height)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
    m_pRenderer->CreateThumbnail(surface, width, height);
}

void CWinRenderManager::FlipPage(DWORD delay /* = 0LL*/, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  DWORD timestamp = 0;

  if(delay > MAXPRESENTDELAY) delay = MAXPRESENTDELAY;
  if(delay > 0)
    timestamp = GetTickCount() + delay;

  while( timestamp > GetTickCount() )
    Sleep(1);

  CSharedLock lock(m_sharedSection);

  if(!m_pRenderer)
    return;

  m_pRenderer->FlipPage(source);
}
