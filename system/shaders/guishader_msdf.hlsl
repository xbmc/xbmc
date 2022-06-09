/*
 *      Copyright (C) 2005-2015 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guishader_common.hlsl"

Texture2D texMain : register(t0);


inline float median(float r, float g, float b)
{
  return max(min(r, g), min(max(r, g), b));
}

inline float screenPxRange()
{
  return 8.;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
  float4 bgColor = vec4(input.color.rgb, 0.);
  float3 msd = texMain.Sample(LinearSampler, input.tex).rgb;
  float sd = median(msd.r, msd.g, msd.b);
  float screenPxDistance = screenPxRange()*(sd - 0.5);
  float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
  float4 color = lerp(bgColor, input.color, opacity);

  return tonemapHDR(adjustColorRange(color));
}


