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
uniform vec2      stepxy;
varying vec2      cord;
uniform float     m_alpha;
uniform sampler2D kernelTex;

vec3 weight(float pos)
{
#if (HAS_FLOAT_TEXTURE)
  return texture2D(kernelTex, vec2(pos - 0.5)).rgb;
#else
  return texture2D(kernelTex, vec2(pos - 0.5)).rgb * 2.0 - 1.0;
#endif
}

vec3 pixel(float xpos, float ypos)
{
  return texture2D(img, vec2(xpos, ypos)).rgb;
}

vec3 line (float ypos, vec3 xpos1, vec3 xpos2, vec3 linetaps1, vec3 linetaps2)
{
  return
    pixel(xpos1.r, ypos) * linetaps1.r +
    pixel(xpos1.g, ypos) * linetaps2.r +
    pixel(xpos1.b, ypos) * linetaps1.g +
    pixel(xpos2.r, ypos) * linetaps2.g +
    pixel(xpos2.g, ypos) * linetaps1.b +
    pixel(xpos2.b, ypos) * linetaps2.b;
}

void main()
{
  vec4 rgb;
  vec2 pos = cord + stepxy * 0.5;
  vec2 f = fract(pos / stepxy);

  vec3 linetaps1   = weight((1.0 - f.x) / 2.0);
  vec3 linetaps2   = weight((1.0 - f.x) / 2.0 + 0.5);
  vec3 columntaps1 = weight((1.0 - f.y) / 2.0);
  vec3 columntaps2 = weight((1.0 - f.y) / 2.0 + 0.5);

  //make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
  float sum = linetaps1.r + linetaps1.g + linetaps1.b + linetaps2.r + linetaps2.g + linetaps2.b;
  linetaps1 /= sum;
  linetaps2 /= sum;
  sum = columntaps1.r + columntaps1.g + columntaps1.b + columntaps2.r + columntaps2.g + columntaps2.b;
  columntaps1 /= sum;
  columntaps2 /= sum;

  vec2 xystart = (-2.5 - f) * stepxy + pos;
  vec3 xpos1 = vec3(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0);
  vec3 xpos2 = vec3(xystart.x + stepxy.x * 3.0, xystart.x + stepxy.x * 4.0, xystart.x + stepxy.x * 5.0);

  rgb.rgb =
   line(xystart.y                 , xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
   line(xystart.y + stepxy.y      , xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
   line(xystart.y + stepxy.y * 2.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
   line(xystart.y + stepxy.y * 3.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
   line(xystart.y + stepxy.y * 4.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
   line(xystart.y + stepxy.y * 5.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

  rgb.a = m_alpha;

  gl_FragColor = rgb;
}

