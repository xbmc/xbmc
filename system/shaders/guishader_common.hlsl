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
  float sdrPeakLum;
  int PQ;
};

inline float3 transferPQ(float3 x)
{
  static const float ST2084_m1 = 2610.0f / (4096.0f * 4.0f);
  static const float ST2084_m2 = (2523.0f / 4096.0f) * 128.0f;
  static const float ST2084_c1 = 3424.0f / 4096.0f;
  static const float ST2084_c2 = (2413.0f / 4096.0f) * 32.0f;
  static const float ST2084_c3 = (2392.0f / 4096.0f) * 32.0f;
  static const float3x3 matx =
  {
    0.627402, 0.329292, 0.043306,
    0.069095, 0.919544, 0.011360,
    0.016394, 0.088028, 0.895578
  };
  // REC.709 to linear
  x = pow(x, 1.0f / 0.45f);
  // REC.709 to BT.2020
  x = mul(matx, x);
  // linear to PQ
  x = pow(x / sdrPeakLum, ST2084_m1);
  x = (ST2084_c1 + ST2084_c2 * x) / (1.0f + ST2084_c3 * x);
  x = pow(x, ST2084_m2);
  return x;
}

inline float4 tonemapHDR(float4 color)
{
  return (PQ) ? float4(transferPQ(color.rgb), color.a) : color;
}

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