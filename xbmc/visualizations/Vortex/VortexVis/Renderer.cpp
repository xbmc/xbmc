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

#include "Renderer.h"
#include "XGraphics.h"
#include <stdio.h>


#define MAX_VERTICES (4096)

struct VertexPC_t
{
  D3DVECTOR coord;
  unsigned int colour;
};

//-----------------------------------------------------------------------------
//                              S H A D E R S
//-----------------------------------------------------------------------------

const char* VShaderSphereMapSrc  =
	"xvs.1.1\n"
	// Transform coord
	"m4x4 oPos, v0, c0                  \n"

	"add r0, v0, -c6                    \n" // r0 = in = vertex - eye

	"dp3 r0.w, r0, r0                   \n"
	"rsq r0.w, r0.w                     \n"
	"mul r0.xyz, r0, r0.w               \n"

	"dp3 r1, r0, v2                     \n"    // E.N
	"mul r1, r1, v2                     \n"    // (E.N) * N
	"mad r1, r1, c7, r0                 \n"

	// r1 = f
	// m = 2.f * sqrt( fx^2 + fy^2 + (fz+1)^2) )

	"add r2, r1, c8						\n" // fz += 1

	"dp3 r2, r2, r2                     \n"
	"rsq r2, r2                         \n" // r2 = 1.f / sqrt( fx^2 + fy^2 + (fz+1)^2) )

	"mul r2, r2, c5.xxx                 \n" // r2 = 1.f / 2.f * sqrt( fx^2 + fy^2 + (fz+1)^2) )

	"mul r0.xyz, r1, r2					\n"
	"add oT0.xyz, r0, c5.xxx            \n"
	"mov oD0.xyz, v1					\n";



const char* VShaderSimpleSrc =
    "xvs.1.1\n"

	// Transform coord
    "mul r0, v0.x, c0                   \n"
    "mad r0, v0.y, c1, r0               \n"
    "mad r0, v0.z, c2, r0               \n"
    "add oPos, c3, r0                   \n"
	"mov oD0, v1                        \n";

const char* VShaderSimpleTex0Src =
    "xvs.1.1\n"
    // Transform coord
    "m4x4 oPos, v0, c0                  \n"
	"mov oD0, v1                        \n"
	"mov oT0, v3                        \n";


// Pixel shader for colour remapping
const char* PShaderColourMapSrc = 
    "xps.1.1\n"
	"tex t0\n"
	"texreg2ar t1, t0\n"
	"mul r0, v0.a, t1\n"
	"xfc zero, zero, zero, r0.rgb, zero, zero, 1-zero\n";


//-----------------------------------------------------------------------------
//                          V E R T E X   D E C L S
//-----------------------------------------------------------------------------

DWORD DeclPosColNormal[] =
{
	D3DVSD_STREAM(0),
	D3DVSD_REG( 0,  D3DVSDT_FLOAT3 ),	// Position
	D3DVSD_REG( 1,  D3DVSDT_D3DCOLOR ),	// Colour
	D3DVSD_REG( 2,  D3DVSDT_FLOAT3 ),	// Normal
	D3DVSD_END()
};

DWORD DeclPosCol[] =
{
	D3DVSD_STREAM(0),
	D3DVSD_REG( 0,  D3DVSDT_FLOAT3 ),	// Position
	D3DVSD_REG( 1,  D3DVSDT_D3DCOLOR ),	// Colour
	D3DVSD_END()
};

DWORD DeclPCNU0[] =
{
	D3DVSD_STREAM(0),
	D3DVSD_REG( 0,  D3DVSDT_FLOAT3 ),	// Position
	D3DVSD_REG( 1,  D3DVSDT_D3DCOLOR ), // Colour
	D3DVSD_REG( 2,  D3DVSDT_FLOAT3 ),   // Normal
	D3DVSD_REG( 3,  D3DVSDT_FLOAT2 ),   // Texture 0
	D3DVSD_END()
};


