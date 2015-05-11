#if (XBMC_DITHER)
uniform sampler2D m_dither;
uniform float     m_ditherquant;
uniform vec2      m_dithersize;
#endif

void main()
{
  vec4 rgb        = process();

#if (XBMC_FULLRANGE)
  rgb             = clamp((rgb-(16.0/255.0)) * 255.0/219.0, 0, 1);
#endif

#if (XBMC_DITHER)
  vec2 ditherpos  = gl_FragCoord.xy / m_dithersize;
  // ditherval is multiplied by 65536/(dither_size^2) to make it [0,1[
  // FIXME: scale dither values before uploading?
  float ditherval = texture2D(m_dither, ditherpos).r * 16.0;
  rgb             = floor(rgb * m_ditherquant + ditherval) / m_ditherquant;
#endif

  gl_FragColor    = rgb;
}
