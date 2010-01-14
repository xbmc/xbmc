uniform sampler2D img;
uniform float     stepx;
uniform float     stepy;
uniform sampler2D kernelTex;

vec3 weight(float pos)
{
  return texture2D(kernelTex, vec2(pos, 0.5)).rgb;
}

vec3 pixel(float xpos, float ypos)
{
  return texture2D(img, vec2(xpos, ypos)).rgb;
}

vec3 line (float ypos, vec3 xpos1, vec3 xpos2, vec3 linetaps1, vec3 linetaps2)
{
  vec3  pixels;

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

  vec3 linetaps1   = weight((1.0 - xf) / 2.0);
  vec3 linetaps2   = weight((1.0 - xf) / 2.0 + 0.5);
  vec3 columntaps1 = weight((1.0 - yf) / 2.0);
  vec3 columntaps2 = weight((1.0 - yf) / 2.0 + 0.5);

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

