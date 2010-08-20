texture g_YTexture;
texture g_UTexture;
texture g_VTexture;
texture g_KernelTexture;

float2 g_YStep;
float2 g_UVStep;

float4x4 g_ColorMatrix;

sampler YSampler =
  sampler_state {
    Texture = <g_YTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = POINT;
    MagFilter = POINT;
  };

sampler USampler = 
  sampler_state {
    Texture = <g_UTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = POINT;
    MagFilter = POINT;
  };

sampler VSampler = 
  sampler_state
  {
    Texture = <g_VTexture>;
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
  float2 TextureY   : TEXCOORD0;
  float2 TextureU   : TEXCOORD1;
  float2 TextureV   : TEXCOORD2;
};

struct PS_OUTPUT
{
  float4 RGBColor : COLOR0;
};

float3 weight(float pos)
{
  return tex1D(KernelSampler, pos).rgb;
}

float pixel(sampler tex, float xpos, float ypos)
{
  return tex2D(tex, float2(xpos, ypos)).r;
}

float getLine(sampler tex, float ypos, float3 xpos1, float3 xpos2, float3 linetaps1, float3 linetaps2)
{
  float  pixels;

  pixels  = pixel(tex, xpos1.r, ypos) * linetaps1.r;
  pixels += pixel(tex, xpos1.g, ypos) * linetaps2.r;
  pixels += pixel(tex, xpos1.b, ypos) * linetaps1.g;
  pixels += pixel(tex, xpos2.r, ypos) * linetaps2.g;
  pixels += pixel(tex, xpos2.g, ypos) * linetaps1.b;
  pixels += pixel(tex, xpos2.b, ypos) * linetaps2.b;

  return pixels;
}

float getPixel(sampler tex, float2 texCoord, float2 step)
{
  float xf = frac(texCoord.x / step.x);
  float yf = frac(texCoord.y / step.y);

  float3 linetaps1   = weight((1.0 - xf) / 2.0);
  float3 linetaps2   = weight((1.0 - xf) / 2.0 + 0.5);
  float3 columntaps1 = weight((1.0 - yf) / 2.0);
  float3 columntaps2 = weight((1.0 - yf) / 2.0 + 0.5);

  float3 xpos1 = float3(
      (-1.5 - xf) * step.x + texCoord.x,
      (-0.5 - xf) * step.x + texCoord.x,
      ( 0.5 - xf) * step.x + texCoord.x);
  float3 xpos2 = float3(
      ( 1.5 - xf) * step.x + texCoord.x,
      ( 2.5 - xf) * step.x + texCoord.x,
      ( 3.5 - xf) * step.x + texCoord.x);

  float fragColor;
  fragColor  = getLine(tex, (-1.5 - yf) * step.y + texCoord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r;
  fragColor += getLine(tex, (-0.5 - yf) * step.y + texCoord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r;
  fragColor += getLine(tex, ( 0.5 - yf) * step.y + texCoord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g;
  fragColor += getLine(tex, ( 1.5 - yf) * step.y + texCoord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g;
  fragColor += getLine(tex, ( 2.5 - yf) * step.y + texCoord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b;
  fragColor += getLine(tex, ( 3.5 - yf) * step.y + texCoord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

  return fragColor;
}

PS_OUTPUT YUV2RGB(VS_OUTPUT In)
{
  PS_OUTPUT OUT;
  float4 YUV = float4(getPixel (YSampler, In.TextureY, g_YStep)
                    , getPixel (USampler, In.TextureU, g_UVStep)
                    , getPixel (VSampler, In.TextureV, g_UVStep)
                    , 1.0);
  OUT.RGBColor = mul(YUV, g_ColorMatrix);
  OUT.RGBColor.a = 1.0;
  return OUT;
}

technique YUV2RGB_T
{
  pass P0
  {
    PixelShader  = compile ps_3_0 YUV2RGB();
    ZEnable = False;
    FillMode = Solid;
    FogEnable = False;
  }
};
