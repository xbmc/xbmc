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

precision mediump float;
uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
uniform sampler2D m_samp2;

varying vec4 m_cord0;

uniform float m_alpha;

uniform int m_layers;

uniform mat4 m_yuvmat;

uniform int m_enableColorConversion;
uniform mat3 m_primMat;
uniform float m_gammaSrc;
uniform float m_gammaDstInv;

uniform int m_toneMapMethod;
uniform float m_toneP1;
uniform vec3 m_coefsDst;

float tonemap(float val)
{
  return val * (1.0 + val / (m_toneP1 * m_toneP1)) / (1.0 + val);
}

void main ()
{
  vec4 yuv;

  if (m_layers == 3)
  {
    yuv = vec4(texture2D(m_samp0, m_cord0.xy).r,
               texture2D(m_samp1, m_cord0.xy).r,
               texture2D(m_samp2, m_cord0.xy).r,
               1.0);
  }
  else if (m_layers == 2)
  {
    yuv = vec4(texture2D(m_samp0, m_cord0.xy).r,
               texture2D(m_samp1, m_cord0.xy).rg,
               1.0);
  }

  vec4 rgb = m_yuvmat * yuv;
  rgb.a = m_alpha;

  if (m_enableColorConversion != 0)
  {
    rgb.rgb = pow(max(vec3(0), rgb.rgb), vec3(m_gammaSrc));
    rgb.rgb = max(vec3(0), m_primMat * rgb.rgb);
    rgb.rgb = pow(rgb.rgb, vec3(m_gammaDstInv));
  }

  if (m_toneMapMethod != 0)
  {
    float luma = dot(rgb.rgb, m_coefsDst);
    rgb.rgb *= tonemap(luma) / luma;
  }

  gl_FragColor = rgb;
}
