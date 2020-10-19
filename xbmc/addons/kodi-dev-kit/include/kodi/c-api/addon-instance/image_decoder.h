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

  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Image format types**\n
  /// Used to define wanted target format where image decoder should give to
  /// Kodi.
  ///
  typedef enum ImageFormat
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
  } ImageFormat;
  //----------------------------------------------------------------------------

  typedef struct AddonProps_ImageDecoder
  {
    const char* mimetype;
  } AddonProps_ImageDecoder;

  typedef struct AddonToKodiFuncTable_ImageDecoder
  {
    KODI_HANDLE kodi_instance;
  } AddonToKodiFuncTable_ImageDecoder;

  struct AddonInstance_ImageDecoder;
  typedef struct KodiToAddonFuncTable_ImageDecoder
  {
    KODI_HANDLE addonInstance;
    bool(__cdecl* load_image_from_memory)(const struct AddonInstance_ImageDecoder* instance,
                                          unsigned char* buffer,
                                          unsigned int buf_size,
                                          unsigned int* width,
                                          unsigned int* height);

    bool(__cdecl* decode)(const struct AddonInstance_ImageDecoder* instance,
                          unsigned char* pixels,
                          unsigned int width,
                          unsigned int height,
                          unsigned int pitch,
                          enum ImageFormat format);
  } KodiToAddonFuncTable_ImageDecoder;

  typedef struct AddonInstance_ImageDecoder
  {
    struct AddonProps_ImageDecoder* props;
    struct AddonToKodiFuncTable_ImageDecoder* toKodi;
    struct KodiToAddonFuncTable_ImageDecoder* toAddon;
  } AddonInstance_ImageDecoder;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_IMAGE_DECODER_H */
