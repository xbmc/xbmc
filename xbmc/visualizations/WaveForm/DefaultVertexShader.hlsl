/*
*      Copyright (C) 2005-2015 Team Kodi
*      http://kodi.tv
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

struct VS_OUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
};

cbuffer cbViewPort : register(b0)
{
  float g_viewPortWidth;
  float g_viewPortHeigh;
  float align1;
  float align2;
};

VS_OUT main(float4 pos : POSITION, float4 col : COLOR)
{
  VS_OUT r = (VS_OUT)0;
  r.pos.x  =  (pos.x / (g_viewPortWidth / 2.0)) - 1;
  r.pos.y  = -(pos.y / (g_viewPortHeigh / 2.0)) + 1;
  r.pos.z  = pos.z;
  r.pos.w  = 1.0;
  r.col    = col;
  return r;
}