#version 300 es

in vec4 m_attrpos;
in vec2 m_attrcordY;
in vec2 m_attrcordU;
in vec2 m_attrcordV;
out vec2 m_cordY;
out vec2 m_cordU;
out vec2 m_cordV;
uniform mat4 m_proj;
uniform mat4 m_model;

void main ()
{
  mat4 mvp = m_proj * m_model;
  gl_Position = mvp * m_attrpos;
  m_cordY = m_attrcordY;
  m_cordU = m_attrcordU;
  m_cordV = m_attrcordV;
}
