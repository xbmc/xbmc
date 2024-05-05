/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/TextureFormats.h"

#include <memory>
#include <string>

enum class TEXTURE_SCALING
{
  LINEAR,
  NEAREST,
};

/*!
\ingroup textures
\brief Base texture class, which holds the state of the texture, as well as all conversion functions.
*/
class CTextureBase
{

public:
  CTextureBase() = default;
  ~CTextureBase() = default;

  bool HasAlpha() const { return m_hasAlpha; }
  void SetAlpha(bool hasAlpha) { m_hasAlpha = hasAlpha; }

  /*! \brief sets mipmapping. do not use in new code. will be replaced with proper scaling. */
  void SetMipmapping() { m_mipmapping = true; }
  /*! \brief return true if we want to mipmap */
  bool IsMipmapped() const { return m_mipmapping; }

  /*! \brief sets the scaling method. scanout systems might support more than NN/linear. */
  void SetScalingMethod(TEXTURE_SCALING scalingMethod) { m_scalingMethod = scalingMethod; }
  /*! \brief returns the scaling method. */
  TEXTURE_SCALING GetScalingMethod() const { return m_scalingMethod; }

  int32_t GetOrientation() const { return m_orientation; }
  void SetOrientation(int orientation) { m_orientation = orientation; }

  /*! \brief retains a shadow copy of the texture on the CPU side. */
  void SetCacheMemory(bool bCacheMemory) { m_bCacheMemory = bCacheMemory; }
  /*! \brief returns true if a shadow copy is kept on the CPU side. */
  bool GetCacheMemory() const { return m_bCacheMemory; }

  /*! \brief returns a pointer to the staging texture. */
  uint8_t* GetPixels() const { return m_pixels; }

  /*! \brief return the size of one row in bytes. */
  uint32_t GetPitch() const { return GetPitch(m_textureWidth); }
  /*! \brief return the number of rows (number of blocks in the Y direction). */
  uint32_t GetRows() const { return GetRows(m_textureHeight); }
  /*! \brief return the total width of the texture, which might be scaled to fit its displayed size. */
  uint32_t GetTextureWidth() const { return m_textureWidth; }
  /*! \brief return the total height of the texture, which might be scaled to fit its displayed size. */
  uint32_t GetTextureHeight() const { return m_textureHeight; }
  /*! \brief return the total width of "active" area, which might be equal or lower than the texture width due to alignment. */
  uint32_t GetWidth() const { return m_imageWidth; }
  /*! \brief return the total height of "active" area, which might be equal or lower than the texture height due to alignment. */
  uint32_t GetHeight() const { return m_imageHeight; }
  /*! \brief return the original width of the image, before scaling/cropping */
  uint32_t GetOriginalWidth() const { return m_originalWidth; }
  /*! \brief return the original height of the image, before scaling/cropping */
  uint32_t GetOriginalHeight() const { return m_originalHeight; }

  // allocates staging texture space.
  void Allocate(uint32_t width, uint32_t height, XB_FMT format);
  void ClampToEdge();

  /*! \brief rounds to power of two. deprecated. */
  static uint32_t PadPow2(uint32_t x);
  /*! \brief swaps the blue/red channel. deprecated in this form. */
  static bool SwapBlueRed(
      uint8_t* pixels, uint32_t height, uint32_t pitch, uint32_t elements = 4, uint32_t offset = 0);

protected:
  // helpers for computation of texture parameters for compressed textures
  uint32_t GetPitch(uint32_t width) const;
  uint32_t GetRows(uint32_t height) const;
  uint32_t GetBlockWidth() const;
  uint32_t GetBlockHeight() const;
  /*! \brief return the size of a compression block (tile) in bytes. */
  uint32_t GetBlockSize() const;

  void SetKDFormat(XB_FMT xbFMT);

  uint32_t m_imageWidth{0};
  uint32_t m_imageHeight{0};
  uint32_t m_textureWidth{0};
  uint32_t m_textureHeight{0};
  uint32_t m_originalWidth{0}; ///< original image width before scaling or cropping
  uint32_t m_originalHeight{0}; ///< original image height before scaling or cropping

  uint8_t* m_pixels{nullptr};
  bool m_loadedToGPU{false};

  KD_TEX_FMT m_textureFormat{KD_TEX_FMT_UNKNOWN}; // new Kodi texture format
  KD_TEX_SWIZ m_textureSwizzle{KD_TEX_SWIZ_RGBA};
  KD_TEX_COL m_textureColorspace{KD_TEX_COL_REC709};
  KD_TEX_TRANSFER m_textureTransfer{KD_TEX_TRANSFER_SRGB};
  KD_TEX_ALPHA m_textureAlpha{KD_TEX_ALPHA_STRAIGHT};

  XB_FMT m_format{XB_FMT_UNKNOWN}; // legacy XB format, deprecated
  int32_t m_orientation{0};
  bool m_hasAlpha{true};
  bool m_mipmapping{false};
  TEXTURE_SCALING m_scalingMethod{TEXTURE_SCALING::LINEAR};
  bool m_bCacheMemory{false};
};
