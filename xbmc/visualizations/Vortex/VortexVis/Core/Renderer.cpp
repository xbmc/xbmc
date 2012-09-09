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

#include "Renderer.h"
#include <d3dx9.h>
#include "DebugConsole.h"
#include <stdio.h>
#include "Shader.h"

D3DVERTEXELEMENT9 declPosNormalColUV[] =
{	// stream, offset, type, method, usage, usage index 
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
//	{ 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
	{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
	D3DDECL_END()
};

D3DVERTEXELEMENT9 declPosColUV[] =
{	// stream, offset, type, method, usage, usage index 
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
//	{ 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
	{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
	D3DDECL_END()
};

D3DVERTEXELEMENT9 declPosNormal[] =
{	// stream, offset, type, method, usage, usage index 
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
	D3DDECL_END()
};



LPDIRECT3DVERTEXDECLARATION9    g_pPosNormalColUVDeclaration = NULL;
LPDIRECT3DVERTEXDECLARATION9    g_pPosColUVDeclaration = NULL;
LPDIRECT3DVERTEXDECLARATION9    g_pPosNormalDeclaration = NULL;

namespace
{
	LPDIRECT3DVERTEXBUFFER9	g_pCubeVbuffer = NULL;
	LPDIRECT3DINDEXBUFFER9	g_pCubeIbuffer = NULL;
	LPDIRECT3DDEVICE9 m_pD3DDevice = NULL;
	IDirect3DSurface9*	g_backBuffer;
	IDirect3DSurface9*	g_depthBuffer;
	IDirect3DSurface9*	g_oldDepthBuffer;
	LPDIRECT3DTEXTURE9 m_pTextureFont = NULL;
	D3DVIEWPORT9  g_viewport;
	D3DXMATRIX  g_matProj;
	D3DXMATRIX  g_matView;
	float g_aspect;
	float g_defaultAspect;
	float g_aspectValue;
	float g_tex0U;
	float g_tex0V;
	DWORD g_colour;
	D3DXVECTOR4 g_fColour;
	D3DXVECTOR3 g_normal;
	ID3DXMatrixStack* g_matrixStack;
	int g_matrixStackLevel;
	bool g_envMapSet;
	bool g_textureSet;

	D3DPRIMITIVETYPE g_primType;

	LPDIRECT3DTEXTURE9 g_scratchTexture;
	HFONT g_hFont;

	// Vertices
#define MAX_VERTICES (65535)

	PosColNormalUVVertex g_vertices[ MAX_VERTICES ];
	int g_currentVertex;

	enum ETextureStageMode
	{
		TSM_COLOUR = 0,
		TSM_TEXTURE = 1,
		TSM_INVALID = -1
	};
	ETextureStageMode CurrentTextureStageMode = TSM_INVALID;
}

void Renderer::Init( LPDIRECT3DDEVICE9 pD3DDevice, int iXPos, int iYPos, int iWidth, int iHeight, float fPixelRatio )
{
	DebugConsole::Log("Renderer: Viewport x%d, y%d, w%d, h%d, pixel%f\n", iXPos, iYPos, iWidth, iHeight, fPixelRatio );
	m_pD3DDevice = pD3DDevice;
	g_viewport.X = iXPos;
	g_viewport.Y = iYPos;
	g_viewport.Width = iWidth;
	g_viewport.Height = iHeight;
	g_viewport.MinZ = 0;
	g_viewport.MaxZ = 1;

	g_defaultAspect = ((float)iWidth / iHeight) * fPixelRatio;

	m_pD3DDevice->CreateDepthStencilSurface(1024, 1024, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &g_depthBuffer, NULL);
/*
	if (D3D_OK != m_pD3DDevice->GetDepthStencilSurface(&g_oldDepthBuffer))
	{
		DebugConsole::Log("Renderer: No default depth buffer\n", iXPos, iYPos, iWidth, iHeight, fPixelRatio );
		//OutputDebugString("Vortex INFO: Renderer::Init - Failed to Get old depth stencil\n");
		// Create our own
// 		D3DSURFACE_DESC desc;
// 		g_backBuffer->GetDesc(&desc);
// 		g_device->CreateDepthStencilSurface(desc.Width, desc.Height, D3DFMT_LIN_D24S8, 0, &g_oldDepthBuffer);
// 		g_ownDepth = true;
	}
	else
	{
//		g_ownDepth = false;
	}
*/
	{
		char fullname[512];
		sprintf_s(fullname, 512, "%sfont.bmp", g_TexturePath );
		m_pTextureFont = LoadTexture( fullname, false );
	}

	D3DXCreateMatrixStack(0, &g_matrixStack);
	g_matrixStackLevel = 0;

	g_scratchTexture = CreateTexture( 512, 512 );

	CompileShaders();

	CreateCubeBuffers();

	CreateFonts();
}

void Renderer::CreateFonts()
{
	HDC hdc = CreateCompatibleDC( NULL );
	if( hdc == NULL )
		return;

	INT nHeight = -MulDiv( 12, GetDeviceCaps( hdc, LOGPIXELSY ), 72 );
	DeleteDC( hdc );

	BOOL bBold = FALSE;
	BOOL bItalic = FALSE;

	g_hFont = CreateFontA( nHeight, 0, 0, 0, bBold ? FW_BOLD : FW_NORMAL, bItalic, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		"Arial" );
}

LPD3DXMESH Renderer::CreateD3DXTextMesh( const char* pString, bool bCentered )
{
	HRESULT hr;
	LPD3DXMESH pMeshNew = NULL;

	HDC hdc = CreateCompatibleDC( NULL );
	if( hdc == NULL )
		return NULL;

	HFONT newfont=CreateFont(10,         //Height
		0,          //Width
		0,          //Escapement
		0,          //Orientation
		FW_NORMAL,  //Weight
		false,      //Italic
		false,      //Underline
		false,      //Strikeout
		DEFAULT_CHARSET,//Charset 
		OUT_DEFAULT_PRECIS,  //Output Precision
		CLIP_DEFAULT_PRECIS, //Clipping Precision
		DEFAULT_QUALITY,     //Quality
		DEFAULT_PITCH|FF_DONTCARE, //Pitch and Family
		"Arial");



	HFONT hFontOld;
	hFontOld = ( HFONT )SelectObject( hdc, newfont );
	hr = D3DXCreateText( m_pD3DDevice, hdc, pString, 0.001f, 0.2f, &pMeshNew, NULL, NULL );
	SelectObject( hdc, hFontOld );
	DeleteDC( hdc );

	if( SUCCEEDED( hr ) )
	{
		if( bCentered )
		{
			// Center text
			D3DXVECTOR3 vMin, vMax;

			PosNormalVertex* pVertices;
			pMeshNew->LockVertexBuffer( 0, reinterpret_cast<VOID**>(&pVertices));
			D3DXComputeBoundingBox( (D3DXVECTOR3*)pVertices, pMeshNew->GetNumVertices(), sizeof ( PosNormalVertex ), &vMin, &vMax );

			D3DXVECTOR3 vOffset;
			D3DXVec3Subtract( &vOffset, &vMax, &vMin );
			D3DXVec3Scale( &vOffset, &vOffset, 0.5f );
			for ( unsigned int i = 0; i < pMeshNew->GetNumVertices(); i++)
			{
				D3DXVec3Subtract( &pVertices[i].Coord, &pVertices[i].Coord, &vOffset );
			}
			pMeshNew->UnlockVertexBuffer();
		}
	}

	return pMeshNew;
}

void Renderer::DrawMesh( LPD3DXMESH	pMesh )
{
	if ( pMesh == NULL )
	{
		return;
	}

	UVNormalEnvVertexShader* pShader = &UVNormalEnvVertexShader::StaticType;
	CommitTransforms( pShader );
	CommitTextureState();
	m_pD3DDevice->SetVertexShader( pShader->GetVertexShader() );
	m_pD3DDevice->SetVertexDeclaration( g_pPosNormalDeclaration );

	pMesh->DrawSubset( 0 );
}

void Renderer::CreateCubeBuffers()
{
	m_pD3DDevice->CreateVertexBuffer( 24 * sizeof( PosColNormalUVVertex ),
									  D3DUSAGE_WRITEONLY,
									  0,
									  D3DPOOL_DEFAULT,
									  &g_pCubeVbuffer,
									  NULL );

	m_pD3DDevice->CreateIndexBuffer( 36 * sizeof( short ),
									 D3DUSAGE_WRITEONLY,
									 D3DFMT_INDEX16,
									 D3DPOOL_DEFAULT,
									&g_pCubeIbuffer,
									NULL );


	PosColNormalUVVertex* v;
	g_pCubeVbuffer->Lock( 0, 0, (void**)&v, 0 );

	short* indices;
	g_pCubeIbuffer->Lock( 0, 0, (void**)&indices, 0 );

	float nx = -1.0f;
	float ny = -1.0f;
	float nz = -1.0f;
	float x = 1.0f;
	float y = 1.0f;
	float z = 1.0f;

	g_colour = 0xfffffff;
	g_fColour = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);

	// Front Face
	v[0].Coord  = D3DXVECTOR3(nx, y, nz);
//	v[0].Diffuse = g_colour;
	v[0].FDiffuse = g_fColour;
	v[0].Normal = D3DXVECTOR3(0, 0, -1);
	v[0].u = 0;
	v[0].v = 0;
	v[1].Coord  = D3DXVECTOR3(x, y, nz);
//	v[1].Diffuse = g_colour;
	v[1].FDiffuse = g_fColour;
	v[1].Normal = D3DXVECTOR3(0, 0, -1);
	v[1].u = 1;
	v[1].v = 0;
	v[2].Coord  = D3DXVECTOR3(x, ny, nz);
//	v[2].Diffuse = g_colour;
	v[2].FDiffuse = g_fColour;
	v[2].Normal = D3DXVECTOR3(0, 0, -1);
	v[2].u = 1;
	v[2].v = 1;
	v[3].Coord  = D3DXVECTOR3(nx, ny, nz);
//	v[3].Diffuse = g_colour;
	v[3].FDiffuse = g_fColour;
	v[3].Normal = D3DXVECTOR3(0, 0, -1);
	v[3].u = 0;
	v[3].v = 1;

	*indices++ = 0;
	*indices++ = 1;
	*indices++ = 2;
	*indices++ = 0;
	*indices++ = 2;
	*indices++ = 3;

	// Back Face
	v[4].Coord  = D3DXVECTOR3(x, y, z);
//	v[4].Diffuse = g_colour;
	v[4].FDiffuse = g_fColour;
	v[4].Normal = D3DXVECTOR3(0, 0, 1);
	v[4].u = 0;
	v[4].v = 0;
	v[5].Coord  = D3DXVECTOR3(nx, y, z);
//	v[5].Diffuse = g_colour;
	v[5].FDiffuse = g_fColour;
	v[5].Normal = D3DXVECTOR3(0, 0, 1);
	v[5].u = 1;
	v[5].v = 0;
	v[6].Coord  = D3DXVECTOR3(nx, ny, z);
//	v[6].Diffuse = g_colour;
	v[6].FDiffuse = g_fColour;
	v[6].Normal = D3DXVECTOR3(0, 0, 1);
	v[6].u = 1;
	v[6].v = 1;
	v[7].Coord  = D3DXVECTOR3(x, ny, z);
//	v[7].Diffuse = g_colour;
	v[7].FDiffuse = g_fColour;
	v[7].Normal = D3DXVECTOR3(0, 0, 1);
	v[7].u = 0;
	v[7].v = 1;

	*indices++ = 4;
	*indices++ = 5;
	*indices++ = 6;
	*indices++ = 4;
	*indices++ = 6;
	*indices++ = 7;

	// Top Face
	v[8].Coord   = D3DXVECTOR3(nx, y, z);
//	v[8].Diffuse  = g_colour;
	v[8].FDiffuse = g_fColour;
	v[8].Normal  = D3DXVECTOR3(0, 1, 0);
	v[8].u = 0;
	v[8].v = 0;
	v[9].Coord   = D3DXVECTOR3(x, y, z);
//	v[9].Diffuse  = g_colour;
	v[9].FDiffuse = g_fColour;
	v[9].Normal  = D3DXVECTOR3(0, 1, 0);
	v[9].u = 1;
	v[9].v = 0;
	v[10].Coord  = D3DXVECTOR3(x, y, nz);
//	v[10].Diffuse = g_colour;
	v[10].FDiffuse = g_fColour;
	v[10].Normal = D3DXVECTOR3(0, 1, 0);
	v[10].u = 1;
	v[10].v = 1;
	v[11].Coord  = D3DXVECTOR3(nx, y, nz);
//	v[11].Diffuse = g_colour;
	v[11].FDiffuse = g_fColour;
	v[11].Normal = D3DXVECTOR3(0, 1, 0);
	v[11].u = 0;
	v[11].v = 1;

	*indices++ = 8;
	*indices++ = 9;
	*indices++ = 10;
	*indices++ = 8;
	*indices++ = 10;
	*indices++ = 11;

	// Bottom Face
	v[12].Coord  = D3DXVECTOR3(nx, ny, nz);
//	v[12].Diffuse = g_colour;
	v[12].FDiffuse = g_fColour;
	v[12].Normal = D3DXVECTOR3(0, -1, 0);
	v[12].u = 0;
	v[12].v = 0;
	v[13].Coord  = D3DXVECTOR3(x, ny, nz);
//	v[13].Diffuse = g_colour;
	v[13].FDiffuse = g_fColour;
	v[13].Normal = D3DXVECTOR3(0, -1, 0);
	v[13].u = 1;
	v[13].v = 0;
	v[14].Coord  = D3DXVECTOR3(x, ny, z);
//	v[14].Diffuse = g_colour;
	v[14].FDiffuse = g_fColour;
	v[14].Normal = D3DXVECTOR3(0, -1, 0);
	v[14].u = 1;
	v[14].v = 1;
	v[15].Coord  = D3DXVECTOR3(nx, ny, z);
//	v[15].Diffuse = g_colour;
	v[15].FDiffuse = g_fColour;
	v[15].Normal = D3DXVECTOR3(0, -1, 0);
	v[15].u = 0;
	v[15].v = 1;

	*indices++ = 12;
	*indices++ = 13;
	*indices++ = 14;
	*indices++ = 12;
	*indices++ = 14;
	*indices++ = 15;

	// Left Face
	v[16].Coord  = D3DXVECTOR3(nx, y, z);
//	v[16].Diffuse = g_colour;
	v[16].FDiffuse = g_fColour;
	v[16].Normal = D3DXVECTOR3(-1, 0, 0);
	v[16].u = 0;
	v[16].v = 0;
	v[17].Coord  = D3DXVECTOR3(nx, y, nz);
//	v[17].Diffuse = g_colour;
	v[17].FDiffuse = g_fColour;
	v[17].Normal = D3DXVECTOR3(-1, 0, 0);
	v[17].u = 1;
	v[17].v = 0;
	v[18].Coord  = D3DXVECTOR3(nx, ny, nz);
//	v[18].Diffuse = g_colour;
	v[18].FDiffuse = g_fColour;
	v[18].Normal = D3DXVECTOR3(-1, 0, 0);
	v[18].u = 1;
	v[18].v = 1;
	v[19].Coord  = D3DXVECTOR3(nx, ny, z);
//	v[19].Diffuse = g_colour;
	v[19].FDiffuse = g_fColour;
	v[19].Normal = D3DXVECTOR3(-1, 0, 0);
	v[19].u = 0;
	v[19].v = 1;

	*indices++ = 16;
	*indices++ = 17;
	*indices++ = 18;
	*indices++ = 16;
	*indices++ = 18;
	*indices++ = 19;

	// Right Face
	v[20].Coord  = D3DXVECTOR3(x, y, nz);
//	v[20].Diffuse = g_colour;
	v[20].FDiffuse = g_fColour;
	v[20].Normal = D3DXVECTOR3(1, 0, 0);
	v[20].u = 0;
	v[20].v = 0;
	v[21].Coord  = D3DXVECTOR3(x, y, z);
//	v[21].Diffuse = g_colour;
	v[21].FDiffuse = g_fColour;
	v[21].Normal = D3DXVECTOR3(1, 0, 0);
	v[21].u = 1;
	v[21].v = 0;
	v[22].Coord  = D3DXVECTOR3(x, ny, z);
//	v[22].Diffuse = g_colour;
	v[22].FDiffuse = g_fColour;
	v[22].Normal = D3DXVECTOR3(1, 0, 0);
	v[22].u = 1;
	v[22].v = 1;
	v[23].Coord  = D3DXVECTOR3(x, ny, nz);
//	v[23].Diffuse = g_colour;
	v[23].FDiffuse = g_fColour;
	v[23].Normal = D3DXVECTOR3(1, 0, 0);
	v[23].u = 0;
	v[23].v = 1;

	*indices++ = 20;
	*indices++ = 21;
	*indices++ = 22;
	*indices++ = 20;
	*indices++ = 22;
	*indices++ = 23;

	g_pCubeVbuffer->Unlock();
	g_pCubeIbuffer->Unlock();

}

void Renderer::Exit()
{
	DeleteObject( g_hFont );

	if ( m_pD3DDevice )
	{
		m_pD3DDevice->SetPixelShader( NULL );
		m_pD3DDevice->SetVertexShader( NULL );
	}

	if ( g_pPosNormalColUVDeclaration )
		g_pPosNormalColUVDeclaration->Release();

	if ( g_pPosColUVDeclaration )
		g_pPosColUVDeclaration->Release();

	if ( g_pPosNormalDeclaration )
		g_pPosNormalDeclaration->Release();

	Shader::ReleaseAllShaders();

	if ( g_pCubeIbuffer )
		g_pCubeIbuffer->Release();
	if ( g_pCubeVbuffer )
		g_pCubeVbuffer->Release();

// 	if ( g_backBuffer )
// 		g_backBuffer->Release();

// 	if ( g_oldDepthBuffer )
// 		g_oldDepthBuffer->Release();

	if ( g_matrixStack )
		g_matrixStack->Release();

	if ( g_scratchTexture )
	{
		g_scratchTexture->Release();
		g_scratchTexture = NULL;
	}

	if ( m_pTextureFont )
	{
		m_pTextureFont->Release();
		m_pTextureFont = NULL;
	}

	if ( g_depthBuffer )
		g_depthBuffer->Release();
}

//-- SetDrawMode2d ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetDrawMode2d()
{
	D3DXMATRIX Ortho2D;    
	D3DXMatrixOrthoLH(&Ortho2D, 2.0f, -2.0f, 0.0f, 1.0f);
	m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	Renderer::SetProjection( Ortho2D );

} // SetDrawMode2d

//-- SetDrawMode3d ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetDrawMode3d()
{
	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, g_aspect, 0.25f, 2000.0f );
	Renderer::SetProjection(matProj);
	m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

} // SetDrawMode3d

