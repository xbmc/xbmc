#version 150

in vec4 m_attrpos;
uniform mat4 m_proj;
uniform mat4 m_model;

void main ()
{
  mat4 mvp    = m_proj * m_model;
  gl_Position = mvp * m_attrpos;
  gl_Position.z = -1. * gl_Position.w;
}
