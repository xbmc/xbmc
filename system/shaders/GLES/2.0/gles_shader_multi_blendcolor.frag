/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#version 100

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
varying vec4 m_cord0;
varying vec4 m_cord1;
uniform lowp vec4 m_unicol;
uniform float m_sdrPeak;
uniform float m_sdrSaturation;

highp float rand(highp vec2 co)
{
  // Simple stable hash for dithering.
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 transferPQ(vec3 x)
{
  const float ST2084_m1 = 2610.0 / (4096.0 * 4.0);
  const float ST2084_m2 = (2523.0 / 4096.0) * 128.0;
  const float ST2084_c1 = 3424.0 / 4096.0;
  const float ST2084_c2 = (2413.0 / 4096.0) * 32.0;
  const float ST2084_c3 = (2392.0 / 4096.0) * 32.0;

  const mat3 matx = mat3(
      0.627402, 0.069095, 0.016394,
      0.329292, 0.919544, 0.088028,
      0.043306, 0.011360, 0.895578);

  x = max(x, vec3(0.0));

  // REC.709 to linear (approximation)
  x = pow(x, vec3(1.0 / 0.45));

  // REC.709 to BT.2020
  x = matx * x;
  x = max(x, vec3(0.0));

  // Optional saturation adjustment for SDR GUI when rendering into HDR PQ output.
  // Apply in linear BT.2020 and clamp to avoid out-of-gamut hue shifts.
  vec3 luma = vec3(dot(x, vec3(0.2627, 0.6780, 0.0593)));
  x = mix(luma, x, m_sdrSaturation);
  x = max(x, vec3(0.0));

  // Scale SDR peak (m_sdrPeak is nits/100)
  float peakNits = 100.0 * m_sdrPeak;

  // Linear (nits) normalized to 10,000 nits, then PQ encode
  x = pow(x * (peakNits / 10000.0), vec3(ST2084_m1));
  x = (ST2084_c1 + ST2084_c2 * x) / (1.0 + ST2084_c3 * x);
  x = pow(x, vec3(ST2084_m2));

  // Dither PQ output to reduce visible banding/stepping in gradients.
  float dither = (rand(gl_FragCoord.xy) - 0.5) / 1024.0;
  x = clamp(x + vec3(dither), vec3(0.0), vec3(1.0));

  return x;
}

void main ()
{
  vec4 rgb;

  rgb = m_unicol * texture2D(m_samp0, m_cord0.xy) * texture2D(m_samp1, m_cord1.xy);

#if defined(KODI_TRANSFER_PQ)
  rgb.rgb = transferPQ(rgb.rgb);
#endif

#if defined(KODI_LIMITED_RANGE)
  rgb.rgb *= (235.0 - 16.0) / 255.0;
  rgb.rgb += 16.0 / 255.0;
#endif

  gl_FragColor = rgb;
}
