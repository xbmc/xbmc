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

#ifndef MAP_H
#define MAP_H

#include <new.h>
#include "..\Renderer.h"

class Map_c
{
public:
	Map_c();
	~Map_c();
	void Init();
	void Render();
	void SetValues(int x, int y, float uOffset, float vOffset, float r, float g, float b);
	void SetTimed();
	LPDIRECT3DTEXTURE8 GetTexture() {return m_texture;}

	static void MapConstruct(Map_c *o)
	{
		new(o) Map_c();
	}

	static void MapDestruct(Map_c *o)
	{
		o->~Map_c();
	}

	void AddRef();
	void Release();


private:
	LPDIRECT3DTEXTURE8  m_texture;
	unsigned short* m_indices;
	Vertex_t* m_vertices;
	int m_numIndices;
	bool m_timed;
};


#endif