uniform sampler2D img;
uniform float     m_stretch;

vec2 stretch(vec2 pos)
{
  float x = pos.x - 0.5;
  return vec2(mix(x, x * abs(x) * 2.0, m_stretch) + 0.5, pos.y);
}

void main()
{
  gl_FragColor.rgb = texture2D(img, stretch(gl_TexCoord[0].xy)).rgb;
  gl_FragColor.a = gl_Color.a;
}

