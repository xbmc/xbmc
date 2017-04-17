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
float     m_CLUTsize;
texture3D m_CLUT;

SamplerState LutSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  AddressW = CLAMP;
  Filter   = MIN_MAG_MIP_LINEAR;
};
#endif

float4 output4(float4 color)
{
#if defined(KODI_3DLUT)
  half3 scale = (m_CLUTsize - 1.0) / m_CLUTsize;
  half3 offset = 1.0 / (2.0 * m_CLUTsize);
  color.rgb = m_CLUT.Sample(LutSampler, color.rgb*scale + offset).rgb;
#endif
  return color;
}

float4 output(float3 color)
{
    return output4(float4(color, 1.0));
}

#if defined(KODI_OUTPUT_T)
#include "convolution_d3d.fx"
float3 m_params; // 0 - range (0 - full, 1 - limited), 1 - contrast, 2 - brightness

float4 OUTPUT_PS(VS_OUTPUT In) : SV_TARGET
{
  float4 color = g_Texture.Sample(KernelSampler, In.TextureUV);
  if (m_params.x)
    color = saturate(0.0625 + color * 219.0 / 255.0);

  color *= m_params.y * 2.0;
  color += m_params.z - 0.5;
  color.a = 1.0;

  return output4(color);
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