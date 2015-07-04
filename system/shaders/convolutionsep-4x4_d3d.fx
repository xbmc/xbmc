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

inline half3 pixel(texture2D tex, float xpos, float ypos)
{
  return tex.Sample(RGBSampler, float2(xpos, ypos)).rgb;
}

// Code for first pass - horizontal

inline half3 getLine(float ypos, float4 xpos, half4 linetaps)
{
  return
    pixel(g_Texture, xpos.r, ypos) * linetaps.r +
    pixel(g_Texture, xpos.g, ypos) * linetaps.g +
    pixel(g_Texture, xpos.b, ypos) * linetaps.b +
    pixel(g_Texture, xpos.a, ypos) * linetaps.a;
}

float4 CONVOLUTION4x4Horiz(VS_OUTPUT In) : SV_TARGET
{
  float2 pos = In.TextureUV + g_StepXY_P0 * 0.5;
  float2 f = frac(pos / g_StepXY_P0);

  half4 linetaps = weight(1.0 - f.x);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float xystart = (-1.0 - f.x) * g_StepXY_P0.x + In.TextureUV.x;

  float4 xpos = float4(
      xystart,
      xystart + g_StepXY_P0.x,
      xystart + g_StepXY_P0.x * 2.0,
      xystart + g_StepXY_P0.x * 3.0);

  return float4(getLine(In.TextureUV.y, xpos, linetaps), 1.0f);
}

// Code for second pass - vertical

inline half3 getRow(float xpos, float4 ypos, half4 columntaps)
{
  return
    pixel(g_Texture, xpos, ypos.r) * columntaps.r +
    pixel(g_Texture, xpos, ypos.g) * columntaps.g +
    pixel(g_Texture, xpos, ypos.b) * columntaps.b +
    pixel(g_Texture, xpos, ypos.a) * columntaps.a;
}

float4 CONVOLUTION4x4Vert(VS_OUTPUT In) : SV_TARGET
{
  float2 pos = In.TextureUV + g_StepXY_P1 * 0.5;
  float2 f = frac(pos / g_StepXY_P1);

  half4 columntaps = weight(1.0 - f.y);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float xystart = (-1.0 - f.y) * g_StepXY_P1.y + In.TextureUV.y;

  float4 ypos = float4(
      xystart,
      xystart + g_StepXY_P1.y,
      xystart + g_StepXY_P1.y * 2.0,
      xystart + g_StepXY_P1.y * 3.0);

  return float4(getRow(In.TextureUV.x, ypos, columntaps), 1.0f);
}

technique10 SCALER_T
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0_level_9_3, VS() ) );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION4x4Horiz() ) );
  }
  pass P1
  {
    SetVertexShader( CompileShader( vs_4_0_level_9_3, VS() ) );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION4x4Vert() ) );
  }

};
