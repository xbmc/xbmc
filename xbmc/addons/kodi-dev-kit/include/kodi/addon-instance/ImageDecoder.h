/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/imagedecoder.h"

#ifdef __cplusplus

#include <stdexcept>

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
/// @defgroup cpp_kodi_addon_imagedecoder_Defs_ImageDecoderInfoTag class ImageDecoderInfoTag
/// @ingroup cpp_kodi_addon_imagedecoder_Defs
/// @brief **Info tag data structure**\n
/// Representation of available information of processed audio file.
///
/// This is used to get all the necessary data of audio stream and to have on
/// created files by encoders.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_imagedecoder_Defs_ImageDecoderInfoTag_Help
///
///@{
class ATTR_DLL_LOCAL ImageDecoderInfoTag
{
public:
  /*! \cond PRIVATE */
  ImageDecoderInfoTag() = default;
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ImageDecoderInfoTag_Help Value Help
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs_ImageDecoderInfoTag
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_imagedecoder_Defs_ImageDecoderInfoTag :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Width** | `int` | @ref ImageDecoderInfoTag::SetWidth "SetWidth" | @ref ImageDecoderInfoTag::GetWidth "GetWidth"
  /// | **Height** | `int` | @ref ImageDecoderInfoTag::SetHeight "SetHeight" | @ref ImageDecoderInfoTag::GetHeight "GetHeight"
  /// | **Distance** | `float` | @ref ImageDecoderInfoTag::SetDistance "SetDistance" | @ref ImageDecoderInfoTag::GetDistance "GetDistance"
  /// | **Color** | @ref cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_COLOR | @ref ImageDecoderInfoTag::SetColor "SetColor" | @ref ImageDecoderInfoTag::GetColor "GetColor"
  /// | **Orientation** | @ref cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_ORIENTATION | @ref ImageDecoderInfoTag::SetOrientation "SetOrientation" | @ref ImageDecoderInfoTag::GetOrientation "GetOrientation"
  /// | **Metering mode** | @ref cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_METERING_MODE | @ref ImageDecoderInfoTag::SetMeteringMode "SetMeteringMode" | @ref ImageDecoderInfoTag::GetMeteringMode "GetMeteringMode"
  /// | **Exposure time** | `float` | @ref ImageDecoderInfoTag::SetExposureTime "SetExposureTime" | @ref ImageDecoderInfoTag::GetExposureTime "GetExposureTime"
  /// | **Exposure bias** | `float` | @ref ImageDecoderInfoTag::SetExposureBias "SetExposureBias" | @ref ImageDecoderInfoTag::GetExposureBias "GetExposureBias"
  /// | **Exposure program** | @ref cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_EXPOSURE_PROGRAM | @ref ImageDecoderInfoTag::SetExposureProgram "SetExposureProgram" | @ref ImageDecoderInfoTag::GetExposureProgram "GetExposureProgram"
  /// | **Exposure mode** | @ref cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_EXPOSURE_MODE | @ref ImageDecoderInfoTag::SetExposureMode "SetExposureMode" | @ref ImageDecoderInfoTag::GetExposureMode "GetExposureMode"
  /// | **Time created** | `time_t` | @ref ImageDecoderInfoTag::SetTimeCreated "SetTimeCreated" | @ref ImageDecoderInfoTag::GetTimeCreated "GetTimeCreated"
  /// | **Aperture F number** | `float` | @ref ImageDecoderInfoTag::SetApertureFNumber "SetApertureFNumber" | @ref ImageDecoderInfoTag::GetApertureFNumber "GetApertureFNumber"
  /// | **Flash used** | @ref ADDON_IMG_FLASH_TYPE | @ref ImageDecoderInfoTag::SetFlashUsed "SetFlashUsed" | @ref ImageDecoderInfoTag::GetFlashUsed "GetFlashUsed"
  /// | **Light source** | @ref ADDON_IMG_LIGHT_SOURCE | @ref ImageDecoderInfoTag::SetLightSource "SetLightSource" | @ref ImageDecoderInfoTag::GetLightSource "GetLightSource"
  /// | **Focal length** | `int` | @ref ImageDecoderInfoTag::SetFocalLength "SetFocalLength" | @ref ImageDecoderInfoTag::GetFocalLength "GetFocalLength"
  /// | **Focal length in 35 mm format** | `int` | @ref ImageDecoderInfoTag::SetFocalLengthIn35mmFormat "SetFocalLengthIn35mmFormat" | @ref ImageDecoderInfoTag::GetFocalLengthIn35mmFormat "GetFocalLengthIn35mmFormat"
  /// | **Digital zoom ratio** | `float` | @ref ImageDecoderInfoTag::SetDigitalZoomRatio "SetDigitalZoomRatio" | @ref ImageDecoderInfoTag::GetDigitalZoomRatio "GetDigitalZoomRatio"
  /// | **ISO sensitivity** | `float` | @ref ImageDecoderInfoTag::SetISOSpeed "SetISOSpeed" | @ref ImageDecoderInfoTag::GetISOSpeed "GetISOSpeed"
  /// | **Camera manufacturer** | `std::string` | @ref ImageDecoderInfoTag::SetCameraManufacturer "SetCameraManufacturer" | @ref ImageDecoderInfoTag::GetCameraManufacturer "GetCameraManufacturer"
  /// | **GPS info** | `bool, char, float[3], char, float[3], int, float` | @ref ImageDecoderInfoTag::SetGPSInfo "SetGPSInfo" | @ref ImageDecoderInfoTag::GetGPSInfo "GetGPSInfo"
  /// | **Camera model** | `std::string` | @ref ImageDecoderInfoTag::SetCameraModel "SetCameraModel" | @ref ImageDecoderInfoTag::GetCameraModel "GetCameraModel"
  /// | **Author** | `std::string` | @ref ImageDecoderInfoTag::SetAuthor "SetAuthor" | @ref ImageDecoderInfoTag::GetAuthor "GetAuthor"
  /// | **Description** | `std::string` | @ref ImageDecoderInfoTag::SetDescription "SetDescription" | @ref ImageDecoderInfoTag::GetDescription "GetDescription"
  /// | **Copyright** | `std::string` | @ref ImageDecoderInfoTag::SetCopyright "SetCopyright" | @ref ImageDecoderInfoTag::GetCopyright "GetCopyright"

