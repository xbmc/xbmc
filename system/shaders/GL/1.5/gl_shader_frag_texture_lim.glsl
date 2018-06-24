#version 150

uniform sampler2D m_samp0;
uniform vec4 m_unicol;
in vec4 m_cord0;
out vec4 fragColor;

// SM_TEXTURE shader
void main ()
{
  fragColor.rgba = vec4(texture(m_samp0, m_cord0.xy).rgba * m_unicol);
#if !defined(KODI_LIMITED_RANGE)
  fragColor.rgb = clamp((fragColor.rgb-(16.0/255.0)) * 255.0/219.0, 0, 1);
#endif
}
