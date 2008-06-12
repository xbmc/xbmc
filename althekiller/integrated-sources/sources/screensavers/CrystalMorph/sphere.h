// $Id: sphere.h,v 1.4 2003/10/14 21:13:28 toolshed Exp $

#pragma once
#include <xtl.h>
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

/**
 * For rendering 3D sphere
 *
 * This class is used for rendering a 3d sphere centered at the origin.
 * It demonstrates how to overide the Render3D function.
 * However, it still create command list so you can still manipulate the primitive.
 */
class C_Sphere {

public:

	C_Sphere();			 ///< Constructor
	virtual ~C_Sphere(); ///< Destructor

	/**
	 * Update the sphere primitive.
	 *
	 * If the updateMemory is true, it will create the vertex buffer and index buffer
	 * based on the change in quality.
	 * If the updateVertices is true, it will change the vertex buffer to reflect
	 * the change in texture scaling and translation.
	 */

	virtual void Update();
	/**
	 * For rendering the sphere to the given world matrix
	 * @param p_WorldMatrix The pointer to world matrix that this primitive will be rendered on
	 *
	 * Overide the default Render3D function of C_Primitive, for demonstration purpose
	 */
	virtual void Render3D();

  void SetColor(DWORD color);

private:
	unsigned int numIndices;
	unsigned int numVertices;		///< Number of slices(rings) along y axis
	unsigned int numSlices;		///< Number of slices(rings) along y axis
	unsigned int numSegments;	///< Number of segments in each slice
	unsigned int numTriangles;  ///< Number of triangles used
};