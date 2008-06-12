////////////////////////////////////////////////////////////////////////////
//
// Planestate Screensaver for XBox Media Center
// Copyright (c) 2005 Joakim Eriksson <je@plane9.com>
//
////////////////////////////////////////////////////////////////////////////
//
// Just renders a simple cube with a star texture
// The texture used is generated in here so we dont have any dependencies
// on extra bitmaps outside the screensaver
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "background.h"

typedef	struct	TBGRenderVertex
{
	f32			x,y,z;
	DWORD		col;
	f32			u, v;
	enum FVF {	FVF_Flags =	D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1	};
} TBGRenderVertex;

#define	UVSCALE	4.0f
TBGRenderVertex	gBGVertices[] =
{
	// Quad	0
	{-1.0f,	1.0f,-1.0f,	 0xffffffff, 0.0f, 0.0f	},
	{-1.0f,-1.0f,-1.0f,	 0xffffffff, UVSCALE, 0.0f },
	{ 1.0f,	1.0f,-1.0f,	 0xffffffff, 0.0f, UVSCALE },
	{ 1.0f,-1.0f,-1.0f,	 0xffffffff, UVSCALE, UVSCALE  },

	// Quad	1
	{-1.0f,	1.0f, 1.0f,	 0xffffffff, 0.0f, 0.0f	},
	{ 1.0f,	1.0f, 1.0f,	 0xffffffff, UVSCALE, 0.0f },
	{-1.0f,-1.0f, 1.0f,	 0xffffffff, 0.0f, UVSCALE },
	{ 1.0f,-1.0f, 1.0f,	 0xffffffff, UVSCALE, UVSCALE },

	// Quad	2
	{-1.0f,	1.0f, 1.0f,	 0xffffffff, 0.0f, 0.0f	},
	{-1.0f,	1.0f,-1.0f,	 0xffffffff, UVSCALE, 0.0f },
	{ 1.0f,	1.0f, 1.0f,	 0xffffffff, 0.0f, UVSCALE },
	{ 1.0f,	1.0f,-1.0f,	 0xffffffff, UVSCALE, UVSCALE },

	// Quad	3
	{-1.0f,-1.0f, 1.0f,	 0xffffffff, 0.0f, 0.0f	},
	{ 1.0f,-1.0f, 1.0f,	 0xffffffff, UVSCALE, 0.0f },
	{-1.0f,-1.0f,-1.0f,	 0xffffffff, 0.0f, UVSCALE },
	{ 1.0f,-1.0f,-1.0f,	 0xffffffff, UVSCALE, UVSCALE },

	// Quad	4
	{ 1.0f,	1.0f,-1.0f,	 0xffffffff, 0.0f, 0.0f	},
	{ 1.0f,-1.0f,-1.0f,	 0xffffffff, UVSCALE, 0.0f },
	{ 1.0f,	1.0f, 1.0f,	 0xffffffff, 0.0f, UVSCALE },
	{ 1.0f,-1.0f, 1.0f,	 0xffffffff, UVSCALE, UVSCALE },

	// Quad	5
	{-1.0f,	1.0f,-1.0f,	 0xffffffff, 0.0f, 0.0f	},
	{-1.0f,	1.0f, 1.0f,	 0xffffffff, UVSCALE, 0.0f },
	{-1.0f,-1.0f,-1.0f,  0xffffffff, 0.0f, UVSCALE},
	{-1.0f,-1.0f, 1.0f,	 0xffffffff, UVSCALE, UVSCALE  }
};

////////////////////////////////////////////////////////////////////////////
//
CBackground::CBackground() : m_RotAnim(0, CVector(0.0f,	0.0f, 0.0f))
{
	m_RotAnim.m_Values[0].SetMinMax(0.0f, 360.0f);	m_RotAnim.m_Values[0].m_AnimMode = AM_RAND;	m_RotAnim.m_Values[0].SetMinMaxITime(20.0f,	40.0f, AM_RAND);
	m_RotAnim.m_Values[1].SetMinMax(0.0f, 360.0f);	m_RotAnim.m_Values[1].m_AnimMode = AM_RAND;	m_RotAnim.m_Values[1].SetMinMaxITime(20.0f,	40.0f, AM_RAND);
	m_RotAnim.m_Values[2].SetMinMax(0.0f, 360.0f);	m_RotAnim.m_Values[2].m_AnimMode = AM_RAND;	m_RotAnim.m_Values[2].SetMinMaxITime(20.0f,	40.0f, AM_RAND);
}

////////////////////////////////////////////////////////////////////////////
//
CBackground::~CBackground()
{
}

