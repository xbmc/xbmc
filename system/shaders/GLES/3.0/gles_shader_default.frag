#version 300 es

precision mediump float;
uniform lowp vec4 m_unicol;
out vec4 fragColor;

void main ()
{
  vec4 rgb;

  rgb = m_unicol;

#if defined(KODI_LIMITED_RANGE)
  rgb.rgb *= (235.0 - 16.0) / 255.0;
  rgb.rgb += 16.0 / 255.0;
#endif

  fragColor = rgb;
}
