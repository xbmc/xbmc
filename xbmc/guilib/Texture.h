/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/AspectRatio.h"
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
   \param aspectRatio the aspect ratio mode of the texture (defaults to "center").
   \param strMimeType mimetype of the given texture if available (defaults to empty)
   \return a CTexture std::unique_ptr to the created texture - nullptr if the texture failed to load.
   */
  static std::unique_ptr<CTexture> LoadFromFile(
      const std::string& texturePath,
      unsigned int idealWidth = 0,
      unsigned int idealHeight = 0,
      CAspectRatio::AspectRatio aspectRatio = CAspectRatio::CENTER,
      const std::string& strMimeType = "");

  /*! \brief Load a texture from a file in memory
   Loads a texture from a file in memory, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param buffer the memory buffer holding the file.
   \param bufferSize the size of buffer.
   \param mimeType the mime type of the file in buffer.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \param aspectRatio the aspect ratio mode of the texture (defaults to "center").
   \return a CTexture std::unique_ptr to the created texture - nullptr if the texture failed to load.
   */
  static std::unique_ptr<CTexture> LoadFromFileInMemory(
      unsigned char* buffer,
      size_t bufferSize,
      const std::string& mimeType,
      unsigned int idealWidth = 0,
      unsigned int idealHeight = 0,
      CAspectRatio::AspectRatio aspectRatio = CAspectRatio::CENTER);

  bool LoadFromMemory(unsigned int width,
                      unsigned int height,
                      unsigned int pitch,
                      XB_FMT format,
                      bool hasAlpha,
                      const unsigned char* pixels);
  /*! \brief Attempts to upload a texture directly from a provided buffer
   Unlike LoadFromMemory() which copies the texture into an intermediate buffer, the texture gets uploaded directly to
   the GPU if circumstances allow.
   \param width the width of the texture.
   \param height the height of the texture.
   \param pitch the pitch of the texture.
   \param pixels pointer to the texture buffer.
   \param format the format of the texture.
   \param alpha the alpha type of the texture.
   \param swizzle the swizzle pattern of the texture.
   */
  bool UploadFromMemory(unsigned int width,
                        unsigned int height,
                        unsigned int pitch,
                        unsigned char* pixels,
                        KD_TEX_FMT format = KD_TEX_FMT_SDR_RGBA8,
                        KD_TEX_ALPHA alpha = KD_TEX_ALPHA_OPAQUE,
                        KD_TEX_SWIZ swizzle = KD_TEX_SWIZ_RGBA);
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

  /*! 
   * \brief Uploads the texture to the GPU. 
   */
  void LoadToGPUAsync();

  virtual void CreateTextureObject() = 0;
  virtual void DestroyTextureObject() = 0;
  virtual void LoadToGPU() = 0;
  /*! 
   * \brief Blocks execution until the previous GFX commands have been processed.
   */
  virtual void SyncGPU(){};
  virtual void BindToUnit(unsigned int unit) = 0;

  /*! 
   * \brief Checks if the processing pipeline can handle the texture format/swizzle
   \param format the format of the texture.
   \return true if the texturing pipeline supports the format
   */
  virtual bool SupportsFormat(KD_TEX_FMT textureFormat, KD_TEX_SWIZ textureSwizzle)
  {
    return !(textureFormat & KD_TEX_FMT_TYPE_MASK) && textureSwizzle == KD_TEX_SWIZ_RGBA;
  }

private:
  // no copy constructor
  CTexture(const CTexture& copy) = delete;

protected:
  bool LoadFromFileInMem(unsigned char* buffer,
                         size_t size,
                         const std::string& mimeType,
                         unsigned int idealWidth,
                         unsigned int idealHeight,
                         CAspectRatio::AspectRatio aspectRatio);
  bool LoadFromFileInternal(const std::string& texturePath,
                            unsigned int idealWidth,
                            unsigned int idealHeight,
                            CAspectRatio::AspectRatio aspectRatio,
                            const std::string& strMimeType = "");
  bool LoadIImage(IImage* pImage,
                  unsigned char* buffer,
                  unsigned int bufSize,
                  unsigned int idealWidth,
                  unsigned int idealHeight,
                  CAspectRatio::AspectRatio aspectRatio);
};
