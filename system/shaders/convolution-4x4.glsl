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

vec4 line (float ypos, float xf, vec4 linetaps)
{
  float xpos;
  vec4  pixels;

  xpos    = -1.0 - xf;
  pixels  = pixel(xpos, ypos) * linetaps.r;
  xpos    =  0.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps.g;
  xpos    =  1.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps.b;
  xpos    =  2.0 - xf;
  pixels += pixel(xpos, ypos) * linetaps.a;

  return pixels;
}

void main()
{
  float xf = fract(gl_TexCoord[0].x / stepx);
  float yf = fract(gl_TexCoord[0].y / stepy);
  float ypos;

  vec4 linetaps   = weight(1.0 - xf);
  vec4 columntaps = weight(1.0 - yf);

  ypos = -1.0 - yf;
  gl_FragColor  = line(ypos, xf, linetaps) * columntaps.r;
  ypos =  0.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps) * columntaps.g;
  ypos =  1.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps) * columntaps.b;
  ypos =  2.0 - yf;
  gl_FragColor += line(ypos, xf, linetaps) * columntaps.a;

  gl_FragColor.a = gl_Color.a;
}

