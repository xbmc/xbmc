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

//##############################################################################
/// @defgroup cpp_kodi_addon_imagedecoder_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_imagedecoder
/// @brief **Image decoder add-on general variables**
///
/// Used to exchange the available options between Kodi and addon.
///
///

//==============================================================================
///
/// @addtogroup cpp_kodi_addon_imagedecoder
/// @brief @cpp_class{ kodi::addon::CInstanceImageDecoder }
/// **Image decoder add-on instance**\n
/// This instance type is used to allow Kodi various additional image format
/// types.
///
/// This usage can be requested under various conditions, by a Mimetype protocol
/// defined in <b>`addon.xml`</b> or supported file extensions.
///
/// Include the header @ref ImageDecoder.h "#include <kodi/addon-instance/ImageDecoder.h>"
/// to use this class.
///
/// ----------------------------------------------------------------------------
///
/// Here is an example of what the <b>`addon.xml.in`</b> would look like for an
/// image decoder addon:
///
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="imagedecoder.myspecialnamefor"
///   version="1.0.0"
///   name="My image decoder addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="kodi.imagedecoder"
///     extension=".imga|.imgb"
///     mimetype="image/mymimea|image/mymimea"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My image decoder addon summary</summary>
///     <description lang="en_GB">My image decoder description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
/// ### Standard values that can be declared for processing in `addon.xml`.
///
/// These values are used by Kodi to identify associated images and file
/// extensions and then to select the associated addon.
///
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`point`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_point
///                  string,
///     The identification of the addon instance to image decoder is mandatory
///     <b>`kodi.imagedecoder`</b>. In addition\, the instance declared in the
///     first <b>`<extension ... />`</b> is also the main type of addon.
///   }
///   \table_row3{   <b>`extension`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_defaultPort
///                  string,
///     The from addon operated and supported image file endings.\n
///     Use a <b>`|`</b> to separate between different ones.
///   }
///   \table_row3{   <b>`defaultPort`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_defaultPort
///                  string,
///     The from addon operated image [mimetypes](https://en.wikipedia.org/wiki/Media_type).\n
///     Use a <b>`|`</b> to separate between different ones.
///   }
///   \table_row3{   <b>`library_@PLATFORM@`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_library
///                  string,
///     The runtime library used for the addon. This is usually declared by `cmake` and correctly displayed in the translated <b>`addon.xml`</b>.
///   }
/// \table_end
///
/// @remark For more detailed description of the <b>`addon.xml`</b>, see also https://kodi.wiki/view/Addon.xml.
///
///
/// --------------------------------------------------------------------------
///
///
/// **Example:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/ImageDecoder.h>
///
/// class ATTRIBUTE_HIDDEN CMyImageDecoder : public kodi::addon::CInstanceImageDecoder
/// {
/// public:
///   CMyImageDecoder(KODI_HANDLE instance, const std::string& kodiVersion);
///
///   bool LoadImageFromMemory(unsigned char* buffer,
///                            unsigned int bufSize,
///                            unsigned int& width,
///                            unsigned int& height) override;
///
///   bool Decode(unsigned char* pixels,
///               unsigned int width,
///               unsigned int height,
///               unsigned int pitch,
///               ImageFormat format) override;
///
///   ...
/// };
///
/// CMyImageDecoder::CMyImageDecoder(KODI_HANDLE instance, const std::string& kodiVersion)
///   : CInstanceImageDecoder(instance, kodiVersion)
/// {
///   ...
/// }
///
/// bool CMyImageDecoder::LoadImageFromMemory(unsigned char* buffer,
///                                           unsigned int bufSize,
///                                           unsigned int& width,
///                                           unsigned int& height)
/// {
///   ...
///   return true;
/// }
///
/// bool CMyImageDecoder::Decode(unsigned char* pixels,
///                              unsigned int width,
///                              unsigned int height,
///                              unsigned int pitch,
///                              ImageFormat format) override;
/// {
///   ...
///   return true;
/// }
///
/// //----------------------------------------------------------------------
///
/// class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() = default;
///   ADDON_STATUS CreateInstance(int instanceType,
///                               const std::string& instanceID,
///                               KODI_HANDLE instance,
///                               const std::string& version,
///                               KODI_HANDLE& addonInstance) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
///                                       const std::string& instanceID,
///                                       KODI_HANDLE instance,
///                                       const std::string& version,
///                                       KODI_HANDLE& addonInstance)
/// {
///   if (instanceType == ADDON_INSTANCE_IMAGEDECODER)
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my image decoder instance");
///     addonInstance = new CMyImageDecoder(instance, version);
///     return ADDON_STATUS_OK;
///   }
///   else if (...)
///   {
///     ...
///   }
///   return ADDON_STATUS_UNKNOWN;
/// }
///
/// ADDONCREATOR(CMyAddon)
/// ~~~~~~~~~~~~~
///
/// The destruction of the example class `CMyImageDecoder` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
//------------------------------------------------------------------------------
class ATTRIBUTE_HIDDEN CInstanceImageDecoder : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief Class constructor.
  ///
  /// @param[in] instance The from Kodi given instance given be add-on
  ///                     CreateInstance call with instance id
  ///                     @ref ADDON_INSTANCE_IMAGEDECODER.
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///
  /// @note Recommended to set <b>`kodiVersion`</b>.
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
  //----------------------------------------------------------------------------

  ~CInstanceImageDecoder() override = default;

  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief Initialize an encoder.
  ///
  /// @param[in] buffer The data to read from memory
  /// @param[in] bufSize The buffer size
  /// @param[in,out] width The optimal width of image on entry, obtained width
  ///                      on return
  /// @param[in,out] height The optimal height of image, actual obtained height
  ///                       on return
  /// @return true if successful done, false on error
  ///
  virtual bool LoadImageFromMemory(unsigned char* buffer,
                                   unsigned int bufSize,
                                   unsigned int& width,
                                   unsigned int& height) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief Decode previously loaded image.
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief **Callback to Kodi Function**\n
  /// Get the wanted mime type from Kodi.
  ///
  /// @return the mimetype wanted from Kodi
  ///
  /// @remarks Only called from addon itself.
  ///
  inline std::string MimeType() { return m_instanceData->props->mimetype; }
  //----------------------------------------------------------------------------

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
