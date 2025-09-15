/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

#extension GL_OES_EGL_image_external : require

precision highp float;
uniform samplerExternalOES m_samp0;
varying vec2 m_cord0;
uniform int m_field;
uniform float m_step;

uniform float m_brightness;
uniform float m_contrast;

void main()
{
  vec2 source = m_cord0;

  float temp1 = mod(source.y, 2.0 * m_step);
  float temp2 = source.y - temp1;
  source.y = temp2 + m_step / 2.0 - float(m_field) * m_step;

  // Blend missing line
  vec2 below;
  float bstep = step(m_step, temp1);
  below.x = source.x;
  below.y = source.y + (2.0*m_step * bstep);

  gl_FragColor = mix(texture2D(m_samp0, source), texture2D(m_samp0, below), 0.5);
  gl_FragColor *= m_contrast;
  gl_FragColor += m_brightness;
}
