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

#include "../Renderer.h"

#include "Tunnel.h"

#define MAX_SIDES (20)
#define NUM_RINGS (64)
#define SEGMENT_LENGTH (32)

extern float g_bass;
extern float g_middle;
extern float g_treble;

float xMul1 = 8.0f;
float xMul2 = 4.0f;
float yMul1 = 5.0f;
float yMul2 = 9.0f;

float tm = 0;
float ofs = 0;
float angle = 0;

float Tiltfunc(float p)
{
	return (sin((ofs+p)*0.1)+cos((ofs+p)*0.123))*2.0;
}

float yTiltfunc(float p)
{
	return (sin((ofs+p)*0.1)+cos((ofs+p)*0.08))*1.5;
}

//-- Constructor --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
Tunnel_c::Tunnel_c()
{
	tm = 0;
	ofs = 0;
	angle = 0;

	m_doneInit = false;
	m_vBuffer = NULL;
	m_iBuffer = NULL;
	Init();

} // Constructor

//-- Constructor --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
Tunnel_c::~Tunnel_c()
{

	if (m_iBuffer)
		m_iBuffer->Release();

	if (m_vBuffer)
		m_vBuffer->Release();

} // Destructor


//-- TunnelFunction -----------------------------------------------------------
//
//-----------------------------------------------------------------------------
void TunnelFunction(float z, float&x, float& y)
{
	//  x = sinf(z);
	//  y = 1.0f;
	x = sinf(cosf(z * xMul1) * xMul2);
	y = sinf(cosf(z * yMul1) * yMul2);
	//  x = sinf(cosf(z * 1) * 1) * 5;
	//  y = sinf(cosf(z * 1) * 1) * 5;

	//  x = cosf(2 * 3.14159265f * z) * 5;// / 40.0f);
	//  x = sinf(4.0f * 3.14159265f * z / 40.0f) + cosf (2 * 3.14159265f * z / 40.0f);
	//  y = sinf(2 * 3.14159265f * z) * 5;// / 30.0f);

	//  x = sinf(z * 8 * 3.1459f) * 5.0f;
	//  y = cosf(z * 8 * 3.1459f) * 5.0f;
}
//-- CreateSegment ------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CreateSegment(float startZ, int numSides, float radius, int numSegmentRings, float zLength, Vertex_t* v, int col, float uRepeat, float vRepeat)
{
	float currAngle = 0;
	float ringLength = 32.0f / numSegmentRings;
	float angle;
	float x, y;

	float z = 0;

	for (int i = 0; i <= numSegmentRings; i++)
	{
		currAngle = 0;
		//    TunnelFunction((startZ + z) / 100.0f, x, y);
		float x = Tiltfunc(z);
		float y = yTiltfunc(z);

		for (int j = 0; j <= numSides; j++)
		{
			angle = D3DXToRadian(currAngle);
			v->coord.x = sinf(angle) * radius + x;
			v->coord.y = cosf(angle) * radius + y;
			//      v->coord.z = startZ;
			v->coord.z = z;
			int c = (1-(i / (float)numSegmentRings)) * 255;
			v->colour = (c << 24) | (c << 16) | (c << 8) | (c << 0) ;
			v->u = (1.0f - (j / (float(numSides))))*uRepeat;
			//      v->v = ((1.0f - ((i + ofs) / (float(numSegmentRings)))) * 10.0f);
			v->v = (1.0f - (((i+0) / (float(numSegmentRings))) + ofs / zLength)) * vRepeat;
			v++;
			currAngle += 360.0f / numSides;

		}

		z += ringLength;
		//    startZ += ringLength;
	}

} // CreateSegment

//-- CreateIndexBuffer --------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CreateIndexBuffer(int numSides, int numRings, short* indices)
{
	int index = 0;

	numSides++;

	for (int i = 0; i < numRings; i++)
	{
		for (int j = 0; j < numSides; j++)
		{
			indices[index++] = i * numSides + j;
			indices[index++] = (i+1) * numSides + j;
		}

		if (i < numRings - 2)
		{
			indices[index++] = (i+1) * numSides + numSides - 1;
			indices[index++] = (i+1) * numSides + 0;
		}

	}

} // CreateIndexBuffer

//-- Init ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Tunnel_c::Init()
{
	D3DDevice* device = Renderer_c::GetDevice();
	Vertex_t* v;

	device->CreateVertexBuffer((MAX_SIDES+1) * (NUM_RINGS + 1) * sizeof(Vertex_t), 0, 0, 0, &m_vBuffer);
	device->CreateIndexBuffer(2 * ((((MAX_SIDES * 2) + 2) * (NUM_RINGS+1)) + ((NUM_RINGS - 1) * 2)), 0, D3DFMT_INDEX16, 0, &m_iBuffer);

	m_doneInit = true;

} // Init

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Tunnel_c::Render()
{


} // Render


//-- RenderLayer --------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Tunnel_c::RenderLayer(int numSides, float radius, float uRepeat, float vRepeat)
{
	//  static int mySides = 20;

	int mySides = max(3, min(20, numSides));
	float myRadius = radius;

	//   angle += (g_treble) * 0.05f;

	Vertex_t*  v;

	m_vBuffer->Lock(0, 0, (BYTE**)&v, 0);
	CreateSegment(tm, mySides, myRadius, NUM_RINGS, SEGMENT_LENGTH, v, 0xffffffff, uRepeat, vRepeat);

	short* indices;
	m_iBuffer->Lock(0, 0, (BYTE**)&indices, 0);
	CreateIndexBuffer(mySides, NUM_RINGS+1, indices);

	D3DDevice* device = Renderer_c::GetDevice();

	d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	float x = Tiltfunc(tm);
	float x2 = Tiltfunc(tm+0.5f/*+sin(angle)*/);

	float y = yTiltfunc(tm);
	float y2 = yTiltfunc(tm+0.5f);

	D3DXVECTOR3 vEyePt    = D3DXVECTOR3(x, y, tm);
	D3DXVECTOR3 vLookatPt = D3DXVECTOR3(x2, y2, tm+0.5);
	D3DXVECTOR3 vUp       = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

	D3DXMATRIX  rotY;

	D3DXMatrixRotationYawPitchRoll(&rotY, 0, 0, angle);
	D3DXVECTOR3 result;

	result.x = rotY._11 * vUp.x + rotY._12 * vUp.y + rotY._13 * vUp.z;
	result.y = rotY._21 * vUp.x + rotY._22 * vUp.y + rotY._23 * vUp.z;
	result.z = rotY._31 * vUp.x + rotY._32 * vUp.y + rotY._33 * vUp.z;

	D3DXMatrixLookAtLH(&m_matView, &vEyePt, &vLookatPt, &result);


	Renderer_c::SetView(m_matView);


	Renderer_c::CommitStates();

	device->SetStreamSource(0, m_vBuffer, sizeof(Vertex_t));
	device->SetIndices(m_iBuffer, 0);
	d3dDrawIndexedPrimitive(D3DPT_QUADSTRIP, 0, 0, 0, ((((mySides * 2) + 2) * (NUM_RINGS)) + ((NUM_RINGS - 1) * 2) ) / 2-1);


} // RenderLayer

//-- Update -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Tunnel_c::Update(float speed, float rot)
{
	//  tm += 1 (1/50f * ;
	// tm = tm + (fabs(g_bass)) * (1/50.0f) * 4;
	tm += speed;
	angle += rot;

	while (tm>1)
	{
		tm = tm-1;
		ofs = ofs + 1;
	}

	while (tm<1)
	{
		tm = tm+1;
		ofs = ofs - 1;
	}


} // Update
