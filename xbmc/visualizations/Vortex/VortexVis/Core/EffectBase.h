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

#ifndef _EFFECT_H_
#define _EFFECT_H_

struct IDirect3DTexture9;

class EffectBase
{
public:
	static void RegisterScriptInterface( class asIScriptEngine* );
	EffectBase()
	{
		m_iRefCount = 1;
	}

	virtual ~EffectBase() {};

	void AddRef()
	{
		m_iRefCount++;
	}

	void Release()
	{
		if ( --m_iRefCount == 0 )
			delete this;
	}

	virtual  IDirect3DTexture9* GetTexture() { return 0; }
	virtual  IDirect3DTexture9* GetRenderTarget() { return 0; }

protected:
	int m_iRefCount;
};

template<class A, class B>
B* refCast(A* a)
{
	// If the handle already is a null handle, then just return the null handle
	if( !a ) return 0;

	// Now try to dynamically cast the pointer to the wanted type
	B* b = dynamic_cast<B*>(a);
	if( b != 0 )
	{
		// Since the cast was made, we need to increase the ref counter for the returned handle
		b->AddRef();
	}
	return b;
}

#endif
