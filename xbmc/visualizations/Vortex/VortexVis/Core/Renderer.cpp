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

#include "Renderer.h"
#include "DebugConsole.h"
#include <stdio.h>
#include "Shader.h"
#include "DirectXHelpers.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "XMMatrixStack.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

D3D11_INPUT_ELEMENT_DESC declPosNormalColUV[] = 
{
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

D3D11_INPUT_ELEMENT_DESC declPosColUV[] =
{	
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

D3D11_INPUT_ELEMENT_DESC declPosNormal[] =
{	
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

ID3D11InputLayout*    g_pPosNormalColUVDeclaration = NULL;
ID3D11InputLayout*    g_pPosColUVDeclaration = NULL;
ID3D11InputLayout*    g_pPosNormalDeclaration = NULL;

namespace
{
  #include "../Shaders/ColorPixelShader.inc"
  #include "../Shaders/TexturePixelShader.inc"
}

class ColorPixelShader : public Shader
{
  DECLARE_SHADER(ColorPixelShader);
public:
  ColorPixelShader(EShaderType ShaderType, const void* pShaderCode, unsigned int iCodeLen) : Shader(ShaderType, pShaderCode, iCodeLen){};
};

class TexturePixelShader : public Shader
{
  DECLARE_SHADER(TexturePixelShader);
public:
  TexturePixelShader(EShaderType ShaderType, const void* pShaderCode, unsigned int iCodeLen) : Shader(ShaderType, pShaderCode, iCodeLen){};
};

IMPLEMENT_SHADER(ColorPixelShader, ColorPixelShaderCode, sizeof(ColorPixelShaderCode), ST_PIXEL);
IMPLEMENT_SHADER(TexturePixelShader, TexturePixelShaderCode, sizeof(TexturePixelShaderCode), ST_PIXEL);

namespace
{
  ID3D11Buffer* g_pCubeVbuffer = NULL;
  ID3D11Buffer* g_pCubeIbuffer = NULL;
  ID3D11Buffer* g_pFanIbuffer  = NULL;
  ID3D11Buffer* g_pFixVbuffer = NULL;
  ID3D11Device* m_pD3D11Device = NULL;
  ID3D11DeviceContext* m_pD3D11Contex = NULL;
  ID3D11RenderTargetView*	g_backBuffer = NULL;
  ID3D11DepthStencilView* g_depthView = NULL;
  ID3D11DepthStencilView*	g_oldDepthBuffer = NULL;
	TextureDX* m_pTextureFont = NULL;
	D3D11_VIEWPORT  g_viewport;
	XMMATRIX  g_matProj;
	XMMATRIX  g_matView;
	float g_aspect;
	float g_defaultAspect;
	float g_aspectValue;
	float g_tex0U;
	float g_tex0V;
	XMFLOAT4 g_fColour;
  XMVECTOR g_normal;
	XMMatrixStack* g_matrixStack = NULL;
	int g_matrixStackLevel;
	bool g_envMapSet;
	bool g_textureSet;
  std::shared_ptr<CommonStates> states;

	D3DPRIMITIVETYPE g_primType;

	TextureDX* g_scratchTexture = NULL;
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

void Renderer::Init( ID3D11DeviceContext* pD3DContext, int iXPos, int iYPos, int iWidth, int iHeight, float fPixelRatio )
{
	DebugConsole::Log("Renderer: Viewport x%d, y%d, w%d, h%d, pixel%f\n", iXPos, iYPos, iWidth, iHeight, fPixelRatio );
  m_pD3D11Contex = pD3DContext;
  pD3DContext->GetDevice(&m_pD3D11Device);
  g_viewport.TopLeftX = (float)iXPos;
  g_viewport.TopLeftY = (float)iYPos;
  g_viewport.Width = (float)iWidth;
  g_viewport.Height = (float)iHeight;
	g_viewport.MinDepth = 0.0f;
	g_viewport.MaxDepth = 1.0f;

	g_defaultAspect = ((float)iWidth / iHeight) * fPixelRatio;

  ID3D11Texture2D* depthStencilBuffer = NULL;

  // Initialize the description of the depth buffer.
  CD3D11_TEXTURE2D_DESC depthBufferDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, 1024, 512, 1, 1, D3D11_BIND_DEPTH_STENCIL);
  // Create the texture for the depth buffer using the filled out description.
  m_pD3D11Device->CreateTexture2D(&depthBufferDesc, NULL, &depthStencilBuffer);

  // Create the depth stencil view.
  CD3D11_DEPTH_STENCIL_VIEW_DESC viewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
  m_pD3D11Device->CreateDepthStencilView(depthStencilBuffer, &viewDesc, &g_depthView);
  depthStencilBuffer->Release();

	{
		char fullname[512];
		sprintf_s(fullname, 512, "%sfont.bmp", g_TexturePath );
		m_pTextureFont = LoadTexture( fullname, false );
	}

  g_matrixStack = new XMMatrixStack();
  g_matrixStackLevel = 0;

	g_scratchTexture = CreateTexture( 512, 512 );

	CompileShaders();

	CreateCubeBuffers();

  // create indecies buffer for FAN topology
  unsigned short indicies[6 * 512];
  for (unsigned int i = 0; i < 512; i += 6)
  {
    indicies[i] = i;
    indicies[i + 1] = i + 1;
    indicies[i + 2] = i + 2;
    indicies[i + 3] = i;
    indicies[i + 4] = i + 2;
    indicies[i + 5] = i + 3;
  }
  CD3D11_BUFFER_DESC bDesc(sizeof(indicies), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE);
  D3D11_SUBRESOURCE_DATA bData = { indicies };
  m_pD3D11Device->CreateBuffer(&bDesc, &bData, &g_pFanIbuffer);

  // create vertecies buffer
  bDesc.ByteWidth = sizeof(g_vertices);
  bDesc.Usage = D3D11_USAGE_DYNAMIC;
  bDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  m_pD3D11Device->CreateBuffer(&bDesc, NULL, &g_pFixVbuffer);

  CreateFonts();

  states.reset(new CommonStates(m_pD3D11Device));
}

void Renderer::CreateFonts()
{
	/*HDC hdc = CreateCompatibleDC( NULL );
	if( hdc == NULL )
		return;

	INT nHeight = -MulDiv( 12, GetDeviceCaps( hdc, LOGPIXELSY ), 72 );
	DeleteDC( hdc );

	BOOL bBold = FALSE;
	BOOL bItalic = FALSE;

	g_hFont = CreateFontA( nHeight, 0, 0, 0, bBold ? FW_BOLD : FW_NORMAL, bItalic, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		"Arial" );*/
}

IUnknown* Renderer::CreateD3DXTextMesh(const char* pString, bool bCentered)
{
	//HRESULT hr;
	IUnknown* pMeshNew = NULL;

	/*HDC hdc = CreateCompatibleDC( NULL );
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
	}*/

	return pMeshNew;
}

void Renderer::DrawMesh(IUnknown*	pMesh)
{
	if ( pMesh == NULL )
	{
		return;
	}

	/*UVNormalEnvVertexShader* pShader = &UVNormalEnvVertexShader::StaticType;
	CommitTransforms( pShader );
	CommitTextureState();
  m_pD3D11Contex->VSSetShader(pShader->GetVertexShader(), NULL, 0);
  m_pD3D11Contex->IASetInputLayout( g_pPosNormalDeclaration );*/

	//pMesh->DrawSubset( 0 );
}

void Renderer::CreateCubeBuffers()
{
  PosColNormalUVVertex v[24];
  short ind[36], *indices = ind;

	float nx = -1.0f;
	float ny = -1.0f;
	float nz = -1.0f;
	float x = 1.0f;
	float y = 1.0f;
	float z = 1.0f;

  g_fColour = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Front Face
	v[0].Coord  = XMFLOAT3(nx, y, nz);
  v[0].FDiffuse = g_fColour;
  v[0].Normal = XMFLOAT3(0, 0, -1);
	v[0].u = 0;
	v[0].v = 0;
  v[1].Coord = XMFLOAT3(x, y, nz);
  v[1].FDiffuse = g_fColour;
  v[1].Normal = XMFLOAT3(0, 0, -1);
	v[1].u = 1;
	v[1].v = 0;
  v[2].Coord = XMFLOAT3(x, ny, nz);
  v[2].FDiffuse = g_fColour;
  v[2].Normal = XMFLOAT3(0, 0, -1);
	v[2].u = 1;
	v[2].v = 1;
  v[3].Coord = XMFLOAT3(nx, ny, nz);
  v[3].FDiffuse = g_fColour;
  v[3].Normal = XMFLOAT3(0, 0, -1);
	v[3].u = 0;
	v[3].v = 1;

	*indices++ = 0;
	*indices++ = 1;
	*indices++ = 2;
	*indices++ = 0;
	*indices++ = 2;
	*indices++ = 3;

	// Back Face
  v[4].Coord = XMFLOAT3(x, y, z);
  v[4].FDiffuse = g_fColour;
  v[4].Normal = XMFLOAT3(0, 0, 1);
	v[4].u = 0;
	v[4].v = 0;
  v[5].Coord = XMFLOAT3(nx, y, z);
  v[5].FDiffuse = g_fColour;
  v[5].Normal = XMFLOAT3(0, 0, 1);
	v[5].u = 1;
	v[5].v = 0;
  v[6].Coord = XMFLOAT3(nx, ny, z);
  v[6].FDiffuse = g_fColour;
  v[6].Normal = XMFLOAT3(0, 0, 1);
	v[6].u = 1;
	v[6].v = 1;
  v[7].Coord = XMFLOAT3(x, ny, z);
  v[7].FDiffuse = g_fColour;
  v[7].Normal = XMFLOAT3(0, 0, 1);
	v[7].u = 0;
	v[7].v = 1;

	*indices++ = 4;
	*indices++ = 5;
	*indices++ = 6;
	*indices++ = 4;
	*indices++ = 6;
	*indices++ = 7;

	// Top Face
  v[8].Coord = XMFLOAT3(nx, y, z);
  v[8].FDiffuse = g_fColour;
  v[8].Normal = XMFLOAT3(0, 1, 0);
	v[8].u = 0;
	v[8].v = 0;
  v[9].Coord = XMFLOAT3(x, y, z);
  v[9].FDiffuse = g_fColour;
  v[9].Normal = XMFLOAT3(0, 1, 0);
	v[9].u = 1;
	v[9].v = 0;
  v[10].Coord = XMFLOAT3(x, y, nz);
  v[10].FDiffuse = g_fColour;
  v[10].Normal = XMFLOAT3(0, 1, 0);
	v[10].u = 1;
	v[10].v = 1;
  v[11].Coord = XMFLOAT3(nx, y, nz);
  v[11].FDiffuse = g_fColour;
  v[11].Normal = XMFLOAT3(0, 1, 0);
	v[11].u = 0;
	v[11].v = 1;

	*indices++ = 8;
	*indices++ = 9;
	*indices++ = 10;
	*indices++ = 8;
	*indices++ = 10;
	*indices++ = 11;

	// Bottom Face
  v[12].Coord = XMFLOAT3(nx, ny, nz);
  v[12].FDiffuse = g_fColour;
  v[12].Normal = XMFLOAT3(0, -1, 0);
	v[12].u = 0;
	v[12].v = 0;
  v[13].Coord = XMFLOAT3(x, ny, nz);
  v[13].FDiffuse = g_fColour;
  v[13].Normal = XMFLOAT3(0, -1, 0);
	v[13].u = 1;
	v[13].v = 0;
  v[14].Coord = XMFLOAT3(x, ny, z);
  v[14].FDiffuse = g_fColour;
  v[14].Normal = XMFLOAT3(0, -1, 0);
	v[14].u = 1;
	v[14].v = 1;
  v[15].Coord = XMFLOAT3(nx, ny, z);
  v[15].FDiffuse = g_fColour;
  v[15].Normal = XMFLOAT3(0, -1, 0);
	v[15].u = 0;
	v[15].v = 1;

	*indices++ = 12;
	*indices++ = 13;
	*indices++ = 14;
	*indices++ = 12;
	*indices++ = 14;
	*indices++ = 15;

	// Left Face
  v[16].Coord = XMFLOAT3(nx, y, z);
  v[16].FDiffuse = g_fColour;
  v[16].Normal = XMFLOAT3(-1, 0, 0);
	v[16].u = 0;
	v[16].v = 0;
  v[17].Coord = XMFLOAT3(nx, y, nz);
  v[17].FDiffuse = g_fColour;
  v[17].Normal = XMFLOAT3(-1, 0, 0);
	v[17].u = 1;
	v[17].v = 0;
  v[18].Coord = XMFLOAT3(nx, ny, nz);
  v[18].FDiffuse = g_fColour;
  v[18].Normal = XMFLOAT3(-1, 0, 0);
	v[18].u = 1;
	v[18].v = 1;
  v[19].Coord = XMFLOAT3(nx, ny, z);
  v[19].FDiffuse = g_fColour;
  v[19].Normal = XMFLOAT3(-1, 0, 0);
	v[19].u = 0;
	v[19].v = 1;

	*indices++ = 16;
	*indices++ = 17;
	*indices++ = 18;
	*indices++ = 16;
	*indices++ = 18;
	*indices++ = 19;

	// Right Face
  v[20].Coord = XMFLOAT3(x, y, nz);
  v[20].FDiffuse = g_fColour;
  v[20].Normal = XMFLOAT3(1, 0, 0);
	v[20].u = 0;
	v[20].v = 0;
  v[21].Coord = XMFLOAT3(x, y, z);
  v[21].FDiffuse = g_fColour;
  v[21].Normal = XMFLOAT3(1, 0, 0);
	v[21].u = 1;
	v[21].v = 0;
  v[22].Coord = XMFLOAT3(x, ny, z);
  v[22].FDiffuse = g_fColour;
  v[22].Normal = XMFLOAT3(1, 0, 0);
	v[22].u = 1;
	v[22].v = 1;
  v[23].Coord = XMFLOAT3(x, ny, nz);
  v[23].FDiffuse = g_fColour;
  v[23].Normal = XMFLOAT3(1, 0, 0);
	v[23].u = 0;
	v[23].v = 1;

	*indices++ = 20;
	*indices++ = 21;
	*indices++ = 22;
	*indices++ = 20;
	*indices++ = 22;
	*indices++ = 23;

  CD3D11_BUFFER_DESC bDesc(sizeof(v), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE);
  D3D11_SUBRESOURCE_DATA bData = { v };
  m_pD3D11Device->CreateBuffer( &bDesc, &bData, &g_pCubeVbuffer );

  bDesc.ByteWidth = sizeof(ind);
  bDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  bData.pSysMem = ind;
  m_pD3D11Device->CreateBuffer(&bDesc, &bData, &g_pCubeIbuffer);
}

void Renderer::Exit()
{
	DeleteObject( g_hFont );

  SAFE_RELEASE( g_pPosNormalColUVDeclaration );
	SAFE_RELEASE( g_pPosColUVDeclaration );
	SAFE_RELEASE( g_pPosNormalDeclaration );

	Shader::ReleaseAllShaders();

	SAFE_RELEASE( g_pCubeIbuffer );
	SAFE_RELEASE( g_pCubeVbuffer );
  SAFE_RELEASE( g_pFanIbuffer );
  SAFE_RELEASE( g_pFixVbuffer );

  if (states.get())
    states.reset();

	SAFE_RELEASE( g_scratchTexture );
	SAFE_RELEASE( m_pTextureFont );
  SAFE_RELEASE( g_depthView );
  SAFE_RELEASE( m_pD3D11Device );
}

//-- SetDrawMode2d ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetDrawMode2d()
{
  XMMATRIX Ortho2D = XMMatrixOrthographicLH(2.0f, -2.0f, 0.0f, 1.0f);
  m_pD3D11Contex->OMSetDepthStencilState(states->DepthNone(), 0);
  Renderer::SetProjection(Ortho2D);

} // SetDrawMode2d

//-- SetDrawMode3d ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetDrawMode3d()
{
  XMMATRIX matProj = XMMatrixPerspectiveFovLH(XM_PI / 4, g_aspect, 0.25f, 2000.0f);
	Renderer::SetProjection(matProj);
  m_pD3D11Contex->OMSetDepthStencilState(states->DepthDefault(), 0);

} // SetDrawMode3d

void Renderer::Clear( DWORD Col )
{
	Col |= 0xff000000;
  ID3D11RenderTargetView* rtView = NULL;
  ID3D11DepthStencilView* depthView = NULL;
  m_pD3D11Contex->OMGetRenderTargets(1, &rtView, &depthView);

  if (rtView != NULL)
  {
    XMCOLOR xcolor(Col);
    float color[4] = { xcolor.r / 255.0f, xcolor.g / 255.0f, xcolor.b / 255.0f, 1.0f };
    m_pD3D11Contex->ClearRenderTargetView(rtView, color);
    rtView->Release();
  }
  if (depthView != NULL)
  {
    m_pD3D11Contex->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1.0, 0);
    depthView->Release();
  }
}

void Renderer::ClearDepth()
{
  m_pD3D11Contex->ClearDepthStencilView(g_depthView, D3D11_CLEAR_DEPTH, 1.0, 0);
}

void Renderer::Rect( float x1, float y1, float x2, float y2, DWORD Col )
{
	unsigned char* pCol = (unsigned char*)&Col;
	XMFLOAT4 FColour(pCol[2],pCol[1], pCol[0], pCol[3] );

	PosColNormalUVVertex v[4];

	v[0].Coord.x = x1;
	v[0].Coord.y = y1;
	v[0].Coord.z = 0.0f;
	v[0].FDiffuse = FColour;

	v[1].Coord.x = x2;
	v[1].Coord.y = y1;
	v[1].Coord.z = 0.0f;
	v[1].FDiffuse = FColour;

	v[2].Coord.x = x2;
	v[2].Coord.y = y2;
	v[2].Coord.z = 0.0f;
	v[2].FDiffuse = FColour;

	v[3].Coord.x = x1;
	v[3].Coord.y = y2;
	v[3].Coord.z = 0.0f;
	v[3].FDiffuse = FColour;

  UpdateVBuffer( v, 4 );
  DrawPrimitive( D3DPT_TRIANGLEFAN, 6, 0 );
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

TextureDX* Renderer::CreateTexture( int iWidth, int iHeight, DXGI_FORMAT Format /* = DXGI_FORMAT_B8G8R8A8_UNORM */, bool bDynamic /* = false */ )
{
  ID3D11Texture2D* pTexture2D = NULL;

  unsigned int bindFlag = D3D11_BIND_SHADER_RESOURCE;
  if (!bDynamic)
    bindFlag |= D3D11_BIND_RENDER_TARGET;

  CD3D11_TEXTURE2D_DESC texDesc(Format, iWidth, iHeight, 1, 1, bindFlag,
                                bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
                                bDynamic ? D3D11_CPU_ACCESS_WRITE : 0);
  m_pD3D11Device->CreateTexture2D(&texDesc, NULL, &pTexture2D);

	if ( pTexture2D == NULL )
	{
		DebugConsole::Log("Renderer: Failed to create texture, Width:%d, Height:%d\n", iWidth, iHeight);
		return NULL;
	}

  return new TextureDX(pTexture2D);
}

TextureDX* Renderer::LoadTexture(char* pFilename, bool bAutoResize)
{
  ID3D11Texture2D* pTexture2D = NULL;

	FILE* iS;
	iS = fopen( pFilename, "rb" );
	if ( iS != NULL )
	{
		fseek( iS, 0, SEEK_END );
		int iLength = ftell( iS );
		fseek( iS, 0, SEEK_SET );
    uint8_t* pTextureInMem = new uint8_t[iLength];
		size_t iReadLength = fread( pTextureInMem, 1, iLength, iS );
		fclose( iS );

    std::string strFileName(pFilename);
    if (GetExtension(strFileName) == "dds")
    {
      CreateDDSTextureFromMemory( m_pD3D11Device,
                                  pTextureInMem,
                                  iReadLength,
                                  reinterpret_cast<ID3D11Resource**>(&pTexture2D),
                                  NULL );
    }
    else
    {
      CreateWICTextureFromMemory( m_pD3D11Device,
                                  pTextureInMem,
                                  iReadLength,
                                  reinterpret_cast<ID3D11Resource**>(&pTexture2D),
                                  NULL );
    }

		delete[] pTextureInMem;
	}

  if (pTexture2D == NULL)
	{
		DebugConsole::Log( "Renderer: Failed to load texture: %s\n", pFilename );
    return NULL;
	}

	return new TextureDX(pTexture2D);
}

void Renderer::DrawText( float x, float y, char* txt, DWORD Col )
{
	unsigned char* pCol = (unsigned char*)&Col;
	XMFLOAT4 FColour(pCol[2],pCol[1], pCol[0], pCol[3] );

	SetTexture( m_pTextureFont );
	DiffuseUVVertexShader* pShader = &DiffuseUVVertexShader::StaticType;

  SetShader(pShader);
  CommitTransforms(pShader);
	CommitTextureState();

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
		v[0].FDiffuse = FColour;

		v[1].Coord.x = x + 9.0f / (g_viewport.Width * 0.5f);
		v[1].Coord.y = y;
		v[1].Coord.z = 0.0f;
		v[1].u = cx + sizeX;
		v[1].v = cy;
		v[1].FDiffuse = FColour;

		v[2].Coord.x = x + 9.0f / (g_viewport.Width * 0.5f);
		v[2].Coord.y = y + 14.0f / (g_viewport.Height * 0.5f);
		v[2].Coord.z = 0.0f;
		v[2].u = cx + sizeX;
		v[2].v = cy + sizeY;
		v[2].FDiffuse = FColour;

		v[3].Coord.x = x;
		v[3].Coord.y = y + 14.0f / (g_viewport.Height * 0.5f);
		v[3].Coord.z = 0.0f;
		v[3].u = cx;
		v[3].v = cy + sizeY;
		v[3].FDiffuse = FColour;

    UpdateVBuffer( v, 4 );
    DrawPrimitive( D3DPT_TRIANGLEFAN, 6, 0 );

		x += 9.0f / (g_viewport.Width * 0.5f);
		txt++;
	}
}

float Renderer::GetViewportHeight()
{
	return g_viewport.Height;
}

float Renderer::GetViewportWidth()
{
	return g_viewport.Width;
}

//-- SetAspect ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetAspect(float aspect)
{
	aspect = min(aspect, 1);
	aspect = max(aspect, 0);
	g_aspectValue = aspect;
	g_aspect = 1.0f + aspect * ((g_defaultAspect) - 1.0f);

  XMMATRIX matProj = XMMatrixPerspectiveFovLH(XM_PI / 4, g_aspect, 0.25f, 2000.0f);
	Renderer::SetProjection(matProj);

} // SetAspect

//-- LookAt -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
{
  XMVECTOR eye = XMVectorSet(eyeX, eyeY, eyeZ, 1.0f);
  XMVECTOR center = XMVectorSet(centerX, centerY, centerZ, 1.0f);
  XMVECTOR up = XMVectorSet(upX, upY, upZ*2, 1.0f);
  XMMATRIX matOut = XMMatrixLookAtLH(eye, center, up);
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
  m_pD3D11Contex->RSSetState(states->CullNone());
  m_pD3D11Contex->OMSetBlendState(states->Opaque(), 0, 0xFFFFFFFF);
  m_pD3D11Contex->OMSetDepthStencilState(states->DepthDefault(), 0);

  ID3D11SamplerState* samplerState[1] = { states->LinearWrap() };
  m_pD3D11Contex->PSSetSamplers(0, 1, samplerState);

  SetShader( &TexturePixelShader::StaticType );
  CurrentTextureStageMode = TSM_TEXTURE;

 	g_envMapSet = false;
 	g_textureSet = false;

	// Globals
  g_fColour = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	g_aspect = g_defaultAspect;
	g_aspectValue = 1.0f;

	SetDrawMode3d();
  SetView( XMMatrixIdentity() );
	SetBlendMode( BLEND_OFF ); 

} // SetDefaults

//-- SetProjection ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetProjection(const XMMATRIX& matProj)
{
	g_matProj = matProj;

} // SetProjection

void Renderer::GetProjection(XMMATRIX& matProj)
{
	matProj = g_matProj;

} // SetProjection

//-- SetView ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetView(const XMMATRIX& matView)
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
void Renderer::CommitTransforms( Shader* pShader )
{
	XMMATRIX worldView, worldViewProjection;
	XMMATRIX worldMat = *(g_matrixStack->GetTop());

  worldView = XMMatrixMultiply(worldMat, g_matView);
  worldViewProjection = XMMatrixMultiply(worldMat, g_matProj);

	pShader->SetWV( &worldView );
	pShader->SetWVP( &worldViewProjection );
  pShader->CommitConstants( m_pD3D11Contex );
}

//-- CommitTextureState -------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::CommitTextureState()
{
	if ( ( g_textureSet || g_envMapSet ) && CurrentTextureStageMode != TSM_TEXTURE )
	{
		CurrentTextureStageMode = TSM_TEXTURE;
    SetShader( &TexturePixelShader::StaticType );
	}
	else if ( !g_textureSet && !g_envMapSet && CurrentTextureStageMode != TSM_COLOUR )
	{
		CurrentTextureStageMode = TSM_COLOUR;
    SetShader( &ColorPixelShader::StaticType );
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
      m_pD3D11Contex->OMSetBlendState(states->Opaque(), 0, 0xFFFFFF);
			//m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			//m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			break;
		}
	case BLEND_ADD:
		{
      m_pD3D11Contex->OMSetBlendState(states->Additive(), 0, 0xFFFFFF);
      /*m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);*/
			break;
		}
	case BLEND_MOD:
		{
      m_pD3D11Contex->OMSetBlendState(states->NonPremultiplied(), 0, 0xFFFFFF);
      /*m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);*/
			break;
		}
	case BLEND_MAX:
		{
      m_pD3D11Contex->OMSetBlendState(states->Maximize(), 0, 0xFFFFFF);
      /*m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
			m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);*/
			break;
		}
	default:
		{
      m_pD3D11Contex->OMSetBlendState(states->AlphaBlend(), 0, 0xFFFFFF);
			/*
      m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);*/
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

  DiffuseUVVertexShader* pShader = g_envMapSet 
                                 ? &DiffuseUVEnvVertexShader::StaticType
                                 : &DiffuseUVVertexShader::StaticType;
  SetShader( pShader );
	CommitTransforms( pShader );
	CommitTextureState();
	
  UpdateVBuffer(g_vertices, g_currentVertex);

  if (g_primType == 8) // QUADLIST
	{
		for ( INT i = 0; i < primCount; i++ )
		{
      DrawPrimitive( D3DPT_TRIANGLEFAN, 6, i * 4 );
		}
	}
	else if ( g_primType == 9 ) // QUADSTRIP
	{
    for (INT i = 0; i < primCount; i++)
		{
      DrawPrimitive( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 4, i * 2 );
		}
	}
	else
	{
    DrawPrimitive( g_primType, g_currentVertex, 0 );
	}
} // End


//-- Colour -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::Colour(float r, float g, float b, float a)
{
	g_fColour = XMFLOAT4(r, g, b, a);

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
  XMStoreFloat3(&v.Normal, g_normal);
 	v.u = g_tex0U;
 	v.v = g_tex0V;
	v.FDiffuse = g_fColour;

} // Vertex

//-- RotateAxis ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::RotateAxis( float angle, float x, float y, float z )
{
  XMVECTOR axis = XMVectorSet( x, y, z, 1.0f );
  g_matrixStack->RotateAxisLocal(&axis, XMConvertToRadians(angle));

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

	Translate( ( nx + x ) * 0.5f,
				     ( ny + y ) * 0.5f,
				     ( nz + z ) * 0.5f );

  DiffuseUVCubeVertexShader* pShader = g_envMapSet 
                                     ? &DiffuseUVEnvCubeVertexShader::StaticType
                                     : &DiffuseUVCubeVertexShader::StaticType;

  XMVECTOR vScale = XMVectorSet((x - nx) * 0.5f, (y - ny) * 0.5f, (z - nz) * 0.5f, 1.0f);

  pShader->SetScale(&vScale);
  pShader->SetColour(&g_fColour);

  SetShader( pShader );
  CommitTransforms(pShader);
	CommitTextureState();

	PopMatrix();

  unsigned int strides = sizeof(PosColNormalUVVertex), offsets = 0;
  m_pD3D11Contex->IASetVertexBuffers(0, 1, &g_pCubeVbuffer, &strides, &offsets);
  m_pD3D11Contex->IASetIndexBuffer( g_pCubeIbuffer, DXGI_FORMAT_R16_UINT, 0 );
  m_pD3D11Contex->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_pD3D11Contex->IASetInputLayout( g_pPosNormalColUVDeclaration );

  m_pD3D11Contex->DrawIndexed(36, 0, 0);

} // Cube

#define PI180	0.0174532925199432957692369076848861f	// pi / 180

inline void RotY( const float angle, XMVECTOR& vec)
{
	float s = (float) sin( PI180*angle );
	float c = (float) cos( PI180*angle );
	float X = XMVectorGetX(vec);
  vec = XMVectorSetX(vec, XMVectorGetX(vec)*c + XMVectorGetZ(vec)*s);
  vec = XMVectorSetZ(vec, -X*s + XMVectorGetZ(vec)*c);
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

  XMVECTOR a, b, ay, by;
	float dy=360.f/del_uhol_y;
	float dx=180.f*PI180/del_uhol_x;
	for(int x=0; x<del_uhol_x; x++)
	{
		float tx = (float)x/(float)del_uhol_x;
		float ang = (tx*180.f-90.f)*PI180;		// <-90, 90)
    a = XMVectorSet(0, (float)sin(ang), (float)cos(ang), 1.0f);
    b = XMVectorSet(0, (float)sin(ang + dx), (float)cos(ang + dx), 1.0f);

		//	v[x*2*(del_uhol_y+1)+0].t0.set( 0, 1.f-tx);
    XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 0].Normal, a);
    XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 0].Coord, size*a);

		//	v[x*2*(del_uhol_y+1)+1].t0.set( 0, 1.f-(tx+1.f/del_uhol_x));
    XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 1].Normal, b);
    XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 1].Coord, size*b);

		ay = a;	by = b;
		for(int y=1; y<del_uhol_y+1; y++)
		{
			float ty = (float)y/(float)del_uhol_y;
			ay = a;	RotY( ty*360.f, ay);
			by = b;	RotY( ty*360.f, by);

			//	v[x*2*(del_uhol_y+1)+2*y].t0.set( ty, 1.f-tx);
      XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 2 * y].Normal, ay);
      XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 2 * y].Coord, size*ay);

			//	v[x*2*(del_uhol_y+1)+2*y+1].t0.set( ty, 1.f-(tx+1.f/del_uhol_x));
      XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 2 * y + 1].Normal, by);
      XMStoreFloat3(&v[x * 2 * (del_uhol_y + 1) + 2 * y + 1].Coord, size*by);
		}
	}

	DiffuseUVVertexShader* pShader = NULL;
	if ( g_envMapSet )
	{
		pShader = &DiffuseUVEnvVertexShader::StaticType;
	}
	else
	{
		pShader = &DiffuseUVVertexShader::StaticType;
	}

  SetShader( pShader );
  CommitTransforms(pShader);
	CommitTextureState();

  UpdateVBuffer(v, del_uhol_x * 2 * (del_uhol_y + 1));
  for (int i = 0; i < del_uhol_x; i++)
    DrawPrimitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 6 * del_uhol_y, i * 2 * (del_uhol_y + 1));

	delete [] v;
}

