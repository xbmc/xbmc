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

precision highp float;
uniform sampler2D m_sampY;
uniform sampler2D m_sampU;
uniform sampler2D m_sampV;
varying vec2 m_cordY;
varying vec2 m_cordU;
varying vec2 m_cordV;
uniform float m_alpha;
uniform mat4 m_yuvmat;
uniform float m_stepX;
uniform float m_stepY;
uniform int m_field;
uniform mat3 m_primMat;
uniform float m_gammaDstInv;
uniform float m_gammaSrc;
uniform float m_toneP1;
uniform float m_luminance;
uniform vec3 m_coefsDst;

void main()
{
  vec4 rgb;

  vec2 offsetY;
  vec2 offsetU;
  vec2 offsetV;
  float temp1 = mod(m_cordY.y, 2.0 * m_stepY);

  offsetY  = m_cordY;
  offsetU  = m_cordU;
  offsetV  = m_cordV;

  offsetY.y -= (temp1 - m_stepY / 2.0 + float(m_field) * m_stepY);
  offsetU.y -= (temp1 - m_stepY / 2.0 + float(m_field) * m_stepY) / 2.0;
  offsetV.y -= (temp1 - m_stepY / 2.0 + float(m_field) * m_stepY) / 2.0;

  float bstep = step(m_stepY, temp1);

  // Blend missing line
  vec2 belowY, belowU, belowV;

  belowY.x = offsetY.x;
  belowY.y = offsetY.y + (2.0 * m_stepY * bstep);
  belowU.x = offsetU.x;
  belowU.y = offsetU.y + (m_stepY * bstep);
  belowV.x = offsetV.x;
  belowV.y = offsetV.y + (m_stepY * bstep);

  vec4 rgbAbove;
  vec4 rgbBelow;
  vec4 yuvAbove;
  vec4 yuvBelow;

  yuvAbove = vec4(texture2D(m_sampY, offsetY).r, texture2D(m_sampU, offsetU).g, texture2D(m_sampV, offsetV).a, 1.0);
  rgbAbove = m_yuvmat * yuvAbove;
  rgbAbove.a = m_alpha;

  yuvBelow = vec4(texture2D(m_sampY, belowY).r, texture2D(m_sampU, belowU).g, texture2D(m_sampV, belowV).a, 1.0);
  rgbBelow = m_yuvmat * yuvBelow;
  rgbBelow.a = m_alpha;

  rgb = mix(rgb, rgbBelow, 0.5);

#if defined(XBMC_COL_CONVERSION)
  rgb.rgb = pow(max(vec3(0), rgb.rgb), vec3(m_gammaSrc));
  rgb.rgb = max(vec3(0), m_primMat * rgb.rgb);
  rgb.rgb = pow(rgb.rgb, vec3(m_gammaDstInv));

#if defined(KODI_TONE_MAPPING_REINHARD)
  float luma = dot(rgb.rgb, m_coefsDst);
  rgb.rgb *= reinhard(luma) / luma;

#elif defined(KODI_TONE_MAPPING_ACES)
  rgb.rgb = inversePQ(rgb.rgb);
  rgb.rgb *= (10000.0 / m_luminance) * (2.0 / m_toneP1);
  rgb.rgb = aces(rgb.rgb);
  rgb.rgb *= (1.24 / m_toneP1);
  rgb.rgb = pow(rgb.rgb, vec3(0.27));

#elif defined(KODI_TONE_MAPPING_HABLE)
  rgb.rgb = inversePQ(rgb.rgb);
  rgb.rgb *= m_toneP1;
  float wp = m_luminance / 100.0;
  rgb.rgb = hable(rgb.rgb * wp) / hable(vec3(wp));
  rgb.rgb = pow(rgb.rgb, vec3(1.0 / 2.2));
#endif

#endif

  gl_FragColor = rgb;
}
