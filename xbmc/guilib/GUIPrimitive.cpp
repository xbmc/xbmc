/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIPrimitive.h"

#include "Interpolators.h"

#include <cassert>

using namespace KODI::GUILIB;

CGUIPrimitive::CGUIPrimitive(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIImage(parentID, controlID, posX, posY, width, height, CTextureInfo())
{
  ControlType = GUICONTROL_PRIMITIVE;
}

void CGUIPrimitive::SetUniform(GUIINFO::CGUIInfoColor& color)
{
  std::array<uint8_t, 4> texelData;

  ConvertToTexel(KODI::UTILS::COLOR::ConvertToFloats(color), texelData, 0);

  KD_TEX_ALPHA alpha = IsOpaque(color) ? KD_TEX_ALPHA_OPAQUE : KD_TEX_ALPHA_STRAIGHT;

  m_texture->SetTexture(1, 1, texelData.data(), KD_TEX_FMT_SDR_RGBA8, alpha);
  m_texture->SetAspectRatio(CAspectRatio::AR_STRETCH);
}

void CGUIPrimitive::Set1DGradient(std::array<KODI::GUILIB::GUIINFO::CGUIInfoColor, 4>& colors,
                                  uint32_t angle,
                                  const std::unique_ptr<Interpolator>& colorInterpolator,
                                  const std::unique_ptr<Interpolator>& alphaInterpolator)
{
  const unsigned int steps =
      std::max(colorInterpolator->GetMinSteps(), alphaInterpolator->GetMinSteps());

  assert(steps <= 32);

  std::array<uint8_t, 128> texelData;

  const KODI::UTILS::COLOR::ColorFloats colorA = KODI::UTILS::COLOR::ConvertToFloats(colors[0]);
  const KODI::UTILS::COLOR::ColorFloats colorB = KODI::UTILS::COLOR::ConvertToFloats(colors[1]);

  for (unsigned int step = 0; step < steps; ++step)
  {
    const float phase = (float)step / (steps - 1.f);
    const float phaseColor = colorInterpolator->Interpolate(phase);
    const float phaseAlpha = alphaInterpolator->Interpolate(phase);
    const KODI::UTILS::COLOR::ColorFloats color = MixColors(colorA, colorB, phaseColor, phaseAlpha);
    ConvertToTexel(color, texelData, step);
  }

  bool opaque = IsOpaque(colors[0] & colors[1]);
  KD_TEX_ALPHA alpha = opaque ? KD_TEX_ALPHA_OPAQUE : KD_TEX_ALPHA_STRAIGHT;

  // real angles require vertex manipulation.
  if (angle < 45)
    m_texture->SetTexture(steps, 1, texelData.data(), KD_TEX_FMT_SDR_RGBA8, alpha);
  else
    m_texture->SetTexture(1, steps, texelData.data(), KD_TEX_FMT_SDR_RGBA8, alpha);

  m_texture->SetAspectRatio(CAspectRatio::AR_STRETCH_TEXEL_EDGE);
}

void CGUIPrimitive::Set2DGradient(std::array<KODI::GUILIB::GUIINFO::CGUIInfoColor, 4>& colors)
{
  std::array<uint8_t, 16> texelData;

  for (unsigned int i = 0; i < 4; ++i)
    ConvertToTexel(KODI::UTILS::COLOR::ConvertToFloats(colors[i]), texelData, i);

  bool opaque = IsOpaque(colors[0] & colors[1] & colors[2] & colors[3]);
  KD_TEX_ALPHA alpha = opaque ? KD_TEX_ALPHA_OPAQUE : KD_TEX_ALPHA_STRAIGHT;

  m_texture->SetTexture(2, 2, texelData.data(), KD_TEX_FMT_SDR_RGBA8, alpha);
  m_texture->SetAspectRatio(CAspectRatio::AR_STRETCH_TEXEL_EDGE);
}

template<unsigned long T>
void CGUIPrimitive::ConvertToTexel(const KODI::UTILS::COLOR::ColorFloats& color,
                                   std::array<uint8_t, T>& texelData,
                                   unsigned int texel)
{
  texelData[texel * 4] = (uint8_t)(color.red * 255.f);
  texelData[texel * 4 + 1] = (uint8_t)(color.green * 255.f);
  texelData[texel * 4 + 2] = (uint8_t)(color.blue * 255.f);
  texelData[texel * 4 + 3] = (uint8_t)(color.alpha * 255.f);
}

KODI::UTILS::COLOR::ColorFloats CGUIPrimitive::MixColors(
    const KODI::UTILS::COLOR::ColorFloats& colorA,
    const KODI::UTILS::COLOR::ColorFloats& colorB,
    float phaseColor,
    float phaseAlpha)
{
  phaseColor = std::clamp(phaseColor, 0.0f, 1.0f);
  phaseAlpha = std::clamp(phaseAlpha, 0.0f, 1.0f);

  KODI::UTILS::COLOR::ColorFloats color;

  color.red = colorA.red * (1.0f - phaseColor) + colorB.red * phaseColor;
  color.green = colorA.green * (1.0f - phaseColor) + colorB.green * phaseColor;
  color.blue = colorA.blue * (1.0f - phaseColor) + colorB.blue * phaseColor;
  color.alpha = colorA.alpha * (1.0f - phaseAlpha) + colorB.alpha * phaseAlpha;
  return color;
}
