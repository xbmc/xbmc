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

#if (XBMC_STRETCH)
  #define POSITION(pos) stretch(pos)
  vec2 stretch(vec2 pos)
  {
    float x = pos.x - 0.5;
    return vec2(mix(x, x * abs(x) * 2.0, m_stretch) + 0.5, pos.y);
  }
#else
  #define POSITION(pos) (pos)
#endif

void main()
{
  vec4 yuv, rgb;
  yuv.rgba = vec4( texture2D(m_sampY, POSITION(m_cordY)).r
                 , texture2D(m_sampU, POSITION(m_cordU)).r
                 , texture2D(m_sampV, POSITION(m_cordV)).a
                 , 1.0 );

  rgb   = m_yuvmat * yuv;
  rgb.a = gl_Color.a;
  gl_FragColor = rgb;
}
