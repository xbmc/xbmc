uniform sampler2D img;
uniform sampler1D kernelTex;
uniform vec2      stepxy;
uniform float     m_stretch;

//nvidia's half is a 16 bit float and can bring some speed improvements
//without affecting quality
#ifndef __GLSL_CG_DATA_TYPES
  #define half float
  #define half3 vec3
  #define half4 vec4
#endif

#if (HAS_FLOAT_TEXTURE)
half4 weight(float pos)
{
  return texture1D(kernelTex, pos);
}
#else
half4 weight(float pos)
{
  return texture1D(kernelTex, pos) * 2.0 - 1.0;
}
#endif

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
  return texture2D(img, vec2(xpos, ypos)).rgb;
}

half3 line (float ypos, vec4 xpos, half4 linetaps)
{
  return
    pixel(xpos.r, ypos) * linetaps.r +
    pixel(xpos.g, ypos) * linetaps.g +
    pixel(xpos.b, ypos) * linetaps.b +
    pixel(xpos.a, ypos) * linetaps.a;
}

void main()
{
  vec2 pos = stretch(gl_TexCoord[0].xy);
  vec2 f = fract(pos / stepxy);

  half4 linetaps   = weight(1.0 - f.x);
  half4 columntaps = weight(1.0 - f.y);

  //make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
  columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

  vec2 xystart = (-0.5 - f) * stepxy + pos;
  vec4 xpos = vec4(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0, xystart.x + stepxy.x * 3.0);

  gl_FragColor.rgb =
    line(xystart.y                 , xpos, linetaps) * columntaps.r +
    line(xystart.y + stepxy.y      , xpos, linetaps) * columntaps.g +
    line(xystart.y + stepxy.y * 2.0, xpos, linetaps) * columntaps.b +
    line(xystart.y + stepxy.y * 3.0, xpos, linetaps) * columntaps.a;

  gl_FragColor.a = gl_Color.a;
}

