texture g_YTexture;
texture g_UTexture;
texture g_VTexture;
float4x4 g_ColorMatrix;

sampler YSampler =
  sampler_state {
    Texture = <g_YTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

sampler USampler = 
  sampler_state {
    Texture = <g_UTexture>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

sampler VSampler = 
  sampler_state
  {
    Texture = <g_VTexture>;
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
  float2 TextureU   : TEXCOORD0;
  float2 TextureV   : TEXCOORD0;
};

struct PS_OUTPUT
{
  float4 RGBColor : COLOR0;
};

PS_OUTPUT YUV2RGB( VS_OUTPUT In)
{
  PS_OUTPUT OUT;
  float4 YUV = float4(tex2D (YSampler, In.TextureY).x
                    , tex2D (USampler, In.TextureU).x
                    , tex2D (VSampler, In.TextureV).x
                    , 1.0);
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
