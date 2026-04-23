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
uniform float m_sdrPeak;

const vec3 kLuma = vec3(0.2126, 0.7152, 0.0722);

void main()
{
  gl_FragColor = texture2D(m_samp0, m_cord0.xy);
  gl_FragColor.a *= texture2D(m_samp1, m_cord1.xy).r;

#if defined(KODI_LIMITED_RANGE)
  gl_FragColor.rgb *= (235.0 - 16.0) / 255.0;
  gl_FragColor.rgb += 16.0 / 255.0;
#endif

#if defined(KODI_TRANSFER_PQ)
  float luma = dot(gl_FragColor.rgb, kLuma);
  vec3 chroma = gl_FragColor.rgb - luma;
  gl_FragColor.rgb = (luma * m_sdrPeak) + chroma;
#endif

}
