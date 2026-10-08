#version 100

attribute vec4 m_attrpos;
attribute vec2 m_attrcord;
varying vec2 cord;
uniform mat4 m_proj;
uniform mat4 m_model;
uniform mat4 m_quad;
uniform vec4 m_texRect;

void main ()
{
  mat4 mvp = m_proj * m_model;
  gl_Position = mvp * (m_quad * m_attrpos);
  cord = m_texRect.xy + m_attrcord.xy * m_texRect.zw;
}
