uniform sampler2D img;
uniform float     stepx;
uniform float     stepy;

//nvidia's half is a 16 bit float and can bring some speed improvements
//without affecting quality
#ifndef __GLSL_CG_DATA_TYPES
  #define half3 vec3
  #define half4 vec4
#endif

#if (HAS_FLOAT_TEXTURE)
uniform sampler1D kernelTex;

half3 weight(float pos)
{
  return texture1D(kernelTex, pos).rgb;
}
#else
uniform sampler2D kernelTex;

half3 weight(float pos)
{
  //row 0 contains the high byte, row 1 contains the low byte
  return ((texture2D(kernelTex, vec2(pos, 0.0)) * 256.0 + texture2D(kernelTex, vec2(pos, 1.0)))).rgb / 128.5 - 1.0;
}
#endif

half3 pixel(float xpos, float ypos)
{
  return texture2D(img, vec2(xpos, ypos)).rgb;
}

half3 line (float ypos, vec3 xpos1, vec3 xpos2, half3 linetaps1, half3 linetaps2)
{
  half3  pixels;

  pixels  = pixel(xpos1.r, ypos) * linetaps1.r;
  pixels += pixel(xpos1.g, ypos) * linetaps2.r;
  pixels += pixel(xpos1.b, ypos) * linetaps1.g;
  pixels += pixel(xpos2.r, ypos) * linetaps2.g;
  pixels += pixel(xpos2.g, ypos) * linetaps1.b;
  pixels += pixel(xpos2.b, ypos) * linetaps2.b;

  return pixels;
}

void main()
{
  float xf = fract(gl_TexCoord[0].x / stepx);
  float yf = fract(gl_TexCoord[0].y / stepy);

  half3 linetaps1   = weight((1.0 - xf) / 2.0);
  half3 linetaps2   = weight((1.0 - xf) / 2.0 + 0.5);
  half3 columntaps1 = weight((1.0 - yf) / 2.0);
  half3 columntaps2 = weight((1.0 - yf) / 2.0 + 0.5);

  vec3 xpos1 = vec3(
      (-1.5 - xf) * stepx + gl_TexCoord[0].x,
      (-0.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 0.5 - xf) * stepx + gl_TexCoord[0].x);
  vec3 xpos2 = vec3(
      ( 1.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 2.5 - xf) * stepx + gl_TexCoord[0].x,
      ( 3.5 - xf) * stepx + gl_TexCoord[0].x);

  gl_FragColor.rgb  = line((-1.5 - yf) * stepy + gl_TexCoord[0].y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r;
  gl_FragColor.rgb += line((-0.5 - yf) * stepy + gl_TexCoord[0].y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r;
  gl_FragColor.rgb += line(( 0.5 - yf) * stepy + gl_TexCoord[0].y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g;
  gl_FragColor.rgb += line(( 1.5 - yf) * stepy + gl_TexCoord[0].y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g;
  gl_FragColor.rgb += line(( 2.5 - yf) * stepy + gl_TexCoord[0].y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b;
  gl_FragColor.rgb += line(( 3.5 - yf) * stepy + gl_TexCoord[0].y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

  gl_FragColor.a = gl_Color.a;
}

