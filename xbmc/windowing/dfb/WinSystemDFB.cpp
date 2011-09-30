/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#ifdef HAS_DIRECTFB

#include "WinSystemDFB.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"

#include <vector>
#include <directfb/directfb.h>
#include <directfb/directfbgl2.h>

CWinSystemDFB::CWinSystemDFB() : CWinSystemBase()
{
  m_dfb           = NULL;
  m_dfb_layer     = NULL;
  m_dfb_surface   = NULL;
  m_flipflags     = DFBSurfaceFlipFlags(DSFLIP_BLIT);
  m_buffermode    = DSCAPS_TRIPLE;
  m_eWindowSystem = WINDOW_SYSTEM_DFB;
}

CWinSystemDFB::~CWinSystemDFB()
{
  DestroyWindowSystem();
}

bool CWinSystemDFB::InitWindowSystem()
{
  CLog::Log(LOGINFO, "CWinSystemDFB: Creating DirectFB WindowSystem");
  int ret;
  DFBSurfaceDescription dsc;

  DirectFBInit(NULL, NULL);
  DirectFBCreate(&m_dfb);

  ret = m_dfb->GetInterface(m_dfb, "IDirectFBGL2", NULL, m_dfb, (void**) &m_gl2);
  if (ret)
  {
    CLog::Log(LOGERROR, "CWinSystemDFB: Unable to get IDirectFBGL2 interface");
    return ret;
  }

  m_dfb->SetCooperativeLevel(m_dfb, DFSCL_FULLSCREEN );

  dsc.flags = DSDESC_CAPS;
  dsc.caps  = (DFBSurfaceCapabilities)(DSCAPS_PRIMARY | m_buffermode);

  m_dfb->CreateSurface( m_dfb, &dsc, &m_dfb_surface );

  m_dfb_surface->Clear(m_dfb_surface, 0, 0, 0, 0);
  m_dfb_surface->Flip(m_dfb_surface, NULL, DFBSurfaceFlipFlags(m_flipflags));

  ret = m_gl2->CreateContext( m_gl2, NULL, &m_gl2context );
  if (ret)
  {
    CLog::Log(LOGERROR, "CWinSystemDFB: Unable to create DirectFBGL2 context");
  }

  ret = m_gl2context->Bind( m_gl2context, m_dfb_surface, m_dfb_surface );
  if (ret)
  {
    CLog::Log(LOGERROR, "CWinSystemDFB: Unable to bind DirectFBGL2 context");
  }

  if (!CWinSystemBase::InitWindowSystem())
    return false;

  return true;
}

bool CWinSystemDFB::DestroyWindowSystem()
{
  if (m_gl2context)
  {
    m_gl2context->Unbind(m_gl2context);
    m_gl2context->Release(m_gl2context);
  }
  m_gl2context = NULL;

  if (m_gl2)
    m_gl2->Release(m_gl2);
  m_gl2 = NULL;

  if (m_dfb_surface)
    m_dfb_surface->Release(m_dfb_surface);
  m_dfb_surface = NULL;

  if (m_dfb_layer)
    m_dfb_layer->Release(m_dfb_layer);
  m_dfb_layer  = NULL;

  if (m_dfb)
    m_dfb->Release(m_dfb);
  m_dfb = NULL;

  return true;
}

bool CWinSystemDFB::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  m_bWindowCreated = true;
  return true;
}

bool CWinSystemDFB::DestroyWindow()
{
  m_bWindowCreated = false;

  return true;
}

bool CWinSystemDFB::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, true, 0);
  return true;
}

bool CWinSystemDFB::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CLog::Log(LOGDEBUG, "CWinSystemDFB::SetFullScreen");
  m_nWidth  = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  CreateNewWindow("", fullScreen, res, NULL);

  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, true, 0);

  return true;
}

void CWinSystemDFB::UpdateResolutions()
{
  int width = 0;
  int height = 0;
  CWinSystemBase::UpdateResolutions();

  m_dfb_surface->GetSize(m_dfb_surface, &width, &height);
  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, width, height, 0.0);
}

bool CWinSystemDFB::PresentRenderImpl(const CDirtyRegionList &dirty)
{
  int ret;
  m_gl2context->Unbind(m_gl2context);
  m_dfb_surface->Flip(m_dfb_surface, NULL, (m_flipflags));
  ret = m_gl2context->Bind(m_gl2context, m_dfb_surface, m_dfb_surface);
  if (ret)
  {
    return false;
  }

  return true;
}

void CWinSystemDFB::SetVSyncImpl(bool enable)
{
  m_flipflags = enable ? (DFBSurfaceFlipFlags)(m_flipflags | DSFLIP_ONSYNC) : (DFBSurfaceFlipFlags)(m_flipflags & ~DSFLIP_ONSYNC);
}

#endif
