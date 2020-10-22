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
#if defined(KODI_TONE_MAPPING)
float g_toneP1;
float3 g_coefsDst;

float tonemap(float val)
{
  return val * (1 + val/(g_toneP1*g_toneP1))/(1 + val);
}
#endif
#if defined(KODI_HLG_TO_PQ)
float inverseHLG(float x)
{
  const float B67_a = 0.17883277;
  const float B67_b = 0.28466892;
  const float B67_c = 0.55991073;
  const float B67_inv_r2 = 4.0;
  if (x <= 0.5)
    x = x * x * B67_inv_r2;
  else
    x = exp((x - B67_c) / B67_a) + B67_b;
  return x;
}

float4 tranferPQ(float4 color)
{
  const float ST2084_m1 = 2610.0 / (4096.0 * 4.0);
  const float ST2084_m2 = (2523.0 / 4096.0) * 128.0;
  const float ST2084_c1 = 3424.0 / 4096.0;
  const float ST2084_c2 = (2413.0 / 4096.0) * 32.0;
  const float ST2084_c3 = (2392.0 / 4096.0) * 32.0;
  color = pow(color / 1000.0, ST2084_m1);
  color = (ST2084_c1 + ST2084_c2 * color) / (1 + ST2084_c3 * color);
  color = pow(color, ST2084_m2);
  return color;
}
#endif


float4 output4(float4 color, float2 uv)
{
#if defined(KODI_TONE_MAPPING)
  float luma = dot(color.rgb, g_coefsDst);
  color.rgb *= tonemap(luma) / luma;
#endif
#if defined(KODI_HLG_TO_PQ)
  color.r = inverseHLG(color.r);
  color.g = inverseHLG(color.g);
  color.b = inverseHLG(color.b);
  float3 ootf_2020 = float3(0.2627, 0.6780, 0.0593);
  float ootf_ys = 2000.0 * dot(ootf_2020, color);
  color = color * pow(ootf_ys, 0.200);
  color = tranferPQ(color);
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
  float ditherval = m_ditherMatrix.Sample(DitherSampler, ditherpos).r * 16.0;
  color = floor(color * m_ditherParams.z + ditherval) / m_ditherParams.z;
#endif
  return color;
}

float4 output(float3 color, float2 uv)
{
  return output4(float4(color, 1.0), uv);
}

#if defined(KODI_OUTPUT_T)
#include "convolution_d3d.fx"
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
    SetPixelShader( CompileShader( ps_4_0_level_9_1, OUTPUT_PS() ) );
  }
};
#endif