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

void CRenderContext::SetViewPort(const CRect& viewPort) const {
  m_rendering->SetViewPort(viewPort);
}

void CRenderContext::GetViewPort(CRect& viewPort) const {
  m_rendering->GetViewPort(viewPort);
}

void CRenderContext::SetScissors(const CRect& rect) const {
  m_rendering->SetScissors(rect);
}

void CRenderContext::ApplyStateBlock() const {
  m_rendering->ApplyStateBlock();
}

bool CRenderContext::IsExtSupported(const char* extension) const {
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

void CRenderContext::EnableGUIShader(GL_SHADER_METHOD method) const {
#if defined(HAS_GL)
  CRenderSystemGL* rendering = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (rendering != nullptr)
    rendering->EnableShader(TranslateShaderMethodGL(method));
#elif HAS_GLES >= 2
  auto renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    renderingGLES->EnableGUIShader(TranslateShaderMethodGLES(method));
#endif
}

void CRenderContext::DisableGUIShader() const {
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    renderingGL->DisableShader();
#elif HAS_GLES >= 2
  auto renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    renderingGLES->DisableGUIShader();
#endif
}

int CRenderContext::GUIShaderGetPos() const {
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetPos());
#elif HAS_GLES >= 2
  auto renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetPos());
#endif

  return -1;
}

int CRenderContext::GUIShaderGetCoord0() const {
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetCoord0());
#elif HAS_GLES >= 2
  auto renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetCoord0());
#endif

  return -1;
}

int CRenderContext::GUIShaderGetUniCol() const {
#if defined(HAS_GL)
  CRenderSystemGL* renderingGL = dynamic_cast<CRenderSystemGL*>(m_rendering);
  if (renderingGL != nullptr)
    return static_cast<int>(renderingGL->ShaderGetUniCol());
#elif HAS_GLES >= 2
  auto renderingGLES = dynamic_cast<CRenderSystemGLES*>(m_rendering);
  if (renderingGLES != nullptr)
    return static_cast<int>(renderingGLES->GUIShaderGetUniCol());
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

bool CRenderContext::UseLimitedColor() const {
  return m_windowing->UseLimitedColor();
}

bool CRenderContext::DisplayHardwareScalingEnabled() const {
  return m_windowing->DisplayHardwareScalingEnabled();
}

void CRenderContext::UpdateDisplayHardwareScaling(const RESOLUTION_INFO& resInfo) const {
  return m_windowing->UpdateDisplayHardwareScaling(resInfo);
}

int CRenderContext::GetScreenWidth() const {
  return m_graphicsContext.GetWidth();
}

int CRenderContext::GetScreenHeight() const {
  return m_graphicsContext.GetHeight();
}

const CRect& CRenderContext::GetScissors() const {
  return m_graphicsContext.GetScissors();
}

CRect CRenderContext::GetViewWindow() const {
  return m_graphicsContext.GetViewWindow();
}

void CRenderContext::SetViewWindow(float left, float top, float right, float bottom) const {
  m_graphicsContext.SetViewWindow(left, top, right, bottom);
}

void CRenderContext::SetFullScreenVideo(bool bOnOff) const {
  m_graphicsContext.SetFullScreenVideo(bOnOff);
}

bool CRenderContext::IsFullScreenVideo() const {
  return m_graphicsContext.IsFullScreenVideo();
}

bool CRenderContext::IsCalibrating() const {
  return m_graphicsContext.IsCalibrating();
}

RESOLUTION CRenderContext::GetVideoResolution() const {
  return m_graphicsContext.GetVideoResolution();
}

void CRenderContext::Clear(UTILS::COLOR::Color color) const {
  m_graphicsContext.Clear(color);
}

RESOLUTION_INFO CRenderContext::GetResInfo() const {
  return m_graphicsContext.GetResInfo();
}

void CRenderContext::SetRenderingResolution(const RESOLUTION_INFO& res, bool needsScaling) const {
  m_graphicsContext.SetRenderingResolution(res, needsScaling);
}

UTILS::COLOR::Color CRenderContext::MergeAlpha(UTILS::COLOR::Color color) const {
  return m_graphicsContext.MergeAlpha(color);
}

void CRenderContext::SetTransform(const TransformMatrix& matrix, float scaleX, float scaleY) const {
  m_graphicsContext.SetTransform(matrix, scaleX, scaleY);
}

void CRenderContext::RemoveTransform() const {
  m_graphicsContext.RemoveTransform();
}

CRect CRenderContext::StereoCorrection(const CRect& rect) const {
  return m_graphicsContext.StereoCorrection(rect);
}

CCriticalSection& CRenderContext::GraphicsMutex() const {
  return m_graphicsContext;
}

RESOLUTION_INFO& CRenderContext::GetResolutionInfo(RESOLUTION resolution) const {
  return m_displaySettings.GetResolutionInfo(resolution);
}

::CGameSettings& CRenderContext::GetGameSettings() const {
  return m_mediaSettings.GetCurrentGameSettings();
}

::CGameSettings& CRenderContext::GetDefaultGameSettings() const {
  return m_mediaSettings.GetDefaultGameSettings();
}

void CRenderContext::StartAgentInput(GAME::GameClientPtr gameClient) const {
  m_gameServices.AgentInput().Start(std::move(gameClient));
}

void CRenderContext::StopAgentInput() const {
  m_gameServices.AgentInput().Stop();
}
