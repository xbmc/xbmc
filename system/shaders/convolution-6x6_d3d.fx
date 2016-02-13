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
float2    g_StepXY;
float2    g_viewPort;
float2    g_colorRange;

SamplerState RGBSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  Filter   = MIN_MAG_MIP_POINT;
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

half3 weight(float pos)
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

inline half3 getLine(float ypos, float3 xpos1, float3 xpos2, half3 linetaps1, half3 linetaps2)
{
  return
    pixel(xpos1.r, ypos) * linetaps1.r +
    pixel(xpos1.g, ypos) * linetaps2.r +
    pixel(xpos1.b, ypos) * linetaps1.g +
    pixel(xpos2.r, ypos) * linetaps2.g +
    pixel(xpos2.g, ypos) * linetaps1.b +
    pixel(xpos2.b, ypos) * linetaps2.b;
}

float4 CONVOLUTION6x6(in float2 TextureUV  : TEXCOORD0) : SV_TARGET
{
  float2 f = frac(TextureUV / g_StepXY + 0.5);

  half3 linetaps1   = weight((1.0 - f.x) / 2.0);
  half3 linetaps2   = weight((1.0 - f.x) / 2.0 + 0.5);
  half3 columntaps1 = weight((1.0 - f.y) / 2.0);
  half3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  // kernel generation code made sure taps add up to 1, no need to adjust here.

  float2 xystart = (-2.0 - f) * g_StepXY + TextureUV;
  float3 xpos1 = xystart.x + g_StepXY.x * float3(0.0, 1.0, 2.0);
  float3 xpos2 = xystart.x + g_StepXY.x * float3(3.0, 4.0, 5.0);
  float3 ypos1 = xystart.y + g_StepXY.y * float3(0.0, 1.0, 2.0);
  float3 ypos2 = xystart.y + g_StepXY.y * float3(3.0, 4.0, 5.0);

  float3 rgb =  
        getLine(ypos1.x, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
		getLine(ypos1.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
		getLine(ypos1.z, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
		getLine(ypos2.x, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
		getLine(ypos2.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
		getLine(ypos2.z, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

  return float4(g_colorRange.x + g_colorRange.y * saturate(rgb), 1.0);
}

technique11 SCALER_T
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0_level_9_1, VS() ) );
    SetPixelShader( CompileShader( ps_4_0_level_9_3, CONVOLUTION6x6() ) );
  }
};
