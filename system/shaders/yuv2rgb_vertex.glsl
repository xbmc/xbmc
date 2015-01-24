/*
<<<<<<< HEAD
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
=======
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
>>>>>>> FETCH_HEAD
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
<<<<<<< HEAD
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
=======
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
>>>>>>> FETCH_HEAD
 *
 */

varying vec2 m_cordY;
varying vec2 m_cordU;
varying vec2 m_cordV;

void main()
{
#if(XBMC_texture_rectangle_hack)
  m_cordY = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0 / 2);
  m_cordU = vec2(gl_TextureMatrix[1] * gl_MultiTexCoord1 * 2);
  m_cordV = vec2(gl_TextureMatrix[2] * gl_MultiTexCoord2);
#else
  m_cordY = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);
  m_cordU = vec2(gl_TextureMatrix[1] * gl_MultiTexCoord1);
  m_cordV = vec2(gl_TextureMatrix[2] * gl_MultiTexCoord2);
#endif
  gl_Position = ftransform();
  gl_FrontColor = gl_Color;
}
