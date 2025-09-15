/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

attribute vec4 m_attrpos;
attribute vec4 m_attrcol;
attribute vec4 m_attrcord0;
attribute vec4 m_attrcord1;
varying vec2 m_cord0;
varying vec2 m_cord1;
varying lowp vec4 m_colour;
uniform mat4 m_proj;
uniform mat4 m_model;
uniform mat4 m_coord0Matrix;
uniform float m_depth;

void main()
{
  mat4 mvp = m_proj * m_model;
  gl_Position = mvp * m_attrpos;
  gl_Position.z = m_depth * gl_Position.w;
  m_colour = m_attrcol;
  vec4 cord0 = m_coord0Matrix * m_attrcord0;
  m_cord0 = cord0.xy;
  m_cord1 = m_attrcord1.xy;
}
