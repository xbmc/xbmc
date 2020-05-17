/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/image_decoder.h"

#ifdef __cplusplus
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
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///                        @note Recommended to set.
  ///
  explicit CInstanceImageDecoder(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_IMAGEDECODER,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_IMAGEDECODER))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceImageDecoder: Creation of multiple together "
                             "with single instance way is not allowed!");

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
  virtual bool LoadImageFromMemory(unsigned char* buffer,
                                   unsigned int bufSize,
                                   unsigned int& width,
                                   unsigned int& height) = 0;
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
                      unsigned int width,
                      unsigned int height,
                      unsigned int pitch,
                      ImageFormat format) = 0;
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Get the wanted mime type from Kodi
  ///
  /// @return the mimetype wanted from Kodi
  ///
  inline std::string MimeType() { return m_instanceData->props->mimetype; }
  //--------------------------------------------------------------------------

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceImageDecoder: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_ImageDecoder*>(instance);
    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->load_image_from_memory = ADDON_LoadImageFromMemory;
    m_instanceData->toAddon->decode = ADDON_Decode;
  }

  inline static bool ADDON_LoadImageFromMemory(const AddonInstance_ImageDecoder* instance,
                                               unsigned char* buffer,
                                               unsigned int bufSize,
                                               unsigned int* width,
                                               unsigned int* height)
  {
    return static_cast<CInstanceImageDecoder*>(instance->toAddon->addonInstance)
        ->LoadImageFromMemory(buffer, bufSize, *width, *height);
  }

  inline static bool ADDON_Decode(const AddonInstance_ImageDecoder* instance,
                                  unsigned char* pixels,
                                  unsigned int width,
                                  unsigned int height,
                                  unsigned int pitch,
                                  enum ImageFormat format)
  {
    return static_cast<CInstanceImageDecoder*>(instance->toAddon->addonInstance)
        ->Decode(pixels, width, height, pitch, format);
  }

  AddonInstance_ImageDecoder* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
