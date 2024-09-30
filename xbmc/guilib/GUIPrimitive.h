/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIImage.h"
#include "utils/ColorUtils.h"

class CGUIPrimitive : public CGUIImage
{
public:
  CGUIPrimitive(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIPrimitive(const CGUIPrimitive& left);

  void SetUniform(KODI::GUILIB::GUIINFO::CGUIInfoColor& color);
  void Set2DGradient(std::array<KODI::GUILIB::GUIINFO::CGUIInfoColor, 4>& colors);
  void Set1DGradient(std::array<KODI::GUILIB::GUIINFO::CGUIInfoColor, 4>& colors,
                     uint32_t angle,
                     const std::unique_ptr<Interpolator>& colorInterpolator,
                     const std::unique_ptr<Interpolator>& alphaInterpolator);

private:
  template<unsigned long T>
  void ConvertToTexel(const KODI::UTILS::COLOR::ColorFloats& color,
                      std::array<uint8_t, T>& texelData,
                      unsigned int pixel);

  KODI::UTILS::COLOR::ColorFloats MixColors(const KODI::UTILS::COLOR::ColorFloats& colorA,
                                            const KODI::UTILS::COLOR::ColorFloats& colorB,
                                            float phaseColor,
                                            float phaseAlpha);
  bool IsOpaque(const KODI::UTILS::COLOR::Color color)
  {
    return (color & 0xFF000000) == 0xFF000000;
  }
};
