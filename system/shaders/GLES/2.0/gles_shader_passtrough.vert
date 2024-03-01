/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

attribute vec2 m_attrpos;
varying vec2 m_cord0;

// this shader expects a full-screen triangle with normalized coordinates.

void main()
{
  gl_Position.xy = m_attrpos * vec2(2.) - vec2(1.);
  m_cord0 = m_attrpos;
}
