#version 150

uniform vec4 m_unicol;
out vec4 fragColor;

// SM_DEFAULT shader
void main ()
{
  fragColor = m_unicol;
#if defined(KODI_LIMITED_RANGE)
 fragColor.rgb *= (235.0-16.0) / 255.0;
 fragColor.rgb += 16.0 / 255.0;
#endif
}