//-----------------------------------------------------------------------------
//                            V A R I A B L E S
//-----------------------------------------------------------------------------
namespace
{
	LPDIRECT3DDEVICE8 g_device;
	D3DVIEWPORT8  g_viewport;
	ID3DXMatrixStack* g_matrixStack;
	int g_matrixStackLevel;
	D3DXMATRIX  g_matView;
	D3DXMATRIX  g_matProj;
	IDirect3DSurface8*	g_backBuffer;
	IDirect3DSurface8*	g_depthBuffer;
	IDirect3DSurface8*	g_oldDepthBuffer;

	LPDIRECT3DTEXTURE8 g_scratchTexture;
	bool g_ownDepth;

	// Vertex Shaders
	DWORD g_vShaders[MAX_VSHADERS]={0};
	DWORD g_pShaders[MAX_PSHADERS]={0};

	// Current state
	D3DVECTOR g_normal;
	float g_tex0U;
	float g_tex0V;
	unsigned int g_colour;
	D3DPRIMITIVETYPE g_primType;
	bool g_envMapSet;
	bool g_textureSet;
	float g_aspect;
	float g_defaultAspect;
	float g_aspectValue;


	// Vertices
	Vertex_t g_vertices[MAX_VERTICES];
	int g_currentVertex;

};

//-- Init ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Init(LPDIRECT3DDEVICE8 device, int xPos, int yPos, int width, int height, float pixelRatio)
{
	//  HRESULT hr;

	//  OutputDebugString("Vortex INFO: Enter Renderer::Init\n");

	g_device = device;
	g_viewport.X = xPos;
	g_viewport.Y = yPos;
	g_viewport.Width = width;
	g_viewport.Height = height;
	g_viewport.MinZ = 0;
	g_viewport.MaxZ = 1;
	g_defaultAspect = ((float)width / height) * pixelRatio;

	//-------------------------------------------------------------------------
	// Get backbuffer info
	g_device->GetBackBuffer(0, 0, &g_backBuffer);
	g_backBuffer->Release();
	g_device->CreateDepthStencilSurface(512, 512, D3DFMT_LIN_D24S8, 0, &g_depthBuffer);
	if (D3D_OK != g_device->GetDepthStencilSurface(&g_oldDepthBuffer))
	{
		//OutputDebugString("Vortex INFO: Renderer::Init - Failed to Get old depth stencil\n");
		// Create our own
		D3DSURFACE_DESC desc;
		g_backBuffer->GetDesc(&desc);
		g_device->CreateDepthStencilSurface(desc.Width, desc.Height, D3DFMT_LIN_D24S8, 0, &g_oldDepthBuffer);
		g_ownDepth = true;
	}
	else
	{
		g_ownDepth = false;
	}

	D3DXCreateMatrixStack(0, &g_matrixStack);
	g_matrixStackLevel = 0;

	g_scratchTexture = CreateTexture(512,512);

	//---------------------------------------------------------------------------
	// Build shaders
	LPXGBUFFER pUcode;
	LPXGBUFFER errorLog;

	//  OutputDebugString("Vortex INFO: Renderer::Init - Assemble shader 1\n");

	AssembleShader(NULL, VShaderSphereMapSrc, strlen(VShaderSphereMapSrc), 0, NULL, &pUcode, &errorLog, NULL, NULL, NULL, NULL );
	g_device->CreateVertexShader(DeclPCNU0, (DWORD*)pUcode->pData, &g_vShaders[VSHADER_SPHEREMAP], 0 );
	pUcode->Release();
	errorLog->Release();


	//  AssembleShader(NULL, ShaderObjectLinear, strlen(ShaderObjectLinear), 0, NULL, &pUcode, NULL, NULL, NULL, NULL, NULL );
	//	g_device->CreateVertexShader(DeclPosColNormal, (DWORD*)pUcode->pData, &g_shaders[ST_SPHEREMAP], 0 );
	//  pUcode->Release();

	//  OutputDebugString("Vortex INFO: Renderer::Init - Assemble shader 2\n");
	AssembleShader(NULL, VShaderSimpleSrc, strlen(VShaderSimpleSrc), 0, NULL, &pUcode, NULL, NULL, NULL, NULL, NULL );
	g_device->CreateVertexShader(DeclPosCol, (DWORD*)pUcode->pData, &g_vShaders[VSHADER_SIMPLE], 0 );
	pUcode->Release();

	//  OutputDebugString("Vortex INFO: Renderer::Init - Assemble shader 3\n");
	AssembleShader(NULL, VShaderSimpleTex0Src, strlen(VShaderSimpleTex0Src), 0, NULL, &pUcode, NULL, NULL, NULL, NULL, NULL );
	g_device->CreateVertexShader(DeclPCNU0, (DWORD*)pUcode->pData, &g_vShaders[VSHADER_SIMPLETEX0], 0 );
	pUcode->Release();

	AssembleShader(NULL, PShaderColourMapSrc, strlen(PShaderColourMapSrc), 0, NULL, &pUcode, NULL, NULL, NULL, NULL, NULL );
	g_device->CreatePixelShader((D3DPIXELSHADERDEF*)(DWORD*)pUcode->pData, &g_pShaders[PSHADER_COLREMAP]);
	pUcode->Release();


	//  OutputDebugString("Vortex INFO: Leave Renderer::Init\n");

} // Init

