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
  if (m_pRenderer)
    delete m_pRenderer;
  m_pRenderer = NULL;
}

bool CWinRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(!m_pRenderer) 
    return false;
 
  bool result = m_pRenderer->Configure(width, height, d_width, d_height, fps, flags);
  if(result)
  {
    if( flags & CONF_FLAGS_FULLSCREEN )
      g_applicationMessenger.SwitchToFullscreen();
 
    m_pRenderer->Update(false);
    m_bIsStarted = true;
  }
  
  return result;
}

void CWinRenderManager::Update(bool bPauseDrawing)
{
  m_bPauseDrawing = bPauseDrawing;
  if (m_pRenderer)
    m_pRenderer->Update(bPauseDrawing);
}

void CWinRenderManager::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (m_pRenderer)
    m_pRenderer->RenderUpdate(clear, flags, alpha);
}

unsigned int CWinRenderManager::PreInit()
{
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
  if (m_pRenderer)
    m_pRenderer->SetupScreenshot();
}

void CWinRenderManager::CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height)
{
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

  if(!m_pRenderer)
    return;

  m_pRenderer->FlipPage(source);
}