void Renderer::Clear( DWORD Col )
{
	Col |= 0xff000000;
	m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Col, 1.0f, 0 );
}

void Renderer::ClearDepth()
{
	m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
}

void Renderer::Rect( float x1, float y1, float x2, float y2, DWORD Col )
{
	unsigned char* pCol = (unsigned char*)&Col;
	D3DXVECTOR4 FColour(pCol[2],pCol[1], pCol[0], pCol[3] );

	PosColNormalUVVertex v[4];

	v[0].Coord.x = x1;
	v[0].Coord.y = y1;
	v[0].Coord.z = 0.0f;
//	v[0].Diffuse = Col;
	v[0].FDiffuse = FColour;

	v[1].Coord.x = x2;
	v[1].Coord.y = y1;
	v[1].Coord.z = 0.0f;
//	v[1].Diffuse = Col;
	v[1].FDiffuse = FColour;

	v[2].Coord.x = x2;
	v[2].Coord.y = y2;
	v[2].Coord.z = 0.0f;
//	v[2].Diffuse = Col;
	v[2].FDiffuse = FColour;

	v[3].Coord.x = x1;
	v[3].Coord.y = y2;
	v[3].Coord.z = 0.0f;
//	v[3].Diffuse = Col;
	v[3].FDiffuse = FColour;

	m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, &v, sizeof(PosColNormalUVVertex));

} // Rect

