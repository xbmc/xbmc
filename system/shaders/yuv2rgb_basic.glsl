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

uniform vec2      m_step;

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
#ifndef XBMC_YUY2

  vec4 yuv, rgb;
  yuv.rgba = vec4( texture2D(m_sampY, stretch(m_cordY)).r
                 , texture2D(m_sampU, stretch(m_cordU)).g
                 , texture2D(m_sampV, stretch(m_cordV)).a
                 , 1.0 );

  rgb   = m_yuvmat * yuv;
  rgb.a = gl_Color.a;
  gl_FragColor = rgb;

#else

#if(XBMC_texture_rectangle)
  vec2 stepxy = vec2(1.0, 1.0);
  vec2 pos    = stretch(vec2(m_cordY.x * 0.5 - 0.25, m_cordY.y));
  vec2 f      = fract(pos);
#else
  vec2 stepxy = vec2(m_step.x * 2.0, m_step.y);
  vec2 pos    = stretch(vec2(m_cordY.x - stepxy.x * 0.25, m_cordY.y));
  vec2 f      = fract(pos / stepxy);
#endif

  //y axis will be correctly interpolated by opengl
  //x axis will not, so we grab two pixels at the center of two columns and interpolate ourselves
  vec4 c1 = texture2D(m_sampY, vec2(pos.x + (-0.5 - f.x) * stepxy.x, pos.y));
  vec4 c2 = texture2D(m_sampY, vec2(pos.x + ( 0.5 - f.x) * stepxy.x, pos.y));

  /* each pixel has two Y subpixels and one UV subpixel
     YUV  Y  YUV
     check if we're left or right of the middle Y subpixel and interpolate accordingly*/
  float leftY   = mix(c1.b, c1.r, f.x * 2.0);
  float rightY  = mix(c1.r, c2.b, f.x * 2.0 - 1.0);
  float outY    = mix(leftY, rightY, step(0.5, f.x));

  //interpolate UV
  vec2  outUV   = mix(c1.ga, c2.ga, f.x);

  vec4  yuv     = vec4(outY, outUV, 1.0);
  vec4  rgb     = m_yuvmat * yuv;

  gl_FragColor   = rgb;
  gl_FragColor.a = gl_Color.a;

#endif
}
