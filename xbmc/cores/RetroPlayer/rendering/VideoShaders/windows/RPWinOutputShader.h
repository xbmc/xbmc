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

#include "cores/VideoPlayer/VideoRenderers/VideoShaders/WinVideoFilter.h"

namespace KODI
{
namespace RETRO
{

class CRPWinOutputShader : public CWinShader
{
public:
  ~CRPWinOutputShader() = default;

  bool Create(ESCALINGMETHOD scalingMethod);
  void Render(CD3DTexture &sourceTexture, unsigned sourceWidth, unsigned sourceHeight, CRect sourceRect, const CPoint points[4]
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
