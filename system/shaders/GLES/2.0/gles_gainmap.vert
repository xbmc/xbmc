/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

attribute vec4 a_pos;
attribute vec2 a_tex;
varying vec2 v_tex;
uniform mat4 u_proj;
uniform mat4 u_model;

void main()
{
  gl_Position = u_proj * u_model * a_pos;
  gl_Position.z = -gl_Position.w;
  v_tex = a_tex;
}
