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

#ifndef VOICEPRINT_H
#define VOICEPRINT_H

#include <new.h>
#include "..\Renderer.h"

class VoicePrint_c
{
public:
	VoicePrint_c();
	~VoicePrint_c();
	void Init();
	void Render();
	void LoadColourMap(char* &filename);
	void SetRect(float minX, float minY, float maxX, float maxY);
	void SetSpeed(float speed);

	LPDIRECT3DTEXTURE8 GetTexture() {return m_texture;}

	static void Construct(VoicePrint_c* o)
	{
		new(o) VoicePrint_c();
	}

	static void Destruct(VoicePrint_c* o)
	{
		o->~VoicePrint_c();
	}


private:
	LPDIRECT3DTEXTURE8  m_texture;
	LPDIRECT3DTEXTURE8  m_colourMap;
	LPDIRECT3DTEXTURE8  m_spectrumTexture;
	float               m_speed;
	float               m_minX;
	float               m_maxX;
	float               m_minY;
	float               m_maxY;

};


#endif