#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../AddonBase.h"

namespace kodi { namespace addon { class CInstanceImageDecoder; }}

extern "C"
{

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
    kodi::addon::CInstanceImageDecoder* addonInstance;
    bool (__cdecl* load_image_from_memory) (const AddonInstance_ImageDecoder* instance,
                                            unsigned char* buffer, unsigned int buf_size,
                                            unsigned int* width, unsigned int* height);

    bool (__cdecl* decode) (const AddonInstance_ImageDecoder* instance,
                            unsigned char* pixels,
                            unsigned int width, unsigned int height,
                            unsigned int pitch, unsigned int format);
  } KodiToAddonFuncTable_ImageDecoder;

  typedef struct AddonInstance_ImageDecoder
  {
    AddonProps_ImageDecoder props;
    AddonToKodiFuncTable_ImageDecoder toKodi;
    KodiToAddonFuncTable_ImageDecoder toAddon;
  } AddonInstance_ImageDecoder;

} /* extern "C" */

typedef enum ImageFormat : unsigned int
{
  ADDON_IMG_FMT_A8R8G8B8 = 1,
  ADDON_IMG_FMT_A8 = 2,
  ADDON_IMG_FMT_RGBA8 = 3,
  ADDON_IMG_FMT_RGB8 = 4
} ImageFormat;

namespace kodi
{
namespace addon
{

  class CInstanceImageDecoder : public IAddonInstance
  {
  public:
    //==========================================================================
    /// @brief Class constructor
    ///
    /// @param[in] instance             The from Kodi given instance given be
    ///                                 add-on CreateInstance call with instance
    ///                                 id ADDON_INSTANCE_IMAGEDECODER.
    CInstanceImageDecoder(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_IMAGEDECODER)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceImageDecoder: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }
    //--------------------------------------------------------------------------

    ~CInstanceImageDecoder() override = default;

    //==========================================================================
    /// @brief Initialize an encoder
    ///
    /// @param[in] buffer The data to read from memory
    /// @param[in] bufSize The buffer size
    /// @param[in,out] width The optimal width of image on entry, obtained width on return
    /// @param[in,out] height The optimal height of image, actual obtained height on return
    /// @return true if successful done, false on error
    ///
    virtual bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                                     unsigned int& width, unsigned int& height) = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @brief Decode previously loaded image
    ///
    /// @param[in] pixels Output buffer
    /// @param[in] width Width of output image
    /// @param[in] height Height of output image
    /// @param[in] pitch Pitch of output image
    /// @param[in] format Format of output image
    /// @return true if successful done, false on error
    ///
    virtual bool Decode(unsigned char* pixels,
                        unsigned int width, unsigned int height,
                        unsigned int pitch, ImageFormat format) = 0;
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @brief Get the wanted mime type from Kodi
    ///
    /// @return the mimetype wanted from Kodi
    ///
    inline std::string MimeType() { return m_instanceData->props.mimetype; }
    //--------------------------------------------------------------------------

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceImageDecoder: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_ImageDecoder*>(instance);
      m_instanceData->toAddon.addonInstance = this;
      m_instanceData->toAddon.load_image_from_memory = ADDON_LoadImageFromMemory;
      m_instanceData->toAddon.decode = ADDON_Decode;
    }

    inline static bool ADDON_LoadImageFromMemory(const AddonInstance_ImageDecoder* instance,
                                                 unsigned char* buffer, unsigned int bufSize,
                                                 unsigned int* width, unsigned int* height)
    {
      return instance->toAddon.addonInstance->LoadImageFromMemory(buffer, bufSize, *width, *height);
    }

    inline static bool ADDON_Decode(const AddonInstance_ImageDecoder* instance,
                                    unsigned char* pixels,
                                    unsigned int width, unsigned int height,
                                    unsigned int pitch, unsigned int format)
    {
      return instance->toAddon.addonInstance->Decode(pixels, width, height, pitch, static_cast<ImageFormat>(format));
    }

    AddonInstance_ImageDecoder* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