  /// @addtogroup cpp_kodi_addon_imagedecoder_Defs_ImageDecoderInfoTag
  ///@{

  /// @brief Set the camera manufacturer as string on info tag.
  void SetWidth(int width) { m_width = width; }

  /// @brief Get image width as pixels
  int GetWidth() const { return m_width; }

  /// @brief Set the image height as pixels.
  void SetHeight(int height) { m_height = height; }

  /// @brief Get image height as pixels.
  int GetHeight() const { return m_height; }

  /// @brief Set the image distance in meters.
  void SetDistance(int distance) { m_distance = distance; }

  /// @brief Get mage distance in meters.
  int GetDistance() const { return m_distance; }

  /// @brief Set the image color type.
  ///
  /// @copydetails cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_COLOR
  void SetColor(ADDON_IMG_COLOR color) { m_color = color; }

  /// @brief Get image image color type.
  ADDON_IMG_COLOR GetColor() const { return m_color; }

  /// @brief Set metering mode.
  ///
  /// @copydetails cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_METERING_MODE
  void SetMeteringMode(ADDON_IMG_METERING_MODE metering_mode) { m_metering_mode = metering_mode; }

  /// @brief Get metering mode.
  ADDON_IMG_METERING_MODE GetMeteringMode() const { return m_metering_mode; }

  /// @brief Set exposure time.
  void SetExposureTime(float exposure_time) { m_exposure_time = exposure_time; }

  /// @brief Get exposure time.
  float GetExposureTime() const { return m_exposure_time; }

  /// @brief Set exposure bias.
  void SetExposureBias(float exposure_bias) { m_exposure_bias = exposure_bias; }

  /// @brief Get exposure bias.
  float GetExposureBias() const { return m_exposure_bias; }

  /// @brief Set Exposure program.
  ///
  /// @copydetails cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_EXPOSURE_PROGRAM
  void SetExposureProgram(ADDON_IMG_EXPOSURE_PROGRAM exposure_program)
  {
    m_exposure_program = exposure_program;
  }

