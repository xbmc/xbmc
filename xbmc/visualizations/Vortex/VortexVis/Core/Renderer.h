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

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <d3d9.h>
#include <d3dx9.h>

enum eBlendMode
{
	BLEND_OFF = 0,
	BLEND_ADD = 1,
	BLEND_MOD = 2,
	BLEND_MAX = 3
};

struct PosColNormalUVVertex
{
	D3DXVECTOR3	Coord;
	D3DXVECTOR3	Normal;
//	DWORD Diffuse;				// diffuse color
	float u, v;					// Texture 0 coords
	D3DXVECTOR4 FDiffuse;		// float diffuse color
};

struct PosNormalVertex
{
	D3DXVECTOR3	Coord;
	D3DXVECTOR3	Normal;
};

#define SCRATCH_TEXTURE ((LPDIRECT3DTEXTURE9)-1)

extern LPDIRECT3DVERTEXDECLARATION9    g_pPosNormalColUVDeclaration;
extern LPDIRECT3DVERTEXDECLARATION9    g_pPosColUVDeclaration;
extern LPDIRECT3DVERTEXDECLARATION9    g_pPosNormalDeclaration;

extern char g_TexturePath[];

class Renderer
{
public:
	static void Init( LPDIRECT3DDEVICE9 pD3DDevice, int iXPos, int iYPos, int iWidth, int iHeight, float fPixelRatio );
	static void Exit();
	static void GetBackBuffer();
	static void ReleaseBackbuffer();
	static void SetDefaults();
	static LPDIRECT3DDEVICE9 GetDevice();

	static void SetRenderTarget(LPDIRECT3DTEXTURE9 texture);
	static void SetRenderTargetBackBuffer();

	static void SetDrawMode2d();
	static void SetDrawMode3d();
	static void SetProjection(D3DXMATRIX& matProj);
	static void SetView(D3DXMATRIX& matView);

	static void GetProjection( D3DXMATRIX& matProj );
	static LPD3DXMESH CreateD3DXTextMesh( const char* pString, bool bCentered );
	static void DrawMesh( LPD3DXMESH pMesh );


	static void Clear( DWORD Col );
	static void ClearDepth();
	static void ClearFloat(float r, float g, float b);
	static void Rect( float x1, float y1, float x2, float y2, DWORD Col );
	static void Line( float x1, float y1, float x2, float y2, DWORD Col );
	static void Point( float x, float y, DWORD Col );

	static LPDIRECT3DTEXTURE9 CreateTexture( int iWidth, int iHeight, D3DFORMAT Format = D3DFMT_X8R8G8B8, bool bDynamic = FALSE );
	static LPDIRECT3DTEXTURE9 LoadTexture( char* pFilename, bool bAutoResize = true );
	static void ReleaseTexture( LPDIRECT3DTEXTURE9 pTexture );
	static void DrawText( float x, float y, char* txt, DWORD Col = 0xffffffff);

	static int GetViewportWidth();
	static int GetViewportHeight();
	static void SetBlendMode(eBlendMode blendMode);
	static float GetAspect();
	static void SetAspect(float aspect);

	static void SetTexture(LPDIRECT3DTEXTURE9 texture);
	static void SetEnvTexture(LPDIRECT3DTEXTURE9 texture);

	static void CopyToScratch();

	static void Begin(D3DPRIMITIVETYPE primType);
	static void End();

	static void Colour(float r, float g, float b, float a);
	static void TexCoord(float u, float v);
	static void Normal(float nx, float ny, float nz);
	static void NormalVec(D3DVECTOR& normal);
	static void Vertex(float x, float y, float z);
	static void VertexVec(D3DVECTOR& pos);

	static void Cube(float nx, float ny, float nz, float x, float y, float z);
	static void Sphere(int del_uhol_x, int del_uhol_y, float size);
	static void SimpleSphere(float size);

	static void Circle(float radius, int numSides, D3DXVECTOR3& dir);
	static void PushMatrix();
	static void PopMatrix();
	static void SetIdentity();
	static void Rotate(float x, float y, float z);
	static void RotateAxis(float angle, float x, float y, float z);
	static void Translate(float x, float y, float z);
	static void Scale(float x, float y, float z);
	static void LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);

	static void Rect(float x1, float y1, float x2, float y2);
	static void TexRect(float x1, float y1, float x2, float y2);

	static void DrawPrimitive(D3DPRIMITIVETYPE primType, unsigned int startIndex, unsigned int numPrims);
	static void SetLineWidth(float width);
	static void SetFillMode(int fillMode);
	
	static void CommitTransforms( class DiffuseUVVertexShader* pShader );
	static void CommitTextureState();

	static IDirect3DPixelShader9* CreatePixelShader( DWORD* pCode );
	static IDirect3DVertexShader9* CreateVertexShader( DWORD* pCode );
	static void SetConstantTableMatrix( LPD3DXCONSTANTTABLE pConstantTable, char* pConstantName, D3DXMATRIX* pMatrix );
	static void SetConstantTableVector( LPD3DXCONSTANTTABLE pConstantTable, char* pConstantName, D3DXVECTOR4* pVector );

private:
	static void CreateCubeBuffers();
	static void CreateFonts();
	static void CompileShaders();

};

#endif