//-- SetEnvTexture ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetEnvTexture(TextureDX* texture)
{
	if ( texture == SCRATCH_TEXTURE )
	{
    SetShaderTexture( 0, g_scratchTexture );
  }
  else
	{
    SetShaderTexture( 0, texture );
  }
  g_envMapSet = texture != NULL;

} // SetEnvTexture

//-- SetTexture ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetTexture(TextureDX* texture)
{
	if ( texture == SCRATCH_TEXTURE )
	{
    SetShaderTexture( 0, g_scratchTexture );
	}
  else
  {
    SetShaderTexture( 0, texture );
  }
  g_textureSet = texture != NULL;

} // SetTexture

//-- SetRenderTarget ----------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetRenderTarget(TextureDX* texture)
{
  if (texture == NULL || texture->GetRenderTarget() == NULL)
    return;

  ID3D11Texture2D* pTexture = texture->GetTexture();
  D3D11_TEXTURE2D_DESC txDesc, dtxDesc;
  pTexture->GetDesc(&txDesc);

  ID3D11Resource* pResource = NULL; ID3D11Texture2D* pDepthTexture = NULL;
  g_depthView->GetResource(&pResource);
  pResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pDepthTexture));
  pDepthTexture->GetDesc(&dtxDesc);
  pDepthTexture->Release();
  pResource->Release();

  // depth view dimension must be equals render target dimension
  if (txDesc.Width != dtxDesc.Width || txDesc.Height != dtxDesc.Height)
  {
    SAFE_RELEASE( g_depthView );

    CD3D11_TEXTURE2D_DESC depthBufferDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, txDesc.Width, txDesc.Height, 1, 1, D3D11_BIND_DEPTH_STENCIL);
    m_pD3D11Device->CreateTexture2D(&depthBufferDesc, NULL, &pDepthTexture);

    CD3D11_DEPTH_STENCIL_VIEW_DESC viewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    m_pD3D11Device->CreateDepthStencilView(pDepthTexture, &viewDesc, &g_depthView);
    pDepthTexture->Release();
  }
  else
    m_pD3D11Contex->ClearDepthStencilView(g_depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);

  ID3D11RenderTargetView* rtView = texture->GetRenderTarget();

  // change view port to render target dimension like dx9 does
  CD3D11_VIEWPORT vp(pTexture, rtView);
  m_pD3D11Contex->RSSetViewports(1, &vp);

  m_pD3D11Contex->OMSetRenderTargets(1, &rtView, g_depthView);

} // SetRenderTarget

