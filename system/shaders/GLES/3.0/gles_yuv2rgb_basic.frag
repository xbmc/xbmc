#version 300 es

precision mediump float;

uniform sampler2D m_sampY;
uniform sampler2D m_sampU;
uniform sampler2D m_sampV;
in vec2 m_cordY;
in vec2 m_cordU;
in vec2 m_cordV;
uniform vec2 m_step;
uniform mat4 m_yuvmat;
uniform mat3 m_primMat;
uniform float m_gammaDstInv;
uniform float m_gammaSrc;
uniform float m_toneP1;
uniform vec3 m_coefsDst;
uniform float m_alpha;
out vec4 fragColor;

void main()
{
  vec4 rgb;
  vec4 yuv;

#if defined(XBMC_YV12)

  yuv = vec4(texture(m_sampY, m_cordY).r,
             texture(m_sampU, m_cordU).g,
             texture(m_sampV, m_cordV).a,
             1.0);

#elif defined(XBMC_NV12_RRG)

  yuv = vec4(texture(m_sampY, m_cordY).r,
             texture(m_sampU, m_cordU).r,
             texture(m_sampV, m_cordV).g,
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

  fragColor = rgb;
}