//-- Exit ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Exit()
{

	// Release shaders

	g_device->SetVertexShader(0);
	g_device->SetPixelShader(0);

	for (int i = 0; i < MAX_PSHADERS; i++)
	{
		if ( g_pShaders[i] )
		{
			g_device->DeletePixelShader(g_pShaders[i]);
		}
	}

	for (int i = 0; i < MAX_VSHADERS; i++)
	{
		if ( g_vShaders[i] )
		{
			g_device->DeleteVertexShader(g_vShaders[i]);
		}
	}

	g_oldDepthBuffer->Release();
	g_depthBuffer->Release();
	g_scratchTexture->Release();

	g_matrixStack->Release();

} // Exit

//-- GetDevice ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
D3DDevice* Renderer_c::GetDevice()
{
	return g_device;

} // GetDevice

//-- CreatTexture -------------------------------------------------------------
//
//-----------------------------------------------------------------------------
LPDIRECT3DTEXTURE8 Renderer_c::CreateTexture(int width, int height, D3DFORMAT format)
{
	LPDIRECT3DTEXTURE8 texture;
	IDirect3DSurface8* surface;
	D3DLOCKED_RECT lock;

	g_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &texture);

	if (texture == NULL)
	{
		//    OutputDebugString("Vortex ERROR: Unable to allocate texture\n");
		return NULL;
	}

	texture->GetSurfaceLevel(0, &surface);
	surface->LockRect(&lock, NULL, 0);
	unsigned int texSize = (unsigned int)(width * height * 4);
	memset(lock.pBits, 0, texSize);
	surface->UnlockRect();
	surface->Release();
	return texture;

} // CreateTexture

