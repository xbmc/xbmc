uniform sampler2D img;
uniform float     stepx;
uniform float     stepy;

//nvidia's half is a 16 bit float and can bring some speed improvements
//without affecting quality
#ifndef __GLSL_CG_DATA_TYPES
  #define half float
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
  return
    pixel(xpos1.r, ypos) * linetaps1.r +
    pixel(xpos1.g, ypos) * linetaps2.r +
    pixel(xpos1.b, ypos) * linetaps1.g +
    pixel(xpos2.r, ypos) * linetaps2.g +
    pixel(xpos2.g, ypos) * linetaps1.b +
    pixel(xpos2.b, ypos) * linetaps2.b; 
}

void main()
{
  float xf = fract(gl_TexCoord[0].x / stepx);
  float yf = fract(gl_TexCoord[0].y / stepy);

  half3 linetaps1   = weight((1.0 - xf) / 2.0);
  half3 linetaps2   = weight((1.0 - xf) / 2.0 + 0.5);
  half3 columntaps1 = weight((1.0 - yf) / 2.0);
  half3 columntaps2 = weight((1.0 - yf) / 2.0 + 0.5);

  //make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  half sum = linetaps1.r + linetaps1.g + linetaps1.b + linetaps2.r + linetaps2.g + linetaps2.b;
  linetaps1 /= sum;
  linetaps2 /= sum;
  sum = columntaps1.r + columntaps1.g + columntaps1.b + columntaps2.r + columntaps2.g + columntaps2.b;
  columntaps1 /= sum;
  columntaps2 /= sum;

  float xstart = (-1.5 - xf) * stepx + gl_TexCoord[0].x;
  vec3 xpos1 = vec3(xstart, xstart + stepx, xstart + stepx * 2.0);
  xstart += stepx * 3.0;
  vec3 xpos2 = vec3(xstart, xstart + stepx, xstart + stepx * 2.0);

  float ystart = (-1.5 - yf) * stepy + gl_TexCoord[0].y;

  gl_FragColor.rgb =
   line(ystart              , xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
   line(ystart + stepy      , xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
   line(ystart + stepy * 2.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
   line(ystart + stepy * 3.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
   line(ystart + stepy * 4.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
   line(ystart + stepy * 5.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

  gl_FragColor.a = gl_Color.a;
}

