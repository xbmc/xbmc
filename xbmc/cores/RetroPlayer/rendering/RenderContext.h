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

  CRenderSystemBase* Rendering() { return m_rendering; }
  CWinSystemBase* Windowing() { return m_windowing; }
  CGraphicContext& GraphicsContext() { return m_graphicsContext; }
  CGUIComponent* GUI() { return m_guiComponent; }

  // Rendering functions
  void SetViewPort(const CRect& viewPort);
  void GetViewPort(CRect& viewPort);
  void SetScissors(const CRect& rect);
  void ApplyStateBlock();
  bool IsExtSupported(const char* extension);

  // OpenGL(ES) rendering functions
  void EnableGUIShader(GL_SHADER_METHOD method);
  void DisableGUIShader();
  int GUIShaderGetPos();
  int GUIShaderGetCoord0();
  int GUIShaderGetUniCol();
  int GUIShaderGetDepth();

  // DirectX rendering functions
  CGUIShaderDX* GetGUIShader();

  // Windowing functions
  bool UseLimitedColor();
  bool DisplayHardwareScalingEnabled();
  void UpdateDisplayHardwareScaling(const RESOLUTION_INFO& resInfo);

  // Graphics functions
  int GetScreenWidth();
  int GetScreenHeight();
  const CRect& GetScissors();
  CRect GetViewWindow();
  void SetViewWindow(float left, float top, float right, float bottom);
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo();
  bool IsCalibrating();
  RESOLUTION GetVideoResolution();
  void Clear(UTILS::COLOR::Color color);
  RESOLUTION_INFO GetResInfo();
  void SetRenderingResolution(const RESOLUTION_INFO& res, bool needsScaling);
  UTILS::COLOR::Color MergeAlpha(UTILS::COLOR::Color color);
  void SetTransform(const TransformMatrix& matrix, float scaleX, float scaleY);
  void RemoveTransform();
  CRect StereoCorrection(const CRect& rect);
  CCriticalSection& GraphicsMutex();

  // Display settings
  RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution);

  // Media settings
  ::CGameSettings& GetGameSettings();
  ::CGameSettings& GetDefaultGameSettings();

  // Agent functions
  void StartAgentInput(GAME::GameClientPtr gameClient);
  void StopAgentInput();

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
