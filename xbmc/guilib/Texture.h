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

#pragma once

#include "system.h"
#include "gui3d.h"
#include "utils/StdString.h"
#include "XBTF.h"

#pragma pack(1)
struct COLOR {unsigned char b,g,r,x;};	// Windows GDI expects 4bytes per color
#pragma pack()

class CTexture;
class CGLTexture;
class CDXTexture;
struct ImageInfo;

/*!
\ingroup textures
\brief Base texture class, subclasses of which depend on the render spec (DX, GL etc.)
*/
class CBaseTexture
{

public:
  CBaseTexture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_A8R8G8B8);
  CBaseTexture(const CBaseTexture &copy);

  virtual ~CBaseTexture();

  /*! \brief Load a texture from a file
   Loads a texture from a file, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param texturePath the path of the texture to load.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \param autoRotate whether the textures should be autorotated based on EXIF information (defaults to false).
   \return a CBaseTexture pointer to the created texture - NULL if the texture failed to load.
   */
  static CBaseTexture *LoadFromFile(const CStdString& texturePath, unsigned int idealWidth = 0, unsigned int idealHeight = 0,
                                    bool autoRotate = false);
  bool LoadFromFile(const CStdString& texturePath, unsigned int maxWidth, unsigned int maxHeight,
                    bool autoRotate, unsigned int *originalWidth, unsigned int *originalHeight);
  bool LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, bool hasAlpha, unsigned char* pixels);
  bool LoadPaletted(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, const COLOR *palette);

  bool HasAlpha() const;

  virtual void CreateTextureObject() = 0;
  virtual void DestroyTextureObject() = 0;
  virtual void LoadToGPU() = 0;
  virtual void BindToUnit(unsigned int unit) = 0;

  unsigned char* GetPixels() const { return m_pixels; }
  unsigned int GetPitch() const { return GetPitch(m_textureWidth); }
  unsigned int GetRows() const { return GetRows(m_textureHeight); }
  unsigned int GetTextureWidth() const { return m_textureWidth; }
  unsigned int GetTextureHeight() const { return m_textureHeight; }
  unsigned int GetWidth() const { return m_imageWidth; }
  unsigned int GetHeight() const { return m_imageHeight; }
  int GetOrientation() const { return m_orientation; }
  void SetOrientation(int orientation) { m_orientation = orientation; }

  void Update(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU);
  void Allocate(unsigned int width, unsigned int height, unsigned int format);
  void ClampToEdge();

  static unsigned int PadPow2(unsigned int x);
  bool SwapBlueRed(unsigned char *pixels, unsigned int height, unsigned int pitch, unsigned int elements = 4, unsigned int offset=0);

protected:
  void LoadFromImage(ImageInfo &image, bool autoRotate = false);
  // helpers for computation of texture parameters for compressed textures
  unsigned int GetPitch(unsigned int width) const;
  unsigned int GetRows(unsigned int height) const;
  unsigned int GetBlockSize() const;

  unsigned int m_imageWidth;
  unsigned int m_imageHeight;
  unsigned int m_textureWidth;
  unsigned int m_textureHeight;

  unsigned char* m_pixels;
  bool m_loadedToGPU;
  unsigned int m_format;
  int m_orientation;
  bool m_hasAlpha;
};

#if defined(HAS_GL) || defined(HAS_GLES)
#include "TextureGL.h"
#define CTexture CGLTexture
#elif defined(HAS_DX)
#include "TextureDX.h"
#define CTexture CDXTexture
#endif
