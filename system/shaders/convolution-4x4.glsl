#version 130

uniform sampler2D img;
uniform sampler1D kernelTex;
uniform vec2      stepxy;
uniform float     m_stretch;
out     vec4      gl_FragColor;
in      vec4      gl_TexCoord[];
in      vec4      gl_Color;

#if (HAS_FLOAT_TEXTURE)
vec4 weight(float pos)
{
  return texture(kernelTex, pos);
}
#else
vec4 weight(float pos)
{
  return texture(kernelTex, pos) * 2.0 - 1.0;
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

vec3 line (vec2 pos, const int yoffset, vec4 linetaps)
{
  return
    textureOffset(img, pos, ivec2(-1, yoffset)).rgb * linetaps.r +
    textureOffset(img, pos, ivec2( 0, yoffset)).rgb * linetaps.g +
    textureOffset(img, pos, ivec2( 1, yoffset)).rgb * linetaps.b +
    textureOffset(img, pos, ivec2( 2, yoffset)).rgb * linetaps.a;
}

void main()
{
  vec2 pos = stretch(gl_TexCoord[0].xy);
  vec2 f = fract(pos / stepxy);

  vec4 linetaps   = weight(1.0 - f.x);
  vec4 columntaps = weight(1.0 - f.y);

  //make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
  columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

  vec2 xystart = (0.5 - f) * stepxy + pos;

  gl_FragColor.rgb =
    line(xystart, -1, linetaps) * columntaps.r +
    line(xystart,  0, linetaps) * columntaps.g +
    line(xystart,  1, linetaps) * columntaps.b +
    line(xystart,  2, linetaps) * columntaps.a;

  gl_FragColor.a = gl_Color.a;
}

