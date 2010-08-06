/*
 *      Copyright (C) 2005-2010 Team XBMC
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
float2  g_StepXY;

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
  return tex1D(KernelSampler, pos).rgb;
}

half3 pixel(float xpos, float ypos)
{
  return tex2D(RGBSampler, float2(xpos, ypos)).rgb;
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

PS_OUTPUT CONVOLUTION6x6(VS_OUTPUT In)
{
  PS_OUTPUT OUT;

  float2 pos = In.TextureUV + g_StepXY * 0.5;
  float2 f = frac(pos / g_StepXY);

  half3 linetaps1   = weight((1.0 - f.x) / 2.0);
  half3 linetaps2   = weight((1.0 - f.x) / 2.0 + 0.5);
  half3 columntaps1 = weight((1.0 - f.y) / 2.0);
  half3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float2 xystart = (-2.0 - f) * g_StepXY + In.TextureUV;
  float3 xpos1 = float3(
      xystart.x,
      xystart.x + g_StepXY.x,
      xystart.x + g_StepXY.x * 2.0);
  float3 xpos2 = half3(
      xystart.x + g_StepXY.x * 3.0,
      xystart.x + g_StepXY.x * 4.0,
      xystart.x + g_StepXY.x * 5.0);

  OUT.RGBColor.rgb = getLine(xystart.y                   , xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
					 getLine(xystart.y + g_StepXY.y      , xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
					 getLine(xystart.y + g_StepXY.y * 2.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
					 getLine(xystart.y + g_StepXY.y * 3.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
					 getLine(xystart.y + g_StepXY.y * 4.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
					 getLine(xystart.y + g_StepXY.y * 5.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

  OUT.RGBColor.a = 1.0;
  return OUT;
}

technique SCALER_T
{
  pass P0
  {
    PixelShader  = compile ps_3_0 CONVOLUTION6x6();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }
};