//-- LoadTexture --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
LPDIRECT3DTEXTURE8 Renderer_c::LoadTexture(char* filename)
{

	LPDIRECT3DTEXTURE8 texture = NULL;
	D3DXCreateTextureFromFileEx(g_device, filename, 0, 0, 1, 0, D3DFMT_UNKNOWN, 0, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, 0x00000000, NULL, NULL, &texture);

	if (texture == NULL)
	{
		OutputDebugString("Vortex ERROR: Unable to load texture ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	}

	return texture;

} // LoadTexture

//-- ReleaseTexture -----------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::ReleaseTexture(LPDIRECT3DTEXTURE8 texture)
{
    if (!texture)
	{
		return;
	}

    texture->Release();

} // ReleaseTexture

//-- CopyToScratch ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::CopyToScratch()
{
	// Copy current render target to scratch buffer
	IDirect3DSurface8* currSurface;
	IDirect3DSurface8* destSurface;

	g_device->GetRenderTarget(&currSurface);
	g_scratchTexture->GetSurfaceLevel(0, &destSurface);

	RECT src;
	POINT dst;
	src.left = 0;
	src.right = 512;
	src.top = 0;
	src.bottom = 512;
	dst.x = 0;
	dst.y = 0;

	g_device->CopyRects(currSurface, &src, 1, destSurface, &dst);
	currSurface->Release();
	destSurface->Release();

} // CopyToScratch

//-- SetRenderTarget ----------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetRenderTarget(LPDIRECT3DTEXTURE8 texture)
{
	IDirect3DSurface8* surface;
	texture->GetSurfaceLevel(0, &surface);

	g_device->SetRenderTarget(surface, g_depthBuffer);
	g_device->Clear(0, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0, 1.0f, 0);

	surface->Release();

} // SetRenderTarget

//-- SetTexture ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetTexture(LPDIRECT3DTEXTURE8 texture)
{
	if (texture == SCRATCH_TEXTURE)
	{
		g_device->SetTexture(0, g_scratchTexture);
	}
	else
	{
		g_device->SetTexture(0, texture);
	}
	g_textureSet = texture != NULL;

} // SetTexture

//-- SetEnvTexture ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetEnvTexture(LPDIRECT3DTEXTURE8 texture)
{
	if (texture == SCRATCH_TEXTURE)
	{
		g_device->SetTexture(0, g_scratchTexture);
	}
	else
	{
		g_device->SetTexture(0, texture);
	}
	g_envMapSet = texture != NULL;

} // SetEnvTexture


//-- SetVShader ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetVShader(VShaderType_e shaderType)
{
	g_device->SetVertexShader(g_vShaders[shaderType]);

} // SetVShader

//-- SetPShader ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetPShader(PShaderType_e shaderType)
{
	g_device->SetPixelShader(g_pShaders[shaderType]);

} // SetPShader

//-- SetRenderTargetBackBuffer ------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetRenderTargetBackBuffer()
{
	g_device->SetRenderTarget(g_backBuffer, g_oldDepthBuffer);	

	g_device->SetViewport(&g_viewport);

} // SetRenderTargetBackBuffer

//-- SetDefaults --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetDefaults()
{
	for (;g_matrixStackLevel > 0; g_matrixStackLevel--)
	{
		g_matrixStack->Pop();
	}

	g_matrixStack->LoadIdentity();

	// Renderstates
	d3dSetRenderState(D3DRS_LIGHTING, FALSE);
	d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3dSetRenderState(D3DRS_ZENABLE, TRUE);
	d3dSetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	d3dSetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	d3dSetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	d3dSetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	d3dSetRenderState(D3DRS_ALPHAREF, 0);

	float lineWidth = 1.0f;

	d3dSetRenderState(D3DRS_LINEWIDTH, *((DWORD*) (&lineWidth)));
	d3dSetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	d3dSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);

	d3dSetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	d3dSetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	for (int i = 0; i < 4; i++)
	{
		d3dSetTextureStageState(i, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
		d3dSetTextureStageState(i, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		d3dSetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
		d3dSetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		g_device->SetTexture(i, NULL);
	}

	g_envMapSet = false;
	g_textureSet = false;

	// Globals
	g_colour = 0xffffffff;
	g_aspect = g_defaultAspect;
	g_aspectValue = 1.0f;

	SetDrawMode3d();
	D3DXMATRIX matView;
	D3DXMatrixIdentity(&matView);
	SetView(matView);

	SetBlendMode(BLEND_OFF); 

} // SetDefaults


//-- SetBlendMode -------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetBlendMode(BlendMode_e blendMode)
{
	switch (blendMode)
	{
	case BLEND_OFF:
		{
			d3dSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			d3dSetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			break;
		}
	case BLEND_ADD:
		{
			d3dSetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			d3dSetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			d3dSetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			d3dSetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			d3dSetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		}
	case BLEND_MOD:
		{
			d3dSetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			d3dSetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			d3dSetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			d3dSetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			d3dSetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		}
	case BLEND_MAX:
		{
			d3dSetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			d3dSetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
			d3dSetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			d3dSetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			d3dSetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		}

	default:
		{
			d3dSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			d3dSetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

			break;
		}
	}

} // SetBlendMode

//-- SetDrawMode2d ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetDrawMode2d()
{
	D3DXMATRIX matProj;
	D3DXMatrixOrthoLH(&matProj, 2, -2, 0, 1);
	Renderer_c::SetProjection(matProj);
	g_device->SetRenderState(D3DRS_ZENABLE, FALSE);

} // SetDrawMode2d

//-- SetDrawMode3d ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetDrawMode3d()
{
	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, g_aspect, 0.25f, 2000.0f );
	Renderer_c::SetProjection(matProj);
	g_device->SetRenderState(D3DRS_ZENABLE, TRUE);

} // SetDrawMode3d

//-- GetAspect ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
float Renderer_c::GetAspect()
{
	return g_aspectValue;

} // GetAspect

//-- SetAspect ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetAspect(float aspect)
{
	D3DXMATRIX matProj;

	aspect = min(aspect, 1);
	aspect = max(aspect, 0);
	g_aspectValue = aspect;
	g_aspect = 1.0f + aspect * ((g_defaultAspect) - 1.0f);

	D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, g_aspect, 0.25f, 2000.0f );
	Renderer_c::SetProjection(matProj);

} // SetAspect

//-- Clear --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Clear(unsigned int colour)
{
	colour |= 0xff000000;
	g_device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, colour, 1.0f, 0);

} // Clear

