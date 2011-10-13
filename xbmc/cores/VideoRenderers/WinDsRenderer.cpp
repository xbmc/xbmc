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
 
#ifdef HAS_DS_PLAYER

#include "WinDsRenderer.h"
#include "Util.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "utils/MathUtils.h"
#include "DSUtil/SmartPtr.h"
#include "StreamsManager.h"
#include "Filters\DX9AllocatorPresenter.h"
#include "IPaintCallback.h"
#include "settings/GUISettings.h"

CWinDsRenderer::CWinDsRenderer():
  m_bConfigured(false), m_paintCallback(NULL)
{
}

CWinDsRenderer::~CWinDsRenderer()
{
  UnInit();
}

void CWinDsRenderer::SetupScreenshot()
{
  // When taking a screenshot, the CDX9AllocatorPreenter::Paint() method is called, but never CDX9AllocatorPresenter::OnAfterPresent().
  // The D3D device is always locked. Setting bPaintAll to false fixes that.
  CDX9AllocatorPresenter::bPaintAll = false;
}

bool CWinDsRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, unsigned int format)
{
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_flags = flags;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();

  m_bConfigured = true;
  return true;
}

void CWinDsRenderer::Reset()
{
}

void CWinDsRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
}

void CWinDsRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured) return;
  
  CSingleLock lock(g_graphicsContext);

  ManageDisplay();
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  if (clear)
    pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  else
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

  Render(flags);
}

unsigned int CWinDsRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;

  UnInit();
  m_resolution = g_guiSettings.m_LookAndFeelResolution;
  if ( m_resolution == RES_WINDOW )
    m_resolution = RES_DESKTOP;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;
  return 0;
}


void CWinDsRenderer::UnInit()
{
  m_bConfigured = false;
}

void CWinDsRenderer::Render(DWORD flags)
{
  // TODO: Take flags into account
  /*if( flags & RENDER_FLAG_NOOSD ) 
    return;*/

  CSingleLock lock(g_graphicsContext);

  if (m_paintCallback)
    m_paintCallback->OnPaint(m_destRect);
}

void CWinDsRenderer::AutoCrop(bool bCrop)
{
}

void CWinDsRenderer::RegisterCallback(IPaintCallback *callback)
{
  m_paintCallback = callback;
}

void CWinDsRenderer::UnregisterCallback()
{
  m_paintCallback = NULL;
}

inline void CWinDsRenderer::OnAfterPresent()
{
  if (m_paintCallback)
    m_paintCallback->OnAfterPresent();
}

EINTERLACEMETHOD CWinDsRenderer::AutoInterlaceMethod()
{
    return VS_INTERLACEMETHOD_NONE;
}

bool CWinDsRenderer::Supports(EDEINTERLACEMODE mode)
{
  if(mode == VS_DEINTERLACEMODE_OFF
  || mode == VS_DEINTERLACEMODE_AUTO
  || mode == VS_DEINTERLACEMODE_FORCE)
    return true;

  return false;
}

bool CWinDsRenderer::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_NONE
  || method == VS_INTERLACEMETHOD_AUTO
  || method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;

  return false;
}

bool CWinDsRenderer::Supports(ESCALINGMETHOD method)
{
  if(method == VS_SCALINGMETHOD_NEAREST
  || method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

bool CWinDsRenderer::Supports( ERENDERFEATURE method )
{
  if ( method == RENDERFEATURE_CONTRAST
    || method == RENDERFEATURE_BRIGHTNESS)
    return true;

  return false;
}

#endif
