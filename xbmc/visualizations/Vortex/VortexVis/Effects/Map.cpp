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

#include "map.h"
#include "angelscript.h"

#include "Shader.h"

#define GRID_WIDTH (32)
#define GRID_HEIGHT (24)

#define NUM_INDICES (1584)

#define TEX_WIDTH ( 1024 )
#define TEX_HEIGHT ( 512 )

Map::Map()
{
	m_iCurrentTexture = 0;
	m_timed = false;
	m_tex1 = Renderer::CreateTexture(TEX_WIDTH, TEX_HEIGHT);
	m_tex2 = Renderer::CreateTexture(TEX_WIDTH, TEX_HEIGHT);

	m_texture = m_tex1;

	m_vertices = NULL;

	Renderer::GetDevice()->CreateIndexBuffer( NUM_INDICES * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_iBuffer, NULL );
	Renderer::GetDevice()->CreateVertexBuffer( GRID_WIDTH * GRID_HEIGHT * sizeof( PosColNormalUVVertex ),
											   D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT,
											   &m_vBuffer, NULL );

	PosColNormalUVVertex* v;
	m_vBuffer->Lock( 0, 0, (void**)&v, 0 );

	for (int y = 0; y < GRID_HEIGHT; y++)
	{
		for (int x = 0; x < GRID_WIDTH; x ++)
		{
			//      0-(32 / 2) / (32 / 2) = -1
			//      15-(32 / 2) / (32 / 2) = -1
			v->Coord.x = ((float)x - ((GRID_WIDTH -1 ) / 2.0f)) / ((GRID_WIDTH -1 ) / 2.0f);
			v->Coord.y = -(((float)y - ((GRID_HEIGHT -1 ) / 2.0f)) / ((GRID_HEIGHT -1 ) / 2.0f));
//			v->Diffuse= 0xffffffff;
			v->FDiffuse.x = 1.0f;
			v->FDiffuse.y = 1.0f;
			v->FDiffuse.z = 1.0f;
			v->FDiffuse.w = 1.0f;
			//			v->coord. = 1.0f;
			v->Coord.z = 0.0f;
			//			v->u = ((((float)x - ((GRID_WIDTH -1 ) / 2.0f)) / ((GRID_WIDTH -1 ) / 2.0f) * 256 + 256) * (1.0f / 512)) + (0.5f / 512);
			//			v->v = ((((float)y - ((GRID_HEIGHT -1 ) / 2.0f)) / ((GRID_HEIGHT -1 ) / 2.0f) * 256 + 256) * (1.0f / 512)) + (0.5f / 512);
			v->u = ((float)x / (GRID_WIDTH -1));// + (0.5f / TEX_SIZE);
			v->v = ((float)y / (GRID_HEIGHT -1));// + (0.5f / TEX_SIZE) ;//+ (5 / 512.0f);
			v++;
		}
	}
	m_vBuffer->Unlock();


	//---------------------------------------------------------------------------
	// Build indices
	unsigned short* pIndices;
	m_iBuffer->Lock( 0, 0, (void**)&pIndices, 0 );
	int numIndices = 0;
	unsigned short iIndex = 0;

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

	m_iBuffer->Unlock();
}

Map::~Map()
{
	if ( m_vertices != NULL )
	{
		m_vBuffer->Unlock();
		m_vertices = NULL;
	}
	m_vBuffer->Release();
	m_iBuffer->Release();
	m_tex1->Release();
	m_tex2->Release();
}

__inline float MapCol( float a )
{
	//	static float speed = 1.02f;
	float res = powf(1.02f, a);
	if (res < 0)
		res = 0;
	else if (res > 1)
		res = 1.0f;
	return res;
}

__inline float Clamp( float a )
{
	if (a < 0)
		a = 0;
	else if (a > 1)
		a = 1.0f;
	return a;
}

void Map::SetTimed()
{
	m_timed = true;
}

