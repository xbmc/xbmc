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

texture g_YTexture;
texture g_UTexture;
texture g_VTexture;
float4x4 g_ColorMatrix;
float2  g_StepXY;

sampler YSampler =
  sampler_state {
    Texture = <g_YTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

sampler USampler =
  sampler_state {
    Texture = <g_UTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

sampler VSampler =
  sampler_state
  {
    Texture = <g_VTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

struct VS_OUTPUT
{
  float4 Position   : POSITION;
  float2 TextureY   : TEXCOORD0;
  float2 TextureU   : TEXCOORD1;
  float2 TextureV   : TEXCOORD2;
};

struct PS_OUTPUT
{
  float4 RGBColor : COLOR0;
};

PS_OUTPUT YUV2RGB( VS_OUTPUT In)
{
  PS_OUTPUT OUT;
#if defined(XBMC_YV12)
  float4 YUV = float4(tex2D (YSampler, In.TextureY).x
                    , tex2D (USampler, In.TextureU).x
                    , tex2D (VSampler, In.TextureV).x
                    , 1.0);
#elif defined(XBMC_NV12)
  float4 YUV = float4(tex2D (YSampler, In.TextureY).x
                    , tex2D (USampler, In.TextureU).ra
                    , 1.0);
#elif defined(XBMC_YUY2) || defined(XBMC_UYVY)
  // The HLSL compiler is smart enough to optimize away these redundant assignments.
  // That way the code is almost identical to the OGL shader.
  float2 stepxy = g_StepXY;
  float2 pos    = In.TextureY;
  pos           = float2(pos.x - (stepxy.x * 0.25), pos.y);
  float2 f      = frac(pos / stepxy);

  //y axis will be correctly interpolated by opengl
  //x axis will not, so we grab two pixels at the center of two columns and interpolate ourselves
  float4 c1 = tex2D(YSampler, float2(pos.x + ((0.5 - f.x) * stepxy.x), pos.y));
  float4 c2 = tex2D(YSampler, float2(pos.x + ((1.5 - f.x) * stepxy.x), pos.y));

  /* each pixel has two Y subpixels and one UV subpixel
      YUV  Y  YUV
      check if we're left or right of the middle Y subpixel and interpolate accordingly*/
  #if defined(XBMC_YUY2) // BGRA = YUYV
    float  leftY  = lerp(c1.b, c1.r, f.x * 2.0);
    float  rightY = lerp(c1.r, c2.b, f.x * 2.0 - 1.0);
    float2 outUV  = lerp(c1.ga, c2.ga, f.x);
  #elif defined(XBMC_UYVY) // BGRA = UYVY
    float  leftY  = lerp(c1.g, c1.a, f.x * 2.0);
    float  rightY = lerp(c1.a, c2.g, f.x * 2.0 - 1.0);
    float2 outUV  = lerp(c1.br, c2.br, f.x);
  #endif
    float  outY   = lerp(leftY, rightY, step(0.5, f.x));
    float4 YUV    = float4(outY, outUV, 1.0);
#endif

  OUT.RGBColor = mul(YUV, g_ColorMatrix);
  OUT.RGBColor.a = 1.0;
  return OUT;
}

technique YUV2RGB_T
{
  pass P0
  {
    PixelShader  = compile ps_2_0 YUV2RGB();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }
};
