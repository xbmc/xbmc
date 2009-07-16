/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

/*!
\file TextureManager.h
\brief 
*/

#ifndef GUILIB_TEXTUREMANAGER_SDL_H
#define GUILIB_TEXTUREMANAGER_SDL_H

#include "TextureManager.h"

#pragma once

/************************************************************************/
/*    CSDLTexture                                                       */
/************************************************************************/
class CGLTexture : public CBaseTexture
{
public:

	CGLTexture(void* surface, bool loadToGPU = true, bool freeSurface = false);  
	virtual ~CGLTexture();

	void LoadToGPU();
	void Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU); 
	void Update(void *surface, bool loadToGPU, bool freeSurface);
};


/************************************************************************/
/*    CTextureArraySDL                                                  */
/************************************************************************/
class CTextureArrayGL : public CTextureArray
{
public:
	CTextureArrayGL();
	CTextureArrayGL(int width, int height, int loops, bool texCoordsArePixels = false);
	virtual ~CTextureArrayGL();
	CTextureArrayGL &operator =(const CTextureArray &base)
	{
		CTextureArray* tmp = dynamic_cast<CTextureArray *>(this);
		*tmp = base;
		return *this;
	}
	void Add(void *texture, int delay);
	void Set(void *texture, int width, int height);
	void Free();
};

/*!
\ingroup textures
\brief 
*/

/************************************************************************/
/*    CTextureMapSDL                                                    */
/************************************************************************/
class CTextureMapGL : public CTextureMap
{
public:
	CTextureMapGL(const CStdString& textureName, int width, int height, int loops);
	virtual ~CTextureMapGL();

	void Add(void* pTexture, int delay);
};

/*!
\ingroup textures
\brief 
*/
/************************************************************************/
/*    CGUITextureManagerSDL                                             */
/************************************************************************/
class CGUITextureManagerGL : public CGUITextureManager
{
public:
	CGUITextureManagerGL(void);
	virtual ~CGUITextureManagerGL(void);

	int Load(const CStdString& strTextureName, bool checkBundleOnly = false);
	const CTextureArrayGL* CGUITextureManagerGL::GetTexture(const CStdString& strTextureName);
};

/*!
\ingroup textures
\brief 
*/
extern CGUITextureManagerGL g_TextureManager;
#endif