void Renderer::GetBackBuffer()
{
  SAFE_RELEASE(g_backBuffer);
  SAFE_RELEASE(g_oldDepthBuffer);
  m_pD3D11Contex->OMGetRenderTargets(1, &g_backBuffer, &g_oldDepthBuffer);
}

void Renderer::ReleaseBackbuffer()
{
  SAFE_RELEASE(g_backBuffer);
  SAFE_RELEASE(g_oldDepthBuffer);
}

//-- SetRenderTargetBackBuffer ------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::SetRenderTargetBackBuffer()
{
  // change view port to render target dimension like dx9 does
  m_pD3D11Contex->RSSetViewports(1, &g_viewport);
  m_pD3D11Contex->OMSetRenderTargets(1, &g_backBuffer, g_oldDepthBuffer);
} // SetRenderTargetBackBuffer


void Renderer::CompileShaders()
{
  m_pD3D11Device->CreateInputLayout( declPosNormalColUV, ARRAYSIZE(declPosNormalColUV), 
                                     DiffuseUVEnvVertexShaderCode,
                                     sizeof(DiffuseUVEnvVertexShaderCode),
                                     &g_pPosNormalColUVDeclaration );
  /*m_pD3D11Device->CreateInputLayout( declPosColUV, ARRAYSIZE(declPosColUV),
                                     DiffuseUVVertexShaderCode,
                                     sizeof(DiffuseUVVertexShaderCode),
                                     &g_pPosColUVDeclaration );
  m_pD3D11Device->CreateInputLayout( declPosNormal, ARRAYSIZE(declPosNormal),
                                     DiffuseNormalEnvCubeVertexShaderCode,
                                     sizeof(DiffuseNormalEnvCubeVertexShaderCode),
                                     &g_pPosNormalDeclaration );*/

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
  g_normal = XMVectorSet(nx, ny, nz, 1.0f);
  g_normal = XMVector3Normalize(g_normal);
}

