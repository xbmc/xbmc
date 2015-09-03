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

#ifndef _RENDERER_H_
#define _RENDERER_H_

#define SAFE_RELEASE(p)      do { if(p) { (p)->Release(); (p)=NULL; } } while (0)
#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)

#include <d3d9.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "TextureDX.h"
#include "CommonStates.h"

enum eBlendMode
{
	BLEND_OFF = 0,
	BLEND_ADD = 1,
	BLEND_MOD = 2,
	BLEND_MAX = 3
};

struct PosColNormalUVVertex
{
  DirectX::XMFLOAT3	Coord;
  DirectX::XMFLOAT3	Normal;
	float u, v;         // Texture 0 coords
  DirectX::XMFLOAT4 FDiffuse;  // float diffuse color
};

struct PosNormalVertex
{
  DirectX::XMFLOAT3	Coord;
  DirectX::XMFLOAT3	Normal;
};

#define SCRATCH_TEXTURE ((TextureDX*)-1)

extern ID3D11InputLayout*    g_pPosNormalColUVDeclaration;
extern ID3D11InputLayout*    g_pPosColUVDeclaration;
extern ID3D11InputLayout*    g_pPosNormalDeclaration;

extern char g_TexturePath[];

class Shader;

class Renderer
{
public:
  static void Init( ID3D11DeviceContext* pD3DContext, int iXPos, int iYPos, int iWidth, int iHeight, float fPixelRatio);
  static void Exit();
	static void GetBackBuffer();
	static void ReleaseBackbuffer();
	static void SetDefaults();
  static ID3D11Device* GetDevice();
  static ID3D11DeviceContext* GetContext();

  static void SetRenderTarget(TextureDX* texture);
	static void SetRenderTargetBackBuffer();

	static void SetDrawMode2d();
	static void SetDrawMode3d();
  static void SetProjection(const DirectX::XMMATRIX& matProj);
  static void SetView(const DirectX::XMMATRIX& matView);

  static void GetProjection(DirectX::XMMATRIX& matProj);
	static IUnknown* CreateD3DXTextMesh(const char* pString, bool bCentered);
	static void DrawMesh(IUnknown* pMesh);


	static void Clear( DWORD Col );
	static void ClearDepth();
	static void ClearFloat(float r, float g, float b);
	static void Rect( float x1, float y1, float x2, float y2, DWORD Col );
	static void Line( float x1, float y1, float x2, float y2, DWORD Col );
	static void Point( float x, float y, DWORD Col );

	static TextureDX* CreateTexture( int iWidth, int iHeight, DXGI_FORMAT Format = DXGI_FORMAT_B8G8R8A8_UNORM, bool bDynamic = FALSE );
  static TextureDX* LoadTexture(char* pFilename, bool bAutoResize = true);
	static void DrawText( float x, float y, char* txt, DWORD Col = 0xffffffff);

	static float GetViewportWidth();
	static float GetViewportHeight();
	static void SetBlendMode(eBlendMode blendMode);
	static float GetAspect();
	static void SetAspect(float aspect);

	static void SetTexture(TextureDX* texture);
  static void SetEnvTexture(TextureDX* texture);

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

	static void Circle(float radius, int numSides, DirectX::XMFLOAT3& dir);
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

  static void DrawPrimitive(unsigned int primType, unsigned int numPrims, unsigned int startIndex);
	static void SetLineWidth(float width);
	static void SetFillMode(int fillMode);
	
	static void CommitTransforms( class Shader* pShader );
	static void CommitTextureState();

	static void CreatePixelShader( const void* pCode, unsigned int iCodeLen, ID3D11PixelShader** ppPixelShader );
	static void CreateVertexShader( const void* pCode, unsigned int iCodeLen, ID3D11VertexShader** ppVertexShader );

  static void CreateRenderTarget(ID3D11Texture2D* pTexture, ID3D11RenderTargetView** ppRTView);
  static void CreateShaderView(ID3D11Texture2D* pTexture, ID3D11ShaderResourceView** ppSRView);

  static DirectX::CommonStates* GetStates();
  static void UpdateVBuffer(PosColNormalUVVertex* verticies, unsigned int iNum);

  static void SetShader( Shader* pShader );
  static void SetShaderTexture(unsigned int iStartSlot, TextureDX* pTextures);

private:
	static void CreateCubeBuffers();
	static void CreateFonts();
	static void CompileShaders();
};

#endif
