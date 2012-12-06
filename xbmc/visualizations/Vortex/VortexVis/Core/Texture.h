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

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "Renderer.h"
#include <string>

using namespace std;

class Texture
{
public:
	Texture();
	void CreateTexture();
	void LoadTexture( string& filename );

	void AddRef();
	void Release();

	LPDIRECT3DTEXTURE9 m_pTexture;
	bool m_renderTarget;

protected:
	~Texture();
	int m_iRefCount;
};

inline Texture* Texture_Factory()
{
	// The class constructor is initializing the reference counter to 1
	return new Texture();
}

#endif
