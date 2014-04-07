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

#ifndef _VOICEPRINT_H_
#define _VOICEPRINT_H_

#include <new.h>
#include "Renderer.h"
#include "EffectBase.h"
#include <string>

class VoicePrint : public EffectBase
{
public:
	static void RegisterScriptInterface( class asIScriptEngine* );
	VoicePrint();
	~VoicePrint();
	void Init();
	void Render();
	void LoadColourMap( std::string& filename );
	void SetRect(float minX, float minY, float maxX, float maxY);
	void SetSpeed(float speed);

	IDirect3DTexture9* GetTexture() { return m_texture; }

private:
	int					m_iCurrentTexture;
	LPDIRECT3DTEXTURE9  m_texture;
	LPDIRECT3DTEXTURE9  m_tex1;
	LPDIRECT3DTEXTURE9	m_tex2;
	LPDIRECT3DTEXTURE9  m_colourMap;
	LPDIRECT3DTEXTURE9  m_spectrumTexture;
	float               m_speed;
	float               m_minX;
	float               m_maxX;
	float               m_minY;
	float               m_maxY;
};

#endif
