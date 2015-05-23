/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "system.h"
#include "XBTF.h"
#include "guilib/imagefactory.h"

#pragma pack(1)
struct COLOR {unsigned char b,g,r,x;};	// Windows GDI expects 4bytes per color
#pragma pack()

class CTexture;
class CGLTexture;
class CPiTexture;
class CDXTexture;

/*!
\ingroup textures
\brief Base texture class, subclasses of which depend on the render spec (DX, GL etc.)
*/
class CBaseTexture
{

public:
  CBaseTexture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_A8R8G8B8);

  virtual ~CBaseTexture();

  /*! \brief Load a texture from a file
   Loads a texture from a file, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param texturePath the path of the texture to load.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \param autoRotate whether the textures should be autorotated based on EXIF information (defaults to false).
   \param strMimeType mimetype of the given texture if available (defaults to empty)
   \return a CBaseTexture pointer to the created texture - NULL if the texture failed to load.
   */
  static CBaseTexture *LoadFromFile(const std::string& texturePath, unsigned int idealWidth = 0, unsigned int idealHeight = 0,
                                    bool autoRotate = false, bool requirePixels = false, const std::string& strMimeType = "");

  /*! \brief Load a texture from a file in memory
   Loads a texture from a file in memory, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param buffer the memory buffer holding the file.
   \param bufferSize the size of buffer.
   \param mimeType the mime type of the file in buffer.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \return a CBaseTexture pointer to the created texture - NULL if the texture failed to load.
   */
  static CBaseTexture *LoadFromFileInMemory(unsigned char* buffer, size_t bufferSize, const std::string& mimeType,
                                            unsigned int idealWidth = 0, unsigned int idealHeight = 0);

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
  /*! \brief return the original width of the image, before scaling/cropping */
  unsigned int GetOriginalWidth() const { return m_originalWidth; }
  /*! \brief return the original height of the image, before scaling/cropping */
  unsigned int GetOriginalHeight() const { return m_originalHeight; }

  int GetOrientation() const { return m_orientation; }
  void SetOrientation(int orientation) { m_orientation = orientation; }

  void Update(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU);
  void Allocate(unsigned int width, unsigned int height, unsigned int format);
  void ClampToEdge();

  static unsigned int PadPow2(unsigned int x);
  static bool SwapBlueRed(unsigned char *pixels, unsigned int height, unsigned int pitch, unsigned int elements = 4, unsigned int offset=0);

private:
  // no copy constructor
  CBaseTexture(const CBaseTexture &copy);

protected:
  bool LoadFromFileInMem(unsigned char* buffer, size_t size, const std::string& mimeType,
                         unsigned int maxWidth, unsigned int maxHeight);
  bool LoadFromFileInternal(const std::string& texturePath, unsigned int maxWidth, unsigned int maxHeight, bool autoRotate, bool requirePixels, const std::string& strMimeType = "");
  bool LoadIImage(IImage* pImage, unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height, bool autoRotate=false);
  // helpers for computation of texture parameters for compressed textures
  unsigned int GetPitch(unsigned int width) const;
  unsigned int GetRows(unsigned int height) const;
  unsigned int GetBlockSize() const;

  unsigned int m_imageWidth;
  unsigned int m_imageHeight;
  unsigned int m_textureWidth;
  unsigned int m_textureHeight;
  unsigned int m_originalWidth;   ///< original image width before scaling or cropping
  unsigned int m_originalHeight;  ///< original image height before scaling or cropping

  unsigned char* m_pixels;
  bool m_loadedToGPU;
  unsigned int m_format;
  int m_orientation;
  bool m_hasAlpha;
};

#if defined(HAS_OMXPLAYER)
#include "TexturePi.h"
#define CTexture CPiTexture
#elif defined(HAS_GL) || defined(HAS_GLES)
#include "TextureGL.h"
#define CTexture CGLTexture
#elif defined(HAS_DX)
#include "TextureDX.h"
#define CTexture CDXTexture
#endif