////////////////////////////////////////////////////////////////////////////
//
bool	CBackground::RestoreDevice(CRenderD3D* render)
{
	LPDIRECT3DDEVICE8	d3dDevice	= render->GetDevice();

	d3dDevice->CreateVertexBuffer( 24*sizeof(TBGRenderVertex), D3DUSAGE_WRITEONLY, TBGRenderVertex::FVF_Flags, D3DPOOL_DEFAULT,	&m_VertexBuffer);
	void *pVertices	= NULL;
	m_VertexBuffer->Lock( 0, sizeof(gBGVertices), (BYTE**)&pVertices, 0 );
	memcpy(	pVertices, gBGVertices,	sizeof(gBGVertices)	);
	m_VertexBuffer->Unlock();

	CreateTexture(render);
	return true;
}

////////////////////////////////////////////////////////////////////////////
//
void	CBackground::InvalidateDevice(CRenderD3D* render)
{
	SAFE_RELEASE( m_Texture ); 
	SAFE_RELEASE( m_VertexBuffer ); 
}

////////////////////////////////////////////////////////////////////////////
//
void		CBackground::Update(f32	dt)
{
	m_RotAnim.Update(dt);
}

////////////////////////////////////////////////////////////////////////////
//
bool		CBackground::Draw(CRenderD3D* render)
{
	LPDIRECT3DDEVICE8	d3dDevice	= render->GetDevice();

	d3dSetTextureStageState(0, D3DTSS_COLOROP,	 D3DTOP_SELECTARG1 );
	d3dSetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE	);
	d3dSetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT	);
	d3dSetTextureStageState(0, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE );
	d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSV,	 D3DTADDRESS_WRAP);
	d3dSetTextureStageState(1, D3DTSS_COLOROP,	 D3DTOP_DISABLE );
	d3dSetTextureStageState(1, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE );

	d3dSetRenderState(D3DRS_ZENABLE,	FALSE);
	d3dSetRenderState(D3DRS_LIGHTING,	FALSE);
	d3dSetRenderState(D3DRS_COLORVERTEX,FALSE);
	d3dSetRenderState(D3DRS_FOGENABLE,  FALSE );
	d3dSetRenderState(D3DRS_FILLMODE,   D3DFILL_SOLID );

	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	d3dDevice->SetTexture( 0, m_Texture );
	d3dDevice->SetStreamSource( 0, m_VertexBuffer, sizeof(TBGRenderVertex) );
	d3dDevice->SetVertexShader( TBGRenderVertex::FVF_Flags );

	CMatrix	m;
	CVector	r =	m_RotAnim.GetValue();
	m.Rotate(r.x, r.y, r.z);
	d3dDevice->SetTransform(D3DTS_WORLD, (D3DXMATRIX*)&m);

	d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,	0, 2 );
	d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,	4, 2 );
	d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,	8, 2 );
	d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 12, 2 );
	d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 16, 2 );
	d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 20, 2 );

	return true;
}

////////////////////////////////////////////////////////////////////////////
// Creates the background texture
// It's just a bunch of randomized stars
//
bool	CBackground::CreateTexture(CRenderD3D* render)
{
	LPDIRECT3DDEVICE8	d3dDevice	= render->GetDevice();

	// Create star texture
	s32		numStars = 50;
	s32		width =	TEXTURESIZE, height	= TEXTURESIZE;
	D3DLOCKED_RECT	rect;
	DVERIFY( d3dDevice->CreateTexture(width, height, 1,	0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_Texture) );

	// D3DFMT_A8R8G8B8 is swizzled but for some reason I can't get the linear format to work with D3DTADDRESS_WRAP 
	// Trying to use the linear format just locks up the xbox. However this is for a star texture so it doesnt matter if its swizzled

	m_Texture->LockRect(0, &rect, NULL, 0);

	for	(int y=0; y<height; y++)
	{
		memset(&((u8*)rect.pBits)[rect.Pitch*y], 0, sizeof(u32)*width);
	}
	
	int		pitch	= rect.Pitch/sizeof(u32);
	u32*	ptr	= (u32*) rect.pBits;
	for	(int i=0; i<numStars; i++)
	{
		s32	x =	Rand(width-2)+1;
		s32	y =	Rand(height-2)+1;
		f32	brightness = RandFloat();
		f32	brightness2 = brightness*RandFloat();

		ptr[x+(y*pitch)] = CRGBA(brightness, brightness, brightness, 1.0f).RenderColor();
		ptr[(x-1)+(y*pitch)] = ptr[(x+1)+(y*pitch)] = ptr[x+((y-1)*pitch)] = ptr[x+((y+1)*pitch)] = CRGBA(brightness2, brightness2, brightness2, 1.0f).RenderColor();
	}

	m_Texture->UnlockRect(0);

	return true;
}




