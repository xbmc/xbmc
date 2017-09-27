/*
 *      Copyright (C) 2010-2017 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#if defined(XBMC_DITHER)
uniform sampler2D m_dither;
uniform float     m_ditherquant;
uniform vec2      m_dithersize;
#endif
#if defined(KODI_3DLUT)
uniform float     m_CLUTsize;
uniform sampler3D m_CLUT;
#endif

void main()
{
  vec4 rgb        = process();

#if defined(KODI_3DLUT)
  // FIXME: can this be optimized?
  rgb             = texture3D(m_CLUT, (rgb.rgb*(m_CLUTsize-1.0) + 0.5) / m_CLUTsize);
#endif

#if defined(XBMC_FULLRANGE)
#if __VERSION__ <= 120
  rgb = (rgb-(16.0/255.0)) * 255.0/219.0;
#else
  rgb             = clamp((rgb-(16.0/255.0)) * 255.0/219.0, 0, 1);
#endif
#endif

#if defined(XBMC_DITHER)
  vec2 ditherpos  = gl_FragCoord.xy / m_dithersize;
  // ditherval is multiplied by 65536/(dither_size^2) to make it [0,1[
  // FIXME: scale dither values before uploading?
  float ditherval = texture2D(m_dither, ditherpos).r * 16.0;
  rgb             = floor(rgb * m_ditherquant + ditherval) / m_ditherquant;
#endif

  gl_FragColor    = rgb;
}
