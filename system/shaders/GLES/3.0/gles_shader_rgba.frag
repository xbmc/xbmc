#version 300 es

precision mediump float;
uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
in vec4 m_cord0;
uniform int m_method;
uniform float m_brightness;
uniform float m_contrast;
out vec4 fragColor;

void main ()
{
  vec4 rgb;

  rgb = texture(m_samp0, m_cord0.xy);
  rgb *= m_contrast;
  rgb += m_brightness;

  fragColor = rgb;
}
