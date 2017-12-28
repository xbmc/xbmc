#version 400

#if(XBMC_texture_rectangle)
# define texture2D texture2DRect
# define sampler2D sampler2DRect
#endif

uniform sampler2D m_sampY;
uniform sampler2D m_sampU;
uniform sampler2D m_sampV;
uniform vec2 m_step;
uniform mat4 m_yuvmat;
uniform float m_stretch;
uniform float m_alpha;
uniform sampler1D m_kernelTex;
in vec2 m_cordY;
in vec2 m_cordU;
in vec2 m_cordV;
out vec4 fragColor;

vec2 stretch(vec2 pos)
{
  float x = pos.x - 0.5;
  return vec2(mix(x * abs(x) * 2.0, x, m_stretch) + 0.5, pos.y);
}

vec4[4] load4x4_0(sampler2D sampler, vec2 pos)
{
  vec4[4] tex4x4;
  vec4 tex2x2 = textureGather(sampler, pos, 0);
  tex4x4[0].xy = tex2x2.wz;
  tex4x4[1].xy = tex2x2.xy;
  tex2x2 = textureGatherOffset(sampler, pos, ivec2(2,0), 0);
  tex4x4[0].zw = tex2x2.wz;
  tex4x4[1].zw = tex2x2.xy;
  tex2x2 = textureGatherOffset(sampler, pos, ivec2(0,2), 0);
  tex4x4[2].xy = tex2x2.wz;
  tex4x4[3].xy = tex2x2.xy;
  tex2x2 = textureGatherOffset(sampler, pos, ivec2(2,2), 0);
  tex4x4[2].zw = tex2x2.wz;
  tex4x4[3].zw = tex2x2.xy;
  return tex4x4;
}

float filter_0(sampler2D sampler, vec2 coord)
{
#if (XBMC_STRETCH)
  vec2 pos = stretch(coord) + m_step * 0.5;
#else
  vec2 pos = coord + m_step * 0.5;
#endif

  vec2 f = fract(pos / m_step);

  vec4 linetaps = texture(m_kernelTex, 1.0 - f.x);
  vec4 coltaps = texture(m_kernelTex, 1.0 - f.y);
  mat4 conv;
  conv[0] = linetaps * coltaps.x;
  conv[1] = linetaps * coltaps.y;
  conv[2] = linetaps * coltaps.z;
  conv[3] = linetaps * coltaps.w;

  vec2 startPos = (-1.0 - f) * m_step + pos;
  vec4[4] tex4x4 = load4x4_0(sampler, startPos);
  vec4 imageLine0 = tex4x4[0];
  vec4 imageLine1 = tex4x4[1];
  vec4 imageLine2 = tex4x4[2];
  vec4 imageLine3 = tex4x4[3];

  return dot(imageLine0, conv[0]) +
         dot(imageLine1, conv[1]) +
         dot(imageLine2, conv[2]) +
         dot(imageLine3, conv[3]);
}

vec4 process()
{
  vec4 rgb;
#if defined(XBMC_YV12)

  vec4 yuv = vec4(filter_0(m_sampY, m_cordY),
                  texture(m_sampU, m_cordU).r,
                  texture(m_sampV, m_cordV).r,
                  1.0);

  rgb = m_yuvmat * yuv;
  rgb.a = m_alpha;

#elif defined(XBMC_NV12)

  vec4 yuv = vec4(filter_0(m_sampY, m_cordY),
                  texture(m_sampU, m_cordU).rg,
                  1.0);
  rgb = m_yuvmat * yuv;
  rgb.a = m_alpha;
#endif

  return rgb;
}
