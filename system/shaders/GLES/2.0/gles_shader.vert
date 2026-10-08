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

attribute vec4 m_attrpos;
attribute vec4 m_attrcol;
attribute vec4 m_attrcord0;
attribute vec4 m_attrcord1;
attribute vec4 m_attrsnap;
attribute vec4 m_attrgrad0;
attribute vec4 m_attrgrad1;
varying vec4 m_cord0;
varying vec4 m_cord1;
varying lowp vec4 m_colour;
uniform mat4 m_proj;
uniform mat4 m_model;
uniform mat4 m_coord0Matrix;
uniform float m_depth;
uniform mat4 m_gui;
uniform float m_snap;
uniform vec4 m_quadClip;

void main ()
{
  vec4 pos = m_attrpos;
  vec4 cord0 = m_attrcord0;
  vec4 cord1 = m_attrcord1;
  if (m_snap > 0.0)
  {
    // CGUITexture quads, clipped and rounded like its CPU path. Each quad is clamped to m_quadClip
    // in skin coordinates; m_attrgrad0/1 hold the texture coordinate change per unit of x (xy) and
    // y (zw). m_attrsnap holds the opposite corner of the quad, and z = 1 pushes this corner one
    // pixel away from it if both round to the same row or column, so that thin quads never vanish.
    pos.xy = clamp(m_attrpos.xy, m_quadClip.xy, m_quadClip.zw);
    vec2 delta = pos.xy - m_attrpos.xy;
    cord0.xy += m_attrgrad0.xy * delta.x + m_attrgrad0.zw * delta.y;
    cord1.xy += m_attrgrad1.xy * delta.x + m_attrgrad1.zw * delta.y;

    vec2 opposite = clamp(m_attrsnap.xy, m_quadClip.xy, m_quadClip.zw);
    float push = all(notEqual(pos.xy, opposite)) ? m_attrsnap.z : 0.0;

    pos = m_gui * pos;
    pos.xyz = floor(pos.xyz + 0.5);
    opposite = floor((m_gui * vec4(opposite, 0.0, 1.0)).xy + 0.5);
    pos.xy += vec2(equal(pos.xy, opposite)) * push;
  }
  else
  {
    pos = m_gui * pos;
  }

  mat4 mvp = m_proj * m_model;
  gl_Position = mvp * pos;
  gl_Position.z = m_depth * gl_Position.w;
  m_colour = m_attrcol;
  m_cord0 = m_coord0Matrix * cord0;
  m_cord1 = cord1;
}
