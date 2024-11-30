/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 120

attribute vec2 m_attrpos;
uniform mat4 m_proj;
uniform mat4 m_model;

void main()
{
  mat4 mvp = m_proj * m_model;
  gl_Position = mvp * vec4(m_attrpos, 0., 1.);
  gl_Position.z = -1. * gl_Position.w;
}
