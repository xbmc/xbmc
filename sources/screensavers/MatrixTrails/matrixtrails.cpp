////////////////////////////////////////////////////////////////////////////
//
// Matrix Trails Screensaver for XBox Media Center
// Copyright (c) 2005 Joakim Eriksson <je@plane9.com>
//
////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "matrixtrails.h"

////////////////////////////////////////////////////////////////////////////
//
CMatrixTrails::CMatrixTrails()
{
	m_NumColumns	= gConfig.m_NumColumns;
	m_NumRows		= gConfig.m_NumRows;

	m_Columns = new CColumn[m_NumColumns];
	for (int cNr=0; cNr<m_NumColumns; cNr++)
	{
		m_Columns[cNr].Init(m_NumRows);
	}

	m_Texture = null;
	m_VertexBuffer = null;
}

////////////////////////////////////////////////////////////////////////////
//
CMatrixTrails::~CMatrixTrails()
{
	SAFE_DELETE_ARRAY(m_Columns);
}

////////////////////////////////////////////////////////////////////////////
//
bool	CMatrixTrails::RestoreDevice(CRenderD3D* render)
{
	LPDIRECT3DDEVICE8	d3dDevice	= render->GetDevice();

	int numQuads	= m_NumRows*m_NumColumns;
	d3dDevice->CreateVertexBuffer( 4*numQuads*sizeof(TRenderVertex), D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, TRenderVertex::FVF_Flags, D3DPOOL_DEFAULT, &m_VertexBuffer );

#ifdef _TEST
	DVERIFY(D3DXCreateTextureFromFileA(d3dDevice, "matrixtrails.tga", &m_Texture))
#else
	DVERIFY(D3DXCreateTextureFromFileA(d3dDevice, "q:\\screensavers\\matrixtrails.tga", &m_Texture))
#endif

	m_CharSize.x	= (f32)render->m_Width  / (f32)m_NumColumns;
	m_CharSize.y	= (f32)render->m_Height / (f32)m_NumRows;
	m_CharSize.z	= 0.0f;

	return true;
}

////////////////////////////////////////////////////////////////////////////
//
void	CMatrixTrails::InvalidateDevice(CRenderD3D* render)
{
	SAFE_RELEASE( m_Texture ); 
	SAFE_RELEASE( m_VertexBuffer ); 
}

////////////////////////////////////////////////////////////////////////////
//
void		CMatrixTrails::Update(f32 dt)
{
	for (int cNr=0; cNr<m_NumColumns; cNr++)
	{
		m_Columns[cNr].Update(dt);
	}	
}

////////////////////////////////////////////////////////////////////////////
//
bool		CMatrixTrails::Draw(CRenderD3D* render)
{
	LPDIRECT3DDEVICE8	d3dDevice	= render->GetDevice();

	// Fill	in the vertex buffers with the quads
	TRenderVertex* vert	= NULL;
	m_VertexBuffer->Lock( 0, 0, (BYTE**)&vert, 0);

	f32 posX = 0, posY = 0;
	for (int cNr=0; cNr<m_NumColumns; cNr++)
	{
		vert = m_Columns[cNr].UpdateVertexBuffer(vert, posX, posY, m_CharSize, gConfig.m_CharSizeTex);
		posX += m_CharSize.x;
	}	

	m_VertexBuffer->Unlock();

	// Setup our texture
	d3dSetTextureStageState(0, D3DTSS_COLOROP,	 D3DTOP_MODULATE);
	d3dSetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	d3dSetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	d3dSetTextureStageState(0, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);
	d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(1, D3DTSS_COLOROP,	 D3DTOP_DISABLE);
	d3dSetTextureStageState(1, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);

	d3dSetRenderState(D3DRS_ZENABLE,	FALSE);
	d3dSetRenderState(D3DRS_LIGHTING,	FALSE);
	d3dSetRenderState(D3DRS_COLORVERTEX,TRUE);
	d3dSetRenderState(D3DRS_FILLMODE,    D3DFILL_SOLID );
	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	d3dDevice->SetTexture( 0, m_Texture );
	d3dDevice->SetStreamSource(	0, m_VertexBuffer, sizeof(TRenderVertex) );
	d3dDevice->SetVertexShader( TRenderVertex::FVF_Flags	);

	CMatrix	m;
	m.Identity();
	d3dDevice->SetTransform(D3DTS_PROJECTION, &m);
	d3dDevice->SetTransform(D3DTS_VIEW, &m);
	d3dDevice->SetTransform(D3DTS_WORLD, &m);

	// TODO: Add an index buffer and just do a single draw call here instead or use quads on xbox
	//		 However quad lists doesn't exists on DX PC so then the 'test' build wont work anymore
	int	index = 0;
	for (int cNr=0; cNr<m_NumColumns; cNr++)
	{
		for	(int rNr=0;	rNr<m_NumRows; rNr++)
		{
			d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,	index, 2 );
			index += 4;
		}
	}

	return true;
}





