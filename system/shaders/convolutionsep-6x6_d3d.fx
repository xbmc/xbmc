/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

texture g_Texture;
texture g_KernelTexture;
texture g_IntermediateTexture;
float2  g_StepXY_P0;
float2  g_StepXY_P1;

sampler RGBSampler =
  sampler_state {
    Texture = <g_Texture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = POINT;
    MagFilter = POINT;
  };

sampler KernelSampler =
  sampler_state
  {
    Texture = <g_KernelTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

sampler IntermediateSampler =
  sampler_state {
    Texture = <g_IntermediateTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = POINT;
    MagFilter = POINT;
  };

struct VS_OUTPUT
{
  float4 Position   : POSITION;
  float2 TextureUV  : TEXCOORD0;
};

struct PS_OUTPUT
{
  float4 RGBColor : COLOR0;
};

half3 weight(float pos)
{
  half3 w;
#ifdef HAS_RGBA
  w = tex1D(KernelSampler, pos).rgb;
#else
  w = tex1D(KernelSampler, pos).bgr;
#endif

#ifdef HAS_FLOAT_TEXTURE
  return w;
#else
  return w * 2.0 - 1.0;
#endif
}

half3 pixel(sampler samp, float xpos, float ypos)
{
  return tex2D(samp, float2(xpos, ypos)).rgb;
}

// Code for first pass - horizontal

half3 getLine(float ypos, float3 xpos1, float3 xpos2, half3 linetaps1, half3 linetaps2)
{
  return
    pixel(RGBSampler, xpos1.r, ypos) * linetaps1.r +
    pixel(RGBSampler, xpos1.g, ypos) * linetaps2.r +
    pixel(RGBSampler, xpos1.b, ypos) * linetaps1.g +
    pixel(RGBSampler, xpos2.r, ypos) * linetaps2.g +
    pixel(RGBSampler, xpos2.g, ypos) * linetaps1.b +
    pixel(RGBSampler, xpos2.b, ypos) * linetaps2.b;
}

PS_OUTPUT CONVOLUTION6x6Horiz(VS_OUTPUT In)
{
  PS_OUTPUT OUT;

  float2 pos = In.TextureUV + g_StepXY_P0 * 0.5;
  float2 f = frac(pos / g_StepXY_P0);

  half3 linetaps1 = weight((1.0 - f.x) / 2.0);
  half3 linetaps2 = weight((1.0 - f.x) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float2 xystart;
  xystart.x = (-2.0 - f.x) * g_StepXY_P0.x + In.TextureUV.x;
  xystart.y = In.TextureUV.y;

  float3 xpos1 = float3(
      xystart.x,
      xystart.x + g_StepXY_P0.x,
      xystart.x + g_StepXY_P0.x * 2.0);
  float3 xpos2 = half3(
      xystart.x + g_StepXY_P0.x * 3.0,
      xystart.x + g_StepXY_P0.x * 4.0,
      xystart.x + g_StepXY_P0.x * 5.0);

  OUT.RGBColor.rgb = getLine(xystart.y, xpos1, xpos2, linetaps1, linetaps2);
  OUT.RGBColor.a = 1.0;

  return OUT;
}

// Code for second pass - vertical

half3 getRow(float xpos, float3 ypos1, float3 ypos2, half3 columntaps1, half3 columntaps2)
{
  return
    pixel(IntermediateSampler, xpos, ypos1.r) * columntaps1.r +
    pixel(IntermediateSampler, xpos, ypos1.g) * columntaps2.r +
    pixel(IntermediateSampler, xpos, ypos1.b) * columntaps1.g +
    pixel(IntermediateSampler, xpos, ypos2.r) * columntaps2.g +
    pixel(IntermediateSampler, xpos, ypos2.g) * columntaps1.b +
    pixel(IntermediateSampler, xpos, ypos2.b) * columntaps2.b;
}

PS_OUTPUT CONVOLUTION6x6Vert(VS_OUTPUT In)
{
  PS_OUTPUT OUT;

  float2 pos = In.TextureUV + g_StepXY_P1 * 0.5;
  float2 f = frac(pos / g_StepXY_P1);

  half3 columntaps1 = weight((1.0 - f.y) / 2.0);
  half3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float2 xystart;
  xystart.x = In.TextureUV.x;
  xystart.y = (-2.0 - f.y) * g_StepXY_P1.y + In.TextureUV.y;

  float3 ypos1 = float3(
      xystart.y,
      xystart.y + g_StepXY_P1.y,
      xystart.y + g_StepXY_P1.y * 2.0);
  float3 ypos2 = half3(
      xystart.y + g_StepXY_P1.y * 3.0,
      xystart.y + g_StepXY_P1.y * 4.0,
      xystart.y + g_StepXY_P1.y * 5.0);

  OUT.RGBColor.rgb = getRow(xystart.x, ypos1, ypos2, columntaps1, columntaps2);
  OUT.RGBColor.a = 1.0;

  return OUT;
}

technique SCALER_T
{
  pass P0
  {
    PixelShader  = compile ps_3_0 CONVOLUTION6x6Horiz();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }
  pass P1
  {
    PixelShader  = compile ps_3_0 CONVOLUTION6x6Vert();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }
};