void Renderer::Line( float x1, float y1, float x2, float y2, DWORD Col )
{
	// TODO
/*	PosColNormalUVVertex v[2];

	v[0].Coord.x = x1;
	v[0].Coord.y = y1;
	v[0].Coord.z = 0.0f;
	v[0].Diffuse = Col;

	v[1].Coord.x = x2;
	v[1].Coord.y = y2;
	v[1].Coord.z = 0.0f;
	v[1].Diffuse = Col;

	m_pD3DDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, &v, sizeof(PosColNormalUVVertex));
*/
} // Line

void Renderer::Point( float x, float y, DWORD Col )
{
/*
	PosColNormalUVVertex v[2];

	v[0].Coord.x = x;
	v[0].Coord.y = y;
	v[0].Coord.z = 0.0f;
	v[0].Diffuse = Col;

	m_pD3DDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &v, sizeof(PosColNormalUVVertex));
*/
} // Point

LPDIRECT3DTEXTURE9 Renderer::CreateTexture( int iWidth, int iHeight, D3DFORMAT Format /* = D3DFMT_X8R8G8B8 */, bool bDynamic /* = false */ )
{
	LPDIRECT3DTEXTURE9 pTexture;
//	IDirect3DSurface9* pSurface;
//	D3DLOCKED_RECT lock;

	if ( bDynamic == false )
	{
		m_pD3DDevice->CreateTexture( iWidth, iHeight, 1, D3DUSAGE_RENDERTARGET, Format, D3DPOOL_DEFAULT, &pTexture, NULL );
	}
	else
	{
		m_pD3DDevice->CreateTexture( iWidth, iHeight, 1, D3DUSAGE_DYNAMIC, Format, D3DPOOL_DEFAULT, &pTexture, NULL );
	}

	if ( pTexture == NULL )
	{
		DebugConsole::Log("Renderer: Failed to create texture, Width:%d, Height:%d\n", iWidth, iHeight);
		return NULL;
	}

	// TODO: Clear target

// 	pTexture->GetSurfaceLevel( 0, &pSurface );
// 	pSurface->LockRect( &lock, NULL, 0 );
// 	int iTexSize = ( iWidth * iHeight * 4 );
// 	memset( lock.pBits, 0, iTexSize );
// 	pSurface->UnlockRect();
// 	pSurface->Release();
	return pTexture;
}

