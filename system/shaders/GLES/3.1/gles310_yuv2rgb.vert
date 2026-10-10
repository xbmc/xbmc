/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 310 es

in vec4 m_attrpos;
in vec2 m_attrcordY;
in vec2 m_attrcordU;
in vec2 m_attrcordV;
out vec2 m_cordY;
out vec2 m_cordU;
out vec2 m_cordV;
uniform mat4 m_proj;
uniform mat4 m_model;
uniform mat4 m_quad;
uniform vec4 m_texRectY;
uniform vec4 m_texRectU;
uniform vec4 m_texRectV;

void main()
{
  mat4 mvp = m_proj * m_model;
  gl_Position = mvp * (m_quad * m_attrpos);
  m_cordY = m_texRectY.xy + m_attrcordY * m_texRectY.zw;
  m_cordU = m_texRectU.xy + m_attrcordU * m_texRectU.zw;
  m_cordV = m_texRectV.xy + m_attrcordV * m_texRectV.zw;
}
