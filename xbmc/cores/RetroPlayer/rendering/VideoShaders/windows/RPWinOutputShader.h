/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/VideoShaders/WinVideoFilter.h"
#include "cores/GameSettings.h"

namespace KODI
{
namespace RETRO
{

class CRPWinOutputShader : public CWinShader
{
public:
  ~CRPWinOutputShader() = default;

  bool Create(SCALINGMETHOD scalingMethod);
  void Render(CD3DTexture &sourceTexture, CRect sourceRect, const CPoint points[4]
    , CRect &viewPort, CD3DTexture *target, unsigned range = 0);

private:
  void PrepareParameters(unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, const CPoint points[4]);
  void SetShaderParameters(CD3DTexture& sourceTexture, unsigned range, CRect &viewPort);

  unsigned m_sourceWidth{ 0 };
  unsigned m_sourceHeight{ 0 };
  CRect m_sourceRect{ 0.f, 0.f, 0.f, 0.f };
  CPoint m_destPoints[4] =
  {
    { 0.f, 0.f },
    { 0.f, 0.f },
    { 0.f, 0.f },
    { 0.f, 0.f },
  };

  struct CUSTOMVERTEX {
    FLOAT x;
    FLOAT y;
    FLOAT z;

    FLOAT tu;
    FLOAT tv;
  };
};

} // namespace RETRO
} // namespace KODI
