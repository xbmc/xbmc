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

#ifndef _MESH_H_
#define _MESH_H_

#include "Renderer.h"
#include <string>

class Mesh
{
public:
		Mesh();
		void CreateTextMesh( std::string& InString, bool bCentered = true );

		void AddRef();
		void Release();
		LPD3DXMESH GetMesh() { return m_pMesh; };
protected:
		~Mesh();
		int m_iRefCount;

		LPD3DXMESH	m_pMesh;

};

inline Mesh* Mesh_Factory()
{
	// The class constructor is initializing the reference counter to 1
	return new Mesh();
}

#endif
