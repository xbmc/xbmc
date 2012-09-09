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

#include "Mesh.h"
#include <new>
#include <stdio.h>

Mesh::Mesh()
{
	m_iRefCount = 1;
	m_pMesh = NULL;
}

Mesh::~Mesh()
{
	if ( m_pMesh )
	{
		m_pMesh->Release();
		m_pMesh = NULL;
	}
}

//--------------------
// reference counting
//--------------------

void Mesh::AddRef()
{
	m_iRefCount++;
}

void Mesh::Release()
{
	if ( --m_iRefCount == 0 )
		delete this;
}

void Mesh::CreateTextMesh( string& InString, bool bCentered )
{
	if ( m_pMesh )
	{
		m_pMesh->Release();
	}

	m_pMesh = Renderer::CreateD3DXTextMesh( InString.c_str(), bCentered );
}
