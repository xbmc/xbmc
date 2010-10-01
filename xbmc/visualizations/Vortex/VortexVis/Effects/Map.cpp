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

#include "map.h"

#define GRID_WIDTH (32)
#define GRID_HEIGHT (24)

#define NUM_INDICES (1584)

extern float g_bass;
extern float g_middle;
extern float g_treble;

extern float Rand();


Map_c::Map_c()
{
	m_timed = false;
	m_texture = Renderer_c::CreateTexture(512, 512);

	m_vertices = new Vertex_t[GRID_WIDTH * GRID_HEIGHT];

	Vertex_t* v = m_vertices;

	for (int y = 0; y < GRID_HEIGHT; y++)
	{
		for (int x = 0; x < GRID_WIDTH; x ++)
		{
			//      0-(32 / 2) / (32 / 2) = -1
			//      15-(32 / 2) / (32 / 2) = -1
			v->coord.x = ((float)x - ((GRID_WIDTH -1 ) / 2.0f)) / ((GRID_WIDTH -1 ) / 2.0f);
			v->coord.y = -(((float)y - ((GRID_HEIGHT -1 ) / 2.0f)) / ((GRID_HEIGHT -1 ) / 2.0f));
			v->colour = 0xffffffff;
			v->coord.z = 0.0f;
			v->u = ((float)x / (GRID_WIDTH -1)) + (0.5f / 512);
			v->v = ((float)y / (GRID_HEIGHT -1)) + (0.5f / 512) ;//+ (5 / 512.0f);
			v++;
		}
	}


	//---------------------------------------------------------------------------
	// Build indices
	m_indices = new unsigned short[NUM_INDICES];
	unsigned short* pIndices = m_indices;
	int numIndices = 0;
	int iIndex = 0;

	for (int a = 0; a < GRID_HEIGHT; ++a)
	{
		for (int i = 0; i < GRID_WIDTH; ++i, pIndices += 2, ++iIndex )
		{
			pIndices[ 0 ] = iIndex + GRID_WIDTH;
			pIndices[ 1 ] = iIndex;
			numIndices+=2;
		}

		// connect two strips by inserting two degenerate triangles 
		if (GRID_WIDTH - 2 > a)
		{
			pIndices[0] = iIndex - 1;
			pIndices[1] = iIndex + GRID_WIDTH;

			pIndices += 2;
			numIndices += 2;
		}
	}

	m_numIndices = numIndices - 2;


}

Map_c::~Map_c()
{

	delete[] m_vertices;
	delete[] m_indices;
	m_texture->Release();
}

__inline float MapCol(float a)
{
	//	static float speed = 1.02f;
	float res = powf(1.02f, a);
	if (res < 0)
		res = 0;
	else if (res > 1)
		res = 1.0f;
	return res;
}

__inline float Clamp(float a)
{
	if (a < 0)
		a = 0;
	else if (a > 1)
		a = 1.0f;
	return a;
}

void Map_c::SetTimed()
{
	m_timed = true;
}

void Map_c::SetValues(int x, int y, float uOffset, float vOffset, float r, float g, float b)
{
	int index = (y * GRID_WIDTH) + x;
	Vertex_t* v = &m_vertices[index];

	if (m_timed)
	{
		v->u = ((float)x / (GRID_WIDTH -1)) + (0.5f / 512)  + ((uOffset) / (GRID_WIDTH -1));
		v->v = ((float)y / (GRID_HEIGHT -1)) + (0.5f / 512) + ((vOffset) / (GRID_HEIGHT -1));
		unsigned char* col = (unsigned char*)&v->colour;
		col[3] = 0xff;
		col[2] = int(MapCol(r) * 255);
		col[1] = int(MapCol(g) * 255);
		col[0] = int(MapCol(b) * 255);
	}
	else
	{
		v->u = ((float)x / (GRID_WIDTH -1)) + (0.5f / 512)  + ((uOffset));
		v->v = ((float)y / (GRID_HEIGHT -1)) + (0.5f / 512) + ((vOffset));
		unsigned char* col = (unsigned char*)&v->colour;
		col[3] = 0xff;
		col[2] = int(Clamp(r) * 255);
		col[1] = int(Clamp(g) * 255);
		col[0] = int(Clamp(b) * 255);
	}
}

void Map_c::Render()
{
	Renderer_c::SetRenderTarget(m_texture);
	Renderer_c::CopyToScratch();

	Renderer_c::PushMatrix();
	Renderer_c::SetIdentity();
	float oldAspect = Renderer_c::GetAspect();
	Renderer_c::SetAspect(0);
	Renderer_c::Translate(0, 0, 2.414f);
	Renderer_c::SetTexture(SCRATCH_TEXTURE);
	Renderer_c::CommitStates();
	Renderer_c::GetDevice()->DrawIndexedPrimitiveUP(D3DPT_TRIANGLESTRIP, 0, 0, 1514, m_indices, D3DFMT_INDEX16, m_vertices, sizeof(Vertex_t));
	Renderer_c::SetTexture(NULL);
	Renderer_c::PopMatrix();
	Renderer_c::SetAspect(oldAspect);
	Renderer_c::CommitStates();

} // Render