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

inline half4 weight(float pos)
{
#ifdef HAS_RGBA
  half4 w = g_KernelTexture.Sample(KernelSampler, pos);
#else
  half4 w = g_KernelTexture.Sample(KernelSampler, pos).bgra;
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

// Code for first pass - horizontal

inline half3 getLine(float ypos, float4 xpos, half4 linetaps)
{
  return
    pixel(xpos.r, ypos) * linetaps.r +
    pixel(xpos.g, ypos) * linetaps.g +
    pixel(xpos.b, ypos) * linetaps.b +
    pixel(xpos.a, ypos) * linetaps.a;
}

float4 CONVOLUTION4x4Horiz(in float2 TextureUV : TEXCOORD0) : SV_TARGET
{
  float2 f = frac(TextureUV / g_StepXY.xy + 0.5);
  half4 linetaps = weight(1.0 - f.x);

  // kernel generation code made sure taps add up to 1, no need to adjust here.
  float xystart = (-1.0 - f.x) * g_StepXY.x + TextureUV.x;

  float4 xpos = xystart + g_StepXY.x * float4(0.0, 1.0, 2.0, 3.0);
  return float4(getLine(TextureUV.y, xpos, linetaps), 1.0f);
}

// Code for second pass - vertical

inline half3 getRow(float xpos, float4 ypos, half4 columntaps)
{
  return
    pixel(xpos, ypos.r) * columntaps.r +
    pixel(xpos, ypos.g) * columntaps.g +
    pixel(xpos, ypos.b) * columntaps.b +
    pixel(xpos, ypos.a) * columntaps.a;
}

float4 CONVOLUTION4x4Vert(in float2 TextureUV : TEXCOORD0) : SV_TARGET
{
  float2 f = frac(TextureUV / g_StepXY.zw + 0.5);
  half4 columntaps = weight(1.0 - f.y);

  // kernel generation code made sure taps add up to 1, no need to adjust here.
  float xystart = (-1.0 - f.y) * g_StepXY.w + TextureUV.y;

  float4 ypos = xystart + g_StepXY.w * float4(0.0, 1.0, 2.0, 3.0);
  return output(g_colorRange.x + g_colorRange.y * saturate(getRow(TextureUV.x, ypos, columntaps)), TextureUV);
}

technique11 SCALER_T
{
  pass P0
  {
    SetVertexShader( VS_SHADER );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION4x4Horiz() ) );
  }
  pass P1
  {
    SetVertexShader( VS_SHADER );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION4x4Vert() ) );
  }

};
