/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "ApplicationRenderer.h"
#include "utils/GUIInfoManager.h"
#include "../guilib/guiImage.h"

CApplicationRenderer g_ApplicationRenderer;

CApplicationRenderer::CApplicationRenderer(void)
{
}

CApplicationRenderer::~CApplicationRenderer()
{
  Stop();
}

void CApplicationRenderer::OnStartup()
{
  m_time = timeGetTime();
  m_enabled = true;
  m_busyShown = false;
  m_explicitbusy = 0;
  m_busycount = 0;
  m_prevbusycount = 0;
#ifndef HAS_SDL
  m_lpSurface = NULL;
#endif
  m_pWindow = NULL;
}

void CApplicationRenderer::OnExit()
{
  m_busycount = m_prevbusycount = m_explicitbusy = 0;
  m_busyShown = false;
  if (m_pWindow) m_pWindow->Close(true);
  m_pWindow = NULL;
#ifndef HAS_SDL
  SAFE_RELEASE(m_lpSurface);
#endif
}

void CApplicationRenderer::Process()
{
#ifndef HAS_SDL
  int iWidth = 0;
  int iHeight = 0;
  int iLeft = 0;
  int iTop = 0;
  LPDIRECT3DSURFACE8 lpSurfaceBack = NULL;
  LPDIRECT3DSURFACE8 lpSurfaceFront = NULL;
  while (!m_bStop)
  {
    if (!m_enabled || g_graphicsContext.IsFullScreenVideo())
    {
      Sleep(50);
      continue;
    }

    if (!m_pWindow || iWidth == 0 || iHeight == 0)
    {
      m_pWindow = (CGUIDialogBusy*)m_gWindowManager.GetWindow(WINDOW_DIALOG_BUSY);
      if (m_pWindow)
      {
        m_pWindow->Initialize();//need to load the window to determine size.
        if (m_pWindow->GetID() == WINDOW_INVALID)
        {
          //busywindow couldn't be loaded so stop this thread.
          m_pWindow = NULL;
          m_bStop = true;
          break;
        }

        SAFE_RELEASE(m_lpSurface);
        FRECT rect = m_pWindow->GetScaledBounds();
        m_pWindow->ClearAll(); //unload

        iLeft = (int)floor(rect.left);
        iTop =  (int)floor(rect.top);
        iWidth = (int)ceil(rect.right - rect.left);
        iHeight = (int)ceil(rect.bottom - rect.top);
      }
    }

    float t0 = (1000.0f/(float)g_graphicsContext.GetFPS());
    float t1 = m_time + t0; //time when we expect a new render
    float t2 = (float)timeGetTime();
    if (t1 < t2) //we're late rendering
    {
      try
      {
        if (timeGetTime() >= (m_time + g_advancedSettings.m_busyDialogDelay))
        {
          CSingleLock lockg (g_graphicsContext);
          if (m_prevbusycount != m_busycount)
          {
            Sleep(1);
            continue;
          }
          if (!m_pWindow || iWidth == 0 || iHeight == 0)
          {
            Sleep(1000);
            continue;
          }
          if (m_busycount > 0) m_busycount--;
          //no busy indicator if a progress dialog is showing
          if (m_gWindowManager.HasModalDialog() || (m_gWindowManager.GetTopMostModalDialogID() == WINDOW_DIALOG_PROGRESS))
          {
            //TODO: render progress dialog here instead of in dialog::Progress
            m_time = timeGetTime();
            Sleep(1);
            continue;
          }
          if (m_lpSurface == NULL)
          {
            D3DSURFACE_DESC desc;
#ifdef HAS_XBOX_D3D
            if (SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( -1, D3DBACKBUFFER_TYPE_MONO, &lpSurfaceFront)))
            {
              lpSurfaceFront->GetDesc( &desc );
            }
            else
#else
            g_application.RenderNoPresent();
            HRESULT result = g_graphicsContext.Get3DDevice()->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &lpSurfaceFront);
            if (SUCCEEDED(result))
            {
              lpSurfaceFront->GetDesc( &desc );
              iLeft = 0;
              iTop = 0;
              iWidth = desc.Width;
              iHeight = desc.Height;
            }
            else
#endif
            {
              Sleep(1000);
              continue;
            }
            if (!SUCCEEDED(g_graphicsContext.Get3DDevice()->CreateImageSurface(iWidth, iHeight, desc.Format, &m_lpSurface)))
            {
              SAFE_RELEASE(lpSurfaceFront);
              Sleep(1000);
              continue;
            }
            //copy part underneeth busy dialog
            const RECT rc = { iLeft, iTop, iLeft + iWidth, iTop + iHeight  };
            const POINT ptDest = { 0, 0 };
            if (!SUCCEEDED(g_graphicsContext.Get3DDevice()->CopyRects(lpSurfaceFront, &rc, 1, m_lpSurface, &ptDest)))
            {
                SAFE_RELEASE(lpSurfaceFront);
                SAFE_RELEASE(m_lpSurface);
                Sleep(1000);
                continue;
            }

            //copy front buffer to backbuffer(s) to avoid jumping
            for (int i = 0; i < g_graphicsContext.GetBackbufferCount(); i++)
            {
              if (!SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( i, D3DBACKBUFFER_TYPE_MONO, &lpSurfaceBack)))
              {
                  SAFE_RELEASE(lpSurfaceFront);
                  SAFE_RELEASE(m_lpSurface);
                  Sleep(1000);
                  continue;
              }
              if (!SUCCEEDED(g_graphicsContext.Get3DDevice()->CopyRects(lpSurfaceFront, NULL, 0, lpSurfaceBack, NULL)))
              {
                  SAFE_RELEASE(lpSurfaceFront);
                  SAFE_RELEASE(lpSurfaceBack);
                  SAFE_RELEASE(m_lpSurface);
                  Sleep(1000);
                  continue;
              }
              SAFE_RELEASE(lpSurfaceBack);
            }
            SAFE_RELEASE(lpSurfaceFront);
          }
          if (!SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &lpSurfaceBack)))
          {
              Sleep(1000);
              continue;
          }
          g_graphicsContext.Get3DDevice()->BeginScene();
          //copy dialog background to backbuffer
          const RECT rc = { 0, 0, iWidth, iHeight };
          const POINT ptDest = { iLeft, iTop };
          const D3DRECT rc2 = { iLeft, iTop, iLeft + iWidth, iTop + iHeight };
          g_graphicsContext.Get3DDevice()->Clear(1, &rc2, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L);
          if (!SUCCEEDED(g_graphicsContext.Get3DDevice()->CopyRects(m_lpSurface, &rc, 1, lpSurfaceBack, &ptDest)))
          {
              SAFE_RELEASE(lpSurfaceBack);
              g_graphicsContext.Get3DDevice()->EndScene();
              Sleep(1000);
              continue;
          }
          SAFE_RELEASE(lpSurfaceBack);
          if (!m_busyShown)
          {
            m_pWindow->Show();
            m_busyShown = true;
          }
          m_pWindow->Render();

          g_graphicsContext.Get3DDevice()->EndScene();
          //D3DSWAPEFFECT_DISCARD is used so we can't just present the busy rect but can only present the entire screen.
          g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
        }
        m_busycount++;
        m_prevbusycount = m_busycount;
      }
      catch (...)
      {
        SAFE_RELEASE(lpSurfaceFront);
        SAFE_RELEASE(lpSurfaceBack);
        SAFE_RELEASE(m_lpSurface);
        g_graphicsContext.Unlock();
        CLog::Log(LOGERROR, __FUNCTION__" - Exception caught when  busy rendering");
      }
    }
    Sleep(1);
  }
