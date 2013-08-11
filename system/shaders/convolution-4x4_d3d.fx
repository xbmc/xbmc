/*
 *      Copyright (C) 2005-2010-2013 Team XBMC
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

texture g_Texture;
texture g_KernelTexture;
float2  g_StepXY;

sampler RGBSampler =
  sampler_state {
    Texture = <g_Texture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MinFilter = POINT;
    MagFilter = POINT;
  };

sampler KernelSampler =
  sampler_state
  {
    Texture = <g_KernelTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
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

half4 weight(float pos)
{
  half4 w;
#ifdef HAS_RGBA
  w = tex1D(KernelSampler, pos);
#else
  w = tex1D(KernelSampler, pos).bgra;
#endif

#ifdef HAS_FLOAT_TEXTURE
  return w;
#else
  return w * 2.0 - 1.0;
#endif
}

half3 pixel(float xpos, float ypos)
{
  return tex2D(RGBSampler, float2(xpos, ypos)).rgb;
}

half3 getLine(float ypos, float4 xpos, half4 linetaps)
{
  return
    pixel(xpos.r, ypos) * linetaps.r +
    pixel(xpos.g, ypos) * linetaps.g +
    pixel(xpos.b, ypos) * linetaps.b +
    pixel(xpos.a, ypos) * linetaps.a;
}

PS_OUTPUT CONVOLUTION4x4(VS_OUTPUT In)
{
  PS_OUTPUT OUT;

  float2 pos = In.TextureUV + g_StepXY * 0.5;
  float2 f = frac(pos / g_StepXY);

  half4 linetaps   = weight(1.0 - f.x);
  half4 columntaps = weight(1.0 - f.y);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float2 xystart = (-1.0 - f) * g_StepXY + In.TextureUV;
  float4 xpos = float4(
      xystart.x,
      xystart.x + g_StepXY.x,
      xystart.x + g_StepXY.x * 2.0,
      xystart.x + g_StepXY.x * 3.0);

  OUT.RGBColor.rgb =
    getLine(xystart.y                   , xpos, linetaps) * columntaps.r +
    getLine(xystart.y + g_StepXY.y      , xpos, linetaps) * columntaps.g +
    getLine(xystart.y + g_StepXY.y * 2.0, xpos, linetaps) * columntaps.b +
    getLine(xystart.y + g_StepXY.y * 3.0, xpos, linetaps) * columntaps.a;

  OUT.RGBColor.a = 1.0;
  return OUT;
}

technique SCALER_T
{
  pass P0
  {
    PixelShader  = compile ps_3_0 CONVOLUTION4x4();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }
};
