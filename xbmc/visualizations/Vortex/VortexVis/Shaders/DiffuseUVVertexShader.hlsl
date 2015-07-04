/*
*  Copyright © 2010-2015 Team Kodi
*  http://kodi.tv
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "constants_table.fx"

struct VS_INPUT
{
  float4 vPosition : POSITION;
  float4 vNormal : NORMAL;
  float4 Colour : COLOR;
  float2 Tex0 : TEXCOORD0;
};

struct VS_OUTPUT
{
  float4 vPosition : SV_POSITION;
  float4 Colour : COLOR;
  float2 Tex0 : TEXCOORD0;
};

VS_OUTPUT Main( VS_INPUT input )
{
  VS_OUTPUT output;
  // Transform coord
  output.vPosition = mul(WVPMat, float4(input.vPosition.xyz, 1.0f));
  output.Colour = input.Colour;
  output.Tex0 = input.Tex0;
  return output;
}
