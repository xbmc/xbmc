/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#version 100

precision mediump float;
uniform lowp vec4 m_unicol;

// SM_DEFAULT shader
void main ()
{
  gl_FragColor = m_unicol;
#if defined(KODI_LIMITED_RANGE)
  gl_FragColor.rgb *= (235.0-16.0) / 255.0;
  gl_FragColor.rgb += 16.0 / 255.0;
#endif
}
