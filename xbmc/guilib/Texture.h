/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/TextureBase.h"
#include "guilib/TextureFormats.h"

#include <cstddef>
#include <memory>
#include <string>

class IImage;


#pragma pack(1)
struct COLOR {unsigned char b,g,r,x;};	// Windows GDI expects 4bytes per color
#pragma pack()

/*!
\ingroup textures
\brief Texture loader class, subclasses of which depend on the render spec (DX, GL etc.)
*/
class CTexture : public CTextureBase
{

public:
  CTexture(unsigned int width = 0, unsigned int height = 0, XB_FMT format = XB_FMT_A8R8G8B8);
  virtual ~CTexture();

  static std::unique_ptr<CTexture> CreateTexture(unsigned int width = 0,
                                                 unsigned int height = 0,
                                                 XB_FMT format = XB_FMT_A8R8G8B8);

  /*! \brief Load a texture from a file
   Loads a texture from a file, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param texturePath the path of the texture to load.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \param strMimeType mimetype of the given texture if available (defaults to empty)
   \return a CTexture std::unique_ptr to the created texture - nullptr if the texture failed to load.
   */
  static std::unique_ptr<CTexture> LoadFromFile(const std::string& texturePath,
                                                unsigned int idealWidth = 0,
                                                unsigned int idealHeight = 0,
                                                bool requirePixels = false,
                                                const std::string& strMimeType = "");

  /*! \brief Load a texture from a file in memory
   Loads a texture from a file in memory, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param buffer the memory buffer holding the file.
   \param bufferSize the size of buffer.
   \param mimeType the mime type of the file in buffer.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \return a CTexture std::unique_ptr to the created texture - nullptr if the texture failed to load.
   */
  static std::unique_ptr<CTexture> LoadFromFileInMemory(unsigned char* buffer,
                                                        size_t bufferSize,
                                                        const std::string& mimeType,
                                                        unsigned int idealWidth = 0,
                                                        unsigned int idealHeight = 0);

  bool LoadFromMemory(unsigned int width,
                      unsigned int height,
                      unsigned int pitch,
                      XB_FMT format,
                      bool hasAlpha,
                      const unsigned char* pixels);
  bool LoadPaletted(unsigned int width,
                    unsigned int height,
                    unsigned int pitch,
                    XB_FMT format,
                    const unsigned char* pixels,
                    const COLOR* palette);

  void Update(unsigned int width,
              unsigned int height,
              unsigned int pitch,
              XB_FMT format,
              const unsigned char* pixels,
              bool loadToGPU);

  virtual void CreateTextureObject() = 0;
  virtual void DestroyTextureObject() = 0;
  virtual void LoadToGPU() = 0;
  virtual void BindToUnit(unsigned int unit) = 0;

private:
  // no copy constructor
  CTexture(const CTexture& copy) = delete;

protected:
  bool LoadFromFileInMem(unsigned char* buffer, size_t size, const std::string& mimeType,
                         unsigned int maxWidth, unsigned int maxHeight);
  bool LoadFromFileInternal(const std::string& texturePath, unsigned int maxWidth, unsigned int maxHeight, bool requirePixels, const std::string& strMimeType = "");
  bool LoadIImage(IImage* pImage,
                  unsigned char* buffer,
                  unsigned int bufSize,
                  unsigned int width,
                  unsigned int height);
};