LPDIRECT3DTEXTURE9 Renderer::LoadTexture( char* pFilename, bool bAutoResize )
{
	LPDIRECT3DTEXTURE9 pTexture = NULL;

	FILE* iS;
	iS = fopen( pFilename, "rb" );
	if ( iS != NULL )
	{
		fseek( iS, 0, SEEK_END );
		int iLength = ftell( iS );
		fseek( iS, 0, SEEK_SET );
		BYTE* pTextureInMem = new BYTE[ iLength ];
		int iReadLength = fread( pTextureInMem, 1, iLength, iS );
		fclose( iS );

		D3DXCreateTextureFromFileInMemoryEx(m_pD3DDevice,		// device
											pTextureInMem,		// SrcData
											iReadLength,		// SrcDataSize
											0,					// Width
											0,					// Height
											1,					// MipLevels
											0,					// Usage
											D3DFMT_UNKNOWN,		// Format
											D3DPOOL_DEFAULT,	// Pool
											bAutoResize ? D3DX_FILTER_POINT : D3DX_FILTER_NONE,	// Filter
											D3DX_FILTER_NONE,	// MipFilter
											0x00000000,			// ColorKey
											NULL,				// SrcInfo
											NULL,				// Palette
											&pTexture );		// DestTexture

		delete[] pTextureInMem;
	}

	if ( pTexture == NULL )
	{
		DebugConsole::Log( "Renderer: Failed to load texture: %s\n", pFilename );
	}

	return pTexture;
}

void Renderer::ReleaseTexture( LPDIRECT3DTEXTURE9 pTexture )
{
	if ( pTexture )
	{
		pTexture->Release();
	}
}

