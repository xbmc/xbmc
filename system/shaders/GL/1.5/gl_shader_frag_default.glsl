/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 150

uniform vec4 m_unicol;
out vec4 fragColor;

// SM_DEFAULT shader
void main()
{
  fragColor = m_unicol;
#if defined(KODI_LIMITED_RANGE)
 fragColor.rgb *= (235.0-16.0) / 255.0;
 fragColor.rgb += 16.0 / 255.0;
#endif
}
