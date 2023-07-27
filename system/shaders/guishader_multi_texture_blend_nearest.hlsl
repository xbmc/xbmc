/*
 *  Copyright (C) 2023-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guishader_common.hlsl"

Texture2D txDiffuse[2] : register(t0);

float4 PS(PS_INPUT input) : SV_TARGET
{
  return tonemapHDR(adjustColorRange(input.color * txDiffuse[0].Sample(NearestSampler, input.tex) *
                                     txDiffuse[1].Sample(NearestSampler, input.tex2)));
}
