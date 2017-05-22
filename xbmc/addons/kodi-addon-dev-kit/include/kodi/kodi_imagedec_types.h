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

#include <stdint.h>

#define ADDON_IMG_FMT_A8R8G8B8 1
#define ADDON_IMG_FMT_A8       2
#define ADDON_IMG_FMT_RGBA8    3
#define ADDON_IMG_FMT_RGB8     4

extern "C"
{
  typedef struct AddonProps_ImageDecoder
  {
    const char* mimetype;
  } AddonProps_ImageDecoder;

  typedef struct AddonToKodiFuncTable_ImageDecoder
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_ImageDecoder;

  typedef struct KodiToAddonFuncTable_ImageDecoder
  {
    //! \brief Initialize an encoder
    //! \param buffer The data to read from memory
    //! \param bufSize The buffer size
    //! \param width The optimal width of image on entry, obtained width on return
    //! \param height The optimal height of image, actual obtained height on return
    //! \return Image or nullptr on error
    void* (__cdecl* LoadImage) (unsigned char* buffer, unsigned int bufSize,
                                unsigned int* width, unsigned int* height);

    //! \brief Decode previously loaded image
    //! \param image Image to decode
    //! \param pixels Output buffer
    //! \param width Width of output image
    //! \param height Height of output image
    //! \param pitch Pitch of output image
    //! \param format Format of output image
    bool (__cdecl* Decode) (void* image, unsigned char* pixels,
                            unsigned int width, unsigned int height,
                            unsigned int pitch, unsigned int format);

    //! \brief Close an opened image
    //! \param image Image to close
    //! \return True on success, false on failure
    void (__cdecl* Close)(void* image);
  } KodiToAddonFuncTable_ImageDecoder;

  typedef struct AddonInstance_ImageDecoder
  {
    AddonProps_ImageDecoder props;
    AddonToKodiFuncTable_ImageDecoder toKodi;
    KodiToAddonFuncTable_ImageDecoder toAddon;
  } AddonInstance_ImageDecoder;
}
