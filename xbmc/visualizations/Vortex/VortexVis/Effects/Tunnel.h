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

#ifndef TUNNEL_H
#define TUNNEL_H

#include "../renderer.h"
#include <new.h>

#define NUM_SEGMENTS (3)
#define NUM_LAYERS (4)

class Tunnel_c
{
public:
	Tunnel_c();
	~Tunnel_c();
	void Init();
	void Render();
	void RenderLayer(int numSides, float radius, float uRepeat, float vRepeat);
	void Update(float speed, float rot);

	static void TunnelConstruct(Tunnel_c *o)
	{
		new(o) Tunnel_c();
	}

	static void TunnelDestruct(Tunnel_c *o)
	{
		o->~Tunnel_c();
	}


private:
	bool  m_doneInit;
	float m_camPos;
	float m_camSpeed;
	int m_startSegment;
	int m_lastSegment;
	int m_segmentCount;
	D3DXMATRIX m_matView;
	D3DCOLOR m_colours[NUM_LAYERS];
	int m_numSides[NUM_LAYERS];
	float m_radius[NUM_LAYERS];
	D3DVertexBuffer*  m_vBuffer;
	D3DIndexBuffer* m_iBuffer;
};


#endif