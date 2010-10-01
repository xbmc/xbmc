/*
 *      Copyright (C) 2010 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "VoicePrint.h"
#include <stdio.h>

float GetSpecLeft(int index);

VoicePrint_c::VoicePrint_c()
{


	m_texture = Renderer_c::CreateTexture(512, 512);
	m_colourMap = NULL;

	m_spectrumTexture = Renderer_c::CreateTexture(1, 512, D3DFMT_A8R8G8B8);

	m_speed = 0.008f;
	m_minX = 0.99f;
	m_maxX = 0.01f;
	m_minY = 0.5f;
	m_maxY = 0.5f;

} // Constructor

VoicePrint_c::~VoicePrint_c()
{

	if (m_colourMap)
	{
		Renderer_c::ReleaseTexture(m_colourMap);
	}

	if (m_texture)
		m_texture->Release();

	if (m_spectrumTexture)
		m_spectrumTexture->Release();

} // Destructor

void VoicePrint_c::LoadColourMap(char* &filename)
{
	char fullname[256];
#ifdef STAND_ALONE
	sprintf(fullname, "d:\\Textures\\%s", filename);
#else
	sprintf(fullname, "q:\\Visualisations\\Vortex\\Textures\\%s", filename);
#endif
	m_colourMap = Renderer_c::LoadTexture(fullname);

} // LoadColourMap

void VoicePrint_c::SetRect(float minX, float minY, float maxX, float maxY)
{
	m_minX = minX;
	m_minY = minY;
	m_maxX = maxX;
	m_maxY = maxY;

} // SetRect

void VoicePrint_c::SetSpeed(float speed)
{
	const float MIN_SPEED = 0.004f;
	const float MAX_SPEED = 0.02f;

	speed = min(speed, 1.0f);
	speed = max(speed, 0.0f);

	m_speed = MIN_SPEED + speed * (MAX_SPEED - MIN_SPEED);

} // SetSpeed

void VoicePrint_c::Render()
{
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

		data[2] = (char)yVal; // r = y
		data[3] = (char)xVal; // a = x
		data += lockedRect.Pitch;
	}


	Renderer_c::SetRenderTarget(m_texture);
	Renderer_c::CopyToScratch();

	Renderer_c::SetDrawMode2d();
	Renderer_c::SetTexture(SCRATCH_TEXTURE);
	Renderer_c::CommitStates();

	d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
	d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_POINT);
	d3dSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
	d3dSetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

	Vertex_t v[4];

	v[0].coord.x = -1 - m_speed;
	v[0].coord.y = -1;
	v[0].coord.z = 0.0f;
	v[0].colour = 0xffffffff;
	v[0].u = 0.0f + (0.5f / 512);
	v[0].v = 0.0f + (0.5f / 512);

	v[1].coord.x = 1 - m_speed;
	v[1].coord.y = -1;
	v[1].coord.z = 0.0f;
	v[1].colour = 0xffffffff;
	v[1].u = 1.0f + (0.5f / 512);
	v[1].v = 0.0f + (0.5f / 512);

	v[2].coord.x = 1 - m_speed;
	v[2].coord.y = 1;
	v[2].coord.z = 0.0f;
	v[2].colour = 0xffffffff;
	v[2].u = 1.0f + (0.5f / 512);
	v[2].v = 1.0f + (0.5f / 512);

	v[3].coord.x = -1 - m_speed;
	v[3].coord.y = 1;
	v[3].coord.z = 0.0f;
	v[3].colour = 0xffffffff;
	v[3].u = 0.0f + (0.5f / 512);
	v[3].v = 1.0f + (0.5f / 512);

	Renderer_c::CommitStates();
	Renderer_c::GetDevice()->DrawPrimitiveUP(D3DPT_QUADLIST, 1, &v, sizeof(Vertex_t));

	Renderer_c::SetTexture(m_spectrumTexture);
	Renderer_c::GetDevice()->SetTexture(1, m_colourMap);
	Renderer_c::SetPShader(PSHADER_COLREMAP);

	v[0].coord.x = (1-m_speed);
	v[0].coord.y = (-1);
	v[1].coord.x = (1);
	v[1].coord.y = (-1);
	v[2].coord.x = (1);
	v[2].coord.y = (1);
	v[3].coord.x = (1-m_speed);
	v[3].coord.y = (1);

	Renderer_c::CommitStates();
	Renderer_c::GetDevice()->DrawPrimitiveUP(D3DPT_QUADLIST, 1, &v, sizeof(Vertex_t));

	Renderer_c::GetDevice()->SetPixelShader(NULL);
	Renderer_c::GetDevice()->SetTexture(0, NULL);
	Renderer_c::SetTexture(NULL);
	d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	d3dSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	d3dSetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);

} // Render