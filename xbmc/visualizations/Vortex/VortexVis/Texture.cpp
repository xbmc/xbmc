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

#include "Texture.h"
#include "Renderer.h"
#include <new>
#include <stdio.h>

void Texture_c::TextureConstruct(Texture_c *o)
{
	new(o) Texture_c();
}


Texture_c::Texture_c()
{
	m_refCount = 1;
	m_texture = NULL;
	m_renderTarget = false;

} // Constructor

Texture_c::~Texture_c()
{
	if (!m_renderTarget)
	{
		Renderer_c::ReleaseTexture(m_texture);
	}
	else if (m_texture != NULL)
	{
		m_texture->Release();
		m_texture = NULL;
	}

} // Destructor

//--------------------
// reference counting
//--------------------

void Texture_c::AddRef()
{
	m_refCount++;
}

void Texture_c::Release()
{
	if (--m_refCount == 0)
		delete this;
}

Texture_c &Texture_c::operator=(const Texture_c &other)
{
	m_texture  = other.m_texture;
	m_renderTarget = other.m_renderTarget;

	// Return a reference to this object
	return *this;
}


void Texture_c::CreateTexture()
{
	m_texture = Renderer_c::CreateTexture(512, 512);
	m_renderTarget = true;

} // CreateTexture

void Texture_c::LoadTexture(char* &filename)
{
	char fullname[256];
#ifdef STAND_ALONE
	sprintf(fullname, "d:\\Textures\\%s", filename);
#else
	sprintf(fullname, "q:\\Visualisations\\Vortex\\Textures\\%s", filename);
#endif
	m_texture = Renderer_c::LoadTexture(fullname);

} // LoadTexture