void Renderer::DrawText( float x, float y, char* txt, DWORD Col )
{
	unsigned char* pCol = (unsigned char*)&Col;
	D3DXVECTOR4 FColour(pCol[2],pCol[1], pCol[0], pCol[3] );
//	D3DXVECTOR4 FColour(1,1, 0, 1 );

	SetTexture( m_pTextureFont );
	DiffuseUVVertexShader* pShader = &DiffuseUVVertexShader::StaticType;
	CommitTransforms( pShader );
	CommitTextureState();
	m_pD3DDevice->SetVertexShader( pShader->GetVertexShader() );
	m_pD3DDevice->SetVertexDeclaration( g_pPosColUVDeclaration );

	x = ( x / g_viewport.Width ) * 2 - 1;
	y = ( y / g_viewport.Height ) * 2 -1;
		
	PosColNormalUVVertex v[4];
	unsigned char ch;
	while (txt[0] != 0)
	{
		ch = txt[0];
		if (ch < 32)
			ch = 32;

		ch = ch - 32;

		float sizeX = 9.0f / 256.0f;
		float sizeY = 16.0f / 256.0f;

		float cx = (((ch % 16) * 9.0f) / 256.0f) + 0.5f / 256.0f;
		float cy = (((ch / 16) * 16.0f) / 256.0f) + 0.5f / 256.0f; 

		v[0].Coord.x = x;
		v[0].Coord.y = y;
		v[0].Coord.z = 0.0f;
		v[0].u = cx;
		v[0].v = cy;
//		v[0].Diffuse = Col;
		v[0].FDiffuse = FColour;

		v[1].Coord.x = x + 9.0f / (g_viewport.Width * 0.5f);
		v[1].Coord.y = y;
		v[1].Coord.z = 0.0f;
		v[1].u = cx + sizeX;
		v[1].v = cy;
//		v[1].Diffuse = Col;
		v[1].FDiffuse = FColour;

		v[2].Coord.x = x + 9.0f / (g_viewport.Width * 0.5f);
		v[2].Coord.y = y + 14.0f / (g_viewport.Height * 0.5f);
		v[2].Coord.z = 0.0f;
		v[2].u = cx + sizeX;
		v[2].v = cy + sizeY;
//		v[2].Diffuse = Col;
		v[2].FDiffuse = FColour;

		v[3].Coord.x = x;
		v[3].Coord.y = y + 14.0f / (g_viewport.Height * 0.5f);
		v[3].Coord.z = 0.0f;
		v[3].u = cx;
		v[3].v = cy + sizeY;
//		v[3].Diffuse = Col;
		v[3].FDiffuse = FColour;

		m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, &v, sizeof(PosColNormalUVVertex));

		x += 9.0f / (g_viewport.Width * 0.5f);
		txt++;
	}
}

int Renderer::GetViewportHeight()
{
	return g_viewport.Height;
}

int Renderer::GetViewportWidth()
{
	return g_viewport.Width;
}

//-- SetAspect ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetAspect(float aspect)
{
	D3DXMATRIX matProj;

	aspect = min(aspect, 1);
	aspect = max(aspect, 0);
	g_aspectValue = aspect;
	//  g_aspect = 1.0f + aspect * ((4.0f / 3.0f) - 1.0f);
	g_aspect = 1.0f + aspect * ((g_defaultAspect) - 1.0f);

	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, g_aspect, 0.25f, 2000.0f );
	//  D3DXMatrixPerspectiveFovLH( &matProj, D3DXToRadian(60), g_aspect, 0.25f, 2000.0f );
	Renderer::SetProjection(matProj);

} // SetAspect

//-- LookAt -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
{
	D3DXVECTOR3 eye(eyeX, eyeY, eyeZ);
	D3DXVECTOR3 center(centerX, centerY, centerZ);
	D3DXVECTOR3 up(upX, upY, upZ);
	D3DXMATRIX matOut;
	D3DXMatrixLookAtLH(&matOut, &eye, &center, &up);
	g_matrixStack->MultMatrix(&matOut);

} // LookAt


//-- SetDefaults --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetDefaults()
{
	for (;g_matrixStackLevel > 0; g_matrixStackLevel--)
	{
		g_matrixStack->Pop();
	}

	g_matrixStack->LoadIdentity();

	// Renderstates
	m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	//  d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	m_pD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	m_pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	m_pD3DDevice->SetRenderState(D3DRS_ALPHAREF, 0);
	m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);

	m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	for (int i = 0; i < 4; i++)
	{
		m_pD3DDevice->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		m_pD3DDevice->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		m_pD3DDevice->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_pD3DDevice->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_pD3DDevice->SetTexture(i, NULL);
	}

	CurrentTextureStageMode = TSM_TEXTURE;
 	g_envMapSet = false;
 	g_textureSet = false;

	// Globals
	g_colour = 0xffffffff;
	g_fColour = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	g_aspect = g_defaultAspect;
	g_aspectValue = 1.0f;

	SetDrawMode3d();
	D3DXMATRIX matView;
	D3DXMatrixIdentity( &matView );
	SetView( matView );

	SetBlendMode( BLEND_OFF ); 

//	m_pD3DDevice->SetVertexDeclaration( g_pVertexDeclaration );

} // SetDefaults

//-- SetProjection ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetProjection(D3DXMATRIX& matProj)
{
	g_matProj = matProj;

} // SetProjection

void Renderer::GetProjection(D3DXMATRIX& matProj)
{
	matProj = g_matProj;

} // SetProjection

//-- SetView ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetView(D3DXMATRIX& matView)
{
	g_matView = matView;

} // SetView

//-- Translate ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::Translate(float x, float y, float z)
{
	g_matrixStack->TranslateLocal(x, y, z);

} // Translate

//-- Scale --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::Scale(float x, float y, float z)
{
	g_matrixStack->ScaleLocal(x, y, z);

} // Scale

//-- CommitTransforms ---------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::CommitTransforms( DiffuseUVVertexShader* pShader )
{
	D3DXMATRIX worldView, worldViewProjection;
	D3DXMATRIX* worldMat = g_matrixStack->GetTop();
	D3DXMatrixMultiply( &worldView, worldMat, &g_matView);
	D3DXMatrixMultiply( &worldViewProjection, &worldView, &g_matProj );

	D3DXMatrixTranspose( &worldViewProjection, &worldViewProjection );
	D3DXMatrixTranspose( &worldView, &worldView );

	pShader->SetWV( &worldView );
	pShader->SetWVP( &worldViewProjection );
}

