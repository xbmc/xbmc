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
uniform mat3 m_primMat;
uniform float m_gammaDstInv;
uniform float m_gammaSrc;
uniform float m_toneP1;
uniform vec3 m_coefsDst;
in vec2 m_cordY;
in vec2 m_cordU;
in vec2 m_cordV;
out vec4 fragColor;

vec2 stretch(vec2 pos)
{
#if (XBMC_STRETCH)
  // our transform should map [0..1] to itself, with f(0) = 0, f(1) = 1, f(0.5) = 0.5, and f'(0.5) = b.
  // a simple curve to do this is g(x) = b(x-0.5) + (1-b)2^(n-1)(x-0.5)^n + 0.5
  // where the power preserves sign. n = 2 is the simplest non-linear case (required when b != 1)
  #if(XBMC_texture_rectangle)
    float x = (pos.x * m_step.x) - 0.5;
    return vec2((mix(2.0 * x * abs(x), x, m_stretch) + 0.5) / m_step.x, pos.y);
  #else
    float x = pos.x - 0.5;
    return vec2(mix(2.0 * x * abs(x), x, m_stretch) + 0.5, pos.y);
  #endif
#else
  return pos;
#endif
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
  vec2 pos = coord + m_step * 0.5;
  vec2 f = fract(pos / m_step);

  vec4 linetaps = texture(m_kernelTex, 1.0 - f.x);
  vec4 coltaps = texture(m_kernelTex, 1.0 - f.y);
  linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
  coltaps /= coltaps.r + coltaps.g + coltaps.b + coltaps.a;
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
  vec4 yuv;

#if defined(XBMC_YV12)

  yuv = vec4(filter_0(m_sampY, stretch(m_cordY)),
             texture(m_sampU, stretch(m_cordU)).r,
             texture(m_sampV, stretch(m_cordV)).r,
             1.0);

#elif defined(XBMC_NV12)

  yuv = vec4(filter_0(m_sampY, stretch(m_cordY)),
             texture(m_sampU, stretch(m_cordU)).rg,
             1.0);

#endif

  rgb = m_yuvmat * yuv;
  rgb.a = m_alpha;

#if defined(XBMC_COL_CONVERSION)
  rgb.rgb = pow(max(vec3(0), rgb.rgb), vec3(m_gammaSrc));
  rgb.rgb = max(vec3(0), m_primMat * rgb.rgb);
  rgb.rgb = pow(rgb.rgb, vec3(m_gammaDstInv));

#if defined(XBMC_TONE_MAPPING)
  float luma = dot(rgb.rgb, m_coefsDst);
  rgb.rgb *= tonemap(luma) / luma;
#endif

#endif

  return rgb;
}
