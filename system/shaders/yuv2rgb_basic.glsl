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


void main()
{
  vec4 yuv, rgb;
  yuv.rgba = vec4( texture2D(m_sampY, m_cordY).r
                 , texture2D(m_sampU, m_cordU).r
                 , texture2D(m_sampV, m_cordV).a
                 , 1.0 );

  rgb   = m_yuvmat * yuv;
  rgb.a = gl_Color.a;
  gl_FragColor = rgb;
}
