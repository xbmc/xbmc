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
varying vec4 m_cord0;
varying vec4 m_cord1;
varying lowp vec4 m_colour;
uniform mat4 m_proj;
uniform mat4 m_model;
uniform mat4 m_coord0Matrix;
uniform float m_depth;
uniform mat4 m_gui;
uniform float m_snap;
uniform vec4 m_quadRect;
uniform vec4 m_quadClip;
uniform vec2 m_texSwap;

void main ()
{
  vec4 pos = m_attrpos;
  vec4 cord0 = m_attrcord0;
  vec4 cord1 = m_attrcord1;
  if (m_snap > 0.0)
  {
    // CGUITexture quads, clipped and rounded like its CPU path. m_attrpos is a corner and
    // m_attrsnap the opposite corner of the quad, each as a fraction of the texture's rectangle
    // m_quadRect (x, y, width, height) in xy plus an offset in zw. cord.xy holds the texture
    // coordinates at this corner and cord.zw at the opposite one; m_texSwap flags textures whose
    // coordinates run along the other axis.
    vec2 p = m_quadRect.xy + m_attrpos.xy * m_quadRect.zw + m_attrpos.zw;
    vec2 q = m_quadRect.xy + m_attrsnap.xy * m_quadRect.zw + m_attrsnap.zw;
    vec2 pc = clamp(p, m_quadClip.xy, m_quadClip.zw);
    vec2 qc = clamp(q, m_quadClip.xy, m_quadClip.zw);
    vec2 t = (pc - p) / (q - p);
    cord0 = vec4(m_attrcord0.xy + (m_texSwap.x > 0.0 ? t.yx : t) * (m_attrcord0.zw - m_attrcord0.xy), 0.0, 1.0);
    cord1 = vec4(m_attrcord1.xy + (m_texSwap.y > 0.0 ? t.yx : t) * (m_attrcord1.zw - m_attrcord1.xy), 0.0, 1.0);

    // Round to whole pixels. A bottom corner is pushed one pixel away from the opposite corner if
    // both round to the same row or column, so that thin quads never vanish.
    float push = (p.y > q.y && all(notEqual(pc, qc))) ? 1.0 : 0.0;
    pos = m_gui * vec4(pc, 0.0, 1.0);
    pos.xyz = floor(pos.xyz + 0.5);
    vec2 opposite = floor((m_gui * vec4(qc, 0.0, 1.0)).xy + 0.5);
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