//-- ClearFloat ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::ClearFloat(float r, float g, float b)
{
  ID3D11RenderTargetView* rtView = NULL;
  ID3D11DepthStencilView* depthView = NULL;
  m_pD3D11Contex->OMGetRenderTargets(1, &rtView, &depthView);

  if (rtView != NULL)
  {
    float color[4] = { r, g, b, 1.0 };
    m_pD3D11Contex->ClearRenderTargetView(rtView, color);
    rtView->Release();
  }
  if (depthView != NULL)
  {
    m_pD3D11Contex->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1.0, 0);
    depthView->Release();
  }
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
	v[0].FDiffuse = g_fColour;
	v[0].u = 0.5f / 256.0f;
	v[0].v = 0.5f / 256.0f;

	v[1].Coord.x = x2;
	v[1].Coord.y = y1;
	v[1].Coord.z = 0.0f;
	v[1].FDiffuse = g_fColour;
	v[1].u = 1.0f;
	v[1].v = 0.5f / 256.0f;

	v[2].Coord.x = x2;
	v[2].Coord.y = y2;
	v[2].Coord.z = 0.0f;
	v[2].FDiffuse = g_fColour;
	v[2].u = 1.0f;
	v[2].v = 1.0f;

	v[3].Coord.x = x1;
	v[3].Coord.y = y2;
	v[3].Coord.z = 0.0f;
	v[3].FDiffuse = g_fColour;
	v[3].u = 0.5f / 256.0f;
	v[3].v = 1.0f;

	DiffuseUVVertexShader* pShader = &DiffuseUVVertexShader::StaticType;
  SetShader( pShader );
	CommitTransforms( pShader );
	CommitTextureState();

  UpdateVBuffer( v, 4 );

  m_pD3D11Contex->OMSetDepthStencilState( states->DepthNone(), 0 );
  DrawPrimitive( D3DPT_TRIANGLEFAN, 6, 0 );
  m_pD3D11Contex->OMSetDepthStencilState( states->DepthDefault(), 0 );

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
ID3D11Device* Renderer::GetDevice()
{
	return m_pD3D11Device;

} // GetDevice

