/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

precision mediump float;
uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
varying vec4 m_cord0;
varying vec4 m_cord1;
uniform lowp vec4 m_unicol;
uniform float m_sdrPeak;

void main()
{
  vec4 rgba = m_unicol;
  rgba *= texture2D(m_samp0, m_cord0.xy);
  rgba.a *= texture2D(m_samp1, m_cord1.xy).r;

#if defined(KODI_LIMITED_RANGE)
  rgba.rgb *= (235.0 - 16.0) / 255.0;
  rgba.rgb += 16.0 / 255.0;
#endif

#if defined(KODI_TRANSFER_PQ)
  rgba.rgb *= m_sdrPeak;
#endif

  gl_FragColor = rgba;
}
