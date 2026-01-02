#version 150

in vec4 m_attrpos;
uniform mat4 m_matrix;

void main()
{
  gl_Position = m_matrix * vec4(m_attrpos.xy, 0.0, 1.0);
}