//-- GetContext ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
ID3D11DeviceContext* Renderer::GetContext()
{
  return m_pD3D11Contex;

} // GetContext

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


//-- CreatePixelShader --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::CreatePixelShader( const void* pCode, unsigned int iCodeLen, ID3D11PixelShader** ppPixelShader )
{
  m_pD3D11Device->CreatePixelShader( pCode, iCodeLen, NULL, ppPixelShader );

} // CreatePixelShader

//-- CreateVertexShader --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer::CreateVertexShader( const void* pCode, unsigned int iCodeLen, ID3D11VertexShader** ppVertexShader )
{
  m_pD3D11Device->CreateVertexShader( pCode, iCodeLen, NULL, ppVertexShader );

} // CreateVertexShader

void Renderer::SetFillMode( int fillMode )
{
	if ( fillMode == 0 )
	{
    m_pD3D11Contex->RSSetState(states->CullNone());
	}
	else
	{
    m_pD3D11Contex->RSSetState(states->Wireframe());
	}
}

void Renderer::CreateRenderTarget(ID3D11Texture2D* pTexture, ID3D11RenderTargetView** ppRTView)
{
  if (pTexture)
  {
    CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(pTexture, D3D11_RTV_DIMENSION_TEXTURE2D);
    m_pD3D11Device->CreateRenderTargetView(pTexture, &rtDesc, ppRTView);
  }
}

