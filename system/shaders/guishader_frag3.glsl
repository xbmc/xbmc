precision mediump float;
uniform sampler2D m_samp0;
varying vec4      m_cord0;
varying vec4      m_colour;

void main ()
{
  gl_FragColor.r   = m_colour.r;
  gl_FragColor.g   = m_colour.g;
  gl_FragColor.b   = m_colour.b;
  gl_FragColor.a   = texture2D(m_samp0, m_cord0.xy).a;
}