//-- ClearFloat ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::ClearFloat(float r, float g, float b)
{
	unsigned char cr = (unsigned char) (r * 255.0f);
	unsigned char cg = (unsigned char) (g * 255.0f);
	unsigned char cb = (unsigned char) (b * 255.0f);
	unsigned char ca = (unsigned char) (1.0f * 255.0f);
	unsigned int colour = D3DCOLOR_ARGB(ca, cr, cg, cb);

	g_device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, colour, 1.0f, 0);

} // ClearFloat

//-- SetProjection ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetProjection(D3DXMATRIX& matProj)
{
	g_matProj = matProj;

} // SetProjection

//-- SetView ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetView(D3DXMATRIX& matView)
{
	g_matView = matView;

} // SetView

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
	case D3DPT_LINELOOP:
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
	case D3DPT_QUADLIST:
		{
			if (numVerts < 4)
				return -1;
			if (numVerts % 4 != 0)
				return -1;
			return numVerts / 4;
		}
	case D3DPT_QUADSTRIP:
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
void Renderer_c::Begin(D3DPRIMITIVETYPE primType)
{
	g_primType = primType;
	g_currentVertex = 0;

} // Begin

//-- End ----------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::End()
{
	int primCount = NumPrimsFromType(g_primType, g_currentVertex);
	if (primCount == -1)
		return;

	CommitStates();

	g_device->DrawPrimitiveUP(g_primType, primCount, g_vertices, sizeof(Vertex_t));

} // End

inline D3DXVECTOR3 MulVec( const D3DXMATRIX &m, const D3DXVECTOR3 &v )
{																
	return D3DXVECTOR3( m._11*v.x + m._12*v.y + m._13*v.z + m._14,	
			            m._21*v.x + m._22*v.y + m._23*v.z + m._24,	
			            m._31*v.x + m._32*v.y + m._33*v.z + m._34 );
}

//-- CommitStates -------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::CommitStates()
{
	D3DXMATRIX worldView, temp2;
	D3DXMATRIX* worldMat = g_matrixStack->GetTop();
	D3DXMatrixMultiply(&worldView, worldMat, &g_matView);
	D3DXMatrixMultiply(&temp2, &worldView, &g_matProj);

	D3DXMatrixTranspose(&temp2, &temp2);

	D3DXMATRIX invMat;
	D3DXMatrixInverse(&invMat, NULL, worldMat);
	D3DXVECTOR3 vec(0,0,0);
	D3DXVECTOR3 camPos;


	D3DXMatrixTranspose(&invMat, &invMat);

	camPos = MulVec(invMat, vec);

	D3DXVECTOR4 c5(0.5f, -0.5f, 0.5f, 0.5f);
	D3DXVECTOR4 c7(-2, -2, -2, -2);
	D3DXVECTOR4 c8(0, 0, 1, 0);

	g_device->SetVertexShaderConstant(0, &temp2, 4);
	g_device->SetVertexShaderConstant(5, &c5, 1);
	g_device->SetVertexShaderConstant(6, &camPos, 1);
	g_device->SetVertexShaderConstant(7, &c7, 1);
	g_device->SetVertexShaderConstant(8, &c8, 1);
	g_device->SetVertexShaderConstant(10, &worldView, 1);

	if (g_envMapSet)
		g_device->SetVertexShader(g_vShaders[VSHADER_SPHEREMAP]);
	else
		g_device->SetVertexShader(g_vShaders[VSHADER_SIMPLETEX0]);

	if (g_textureSet || g_envMapSet)
	{
		d3dSetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		d3dSetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		d3dSetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
		d3dSetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
		d3dSetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		d3dSetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	}
	else
	{
		d3dSetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		d3dSetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
		d3dSetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
		d3dSetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
	}


} // CommitStates

