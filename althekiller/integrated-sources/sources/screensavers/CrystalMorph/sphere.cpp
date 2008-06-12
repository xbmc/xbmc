 	

// $Id: sphere.cpp,v 1.9 2003/10/14 21:13:28 toolshed Exp $

/* dxframework - "The Engine Engine" - DirectX Game Framework
 * Copyright (C) 2003  Corey Johnson, Jonathan Voigt, Nuttapong Chentanez
 * Contributions by Adam Tercala, Jeremy Lee, David Yeung, Evan Leung
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */


#include "sphere.h"
#include <d3d8types.h>

extern "C" void d3dDrawIndexedPrimitive(D3DPRIMITIVETYPE primtype, unsigned int minindex, unsigned int numvertices, unsigned int startindex, unsigned int primcount);
extern LPDIRECT3DDEVICE8 g_pd3dDevice;

//#define D3DFVF_SPHEREVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE )
#define D3DFVF_SPHEREVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL ) 

struct SPHEREVERTEX
{
	D3DXVECTOR3 position; // The untransformed position for the vertex.
	D3DXVECTOR3 normal; // Normal vector for lighting calculations	
    //DWORD diffuseColor; // The vertex colour.
};
LPDIRECT3DVERTEXBUFFER8 g_pSphereVertexBuffer = NULL; // Vertices Buffer
LPDIRECT3DINDEXBUFFER8 g_pSphereIndexBuffer = NULL; // Index Buffer
int quality;

C_Sphere::C_Sphere() {
	// Default quality comprised of 9 rings of 10 segments
	quality = 7;
	Update();
}

C_Sphere::~C_Sphere() {
}



void C_Sphere::Update() {
	numSlices = quality + 2;	// Find number of slices
	numSegments = quality + 3;  // Find number of segment in a slice

	numVertices = numSlices * numSegments; // Total number of vertices in vertex buffer
	numTriangles = 2 * (numSlices - 1) * (numSegments - 1);	// Total number of triangle
	numIndices = numTriangles * 3; // Total number of indices in index buffer

	// Create index and vertex buffer, if they exist, recreate them
    g_pd3dDevice->CreateVertexBuffer(numVertices*  sizeof(SPHEREVERTEX),
                                               D3DUSAGE_WRITEONLY, 
											   D3DFVF_SPHEREVERTEX,
                                               D3DPOOL_MANAGED, 
											   &g_pSphereVertexBuffer);
    g_pd3dDevice->CreateIndexBuffer(numIndices * sizeof(short), D3DUSAGE_DYNAMIC,
		D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pSphereIndexBuffer);
	
	short * p_DeviceIBMem;
	g_pSphereIndexBuffer->Lock(0, numIndices * sizeof(short), (BYTE**)&p_DeviceIBMem, 0);


	// Write triangle indicies
	for (unsigned int slice = 0; slice < numSlices - 1; slice++) {
		for (unsigned int segment = 0; segment < numSegments - 1; segment++) {
			unsigned int sliceBase = slice * numSegments;
			unsigned int segIndex = sliceBase + segment;
			unsigned int baseIndex = (slice * numSlices + segment) * 6;
			p_DeviceIBMem[baseIndex] = segIndex;
			p_DeviceIBMem[baseIndex + 1] = segIndex + numSegments;
			p_DeviceIBMem[baseIndex + 2] = sliceBase + segment + 1;
			p_DeviceIBMem[baseIndex + 3] = sliceBase + segment + 1;
			p_DeviceIBMem[baseIndex + 4] = segIndex + numSegments;
			p_DeviceIBMem[baseIndex + 5] = segIndex + numSegments + 1;
		}
	}
	g_pSphereIndexBuffer->Unlock();

	

	int i;
	SPHEREVERTEX * vertexMemory;
	g_pSphereVertexBuffer->Lock(0, numVertices*  sizeof(SPHEREVERTEX), (BYTE**)&vertexMemory, 0);
	

	// Write vertex data
	// Loop trough each slices
	for (unsigned int slice = 0; slice < numSlices; slice++) 
	{
		float heightAngle = ((float)slice / (numSlices - 1)) * D3DX_PI; // Find height angle
		float y = 0.5f * 1.0f * cosf(heightAngle);	// Obtain the y component of the slice
		float radius = (float)fabs(0.5f * sinf(heightAngle)); // Find the radius of the slice

		// Loop trough each segments in the slice
		for (unsigned int segment = 0; segment < numSegments; segment++) 
		{
			float angle = ((float)segment / numSlices) * 2 * D3DX_PI; // Find angle for this segment

			// Obtains x and z component of the vertex
			float x = radius * 1.0f * sinf(angle);
			float z = radius * 1.0f * cosf(angle);

			unsigned int point = slice * numSegments + segment;
            vertexMemory[point].position = D3DXVECTOR3(x, y, z);


			vertexMemory[point].normal = D3DXVECTOR3(x, y, z);
//			D3DXVec3Normalize(&vertexMemory[point].normal, &vertexMemory[point].normal);
		}
		

		// Write vertex diffuse color, by default, we won't use it for rendering
		// however, you can always change!
       /* for (i = 0; i < (int)numVertices; i++) 
		{
			vertexMemory[i].diffuseColor = (D3DXCOLOR)D3DCOLOR_RGBA(200,100,100,255);; // White, completely opaque
		}*/
		
	}
	g_pSphereVertexBuffer->Unlock();
}

void C_Sphere::SetColor(DWORD color)
{
  SPHEREVERTEX * vertexMemory;
  g_pSphereVertexBuffer->Lock(0, numVertices*  sizeof(SPHEREVERTEX), (BYTE**)&vertexMemory, 0);
  /*for (int i = 0; i < (int)numVertices; i++) 
			vertexMemory[i].diffuseColor = color; */
	g_pSphereVertexBuffer->Unlock();
}

void C_Sphere::Render3D() {

	//g_pd3dDevice->SetTransform(D3DTS_WORLD, p_WorldMatrix);
	

	// Setup D3D stuffs
	g_pd3dDevice->SetStreamSource(0, g_pSphereVertexBuffer, sizeof(SPHEREVERTEX));
	g_pd3dDevice->SetVertexShader(D3DFVF_SPHEREVERTEX);
	g_pd3dDevice->SetIndices(g_pSphereIndexBuffer, 0);

	//g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, numVertices, 0, numTriangles);
	d3dDrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, numVertices, 0, numTriangles);
	//g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0, numTriangles);
	
}
