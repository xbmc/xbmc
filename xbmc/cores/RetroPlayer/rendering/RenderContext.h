/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ColorUtils.h"
#include "utils/Geometry.h"
#include "windowing/Resolution.h"

class CCriticalSection;
class CDisplaySettings;
class CGameSettings;
class CGraphicContext;
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
namespace RETRO
{
class CRenderContext
{
public:
  CRenderContext(CRenderSystemBase* rendering,
                 CWinSystemBase* windowing,
                 CGraphicContext& graphicsContext,
                 CDisplaySettings& displaySettings,
                 CMediaSettings& mediaSettings);

  CRenderSystemBase* Rendering() { return m_rendering; }
  CWinSystemBase* Windowing() { return m_windowing; }
  CGraphicContext& GraphicsContext() { return m_graphicsContext; }

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
  void Clear(UTILS::COLOR::Color color = 0);
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
  CGameSettings& GetGameSettings();
  CGameSettings& GetDefaultGameSettings();

private:
  // Construction parameters
  CRenderSystemBase* const m_rendering;
  CWinSystemBase* const m_windowing;
  CGraphicContext& m_graphicsContext;
  CDisplaySettings& m_displaySettings;
  CMediaSettings& m_mediaSettings;
};
} // namespace RETRO
} // namespace KODI
