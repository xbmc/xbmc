/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

struct VS_INPUT
{
  float4 pos : POSITION; // equivalent of m_attrpos
  float4 color: COLOR0; // equivalent of m_attrcol
  float2 tex : TEXCOORD0; // equivalent of m_attrcord0
  float2 tex2 : TEXCOORD1; // equivalent of m_attrcord1
};

struct PS_INPUT
{
  float4 pos : SV_POSITION; // equivalent of gl_Position
  float4 color: COLOR0; // equivalent of m_colour
  float2 tex : TEXCOORD0; // equivalent of m_cord0
  float2 tex2 : TEXCOORD1; // equivalent of m_cord1
};

SamplerState LinearSampler : register(s0)
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = CLAMP;
  AddressV = CLAMP;
  Comparison = NEVER;
};

SamplerState NearestSampler : register(s1);

cbuffer cbWorld : register(b0)
{
  float4x4 worldViewProj;
  float4   m_shaderClip; // clip rect (x1,y1,x2,y2) in font-local space
  float2   m_texStep; // tex step per position unit - equivalent of m_cordStep.xy
  float2   m_texStep2; // tex2 step per position unit - equivalent of m_cordStep.zw
  float blackLevel;
  float colorRange;
  float sdrPeakLum;
  int PQ;
  float depth;
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