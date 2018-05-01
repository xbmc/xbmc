#version 150

uniform sampler2D img;
uniform float m_stretch;
uniform float m_alpha;
in vec2 m_cord;
out vec4 fragColor;

vec2 stretch(vec2 pos)
{
  // our transform should map [0..1] to itself, with f(0) = 0, f(1) = 1, f(0.5) = 0.5, and f'(0.5) = b.
  // a simple curve to do this is g(x) = b(x-0.5) + (1-b)2^(n-1)(x-0.5)^n + 0.5
  // where the power preserves sign. n = 2 is the simplest non-linear case (required when b != 1)
  float x = pos.x - 0.5;
  return vec2(mix(x * abs(x) * 2.0, x, m_stretch) + 0.5, pos.y);
}

void main()
{
  fragColor.rgb = texture(img, stretch(m_cord)).rgb;
  fragColor.a = m_alpha;
}