//-- CommitTextureState -------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::CommitTextureState()
{
	if ( ( g_textureSet || g_envMapSet ) && CurrentTextureStageMode != TSM_TEXTURE )
	{
		CurrentTextureStageMode = TSM_TEXTURE;
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	}
	else if ( !g_textureSet && !g_envMapSet && CurrentTextureStageMode != TSM_COLOUR )
	{
		CurrentTextureStageMode = TSM_COLOUR;
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
		m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
	}
}

//-- SetBlendMode -------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetBlendMode(eBlendMode blendMode)
{
	switch (blendMode)
	{
	case BLEND_OFF:
		{
			m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			break;
		}
	case BLEND_ADD:
		{
			m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		}
	case BLEND_MOD:
		{
			m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		}
	case BLEND_MAX:
		{
			m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
			m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		}
	default:
		{
			m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			break;
		}
	}

} // SetBlendMode

//-- NumPrimsFromType ---------------------------------------------------------
//
//-----------------------------------------------------------------------------
int NumPrimsFromType(D3DPRIMITIVETYPE primType, int numVerts)
{
	switch (primType)
	{
	case D3DPT_POINTLIST:
		return numVerts;
	case D3DPT_LINELIST:
		{
			if (numVerts == 1)
				return -1;
			if (numVerts & 0x1)
				return -1;

			return numVerts / 2;
		}
	case D3DPT_LINESTRIP:
		{
			if (numVerts & 0x1)
				return -1;

			return numVerts - 1;
		}
	case D3DPT_TRIANGLELIST:
		{
			if (numVerts < 3)
				return -1;
			if (numVerts % 3 != 0)
				return -1;
			return numVerts / 3;
		}
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_TRIANGLEFAN:
		{
			if (numVerts < 3)
				return -1;
			return numVerts - 2;
		}
	case 8: // QUADLIST
	{
		if (numVerts < 4)
			return -1;
		if (numVerts % 4 != 0)
			return -1;
		return numVerts / 4;
	}
	case 9: // QUADSTRIP
		{
			if (numVerts < 4)
				return -1;
			if ((numVerts & 0x1) != 0)
				return -1;
			return numVerts / 2 - 1;

		}
	default:
		return -1;
	}

} // NumPrimsFromType


//-- Begin --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::Begin(D3DPRIMITIVETYPE primType)
{
	g_primType = primType;
	g_currentVertex = 0;

} // Begin

//-- End ----------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::End()
{
	int primCount = NumPrimsFromType(g_primType, g_currentVertex);
	if ( primCount == -1 )
		return;

	DiffuseUVVertexShader* pShader = NULL;
	if ( g_envMapSet )
	{
		pShader = &DiffuseUVEnvVertexShader::StaticType;
		m_pD3DDevice->SetVertexDeclaration( g_pPosNormalColUVDeclaration );
	}
	else
	{
		pShader = &DiffuseUVVertexShader::StaticType;
		m_pD3DDevice->SetVertexDeclaration( g_pPosColUVDeclaration );
	}

	CommitTransforms( pShader );
	CommitTextureState();
	m_pD3DDevice->SetVertexShader( pShader->GetVertexShader() );
	
	if ( g_primType == 8 ) // QUADLIST
	{
		for ( INT i = 0; i < primCount; i++ )
		{
			m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, &g_vertices[ i * 4], sizeof(PosColNormalUVVertex));
		}
	}
	else if ( g_primType == 9 ) // QUADSTRIP
	{
		for ( INT i = 0; i < primCount; i++ )
		{
			m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &g_vertices[ i * 2], sizeof(PosColNormalUVVertex));
		}
	}

	else
	{
		m_pD3DDevice->DrawPrimitiveUP(g_primType, primCount, g_vertices, sizeof(PosColNormalUVVertex));
	}

} // End


/*
__forceinline float ClampCol(float c)
{
	if (c < 0)
	{
		c = 0;
	}
	else if (c > 1)
	{
		c = 1;
	}

	return c;
}
*/

//-- Colour -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::Colour(float r, float g, float b, float a)
{
// 	r = ClampCol(r);
// 	g = ClampCol(g);
// 	b = ClampCol(b);
// 	a = ClampCol(a);
// 	unsigned char cr = (unsigned char) (r * 255.0f);
// 	unsigned char cg = (unsigned char) (g * 255.0f);
// 	unsigned char cb = (unsigned char) (b * 255.0f);
// 	unsigned char ca = (unsigned char) (a * 255.0f);
// 	g_colour = D3DCOLOR_ARGB(ca, cr, cg, cb);
	g_fColour = D3DXVECTOR4(r, g, b, a);

} // Colour

//-- Vertex -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::Vertex( float x, float y, float z )
{
	if ( g_currentVertex == MAX_VERTICES )
	{
		return;
	}

	PosColNormalUVVertex& v = g_vertices[ g_currentVertex++ ];

	v.Coord.x = x;
	v.Coord.y = y;
	v.Coord.z = z;
//	v.Diffuse = g_colour;
	v.Normal = g_normal;
 	v.u = g_tex0U;
 	v.v = g_tex0V;
	v.FDiffuse = g_fColour;

} // Vertex

//-- RotateAxis ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::RotateAxis( float angle, float x, float y, float z )
{
	D3DXVECTOR3 axis( x, y, z );
	g_matrixStack->RotateAxisLocal( &axis, D3DXToRadian( angle ) );

} // RotateAxis

