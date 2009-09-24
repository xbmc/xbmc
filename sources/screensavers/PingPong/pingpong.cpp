////////////////////////////////////////////////////////////////////////////
//
// PingPong Screensaver for XBox Media Center
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
#include "pingpong.h"

#define NUMQUADS	3

#define TOPANDBOTTOM	34.0f
#define PADDLEPOSX		40.0f
#define PADDLEMAXSPEED	20.0f

////////////////////////////////////////////////////////////////////////////
//
CPingPong::CPingPong()
{
	m_Paddle[0].m_Pos.Set(-PADDLEPOSX, 0.0f, 0.0f);
	m_Paddle[1].m_Pos.Set( PADDLEPOSX, 0.0f, 0.0f);

	m_Ball.m_Pos.Set(0.0f, 0.0f, 0.0f);
	m_Ball.m_Vel.Set(20.0f, 20.0f, 0.0f);	
}

////////////////////////////////////////////////////////////////////////////
//
CPingPong::~CPingPong()
{
}

////////////////////////////////////////////////////////////////////////////
//
bool	CPingPong::RestoreDevice(CRenderD3D* render)
{
	LPDIRECT3DDEVICE8	d3dDevice	= render->GetDevice();
	d3dDevice->CreateVertexBuffer( 4*NUMQUADS*sizeof(TRenderVertex), D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, TRenderVertex::FVF_Flags, D3DPOOL_DEFAULT, &m_VertexBuffer );
	return true;
}

////////////////////////////////////////////////////////////////////////////
//
void	CPingPong::InvalidateDevice(CRenderD3D* render)
{
	SAFE_RELEASE( m_VertexBuffer ); 
}

////////////////////////////////////////////////////////////////////////////
//
void		CPingPong::Update(f32 dt)
{
	// The paddle 'ai'. If you now can call it that
	for (int i=0; i<2; i++)
	{
		f32 speed = 0.5f;
		// If the ball is moving toward us then meet up with it (quickly)
		if (DotProduct(m_Ball.m_Vel, m_Paddle[i].m_Pos) > 0.0f)
		{
			speed = 1.0f;
		}

		if (m_Ball.m_Pos.y > m_Paddle[i].m_Pos.y)
			m_Paddle[i].m_Pos.y += PADDLEMAXSPEED*dt*speed;
		else
			m_Paddle[i].m_Pos.y -= PADDLEMAXSPEED*dt*speed;
	}

	// Perform collisions
	if (m_Ball.m_Pos.y > TOPANDBOTTOM)		m_Ball.m_Vel.y *= -1.0f;
	if (m_Ball.m_Pos.y <-TOPANDBOTTOM)		m_Ball.m_Vel.y *= -1.0f;

	if ((m_Ball.m_Pos.x-m_Ball.m_Size.x) < (m_Paddle[0].m_Pos.x+m_Paddle[0].m_Size.x))	m_Ball.m_Vel.x *= -1.0f;
	if ((m_Ball.m_Pos.x+m_Ball.m_Size.x) > (m_Paddle[1].m_Pos.x-m_Paddle[1].m_Size.x))	m_Ball.m_Vel.x *= -1.0f;
	
	m_Ball.m_Pos += m_Ball.m_Vel*dt;
}

////////////////////////////////////////////////////////////////////////////
//
bool		CPingPong::Draw(CRenderD3D* render)
{
	LPDIRECT3DDEVICE8	d3dDevice	= render->GetDevice();

	D3DXMATRIX mProjection;
	D3DXMatrixPerspectiveFovLH(	&mProjection, DEGTORAD(	60.0f ), (f32)render->m_Width / (f32)render->m_Height, 0.1f, 2000.0f );
	d3dDevice->SetTransform( D3DTS_PROJECTION, &mProjection );

	// Fill	in the vertex buffers with the quads
	TRenderVertex* vert	= NULL;
	m_VertexBuffer->Lock( 0, 0, (BYTE**)&vert, 0);

	vert = AddQuad(vert, m_Ball.m_Pos, m_Ball.m_Size, m_Ball.m_Col.RenderColor());
	vert = AddQuad(vert, m_Paddle[0].m_Pos, m_Paddle[0].m_Size, m_Paddle[0].m_Col.RenderColor());
	vert = AddQuad(vert, m_Paddle[1].m_Pos, m_Paddle[1].m_Size, m_Paddle[1].m_Col.RenderColor());

	m_VertexBuffer->Unlock();

	d3dSetRenderState(D3DRS_ZENABLE,	FALSE);
	d3dSetRenderState(D3DRS_LIGHTING,	FALSE);
	d3dSetRenderState(D3DRS_COLORVERTEX,TRUE);
	d3dSetRenderState(D3DRS_FILLMODE,    D3DFILL_SOLID );
	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	d3dSetTextureStageState(0, D3DTSS_COLOROP,	 D3DTOP_SELECTARG1);
	d3dSetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	d3dSetTextureStageState(1, D3DTSS_COLOROP,	 D3DTOP_DISABLE);
	d3dSetTextureStageState(1, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);

	d3dDevice->SetTexture( 0, NULL );
	d3dDevice->SetStreamSource(	0, m_VertexBuffer, sizeof(TRenderVertex) );
	d3dDevice->SetVertexShader( TRenderVertex::FVF_Flags	);

	CMatrix	m;
	m.Identity();
	d3dDevice->SetTransform(D3DTS_VIEW, (D3DXMATRIX*)&m);
	m._43 = 70.0f;
	d3dDevice->SetTransform(D3DTS_WORLD, (D3DXMATRIX*)&m);

	for	(int pnr=0;	pnr<NUMQUADS; pnr++)
	{
		d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,	pnr*4, 2 );
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////
// Adds a quad to a vertex buffer
//
TRenderVertex*	CPingPong::AddQuad(TRenderVertex* vert, const CVector& pos, const CVector& size, DWORD col)
{
	vert->pos =	CVector(pos.x-size.x, pos.y+size.y, 0.0f);	vert->col =	col; vert++;
	vert->pos =	CVector(pos.x-size.x, pos.y-size.y, 0.0f);	vert->col =	col; vert++;
	vert->pos =	CVector(pos.x+size.x, pos.y+size.y, 0.0f);	vert->col =	col; vert++;
	vert->pos =	CVector(pos.x+size.x, pos.y-size.y, 0.0f);	vert->col =	col; vert++;
	return vert;
}







