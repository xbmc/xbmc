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

#include "system.h"
#include "GUIFont.h"
#include "GUIFontTTFGL.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "gui3d.h"

// stuff for freetype
#ifndef _LINUX
#include "ft2build.h"
#else
#include <ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

using namespace std;

#ifdef HAS_GLES

CGUIFontTTFGLES::CGUIFontTTFGLES(const CStdString& strFileName)
: CGUIFontTTFBase(strFileName)
{

}

CGUIFontTTFGLES::~CGUIFontTTFGLES(void)
{

}

void CGUIFontTTFGLES::Begin()
{
  // TODO: GLES
}

void CGUIFontTTFGLES::End()
{
  // TODO: GLES
}

CBaseTexture* CGUIFontTTFGLES::ReallocTexture(unsigned int& newHeight)
{
  // TODO: GLES
  return NULL;
}

bool CGUIFontTTFGLES::CopyCharToTexture(FT_BitmapGlyph bitGlyph, Character* ch)
{
  // TODO: GLES
  return false;
}

void CGUIFontTTFGLES::DeleteHardwareTexture()
{
  // TODO: GLES
}

#endif
