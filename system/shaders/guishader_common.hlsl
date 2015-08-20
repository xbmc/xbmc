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

struct VS_INPUT
{
  float4 pos : POSITION;
  float4 color: COLOR0;
  float2 tex : TEXCOORD0;
  float2 tex2 : TEXCOORD1;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 color: COLOR0;
  float2 tex : TEXCOORD0;
  float2 tex2 : TEXCOORD1;
};

SamplerState LinearSampler : register(s0)
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = CLAMP;
  AddressV = CLAMP;
  Comparison = NEVER;
};

cbuffer cbWorld : register(b0)
{
  float4x4 worldViewProj;
  float blackLevel;
  float colorRange;
};

inline float4 adjustColorRange(float4 color)
{
  return float4(blackLevel + colorRange * color.rgb, color.a);
}

#define STEREO_LEFT_EYE_INDEX  0
#define STEREO_RIGHT_EYE_INDEX 1

#ifdef STEREO_MODE_SHADER

inline float4 StereoInterlaced(PS_INPUT input, int eye)
{
  uint pixelY = abs(trunc(input.tex.y * g_viewPortHeigh));
  uint odd = pixelY % 2;

  if ((odd == 0 && !eye) || (odd != 0 && eye))
    return float4(texView.Sample(LinearSampler, input.tex).rgb, 1.0);
  else
    return float4(0.0, 0.0, 0.0, 0.0);
}

inline float4 StereoCheckerboard(PS_INPUT input, int eye)
{
  uint pixelX = abs(trunc(input.tex.x * g_viewPortWidth));
  uint pixelY = abs(trunc(input.tex.y * g_viewPortHeigh));
  uint odd = (pixelX + pixelY) % 2;

  if ((odd == 0 && !eye) || (odd != 0 && eye))
    return float4(texView.Sample(LinearSampler, input.tex).rgb, 1.0);
  else
    return float4(0.0, 0.0, 0.0, 0.0);
}

#endif