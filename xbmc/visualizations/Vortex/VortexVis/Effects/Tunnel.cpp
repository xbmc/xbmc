/*
 *  Copyright © 2010-2012 Team XBMC
 *  http://xbmc.org
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

#include "Tunnel.h"
#include "Renderer.h"
#include "angelscript.h"

struct TunnelParameters
{
	float	Time;
	float	XRadius;
	float	YRadius;
	float	XCounter;
	float	YCounter;
	int		CircleSegments;
	int		TunnelLength;
	float	TunnelZ;
	float	RotateX;
	float	RotateY;
	float	RotateZ;
};

float Min( float x, float y )
{
	return x < y ? x : y;
}

float Max( float x, float y )
{
	return x > y ? x : y;
}

float ApplySinX(int i, float time)
{
	const float radian = (i/40.0f*(2.0f*3.14159f));
	float returnvalue = sinf(radian/2.0f+time)+6.0f*cosf(radian/4.0f+time);//+6*cosf((radian+time)/10.0f);
	return returnvalue*1.0f;
}

float ApplySinY(int i, float time)
{
	const float radian = (i/40.0f*(2.0f*3.14159f));
	float returnvalue = 3.0f*sinf(radian/8.0f+time)+3.0f*cosf(radian/2.0f+time)*sinf(radian/4.0f+time);
	return returnvalue*0.2f;
}

void RenderTunnel( TunnelParameters& tp )
{
	int TunnelLength = max( min( tp.TunnelLength, 100 ), 2 );
	int CircleSegments = max( min( tp.CircleSegments, 30 ), 3 );

// 	Renderer::RotateAxis(atanf((ApplySinX(3, tp.XCounter)-ApplySinX(0,tp.XCounter))/3.0f)*(180.0f/3.14159f), 0.0f, 1.0f, 0.0f);
// 	Renderer::RotateAxis(atanf((ApplySinY(0,tp.YCounter)-ApplySinY(7, tp.YCounter))/7.0f)*(180.0f/3.14159f), 1.0f, 0.0f, 0.0f);

	Renderer::RotateAxis( (tp.RotateZ) * 45, 0, 0, 1 );

	Renderer::Translate( -ApplySinX( 0, tp.XCounter ), -ApplySinY( 0, tp.YCounter ), 0.0f );

	for ( int z = 0; z < CircleSegments; z++ ) 
	{
		Renderer::Begin( D3DPT_TRIANGLESTRIP );
		for ( int i = 0; i < TunnelLength; i++ )
		{
			float col = 1-(((1.0f * i / TunnelLength))*1);
			Renderer::Colour( col, col, col, col );
			Renderer::TexCoord( 1-((float)z/CircleSegments*4.0f),
								1 - ( (float)i / TunnelLength * 8 + tp.TunnelZ ) );
			Renderer::Vertex( tp.XRadius * sinf( (float)z / CircleSegments * ( 2.0f * 3.14159f ) ) + ApplySinX( i, tp.XCounter ),
							  tp.YRadius * cosf( (float)z / CircleSegments * ( 2.0f * 3.14159f ) ) + ApplySinY( i, tp.YCounter ),
							  (float)i );
			Renderer::TexCoord( 1 - ( ( (float)z + 1 ) / CircleSegments * 4.0f ),
								1 - ( (float)i / TunnelLength * 8 + tp.TunnelZ ) );
			Renderer::Vertex( tp.XRadius * sinf( ( (float)z + 1 ) / CircleSegments * ( 2.0f * 3.14159f ) ) + ApplySinX( i, tp.XCounter ),
							  tp.YRadius * cosf( ( (float)z + 1 ) / CircleSegments * ( 2.0f * 3.14159f ) ) + ApplySinY( i, tp.YCounter ),
							  (float)i );
		}
		Renderer::End();
	}
}

#ifndef assert
#define assert
#endif

void Tunnel::RegisterScriptInterface( asIScriptEngine* pScriptEngine )
{
	int r;

	//----------------------------------------------------------------------------
	// Register Tunnel Parameter structure
	r = pScriptEngine->RegisterObjectType("TunnelParameters", sizeof(TunnelParameters), asOBJ_VALUE | asOBJ_POD); assert( r >= 0 );

	// Register the object properties
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float Time", offsetof(TunnelParameters, Time)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float XRadius", offsetof(TunnelParameters, XRadius)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float YRadius", offsetof(TunnelParameters, YRadius)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float XCounter", offsetof(TunnelParameters, XCounter)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float YCounter", offsetof(TunnelParameters, YCounter)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "int CircleSegments", offsetof(TunnelParameters, CircleSegments)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "int TunnelLength", offsetof(TunnelParameters, TunnelLength)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float TunnelZ", offsetof(TunnelParameters, TunnelZ)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float RotateX", offsetof(TunnelParameters, RotateX)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float RotateY", offsetof(TunnelParameters, RotateY)); assert( r >= 0 );
	r = pScriptEngine->RegisterObjectProperty("TunnelParameters", "float RotateZ", offsetof(TunnelParameters, RotateZ)); assert( r >= 0 );

	r = pScriptEngine->RegisterGlobalFunction("void RenderTunnel(TunnelParameters& in)", asFUNCTION(RenderTunnel), asCALL_CDECL); assert(r >= 0);
}