__forceinline float ClampCol(float c)
{
	if (c < 0)
		c = 0;
	else if (c > 1)
		c = 1;

	return c;
}

//-- Colour -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Colour(float r, float g, float b, float a)
{
	ClampCol(r);
	ClampCol(g);
	ClampCol(b);
	ClampCol(a);
	unsigned char cr = (unsigned char) (r * 255.0f);
	unsigned char cg = (unsigned char) (g * 255.0f);
	unsigned char cb = (unsigned char) (b * 255.0f);
	unsigned char ca = (unsigned char) (a * 255.0f);
	g_colour = D3DCOLOR_ARGB(ca, cr, cg, cb);

} // Colour

//-- Normal -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Normal(float nx, float ny, float nz)
{
	g_normal.x = nx;
	g_normal.y = ny;
	g_normal.z = nz;

} // Normal

//-- Normal -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::NormalVec(D3DVECTOR& normal)
{
	g_normal = normal;

} // Normal

//-- TexCoord -----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::TexCoord(float u, float v)
{
	g_tex0U = u;
	g_tex0V = v;

} // TexCoord

//-- Vertex -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Vertex(float x, float y, float z)
{
	if (g_currentVertex == MAX_VERTICES)
	{
		OutputDebugString("Vortex WARNING: Too many vertices\n");
		return;
	}

	Vertex_t& v = g_vertices[g_currentVertex++];

	v.coord.x = x;
	v.coord.y = y;
	v.coord.z = z;
	v.colour = g_colour;
	v.normal = g_normal;
	v.u = g_tex0U;
	v.v = g_tex0V;

} // Vertex

//-- VertexVec ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::VertexVec(D3DVECTOR& pos)
{
	Vertex(pos.x, pos.y, pos.z);

} // VertexVec

