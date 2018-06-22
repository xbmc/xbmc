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

#pragma once

#include "windowing/Resolution.h"
#include "rendering/RenderSystemTypes.h"
#include "utils/Color.h"
#include "utils/Geometry.h"

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
  TEXTURE_RGBA_OES,
};

namespace KODI
{
namespace RETRO
{
  class CRenderContext
  {
  public:
    CRenderContext(CRenderSystemBase *rendering,
                  CWinSystemBase *windowing,
                  CGraphicContext &graphicsContext,
                  CDisplaySettings &displaySettings,
                  CMediaSettings &mediaSettings);

    CRenderSystemBase *Rendering() { return m_rendering; }
    CWinSystemBase *Windowing() { return m_windowing; }
    CGraphicContext &GraphicsContext() { return m_graphicsContext; }

    // Rendering functions
    void SetViewPort(const CRect& viewPort);
    void GetViewPort(CRect &viewPort);
    void SetScissors(const CRect &rect);
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

    // Graphics functions
    int GetScreenWidth();
    int GetScreenHeight();
    const CRect &GetScissors();
    CRect GetViewWindow();
    void SetViewWindow(float left, float top, float right, float bottom);
    void SetFullScreenVideo(bool bOnOff);
    bool IsFullScreenVideo();
    bool IsCalibrating();
    RESOLUTION GetVideoResolution();
    void Clear(UTILS::Color color = 0);
    RESOLUTION_INFO GetResInfo();
    void SetRenderingResolution(const RESOLUTION_INFO &res, bool needsScaling);
    UTILS::Color MergeAlpha(UTILS::Color color);
    void SetTransform(const TransformMatrix &matrix, float scaleX, float scaleY);
    void RemoveTransform();
    CRect StereoCorrection(const CRect &rect);
    CCriticalSection &GraphicsMutex();

    // Display settings
    RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution);

    // Media settings
    CGameSettings &GetGameSettings();
    CGameSettings &GetDefaultGameSettings();

  private:
    // Construction parameters
    CRenderSystemBase *const m_rendering;
    CWinSystemBase *const m_windowing;
    CGraphicContext &m_graphicsContext;
    CDisplaySettings &m_displaySettings;
    CMediaSettings &m_mediaSettings;
  };
}
}
