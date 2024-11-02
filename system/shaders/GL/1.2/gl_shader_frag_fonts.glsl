/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 120

uniform sampler2D m_samp0;
varying vec2 m_cord0;
varying vec4 m_colour;

// SM_FONTS shader
void main()
{
  gl_FragColor = m_colour;
  gl_FragColor.a *= texture2D(m_samp0, m_cord0).r;
}
