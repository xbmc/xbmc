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

#ifndef RENDERER_H
#define RENDERER_H

#include <xtl.h>

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
extern "C" void d3dDrawIndexedPrimitive(D3DPRIMITIVETYPE primType, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primCount);


enum VShaderType_e {VSHADER_SIMPLE = 0,
VSHADER_SPHEREMAP = 1,
VSHADER_SIMPLETEX0 = 2,
VSHADER_METABALL = 3,
MAX_VSHADERS};

enum PShaderType_e {PSHADER_COLREMAP = 0,
MAX_PSHADERS};


enum BlendMode_e {BLEND_OFF = 0,
BLEND_ADD = 1,
BLEND_MOD = 2,
BLEND_MAX = 3};

#define SCRATCH_TEXTURE ((LPDIRECT3DTEXTURE8)-1)

struct Vertex_t
{
	D3DVECTOR coord;
	unsigned int colour;
	D3DVECTOR normal;
	float u, v;
};

class Renderer_c
{
public:
	static void Init(LPDIRECT3DDEVICE8 device, int xPos, int yPos, int width, int height, float pixelRatio);
	static void Exit();
	static D3DDevice* GetDevice();

	static LPDIRECT3DTEXTURE8 CreateTexture(int width, int height, D3DFORMAT = D3DFMT_X8R8G8B8);
	static LPDIRECT3DTEXTURE8 LoadTexture(char* filename);
	static void ReleaseTexture(LPDIRECT3DTEXTURE8 texture);
	static void SetRenderTarget(LPDIRECT3DTEXTURE8 texture);
	static void SetRenderTargetBackBuffer();
	static void SetTexture(LPDIRECT3DTEXTURE8 texture);
	static void SetEnvTexture(LPDIRECT3DTEXTURE8 texture);
	static void SetVShader(VShaderType_e shaderType);
	static void SetPShader(PShaderType_e shaderType);
	static void CopyToScratch();
	static void SetBlendMode(BlendMode_e blendMode);

	static void SetDefaults();
	static void Clear(unsigned int col);
	static void ClearFloat(float r, float g, float b);
	static void SetProjection(D3DXMATRIX& matProj);
	static void SetView(D3DXMATRIX& matView);
	static float GetAspect();
	static void SetAspect(float aspect);

	static void CommitStates();

	static void SetDrawMode2d();
	static void SetDrawMode3d();

	// Gl stuff
	static void Begin(D3DPRIMITIVETYPE primType);
	static void End();

	static void Colour(float r, float g, float b, float a);
	static void TexCoord(float u, float v);
	static void Normal(float nx, float ny, float nz);
	static void NormalVec(D3DVECTOR& normal);
	static void Vertex(float x, float y, float z);
	static void VertexVec(D3DVECTOR& pos);

	static void Cube(float nx, float ny, float nz, float x, float y, float z);
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
	static void SetFillMode(int filLMode);

private:

};
#endif