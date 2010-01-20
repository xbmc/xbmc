precision mediump float;
uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
varying vec4      m_cord0;
varying vec4      m_cord1;
varying vec4      m_colour;
uniform int       m_method;

void main ()
{
  if (m_method == 3) /*SM_FONTS*/
  {
    gl_FragColor   = m_colour;
    gl_FragColor.a = texture2D(m_samp0, m_cord0.xy).a;
  }
  else if (m_method == 1) /*SM_TEXTURE*/
  {
    gl_FragColor.rgba = texture2D(m_samp0, m_cord0.xy).bgra;
  }
  else if (m_method == 2) /*SM_MULTI*/
  {
    gl_FragColor.rgba = (texture2D(m_samp0, m_cord0.xy) * texture2D(m_samp1, m_cord1.xy)).bgra;
  }
  else /*SM_DEFAULT*/
  {
    gl_FragColor = m_colour;
  }
}
