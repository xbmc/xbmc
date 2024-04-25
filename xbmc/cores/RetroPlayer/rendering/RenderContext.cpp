/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderContext.h"

#include "games/GameServices.h"
#include "games/agents/input/AgentInput.h"
#include "rendering/RenderSystem.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include "system_gl.h"

#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#elif HAS_GLES >= 2
#include "rendering/gles/RenderSystemGLES.h"
#elif defined(TARGET_WINDOWS)
#include "rendering/dx/RenderSystemDX.h"
#endif

using namespace KODI;
using namespace RETRO;

CRenderContext::CRenderContext(CRenderSystemBase* rendering,
                               CWinSystemBase* windowing,
                               CGraphicContext& graphicsContext,
                               CDisplaySettings& displaySettings,
                               CMediaSettings& mediaSettings,
                               GAME::CGameServices& gameServices,
                               CGUIComponent* guiComponent)
  : m_rendering(rendering),
    m_windowing(windowing),
    m_graphicsContext(graphicsContext),
    m_displaySettings(displaySettings),
    m_mediaSettings(mediaSettings),
    m_gameServices(gameServices),
    m_guiComponent(guiComponent)
{
}

void CRenderContext::SetViewPort(const CRect& viewPort)
{
  m_rendering->SetViewPort(viewPort);
}

void CRenderContext::GetViewPort(CRect& viewPort)
{
  m_rendering->GetViewPort(viewPort);
}

void CRenderContext::SetScissors(const CRect& rect)
{
  m_rendering->SetScissors(rect);
}

void CRenderContext::ApplyStateBlock()
{
  m_rendering->ApplyStateBlock();
}

bool CRenderContext::IsExtSupported(const char* extension)
{
  return m_rendering->IsExtSupported(extension);
}

#if defined(HAS_GL) || defined(HAS_GLES)
namespace
{

#ifdef HAS_GL
static ShaderMethodGL TranslateShaderMethodGL(GL_SHADER_METHOD method)
{
  switch (method)
  {
    case GL_SHADER_METHOD::DEFAULT:
      return ShaderMethodGL::SM_DEFAULT;
    case GL_SHADER_METHOD::TEXTURE:
      return ShaderMethodGL::SM_TEXTURE;
    default:
      break;
  }

  return ShaderMethodGL::SM_DEFAULT;
}
#endif
#ifdef HAS_GLES
static ShaderMethodGLES TranslateShaderMethodGLES(GL_SHADER_METHOD method)
{
  switch (method)
  {
    case GL_SHADER_METHOD::DEFAULT:
      return ShaderMethodGLES::SM_DEFAULT;
    case GL_SHADER_METHOD::TEXTURE:
      return ShaderMethodGLES::SM_TEXTURE;
    case GL_SHADER_METHOD::TEXTURE_NOALPHA:
      return ShaderMethodGLES::SM_TEXTURE_NOALPHA;
    default:
      break;
  }

  return ShaderMethodGLES::SM_DEFAULT;
}
#endif

} // namespace
#endif

void CRenderContext::EnableGUIShader(GL_SHADER_METHOD method)
{
#if defined(HAS_GL)
  CRenderSystemGL* rendering = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (rendering != nullptr)
    rendering->EnableShader(TranslateShaderMethodGL(method));
#elif HAS_GLES >= 2
  CRenderSystemGLES* renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    renderingGLES->EnableGUIShader(TranslateShaderMethodGLES(method));
#endif
}

void CRenderContext::DisableGUIShader()
{
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    renderingGL->DisableShader();
#elif HAS_GLES >= 2
  CRenderSystemGLES* renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    renderingGLES->DisableGUIShader();
#endif
}

int CRenderContext::GUIShaderGetPos()
{
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetPos());
#elif HAS_GLES >= 2
  CRenderSystemGLES* renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetPos());
#endif

  return -1;
}

int CRenderContext::GUIShaderGetCoord0()
{
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetCoord0());
#elif HAS_GLES >= 2
  CRenderSystemGLES* renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetCoord0());
#endif

  return -1;
}

int CRenderContext::GUIShaderGetUniCol()
{
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetUniCol());
#elif HAS_GLES >= 2
  CRenderSystemGLES* renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetUniCol());
#endif

  return -1;
}

int CRenderContext::GUIShaderGetDepth()
{
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetDepth());
#elif HAS_GLES >= 2
  CRenderSystemGLES* renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetDepth());
#endif

  return -1;
}

CGUIShaderDX* CRenderContext::GetGUIShader()
{
#if defined(HAS_DX)
  CRenderSystemDX* renderingDX = dynamic_cast<CRenderSystemDX*>(m_rendering);
  if (renderingDX != nullptr)
    return renderingDX->GetGUIShader();
#endif

  return nullptr;
}

bool CRenderContext::UseLimitedColor()
{
  return m_windowing->UseLimitedColor();
}

bool CRenderContext::DisplayHardwareScalingEnabled()
{
  return m_windowing->DisplayHardwareScalingEnabled();
}

void CRenderContext::UpdateDisplayHardwareScaling(const RESOLUTION_INFO& resInfo)
{
  return m_windowing->UpdateDisplayHardwareScaling(resInfo);
}

int CRenderContext::GetScreenWidth()
{
  return m_graphicsContext.GetWidth();
}

int CRenderContext::GetScreenHeight()
{
  return m_graphicsContext.GetHeight();
}

const CRect& CRenderContext::GetScissors()
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

void CRenderContext::Clear(UTILS::COLOR::Color color)
{
  m_graphicsContext.Clear(color);
}

RESOLUTION_INFO CRenderContext::GetResInfo()
{
  return m_graphicsContext.GetResInfo();
}

void CRenderContext::SetRenderingResolution(const RESOLUTION_INFO& res, bool needsScaling)
{
  m_graphicsContext.SetRenderingResolution(res, needsScaling);
}

UTILS::COLOR::Color CRenderContext::MergeAlpha(UTILS::COLOR::Color color)
{
  return m_graphicsContext.MergeAlpha(color);
}

void CRenderContext::SetTransform(const TransformMatrix& matrix, float scaleX, float scaleY)
{
  m_graphicsContext.SetTransform(matrix, scaleX, scaleY);
}

void CRenderContext::RemoveTransform()
{
  m_graphicsContext.RemoveTransform();
}

CRect CRenderContext::StereoCorrection(const CRect& rect)
{
  return m_graphicsContext.StereoCorrection(rect);
}

CCriticalSection& CRenderContext::GraphicsMutex()
{
  return m_graphicsContext;
}

RESOLUTION_INFO& CRenderContext::GetResolutionInfo(RESOLUTION resolution)
{
  return m_displaySettings.GetResolutionInfo(resolution);
}

::CGameSettings& CRenderContext::GetGameSettings()
{
  return m_mediaSettings.GetCurrentGameSettings();
}

::CGameSettings& CRenderContext::GetDefaultGameSettings()
{
  return m_mediaSettings.GetDefaultGameSettings();
}

void CRenderContext::StartAgentInput(GAME::GameClientPtr gameClient)
{
  m_gameServices.AgentInput().Start(std::move(gameClient));
}

void CRenderContext::StopAgentInput()
{
  m_gameServices.AgentInput().Stop();
}
