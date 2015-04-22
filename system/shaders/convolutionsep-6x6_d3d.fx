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
float2    g_StepXY_P0;
float2    g_StepXY_P1;
float     g_viewportWidth;
float     g_viewportHeight;

SamplerState RGBSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  Filter   = MIN_MAG_POINT_MIP_LINEAR;
};

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
  float4 Position   : SV_POSITION;
  float2 TextureUV  : TEXCOORD0;
};

//
// VS for rendering in screen space
//
VS_OUTPUT VS(VS_INPUT In)
{
  VS_OUTPUT output  = (VS_OUTPUT)0;
  output.Position.x =  (In.Position.x / (g_viewportWidth  / 2.0)) - 1;
  output.Position.y = -(In.Position.y / (g_viewportHeight / 2.0)) + 1;
  output.Position.z = output.Position.z;
  output.Position.w = 1.0;
  output.TextureUV  = In.TextureUV;

  return output;
}

inline half3 weight(float pos)
{
#ifdef HAS_RGBA
  half3 w = g_KernelTexture.Sample(KernelSampler, pos).rgb;
#else
  half3 w = g_KernelTexture.Sample(KernelSampler, pos).bgr;
#endif

#ifndef HAS_FLOAT_TEXTURE
  w = w * 2.0 - 1.0;
#endif
  return w;
}

inline half3 pixel(texture2D tex, float xpos, float ypos)
{
  return tex.Sample(RGBSampler, float2(xpos, ypos)).rgb;
}

half3 getLine(float ypos, float3 xpos1, float3 xpos2, half3 linetaps1, half3 linetaps2)
{
  return
    g_Texture.Sample(RGBSampler, float2(xpos1.r, ypos)).rgb * linetaps1.r +
    g_Texture.Sample(RGBSampler, float2(xpos1.g, ypos)).rgb * linetaps2.r +
    g_Texture.Sample(RGBSampler, float2(xpos1.b, ypos)).rgb * linetaps1.g +
    g_Texture.Sample(RGBSampler, float2(xpos2.r, ypos)).rgb * linetaps2.g +
    g_Texture.Sample(RGBSampler, float2(xpos2.g, ypos)).rgb * linetaps1.b +
    g_Texture.Sample(RGBSampler, float2(xpos2.b, ypos)).rgb * linetaps2.b;
}

// Code for first pass - horizontal
float4 CONVOLUTION6x6Horiz(VS_OUTPUT In) : SV_TARGET
{
  float2 pos = In.TextureUV + g_StepXY_P0 * 0.5;
  float2 f = frac(pos / g_StepXY_P0);

  half3 linetaps1 = weight((1.0 - f.x) / 2.0);
  half3 linetaps2 = weight((1.0 - f.x) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float xstart = (-2.0 - f.x) * g_StepXY_P0.x + In.TextureUV.x;

  float3 xpos1 = float3(
      xstart,
      xstart + g_StepXY_P0.x,
      xstart + g_StepXY_P0.x * 2.0);
  float3 xpos2 = half3(
      xstart + g_StepXY_P0.x * 3.0,
      xstart + g_StepXY_P0.x * 4.0,
      xstart + g_StepXY_P0.x * 5.0);

  return float4(getLine(In.TextureUV.y, xpos1, xpos2, linetaps1, linetaps2), 1.0);
}

// Code for second pass - vertical

half3 getRow(float xpos, float3 ypos1, float3 ypos2, half3 columntaps1, half3 columntaps2)
{
  return
    g_Texture.Sample(RGBSampler, float2(xpos, ypos1.r)).rgb * columntaps1.r +
    g_Texture.Sample(RGBSampler, float2(xpos, ypos1.g)).rgb * columntaps2.r +
    g_Texture.Sample(RGBSampler, float2(xpos, ypos1.b)).rgb * columntaps1.g +
    g_Texture.Sample(RGBSampler, float2(xpos, ypos2.r)).rgb * columntaps2.g +
    g_Texture.Sample(RGBSampler, float2(xpos, ypos2.g)).rgb * columntaps1.b +
    g_Texture.Sample(RGBSampler, float2(xpos, ypos2.b)).rgb * columntaps2.b;
}

float4 CONVOLUTION6x6Vert(VS_OUTPUT In) : SV_TARGET
{
  float2 pos = In.TextureUV + g_StepXY_P1 * 0.5;
  float2 f = frac(pos / g_StepXY_P1);

  half3 columntaps1 = weight((1.0 - f.y) / 2.0);
  half3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float ystart = (-2.0 - f.y) * g_StepXY_P1.y + In.TextureUV.y;

  float3 ypos1 = float3(
      ystart,
      ystart + g_StepXY_P1.y,
      ystart + g_StepXY_P1.y * 2.0);
  float3 ypos2 = half3(
      ystart + g_StepXY_P1.y * 3.0,
      ystart + g_StepXY_P1.y * 4.0,
      ystart + g_StepXY_P1.y * 5.0);

  return float4(getRow(In.TextureUV.x, ypos1, ypos2, columntaps1, columntaps2), 1.0);
}

technique10 SCALER_T
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0_level_9_3, VS() ) );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION6x6Horiz() ) );
  }
  pass P1
  {
    SetVertexShader( CompileShader( vs_4_0_level_9_3, VS() ) );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION6x6Vert() ) );
  }
};
