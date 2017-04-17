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

texture2D g_Texture;
texture2D g_KernelTexture;
float4    g_StepXY;
float2    g_viewPort;
float2    g_colorRange;

SamplerState KernelSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  Filter   = MIN_MAG_MIP_LINEAR;
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
VS_OUTPUT VS(VS_INPUT In)
{
  VS_OUTPUT output  = (VS_OUTPUT)0;
  output.Position.x =  (In.Position.x / (g_viewPort.x  / 2.0)) - 1;
  output.Position.y = -(In.Position.y / (g_viewPort.y / 2.0)) + 1;
  output.Position.z = output.Position.z;
  output.Position.w = 1.0;
  output.TextureUV  = In.TextureUV;

  return output;
}

#define VS_SHADER CompileShader( vs_4_0_level_9_1, VS() )
