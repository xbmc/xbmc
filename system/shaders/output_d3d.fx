/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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

#if defined(KODI_3DLUT)
float2    m_LUTParams; // x- scale, y- offset
texture3D m_LUT;

SamplerState LutSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  AddressW = CLAMP;
  Filter   = MIN_MAG_MIP_LINEAR;
};
#endif
#if defined(KODI_DITHER)
float3    m_ditherParams;
texture2D m_ditherMatrix;

SamplerState DitherSampler : IMMUTABLE
{
  AddressU = WRAP;
  AddressV = WRAP;
  Filter   = MIN_MAG_MIP_POINT;
};
#endif
#if (defined(KODI_TONE_MAPPING_ACES) || defined(KODI_TONE_MAPPING_HABLE) || defined(KODI_HLG_TO_PQ))
static const float ST2084_m1 = 2610.0f / (4096.0f * 4.0f);
static const float ST2084_m2 = (2523.0f / 4096.0f) * 128.0f;
static const float ST2084_c1 = 3424.0f / 4096.0f;
static const float ST2084_c2 = (2413.0f / 4096.0f) * 32.0f;
static const float ST2084_c3 = (2392.0f / 4096.0f) * 32.0f;
#endif
#if defined(KODI_TONE_MAPPING_REINHARD)
float g_toneP1;
float3 g_coefsDst;

float reinhard(float x)
{
  return x * (1.0f + x / (g_toneP1 * g_toneP1)) / (1.0f + x);
}
#endif
#if defined(KODI_TONE_MAPPING_ACES)
float g_luminance;
float g_toneP1;

float3 aces(float3 x)
{
  const float A = 2.51f;
  const float B = 0.03f;
  const float C = 2.43f;
  const float D = 0.59f;
  const float E = 0.14f;
  return (x * (A * x + B)) / (x * (C * x + D) + E);
}
#endif
#if defined(KODI_TONE_MAPPING_HABLE)
float g_toneP1;
float g_toneP2;

float3 hable(float3 x)
{
  const float A = 0.15f;
  const float B = 0.5f;
  const float C = 0.1f;
  const float D = 0.2f;
  const float E = 0.02f;
  const float F = 0.3f;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
#endif
#if (defined(KODI_TONE_MAPPING_ACES) || defined(KODI_TONE_MAPPING_HABLE))
float3 inversePQ(float3 x)
{
  x = pow(max(x, 0.0f), 1.0f / ST2084_m2);
  x = max(x - ST2084_c1, 0.0f) / (ST2084_c2 - ST2084_c3 * x);
  x = pow(x, 1.0f / ST2084_m1);
  return x;
}
#endif
#if defined(KODI_HLG_TO_PQ)

// HLG inverse OETF - BT.2100
// input: non-linear signal [0,1] range
// output: linear [0,1] range
float3 inverseHLG(float3 x)
{
  static const float B67_a = 0.17883277f;
  static const float B67_b = 0.28466892f; // b = 1 - 4*a
  static const float B67_c = 0.55991073f; // c = 0.5 - a*log(4*a)
  x = (x <= 0.5f) ? x * x / 3.0f : (exp((x - B67_c) / B67_a) + B67_b) / 12.0f;
  return x;
}

// PQ inverse EOTF, BT.2100
// input: linear cd/m2 [0,10000] range
// output: non-linear [0,1] range
float3 tranferPQ(float3 x)
{
  x = pow(x / 10000.0f, ST2084_m1);
  x = (ST2084_c1 + ST2084_c2 * x) / (1.0f + ST2084_c3 * x);
  x = pow(x, ST2084_m2);
  return x;
}
#endif


float4 output4(float4 color, float2 uv)
{
#if defined(KODI_TONE_MAPPING_REINHARD)
  float luma = dot(color.rgb, g_coefsDst);
  color.rgb *= reinhard(luma) / luma;
#endif
#if defined(KODI_TONE_MAPPING_ACES)
  color.rgb = inversePQ(color.rgb);
  color.rgb *= (10000.0f / g_luminance) * (2.0f / g_toneP1);
  color.rgb = aces(color.rgb);
  color.rgb *= (1.24f / g_toneP1);
  color.rgb = pow(color.rgb, 0.27f);
#endif
#if defined(KODI_TONE_MAPPING_HABLE)
  color.rgb = inversePQ(color.rgb);
  color.rgb *= g_toneP1;
  color.rgb = hable(color.rgb * g_toneP2) / hable(g_toneP2);
  color.rgb = pow(color.rgb, 1.0f / 2.2f);
#endif
#if defined(KODI_HLG_TO_PQ)

  // Reference: BT.2100, Table 5, HLG Reference EOTF

  // Display peak luminance in cd/m2
  static const float HLG_Lw = 1000.0f;
  static const float HLG_gamma = 1.2f + 0.42f * log10(HLG_Lw / 1000.0f);

  // color.rgb: E', range [0,1]
  color.rgb = inverseHLG(color.rgb);
  // color.rgb: E, range [0,1]
  static const float3 bt2020_lum_rgbweights = float3(0.2627f, 0.6780f, 0.0593f);
  float HLG_Ys = dot(bt2020_lum_rgbweights, color.rgb);
  color.rgb *= HLG_Lw * pow(HLG_Ys, HLG_gamma - 1.0f);

  // color.rgb: FD, in cd/m2
  color.rgb = tranferPQ(color.rgb);
#endif
#if defined(KODI_3DLUT)
  half3 scale = m_LUTParams.x;
  half3 offset = m_LUTParams.y;
  float3 lutRGB = m_LUT.Sample(LutSampler, color.rgb*scale + offset).rgb;
  color.rgb = scale.x ? lutRGB : color.rgb;
#endif
#if defined(KODI_DITHER)
  half2 ditherpos  = uv * m_ditherParams.xy;
  // scale ditherval to [0,1)
  float ditherval = m_ditherMatrix.Sample(DitherSampler, ditherpos).r * 16.0f;
  color.rgb = floor(color.rgb * m_ditherParams.z + ditherval) / m_ditherParams.z;
#endif
  return color;
}

float4 output(float3 color, float2 uv)
{
  return output4(float4(color, 1.0), uv);
}

#if defined(KODI_OUTPUT_T)
#include "convolution_d3d.fx"

#if (defined(KODI_TONE_MAPPING_ACES) || defined(KODI_TONE_MAPPING_HABLE) || defined(KODI_HLG_TO_PQ))
#define PS_PROFILE ps_4_0_level_9_3
#else
#define PS_PROFILE ps_4_0_level_9_1
#endif

float3 m_params; // 0 - range (0 - full, 1 - limited), 1 - contrast, 2 - brightness

float4 OUTPUT_PS(VS_OUTPUT In) : SV_TARGET
{
  float4 color = g_Texture.Sample(KernelSampler, In.TextureUV);
  [flatten] if (m_params.x)
    color = saturate((64.0 / 1023.0) + color * (940.0 - 64.0) / 1023.0);

  color *= m_params.y * 2.0;
  color += m_params.z - 0.5;
  color.a = 1.0;

  return output4(color, In.TextureUV);
}

technique11 OUTPUT_T
{
  pass P0
  {
    SetVertexShader( VS_SHADER );
    SetPixelShader( CompileShader( PS_PROFILE, OUTPUT_PS() ) );
  }
};
#endif