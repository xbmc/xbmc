/*
 *      Copyright (C) 2005-2013 Team XBMC
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

half3 pixel(sampler samp, float xpos, float ypos)
{
  return tex2D(samp, float2(xpos, ypos)).rgb;
}

// Code for first pass - horizontal

half3 getLine(float ypos, float4 xpos, half4 linetaps)
{
  return
    pixel(RGBSampler, xpos.r, ypos) * linetaps.r +
    pixel(RGBSampler, xpos.g, ypos) * linetaps.g +
    pixel(RGBSampler, xpos.b, ypos) * linetaps.b +
    pixel(RGBSampler, xpos.a, ypos) * linetaps.a;
}

PS_OUTPUT CONVOLUTION4x4Horiz(VS_OUTPUT In)
{
  PS_OUTPUT OUT;

  float2 pos = In.TextureUV + g_StepXY_P0 * 0.5;
  float2 f = frac(pos / g_StepXY_P0);

  half4 linetaps = weight(1.0 - f.x);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float2 xystart;
  xystart.x = (-1.0 - f.x) * g_StepXY_P0.x + In.TextureUV.x;
  xystart.y = In.TextureUV.y;

  float4 xpos = float4(
      xystart.x,
      xystart.x + g_StepXY_P0.x,
      xystart.x + g_StepXY_P0.x * 2.0,
      xystart.x + g_StepXY_P0.x * 3.0);

  OUT.RGBColor.rgb = getLine(xystart.y, xpos, linetaps);
  OUT.RGBColor.a = 1.0;

  return OUT;
}

// Code for second pass - vertical

half3 getRow(float xpos, float4 ypos, half4 columntaps)
{
  return
    pixel(IntermediateSampler, xpos, ypos.r) * columntaps.r +
    pixel(IntermediateSampler, xpos, ypos.g) * columntaps.g +
    pixel(IntermediateSampler, xpos, ypos.b) * columntaps.b +
    pixel(IntermediateSampler, xpos, ypos.a) * columntaps.a;
}

PS_OUTPUT CONVOLUTION4x4Vert(VS_OUTPUT In)
{
  PS_OUTPUT OUT;

  float2 pos = In.TextureUV + g_StepXY_P1 * 0.5;
  float2 f = frac(pos / g_StepXY_P1);

  half4 columntaps = weight(1.0 - f.y);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float2 xystart;
  xystart.x = In.TextureUV.x;
  xystart.y = (-1.0 - f.y) * g_StepXY_P1.y + In.TextureUV.y;

  float4 ypos = float4(
      xystart.y,
      xystart.y + g_StepXY_P1.y,
      xystart.y + g_StepXY_P1.y * 2.0,
      xystart.y + g_StepXY_P1.y * 3.0);

  OUT.RGBColor.rgb = getRow(xystart.x, ypos, columntaps);
  OUT.RGBColor.a = 1.0;

  return OUT;
}

technique SCALER_T
{
  pass P0
  {
    PixelShader  = compile ps_3_0 CONVOLUTION4x4Horiz();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }
  pass P1
  {
    PixelShader  = compile ps_3_0 CONVOLUTION4x4Vert();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }

};
