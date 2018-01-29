#version 100

attribute vec4 m_attrpos;
attribute vec2 m_attrcord;
varying vec2 cord;
uniform mat4 m_proj;
uniform mat4 m_model;

void main ()
{
  mat4 mvp = m_proj * m_model;
  gl_Position = mvp * m_attrpos;
  cord = m_attrcord.xy;
}
