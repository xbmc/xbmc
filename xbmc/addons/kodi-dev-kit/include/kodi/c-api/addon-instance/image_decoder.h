/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_IMAGE_DECODER_H
#define C_API_ADDONINSTANCE_IMAGE_DECODER_H

#include "../addon_base.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef void* KODI_ADDON_IMAGEDECODER_HDL;

  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Image format types**\n
  /// Used to define wanted target format where image decoder should give to
  /// Kodi.
  ///
  typedef enum ADDON_IMG_FMT
  {
    /// @brief A 32-bit ARGB pixel format, with alpha, that uses 8 bits per
    /// channel, ARGBARGB...
    ADDON_IMG_FMT_A8R8G8B8 = 1,

    /// @brief A 8, alpha only, 8bpp, AAA...
    ADDON_IMG_FMT_A8 = 2,

    /// @brief RGBA 8:8:8:8, with alpha, 32bpp, RGBARGBA...
    ADDON_IMG_FMT_RGBA8 = 3,

    /// @brief RGB 8:8:8, with alpha, 24bpp, RGBRGB...
    ADDON_IMG_FMT_RGB8 = 4
  } ADDON_IMG_FMT;
  //----------------------------------------------------------------------------

  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_IMAGEDECODER_SUPPORTS_FILE_V1)(
      const KODI_ADDON_IMAGEDECODER_HDL hdl, const char* file);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_IMAGEDECODER_LOAD_IMAGE_FROM_MEMORY_V1)(
      const KODI_ADDON_IMAGEDECODER_HDL hdl,
      const uint8_t* buffer,
      size_t buf_size,
      unsigned int* width,
      unsigned int* height);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_IMAGEDECODER_DECODE_V1)(
      const KODI_ADDON_IMAGEDECODER_HDL hdl,
      uint8_t* pixels,
      size_t pixels_size,
      unsigned int width,
      unsigned int height,
      unsigned int pitch,
      enum ADDON_IMG_FMT format);

  typedef struct KodiToAddonFuncTable_ImageDecoder
  {
    PFN_KODI_ADDON_IMAGEDECODER_SUPPORTS_FILE_V1 supports_file;
    PFN_KODI_ADDON_IMAGEDECODER_LOAD_IMAGE_FROM_MEMORY_V1 load_image_from_memory;
    PFN_KODI_ADDON_IMAGEDECODER_DECODE_V1 decode;
  } KodiToAddonFuncTable_ImageDecoder;

  typedef struct AddonToKodiFuncTable_ImageDecoder
  {
    KODI_HANDLE kodi_instance;
    char* (*get_mime_type)(KODI_HANDLE hdl);
  } AddonToKodiFuncTable_ImageDecoder;

  typedef struct AddonInstance_ImageDecoder
  {
    struct AddonToKodiFuncTable_ImageDecoder* toKodi;
    struct KodiToAddonFuncTable_ImageDecoder* toAddon;
  } AddonInstance_ImageDecoder;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_IMAGE_DECODER_H */
