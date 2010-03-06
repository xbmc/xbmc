#if(XBMC_texture_rectangle)
# extension GL_ARB_texture_rectangle : enable
# define texture2D texture2DRect
# define sampler2D sampler2DRect
#endif

uniform sampler2D m_sampY;
uniform sampler2D m_sampU;
uniform sampler2D m_sampV;
varying vec2      m_cordY;
varying vec2      m_cordU;
varying vec2      m_cordV;

uniform mat4      m_yuvmat;

uniform float     m_stretch;

vec2 stretch(vec2 pos)
{
#if (XBMC_STRETCH)
  // our transform should map [0..1] to itself, with f(0) = 0, f(1) = 1, f(0.5) = 0.5, and f'(0.5) = b.
  // a simple curve to do this is g(x) = b(x-0.5) + (1-b)2^(n-1)(x-0.5)^n + 0.5
  // where the power preserves sign. n = 2 is the simplest non-linear case (required when b != 1)
  float x = pos.x - 0.5;
  return vec2(mix(2.0 * x * abs(x), x, m_stretch) + 0.5, pos.y);
#else
  return pos;
#endif
}

void main()
{
  vec4 yuv, rgb;
  yuv.rgba = vec4( texture2D(m_sampY, stretch(m_cordY)).r
                 , texture2D(m_sampU, stretch(m_cordU)).r
                 , texture2D(m_sampV, stretch(m_cordV)).a
                 , 1.0 );

  rgb   = m_yuvmat * yuv;
  rgb.a = gl_Color.a;
  gl_FragColor = rgb;
}
