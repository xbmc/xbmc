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
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "MadvrCallback.h"

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

bool CWinDsRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  if (m_sourceWidth != width
    || m_sourceHeight != height)
  {
    m_sourceWidth = width;
    m_sourceHeight = height;
    // need to recreate textures
  }

  m_fps = fps;
  m_iFlags = flags;
  m_flags = flags;
  m_format = format;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);

  ChooseBestResolution(fps);

  SetViewMode(CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
  ManageDisplay();

  m_bConfigured = true;

  return true;
}

void CWinDsRenderer::Reset()
{
}

void CWinDsRenderer::Update()
{
  if (!m_bConfigured) return;
  ManageDisplay();
}

bool CWinDsRenderer::RenderCapture(CRenderCapture* capture)
{
  if (!m_bConfigured)
    return false;

  bool succeeded = false;

  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  CRect saveSize = m_destRect;
  saveRotatedCoords();//backup current m_rotatedDestCoords

  m_destRect.SetRect(0, 0, (float)capture->GetWidth(), (float)capture->GetHeight());
  syncDestRectToRotatedPoints();//syncs the changed destRect to m_rotatedDestCoords

  LPDIRECT3DSURFACE9 oldSurface;
  pD3DDevice->GetRenderTarget(0, &oldSurface);

  capture->BeginRender();
  if (capture->GetState() != CAPTURESTATE_FAILED)
  {
    pD3DDevice->BeginScene();
    Render(0);
    pD3DDevice->EndScene();
    capture->EndRender();
    succeeded = true;
  }

  pD3DDevice->SetRenderTarget(0, oldSurface);
  oldSurface->Release();

  m_destRect = saveSize;
  restoreRotatedCoords();//restores the previous state of the rotated dest coords

  return succeeded;
}


RESOLUTION CWinDsRenderer::ChooseBestMadvrResolution(float fps)
{
  if (fps == 0.0) return (RESOLUTION)0;

  float weight;
  if (!FindResolutionFromOverride(fps, weight, false)) //find a refreshrate from overrides
  {
    if (!FindResolutionFromOverride(fps, weight, true))//if that fails find it from a fallback
      FindResolutionFromFpsMatch(fps, weight);//if that fails use automatic refreshrate selection
  }

  CLog::Log(LOGNOTICE, "Display resolution for madVR ADJUST : %s (%d) (weight: %.3f)",
    g_graphicsContext.GetResInfo(m_resolution).strMode.c_str(), m_resolution, weight);

  return m_resolution;
}

void CWinDsRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (clear)
    g_graphicsContext.Clear(m_clearColour);

  if (alpha < 255)
    pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  else
    pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

  if (!m_bConfigured)
    return;

  CSingleLock lock(g_graphicsContext);

  if (!CMadvrCallback::Get()->UsingMadvr()) 
    ManageDisplay();

  Render(flags);
}

void CWinDsRenderer::Flush()
{
  PreInit();
  SetViewMode(CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
  if (!CMadvrCallback::Get()->UsingMadvr())
    ManageDisplay();

  m_bConfigured = true;
}

unsigned int CWinDsRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;

  UnInit();
  m_resolution = CDisplaySettings::Get().GetCurrentResolution();
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
    || method == RENDERFEATURE_BRIGHTNESS
	|| method == RENDERFEATURE_ZOOM
	|| method == RENDERFEATURE_VERTICAL_SHIFT
	|| method == RENDERFEATURE_PIXEL_RATIO
	|| method == RENDERFEATURE_POSTPROCESS)
    return true;

  return false;
}

#endif