void Renderer::CreateShaderView(ID3D11Texture2D* pTexture, ID3D11ShaderResourceView** ppSRView)
{
  if (pTexture)
  {
    CD3D11_SHADER_RESOURCE_VIEW_DESC srDesc(pTexture, D3D11_SRV_DIMENSION_TEXTURE2D);
    m_pD3D11Device->CreateShaderResourceView(pTexture, &srDesc, ppSRView);
  }
}

CommonStates* Renderer::GetStates()
{
  return states.get();
}

void Renderer::UpdateVBuffer(PosColNormalUVVertex* verticies, unsigned int iNum)
{
  D3D11_MAPPED_SUBRESOURCE res;
  if (S_OK == m_pD3D11Contex->Map(g_pFixVbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, verticies, sizeof(PosColNormalUVVertex) * iNum);
    m_pD3D11Contex->Unmap(g_pFixVbuffer, 0);
  }
}

void Renderer::SetShader(Shader* pShader)
{
  if (pShader->m_ShaderType == ST_VERTEX)
    m_pD3D11Contex->VSSetShader(pShader->GetVertexShader(), NULL, 0);
  else if (pShader->m_ShaderType == ST_PIXEL)
    m_pD3D11Contex->PSSetShader(pShader->GetPixelShader(), NULL, 0);
}

