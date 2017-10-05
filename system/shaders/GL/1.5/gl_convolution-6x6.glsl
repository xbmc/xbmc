#version 150

uniform sampler2D img;
uniform vec2 stepxy;
uniform float m_stretch;
uniform float m_alpha;
in vec2 m_cord;
out vec4 fragColor;

#if (USE1DTEXTURE)
  uniform sampler1D kernelTex;
#else
  uniform sampler2D kernelTex;
#endif

//nvidia's half is a 16 bit float and can bring some speed improvements
//without affecting quality
#ifndef __GLSL_CG_DATA_TYPES
  #define half float
  #define half3 vec3
  #define half4 vec4
#endif

half3 weight(float pos)
{
#if (HAS_FLOAT_TEXTURE)
  #if (USE1DTEXTURE)
    return texture(kernelTex, pos).rgb;
  #else
    return texture(kernelTex, vec2(pos, 0.5)).rgb;
  #endif
#else
  #if (USE1DTEXTURE)
    return texture(kernelTex, pos).rgb * 2.0 - 1.0;
  #else
    return texture(kernelTex, vec2(pos, 0.5)).rgb * 2.0 - 1.0;
  #endif
#endif
}

vec2 stretch(vec2 pos)
{
#if (XBMC_STRETCH)
  // our transform should map [0..1] to itself, with f(0) = 0, f(1) = 1, f(0.5) = 0.5, and f'(0.5) = b.
  // a simple curve to do this is g(x) = b(x-0.5) + (1-b)2^(n-1)(x-0.5)^n + 0.5
  // where the power preserves sign. n = 2 is the simplest non-linear case (required when b != 1)
  float x = pos.x - 0.5;
  return vec2(mix(x * abs(x) * 2.0, x, m_stretch) + 0.5, pos.y);
#else
  return pos;
#endif
}

half3 pixel(float xpos, float ypos)
{
  return texture(img, vec2(xpos, ypos)).rgb;
}

half3 line (float ypos, vec3 xpos1, vec3 xpos2, half3 linetaps1, half3 linetaps2)
{
  return
    pixel(xpos1.r, ypos) * linetaps1.r +
    pixel(xpos1.g, ypos) * linetaps2.r +
    pixel(xpos1.b, ypos) * linetaps1.g +
    pixel(xpos2.r, ypos) * linetaps2.g +
    pixel(xpos2.g, ypos) * linetaps1.b +
    pixel(xpos2.b, ypos) * linetaps2.b;
}

vec4 process()
{
  vec4 rgb;
  vec2 pos = stretch(m_cord) + stepxy * 0.5;
  vec2 f = fract(pos / stepxy);

  half3 linetaps1   = weight((1.0 - f.x) / 2.0);
  half3 linetaps2   = weight((1.0 - f.x) / 2.0 + 0.5);
  half3 columntaps1 = weight((1.0 - f.y) / 2.0);
  half3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  //make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  half sum = linetaps1.r + linetaps1.g + linetaps1.b + linetaps2.r + linetaps2.g + linetaps2.b;
  linetaps1 /= sum;
  linetaps2 /= sum;
  sum = columntaps1.r + columntaps1.g + columntaps1.b + columntaps2.r + columntaps2.g + columntaps2.b;
  columntaps1 /= sum;
  columntaps2 /= sum;

  vec2 xystart = (-2.5 - f) * stepxy + pos;
  vec3 xpos1 = vec3(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0);
  vec3 xpos2 = vec3(xystart.x + stepxy.x * 3.0, xystart.x + stepxy.x * 4.0, xystart.x + stepxy.x * 5.0);

  rgb.rgb =
   line(xystart.y                 , xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
   line(xystart.y + stepxy.y      , xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
   line(xystart.y + stepxy.y * 2.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
   line(xystart.y + stepxy.y * 3.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
   line(xystart.y + stepxy.y * 4.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
   line(xystart.y + stepxy.y * 5.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

  rgb.a = m_alpha;

  return rgb;
}
