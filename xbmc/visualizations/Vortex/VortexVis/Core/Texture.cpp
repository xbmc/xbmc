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

#include "Texture.h"
#include "Renderer.h"
#include <new>
#include <stdio.h>

using namespace std;

Texture::Texture()
{
	m_iRefCount = 1;
	m_pTexture = NULL;
	m_renderTarget = false;

} // Constructor

Texture::~Texture()
{
  if (!m_renderTarget)
  {
	  Renderer::ReleaseTexture(m_pTexture);
  }
  else if (m_pTexture != NULL)
	{
		m_pTexture->Release();
		m_pTexture = NULL;
	}

} // Destructor

//--------------------
// reference counting
//--------------------

void Texture::AddRef()
{
	m_iRefCount++;
}

void Texture::Release()
{
	if ( --m_iRefCount == 0 )
		delete this;
}

void Texture::CreateTexture()
{
	m_pTexture = Renderer::CreateTexture(512, 512);
	m_renderTarget = true;

} // CreateTexture

void Texture::LoadTexture(string& filename)
{
	char fullname[ 512 ];
	sprintf_s(fullname, 512, "%s%s", g_TexturePath, filename.c_str() );
	m_pTexture = Renderer::LoadTexture(fullname);

} // LoadTexture
