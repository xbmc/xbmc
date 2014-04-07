/*
 *  Copyright © 2010-2013 Team XBMC
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

#include "VoicePrint.h"
#include <stdio.h>
#include "Shader.h"
#include "angelscript.h"

using namespace std;

char ColourRemapPixelShaderSrc[] =
{
	" sampler2D SpectrumTexture : tex0;										\n"
	" sampler2D RemapTexture : tex1;										\n"
	"		struct PS_INPUT													\n"
	"		{																\n"
	"			float2 Tex0 : TEXCOORD0;	    							\n"
	"		};																\n"
	"																		\n"
	" void Main( PS_INPUT input, out float4 OutColor : COLOR0 )				\n"
	" {																		\n"
	"	float4 SpecCol = tex2D( SpectrumTexture, input.Tex0 );				\n"
	"	OutColor = tex2D( RemapTexture, SpecCol.ra );						\n"
	" }																		\n"
	"																		\n"
};


class ColourRemapPixelShader : public Shader
{
	DECLARE_SHADER(ColourRemapPixelShader);
public:
	ColourRemapPixelShader( EShaderType ShaderType, char* pShaderSrc ) : Shader( ShaderType, pShaderSrc ){};
};

IMPLEMENT_SHADER(ColourRemapPixelShader, ColourRemapPixelShaderSrc, ST_PIXEL);

// TODO
float GetSpecLeft(int index);
extern float g_timePass;

VoicePrint::VoicePrint()
{
	m_iCurrentTexture = 0;
	m_tex1 = Renderer::CreateTexture(512, 512);
	m_tex2 = Renderer::CreateTexture(512, 512);
	m_colourMap = NULL;

	m_spectrumTexture = Renderer::CreateTexture(1, 512, D3DFMT_A8R8G8B8, true);

	m_speed = 0.008f;
	m_minX = 0.99f;
	m_maxX = 0.01f;
	m_minY = 0.5f;
	m_maxY = 0.5f;

} // Constructor

VoicePrint::~VoicePrint()
{
	if (m_colourMap)
	{
		Renderer::ReleaseTexture( m_colourMap );
	}

	if ( m_tex1 )
		m_tex1->Release();

	if ( m_tex2 )
		m_tex2->Release();

	if ( m_spectrumTexture )
		m_spectrumTexture->Release();

} // Destructor

void VoicePrint::LoadColourMap(string& filename)
{
	char fullname[ 512 ];
	sprintf_s(fullname, 512, "%s%s", g_TexturePath, filename.c_str() );

	m_colourMap = Renderer::LoadTexture( fullname );

} // LoadColourMap

void VoicePrint::SetRect(float minX, float minY, float maxX, float maxY)
{
	m_minX = minX;
	m_minY = minY;
	m_maxX = maxX;
	m_maxY = maxY;

} // SetRect

void VoicePrint::SetSpeed(float speed)
{
	const float MIN_SPEED = 0.004f;
	const float MAX_SPEED = 0.02f;

	speed = min(speed, 1.0f);
	speed = max(speed, 0.0f);

	m_speed = MIN_SPEED + speed * (MAX_SPEED - MIN_SPEED);

} // SetSpeed

void VoicePrint::Render()
{
	float mySpeed = m_speed;// * (g_timePass / (1.0f/50.0f));

	//---------------------------------------------------------------------------
	// Update spectrum texture
	D3DLOCKED_RECT	lockedRect;
	m_spectrumTexture->LockRect(0, &lockedRect, NULL, 0);

	unsigned char* data = (unsigned char*)lockedRect.pBits; 

	for (int i=0; i<512; i++)
	{
		int val = ((int)(GetSpecLeft(i) * 0.8f * 255.0f));
		val = min(val, 255);
		val = max(0, val);

		float xVal = (m_minX * 255) + (val * (m_maxX - m_minX));
		float yVal = ((m_minY * 255) + (val * (m_maxY - m_minY)));

		data[3] = (char)yVal; // r = y
		data[2] = (char)xVal; // a = x
		data += lockedRect.Pitch;
	}
	m_spectrumTexture->UnlockRect( 0 );

	//---------------------------------------------------------------------------
	// Shift voiceprint texture to the left a bit
	if ( m_iCurrentTexture == 0 )
	{
		Renderer::SetRenderTarget( m_tex1 );
		Renderer::SetTexture( m_tex2 );
		m_texture = m_tex1;
	}
	else
	{
		Renderer::SetRenderTarget( m_tex2 );
		Renderer::SetTexture( m_tex1 );
		m_texture = m_tex2;
	}
	m_iCurrentTexture = 1 - m_iCurrentTexture;


	Renderer::SetDrawMode2d();

	DiffuseUVVertexShader* pShader = &DiffuseUVVertexShader::StaticType;
	Renderer::CommitTransforms( pShader );
	Renderer::CommitTextureState();
	Renderer::GetDevice()->SetVertexShader( pShader->GetVertexShader() );
	Renderer::GetDevice()->SetVertexDeclaration( g_pPosColUVDeclaration );

	for (int i = 0; i < 2; i++)
	{
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	}

	PosColNormalUVVertex v[4];

	v[0].Coord.x = -1 - mySpeed;
	v[0].Coord.y = -1;
	v[0].Coord.z = 0.0f;
//	v[0].Diffuse = 0xffffffff;
	v[0].FDiffuse.x = 1.0f;
	v[0].FDiffuse.y = 1.0f;
	v[0].FDiffuse.z = 1.0f;
	v[0].FDiffuse.w = 1.0f;
	v[0].u = 0.0f + (0.5f / 512);
	v[0].v = 0.0f + (0.5f / 512);

	v[1].Coord.x = 1 - mySpeed;
	v[1].Coord.y = -1;
	v[1].Coord.z = 0.0f;
//	v[1].Diffuse = 0xffffffff;
	v[1].FDiffuse = v[0].FDiffuse;
	v[1].u = 1.0f + (0.5f / 512);
	v[1].v = 0.0f + (0.5f / 512);

	v[2].Coord.x = 1 - mySpeed;
	v[2].Coord.y = 1;
	v[2].Coord.z = 0.0f;
//	v[2].Diffuse = 0xffffffff;
	v[2].FDiffuse = v[0].FDiffuse;
	v[2].u = 1.0f + (0.5f / 512);
	v[2].v = 1.0f + (0.5f / 512);

	v[3].Coord.x = -1 - mySpeed;
	v[3].Coord.y = 1;
	v[3].Coord.z = 0.0f;
//	v[3].Diffuse = 0xffffffff;
	v[3].FDiffuse = v[0].FDiffuse;
	v[3].u = 0.0f + (0.5f / 512);
	v[3].v = 1.0f + (0.5f / 512);

	Renderer::GetDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, &v, sizeof(PosColNormalUVVertex));


	//---------------------------------------------------------------------------
	// Draw this frame's voiceprint data to right hand side of voice print texture
	Renderer::SetTexture(m_spectrumTexture);
 	Renderer::GetDevice()->SetTexture(1, m_colourMap);
	ColourRemapPixelShader* pPShader = &ColourRemapPixelShader::StaticType;

	Renderer::GetDevice()->SetPixelShader( pPShader->GetPixelShader() );
	v[0].Coord.x = (1-mySpeed);
	v[0].Coord.y = (-1);
	v[1].Coord.x = (1);
	v[1].Coord.y = (-1);
	v[2].Coord.x = (1);
	v[2].Coord.y = (1);
	v[3].Coord.x = (1-mySpeed);
	v[3].Coord.y = (1);

	Renderer::GetDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, &v, sizeof(PosColNormalUVVertex));
	for (int i = 0; i < 2; i++)
	{
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		Renderer::GetDevice()->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	}
	Renderer::GetDevice()->SetTexture(1, NULL);
	Renderer::SetTexture(NULL);
	Renderer::GetDevice()->SetPixelShader( NULL );
	Renderer::SetRenderTargetBackBuffer();

} // Render

//-----------------------------------------------------------------------------
// Script interface

VoicePrint* VoicePrint_Factory()
{
	/* The class constructor is initializing the reference counter to 1*/
	return new VoicePrint();
}

