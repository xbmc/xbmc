/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if(XBMC_texture_rectangle)
# extension GL_ARB_texture_rectangle : enable
# define texture2D texture2DRect
# define sampler2D sampler2DRect
#endif

uniform sampler2D m_sampY;
uniform sampler2D m_sampU;
uniform sampler2D m_sampV;
varying vec2      m_cordY;
varying vec2      m_cordU;
varying vec2      m_cordV;

uniform mat4      m_yuvmat;

uniform float     m_stepX;
uniform float     m_stepY;
uniform int       m_field;

void main()
{
  vec4 yuv, rgb;

  vec2 offsetY;
  vec2 offsetU;
  vec2 offsetV;
  float temp1 = mod(m_cordY.y, 2*m_stepY);

  offsetY  = m_cordY;
  offsetU  = m_cordU;
  offsetV  = m_cordV;

  offsetY.y -= (temp1 - m_stepY/2 + float(m_field)*m_stepY);
  offsetU.y -= (temp1 - m_stepY/2 + float(m_field)*m_stepY)/2;
  offsetV.y -= (temp1 - m_stepY/2 + float(m_field)*m_stepY)/2;

  yuv.rgba = vec4( texture2D(m_sampY, offsetY).r
                 , texture2D(m_sampU, offsetU).r
                 , texture2D(m_sampV, offsetV).a
                 , 1.0);
  rgb   = m_yuvmat * yuv;
  rgb.a = gl_Color.a;
  gl_FragColor = rgb;
}
