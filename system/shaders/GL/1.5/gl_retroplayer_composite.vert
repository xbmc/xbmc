/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 150

in vec2 a_pos; // Corner of the unit quad
uniform mat4 u_matrix; // Projection and model view
uniform mat4 u_quad; // Unit quad to the destination corners
uniform mat4 u_texQuad; // Unit quad to the texture coordinates
out vec2 v_tex;

void main()
{
  vec4 corner = vec4(a_pos, 0.0, 1.0);
  gl_Position = u_matrix * (u_quad * corner);
  gl_Position.z = -gl_Position.w;
  v_tex = (u_texQuad * corner).xy;
}
