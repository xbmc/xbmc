/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#version 100

precision highp float;

uniform sampler2D img;
uniform vec2 stepxy;
varying vec2 cord;
uniform float m_alpha;
uniform sampler2D kernelTex;

vec4 weight(float pos)
{
#if defined(HAS_FLOAT_TEXTURE)
  return texture2D(kernelTex, vec2(pos - 0.5));
#else
  return texture2D(kernelTex, vec2(pos - 0.5)) * 2.0 - 1.0;
#endif
}

vec3 pixel(float xpos, float ypos)
{
  return texture2D(img, vec2(xpos, ypos)).rgb;
}

vec3 line (float ypos, vec4 xpos, vec4 linetaps)
{
  return pixel(xpos.r, ypos) * linetaps.r +
         pixel(xpos.g, ypos) * linetaps.g +
         pixel(xpos.b, ypos) * linetaps.b +
         pixel(xpos.a, ypos) * linetaps.a;
}

void main()
{
  vec4 rgb;
  vec2 pos = cord + stepxy * 0.5;
  vec2 f = fract(pos / stepxy);

  vec4 linetaps = weight(1.0 - f.x);
  vec4 columntaps = weight(1.0 - f.y);

  // make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
  columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

  vec2 xystart = (-1.5 - f) * stepxy + pos;
  vec4 xpos = vec4(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0, xystart.x + stepxy.x * 3.0);

  rgb.rgb = line(xystart.y, xpos, linetaps) * columntaps.r +
            line(xystart.y + stepxy.y, xpos, linetaps) * columntaps.g +
            line(xystart.y + stepxy.y * 2.0, xpos, linetaps) * columntaps.b +
            line(xystart.y + stepxy.y * 3.0, xpos, linetaps) * columntaps.a;

  rgb.a = m_alpha;

  gl_FragColor = rgb;
}

