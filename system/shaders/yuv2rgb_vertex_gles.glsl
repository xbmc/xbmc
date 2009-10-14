attribute vec4 m_attrpos;
attribute vec2 m_attrcordY;
attribute vec2 m_attrcordU;
attribute vec2 m_attrcordV;
varying vec2 m_cordY;
varying vec2 m_cordU;
varying vec2 m_cordV;
uniform mat4 m_proj;
uniform mat4 m_model;

void main ()
{
  mat4 mvp    = m_proj * m_model;
  gl_Position = mvp * m_attrpos;
  m_cordY     = m_attrcordY;
  m_cordU     = m_attrcordU;
  m_cordV     = m_attrcordV;
}
