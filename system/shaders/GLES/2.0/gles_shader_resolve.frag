/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

#if defined(KODI_FETCH_ARM)
#extension GL_ARM_shader_framebuffer_fetch : enable
#elif defined(KODI_FETCH_EXT)
#extension GL_EXT_shader_framebuffer_fetch : enable
#elif defined(KODI_FETCH_NV)
#extension GL_NV_shader_framebuffer_fetch : enable
#endif

precision mediump float;
uniform sampler2D m_samp0;
varying vec2 m_cord0;

void main()
{
#if defined(KODI_FETCH_ARM)
  vec4 rgba = gl_LastFragColorARM;
#elif defined(KODI_FETCH_EXT)
  vec4 rgba = gl_LastFragColor;
#elif defined(KODI_FETCH_NV)
  vec4 rgba = gl_LastFragColor;
#else
  vec4 rgba = texture2D(m_samp0, m_cord0);
#endif

#if defined(KODI_LIMITED_RANGE_PASS)
  rgba.rgb *= (235.0 - 16.0) / 255.0;
  rgba.rgb += 16.0 / 255.0;
#endif

  gl_FragColor = rgba;
}