void Map::SetValues(unsigned int x, unsigned int y, float uOffset, float vOffset, float r, float g, float b)
{
	int index = ((y )* GRID_WIDTH) + x;
	if( index >= GRID_HEIGHT * GRID_WIDTH )
	{
		return;
	}

	if( m_vertices == NULL )
	{
		m_vBuffer->Lock( 0, 0, (void**)&m_vertices, 0 );
	}

	PosColNormalUVVertex* v = &m_vertices[ index ];

	static const float invWidth = 1.0f / ( GRID_WIDTH - 1 );
	static const float invHeight = 1.0f / ( GRID_HEIGHT - 1 );

	if( m_timed )
	{
		v->u = ((float)x * invWidth ) + (0.5f / TEX_WIDTH)  + ( ( uOffset ) * invWidth );
		v->v = ((float)y * invHeight ) + (0.5f / TEX_HEIGHT) + ( ( vOffset ) * invHeight );
/*		unsigned char* col = (unsigned char*)&v->Diffuse;
		col[3] = 0xff;
		col[2] = int(MapCol(r) * 255);
		col[1] = int(MapCol(g) * 255);
		col[0] = int(MapCol(b) * 255);
*/
		v->FDiffuse.x = MapCol(r);
		v->FDiffuse.y = MapCol(g);
		v->FDiffuse.z = MapCol(b);
		v->FDiffuse.w = 1.0f;
	}
	else
	{
		v->u = ((float)x * invWidth ) + (0.5f / TEX_WIDTH)  + ((uOffset));
		v->v = ((float)y * invHeight ) + (0.5f / TEX_HEIGHT) + ((vOffset));
//		unsigned char* col = (unsigned char*)&v->Diffuse;
// 		col[3] = 0xff;
// 		col[2] = int(Clamp(r) * 255);
// 		col[1] = int(Clamp(g) * 255);
// 		col[0] = int(Clamp(b) * 255);
		v->FDiffuse.x = r;
		v->FDiffuse.y = g;
		v->FDiffuse.z = b;
		v->FDiffuse.w = 1;
	}
}

void Map::Render()
{
	if ( m_vertices != NULL )
	{
		m_vBuffer->Unlock();
		m_vertices = NULL;
	}

	if ( m_iCurrentTexture == 0 )
	{
		Renderer::SetRenderTarget(m_tex1);
		Renderer::SetTexture(m_tex2);
		m_texture = m_tex1;
	}
	else
	{
		Renderer::SetRenderTarget(m_tex2);
		Renderer::SetTexture(m_tex1);
		m_texture = m_tex2;
	}

	Renderer::PushMatrix();
	Renderer::SetIdentity();
	float oldAspect = Renderer::GetAspect();
	Renderer::SetAspect(0);
	Renderer::Translate(0, 0, 2.414f);

	DiffuseUVVertexShader* pShader = &DiffuseUVVertexShader::StaticType;
	Renderer::CommitTransforms( pShader );
	Renderer::CommitTextureState();
	Renderer::GetDevice()->SetVertexShader( pShader->GetVertexShader() );
	Renderer::GetDevice()->SetVertexDeclaration( g_pPosColUVDeclaration );

	Renderer::GetDevice()->SetStreamSource( 0,
											m_vBuffer,
											0,
											sizeof( PosColNormalUVVertex ) );

	Renderer::GetDevice()->SetIndices( m_iBuffer );

	Renderer::GetDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLESTRIP, 0, 0, GRID_WIDTH * GRID_HEIGHT, 0, 1514 );

	Renderer::GetDevice()->SetStreamSource( 0,
											NULL,
											0,
											0 );

	Renderer::GetDevice()->SetIndices( NULL );

	Renderer::SetTexture(NULL);
	Renderer::PopMatrix();
	Renderer::SetAspect(oldAspect);

	m_iCurrentTexture = 1 - m_iCurrentTexture;

} // Render

//-----------------------------------------------------------------------------
// Script interface

Map* Map_Factory()
{
	/* The class constructor is initializing the reference counter to 1*/
	return new Map();
}

void Map::RegisterScriptInterface( asIScriptEngine* pScriptEngine )
{
#ifndef assert
#define assert
#endif
	int r;

	// Registering the reference type
	r = pScriptEngine->RegisterObjectType("Map", 0, asOBJ_REF);	 assert(r >= 0);

	// Registering the factory behaviour
	r = pScriptEngine->RegisterObjectBehaviour("Map", asBEHAVE_FACTORY, "Map@ f()", asFUNCTION(Map_Factory), asCALL_CDECL); assert( r >= 0 );

	// Registering the addref/release behaviours
	r = pScriptEngine->RegisterObjectBehaviour("Map", asBEHAVE_ADDREF, "void f()", asMETHOD(Map, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectBehaviour("Map", asBEHAVE_RELEASE, "void f()", asMETHOD(Map, Release), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectMethod("Map", "void Render()", asMETHOD(Map, Render), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectMethod("Map", "void SetValues(int, int, float, float, float, float, float)", asMETHOD(Map, SetValues), asCALL_THISCALL); assert(r >= 0);
	r = pScriptEngine->RegisterObjectMethod("Map", "void SetTimed()", asMETHOD(Map, SetTimed), asCALL_THISCALL); assert(r >= 0);

	r = pScriptEngine->RegisterObjectBehaviour("Map", asBEHAVE_IMPLICIT_REF_CAST, "EffectBase@ f()", asFUNCTION((refCast<Map,EffectBase>)), asCALL_CDECL_OBJLAST); assert( r >= 0 );
}
