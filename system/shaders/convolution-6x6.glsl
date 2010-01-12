uniform sampler2D img;
uniform float     stepx;
uniform float     stepy;
uniform sampler2D kernelTex;

vec4 weight(float pos)
{
  return texture2D(kernelTex, vec2(pos, 0.5));
}

vec4 pixel(float xpos, float ypos)
{
  return texture2D(img, vec2(gl_TexCoord[0].x + xpos * stepx + stepx / 2.0, gl_TexCoord[0].y + ypos * stepy + stepy / 2.0));
}

vec4 line (float ypos, float xf, vec4 linetaps1, vec4 linetaps2)
{
  float xpos;
  vec4  pixels;

  xpos    = -2.0 - xf;
  pixels  = pixel(xpos, ypos) * linetaps1.r;
  xpos    = -1.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps2.r;
  xpos    =  0.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps1.g;
  xpos    =  1.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps2.g;
  xpos    =  2.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps1.b;
  xpos    =  3.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps2.b;

  return pixels;
}

void main()
{
  float xf = fract(gl_TexCoord[0].x / stepx);
  float yf = fract(gl_TexCoord[0].y / stepy);
  float ypos;

  vec4 linetaps1   = weight((1.0 - xf) / 2.0);
  vec4 linetaps2   = weight((1.0 - xf) / 2.0 + 0.5);
  vec4 columntaps1 = weight((1.0 - yf) / 2.0);
  vec4 columntaps2 = weight((1.0 - yf) / 2.0 + 0.5);

  ypos = -2.0 - yf;
  gl_FragColor  = line(ypos, xf, linetaps1, linetaps2) * columntaps1.r;
  ypos = -1.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps1, linetaps2) * columntaps2.r;
  ypos =  0.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps1, linetaps2) * columntaps1.g;
  ypos =  1.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps1, linetaps2) * columntaps2.g;
  ypos =  2.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps1, linetaps2) * columntaps1.b;
  ypos =  3.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps1, linetaps2) * columntaps2.b;

  gl_FragColor.a = gl_Color.a;
}