#endif
}

void CApplicationRenderer::UpdateBusyCount()
{
  if (m_busycount == 0)
  {
    m_prevbusycount = 0;
  }
  else
  {
    m_busycount--;
    m_prevbusycount = m_busycount;
    if (m_pWindow && m_busyShown)
    {
      m_busyShown = false;
      m_pWindow->Close();
    }
  }
}

void CApplicationRenderer::Render(bool bFullscreen)
{
  CSingleLock lockg (g_graphicsContext);
  Disable();
  UpdateBusyCount();
#ifndef HAS_SDL
  SAFE_RELEASE(m_lpSurface);
#endif
  if (bFullscreen)
  {
    g_application.DoRenderFullScreen();
  }
  else
  {
    g_application.DoRender();
  }
  m_time = timeGetTime();
  Enable();
}

void CApplicationRenderer::Enable()
{
  m_enabled = true;
}

void CApplicationRenderer::Disable()
{
  m_enabled = false;
}

bool CApplicationRenderer::Start()
{
  Create();
  return true;
}

void CApplicationRenderer::Stop()
{
  StopThread();
}

bool CApplicationRenderer::IsBusy() const
{
  return ((m_explicitbusy > 0) || m_busyShown);
}

void CApplicationRenderer::SetBusy(bool bBusy)
{
  bBusy?m_explicitbusy++:m_explicitbusy--;
  if (m_explicitbusy < 0) m_explicitbusy = 0;
  if (m_pWindow) 
  {
    if (m_explicitbusy > 0)
      m_pWindow->Show();
    else 
      m_pWindow->Close();
  }
};
