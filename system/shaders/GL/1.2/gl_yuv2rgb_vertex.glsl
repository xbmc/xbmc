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

attribute vec4 m_attrpos;
attribute vec2 m_attrcordY;
attribute vec2 m_attrcordU;
attribute vec2 m_attrcordV;
varying vec2 m_cordY;
varying vec2 m_cordU;
varying vec2 m_cordV;
uniform mat4 m_proj;
uniform mat4 m_model;

void main ()
{
  mat4 mvp    = m_proj * m_model;
  gl_Position = mvp * m_attrpos;
  gl_Position.z = -1. * gl_Position.w;
  m_cordY     = m_attrcordY;
  m_cordU     = m_attrcordU;
  m_cordV     = m_attrcordV;
}