//-- Cube ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::Cube( float nx, float ny, float nz, float x, float y, float z )
{
	//    4-------5
	//   /|      /|
	//  0-------1 |
	//  | 7-----|-6
	//  |/      |/
	//  3-------2

	PushMatrix();

	Translate(  ( nx + x ) * 0.5f,
				( ny + y ) * 0.5f,
				( nz + z ) * 0.5f );

	DiffuseUVCubeVertexShader* pShader = NULL;
	if ( g_envMapSet )
	{
		pShader = &DiffuseUVEnvCubeVertexShader::StaticType;
	}
	else
	{
		pShader = &DiffuseUVCubeVertexShader::StaticType;
	}

	CommitTransforms( pShader );
	CommitTextureState();

	PopMatrix();

	D3DXVECTOR4 vScale( ( x - nx ) * 0.5f, ( y - ny ) * 0.5f, ( z - nz ) * 0.5f, 1.0f );

	pShader->SetScale( &vScale );
	pShader->SetColour( &g_fColour );

	m_pD3DDevice->SetVertexShader( pShader->GetVertexShader() );
	m_pD3DDevice->SetVertexDeclaration( g_pPosNormalColUVDeclaration );

	m_pD3DDevice->SetStreamSource( 0,
								   g_pCubeVbuffer,
								   0,
								   sizeof( PosColNormalUVVertex ) );

	m_pD3DDevice->SetIndices( g_pCubeIbuffer );

	m_pD3DDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12 );

	m_pD3DDevice->SetStreamSource( 0,
								   NULL,
								   0,
								   sizeof( PosColNormalUVVertex ) );

	m_pD3DDevice->SetIndices( NULL );

} // Cube

#define PI180	0.0174532925199432957692369076848861f	// pi / 180

inline void RotY( const float angle, D3DXVECTOR3& vec)
{
	float s = (float) sin( PI180*angle );
	float c = (float) cos( PI180*angle );
	float X = vec.x;
	vec.x =  vec.x*c + vec.z*s;
	vec.z = -X*s + vec.z*c;
}
void Renderer::SimpleSphere(float size)
{
	Sphere(10,10, size);
}

void Renderer::Sphere(int del_uhol_x, int del_uhol_y, float size)
{
	// del_uhol_x krat 2*(del_uhol_y+1) bodov

	//  Vertex_t v[del_uhol_x*2*(del_uhol_y+1)];
	PosColNormalUVVertex* v = new PosColNormalUVVertex[del_uhol_x*2*(del_uhol_y+1)];

	D3DXVECTOR3 a,b,ay,by;
	float dy=360.f/del_uhol_y;
	float dx=180.f*PI180/del_uhol_x;
	for(int x=0; x<del_uhol_x; x++)
	{
		float tx = (float)x/(float)del_uhol_x;
		float ang = (tx*180.f-90.f)*PI180;		// <-90, 90)
		a = D3DXVECTOR3( 0, (float)sin(ang), (float)cos(ang));
		b = D3DXVECTOR3( 0, (float)sin(ang+dx), (float)cos(ang+dx));

		//	v[x*2*(del_uhol_y+1)+0].t0.set( 0, 1.f-tx);
		v[x*2*(del_uhol_y+1)+0].Normal = a;
		v[x*2*(del_uhol_y+1)+0].Coord = size*a;

		//	v[x*2*(del_uhol_y+1)+1].t0.set( 0, 1.f-(tx+1.f/del_uhol_x));
		v[x*2*(del_uhol_y+1)+1].Normal= b;
		v[x*2*(del_uhol_y+1)+1].Coord = size*b;

		ay = a;	by = b;
		for(int y=1; y<del_uhol_y+1; y++)
		{
			float ty = (float)y/(float)del_uhol_y;
			ay = a;	RotY( ty*360.f, ay);
			by = b;	RotY( ty*360.f, by);

			//	v[x*2*(del_uhol_y+1)+2*y].t0.set( ty, 1.f-tx);
			v[x*2*(del_uhol_y+1)+2*y].Normal = ay;
			v[x*2*(del_uhol_y+1)+2*y].Coord = size*ay;

			//	v[x*2*(del_uhol_y+1)+2*y+1].t0.set( ty, 1.f-(tx+1.f/del_uhol_x));
			v[x*2*(del_uhol_y+1)+2*y+1].Normal = by;
			v[x*2*(del_uhol_y+1)+2*y+1].Coord = size*by;
		}
	}

	DiffuseUVVertexShader* pShader = NULL;
	if ( g_envMapSet )
	{
		pShader = &DiffuseUVEnvVertexShader::StaticType;
		m_pD3DDevice->SetVertexDeclaration( g_pPosNormalColUVDeclaration );
	}
	else
	{
		pShader = &DiffuseUVVertexShader::StaticType;
		m_pD3DDevice->SetVertexDeclaration( g_pPosColUVDeclaration );
	}

	CommitTransforms( pShader );
	CommitTextureState();
	m_pD3DDevice->SetVertexShader( pShader->GetVertexShader() );
	for(int i=0; i<del_uhol_x; i++)
		//		g_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, i*2*(del_y+1), 2*del_y );
		m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2*del_uhol_y, &v[i*2*(del_uhol_y+1)], sizeof(PosColNormalUVVertex));

	delete v;
	//  pd->DrawPrimitive( D3DPT_TRIANGLESTRIP, i*2*(del_y+1), 2*del_y );
}

//-- SetEnvTexture ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetEnvTexture( LPDIRECT3DTEXTURE9 texture )
{
	if ( texture == SCRATCH_TEXTURE )
	{
		m_pD3DDevice->SetTexture(0, g_scratchTexture);
	}
	else
	{
		m_pD3DDevice->SetTexture( 0, texture );
	}
	g_envMapSet = texture != NULL;

} // SetEnvTexture

//-- SetTexture ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetTexture (LPDIRECT3DTEXTURE9 texture )
{
	if ( texture == SCRATCH_TEXTURE )
	{
		m_pD3DDevice->SetTexture( 0, g_scratchTexture );
	}
	else
	{
		m_pD3DDevice->SetTexture( 0, texture );
	}
	g_textureSet = texture != NULL;

} // SetTexture

//-- SetRenderTarget ----------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetRenderTarget( LPDIRECT3DTEXTURE9 texture )
{
	IDirect3DSurface9* surface;
	texture->GetSurfaceLevel( 0, &surface );

	m_pD3DDevice->SetRenderTarget( 0, surface );
	m_pD3DDevice->SetDepthStencilSurface( g_depthBuffer );
	m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 1.0f, 0 );

	surface->Release();

	//	m_renderTargetWidth = texture->Width();
	//	m_renderTargetHeight = texture->Height();

} // SetRenderTarget

void Renderer::GetBackBuffer()
{
	m_pD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &g_backBuffer);
	m_pD3DDevice->GetDepthStencilSurface(&g_oldDepthBuffer);
}

