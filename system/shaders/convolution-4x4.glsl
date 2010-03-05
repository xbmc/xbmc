uniform sampler2D img;
uniform float     stepx;
uniform float     stepy;
uniform float     m_stretch;

//nvidia's half is a 16 bit float and can bring some speed improvements
//without affecting quality
#ifndef __GLSL_CG_DATA_TYPES
  #define half float
  #define half3 vec3
  #define half4 vec4
#endif

#if (HAS_FLOAT_TEXTURE)
uniform sampler1D kernelTex;

half4 weight(float pos)
{
  return texture1D(kernelTex, pos);
}
#else
uniform sampler2D kernelTex;

half4 weight(float pos)
{
  //row 0 contains the high byte, row 1 contains the low byte
  return (texture2D(kernelTex, vec2(pos, 0.0)) * 256.0 + texture2D(kernelTex, vec2(pos, 1.0))) / 128.5 - 1.0;
}
#endif

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
  vec2 pos = POSITION(gl_TexCoord[0].xy);

  float xf = fract(pos.x / stepx);
  float yf = fract(pos.y / stepy);

  half4 linetaps   = weight(1.0 - xf);
  half4 columntaps = weight(1.0 - yf);

  //make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
  columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

  float xstart = (-0.5 - xf) * stepx + pos.x;
  vec4 xpos = vec4(xstart, xstart + stepx, xstart + stepx * 2.0, xstart + stepx * 3.0);

  float ystart = (-0.5 - yf) * stepy + pos.y;

  gl_FragColor.rgb =
    line(ystart              , xpos, linetaps) * columntaps.r +
    line(ystart + stepy      , xpos, linetaps) * columntaps.g +
    line(ystart + stepy * 2.0, xpos, linetaps) * columntaps.b +
    line(ystart + stepy * 3.0, xpos, linetaps) * columntaps.a;

  gl_FragColor.a = gl_Color.a;
}

