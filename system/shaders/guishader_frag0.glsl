precision mediump float;
uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
varying vec4      m_cord0;
varying vec4      m_cord1;
varying vec4      m_colour;
uniform int       m_method;

void main ()
{
  gl_FragColor = m_colour;
}