void Renderer::SetShaderTexture(unsigned int iStartSlot, TextureDX* pTexture)
{
  ID3D11ShaderResourceView* views[1] = { NULL };
  if (pTexture != NULL && pTexture->GetTexture() != NULL)
      views[0] = pTexture->GetShaderView();
  m_pD3D11Contex->PSSetShaderResources(iStartSlot, 1, views);
}

void Renderer::DrawPrimitive(unsigned int primType, unsigned int numPrims, unsigned int startIndex)
{
  unsigned int strides = sizeof(PosColNormalUVVertex), offsets = 0;
  m_pD3D11Contex->IASetVertexBuffers( 0, 1, &g_pFixVbuffer, &strides, &offsets );
  m_pD3D11Contex->IASetInputLayout( g_pPosNormalColUVDeclaration );

  if (primType == D3DPT_TRIANGLEFAN)
  {
    m_pD3D11Contex->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    m_pD3D11Contex->IASetIndexBuffer( g_pFanIbuffer, DXGI_FORMAT_R16_UINT, 0 );
    m_pD3D11Contex->DrawIndexed( numPrims, 0, startIndex );
  }
  else
  {
    m_pD3D11Contex->IASetPrimitiveTopology( (D3D_PRIMITIVE_TOPOLOGY)primType );
    m_pD3D11Contex->Draw( numPrims, startIndex );
  }
}