  /// @brief Get Exposure program.
  ADDON_IMG_EXPOSURE_PROGRAM GetExposureProgram() const { return m_exposure_program; }

  /// @brief Set Exposure mode.
  ///
  /// @copydetails cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_EXPOSURE_MODE
  void SetExposureMode(ADDON_IMG_EXPOSURE_MODE exposure_mode) { m_exposure_mode = exposure_mode; }

  /// @brief Get Exposure mode.
  ADDON_IMG_EXPOSURE_MODE GetExposureMode() const { return m_exposure_mode; }

  /// @brief Set the orientation.
  ///
  /// @copydetails cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_ORIENTATION
  void SetOrientation(ADDON_IMG_ORIENTATION orientation) { m_orientation = orientation; }

  /// @brief Get image orientation.
  ADDON_IMG_ORIENTATION GetOrientation() const { return m_orientation; }

  /// @brief Set the image creation time in time_t format (number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC).
  void SetTimeCreated(time_t time_created) { m_time_created = time_created; }

  /// @brief Get image creation time.
  time_t GetTimeCreated() const { return m_time_created; }

  /// @brief Set Aperture F number.
  void SetApertureFNumber(float aperture_f_number) { m_aperture_f_number = aperture_f_number; }

  /// @brief Get Aperture F number.
  float GetApertureFNumber() const { return m_aperture_f_number; }

  /// @brief Set to true if image was created with flash.
  void SetFlashUsed(ADDON_IMG_FLASH_TYPE flash_used) { m_flash_used = flash_used; }

  /// @brief Get info about image was created with flash.
  ADDON_IMG_FLASH_TYPE GetFlashUsed() const { return m_flash_used; }

  /// @brief Set focal length
  void SetFocalLength(int focal_length) { m_focal_length = focal_length; }

  /// @brief Get focal length
  int GetFocalLength() const { return m_focal_length; }

  /// @brief Set light source
  void SetLightSource(ADDON_IMG_LIGHT_SOURCE light_source) { m_light_source = light_source; }

  /// @brief Get light source
  ADDON_IMG_LIGHT_SOURCE GetLightSource() const { return m_light_source; }

  /// @brief Set focal length in 35 mm format.
  ///
  /// @note Same as FocalLengthIn35mmFilm in EXIF standard, tag 0xa405.
  void SetFocalLengthIn35mmFormat(int focal_length_in_35mm_format)
  {
    m_focal_length_in_35mm_format = focal_length_in_35mm_format;
  }

  /// @brief Get focal length in 35 mm format.
  int GetFocalLengthIn35mmFormat() const { return m_focal_length_in_35mm_format; }

  /// @brief Set digital zoom ratio.
  void SetDigitalZoomRatio(float digital_zoom_ratio) { m_digital_zoom_ratio = digital_zoom_ratio; }

  /// @brief Get digital zoom ratio.
  float GetDigitalZoomRatio() const { return m_digital_zoom_ratio; }

  /// @brief Set ISO sensitivity.
  void SetISOSpeed(float iso_speed) { m_iso_speed = iso_speed; }

  /// @brief Get ISO sensitivity.
  float GetISOSpeed() const { return m_iso_speed; }

  /// @brief Set GPS position information.
  void SetGPSInfo(bool gps_info_present,
                  char latitude_ref,
                  float latitude[3],
                  char longitude_ref,
                  float longitude[3],
                  int altitude_ref,
                  float altitude)
  {
    if (!latitude || !longitude)
      return;

    m_gps_info_present = gps_info_present;
    if (gps_info_present)
    {
      m_latitude_ref = latitude_ref;
      m_longitude_ref = longitude_ref;
      for (int i = 0; i < 3; i++)
      {
        m_latitude[i] = latitude[i];
        m_longitude[i] = longitude[i];
      }
      m_altitude_ref = altitude_ref;
      m_altitude = altitude;
    }
    else
    {
      m_latitude_ref = 0.0f;
      m_longitude_ref = 0.0f;
      for (int i = 0; i < 3; i++)
        latitude[i] = longitude[i] = 0.0f;
      m_altitude_ref = 0;
      m_altitude = 0.0f;
    }
  }

