/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 120

uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
varying vec2 m_cord0;
varying vec2 m_cord1;
uniform vec4 m_unicol;

// SM_MULTI shader
void main()
{
  gl_FragColor = m_unicol * texture2D(m_samp0, m_cord0) * texture2D(m_samp1, m_cord1);
}
