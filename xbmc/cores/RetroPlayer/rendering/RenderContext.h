/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "utils/ColorUtils.h"
#include "utils/Geometry.h"
#include "windowing/Resolution.h"

class CCriticalSection;
class CDisplaySettings;
class CGameSettings;
class CGraphicContext;
class CGUIComponent;
class CGUIShaderDX;
class CMediaSettings;
class CRenderSystemBase;
class CWinSystemBase;
class TransformMatrix;

enum class GL_SHADER_METHOD
{
  DEFAULT,
  TEXTURE,
  TEXTURE_NOALPHA,
};

namespace KODI
{
namespace GAME
{
class CGameServices;
}

namespace RETRO
{
class CRenderContext
{
public:
  CRenderContext(CRenderSystemBase* rendering,
                 CWinSystemBase* windowing,
                 CGraphicContext& graphicsContext,
                 CDisplaySettings& displaySettings,
                 CMediaSettings& mediaSettings,
                 GAME::CGameServices& gameServices,
                 CGUIComponent* guiComponent);

  CRenderSystemBase* Rendering() const { return m_rendering; }
  CWinSystemBase* Windowing() const { return m_windowing; }
  CGraphicContext& GraphicsContext() const { return m_graphicsContext; }
  CGUIComponent* GUI() const { return m_guiComponent; }

  // Rendering functions
  void SetViewPort(const CRect& viewPort) const;
  void GetViewPort(CRect& viewPort) const;
  void SetScissors(const CRect& rect) const;
  void ApplyStateBlock() const;
  bool IsExtSupported(const char* extension) const;

  // OpenGL(ES) rendering functions
  void EnableGUIShader(GL_SHADER_METHOD method) const;
  void DisableGUIShader() const;
  int GUIShaderGetPos() const;
  int GUIShaderGetCoord0() const;
  int GUIShaderGetUniCol() const;

  // DirectX rendering functions
  CGUIShaderDX* GetGUIShader();

  // Windowing functions
  bool UseLimitedColor() const;
  bool DisplayHardwareScalingEnabled() const;
  void UpdateDisplayHardwareScaling(const RESOLUTION_INFO& resInfo) const;

  // Graphics functions
  int GetScreenWidth() const;
  int GetScreenHeight() const;
  const CRect& GetScissors() const;
  CRect GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom) const;
  void SetFullScreenVideo(bool bOnOff) const;
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  RESOLUTION GetVideoResolution() const;
  void Clear(UTILS::COLOR::Color color) const;
  RESOLUTION_INFO GetResInfo() const;
  void SetRenderingResolution(const RESOLUTION_INFO& res, bool needsScaling) const;
  UTILS::COLOR::Color MergeAlpha(UTILS::COLOR::Color color) const;
  void SetTransform(const TransformMatrix& matrix, float scaleX, float scaleY) const;
  void RemoveTransform() const;
  CRect StereoCorrection(const CRect& rect) const;
  CCriticalSection& GraphicsMutex() const;

  // Display settings
  RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution) const;

  // Media settings
  ::CGameSettings& GetGameSettings() const;
  ::CGameSettings& GetDefaultGameSettings() const;

  // Agent functions
  void StartAgentInput(GAME::GameClientPtr gameClient) const;
  void StopAgentInput() const;

private:
  // Construction parameters
  CRenderSystemBase* const m_rendering;
  CWinSystemBase* const m_windowing;
  CGraphicContext& m_graphicsContext;
  CDisplaySettings& m_displaySettings;
  CMediaSettings& m_mediaSettings;
  GAME::CGameServices& m_gameServices;
  CGUIComponent* const m_guiComponent;
};
} // namespace RETRO
} // namespace KODI
