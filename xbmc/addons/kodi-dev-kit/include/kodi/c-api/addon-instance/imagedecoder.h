/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_IMAGEDECODER_H
#define C_API_ADDONINSTANCE_IMAGEDECODER_H

#include "../addon_base.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef KODI_ADDON_INSTANCE_HDL KODI_ADDON_IMAGEDECODER_HDL;

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_FMT enum ADDON_IMG_FMT
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Image format types**\n
  /// Used to define wanted target format where image decoder should give to
  /// Kodi.
  ///
  ///@{
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
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_ORIENTATION enum ADDON_IMG_ORIENTATION
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Image orientation types**\n
  /// Used to define how image becomes orientated for show.
  ///
  ///@{
  typedef enum ADDON_IMG_ORIENTATION
  {
    /// @brief If not available
    ADDON_IMG_ORIENTATION_NONE = 0,

    /// @brief Flip horizontal
    ADDON_IMG_ORIENTATION_FLIP_HORIZONTAL = 1,

    /// @brief Rotate 180° CCW
    ADDON_IMG_ORIENTATION_ROTATE_180_CCW = 2,

    /// @brief Flip vertical
    ADDON_IMG_ORIENTATION_FLIP_VERTICAL = 3,

    /// @brief Transpose
    ADDON_IMG_ORIENTATION_TRANSPOSE = 4,

    /// @brief Rotate 270° CCW
    ADDON_IMG_ORIENTATION_ROTATE_270_CCW = 5,

    /// @brief Transpose off axis
    ADDON_IMG_ORIENTATION_TRANSPOSE_OFF_AXIS = 6,

    /// @brief Rotate 90° CCW
    ADDON_IMG_ORIENTATION_ROTATE_90_CCW = 7,
  } ADDON_IMG_ORIENTATION;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_COLOR enum ADDON_IMG_COLOR
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Image color type**\n
  /// To set image as colored or black/white.
  ///
  ///@{
  typedef enum ADDON_IMG_COLOR
  {
    /// @brief Colored image
    ADDON_IMG_COLOR_COLORED,

    /// @brief Black/White image
    ADDON_IMG_COLOR_BLACK_WHITE
  } ADDON_IMG_COLOR;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_METERING_MODE enum ADDON_IMG_METERING_MODE
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Image metering mode**
  ///
  ///@{
  typedef enum ADDON_IMG_METERING_MODE
  {
    /// @brief 0 = Unknown
    ADDON_IMG_METERING_MODE_UNKNOWN = 0,

    /// @brief 1 = Average
    ADDON_IMG_METERING_MODE_AVERAGE = 1,

    /// @brief 2 = Center-weighted average
    ADDON_IMG_METERING_MODE_CENTER_WEIGHT = 2,

    /// @brief 3 = Spot
    ADDON_IMG_METERING_MODE_SPOT = 3,

    /// @brief 4 = MultiSpot
    ADDON_IMG_METERING_MODE_MULTI_SPOT = 4,

    /// @brief 5 = Pattern
    ADDON_IMG_METERING_MODE_MULTI_SEGMENT = 5,

    /// @brief 6 = Partial
    ADDON_IMG_METERING_MODE_PARTIAL = 6,

    /// @brief 255 = other
    ADDON_IMG_METERING_MODE_OTHER = 255,
  } ADDON_IMG_METERING_MODE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_EXPOSURE_PROGRAM enum ADDON_IMG_EXPOSURE_PROGRAM
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Exposure program**\n
  /// The class of the program used by the camera to set exposure when the picture is taken.
  ///
  ///@{
  typedef enum ADDON_IMG_EXPOSURE_PROGRAM
  {
    /// @brief 0 = Not Defined
    ADDON_IMG_EXPOSURE_PROGRAM_UNDEFINED = 0,

    /// @brief 1 = Manual
    ADDON_IMG_EXPOSURE_PROGRAM_MANUAL = 1,

    /// @brief 2 = Normal program
    ADDON_IMG_EXPOSURE_PROGRAM_NORMAL = 2,

    /// @brief 3 = Aperture-priority
    ADDON_IMG_EXPOSURE_PROGRAM_APERTURE_PRIORITY = 3,

    /// @brief 4 = Shutter speed priority
    ADDON_IMG_EXPOSURE_PROGRAM_SHUTTER_SPEED_PRIORITY = 4,

    /// @brief 5 = Creative program (biased toward depth of field)
    ADDON_IMG_EXPOSURE_PROGRAM_CREATIVE = 5,

    /// @brief 6 = Action program (biased toward fast shutter speed)
    ADDON_IMG_EXPOSURE_PROGRAM_ACTION = 6,

    /// @brief 7 = Portrait mode (for closeup photos with the background out of focus)
    ADDON_IMG_EXPOSURE_PROGRAM_PORTRAIT = 7,

    /// @brief 8 = Landscape mode (for landscape photos with the background in focus)
    ADDON_IMG_EXPOSURE_PROGRAM_LANDSCAPE = 8,

    /// @brief 9 = Bulb
    /// @note The value of 9 is not standard EXIF, but is used by the Canon EOS 7D
    ADDON_IMG_EXPOSURE_PROGRAM_BULB = 9
  } ADDON_IMG_EXPOSURE_PROGRAM;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_EXPOSURE_MODE enum ADDON_IMG_EXPOSURE_MODE
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Exposure mode**\n
  /// Indicates the exposure mode set when the image was shot.
  ///
  /// In auto-bracketing mode, the camera shoots a series of frames of the same
  /// scene at different exposure settings.
  ///
  ///@{
  typedef enum ADDON_IMG_EXPOSURE_MODE
  {
    /// @brief 0 = Auto exposure
    ADDON_IMG_EXPOSURE_MODE_AUTO = 0,

    /// @brief 1 = Manual exposure
    ADDON_IMG_EXPOSURE_MODE_MANUAL = 1,

    /// @brief 2 = Auto bracket
    ADDON_IMG_EXPOSURE_MODE_AUTO_TRACKED = 2,
  } ADDON_IMG_EXPOSURE_MODE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_LIGHT_SOURCE enum ADDON_IMG_LIGHT_SOURCE
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Kind of light source**
  ///
  ///@{
  typedef enum ADDON_IMG_LIGHT_SOURCE
  {
    /// @brief 0 = Unknown
    ADDON_IMG_LIGHT_SOURCE_UNKNOWN = 0,

    /// @brief 1 = Daylight
    ADDON_IMG_LIGHT_SOURCE_DAYLIGHT = 1,

    /// @brief 2 = Fluorescent
    ADDON_IMG_LIGHT_SOURCE_FLUORESCENT = 2,

    /// @brief 3 = Tungsten (incandescent light)
    ADDON_IMG_LIGHT_SOURCE_TUNGSTEN = 3,

    /// @brief 4 = Flash
    ADDON_IMG_LIGHT_SOURCE_FLASH = 4,

    /// @brief 9 = Fine weather
    ADDON_IMG_LIGHT_SOURCE_FINE_WEATHER = 9,

    /// @brief 10 = Cloudy weather
    ADDON_IMG_LIGHT_SOURCE_CLOUDY_WEATHER = 10,

    /// @brief 11 = Shade
    ADDON_IMG_LIGHT_SOURCE_SHADE = 11,

    /// @brief 12 = Daylight fluorescent (D 5700 - 7100K)
    ADDON_IMG_LIGHT_SOURCE_DAYLIGHT_FLUORESCENT = 12,

    /// @brief 13 = Day white fluorescent (N 4600 - 5400K)
    ADDON_IMG_LIGHT_SOURCE_DAY_WHITE_FLUORESCENT = 13,

    /// @brief 14 = Cool white fluorescent (W 3900 - 4500K)
    ADDON_IMG_LIGHT_SOURCE_COOL_WHITE_FLUORESCENT = 14,

    /// @brief 15 = White fluorescent (WW 3200 - 3700K)
    ADDON_IMG_LIGHT_SOURCE_WHITE_FLUORESCENT = 15,

    /// @brief 17 = Standard light A
    ADDON_IMG_LIGHT_SOURCE_STANDARD_LIGHT_A = 17,

    /// @brief 18 = Standard light B
    ADDON_IMG_LIGHT_SOURCE_STANDARD_LIGHT_B = 18,

    /// @brief 19 = Standard light C
    ADDON_IMG_LIGHT_SOURCE_STANDARD_LIGHT_C = 19,

    /// @brief 20 = D55
    ADDON_IMG_LIGHT_SOURCE_D55 = 20,

    /// @brief 21 = D65
    ADDON_IMG_LIGHT_SOURCE_D65 = 21,

    /// @brief 22 = D75
    ADDON_IMG_LIGHT_SOURCE_D75 = 22,

    /// @brief 23 = D50
    ADDON_IMG_LIGHT_SOURCE_D50 = 23,

    /// @brief 24 = ISO Studio Tungsten
    ADDON_IMG_LIGHT_SOURCE_ISO_STUDIO_TUNGSTEN = 24,

    /// @brief 255 = Other
    ADDON_IMG_LIGHT_SOURCE_OTHER = 255,
  } ADDON_IMG_LIGHT_SOURCE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_imagedecoder_Defs_ADDON_IMG_FLASH_TYPE enum ADDON_IMG_FLASH_TYPE
  /// @ingroup cpp_kodi_addon_imagedecoder_Defs
  /// @brief **Flash Values**
  ///
  ///@{
  typedef enum ADDON_IMG_FLASH_TYPE
  {
    /// @brief 0x0 = No Flash
    ADDON_IMG_FLASH_TYPE_NO_FLASH = 0x0,

    /// @brief 0x1 = Fired
    ADDON_IMG_FLASH_TYPE_FIRED = 0x1,

    /// @brief 0x5 = Fired, Return not detected
    ADDON_IMG_FLASH_TYPE_FIRED_RETURN_NOT_DETECTED = 0x5,

    /// @brief 0x7 = Fired, Return detected
    ADDON_IMG_FLASH_TYPE_FIRED_RETURN_DETECTED = 0x7,

    /// @brief 0x8 = On, Did not fire
    ADDON_IMG_FLASH_TYPE_ = 0x8,

    /// @brief 0x9 = On, Fired
    ADDON_IMG_FLASH_TYPE_ON_Fired = 0x9,

    /// @brief 0xd = On, Return not detected
    ADDON_IMG_FLASH_TYPE_ON_RETURN_NOT_DETECTED = 0xd,

    /// @brief 0xf = On, Return detected
    ADDON_IMG_FLASH_TYPE_ON_RETURN_DETECTED = 0xf,

    /// @brief 0x10 = Off, Did not fire
    ADDON_IMG_FLASH_TYPE_OFF_DID_NOT_FIRE = 0x10,

    /// @brief 0x14 = Off, Did not fire, Return not detected
    ADDON_IMG_FLASH_TYPE_OFF_DID_NOT_FIRE_RETURN_NOT_DETECTED = 0x14,

    /// @brief 0x18 = Auto, Did not fire
    ADDON_IMG_FLASH_TYPE_AUTO_DID_NOT_FIRE = 0x18,

    /// @brief 0x19 = Auto, Fired
    ADDON_IMG_FLASH_TYPE_AUTO_FIRED = 0x19,

    /// @brief 0x1d = Auto, Fired, Return not detected
    ADDON_IMG_FLASH_TYPE_AUTO_FIRED_RETURN_NOT_DETECTED = 0x1d,

    /// @brief 0x1f = Auto, Fired, Return detected
    ADDON_IMG_FLASH_TYPE_AUTO_FIRED_RETURN_DETECTED = 0x1f,

    /// @brief 0x20 = No flash function
    ADDON_IMG_FLASH_TYPE_NO_FLASH_FUNCTION = 0x20,

    /// @brief 0x30 = Off, No flash function
    ADDON_IMG_FLASH_TYPE_OFF_NO_FLASH_FUNCTION = 0x30,

    /// @brief 0x41 = Fired, Red-eye reduction
    ADDON_IMG_FLASH_TYPE_FIRED_REDEYE_REDUCTION = 0x41,

    /// @brief 0x45 = Fired, Red-eye reduction, Return not detected
    ADDON_IMG_FLASH_TYPE_FIRED_REDEYE_REDUCTION_RETURN_NOT_DETECTED = 0x45,

    /// @brief 0x47 = Fired, Red-eye reduction, Return detected
    ADDON_IMG_FLASH_TYPE_FIRED_REDEYE_REDUCTION_RETURN_DETECTED = 0x47,

    /// @brief 0x49 = On, Red-eye reduction
    ADDON_IMG_FLASH_TYPE_ON_REDEYE_REDUCTION = 0x49,

    /// @brief 0x4d = On, Red-eye reduction, Return not detected
    ADDON_IMG_FLASH_TYPE_ON_REDEYE_REDUCTION_RETURN_NOT_DETECTED = 0x4d,

    /// @brief 0x4f = On, Red-eye reduction, Return detected
    ADDON_IMG_FLASH_TYPE_ON_REDEYE_REDUCTION_RETURN_DETECTED = 0x4f,

    /// @brief 0x50 = Off, Red-eye reduction
    ADDON_IMG_FLASH_TYPE_OFF_REDEYE_REDUCTION = 0x50,

    /// @brief 0x58 = Auto, Did not fire, Red-eye reduction
    ADDON_IMG_FLASH_TYPE_AUTO_DID_NOT_FIRE_REDEYE_REDUCTION = 0x58,

    /// @brief 0x59 = Auto, Fired, Red-eye reduction
    ADDON_IMG_FLASH_TYPE_AUTO_FIRED_REDEYE_REDUCTION = 0x59,

    /// @brief 0x5d = Auto, Fired, Red-eye reduction, Return not detected
    ADDON_IMG_FLASH_TYPE_AUTO_FIRED_REDEYE_REDUCTION_RETURN_NOT_DETECTED = 0x5d,

    /// @brief 0x5f = Auto, Fired, Red-eye reduction, Return detected
    ADDON_IMG_FLASH_TYPE_AUTO_FIRED_REDEYE_REDUCTION_RETURN_DETECTED = 0x5f,
  } ADDON_IMG_FLASH_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  struct KODI_ADDON_IMAGEDECODER_INFO_TAG
  {
    int width;
    int height;
    float distance;
    enum ADDON_IMG_ORIENTATION orientation;
    enum ADDON_IMG_COLOR color;
    enum ADDON_IMG_METERING_MODE metering_mode;
    float exposure_time;
    float exposure_bias;
    enum ADDON_IMG_EXPOSURE_PROGRAM exposure_program;
    enum ADDON_IMG_EXPOSURE_MODE exposure_mode;
    time_t time_created;
    float aperture_f_number;
    enum ADDON_IMG_FLASH_TYPE flash_used;
    int focal_length;
    int focal_length_in_35mm_format;
    float digital_zoom_ratio;
    float iso_speed;
    enum ADDON_IMG_LIGHT_SOURCE light_source;

    bool gps_info_present;
    char latitude_ref;
    float latitude[3]; /* Deg,min,sec */
    char longitude_ref;
    float longitude[3]; /* Deg,min,sec */
    int altitude_ref;
    float altitude;

    char* camera_manufacturer;
    char* camera_model;
    char* author;
    char* description;
    char* copyright;
  };

  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_IMAGEDECODER_SUPPORTS_FILE_V1)(
      const KODI_ADDON_IMAGEDECODER_HDL hdl, const char* file);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_IMAGEDECODER_READ_TAG_V1)(
      const KODI_ADDON_IMAGEDECODER_HDL hdl,
      const char* file,
      struct KODI_ADDON_IMAGEDECODER_INFO_TAG* info);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_IMAGEDECODER_LOAD_IMAGE_FROM_MEMORY_V1)(
      const KODI_ADDON_IMAGEDECODER_HDL hdl,
      const char* mimetype,
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
    PFN_KODI_ADDON_IMAGEDECODER_READ_TAG_V1 read_tag;
    PFN_KODI_ADDON_IMAGEDECODER_LOAD_IMAGE_FROM_MEMORY_V1 load_image_from_memory;
    PFN_KODI_ADDON_IMAGEDECODER_DECODE_V1 decode;
  } KodiToAddonFuncTable_ImageDecoder;

  typedef struct AddonToKodiFuncTable_ImageDecoder
  {
    KODI_HANDLE kodi_instance;
  } AddonToKodiFuncTable_ImageDecoder;

  typedef struct AddonInstance_ImageDecoder
  {
    struct AddonToKodiFuncTable_ImageDecoder* toKodi;
    struct KodiToAddonFuncTable_ImageDecoder* toAddon;
  } AddonInstance_ImageDecoder;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_IMAGEDECODER_H */
