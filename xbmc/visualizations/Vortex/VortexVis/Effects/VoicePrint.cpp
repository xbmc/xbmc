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
#include "CommonStates.h"

namespace
{
  #include "../Shaders/ColourRemapPixelShader.inc"
}

class ColourRemapPixelShader : public Shader
{
	DECLARE_SHADER(ColourRemapPixelShader);
public:
	ColourRemapPixelShader( EShaderType ShaderType, const void* pShaderCode, unsigned int iCodeLen ) : Shader( ShaderType, pShaderCode, iCodeLen ){};
};

IMPLEMENT_SHADER(ColourRemapPixelShader, ColourRemapPixelShaderCode, sizeof(ColourRemapPixelShaderCode), ST_PIXEL);

// TODO
float GetSpecLeft(int index);
extern float g_timePass;

VoicePrint::VoicePrint()
{
	m_iCurrentTexture = 0;
	m_tex1 = Renderer::CreateTexture(512, 512);
	m_tex2 = Renderer::CreateTexture(512, 512);
	m_colourMap = NULL;

  m_spectrumTexture = Renderer::CreateTexture(1, 512, DXGI_FORMAT_B8G8R8A8_UNORM, true);

	m_speed = 0.008f;
	m_minX = 0.99f;
	m_maxX = 0.01f;
	m_minY = 0.5f;
	m_maxY = 0.5f;

} // Constructor

VoicePrint::~VoicePrint()
{
	if ( m_colourMap )
    m_colourMap->Release();

	if ( m_tex1 )
		m_tex1->Release();

	if ( m_tex2 )
		m_tex2->Release();

	if ( m_spectrumTexture )
		m_spectrumTexture->Release();

} // Destructor

void VoicePrint::LoadColourMap(std::string& filename)
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
	D3D11_MAPPED_SUBRESOURCE	lockedRect;
  if (m_spectrumTexture->LockRect(0, &lockedRect))
  {
    unsigned char* data = (unsigned char*)lockedRect.pData;

    for (int i = 0; i < 512; i++)
    {
      int val = ((int)(GetSpecLeft(i) * 0.8f * 255.0f));
      val = min(val, 255);
      val = max(0, val);

      float xVal = (m_minX * 255) + (val * (m_maxX - m_minX));
      float yVal = ((m_minY * 255) + (val * (m_maxY - m_minY)));

      // 0 - b, 1 - g, 2 - r, 3 - a
      data[3] = (char)yVal; // r = y
      data[2] = (char)xVal; // a = x
      data += lockedRect.RowPitch;
    }
    m_spectrumTexture->UnlockRect(0);
  }

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
  Renderer::SetShader( pShader );
  Renderer::CommitTransforms( pShader );
	Renderer::CommitTextureState();
  {
    ID3D11SamplerState* states[1] = { Renderer::GetStates()->PointClamp() };
    Renderer::GetContext()->PSSetSamplers(0, ARRAYSIZE(states), states);
  }

	PosColNormalUVVertex v[4];

	v[0].Coord.x = -1 - mySpeed;
	v[0].Coord.y = -1;
	v[0].Coord.z = 0.0f;
	v[0].FDiffuse.x = 1.0f;
	v[0].FDiffuse.y = 1.0f;
	v[0].FDiffuse.z = 1.0f;
	v[0].FDiffuse.w = 1.0f;
	v[0].u = 0.0f + (0.5f / 512);
	v[0].v = 0.0f + (0.5f / 512);

	v[1].Coord.x = 1 - mySpeed;
	v[1].Coord.y = -1;
	v[1].Coord.z = 0.0f;
	v[1].FDiffuse = v[0].FDiffuse;
	v[1].u = 1.0f + (0.5f / 512);
	v[1].v = 0.0f + (0.5f / 512);

	v[2].Coord.x = 1 - mySpeed;
	v[2].Coord.y = 1;
	v[2].Coord.z = 0.0f;
	v[2].FDiffuse = v[0].FDiffuse;
	v[2].u = 1.0f + (0.5f / 512);
	v[2].v = 1.0f + (0.5f / 512);

	v[3].Coord.x = -1 - mySpeed;
	v[3].Coord.y = 1;
	v[3].Coord.z = 0.0f;
	v[3].FDiffuse = v[0].FDiffuse;
	v[3].u = 0.0f + (0.5f / 512);
	v[3].v = 1.0f + (0.5f / 512);

  Renderer::UpdateVBuffer(v, 4);
  Renderer::DrawPrimitive(D3DPT_TRIANGLEFAN, 6, 0);

	//---------------------------------------------------------------------------
	// Draw this frame's voiceprint data to right hand side of voice print texture
  ColourRemapPixelShader* pPShader = &ColourRemapPixelShader::StaticType;
  Renderer::SetShader(pPShader);
  Renderer::SetTexture(m_spectrumTexture);
  Renderer::SetShaderTexture(1, m_colourMap);

  v[0].Coord.x = (1-mySpeed);
	v[0].Coord.y = (-1);
	v[1].Coord.x = (1);
	v[1].Coord.y = (-1);
	v[2].Coord.x = (1);
	v[2].Coord.y = (1);
	v[3].Coord.x = (1-mySpeed);
	v[3].Coord.y = (1);

  Renderer::UpdateVBuffer(v, 4);
  Renderer::DrawPrimitive(D3DPT_TRIANGLEFAN, 6, 0);
  {
    ID3D11SamplerState* states[1] = { Renderer::GetStates()->LinearWrap() };
    Renderer::GetContext()->PSSetSamplers(0, ARRAYSIZE(states), states);
  }
  Renderer::SetShaderTexture(1, NULL);
  Renderer::SetTexture(NULL);
  Renderer::CommitTextureState();        // need to unbind color remap shader
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
