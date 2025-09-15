/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

precision mediump float;
uniform sampler2D m_samp0;
uniform lowp vec4 m_unicol;
varying vec2 m_cord0;
uniform float m_sdrPeak;

void main()
{
  gl_FragColor = texture2D(m_samp0, m_cord0) * m_unicol;

#if defined(KODI_LIMITED_RANGE)
  gl_FragColor.rgb *= (235.0 - 16.0) / 255.0;
  gl_FragColor.rgb += 16.0 / 255.0;
#endif

#if defined(KODI_TRANSFER_PQ)
  gl_FragColor.rgb *= m_sdrPeak;
#endif
}
