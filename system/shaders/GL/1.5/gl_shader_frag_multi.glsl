#version 150

uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
in vec4 m_cord0;
in vec4 m_cord1;
out vec4 fragColor;

// SM_MULTI shader
void main ()
{
  fragColor.rgba = (texture(m_samp0, m_cord0.xy) * texture(m_samp1, m_cord1.xy)).rgba;
}