void Renderer::ReleaseBackbuffer()
{
	g_oldDepthBuffer->Release();
	g_backBuffer->Release();
}

//-- SetRenderTargetBackBuffer ------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetRenderTargetBackBuffer()
{

	m_pD3DDevice->SetRenderTarget(0, g_backBuffer);
	m_pD3DDevice->SetDepthStencilSurface( g_oldDepthBuffer );

//	m_pD3DDevice->SetViewport(&g_viewport);

	//	m_renderTargetWidth = 640;
	//	m_renderTargetHeight = 480;

} // SetRenderTargetBackBuffer


void Renderer::CompileShaders()
{
	m_pD3DDevice->CreateVertexDeclaration( declPosNormalColUV, &g_pPosNormalColUVDeclaration );
	m_pD3DDevice->CreateVertexDeclaration( declPosColUV, &g_pPosColUVDeclaration );
	m_pD3DDevice->CreateVertexDeclaration( declPosNormal, &g_pPosNormalDeclaration );

	Shader::CompileAllShaders();
}

//-- PushMatrix ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::PushMatrix()
{
	g_matrixStack->Push();

} // PushMatrix

//-- PopMatrix ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::PopMatrix()
{
	g_matrixStack->Pop();

} // PopMatrix


void Renderer::TexCoord( float u, float v )
{
	g_tex0U = u;
	g_tex0V = v;

} // TexCoord

void Renderer::Normal( float nx, float ny, float nz )
{
	g_normal.x = nx;
	g_normal.y = ny;
	g_normal.z = nz;
	D3DXVec3Normalize( &g_normal, &g_normal );
}

//-- ClearFloat ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::ClearFloat(float r, float g, float b)
{
	unsigned char cr = (unsigned char) (r * 255.0f);
	unsigned char cg = (unsigned char) (g * 255.0f);
	unsigned char cb = (unsigned char) (b * 255.0f);
	unsigned char ca = (unsigned char) (1.0f * 255.0f);
	unsigned int colour = D3DCOLOR_ARGB(ca, cr, cg, cb);

	m_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, colour, 1.0f, 0);

} // ClearFloat

//-- TexRect ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::TexRect(float x1, float y1, float x2, float y2)
{
	PosColNormalUVVertex v[4];

	v[0].Coord.x = x1;
	v[0].Coord.y = y1;
	v[0].Coord.z = 0.0f;
//	v[0].Diffuse = g_colour;
	v[0].FDiffuse = g_fColour;
	v[0].u = 0.5f / 256.0f;
	v[0].v = 0.5f / 256.0f;

	v[1].Coord.x = x2;
	v[1].Coord.y = y1;
	v[1].Coord.z = 0.0f;
//	v[1].Diffuse = g_colour;
	v[1].FDiffuse = g_fColour;
	v[1].u = 1.0f;
	v[1].v = 0.5f / 256.0f;

	v[2].Coord.x = x2;
	v[2].Coord.y = y2;
	v[2].Coord.z = 0.0f;
//	v[2].Diffuse = g_colour;
	v[2].FDiffuse = g_fColour;
	v[2].u = 1.0f;
	v[2].v = 1.0f;

	v[3].Coord.x = x1;
	v[3].Coord.y = y2;
	v[3].Coord.z = 0.0f;
//	v[3].Diffuse = g_colour;
	v[3].FDiffuse = g_fColour;
	v[3].u = 0.5f / 256.0f;
	v[3].v = 1.0f;

	DiffuseUVVertexShader* pShader = &DiffuseUVVertexShader::StaticType;
	CommitTransforms( pShader );
	CommitTextureState();
	m_pD3DDevice->SetVertexShader( pShader->GetVertexShader() );
	m_pD3DDevice->SetVertexDeclaration( g_pPosColUVDeclaration );

	m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, &v, sizeof(PosColNormalUVVertex));
	m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

} // TexRect

//-- CopyToScratch ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::CopyToScratch()
{
	// TODO:
/*
	// Copy current render target to scratch buffer
	IDirect3DSurface9* currSurface;
	IDirect3DSurface9* destSurface;

	m_pD3DDevice->GetRenderTarget(0, &currSurface);
	g_scratchTexture->GetSurfaceLevel(0, &destSurface);

	RECT src;
	POINT dst;
	src.left = 0;
	src.right = 512;
	src.top = 0;
	src.bottom = 512;
	dst.x = 0;
	dst.y = 0;

//	m_pD3DDevice->CopyRects(currSurface, &src, 1, destSurface, &dst);
	m_pD3DDevice->UpdateSurface( currSurface, &src, destSurface, &dst );
	currSurface->Release();
	destSurface->Release();
*/
} // CopyToScratch

//-- GetDevice ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
LPDIRECT3DDEVICE9 Renderer::GetDevice()
{
	return m_pD3DDevice;

} // GetDevice

//-- GetAspect ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
float Renderer::GetAspect()
{
	return g_aspectValue;

} // GetAspect

//-- SetIdentity --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetIdentity()
{
	g_matrixStack->LoadIdentity();

} // SetIdentity


IDirect3DPixelShader9* Renderer::CreatePixelShader( DWORD* pCode )
{
	IDirect3DPixelShader9*	pPixelShader;
	m_pD3DDevice->CreatePixelShader( pCode, &pPixelShader );
	return pPixelShader;
}

IDirect3DVertexShader9* Renderer::CreateVertexShader( DWORD* pCode )
{
	IDirect3DVertexShader9*	pVertexShader;
	m_pD3DDevice->CreateVertexShader( pCode, &pVertexShader );
	return pVertexShader;
}

void Renderer::SetConstantTableMatrix( LPD3DXCONSTANTTABLE pConstantTable, char* pConstantName, D3DXMATRIX* pMatrix )
{
	pConstantTable->SetMatrix( m_pD3DDevice, pConstantName, pMatrix );
}

void Renderer::SetConstantTableVector( LPD3DXCONSTANTTABLE pConstantTable, char* pConstantName, D3DXVECTOR4* pVector)
{
	pConstantTable->SetVector( m_pD3DDevice, pConstantName, pVector );
}

void Renderer::SetFillMode( int fillMode )
{
	if ( fillMode == 0 )
	{
		m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	}
	else
	{
		m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	}
}
