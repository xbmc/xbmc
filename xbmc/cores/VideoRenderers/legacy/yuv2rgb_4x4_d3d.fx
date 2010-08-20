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

float4 weight(float pos)
{
  return tex1D(KernelSampler, pos);
}

float pixel(sampler tex, float xpos, float ypos)
{
  return tex2D(tex, float2(xpos, ypos)).r;
}

float getLine(sampler tex, float ypos, float4 xpos, float4 linetaps)
{
  float  pixels;

  pixels  = pixel(tex, xpos.r, ypos) * linetaps.r;
  pixels += pixel(tex, xpos.g, ypos) * linetaps.g;
  pixels += pixel(tex, xpos.b, ypos) * linetaps.b;
  pixels += pixel(tex, xpos.a, ypos) * linetaps.a;

  return pixels;
}

float getPixel(sampler tex, float2 texCoord, float2 step)
{
  float xf = frac(texCoord.x / step.x);
  float yf = frac(texCoord.y / step.y);

  float4 linetaps   = weight(1.0 - xf);
  float4 columntaps = weight(1.0 - yf);

  float4 xpos = float4(
      (-0.5 - xf) * step.x + texCoord.x,
      ( 0.5 - xf) * step.x + texCoord.x,
      ( 1.5 - xf) * step.x + texCoord.x,
      ( 2.5 - xf) * step.x + texCoord.x);

  float fragColor;
  fragColor  = getLine(tex, (-0.5 - yf) * step.y + texCoord.y, xpos, linetaps) * columntaps.r;
  fragColor += getLine(tex, ( 0.5 - yf) * step.y + texCoord.y, xpos, linetaps) * columntaps.g;
  fragColor += getLine(tex, ( 1.5 - yf) * step.y + texCoord.y, xpos, linetaps) * columntaps.b;
  fragColor += getLine(tex, ( 2.5 - yf) * step.y + texCoord.y, xpos, linetaps) * columntaps.a;
  
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
