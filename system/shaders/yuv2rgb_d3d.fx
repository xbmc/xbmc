texture g_YTexture;
texture g_UTexture;
texture g_VTexture;

sampler YSampler =
  sampler_state {
    Texture = <g_YTexture>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

sampler USampler = 
  sampler_state {
    Texture = <g_UTexture>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
  };

sampler VSampler = 
  sampler_state
  {
    Texture = <g_VTexture>;
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
  float3 YUV = float3(tex2D (YSampler, In.TextureY).x - (16.0  / 256.0)
                    , tex2D (USampler, In.TextureU).x - (128.0 / 256.0)
                    , tex2D (VSampler, In.TextureV).x - (128.0 / 256.0)); 
  OUT.RGBColor.r = clamp((1.164 * YUV.x + 1.596 * YUV.z),0,255);
  OUT.RGBColor.g = clamp((1.164 * YUV.x - 0.813 * YUV.z - 0.391 * YUV.y), 0,255); 
  OUT.RGBColor.b = clamp((1.164 * YUV.x + 2.018 * YUV.y),0,255);
  OUT.RGBColor.a = 1.0;
  return OUT;
}

technique YUV2RGB_T
{
  pass P0
  {
    PixelShader  = compile ps_2_0 YUV2RGB();
  }
};