  /// @brief Get GPS position information.
  void GetGPSInfo(bool& gps_info_present,
                  char& latitude_ref,
                  float latitude[3],
                  char& longitude_ref,
                  float longitude[3],
                  int& altitude_ref,
                  float& altitude)
  {
    if (!latitude || !longitude)
      return;

    gps_info_present = m_gps_info_present;
    if (m_gps_info_present)
    {
      latitude_ref = m_latitude_ref;
      longitude_ref = m_longitude_ref;
      for (int i = 0; i < 3; i++)
      {
        latitude[i] = m_latitude[i];
        longitude[i] = m_longitude[i];
      }
      altitude_ref = m_altitude_ref;
      altitude = m_altitude;
    }
    else
    {
      latitude_ref = ' ';
      longitude_ref = ' ';
      for (int i = 0; i < 3; i++)
        latitude[i] = longitude[i] = 0.0f;
      altitude_ref = 0;
      altitude = 0.0f;
    }
  }

  /// @brief Set the camera manufacturer as string on info tag.
  void SetCameraManufacturer(const std::string& camera_manufacturer)
  {
    m_camera_manufacturer = camera_manufacturer;
  }

  /// @brief Get camera manufacturer
  const std::string& GetCameraManufacturer() const { return m_camera_manufacturer; }

  /// @brief Set camera model
  void SetCameraModel(const std::string& camera_model) { m_camera_model = camera_model; }

  /// @brief Get camera model
  const std::string& GetCameraModel() const { return m_camera_model; }

  /// @brief Set author
  void SetAuthor(const std::string& author) { m_author = author; }

  /// @brief Get author
  const std::string& GetAuthor() const { return m_author; }

  /// @brief Set description
  void SetDescription(const std::string& description) { m_description = description; }

  /// @brief Get description
  const std::string& GetDescription() const { return m_description; }

  /// @brief Set copyright
  void SetCopyright(const std::string& copyright) { m_copyright = copyright; }

  /// @brief Get copyright
  const std::string& GetCopyright() const { return m_copyright; }

  ///@}

private:
  int m_width{};
  int m_height{};
  float m_distance{};
  ADDON_IMG_ORIENTATION m_orientation{ADDON_IMG_ORIENTATION_NONE};
  ADDON_IMG_COLOR m_color{ADDON_IMG_COLOR_COLORED};
  ADDON_IMG_METERING_MODE m_metering_mode{ADDON_IMG_METERING_MODE_UNKNOWN};
  float m_exposure_time{};
  float m_exposure_bias{};
  ADDON_IMG_EXPOSURE_PROGRAM m_exposure_program{ADDON_IMG_EXPOSURE_PROGRAM_UNDEFINED};
  ADDON_IMG_EXPOSURE_MODE m_exposure_mode{ADDON_IMG_EXPOSURE_MODE_AUTO};
  time_t m_time_created{};
  float m_aperture_f_number{};
  ADDON_IMG_FLASH_TYPE m_flash_used{ADDON_IMG_FLASH_TYPE_NO_FLASH};
  ADDON_IMG_LIGHT_SOURCE m_light_source{};
  int m_focal_length{};
  int m_focal_length_in_35mm_format{};
  float m_digital_zoom_ratio{};
  float m_iso_speed{};

  bool m_gps_info_present{false};
  char m_latitude_ref{' '};
  float m_latitude[3]{}; /* Deg,min,sec */
  char m_longitude_ref{' '};
  float m_longitude[3]{}; /* Deg,min,sec */
  int m_altitude_ref{};
  float m_altitude{};

