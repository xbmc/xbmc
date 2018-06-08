/*
 *      Copyright (C) 2017 Team XBMC
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

texture2D g_Texture;
float2    g_viewPort;
float     m_params[1]; // 0 - range (0 - full, 1 - limited)

SamplerState TextureSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
#ifdef SAMP_NEAREST
  Filter   = MIN_MAG_MIP_POINT;
#else
  Filter   = MIN_MAG_MIP_LINEAR;
#endif
};

struct VS_INPUT
{
  float4 Position   : POSITION;
  float2 TextureUV  : TEXCOORD0;
};

struct VS_OUTPUT
{
  float2 TextureUV  : TEXCOORD0;
  float4 Position   : SV_POSITION;
};

//
// VS for rendering in screen space
//
VS_OUTPUT OUTPUT_VS(VS_INPUT In)
{
  VS_OUTPUT output  = (VS_OUTPUT)0;
  output.Position.x =  (In.Position.x / (g_viewPort.x  / 2.0)) - 1;
  output.Position.y = -(In.Position.y / (g_viewPort.y / 2.0)) + 1;
  output.Position.z = output.Position.z;
  output.Position.w = 1.0;
  output.TextureUV  = In.TextureUV;

  return output;
}

float4 OUTPUT_PS(VS_OUTPUT In) : SV_TARGET
{
  float4 color = g_Texture.Sample(TextureSampler, In.TextureUV);
  [flatten] if (m_params[0])
    color = saturate(0.0625 + color * 219.0 / 255.0);

  color.a = 1.0;

  return color;
}

technique11 OUTPUT_T
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0_level_9_1, OUTPUT_VS()) );
    SetPixelShader( CompileShader( ps_4_0_level_9_1, OUTPUT_PS() ) );
  }
};
