#version 150

uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
uniform vec4 m_unicol;
in vec4 m_cord0;
in vec4 m_cord1;
out vec4 fragColor;

// SM_MULTI shader
void main ()
{
  fragColor.rgba = m_unicol * texture(m_samp0, m_cord0.xy) * texture(m_samp1, m_cord1.xy);
#if defined(KODI_LIMITED_RANGE)
  fragColor.rgb *= (235.0-16.0) / 255.0;
  fragColor.rgb += 16.0 / 255.0;
#endif
}
