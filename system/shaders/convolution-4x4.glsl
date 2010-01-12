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
  return texture2D(img, vec2(gl_TexCoord[0].x + xpos * stepx, gl_TexCoord[0].y + ypos * stepy));
}

vec4 line (float ypos, float xf, vec4 linetaps)
{
  vec4  pixels;

  pixels  = pixel(-0.5 - xf, ypos) * linetaps.r;
  pixels += pixel( 0.5 - xf, ypos) * linetaps.g;
  pixels += pixel( 1.5 - xf, ypos) * linetaps.b;
  pixels += pixel( 2.5 - xf, ypos) * linetaps.a;

  return pixels;
}

void main()
{
  float xf = fract(gl_TexCoord[0].x / stepx);
  float yf = fract(gl_TexCoord[0].y / stepy);

  vec4 linetaps   = weight(1.0 - xf);
  vec4 columntaps = weight(1.0 - yf);

  gl_FragColor  = line(-0.5 - yf, xf, linetaps) * columntaps.r;
  gl_FragColor += line( 0.5 - yf, xf, linetaps) * columntaps.g;
  gl_FragColor += line( 1.5 - yf, xf, linetaps) * columntaps.b;
  gl_FragColor += line( 2.5 - yf, xf, linetaps) * columntaps.a;

  gl_FragColor.a = gl_Color.a;
}