//-- Cube ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Cube(float nx, float ny, float nz, float x, float y, float z)
{
	//    4-------5
	//   /|      /|
	//  0-------1 |
	//  | 7-----|-6
	//  |/      |/
	//  3-------2

	static float mySize = 1.5f;

	Vertex_t v[24];

	// Front Face
	v[0].coord  = D3DXVECTOR3(nx, y, nz);
	v[0].colour = g_colour;
	v[0].normal = D3DXVECTOR3(0, 0, -1);
	v[0].u = 0;
	v[0].v = 0;
	v[1].coord  = D3DXVECTOR3(x, y, nz);
	v[1].colour = g_colour;
	v[1].normal = D3DXVECTOR3(0, 0, -1);
	v[1].u = 1;
	v[1].v = 0;
	v[2].coord  = D3DXVECTOR3(x, ny, nz);
	v[2].colour = g_colour;
	v[2].normal = D3DXVECTOR3(0, 0, -1);
	v[2].u = 1;
	v[2].v = 1;
	v[3].coord  = D3DXVECTOR3(nx, ny, nz);
	v[3].colour = g_colour;
	v[3].normal = D3DXVECTOR3(0, 0, -1);
	v[3].u = 0;
	v[3].v = 1;

	// Back Face
	v[4].coord  = D3DXVECTOR3(x, y, z);
	v[4].colour = g_colour;
	v[4].normal = D3DXVECTOR3(0, 0, 1);
	v[4].u = 0;
	v[4].v = 0;
	v[5].coord  = D3DXVECTOR3(nx, y, z);
	v[5].colour = g_colour;
	v[5].normal = D3DXVECTOR3(0, 0, 1);
	v[5].u = 1;
	v[5].v = 0;
	v[6].coord  = D3DXVECTOR3(nx, ny, z);
	v[6].colour = g_colour;
	v[6].normal = D3DXVECTOR3(0, 0, 1);
	v[6].u = 1;
	v[6].v = 1;
	v[7].coord  = D3DXVECTOR3(x, ny, z);
	v[7].colour = g_colour;
	v[7].normal = D3DXVECTOR3(0, 0, 1);
	v[7].u = 0;
	v[7].v = 1;

	// Top Face
	v[8].coord   = D3DXVECTOR3(nx, y, z);
	v[8].colour  = g_colour;
	v[8].normal  = D3DXVECTOR3(0, 1, 0);
	v[8].u = 0;
	v[8].v = 0;
	v[9].coord   = D3DXVECTOR3(x, y, z);
	v[9].colour  = g_colour;
	v[9].normal  = D3DXVECTOR3(0, 1, 0);
	v[9].u = 1;
	v[9].v = 0;
	v[10].coord  = D3DXVECTOR3(x, y, nz);
	v[10].colour = g_colour;
	v[10].normal = D3DXVECTOR3(0, 1, 0);
	v[10].u = 1;
	v[10].v = 1;
	v[11].coord  = D3DXVECTOR3(nx, y, nz);
	v[11].colour = g_colour;
	v[11].normal = D3DXVECTOR3(0, 1, 0);
	v[11].u = 0;
	v[11].v = 1;

	// Bottom Face
	v[12].coord  = D3DXVECTOR3(nx, ny, nz);
	v[12].colour = g_colour;
	v[12].normal = D3DXVECTOR3(0, -1, 0);
	v[12].u = 0;
	v[12].v = 0;
	v[13].coord  = D3DXVECTOR3(x, ny, nz);
	v[13].colour = g_colour;
	v[13].normal = D3DXVECTOR3(0, -1, 0);
	v[13].u = 1;
	v[13].v = 0;
	v[14].coord  = D3DXVECTOR3(x, ny, z);
	v[14].colour = g_colour;
	v[14].normal = D3DXVECTOR3(0, -1, 0);
	v[14].u = 1;
	v[14].v = 1;
	v[15].coord  = D3DXVECTOR3(nx, ny, z);
	v[15].colour = g_colour;
	v[15].normal = D3DXVECTOR3(0, -1, 0);
	v[15].u = 0;
	v[15].v = 1;

	// Left Face
	v[16].coord  = D3DXVECTOR3(nx, y, z);
	v[16].colour = g_colour;
	v[16].normal = D3DXVECTOR3(-1, 0, 0);
	v[16].u = 0;
	v[16].v = 0;
	v[17].coord  = D3DXVECTOR3(nx, y, nz);
	v[17].colour = g_colour;
	v[17].normal = D3DXVECTOR3(-1, 0, 0);
	v[17].u = 1;
	v[17].v = 0;
	v[18].coord  = D3DXVECTOR3(nx, ny, nz);
	v[18].colour = g_colour;
	v[18].normal = D3DXVECTOR3(-1, 0, 0);
	v[18].u = 1;
	v[18].v = 1;
	v[19].coord  = D3DXVECTOR3(nx, ny, z);
	v[19].colour = g_colour;
	v[19].normal = D3DXVECTOR3(-1, 0, 0);
	v[19].u = 0;
	v[19].v = 1;

	// Right Face
	v[20].coord  = D3DXVECTOR3(x, y, nz);
	v[20].colour = g_colour;
	v[20].normal = D3DXVECTOR3(1, 0, 0);
	v[20].u = 0;
	v[20].v = 0;
	v[21].coord  = D3DXVECTOR3(x, y, z);
	v[21].colour = g_colour;
	v[21].normal = D3DXVECTOR3(1, 0, 0);
	v[21].u = 1;
	v[21].v = 0;
	v[22].coord  = D3DXVECTOR3(x, ny, z);
	v[22].colour = g_colour;
	v[22].normal = D3DXVECTOR3(1, 0, 0);
	v[22].u = 1;
	v[22].v = 1;
	v[23].coord  = D3DXVECTOR3(x, ny, nz);
	v[23].colour = g_colour;
	v[23].normal = D3DXVECTOR3(1, 0, 0);
	v[23].u = 0;
	v[23].v = 1;

	CommitStates();

	g_device->DrawPrimitiveUP(D3DPT_QUADLIST, 6, &v, sizeof(Vertex_t));

} // Cube


//-- PushMatrix ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::PushMatrix()
{
	g_matrixStack->Push();

} // PushMatrix

