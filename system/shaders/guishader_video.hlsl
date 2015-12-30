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

#include "guishader_common.hlsl"

Texture2D texVideo : register(t0);

SamplerState DynamicSampler : register(s1)
{
  Filter = MIN_MAG_MIP_POINT; // default
  AddressU = CLAMP;
  AddressV = CLAMP;
  Comparison = NEVER;
};

float4 PS(PS_INPUT input) : SV_TARGET
{
  return adjustColorRange(texVideo.Sample(DynamicSampler, input.tex));
}