void VoicePrint::RegisterScriptInterface( asIScriptEngine* pScriptEngine )
{
#ifndef assert
#define assert
#endif

	int r;

	// Registering the reference type
	r = pScriptEngine->RegisterObjectType("VoicePrint", 0, asOBJ_REF);	 assert(r >= 0);

	// Registering the factory behaviour
	r = pScriptEngine->RegisterObjectBehaviour("VoicePrint", asBEHAVE_FACTORY, "VoicePrint@ f()", asFUNCTION(VoicePrint_Factory), asCALL_CDECL); assert( r >= 0 );

	// Registering the addref/release behaviours
	r = pScriptEngine->RegisterObjectBehaviour("VoicePrint", asBEHAVE_ADDREF, "void f()", asMETHOD(VoicePrint, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectBehaviour("VoicePrint", asBEHAVE_RELEASE, "void f()", asMETHOD(VoicePrint, Release), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectMethod("VoicePrint", "void Render()", asMETHOD(VoicePrint, Render), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectMethod("VoicePrint", "void LoadColourMap(string& in)", asMETHOD(VoicePrint, LoadColourMap), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectMethod("VoicePrint", "void SetSpeed(float)", asMETHOD(VoicePrint, SetSpeed), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectMethod("VoicePrint", "void SetRect(float, float, float, float)", asMETHOD(VoicePrint, SetRect), asCALL_THISCALL); assert(r >= 0);

	r = pScriptEngine->RegisterObjectBehaviour("VoicePrint", asBEHAVE_IMPLICIT_REF_CAST, "EffectBase@ f()", asFUNCTION((refCast<VoicePrint,EffectBase>)), asCALL_CDECL_OBJLAST); assert( r >= 0 );

}
