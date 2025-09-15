/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

#extension GL_OES_EGL_image_external : require

precision mediump float;
uniform samplerExternalOES m_samp0;
varying vec2 m_cord0;

uniform float m_brightness;
uniform float m_contrast;

void main()
{
  gl_FragColor = texture2D(m_samp0, m_cord0);
  gl_FragColor *= m_contrast;
  gl_FragColor += m_brightness;
}
