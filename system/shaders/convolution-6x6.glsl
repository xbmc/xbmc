#version 130

uniform sampler2D img;
uniform sampler1D kernelTex;
uniform vec2      stepxy;
uniform float     m_stretch;
out     vec4      gl_FragColor;
in      vec4      gl_TexCoord[];
in      vec4      gl_Color;

#if (HAS_FLOAT_TEXTURE)
vec3 weight(float pos)
{
  return texture(kernelTex, pos).rgb;
}
#else
vec3 weight(float pos)
{
  return texture(kernelTex, pos).rgb * 2.0 - 1.0;
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

vec3 line (vec2 pos, const int yoffset, vec3 linetaps1, vec3 linetaps2)
{
  return
    textureOffset(img, pos, ivec2(-2, yoffset)).rgb * linetaps1.r +
    textureOffset(img, pos, ivec2(-1, yoffset)).rgb * linetaps2.r +
    textureOffset(img, pos, ivec2( 0, yoffset)).rgb * linetaps1.g +
    textureOffset(img, pos, ivec2( 1, yoffset)).rgb * linetaps2.g +
    textureOffset(img, pos, ivec2( 2, yoffset)).rgb * linetaps1.b +
    textureOffset(img, pos, ivec2( 3, yoffset)).rgb * linetaps2.b;
}

void main()
{
  vec2 pos = stretch(gl_TexCoord[0].xy);
  vec2 f = fract(pos / stepxy);

  vec3 linetaps1   = weight((1.0 - f.x) / 2.0);
  vec3 linetaps2   = weight((1.0 - f.x) / 2.0 + 0.5);
  vec3 columntaps1 = weight((1.0 - f.y) / 2.0);
  vec3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  //make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  float sum = linetaps1.r + linetaps1.g + linetaps1.b + linetaps2.r + linetaps2.g + linetaps2.b;
  linetaps1 /= sum;
  linetaps2 /= sum;
  sum = columntaps1.r + columntaps1.g + columntaps1.b + columntaps2.r + columntaps2.g + columntaps2.b;
  columntaps1 /= sum;
  columntaps2 /= sum;

  vec2 xystart = (0.5 - f) * stepxy + pos;

  gl_FragColor.rgb =
   line(xystart, -2, linetaps1, linetaps2) * columntaps1.r +
   line(xystart, -1, linetaps1, linetaps2) * columntaps2.r +
   line(xystart,  0, linetaps1, linetaps2) * columntaps1.g +
   line(xystart,  1, linetaps1, linetaps2) * columntaps2.g +
   line(xystart,  2, linetaps1, linetaps2) * columntaps1.b +
   line(xystart,  3, linetaps1, linetaps2) * columntaps2.b;

  gl_FragColor.a = gl_Color.a;
}

