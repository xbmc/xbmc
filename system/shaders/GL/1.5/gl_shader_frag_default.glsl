#version 150

uniform vec4 m_unicol;
out vec4 fragColor;

// SM_DEFAULT shader
void main ()
{
  fragColor = m_unicol;
}
