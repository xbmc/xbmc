precision mediump float;
uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
varying vec4      m_cord0;
varying vec4      m_cord1;
varying vec4      m_colour;
uniform int       m_method;

// SM_TEXTURE
void main ()
{
  gl_FragColor.rgba = vec4(texture2D(m_samp0, m_cord0.xy).bgra * m_colour);
}
