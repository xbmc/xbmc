#version 150

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
uniform mat3 m_primMat;
uniform float m_gammaDstInv;
uniform float m_gammaSrc;
uniform float m_toneP1;
uniform float m_luminance;
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

vec4 process()
{
  vec4 rgb;
  vec4 yuv;

#if defined(XBMC_YV12)

  yuv.rgba = vec4( texture(m_sampY, stretch(m_cordY)).r
                 , texture(m_sampU, stretch(m_cordU)).r
                 , texture(m_sampV, stretch(m_cordV)).r
                 , 1.0 );

#elif defined(XBMC_NV12)

  yuv.rgba = vec4( texture(m_sampY, stretch(m_cordY)).r
                 , texture(m_sampU, stretch(m_cordU)).rg
                 , 1.0 );

#elif defined(XBMC_YUY2) || defined(XBMC_UYVY)

  vec2 stepxy = m_step;
  vec2 pos    = stretch(m_cordY);
  pos         = vec2(pos.x - stepxy.x * 0.25, pos.y);
  vec2 f      = fract(pos / stepxy);

  //y axis will be correctly interpolated by opengl
  //x axis will not, so we grab two pixels at the center of two columns and interpolate ourselves
  vec4 c1 = texture(m_sampY, vec2(pos.x + (0.5 - f.x) * stepxy.x, pos.y));
  vec4 c2 = texture(m_sampY, vec2(pos.x + (1.5 - f.x) * stepxy.x, pos.y));

  /* each pixel has two Y subpixels and one UV subpixel
     YUV  Y  YUV
     check if we're left or right of the middle Y subpixel and interpolate accordingly*/
#ifdef XBMC_YUY2 //BGRA = YUYV
  float leftY   = mix(c1.b, c1.r, f.x * 2.0);
  float rightY  = mix(c1.r, c2.b, f.x * 2.0 - 1.0);
  vec2  outUV   = mix(c1.ga, c2.ga, f.x);
#else //BGRA = UYVY
  float leftY   = mix(c1.g, c1.a, f.x * 2.0);
  float rightY  = mix(c1.a, c2.g, f.x * 2.0 - 1.0);
  vec2  outUV   = mix(c1.br, c2.br, f.x);
#endif //XBMC_YUY2

  float outY = mix(leftY, rightY, step(0.5, f.x));

  yuv = vec4(outY, outUV, 1.0);

#endif

  rgb = m_yuvmat * yuv;
  rgb.a = m_alpha;

#if defined(XBMC_COL_CONVERSION)
  rgb.rgb = pow(max(vec3(0), rgb.rgb), vec3(m_gammaSrc));
  rgb.rgb = max(vec3(0), m_primMat * rgb.rgb);
  rgb.rgb = pow(rgb.rgb, vec3(m_gammaDstInv));

#if defined(KODI_TONE_MAPPING_REINHARD)
  float luma = dot(rgb.rgb, m_coefsDst);
  rgb.rgb *= reinhard(luma) / luma;

#elif defined(KODI_TONE_MAPPING_ACES)
  rgb.rgb = inversePQ(rgb.rgb);
  rgb.rgb *= (10000.0 / m_luminance) * (2.0 / m_toneP1);
  rgb.rgb = aces(rgb.rgb);
  rgb.rgb *= (1.24 / m_toneP1);
  rgb.rgb = pow(rgb.rgb, vec3(0.27));

#elif defined(KODI_TONE_MAPPING_HABLE)
  rgb.rgb = inversePQ(rgb.rgb);
  rgb.rgb *= m_toneP1;
  float wp = m_luminance / 100.0;
  rgb.rgb = hable(rgb.rgb * wp) / hable(vec3(wp));
  rgb.rgb = pow(rgb.rgb, vec3(1.0 / 2.2));
#endif

#endif

  return rgb;
}
