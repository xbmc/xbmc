#if defined(XBMC_DITHER)
  if (m_ditherEnabled > 0.0)
  {
    vec2 ditherpos = gl_FragCoord.xy / m_dithersize;
    float ditherval = texture2D(m_dither, ditherpos).r * 16.0;
    rgb = floor(rgb * m_ditherquant + ditherval) / m_ditherquant;
  }
#endif
