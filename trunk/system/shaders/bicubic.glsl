uniform sampler2D img;
uniform float stepx;
uniform float stepy;
uniform sampler2D kernelTex;

vec4 cubicFilter(float xValue, vec4 c0, vec4 c1, vec4 c2, vec4 c3)
{
  vec4 h = texture2D(kernelTex, vec2(xValue, 0.5));
  vec4 r = c0 * h.r;
  r += c1 * h.g;
  r += c2 * h.b;
  r += c3 * h.a;
  return r;
}

void main()
{
  vec2 f = vec2(gl_TexCoord[0].x / stepx , gl_TexCoord[0].y / stepy);
  f = fract(f);
  vec4 t0 = cubicFilter(f.x,
  texture2D(img, gl_TexCoord[0].xy + vec2(-stepx,    -stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(0.0,       -stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(stepx,     -stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(2.0*stepx, -stepy)));

  vec4 t1 = cubicFilter(f.x,
  texture2D(img, gl_TexCoord[0].xy + vec2(-stepx,    0.0)),
  texture2D(img, gl_TexCoord[0].xy + vec2(0.0,       0.0)),
  texture2D(img, gl_TexCoord[0].xy + vec2(stepx,     0.0)),
  texture2D(img, gl_TexCoord[0].xy + vec2(2.0*stepx, 0.0)));

  vec4 t2 = cubicFilter(f.x,
  texture2D(img, gl_TexCoord[0].xy + vec2(-stepx,    stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(0.0,       stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(stepx,     stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(2.0*stepx, stepy)));

  vec4 t3 = cubicFilter(f.x,
  texture2D(img, gl_TexCoord[0].xy + vec2(-stepx,    2.0*stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(0,         2.0*stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(stepx,     2.0*stepy)),
  texture2D(img, gl_TexCoord[0].xy + vec2(2.0*stepx, 2.0*stepy)));

  gl_FragColor = cubicFilter(f.y, t0, t1, t2, t3);   
  gl_FragColor.a = gl_Color.a;
};