  std::string m_camera_manufacturer;
  std::string m_camera_model;
  std::string m_author;
  std::string m_description;
  std::string m_copyright;
};
///@}
//------------------------------------------------------------------------------

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
///     library_@PLATFORM@="@LIBRARY_FILENAME@">
///     <support>
///       <mimetype name="image/mymimea">
///         <extension>.imga</extension>
///         <description>30100</description>
///         <icon>resources/file_format_icon_a.png</icon>
///       </mimetype>
///       <mimetype name="image/mymimeb">
///         <extension>.imgb</extension>
///         <description>30101</description>
///         <icon>resources/file_format_icon_b.png</icon>
///       </mimetype>
///     </support>
///   </extension>
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
///     first <b>`<support>`</b> is also the main type of addon.
///   }
///   \table_row3{   <b>`library_@PLATFORM@`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_library
///                  string,
///     The runtime library used for the addon. This is usually declared by `cmake` and correctly displayed in the translated <b>`addon.xml`</b>.
///   }
///   \table_row3{   <b>`<support>...</support>`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_support
///                  XML group,
///     Contains the formats supported by the addon.
///   }
///   \table_row3{   <b>`<mimetype name="image/mymimea">...</mimetype>`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_mimetype
///                  string / group,
///     The from addon operated image [mimetypes](https://en.wikipedia.org/wiki/Media_type).\n
///     Optional can be with `<description>` and `<icon>` additional info added where used for list views in Kodi.
///   }
///   \table_row3{   <b>`<mimetype ...><extension>...</extension></mimetype>`</b>,
///                  @anchor cpp_kodi_addon_imagedecoder_mimetype
///                  string,
///     The file extensions / styles supported by this addon and relates to given mimetype before.\n
///     @note Required to use about info support by @ref CInstanceImageDecoder::SupportsFile and @ref CInstanceImageDecoder::ReadTag!
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
/// class ATTR_DLL_LOCAL CMyImageDecoder : public kodi::addon::CInstanceImageDecoder
/// {
/// public:
///   CMyImageDecoder(const kodi::addon::IInstanceInfo& instance);
///
///   bool LoadImageFromMemory(const uint8_t* buffer,
///                            size_t bufSize,
///                            unsigned int& width,
///                            unsigned int& height) override;
///
///   bool Decode(uint8_t* pixels,
///               unsigned int width,
///               unsigned int height,
///               unsigned int pitch,
///               ADDON_IMG_FMT format) override;
///
///   ...
/// };
///
/// CMyImageDecoder::CMyImageDecoder(const kodi::addon::IInstanceInfo& instance)
///   : CInstanceImageDecoder(instance)
/// {
///   ...
/// }
///
/// bool CMyImageDecoder::LoadImageFromMemory(const uint8_t* buffer,
///                                           size_t bufSize,
///                                           unsigned int& width,
///                                           unsigned int& height)
/// {
///   ...
///   return true;
/// }
///
/// bool CMyImageDecoder::Decode(uint8_t* pixels,
///                              unsigned int width,
///                              unsigned int height,
///                              unsigned int pitch,
///                              ADDON_IMG_FMT format) override;
/// {
///   ...
///   return true;
/// }
///
/// //----------------------------------------------------------------------
///
/// class ATTR_DLL_LOCAL CMyAddon : public kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() = default;
///   ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                               KODI_ADDON_INSTANCE_HDL& hdl) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                                       KODI_ADDON_INSTANCE_HDL& hdl)
/// {
///   if (instance.IsType(ADDON_INSTANCE_IMAGEDECODER))
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my image decoder instance");
///     hdl = new CMyImageDecoder(instance);
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
class ATTR_DLL_LOCAL CInstanceImageDecoder : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief Class constructor.
  ///
  /// @param[in] instance The from Kodi given instance given be add-on
  ///                     CreateInstance call with instance id
  ///                     @ref ADDON_INSTANCE_IMAGEDECODER.
  ///
  explicit CInstanceImageDecoder(const IInstanceInfo& instance) : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceImageDecoder: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  ~CInstanceImageDecoder() override = default;

  //==========================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief Checks addon support given file path.
  ///
  /// @param[in] filename The file to read
  /// @return true if successfully done and supported, otherwise false
  ///
  /// @note Optional to add, as default becomes `true` used.
  ///
  virtual bool SupportsFile(const std::string& filename) { return true; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief Read tag of a file.
  ///
  /// @param[in] file File to read tag for
  /// @param[out] tag Information tag about
  /// @return True on success, false on failure
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_imagedecoder_Defs_ImageDecoderInfoTag_Help
  ///
  virtual bool ReadTag(const std::string& file, kodi::addon::ImageDecoderInfoTag& tag)
  {
    return false;
  }
  //--------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_imagedecoder
  /// @brief Initialize an encoder.
  ///
  /// @param[in] mimetype The mimetype wanted from Kodi
  /// @param[in] buffer The data to read from memory
  /// @param[in] bufSize The buffer size
  /// @param[in,out] width The optimal width of image on entry, obtained width
  ///                      on return
  /// @param[in,out] height The optimal height of image, actual obtained height
  ///                       on return
  /// @return true if successful done, false on error
  ///
  virtual bool LoadImageFromMemory(const std::string& mimetype,
                                   const uint8_t* buffer,
                                   size_t bufSize,
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
  virtual bool Decode(uint8_t* pixels,
                      unsigned int width,
                      unsigned int height,
                      unsigned int pitch,
                      ADDON_IMG_FMT format) = 0;
  //----------------------------------------------------------------------------

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;
    instance->imagedecoder->toAddon->supports_file = ADDON_supports_file;
    instance->imagedecoder->toAddon->read_tag = ADDON_read_tag;
    instance->imagedecoder->toAddon->load_image_from_memory = ADDON_load_image_from_memory;
    instance->imagedecoder->toAddon->decode = ADDON_decode;
  }

  inline static bool ADDON_supports_file(const KODI_ADDON_IMAGEDECODER_HDL hdl, const char* file)
  {
    return static_cast<CInstanceImageDecoder*>(hdl)->SupportsFile(file);
  }

  inline static bool ADDON_read_tag(const KODI_ADDON_IMAGEDECODER_HDL hdl,
                                    const char* file,
                                    struct KODI_ADDON_IMAGEDECODER_INFO_TAG* tag)
  {
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif // _WIN32
    kodi::addon::ImageDecoderInfoTag cppTag;
    bool ret = static_cast<CInstanceImageDecoder*>(hdl)->ReadTag(file, cppTag);
    if (ret)
    {
      tag->width = cppTag.GetWidth();
      tag->height = cppTag.GetHeight();
      tag->distance = cppTag.GetDistance();
      tag->color = cppTag.GetColor();
      tag->orientation = cppTag.GetOrientation();
      tag->metering_mode = cppTag.GetMeteringMode();
      tag->exposure_time = cppTag.GetExposureTime();
      tag->exposure_bias = cppTag.GetExposureBias();
      tag->exposure_mode = cppTag.GetExposureMode();
      tag->exposure_program = cppTag.GetExposureProgram();
      tag->time_created = cppTag.GetTimeCreated();
      tag->aperture_f_number = cppTag.GetApertureFNumber();
      tag->flash_used = cppTag.GetFlashUsed();
      tag->light_source = cppTag.GetLightSource();
      tag->focal_length = cppTag.GetFocalLength();
      tag->focal_length_in_35mm_format = cppTag.GetFocalLengthIn35mmFormat();
      tag->iso_speed = cppTag.GetISOSpeed();
      tag->digital_zoom_ratio = cppTag.GetDigitalZoomRatio();
      cppTag.GetGPSInfo(tag->gps_info_present, tag->latitude_ref, tag->latitude, tag->longitude_ref,
                        tag->longitude, tag->altitude_ref, tag->altitude);
      tag->camera_manufacturer = strdup(cppTag.GetCameraManufacturer().c_str());
      tag->camera_model = strdup(cppTag.GetCameraModel().c_str());
      tag->author = strdup(cppTag.GetAuthor().c_str());
      tag->description = strdup(cppTag.GetDescription().c_str());
      tag->copyright = strdup(cppTag.GetCopyright().c_str());
    }
    return ret;
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32
  }

  inline static bool ADDON_load_image_from_memory(const KODI_ADDON_IMAGEDECODER_HDL hdl,
                                                  const char* mimetype,
                                                  const uint8_t* buffer,
                                                  size_t bufSize,
                                                  unsigned int* width,
                                                  unsigned int* height)
  {
    return static_cast<CInstanceImageDecoder*>(hdl)->LoadImageFromMemory(mimetype, buffer, bufSize,
                                                                         *width, *height);
  }

  inline static bool ADDON_decode(const KODI_ADDON_IMAGEDECODER_HDL hdl,
                                  uint8_t* pixels,
                                  size_t pixels_size,
                                  unsigned int width,
                                  unsigned int height,
                                  unsigned int pitch,
                                  enum ADDON_IMG_FMT format)
  {
    return static_cast<CInstanceImageDecoder*>(hdl)->Decode(pixels, width, height, pitch, format);
  }
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
