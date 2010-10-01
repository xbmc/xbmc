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

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "Renderer.h"

class Texture_c
{
public:
	Texture_c();
	//  ~Texture_c();
	void CreateTexture();
	void LoadTexture(char* &filename);

	void AddRef();
	void Release();

	LPDIRECT3DTEXTURE8 m_texture;
	bool m_renderTarget;

	Texture_c &operator=(const Texture_c &other);

	static void TextureConstruct(Texture_c *o);


protected:
	~Texture_c();
	int m_refCount;

};

#endif