/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderContext.h"
#include "guilib/GraphicContext.h"
#include "rendering/RenderSystem.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "system_gl.h"

#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#elif HAS_GLES >= 2
#include "rendering/gles/RenderSystemGLES.h"
#endif

using namespace KODI;
using namespace RETRO;

CRenderContext::CRenderContext(CRenderSystemBase *rendering,
                               CWinSystemBase *windowing,
                               CGraphicContext &graphicsContext,
                               CDisplaySettings &displaySettings,
                               CMediaSettings &mediaSettings) :
  m_rendering(rendering),
  m_windowing(windowing),
  m_graphicsContext(graphicsContext),
  m_displaySettings(displaySettings),
  m_mediaSettings(mediaSettings)
{
}

void CRenderContext::SetViewPort(const CRect& viewPort)
{
  m_rendering->SetViewPort(viewPort);
}

void CRenderContext::GetViewPort(CRect &viewPort)
{
  m_rendering->GetViewPort(viewPort);
}

void CRenderContext::SetScissors(const CRect &rect)
{
  m_rendering->SetScissors(rect);
}

void CRenderContext::ApplyStateBlock()
{
  m_rendering->ApplyStateBlock();
}

void CRenderContext::EnableGUIShader()
{
#if defined(HAS_GL)
  CRenderSystemGL *rendering = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (rendering != nullptr)
    rendering->EnableShader(SM_TEXTURE);
#elif HAS_GLES >= 2
  CRenderSystemGLES *renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    renderingGLES->EnableGUIShader(SM_TEXTURE);
#endif
}

void CRenderContext::DisableGUIShader()
{
#if defined(HAS_GL)
  CRenderSystemGL *renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    renderingGL->DisableShader();
#elif HAS_GLES >= 2
  CRenderSystemGLES *renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    renderingGLES->DisableGUIShader();
#endif
}

int CRenderContext::GUIShaderGetPos()
{
#if defined(HAS_GL)
  CRenderSystemGL *renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetPos());
#elif HAS_GLES >= 2
  CRenderSystemGLES *renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetPos());
#endif

  return -1;
}

int CRenderContext::GUIShaderGetCoord0()
{
#if defined(HAS_GL)
  CRenderSystemGL *renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetCoord0());
#elif HAS_GLES >= 2
  CRenderSystemGLES *renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetCoord0());
#endif

  return -1;
}

int CRenderContext::GUIShaderGetUniCol()
{
#if defined(HAS_GL)
  CRenderSystemGL *renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetUniCol());
#elif HAS_GLES >= 2
  CRenderSystemGLES *renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetUniCol());
#endif

  return -1;
}

CGUIShaderDX* CRenderContext::GetGUIShader()
{
#if defined(HAS_DX)
  CRenderSystemDX *renderingDX = dynamic_cast<CRenderSystemDX*>(m_rendering);
  if (renderingDX != nullptr)
    return renderingDX->GetGUIShader();
#endif

  return nullptr;
}

bool CRenderContext::UseLimitedColor()
{
  return m_windowing->UseLimitedColor();
}

int CRenderContext::GetScreenWidth()
{
  return m_graphicsContext.GetWidth();
}

int CRenderContext::GetScreenHeight()
{
  return m_graphicsContext.GetHeight();
}

const CRect &CRenderContext::GetScissors()
{
  return m_graphicsContext.GetScissors();
}

CRect CRenderContext::GetViewWindow()
{
  return m_graphicsContext.GetViewWindow();
}

void CRenderContext::SetViewWindow(float left, float top, float right, float bottom)
{
  m_graphicsContext.SetViewWindow(left, top, right, bottom);
}

void CRenderContext::SetFullScreenVideo(bool bOnOff)
{
  m_graphicsContext.SetFullScreenVideo(bOnOff);
}

bool CRenderContext::IsFullScreenVideo()
{
  return m_graphicsContext.IsFullScreenVideo();
}

bool CRenderContext::IsCalibrating()
{
  return m_graphicsContext.IsCalibrating();
}

RESOLUTION CRenderContext::GetVideoResolution()
{
  return m_graphicsContext.GetVideoResolution();
}

void CRenderContext::Clear(color_t color /* = 0 */)
{
  m_graphicsContext.Clear(color);
}

RESOLUTION_INFO CRenderContext::GetResInfo()
{
  return m_graphicsContext.GetResInfo();
}

void CRenderContext::SetRenderingResolution(const RESOLUTION_INFO &res, bool needsScaling)
{
  m_graphicsContext.SetRenderingResolution(res, needsScaling);
}

color_t CRenderContext::MergeAlpha(color_t color)
{
  return m_graphicsContext.MergeAlpha(color);
}

void CRenderContext::SetTransform(const TransformMatrix &matrix, float scaleX, float scaleY)
{
  m_graphicsContext.SetTransform(matrix, scaleX, scaleY);
}

void CRenderContext::RemoveTransform()
{
  m_graphicsContext.RemoveTransform();
}

CRect CRenderContext::StereoCorrection(const CRect &rect)
{
  return m_graphicsContext.StereoCorrection(rect);
}

CCriticalSection &CRenderContext::GraphicsMutex()
{
  return m_graphicsContext;
}

RESOLUTION_INFO &CRenderContext::GetResolutionInfo(RESOLUTION resolution)
{
  return m_displaySettings.GetResolutionInfo(resolution);
}

CGameSettings &CRenderContext::GetGameSettings()
{
  return m_mediaSettings.GetCurrentGameSettings();
}

CGameSettings &CRenderContext::GetDefaultGameSettings()
{
  return m_mediaSettings.GetDefaultGameSettings();
}
