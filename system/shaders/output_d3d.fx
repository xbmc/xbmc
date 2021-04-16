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
float3 inverseHLG(float3 x)
{
  const float B67_a = 0.17883277f;
  const float B67_b = 0.28466892f;
  const float B67_c = 0.55991073f;
  const float B67_inv_r2 = 4.0f;
  x = (x <= 0.5f) ? x * x * B67_inv_r2 : exp((x - B67_c) / B67_a) + B67_b;
  return x;
}

float3 tranferPQ(float3 x)
{
  x = pow(x / 1000.0f, ST2084_m1);
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
  color.rgb = inverseHLG(color.rgb);
  float3 ootf_2020 = float3(0.2627f, 0.6780f, 0.0593f);
  float ootf_ys = 2000.0f * dot(ootf_2020, color.rgb);
  color.rgb *= pow(ootf_ys, 0.2f);
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
    color = saturate(0.0625 + color * 219.0 / 255.0);

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