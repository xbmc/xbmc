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

#include "convolution_d3d.fx"
#include "output_d3d.fx"

SamplerState RGBSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  Filter   = MIN_MAG_POINT_MIP_LINEAR;
};

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

inline half3 pixel(float xpos, float ypos)
{
  return g_Texture.Sample(RGBSampler, float2(xpos, ypos)).rgb;
}

half3 getLine(float ypos, float3 xpos1, float3 xpos2, half3 linetaps1, half3 linetaps2)
{
  return
    pixel(xpos1.r, ypos) * linetaps1.r +
    pixel(xpos1.g, ypos) * linetaps2.r +
    pixel(xpos1.b, ypos) * linetaps1.g +
    pixel(xpos2.r, ypos) * linetaps2.g +
    pixel(xpos2.g, ypos) * linetaps1.b +
    pixel(xpos2.b, ypos) * linetaps2.b;
}

// Code for first pass - horizontal
float4 CONVOLUTION6x6Horiz(in float2 TextureUV : TEXCOORD0) : SV_TARGET
{
  float2 f = frac(TextureUV / g_StepXY.xy + 0.5);

  half3 linetaps1 = weight((1.0 - f.x) / 2.0);
  half3 linetaps2 = weight((1.0 - f.x) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float xstart = (-2.0 - f.x) * g_StepXY.x + TextureUV.x;

  float3 xpos1 = xstart + g_StepXY.x * float3(0.0, 1.0, 2.0);
  float3 xpos2 = xstart + g_StepXY.x * float3(3.0, 4.0, 5.0);

  return float4(getLine(TextureUV.y, xpos1, xpos2, linetaps1, linetaps2), 1.0);
}

// Code for second pass - vertical

half3 getRow(float xpos, float3 ypos1, float3 ypos2, half3 columntaps1, half3 columntaps2)
{
  return
    pixel(xpos, ypos1.r) * columntaps1.r +
    pixel(xpos, ypos1.g) * columntaps2.r +
    pixel(xpos, ypos1.b) * columntaps1.g +
    pixel(xpos, ypos2.r) * columntaps2.g +
    pixel(xpos, ypos2.g) * columntaps1.b +
    pixel(xpos, ypos2.b) * columntaps2.b;
}

float4 CONVOLUTION6x6Vert(in float2 TextureUV : TEXCOORD0) : SV_TARGET
{
  float2 f = frac(TextureUV / g_StepXY.zw + 0.5);

  half3 columntaps1 = weight((1.0 - f.y) / 2.0);
  half3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float ystart = (-2.0 - f.y) * g_StepXY.w + TextureUV.y;

  float3 ypos1 = ystart + g_StepXY.w * float3(0.0, 1.0, 2.0);
  float3 ypos2 = ystart + g_StepXY.w * float3(3.0, 4.0, 5.0);

  return output(g_colorRange.x + g_colorRange.y * saturate(getRow(TextureUV.x, ypos1, ypos2, columntaps1, columntaps2)));
}

technique11 SCALER_T
{
  pass P0
  {
    SetVertexShader( VS_SHADER );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION6x6Horiz() ) );
  }
  pass P1
  {
    SetVertexShader( VS_SHADER );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION6x6Vert() ) );
  }
};