//-- PopMatrix ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::PopMatrix()
{
	g_matrixStack->Pop();

} // PopMatrix

//-- SetIdentity --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::SetIdentity()
{
	g_matrixStack->LoadIdentity();

} // SetIdentity

//-- Rotate -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Rotate(float x, float y, float z)
{
	// g_matrixStack->RotateYawPitchRollLocal(x, y, z);
}

//-- RotateAxis ---------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::RotateAxis(float angle, float x, float y, float z)
{
	D3DXVECTOR3 axis(x, y, z);
	g_matrixStack->RotateAxisLocal(&axis, D3DXToRadian(angle));

} // RotateAxis

//-- Translate ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Translate(float x, float y, float z)
{
	g_matrixStack->TranslateLocal(x, y, z);

} // Translate

//-- Scale --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Scale(float x, float y, float z)
{
	g_matrixStack->ScaleLocal(x, y, z);

} // Scale

//-- LookAt -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
{
	D3DXVECTOR3 eye(eyeX, eyeY, eyeZ);
	D3DXVECTOR3 center(centerX, centerY, centerZ);
	D3DXVECTOR3 up(upX, upY, upZ);
	D3DXMATRIX matOut;
	D3DXMatrixLookAtLH(&matOut, &eye, &center, &up);
	g_matrixStack->MultMatrix(&matOut);

} // LookAt


//-- Rect ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::Rect(float x1, float y1, float x2, float y2)
{
	VertexPC_t v[4];

	v[0].coord.x = x1;
	v[0].coord.y = y1;
	v[0].coord.z = 0.0f;
	v[0].colour = g_colour;

	v[1].coord.x = x2;
	v[1].coord.y = y1;
	v[1].coord.z = 0.0f;
	v[1].colour = g_colour;

	v[2].coord.x = x2;
	v[2].coord.y = y2;
	v[2].coord.z = 0.0f;
	v[2].colour = g_colour;

	v[3].coord.x = x1;
	v[3].coord.y = y2;
	v[3].coord.z = 0.0f;
	v[3].colour = g_colour;

	CommitStates();
	g_device->DrawPrimitiveUP(D3DPT_QUADLIST, 1, &v, sizeof(VertexPC_t));

} // Rect

//-- TexRect ----------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::TexRect(float x1, float y1, float x2, float y2)
{
	Vertex_t v[4];

	v[0].coord.x = x1;
	v[0].coord.y = y1;
	v[0].coord.z = 0.0f;
	v[0].colour = g_colour;
	v[0].u = 0.5f / 256.0f;
	v[0].v = 0.5f / 256.0f;

	v[1].coord.x = x2;
	v[1].coord.y = y1;
	v[1].coord.z = 0.0f;
	v[1].colour = g_colour;
	v[1].u = 1.0f;
	v[1].v = 0.5f / 256.0f;

	v[2].coord.x = x2;
	v[2].coord.y = y2;
	v[2].coord.z = 0.0f;
	v[2].colour = g_colour;
	v[2].u = 1.0f;
	v[2].v = 1.0f;

	v[3].coord.x = x1;
	v[3].coord.y = y2;
	v[3].coord.z = 0.0f;
	v[3].colour = g_colour;
	v[3].u = 0.5f / 256.0f;
	v[3].v = 1.0f;

	CommitStates();
	d3dSetRenderState(D3DRS_ZENABLE, FALSE);
	g_device->DrawPrimitiveUP(D3DPT_QUADLIST, 1, &v, sizeof(Vertex_t));
	d3dSetRenderState(D3DRS_ZENABLE, TRUE);

} // TexRect

//-- DrawPrimitive ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Renderer_c::DrawPrimitive(D3DPRIMITIVETYPE primType, unsigned int startIndex, unsigned int numPrims)
{
	CommitStates();
	g_device->DrawPrimitive(primType, startIndex, numPrims);

} // DrawPrimitive

void Renderer_c::SetLineWidth(float width)
{
	d3dSetRenderState(D3DRS_LINEWIDTH, *((DWORD*) (&width)));
}
void Renderer_c::SetFillMode(int fillMode)
{
	if (fillMode == 0)
		d3dSetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	else
		d3dSetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
}
