/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 150

uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
in vec2 m_cord0;
in vec2 m_cord1;
out vec4 fragColor;

// SM_MULTI shader
void main()
{
  fragColor = texture(m_samp0, m_cord0) * texture(m_samp1, m_cord1);
#if defined(KODI_LIMITED_RANGE)
  fragColor.rgb *= (235.0-16.0) / 255.0;
  fragColor.rgb += 16.0 / 255.0;
#endif
}
