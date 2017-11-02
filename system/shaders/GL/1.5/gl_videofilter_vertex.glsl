#version 150

in vec4 m_attrpos;
in vec2 m_attrcord;
out vec2 m_cord;
uniform mat4 m_proj;
uniform mat4 m_model;

void main ()
{
  mat4 mvp  = m_proj * m_model;
  gl_Position = mvp * m_attrpos;
  m_cord = m_attrcord;
}
