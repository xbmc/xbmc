/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/inputstream.h"
#include "inputstream/StreamCodec.h"
#include "inputstream/StreamConstants.h"
#include "inputstream/StreamCrypto.h"
#include "inputstream/TimingConstants.h"

#ifdef __cplusplus

#include <map>

namespace kodi
{
namespace addon
{

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Doxygen group set for the definitions
//{{{

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_inputstream
/// @brief **Inputstream add-on instance definition values**\n
/// All inputstream functions associated data structures.
///
/// Used to exchange the available options between Kodi and addon.\n
/// The groups described here correspond to the groups of functions on inputstream
/// instance class.
///
/// In addition, some of the things described here are also used on the
/// @ref cpp_kodi_addon_videocodec "video codec" addon.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_inputstream_Defs_Interface 1. Interface
/// @ingroup cpp_kodi_addon_inputstream_Defs
/// @brief **Inputstream add-on general variables**\n
/// Used to exchange the available options between Kodi and addon.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_inputstream_Defs_StreamConstants 2. Stream constants
/// @ingroup cpp_kodi_addon_inputstream_Defs
/// @brief **Used to exchange any additional data between the caller and processor.**\n
/// This includes the standardized values, in addition, an addon can also use
/// its own special uses to be exchanged in the same way.
///
/// These values can be used by other addons (e.g. video addon) or stream files
/// to select the respective inputstream addon and to transfer additional values.
///
/// ----------------------------------------------------------------------------
///
/// **Example:**
///
/// This example use the @ref STREAM_PROPERTY_INPUTSTREAM on a <b>`*.strm`</b>
/// file to select needed addon and a additional value where related to selected
/// addon itself.
///
/// ~~~~~~~~~~~~
/// #KODIPROP:inputstream=inputstream.adaptive
/// #KODIPROP:inputstream.adaptive.manifest_type=mpd
/// http://rdmedia.bbc.co.uk/dash/ondemand/testcard/1/client_manifest-events-multilang.mpd
/// ~~~~~~~~~~~~
///
/// These are then given to Kodi and the respectively selected addon by means of
/// his @ref kodi::addon::CInstanceInputStream::Open "Open" call
/// in @ref kodi::addon::InputstreamProperty::GetProperties.
///
/// The largest possible amount of these <b>`#KODIPROP`</b> values is defined
/// with @ref STREAM_MAX_PROPERTY_COUNT.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_inputstream_Defs_TimingConstants 3. Stream timing
/// @ingroup cpp_kodi_addon_inputstream_Defs
/// @brief **Timebase and timestamp definitions.**\n
/// Used to exchange the available options between Kodi and addon.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_inputstream_Defs_StreamCodec 4. Stream codec
/// @ingroup cpp_kodi_addon_inputstream_Defs
/// @brief **Inputstream codec control**\n
/// Used to manage stream codec data.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_inputstream_Defs_StreamEncryption 5. Stream encryption
/// @ingroup cpp_kodi_addon_inputstream_Defs
/// @brief **Inputstream encryption control**\n
/// Used to manage encrypted streams within addon.
///

//}}}
//______________________________________________________________________________

class CInstanceInputStream;

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs_InputstreamProperty class InputstreamProperty
/// @ingroup cpp_kodi_addon_inputstream_Defs_Interface
/// @brief <b>URL and Data of key/value pairs passed to addon on @ref kodi::addon::CInstanceInputStream::Open "Open".</b>\n
/// This is used to have the necessary data of the stream to be opened.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_inputstream_Defs_InputstreamProperty_Help
///
/// @warning This data are only given from Kodi to addon and can't be used
/// on other places on addon.
///
///@{
class ATTR_DLL_LOCAL InputstreamProperty
  : public CStructHdl<InputstreamProperty, INPUTSTREAM_PROPERTY>
{
  /*! \cond PRIVATE */
  friend class CInstanceInputStream;
  /*! \endcond */

public:
  /// @defgroup cpp_kodi_addon_inputstream_Defs_InputstreamProperty_Help Value Help
  /// @ingroup cpp_kodi_addon_inputstream_Defs_InputstreamProperty
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_inputstream_Defs_InputstreamProperty :</b>
  /// | Name | Type | Get call
  /// |------|------|----------
  /// | **Stream URL** | `std::string` | @ref InputstreamProperty::GetURL "GetURL"
  /// | **Mime type** | `std::string` | @ref InputstreamProperty::GetMimeType "GetMimeType"
  /// | **Available amount of properties** | `unsigned int` | @ref InputstreamProperty::GetPropertiesAmount "GetPropertiesAmount"
  /// | **List of properties** | `std::map<std::string, std::string>` | @ref InputstreamProperty::GetProperties "GetProperties"
  /// | **Get addon library folder** | `std::string` | @ref InputstreamProperty::GetLibFolder "GetLibFolder"
  /// | **Get addon profile/user folder** | `std::string` | @ref InputstreamProperty::GetProfileFolder "GetProfileFolder"

  /// @addtogroup cpp_kodi_addon_inputstream_Defs_InputstreamProperty
  ///@{

  /// @brief Stream URL to open.
  std::string GetURL() const { return m_cStructure->m_strURL; }

  /// @brief Stream mime type.
  std::string GetMimeType() const { return m_cStructure->m_mimeType; }

  /// @brief Amount of available properties.
  unsigned int GetPropertiesAmount() const { return m_cStructure->m_nCountInfoValues; }

  /// @brief List of available properties-
  const std::map<std::string, std::string> GetProperties() const
  {
    std::map<std::string, std::string> props;
    for (unsigned int i = 0; i < m_cStructure->m_nCountInfoValues; ++i)
    {
      props.emplace(m_cStructure->m_ListItemProperties[i].m_strKey,
                    m_cStructure->m_ListItemProperties[i].m_strValue);
    }
    return props;
  }

  /// @brief Get addon library folder.
  ///
  /// @note As alternative can also @ref kodi::GetAddonPath used.
  std::string GetLibFolder() const { return m_cStructure->m_libFolder; }

  /// @brief Get addon profile/user folder.
  ///
  /// @note As alternative can also @ref kodi::GetBaseUserPath used.
  std::string GetProfileFolder() const { return m_cStructure->m_profileFolder; }

  ///@}

private:
  InputstreamProperty() = delete;
  InputstreamProperty(const InputstreamProperty& stream) = delete;
  InputstreamProperty(const INPUTSTREAM_PROPERTY* stream) : CStructHdl(stream) {}
  InputstreamProperty(INPUTSTREAM_PROPERTY* stream) : CStructHdl(stream) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamCapabilities class InputstreamCapabilities
/// @ingroup cpp_kodi_addon_inputstream_Defs_Interface
/// @brief **InputStream add-on capabilities. All capabilities are set to "false" as default.**\n
/// Asked to addon on @ref kodi::addon::CInstanceInputStream::GetCapabilities "GetCapabilities".
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_inputstream_Defs_Interface_InputstreamCapabilities_Help
///
///@{
class ATTR_DLL_LOCAL InputstreamCapabilities
  : public CStructHdl<InputstreamCapabilities, INPUTSTREAM_CAPABILITIES>
{
  /*! \cond PRIVATE */
  friend class CInstanceInputStream;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  InputstreamCapabilities() = default;
  InputstreamCapabilities(const InputstreamCapabilities& stream) : CStructHdl(stream) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamCapabilities_Help Value Help
  /// @ingroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamCapabilities
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamCapabilities :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Capabilities bit mask** | `uint32_t` | @ref InputstreamCapabilities::SetMask "SetMask" | @ref InputstreamCapabilities::GetMask "GetMask"

  /// @addtogroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamCapabilities
  ///@{

  /// @brief Set of supported capabilities.
  void SetMask(uint32_t mask) const { m_cStructure->m_mask = mask; }

  /// @brief Get of supported capabilities.
  uint32_t GetMask() const { return m_cStructure->m_mask; }

  ///@}

private:
  InputstreamCapabilities(const INPUTSTREAM_CAPABILITIES* stream) : CStructHdl(stream) {}
  InputstreamCapabilities(INPUTSTREAM_CAPABILITIES* stream) : CStructHdl(stream) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata class InputstreamMasteringMetadata
/// @ingroup cpp_kodi_addon_inputstream_Defs_Interface
/// @brief **Mastering metadata.**\n
/// Describes the metadata for [HDR10](https://en.wikipedia.org/wiki/High-dynamic-range_video).
///
/// Used when video is compressed using [High Efficiency Video Coding (HEVC)](https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding).
/// This is used to describe the capabilities  of the display used to master the
/// content and the luminance values of the content.
///
/// Used on @ref kodi::addon::InputstreamInfo::SetMasteringMetadata and @ref kodi::addon::InputstreamInfo::GetMasteringMetadata.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata_Help
///
///@{
class ATTR_DLL_LOCAL InputstreamMasteringMetadata
  : public CStructHdl<InputstreamMasteringMetadata, INPUTSTREAM_MASTERING_METADATA>
{
  /*! \cond PRIVATE */
  friend class CInstanceInputStream;
  friend class InputstreamInfo;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  InputstreamMasteringMetadata() = default;
  InputstreamMasteringMetadata(const InputstreamMasteringMetadata& stream) : CStructHdl(stream) {}
  InputstreamMasteringMetadata& operator=(const InputstreamMasteringMetadata&) = default;

  /*! \endcond */

  /// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata_Help Value Help
  /// @ingroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Chromaticity X coordinates of the red** | `double` | @ref InputstreamMasteringMetadata::SetPrimaryR_ChromaticityX "SetPrimaryR_ChromaticityX" | @ref InputstreamMasteringMetadata::GetPrimaryR_ChromaticityX "GetPrimaryR_ChromaticityX"
  /// | **Chromaticity Y coordinates of the red** | `double` | @ref InputstreamMasteringMetadata::SetPrimaryR_ChromaticityY "SetPrimaryR_ChromaticityY" | @ref InputstreamMasteringMetadata::GetPrimaryR_ChromaticityY "GetPrimaryR_ChromaticityY"
  /// | **Chromaticity X coordinates of the green** | `double` | @ref InputstreamMasteringMetadata::SetPrimaryG_ChromaticityX "SetPrimaryG_ChromaticityX" | @ref InputstreamMasteringMetadata::GetPrimaryG_ChromaticityX "GetPrimaryG_ChromaticityX"
  /// | **Chromaticity Y coordinates of the green** | `double` | @ref InputstreamMasteringMetadata::SetPrimaryG_ChromaticityY "SetPrimaryG_ChromaticityY" | @ref InputstreamMasteringMetadata::GetPrimaryG_ChromaticityY "GetPrimaryG_ChromaticityY"
  /// | **Chromaticity X coordinates of the blue** | `double` | @ref InputstreamMasteringMetadata::SetPrimaryB_ChromaticityX "SetPrimaryB_ChromaticityX" | @ref InputstreamMasteringMetadata::GetPrimaryB_ChromaticityX "GetPrimaryB_ChromaticityX"
  /// | **Chromaticity Y coordinates of the blue** | `double` | @ref InputstreamMasteringMetadata::SetPrimaryB_ChromaticityY "SetPrimaryB_ChromaticityY" | @ref InputstreamMasteringMetadata::GetPrimaryB_ChromaticityY "GetPrimaryB_ChromaticityY"
  /// | **Chromaticity X coordinates of the white point** | `double` | @ref InputstreamMasteringMetadata::SetWhitePoint_ChromaticityX "SetWhitePoint_ChromaticityX" | @ref InputstreamMasteringMetadata::GetWhitePoint_ChromaticityX "GetWhitePoint_ChromaticityX"
  /// | **Chromaticity Y coordinates of the white point** | `double` | @ref InputstreamMasteringMetadata::SetWhitePoint_ChromaticityY "SetWhitePoint_ChromaticityY" | @ref InputstreamMasteringMetadata::GetWhitePoint_ChromaticityY "GetWhitePoint_ChromaticityY"
  /// | **Maximum number of bits of the display** | `double` | @ref InputstreamMasteringMetadata::SetLuminanceMax "SetLuminanceMax" | @ref InputstreamMasteringMetadata::GetLuminanceMax "GetLuminanceMax"
  /// | **Minimum number of bits of the display** | `double` | @ref InputstreamMasteringMetadata::SetLuminanceMin "SetLuminanceMin" | @ref InputstreamMasteringMetadata::GetLuminanceMin "GetLuminanceMin"

  /// @addtogroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata
  ///@{

  /// @brief Metadata class compare.
  ///
  /// To compare the metadata with another one.
  ///
  /// @return true if they equal, false otherwise
  bool operator==(const kodi::addon::InputstreamMasteringMetadata& right) const
  {
    if (memcmp(m_cStructure, right.m_cStructure, sizeof(INPUTSTREAM_MASTERING_METADATA)) == 0)
      return true;
    return false;
  }

  /// @brief Set the chromaticity coordinates of the red value in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// X coordinate. The values are normalized to 50,000.
  void SetPrimaryR_ChromaticityX(double value) { m_cStructure->primary_r_chromaticity_x = value; }

  /// @brief Get the chromaticity X coordinates of the red value.
  double GetPrimaryR_ChromaticityX() { return m_cStructure->primary_r_chromaticity_x; }

  /// @brief The chromaticity coordinates of the red value in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// Y coordinate. The values are normalized to 50,000.
  void SetPrimaryR_ChromaticityY(double value) { m_cStructure->primary_r_chromaticity_y = value; }

  /// @brief Get the chromaticity Y coordinates of the red value.
  double GetPrimaryR_ChromaticityY() { return m_cStructure->primary_r_chromaticity_y; }

  /// @brief Set the chromaticity coordinates of the green value in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// X coordinate. The values are normalized to 50,000.
  void SetPrimaryG_ChromaticityX(double value) { m_cStructure->primary_g_chromaticity_x = value; }

  /// @brief Get the chromaticity X coordinates of the green value.
  double GetPrimaryG_ChromaticityX() { return m_cStructure->primary_g_chromaticity_x; }

  /// @brief Set the chromaticity coordinates of the green value in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// Y coordinate. The values are normalized to 50,000.
  void SetPrimaryG_ChromaticityY(double value) { m_cStructure->primary_g_chromaticity_y = value; }

  /// @brief Get the chromaticity Y coordinates of the green value.
  double GetPrimaryG_ChromaticityY() { return m_cStructure->primary_g_chromaticity_y; }

  /// @brief The chromaticity coordinates of the blue value in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// X coordinate. The values are normalized to 50,000.
  void SetPrimaryB_ChromaticityX(double value) { m_cStructure->primary_b_chromaticity_x = value; }

  /// @brief Get the chromaticity X coordinates of the blue value.
  double GetPrimaryB_ChromaticityX() { return m_cStructure->primary_b_chromaticity_x; }

  /// @brief The chromaticity coordinates of the blue value in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// Y coordinate. The values are normalized to 50,000.
  void SetPrimaryB_ChromaticityY(double value) { m_cStructure->primary_b_chromaticity_y = value; }

  /// @brief Get the chromaticity Y coordinates of the blue value.
  double GetPrimaryB_ChromaticityY() { return m_cStructure->primary_b_chromaticity_y; }

  /// @brief Set the chromaticity coordinates of the white point in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// X coordinate. The values are normalized to 50,000.
  void SetWhitePoint_ChromaticityX(double value)
  {
    m_cStructure->white_point_chromaticity_x = value;
  }

  /// @brief Get the chromaticity X coordinates of the white point
  double GetWhitePoint_ChromaticityX() { return m_cStructure->white_point_chromaticity_x; }

  /// @brief Set the chromaticity coordinates of the white point in the
  /// [CIE1931](https://en.wikipedia.org/wiki/CIE_1931_color_space) color space.
  ///
  /// Y coordinate. The values are normalized to 50,000.
  void SetWhitePoint_ChromaticityY(double value)
  {
    m_cStructure->white_point_chromaticity_y = value;
  }

  /// @brief Get the chromaticity Y coordinates of the white point.
  double GetWhitePoint_ChromaticityY() { return m_cStructure->white_point_chromaticity_y; }

  /// @brief Set the maximum number of bits of the display used to master the content.
  ///
  /// Values are normalized to 10,000.
  void SetLuminanceMax(double value) { m_cStructure->luminance_max = value; }

  /// @brief Get the maximum number of bits of the display.
  double GetLuminanceMax() { return m_cStructure->luminance_max; }

  /// @brief Set the minimum number of bits of the display used to master the content.
  ///
  /// Values are normalized to 10,000.
  void SetLuminanceMin(double value) { m_cStructure->luminance_min = value; }

  /// @brief Get the minimum number of bits of the display.
  double GetLuminanceMin() { return m_cStructure->luminance_min; }

  ///@}

private:
  InputstreamMasteringMetadata(const INPUTSTREAM_MASTERING_METADATA* stream) : CStructHdl(stream) {}
  InputstreamMasteringMetadata(INPUTSTREAM_MASTERING_METADATA* stream) : CStructHdl(stream) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamContentlightMetadata class InputstreamContentlightMetadata
/// @ingroup cpp_kodi_addon_inputstream_Defs_Interface
/// @brief **Contentlight metadata**\n
/// Describes the metadata for [HDR10](https://en.wikipedia.org/wiki/High-dynamic-range_video).
/// See also @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata "InputstreamMasteringMetadata".
///
/// Used on @ref kodi::addon::InputstreamInfo::SetContentLightMetadata and @ref kodi::addon::InputstreamInfo::GetContentLightMetadata.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_inputstream_Defs_Interface_InputstreamContentlightMetadata_Help
///
///@{
class ATTR_DLL_LOCAL InputstreamContentlightMetadata
  : public CStructHdl<InputstreamContentlightMetadata, INPUTSTREAM_CONTENTLIGHT_METADATA>
{
  /*! \cond PRIVATE */
  friend class CInstanceInputStream;
  friend class InputstreamInfo;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  InputstreamContentlightMetadata() = default;
  InputstreamContentlightMetadata(const InputstreamContentlightMetadata& stream)
    : CStructHdl(stream)
  {
  }
  InputstreamContentlightMetadata& operator=(const InputstreamContentlightMetadata&) = default;
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamContentlightMetadata_Help Value Help
  /// @ingroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamContentlightMetadata
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamContentlightMetadata :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Maximum content light level** | `double` | @ref InputstreamContentlightMetadata::SetMaxCll "SetMaxCll" | @ref InputstreamContentlightMetadata::GetMaxCll "GetMaxCll"
  /// | **Maximum frame average light level** | `double` | @ref InputstreamContentlightMetadata::SetMaxFall "SetMaxFall" | @ref InputstreamContentlightMetadata::GetMaxFall "GetMaxFall"

  /// @addtogroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamContentlightMetadata
  ///@{

  /// @brief Metadata class compare.
  ///
  /// To compare the metadata with another one.
  ///
  /// @return true if they equal, false otherwise
  bool operator==(const kodi::addon::InputstreamContentlightMetadata& right) const
  {
    if (memcmp(m_cStructure, right.m_cStructure, sizeof(INPUTSTREAM_CONTENTLIGHT_METADATA)) == 0)
      return true;
    return false;
  }

  /// @brief Set the maximum content light level (MaxCLL).
  ///
  /// This is the bit value corresponding to the brightest pixel used anywhere
  /// in the content.
  void SetMaxCll(uint64_t value) { m_cStructure->max_cll = value; }

  /// @brief Get the maximum content light level (MaxCLL).
  uint64_t GetMaxCll() { return m_cStructure->max_cll; }

  /// @brief Set the maximum frame average light level (MaxFALL).
  ///
  /// This is the bit value corresponding to the average luminance of the frame
  /// which has the brightest average luminance anywhere in the content.
  void SetMaxFall(uint64_t value) { m_cStructure->max_fall = value; }

  /// @brief Get the maximum frame average light level (MaxFALL).
  uint64_t GetMaxFall() { return m_cStructure->max_fall; }

  ///@}

private:
  InputstreamContentlightMetadata(const INPUTSTREAM_CONTENTLIGHT_METADATA* stream)
    : CStructHdl(stream)
  {
  }
  InputstreamContentlightMetadata(INPUTSTREAM_CONTENTLIGHT_METADATA* stream) : CStructHdl(stream) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo class InputstreamInfo
/// @ingroup cpp_kodi_addon_inputstream_Defs_Interface
/// @brief **Inputstream add-on stream info**\n
/// This is used to give Kodi the associated and necessary data for an open stream.
///
/// Used on @ref kodi::addon::CInstanceInputStream::GetStream().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo_Help
///
///@{
class ATTR_DLL_LOCAL InputstreamInfo : public CStructHdl<InputstreamInfo, INPUTSTREAM_INFO>
{
  /*! \cond PRIVATE */
  friend class CInstanceInputStream;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  InputstreamInfo() = default;
  InputstreamInfo(const InputstreamInfo& stream) : CStructHdl(stream)
  {
    SetCryptoSession(stream.GetCryptoSession());
    CopyExtraData();
  }
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo_Help Value Help
  /// @ingroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo :</b>
  /// | Name | Type used | Required | Set call | Get call
  /// |------|-----------|----------|----------|---------
  /// | **Stream type** | all | yes | @ref InputstreamInfo::SetStreamType "SetStreamType" | @ref InputstreamInfo::GetStreamType "GetStreamType"
  /// | **Feature flags** | all | yes | @ref InputstreamInfo::SetFeatures "SetFeatures" | @ref InputstreamInfo::GetFeatures "GetFeatures"
  /// | **Flags** | all | yes | @ref InputstreamInfo::SetFlags "SetFlags" | @ref InputstreamInfo::GetFlags "GetFlags"
  /// | **Name** | all | no | @ref InputstreamInfo::SetName "SetName" | @ref InputstreamInfo::GetName "GetName"
  /// | **Codec name** | all | yes | @ref InputstreamInfo::SetCodecName "SetCodecName" | @ref InputstreamInfo::GetCodecName "GetCodecName"
  /// | **Codec internal name** | all | no | @ref InputstreamInfo::SetCodecInternalName "SetCodecInternalName" | @ref InputstreamInfo::GetCodecInternalName "GetCodecInternalName"
  /// | **Codec Profile** | all | no | @ref InputstreamInfo::SetCodecProfile "SetCodecProfile" | @ref InputstreamInfo::GetCodecProfile "GetCodecProfile"
  /// | **Physical index** | all | yes | @ref InputstreamInfo::SetPhysicalIndex "SetPhysicalIndex" | @ref InputstreamInfo::GetPhysicalIndex "GetPhysicalIndex"
  /// | **Extra data** | Subtitle / all | Type related required | @ref InputstreamInfo::SetExtraData "SetExtraData" | @ref InputstreamInfo::GetExtraData "GetExtraData"
  /// | **RFC 5646 language code** | all | no | @ref InputstreamInfo::SetLanguage "SetLanguage" | @ref InputstreamInfo::GetLanguage "GetLanguage"
  /// | **FPS scale** | Video | Type related required | @ref InputstreamInfo::SetFpsScale "SetFpsScale" | @ref InputstreamInfo::GetFpsScale "GetFpsScale"
  /// | **FPS rate** | Video | Type related required | @ref InputstreamInfo::SetFpsRate "SetFpsRate" | @ref InputstreamInfo::GetFpsRate "GetFpsRate"
  /// | **Height** | Video | Type related required | @ref InputstreamInfo::SetHeight "SetHeight" | @ref InputstreamInfo::GetHeight "GetHeight"
  /// | **Width** | Video | Type related required | @ref InputstreamInfo::SetWidth "SetWidth" | @ref InputstreamInfo::GetWidth "GetWidth"
  /// | **Aspect** | Video | Type related required | @ref InputstreamInfo::SetAspect "SetAspect" | @ref InputstreamInfo::GetAspect "GetAspect"
  /// | **Channel quantity** | Audio | Type related required | @ref InputstreamInfo::SetChannels "SetChannels" | @ref InputstreamInfo::GetChannels "GetChannels"
  /// | **Sample rate** | Audio | Type related required | @ref InputstreamInfo::SetSampleRate "SetSampleRate" | @ref InputstreamInfo::GetSampleRate "GetSampleRate"
  /// | **Bit rate** | Audio | Type related required | @ref InputstreamInfo::SetBitRate "SetBitRate" | @ref InputstreamInfo::GetBitRate "GetBitRate"
  /// | **Bits per sample** | Audio | Type related required | @ref InputstreamInfo::SetBitsPerSample "SetBitsPerSample" | @ref InputstreamInfo::GetBitsPerSample "GetBitsPerSample"
  /// | **Block align** |  | no | @ref InputstreamInfo::SetBlockAlign "SetBlockAlign" | @ref InputstreamInfo::GetBlockAlign "GetBlockAlign"
  /// | **Crypto session info** |  | no | @ref InputstreamInfo::SetCryptoSession "SetCryptoSession" | @ref InputstreamInfo::GetCryptoSession "GetCryptoSession"
  /// | **Four CC code** |  | no | @ref InputstreamInfo::SetCodecFourCC "SetCodecFourCC" | @ref InputstreamInfo::GetCodecFourCC "GetCodecFourCC"
  /// | **Color space** |  | no | @ref InputstreamInfo::SetColorSpace "SetColorSpace" | @ref InputstreamInfo::GetColorSpace "GetColorSpace"
  /// | **Color range** |  | no | @ref InputstreamInfo::SetColorRange "SetColorRange" | @ref InputstreamInfo::GetColorRange "GetColorRange"
  /// | **Color primaries** |  | no | @ref InputstreamInfo::SetColorPrimaries "SetColorPrimaries" | @ref InputstreamInfo::GetColorPrimaries "GetColorPrimaries"
  /// | **Color transfer characteristic** |  | no | @ref InputstreamInfo::SetColorTransferCharacteristic "SetColorTransferCharacteristic" | @ref InputstreamInfo::GetColorTransferCharacteristic "GetColorTransferCharacteristic"
  /// | **Mastering metadata** |  | no | @ref InputstreamInfo::SetMasteringMetadata "SetMasteringMetadata" | @ref InputstreamInfo::GetMasteringMetadata "GetMasteringMetadata"
  /// | **Content light metadata** |  | no | @ref InputstreamInfo::SetContentLightMetadata "SetContentLightMetadata" | @ref InputstreamInfo::GetContentLightMetadata "GetContentLightMetadata"
  ///

  /// @addtogroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo
  ///@{

  /// @brief Set the wanted stream type.
  ///
  /// @param[in] streamType By @ref INPUTSTREAM_TYPE defined type
  void SetStreamType(INPUTSTREAM_TYPE streamType) { m_cStructure->m_streamType = streamType; }

  /// @brief To get with @ref SetStreamType changed values.
  INPUTSTREAM_TYPE GetStreamType() const { return m_cStructure->m_streamType; }

  /// @brief Set special supported feature flags of inputstream.
  ///
  /// @param[in] features By @ref INPUTSTREAM_CODEC_FEATURES defined type
  void SetFeatures(uint32_t features) { m_cStructure->m_features = features; }

  /// @brief To get with @ref SetFeatures changed values.
  uint32_t GetFeatures() const { return m_cStructure->m_features; }

  /// @brief Set supported flags of inputstream.
  ///
  /// @param[in] flags The on @ref INPUTSTREAM_FLAGS defined flags to set
  void SetFlags(uint32_t flags) { m_cStructure->m_flags = flags; }

  /// @brief To get with @ref SetFeatures changed values.
  uint32_t GetFlags() const { return m_cStructure->m_flags; }

  /// @brief (optional) Name of the stream, leave empty for default handling.
  ///
  /// @param[in] name Stream name
  void SetName(const std::string& name)
  {
    strncpy(m_cStructure->m_name, name.c_str(), INPUTSTREAM_MAX_STRING_NAME_SIZE);
  }

  /// @brief To get with @ref SetName changed values.
  std::string GetName() const { return m_cStructure->m_name; }

  /// @brief (required) Name of codec according to ffmpeg.
  ///
  /// See https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/codec_desc.c about
  /// available names.
  ///
  /// @remark On @ref INPUTSTREAM_TYPE_TELETEXT, @ref INPUTSTREAM_TYPE_RDS, and
  /// @ref INPUTSTREAM_TYPE_ID3 this can be ignored and leaved empty.
  ///
  /// @param[in] codeName Codec name
  void SetCodecName(const std::string& codecName)
  {
    strncpy(m_cStructure->m_codecName, codecName.c_str(), INPUTSTREAM_MAX_STRING_CODEC_SIZE);
  }

  /// @brief To get with @ref SetCodecName changed values.
  std::string GetCodecName() const { return m_cStructure->m_codecName; }

  /// @brief (optional) Internal name of codec (selectionstream info).
  ///
  /// @param[in] codecName Internal codec name
  void SetCodecInternalName(const std::string& codecName)
  {
    strncpy(m_cStructure->m_codecInternalName, codecName.c_str(),
            INPUTSTREAM_MAX_STRING_CODEC_SIZE);
  }

  /// @brief To get with @ref SetCodecInternalName changed values.
  std::string GetCodecInternalName() const { return m_cStructure->m_codecInternalName; }

  /// @brief (optional) The profile of the codec.
  ///
  /// @param[in] codecProfile Values with @ref STREAMCODEC_PROFILE to use
  void SetCodecProfile(STREAMCODEC_PROFILE codecProfile)
  {
    m_cStructure->m_codecProfile = codecProfile;
  }

  /// @brief To get with @ref SetCodecProfile changed values.
  STREAMCODEC_PROFILE GetCodecProfile() const { return m_cStructure->m_codecProfile; }

  /// @brief (required) Physical index.
  ///
  /// @param[in] id Index identifier
  void SetPhysicalIndex(unsigned int id) { m_cStructure->m_pID = id; }

  /// @brief To get with @ref SetPhysicalIndex changed values.
  unsigned int GetPhysicalIndex() const { return m_cStructure->m_pID; }

  /// @brief Additional data where can needed on streams.
  ///
  /// @param[in] extraData List with memory of extra data
  void SetExtraData(const std::vector<uint8_t>& extraData)
  {
    m_extraData = extraData;
    m_cStructure->m_ExtraData = m_extraData.data();
    m_cStructure->m_ExtraSize = static_cast<unsigned int>(m_extraData.size());
  }

  /// @brief Additional data where can needed on streams.
  ///
  /// @param[in] extraData Pointer with memory of extra data
  /// @param[in] extraSize Size to store
  void SetExtraData(const uint8_t* extraData, size_t extraSize)
  {
    m_extraData.clear();
    if (extraData && extraSize > 0)
    {
      for (size_t i = 0; i < extraSize; ++i)
        m_extraData.emplace_back(extraData[i]);
    }

    m_cStructure->m_ExtraData = m_extraData.data();
    m_cStructure->m_ExtraSize = static_cast<unsigned int>(m_extraData.size());
  }

  /// @brief To get with @ref SetExtraData changed values.
  const std::vector<uint8_t>& GetExtraData() { return m_extraData; }

  /// @brief To get size with @ref SetExtraData changed values.
  size_t GetExtraDataSize() { return m_extraData.size(); }

  /// @brief Compare extra data from outside with class
  ///
  /// @param[in] extraData Pointer with memory of extra data for compare
  /// @param[in] extraSize Size to compare
  /// @return true if they equal, false otherwise
  bool CompareExtraData(const uint8_t* extraData, size_t extraSize) const
  {
    if (!extraData || m_extraData.size() != extraSize)
      return false;
    for (size_t i = 0; i < extraSize; ++i)
    {
      if (m_extraData[i] != extraData[i])
        return false;
    }
    return true;
  }

  /// @brief Clear additional data.
  void ClearExtraData()
  {
    m_extraData.clear();
    m_cStructure->m_ExtraData = m_extraData.data();
    m_cStructure->m_ExtraSize = static_cast<unsigned int>(m_extraData.size());
  }

  /// @brief RFC 5646 language code (empty string if undefined).
  ///
  /// @param[in] language The language to set
  void SetLanguage(const std::string& language)
  {
    strncpy(m_cStructure->m_language, language.c_str(), INPUTSTREAM_MAX_STRING_LANGUAGE_SIZE);
  }

  /// @brief To get with @ref SetLanguage changed values.
  std::string GetLanguage() const { return m_cStructure->m_language; }

  /// @brief Scale of 1000 and a rate of 29970 will result in 29.97 fps.
  ///
  /// @param[in] fpsScale Scale rate
  void SetFpsScale(unsigned int fpsScale) { m_cStructure->m_FpsScale = fpsScale; }

  /// @brief To get with @ref SetFpsScale changed values.
  unsigned int GetFpsScale() const { return m_cStructure->m_FpsScale; }

  /// @brief Rate to use for stream.
  ///
  /// @param[in] fpsRate Rate to use
  void SetFpsRate(unsigned int fpsRate) { m_cStructure->m_FpsRate = fpsRate; }

  /// @brief To get with @ref SetFpsRate changed values.
  unsigned int GetFpsRate() const { return m_cStructure->m_FpsRate; }

  /// @brief Height of the stream reported by the demuxer.
  ///
  /// @param[in] height Height to use
  void SetHeight(unsigned int height) { m_cStructure->m_Height = height; }

  /// @brief To get with @ref SetHeight changed values.
  unsigned int GetHeight() const { return m_cStructure->m_Height; }

  /// @brief Width of the stream reported by the demuxer.
  ///
  /// @param[in] width Width to use
  void SetWidth(unsigned int width) { m_cStructure->m_Width = width; }

  /// @brief To get with @ref SetWidth changed values.
  unsigned int GetWidth() const { return m_cStructure->m_Width; }

  /// @brief Display aspect of stream.
  ///
  /// @param[in] aspect Aspect ratio to use
  void SetAspect(float aspect) { m_cStructure->m_Aspect = aspect; }

  /// @brief To get with @ref SetAspect changed values.
  float GetAspect() const { return m_cStructure->m_Aspect; }

  /// @brief (required) Amount of channels.
  ///
  /// @param[in] sampleRate Channels to use
  void SetChannels(unsigned int channels) { m_cStructure->m_Channels = channels; }

  /// @brief To get with @ref SetChannels changed values.
  unsigned int GetChannels() const { return m_cStructure->m_Channels; }

  /// @brief (required) Sample rate.
  ///
  /// @param[in] sampleRate Rate to use
  void SetSampleRate(unsigned int sampleRate) { m_cStructure->m_SampleRate = sampleRate; }

  /// @brief To get with @ref SetSampleRate changed values.
  unsigned int GetSampleRate() const { return m_cStructure->m_SampleRate; }

  /// @brief Bit rate.
  ///
  /// @param[in] bitRate Rate to use
  void SetBitRate(unsigned int bitRate) { m_cStructure->m_BitRate = bitRate; }

  /// @brief To get with @ref SetBitRate changed values.
  unsigned int GetBitRate() const { return m_cStructure->m_BitRate; }

  /// @brief (required) Bits per sample.
  ///
  /// @param[in] bitsPerSample Bits per sample to use
  void SetBitsPerSample(unsigned int bitsPerSample)
  {
    m_cStructure->m_BitsPerSample = bitsPerSample;
  }

  /// @brief To get with @ref SetBitsPerSample changed values.
  unsigned int GetBitsPerSample() const { return m_cStructure->m_BitsPerSample; }

  /// @brief To set the necessary stream block alignment size.
  ///
  /// @param[in] blockAlign Block size in byte
  void SetBlockAlign(unsigned int blockAlign) { m_cStructure->m_BlockAlign = blockAlign; }

  /// @brief To get with @ref SetBlockAlign changed values.
  unsigned int GetBlockAlign() const { return m_cStructure->m_BlockAlign; }

  /// @brief To set stream crypto session information.
  ///
  /// @param[in] cryptoSession The with @ref cpp_kodi_addon_inputstream_Defs_Interface_StreamCryptoSession setable info
  ///
  void SetCryptoSession(const kodi::addon::StreamCryptoSession& cryptoSession)
  {
    m_cryptoSession = cryptoSession;
    memcpy(&m_cStructure->m_cryptoSession, m_cryptoSession.GetCStructure(),
           sizeof(STREAM_CRYPTO_SESSION));
  }

  /// @brief To get with @ref GetCryptoSession changed values.
  const kodi::addon::StreamCryptoSession& GetCryptoSession() const { return m_cryptoSession; }

  /// @brief Codec If available, the fourcc code codec.
  ///
  /// @param[in] codecFourCC Codec four CC code
  void SetCodecFourCC(unsigned int codecFourCC) { m_cStructure->m_codecFourCC = codecFourCC; }

  /// @brief To get with @ref SetCodecFourCC changed values
  unsigned int GetCodecFourCC() const { return m_cStructure->m_codecFourCC; }

  /// @brief Definition of colorspace.
  ///
  /// @param[in] colorSpace The with @ref INPUTSTREAM_COLORSPACE setable color space
  void SetColorSpace(INPUTSTREAM_COLORSPACE colorSpace) { m_cStructure->m_colorSpace = colorSpace; }

  /// @brief To get with @ref SetColorSpace changed values.
  INPUTSTREAM_COLORSPACE GetColorSpace() const { return m_cStructure->m_colorSpace; }

  /// @brief Color range if available.
  ///
  /// @param[in] colorRange The with @ref INPUTSTREAM_COLORRANGE setable color space
  void SetColorRange(INPUTSTREAM_COLORRANGE colorRange) { m_cStructure->m_colorRange = colorRange; }

  /// @brief To get with @ref SetColorRange changed values.
  INPUTSTREAM_COLORRANGE GetColorRange() const { return m_cStructure->m_colorRange; }

  /// @brief Chromaticity coordinates of the source primaries. These values match the ones defined by ISO/IEC 23001-8_2013 § 7.1.
  ///
  /// @param[in] colorPrimaries The with @ref INPUTSTREAM_COLORPRIMARIES setable values
  void SetColorPrimaries(INPUTSTREAM_COLORPRIMARIES colorPrimaries)
  {
    m_cStructure->m_colorPrimaries = colorPrimaries;
  }

  /// @brief To get with @ref SetColorPrimaries changed values.
  INPUTSTREAM_COLORPRIMARIES GetColorPrimaries() const { return m_cStructure->m_colorPrimaries; }

  /// @brief Color Transfer Characteristic. These values match the ones defined by ISO/IEC 23001-8_2013 § 7.2.
  ///
  /// @param[in] colorTransferCharacteristic The with @ref INPUTSTREAM_COLORTRC setable characteristic
  void SetColorTransferCharacteristic(INPUTSTREAM_COLORTRC colorTransferCharacteristic)
  {
    m_cStructure->m_colorTransferCharacteristic = colorTransferCharacteristic;
  }

  /// @brief To get with @ref SetColorTransferCharacteristic changed values.
  INPUTSTREAM_COLORTRC GetColorTransferCharacteristic() const
  {
    return m_cStructure->m_colorTransferCharacteristic;
  }

  /// @brief Mastering static Metadata.
  ///
  /// Describes the metadata for HDR10, used when video is compressed using High
  /// Efficiency Video Coding (HEVC). This is used to describe the capabilities
  /// of the display used to master the content and the luminance values of the
  /// content.
  ///
  /// @param[in] masteringMetadata The with @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamMasteringMetadata setable metadata
  void SetMasteringMetadata(const kodi::addon::InputstreamMasteringMetadata& masteringMetadata)
  {
    m_masteringMetadata = masteringMetadata;
    m_cStructure->m_masteringMetadata = m_masteringMetadata;
  }

  /// @brief To get with @ref SetMasteringMetadata changed values.
  const kodi::addon::InputstreamMasteringMetadata& GetMasteringMetadata() const
  {
    return m_masteringMetadata;
  }

  /// @brief Clear mastering static Metadata.
  void ClearMasteringMetadata() { m_cStructure->m_masteringMetadata = nullptr; }

  /// @brief Content light static Metadata.
  ///
  /// The maximum content light level (MaxCLL) and frame average light level
  /// (MaxFALL) for the metadata for HDR10.
  ///
  /// @param[in] contentLightMetadata The with @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamContentlightMetadata setable metadata
  void SetContentLightMetadata(
      const kodi::addon::InputstreamContentlightMetadata& contentLightMetadata)
  {
    m_contentLightMetadata = contentLightMetadata;
    m_cStructure->m_contentLightMetadata = m_contentLightMetadata;
  }

  /// @brief To get with @ref SetContentLightMetadata changed values.
  const kodi::addon::InputstreamContentlightMetadata& GetContentLightMetadata() const
  {
    return m_contentLightMetadata;
  }

  /// @brief Clear content light static Metadata.
  void ClearContentLightMetadata() { m_cStructure->m_contentLightMetadata = nullptr; }

  ///@}

private:
  InputstreamInfo(const INPUTSTREAM_INFO* stream) : CStructHdl(stream)
  {
    SetCryptoSession(StreamCryptoSession(&stream->m_cryptoSession));
    CopyExtraData();
  }
  InputstreamInfo(INPUTSTREAM_INFO* stream) : CStructHdl(stream)
  {
    SetCryptoSession(StreamCryptoSession(&stream->m_cryptoSession));
    CopyExtraData();
  }

  void CopyExtraData()
  {
    if (m_cStructure->m_ExtraData && m_cStructure->m_ExtraSize > 0)
    {
      for (unsigned int i = 0; i < m_cStructure->m_ExtraSize; ++i)
        m_extraData.emplace_back(m_cStructure->m_ExtraData[i]);
    }
    if (m_cStructure->m_masteringMetadata)
      m_masteringMetadata = m_cStructure->m_masteringMetadata;
    if (m_cStructure->m_contentLightMetadata)
      m_contentLightMetadata = m_cStructure->m_contentLightMetadata;
  }
  std::vector<uint8_t> m_extraData;
  StreamCryptoSession m_cryptoSession;
  InputstreamMasteringMetadata m_masteringMetadata;
  InputstreamContentlightMetadata m_contentLightMetadata;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamTimes class InputstreamTimes
/// @ingroup cpp_kodi_addon_inputstream_Defs_Interface
/// @brief **Inputstream add-on times**\n
/// Used on @ref kodi::addon::CInstanceInputStream::GetTimes().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_inputstream_Defs_Interface_InputstreamTimes_Help
///
///@{
class ATTR_DLL_LOCAL InputstreamTimes : public CStructHdl<InputstreamTimes, INPUTSTREAM_TIMES>
{
  /*! \cond PRIVATE */
  friend class CInstanceInputStream;
  /*! \endcond */

public:
  /*! \cond PRIVATE */
  InputstreamTimes() = default;
  InputstreamTimes(const InputstreamTimes& stream) : CStructHdl(stream) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamTimes_Help Value Help
  /// @ingroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamTimes
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamTimes :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|--------------------
  /// | **Start time** | `time_t` | @ref InputstreamTimes::SetStartTime "SetStartTime" | @ref InputstreamTimes::GetStartTime "GetStartTime"
  /// | **PTS start** | `double` | @ref InputstreamTimes::SetPtsStart "SetPtsStart" | @ref InputstreamTimes::GetPtsStart "GetPtsStart"
  /// | **PTS begin** | `double` | @ref InputstreamTimes::SetPtsBegin "SetPtsBegin" | @ref InputstreamTimes::GetPtsBegin "GetPtsBegin"
  /// | **PTS end** | `double` | @ref InputstreamTimes::SetPtsEnd "SetPtsEnd" | @ref InputstreamTimes::GetPtsEnd "GetPtsEnd"
  ///

  /// @addtogroup cpp_kodi_addon_inputstream_Defs_Interface_InputstreamTimes
  ///@{

  /// @brief Start time in milliseconds
  void SetStartTime(time_t startTime) const { m_cStructure->startTime = startTime; }

  /// @brief To get with @ref SetStartTime changed values
  time_t GetStartTime() const { return m_cStructure->startTime; }

  /// @brief Start PTS
  void SetPtsStart(double ptsStart) const { m_cStructure->ptsStart = ptsStart; }

  /// @brief To get with @ref SetPtsStart changed values
  double GetPtsStart() const { return m_cStructure->ptsStart; }

  /// @brief Begin PTS
  void SetPtsBegin(double ptsBegin) const { m_cStructure->ptsBegin = ptsBegin; }

  /// @brief To get with @ref SetPtsBegin changed values
  double GetPtsBegin() const { return m_cStructure->ptsBegin; }

  /// @brief End PTS
  void SetPtsEnd(double ptsEnd) const { m_cStructure->ptsEnd = ptsEnd; }

  /// @brief To get with @ref SetPtsEnd changed values
  double GetPtsEnd() const { return m_cStructure->ptsEnd; }

  ///@}

private:
  InputstreamTimes(const INPUTSTREAM_TIMES* stream) : CStructHdl(stream) {}
  InputstreamTimes(INPUTSTREAM_TIMES* stream) : CStructHdl(stream) {}
};
///@}
//------------------------------------------------------------------------------

//============================================================================
///
/// @addtogroup cpp_kodi_addon_inputstream
/// @brief \cpp_class{ kodi::addon::CInstanceInputStream }
/// **Inputstream add-on instance**
///
/// This instance type is for using input streams to video and audio, to process
/// and then give them to Kodi.
///
/// This usage can be requested under various conditions, for example explicitly
/// by another addon, by a Mimetype protocol defined in `addon.xml` or supported
/// file extensions.
///
/// In addition, stream files (* .strm) can be linked to an inputstream addon
/// using <b>`#KODIPROP:inputstream=<ADDON_NAME>`</b>.
///
/// Include the header @ref Inputstream.h "#include <kodi/addon-instance/Inputstream.h>"
/// to use this class.
///
/// ----------------------------------------------------------------------------
///
/// Here is an example of what the <b>`addon.xml.in`</b> would look like for an inputstream addon:
///
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="inputstream.myspecialnamefor"
///   version="1.0.0"
///   name="My InputStream addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="kodi.inputstream"
///     extension=".xyz|.zyx"
///     listitemprops="license_type|license_key|license_data|license_flags"
///     protocols="myspecialnamefor|myspecialnamefors"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My InputStream addon</summary>
///     <description lang="en_GB">My InputStream description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
///
/// At <b>`<extension point="kodi.inputstream" ...>`</b> the basic instance definition is declared, this is intended to identify the addon as an input stream and to see its supported types:
/// | Name | Description
/// |------|----------------------
/// | <b>`point`</b> | The identification of the addon instance to inputstream is mandatory <b>`kodi.inputstream`</b>. In addition, the instance declared in the first <b>`<extension ... />`</b> is also
/// | <b>`extension`</b> | A filename extension is an identifier specified as a suffix to the name of a computer file where supported by addon.
/// | <b>`listitemprops`</b> | Values that are available to the addon at @ref InputstreamProperty::GetProperties() and that can be passed to @ref CInstanceInputStream::Open() ith the respective values.
/// | <b>`protocols`</b> | The streaming protocol is a special protocol supported by the addon for the transmission of streaming media data over a network.
/// | <b>`library_@PLATFORM@`</b> | The runtime library used for the addon. This is usually declared by cmake and correctly displayed in the translated `addon.xml`.
///
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
/// #include <kodi/addon-instance/Inputstream.h>
///
/// class CMyInputstream : public kodi::addon::CInstanceInputStream
/// {
/// public:
///   CMyInputstream(const kodi::addon::IInstanceInfo& instance);
///
///   void GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities) override;
///   bool Open(const kodi::addon::InputstreamProperty& props) override;
///   void Close() override;
///   ...
/// };
///
/// CMyInputstream::CMyInputstream(const kodi::addon::IInstanceInfo& instance)
///   : kodi::addon::CInstanceInputStream(instance)
/// {
///   ...
/// }
///
/// void CMyInputstream::GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities)
/// {
///   capabilities.SetMask(INPUTSTREAM_SUPPORTS_IDEMUX | INPUTSTREAM_SUPPORTS_PAUSE);
/// }
///
/// void CMyInputstream::Open(const kodi::addon::InputstreamProperty& props)
/// {
///   std::string url = props.GetURL();
///   ...
/// }
///
/// void CMyInputstream::Close()
/// {
///   ...
/// }
///
/// ...
///
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public kodi::addon::CAddonBase
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
///   if (instance.IsType(ADDON_INSTANCE_INPUTSTREAM))
///   {
///     kodi::Log(ADDON_LOG_NOTICE, "Creating my Inputstream");
///     hdl = new CMyInputstream(instance);
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
/// The destruction of the example class `CMyInputstream` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
//------------------------------------------------------------------------------
class ATTR_DLL_LOCAL CInstanceInputStream : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_inputstream
  /// @brief Inputstream class constructor used to support multiple instance
  /// types
  ///
  /// @param[in] instance The instance value given to <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///
  /// @warning Only use `instance` from the @ref CAddonBase::CreateInstance call.
  ///
  explicit CInstanceInputStream(const IInstanceInfo& instance) : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceInputStream: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_inputstream
  /// @brief Destructor
  ///
  ~CInstanceInputStream() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_inputstream
  /// @brief Get the list of features that this add-on provides.
  ///
  /// Called by Kodi to query the add-on's capabilities.
  /// Used to check which options should be presented in the UI, which methods to call, etc.
  /// All capabilities that the add-on supports should be set to true.
  ///
  /// @param[out] capabilities The with @ref cpp_kodi_addon_inputstream_Defs_Capabilities defined add-on's capabilities.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_inputstream_Defs_Interface_InputstreamCapabilities_Help
  ///
  /// --------------------------------------------------------------------------
  /// @note Valid implementation required.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  ///
  /// ~~~~~~~~~~~~~{.cpp}
  /// void CMyInputstream::GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities)
  /// {
  ///   capabilities.SetMask(INPUTSTREAM_SUPPORTS_IDEMUX | INPUTSTREAM_SUPPORTS_PAUSE);
  /// }
  /// ~~~~~~~~~~~~~
  ///
  virtual void GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_inputstream
  /// @brief Open a stream.
  ///
  /// @param[in] props The used properties about the stream
  /// @return True if the stream has been opened successfully, false otherwise.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_inputstream_Defs_InputstreamProperty_Help
  ///
  /// --------------------------------------------------------------------------
  /// @note Valid implementation required.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  ///
  /// ~~~~~~~~~~~~~{.cpp}
  /// void CMyInputstream::Open(const kodi::addon::InputstreamProperty& props)
  /// {
  ///   std::string url = props.GetURL();
  ///   std::string license_key = props.GetProperties()["inputstream.myspecialnamefor.license_key"];
  ///   ...
  /// }
  /// ~~~~~~~~~~~~~
  ///
  virtual bool Open(const kodi::addon::InputstreamProperty& props) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_inputstream
  /// @brief Close an open stream.
  ///
  /// @remarks
  ///
  ///
  /// --------------------------------------------------------------------------
  /// @note Valid implementation required.
  ///
  virtual void Close() = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_inputstream
  /// @brief Check for real-time streaming
  ///
  /// @return true if current stream is real-time
  ///
  virtual bool IsRealTimeStream() { return true; }
  //----------------------------------------------------------------------------

  //############################################################################
  /// @defgroup cpp_kodi_addon_inputstream_Read 1. Stream read
  /// @brief **Functions required to read streams direct and demux inside Kodi.**
  ///
  /// This part contains at least the functions necessary for addon that have to
  /// be supported. This can only be ignored if you use your own demux.
  ///
  /// The data loaded by Kodi is then processed in it itself. The source does not
  /// matter, only Kodi must be able to process this stream data itself and is
  /// therefore limited in some things.
  ///
  /// For more complex things, the addon must integrate its own demuxer and,
  /// if necessary, even its own codec processing (see @ref cpp_kodi_addon_codec "Codec").
  ///
  /// @note These are used and must be set by the addon if the @ref INPUTSTREAM_SUPPORTS_IDEMUX
  /// is <em><b>undefined</b></em> in the capabilities (see @ref GetCapabilities()).
  /// Otherwise becomes @ref cpp_kodi_addon_inputstream_Demux "demuxing" used.
  ///
  /// @ingroup cpp_kodi_addon_inputstream
  ///@{

  //============================================================================
  /// @brief Read from an open stream.
  ///
  /// @param[in] buffer The buffer to store the data in.
  /// @param[in] bufferSize The amount of bytes to read.
  /// @return The amount of bytes that were actually read from the stream.
  ///
  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) { return -1; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Seek in a stream.
  ///
  /// @param[in] position The position to seek to
  /// @param[in] whence offset relative to<br>
  /// You can set the value of whence to one of three things:
  /// |     Value    |  int  | Description                                         |
  /// |:------------:|:-----:|:----------------------------------------------------|
  /// | **SEEK_SET** |  `0`  | position is relative to the beginning of the file. This is probably what you had in mind anyway, and is the most commonly used value for whence.
  /// | **SEEK_CUR** |  `1`  | position is relative to the current file pointer position. So, in effect, you can say, "Move to my current position plus 30 bytes," or, "move to my current position minus 20 bytes."
  /// | **SEEK_END** |  `2`  | position is relative to the end of the file. Just like SEEK_SET except from the other end of the file. Be sure to use negative values for offset if you want to back up from the end of the file, instead of going past the end into oblivion.
  ///
  /// @return Returns the resulting offset location as measured in bytes from
  /// the beginning of the file. On error, the value -1 is returned.
  ///
  /// @remarks Optional and can leaved away or return -1 if this add-on won't
  /// provide this function.
  ///
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) { return -1; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief The position in the stream that's currently being read.
  ///
  /// @return Stream position
  ///
  /// @remarks Optional and can leaved away or return -1 if this add-on won't
  /// provide this function.
  ///
  virtual int64_t PositionStream() { return -1; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief The Total length of the stream that's currently being read.
  ///
  /// @return Length of the stream
  ///
  /// @remarks Optional and can leaved away or return -1 if this add-on won't
  /// provide this function.
  ///
  virtual int64_t LengthStream() { return -1; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Obtain the chunk size to use when reading streams.
  ///
  /// @return Block chunk size
  ///
  /// @remarks Optional and can leaved away or return 0 if this add-on won't
  /// provide this function.
  ///
  virtual int GetBlockSize() { return 0; }
  //--------------------------------------------------------------------------

  ///@}

  //############################################################################
  /// @defgroup cpp_kodi_addon_inputstream_Demux 2. Stream demuxing (optional)
  /// @brief **Read demux streams.**
  ///
  /// @note These are used and must be set by the addon if the @ref INPUTSTREAM_SUPPORTS_IDEMUX is set in the capabilities (see @ref GetCapabilities()).
  ///
  /// @ingroup cpp_kodi_addon_inputstream
  ///@{

  //============================================================================
  /// @brief Get IDs of available streams
  ///
  /// @param[in] ids list of used identifications
  /// @return true if successfully done, otherwise false
  ///
  /// @remarks This id's are used to identify wanted data on @ref GetStream call.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  ///
  /// ~~~~~~~~~~~~~{.cpp}
  ///
  /// bool CMyInputstream::GetStreamIds(std::vector<unsigned int>& ids)
  /// {
  ///   kodi::Log(ADDON_LOG_DEBUG, "GetStreamIds(...)");
  ///
  ///   if (m_opened)
  ///   {
  ///     // This check not needed to have, the ABI checks also about, but make
  ///     // sure you not give more as 32 streams.
  ///     if (m_myStreams.size() > MAX_STREAM_COUNT)
  ///     {
  ///       kodi::Log(ADDON_LOG_ERROR, "Too many streams, only %u supported", MAX_STREAM_COUNT);
  ///       return false;
  ///     }
  ///
  ///     ids.emplace_back(m_myAudioStreamId);
  ///
  ///     for (const auto& streamPair : m_myOtherStreams)
  ///     {
  ///       ids.emplace_back(streamPair.second->uniqueId);
  ///     }
  ///   }
  ///
  ///   return !ids.empty();
  /// }
  /// ~~~~~~~~~~~~~
  ///
  virtual bool GetStreamIds(std::vector<unsigned int>& ids) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Function for giving detailed stream information
  ///
  /// The associated information is set here for IDs previously given with
  /// @ref GetStreamIds.
  ///
  /// This data is required to identify the associated codec and, if necessary,
  /// to refer to your own codec (if available in the addon).
  ///
  /// @param[in] streamid unique id of stream
  /// @param[out] stream Information data of wanted stream
  /// @return true if successfully done, otherwise false
  ///
  /// @remarks Available stream id's previously asked by @ref GetStreamIds
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo_Help
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  ///
  /// ~~~~~~~~~~~~~{.cpp}
  /// bool CMyInputstream::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
  /// {
  ///   // This is just a small example, this type will be significantly larger
  ///   // for larger and more complex streams.
  ///   if (streamid == m_myAudioStreamId)
  ///   {
  ///     // This only a minimal exampl
  ///     stream.SetStreamType(INPUTSTREAM_TYPE_AUDIO);
  ///     stream.SetFeatures(INPUTSTREAM_FEATURE_NONE); // Only added to example, INPUTSTREAM_FEATURE_NONE is default and no need to call
  ///     stream.SetFlags(INPUTSTREAM_FLAG_NONE); // Only added to example, INPUTSTREAM_FLAG_NONE is default and no need to call
  ///     stream.SetCodecName("mp2"); // Codec name, string must by equal with FFmpeg, see https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/codec_desc.c
  ///     stream.SetPhysicalIndex(1); // Identifier required to set
  ///     stream.SetLanguage("en");
  ///     stream.SetChannels(2);
  ///     stream.SetSampleRate(48000);
  ///     stream.SetBitRate(0);
  ///     stream.SetBitsPerSample(16);
  ///   }
  ///   else ...
  ///   ...
  ///   return true;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  virtual bool GetStream(int streamid, kodi::addon::InputstreamInfo& stream) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Enable or disable a stream.
  ///
  /// A disabled stream does not send demux packets
  ///
  /// @param[in] streamid unique id of stream
  /// @param[in] enable true for enable, false for disable
  ///
  /// @remarks Available stream id's previously asked by @ref GetStreamIds
  ///
  ///
  /// --------------------------------------------------------------------------
  /// @note Valid implementation required.
  ///
  virtual void EnableStream(int streamid, bool enable) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Opens a stream for playback.
  ///
  /// @param[in] streamid unique id of stream
  ///
  /// @remarks Available stream id's previously asked by @ref GetStreamIds
  ///
  ///
  /// --------------------------------------------------------------------------
  /// @note Valid implementation required.
  ///
  virtual bool OpenStream(int streamid) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Reset the demultiplexer in the add-on.
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///
  virtual void DemuxReset() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Abort the demultiplexer thread in the add-on.
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///
  virtual void DemuxAbort() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Flush all data that's currently in the demultiplexer buffer in the add-on.
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///
  virtual void DemuxFlush() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Read the next packet from the demultiplexer, if there is one.
  ///
  /// @return The next packet.
  ///         If there is no next packet, then the add-on should return the
  ///         packet created by calling @ref AllocateDemuxPacket "AllocateDemuxPacket(0)" on the callback.
  ///         If the stream changed and Kodi's player needs to be reinitialised,
  ///         then, the add-on should call @ref AllocateDemuxPacket "AllocateDemuxPacket(0)" on the
  ///         callback, and set the streamid to DMX_SPECIALID_STREAMCHANGE and
  ///         return the value.
  ///         The add-on should return <b>`nullptr`</b> if an error occurred.
  ///
  /// @remarks Return <b>`nullptr`</b> if this add-on won't provide this function.
  ///
  virtual DEMUX_PACKET* DemuxRead() { return nullptr; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify the InputStream addon/demuxer that Kodi wishes to seek the stream by time
  ///
  /// Demuxer is required to set stream to an IDR frame
  ///
  /// @param[in] time The absolute time since stream start
  /// @param[in] backwards True to seek to keyframe BEFORE time, else AFTER
  /// @param[in] startpts can be updated to point to where display should start
  /// @return True if the seek operation was possible
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify the InputStream addon/demuxer that Kodi wishes to change playback speed
  ///
  /// @param[in] speed The requested playback speed
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///
  virtual void DemuxSetSpeed(int speed) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify current screen resolution
  ///
  /// @param[in] width Width to set
  /// @param[in] height Height to set
  ///
  virtual void SetVideoResolution(unsigned int width, unsigned int height) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify current screen resolution and max screen resolution allowed
  ///
  /// @param[in] width Width to set
  /// @param[in] height Height to set
  /// @param[in] maxWidth Max width allowed
  /// @param[in] maxHeight Max height allowed
  ///
  virtual void SetVideoResolution(unsigned int width,
                                  unsigned int height,
                                  unsigned int maxWidth,
                                  unsigned int maxHeight)
  {
  }
  //----------------------------------------------------------------------------

  //=============================================================================
  /// @brief Allocate a demux packet. Free with @ref FreeDemuxPacket
  ///
  /// @param[in] dataSize The size of the data that will go into the packet
  /// @return The allocated packet
  ///
  /// @remarks Only called from addon itself
  ///
  DEMUX_PACKET* AllocateDemuxPacket(int dataSize)
  {
    return m_instanceData->toKodi->allocate_demux_packet(m_instanceData->toKodi->kodiInstance,
                                                         dataSize);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Allocate a encrypted demux packet. Free with @ref FreeDemuxPacket
  ///
  /// @param[in] dataSize The size of the data that will go into the packet
  /// @param[in] encryptedSubsampleCount The encrypted subsample count
  /// @return The allocated packet
  ///
  /// @remarks Only called from addon itself
  ///
  DEMUX_PACKET* AllocateEncryptedDemuxPacket(int dataSize, unsigned int encryptedSubsampleCount)
  {
    return m_instanceData->toKodi->allocate_encrypted_demux_packet(
        m_instanceData->toKodi->kodiInstance, dataSize, encryptedSubsampleCount);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Free a packet that was allocated with AllocateDemuxPacket
  ///
  /// @param[in] packet The packet to free
  ///
  /// @remarks Only called from addon itself
  ///
  void FreeDemuxPacket(DEMUX_PACKET* packet)
  {
    return m_instanceData->toKodi->free_demux_packet(m_instanceData->toKodi->kodiInstance, packet);
  }
  //----------------------------------------------------------------------------

  ///@}

  //############################################################################
  /// @defgroup cpp_kodi_addon_inputstream_Time 3. Time (optional)
  /// @brief **To get stream position time.**
  ///
  /// @note These are used and must be set by the addon if the @ref INPUTSTREAM_SUPPORTS_IDISPLAYTIME is set in the capabilities (see @ref GetCapabilities()).
  ///
  /// @ingroup cpp_kodi_addon_inputstream
  ///@{

  //==========================================================================
  /// @brief Totel time in ms
  ///
  /// @return Total time in milliseconds
  ///
  /// @remarks
  ///
  virtual int GetTotalTime() { return -1; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Playing time in ms
  ///
  /// @return Playing time in milliseconds
  ///
  /// @remarks
  ///
  virtual int GetTime() { return -1; }
  //--------------------------------------------------------------------------

  ///@}

  //############################################################################
  /// @defgroup cpp_kodi_addon_inputstream_Times 4. Times (optional)
  /// @brief **Another way to get stream position time.**
  ///
  /// @note These are used and must be set by the addon if the @ref INPUTSTREAM_SUPPORTS_ITIME is set in the capabilities (see @ref GetCapabilities()).
  ///
  /// @ingroup cpp_kodi_addon_inputstream
  ///@{

  //============================================================================
  /// @brief Get current timing values in PTS scale
  ///
  /// @param[out] times The with @ref InputstreamTimes to given times
  /// @return true if successfully done, false if not
  ///
  /// @copydetails cpp_kodi_addon_inputstream_Defs_Times_Help
  ///
  /// @remarks
  ///
  virtual bool GetTimes(InputstreamTimes& times) { return false; }
  //----------------------------------------------------------------------------

  ///@}

  //############################################################################
  /// @defgroup cpp_kodi_addon_inputstream_PosTime 5. Position time (optional)
  /// @brief **Third way get stream position time.**
  ///
  /// @note These are used and must be set by the addon if the @ref INPUTSTREAM_SUPPORTS_IPOSTIME is set in the capabilities (see @ref GetCapabilities()).
  ///
  /// @ingroup cpp_kodi_addon_inputstream
  ///@{

  //============================================================================
  /// @brief Positions inputstream to playing time given in ms
  ///
  /// @param[in] ms Position time in milliseconds
  ///
  /// @remarks
  ///
  virtual bool PosTime(int ms) { return false; }
  //----------------------------------------------------------------------------

  ///@}

  //############################################################################
  /// @defgroup cpp_kodi_addon_inputstream_Chapter 6. Chapter (optional)
  /// @brief **Used to get available chapters.**
  ///
  /// @note These are used and must be set by the addon if the @ref INPUTSTREAM_SUPPORTS_ICHAPTER is set in the capabilities (see @ref GetCapabilities()).
  ///
  /// @ingroup cpp_kodi_addon_inputstream
  ///@{

  //==========================================================================
  ///
  /// @brief Return currently selected chapter
  ///
  /// @return Chapter number
  ///
  /// @remarks
  ///
  virtual int GetChapter() { return -1; }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// @brief Return number of available chapters
  ///
  /// @return Chapter count
  ///
  /// @remarks
  ///
  virtual int GetChapterCount() { return 0; }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// @brief Return name of chapter
  ///
  /// @param[in] ch Chapter identifier
  /// @return Chapter name
  ///
  /// @remarks
  ///
  virtual const char* GetChapterName(int ch) { return nullptr; }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// @brief Return position if chapter # ch in milliseconds
  ///
  /// @param[in] ch Chapter to get position from
  /// @return Position in milliseconds
  ///
  /// @remarks
  ///
  virtual int64_t GetChapterPos(int ch) { return 0; }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// @brief Seek to the beginning of chapter # ch
  ///
  /// @param[in] ch Chapter to seek
  /// @return True if successfully done, false if not
  ///
  /// @remarks
  ///
  virtual bool SeekChapter(int ch) { return false; }
  //--------------------------------------------------------------------------

  ///@}

private:
  static int compareVersion(const int v1[3], const int v2[3])
  {
    for (unsigned i(0); i < 3; ++i)
      if (v1[i] != v2[i])
        return v1[i] - v2[i];
    return 0;
  }

  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    int api[3] = {0, 0, 0};
    sscanf(GetInstanceAPIVersion().c_str(), "%d.%d.%d", &api[0], &api[1], &api[2]);

    instance->hdl = this;
    instance->inputstream->toAddon->open = ADDON_Open;
    instance->inputstream->toAddon->close = ADDON_Close;
    instance->inputstream->toAddon->get_capabilities = ADDON_GetCapabilities;

    instance->inputstream->toAddon->get_stream_ids = ADDON_GetStreamIds;
    instance->inputstream->toAddon->get_stream = ADDON_GetStream;
    instance->inputstream->toAddon->enable_stream = ADDON_EnableStream;
    instance->inputstream->toAddon->open_stream = ADDON_OpenStream;
    instance->inputstream->toAddon->demux_reset = ADDON_DemuxReset;
    instance->inputstream->toAddon->demux_abort = ADDON_DemuxAbort;
    instance->inputstream->toAddon->demux_flush = ADDON_DemuxFlush;
    instance->inputstream->toAddon->demux_read = ADDON_DemuxRead;
    instance->inputstream->toAddon->demux_seek_time = ADDON_DemuxSeekTime;
    instance->inputstream->toAddon->demux_set_speed = ADDON_DemuxSetSpeed;
    instance->inputstream->toAddon->set_video_resolution = ADDON_SetVideoResolution;

    instance->inputstream->toAddon->get_total_time = ADDON_GetTotalTime;
    instance->inputstream->toAddon->get_time = ADDON_GetTime;

    instance->inputstream->toAddon->get_times = ADDON_GetTimes;
    instance->inputstream->toAddon->pos_time = ADDON_PosTime;

    instance->inputstream->toAddon->read_stream = ADDON_ReadStream;
    instance->inputstream->toAddon->seek_stream = ADDON_SeekStream;
    instance->inputstream->toAddon->position_stream = ADDON_PositionStream;
    instance->inputstream->toAddon->length_stream = ADDON_LengthStream;
    instance->inputstream->toAddon->is_real_time_stream = ADDON_IsRealTimeStream;

    // Added on 2.0.10
    instance->inputstream->toAddon->get_chapter = ADDON_GetChapter;
    instance->inputstream->toAddon->get_chapter_count = ADDON_GetChapterCount;
    instance->inputstream->toAddon->get_chapter_name = ADDON_GetChapterName;
    instance->inputstream->toAddon->get_chapter_pos = ADDON_GetChapterPos;
    instance->inputstream->toAddon->seek_chapter = ADDON_SeekChapter;

    // Added on 2.0.12
    instance->inputstream->toAddon->block_size_stream = ADDON_GetBlockSize;

    /*
    // Way to include part on new API version
    int minPartVersion[3] = { 3, 0, 0 };
    if (compareVersion(api, minPartVersion) >= 0)
    {

    }
    */

    m_instanceData = instance->inputstream;
    m_instanceData->toAddon->addonInstance = this;
  }

  inline static bool ADDON_Open(const AddonInstance_InputStream* instance,
                                INPUTSTREAM_PROPERTY* props)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->Open(props);
  }

  inline static void ADDON_Close(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->Close();
  }

  inline static void ADDON_GetCapabilities(const AddonInstance_InputStream* instance,
                                           INPUTSTREAM_CAPABILITIES* capabilities)
  {
    InputstreamCapabilities caps(capabilities);
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetCapabilities(caps);
  }

  // IDemux
  inline static bool ADDON_GetStreamIds(const AddonInstance_InputStream* instance,
                                        struct INPUTSTREAM_IDS* ids)
  {
    std::vector<unsigned int> idList;
    bool ret =
        static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetStreamIds(idList);
    if (ret)
    {
      for (size_t i = 0; i < idList.size() && i < INPUTSTREAM_MAX_STREAM_COUNT; ++i)
      {
        ids->m_streamCount++;
        ids->m_streamIds[i] = idList[i];
      }
    }
    return ret;
  }

  inline static bool ADDON_GetStream(
      const AddonInstance_InputStream* instance,
      int streamid,
      struct INPUTSTREAM_INFO* info,
      KODI_HANDLE* demuxStream,
      KODI_HANDLE (*transfer_stream)(KODI_HANDLE handle,
                                     int streamId,
                                     struct INPUTSTREAM_INFO* stream))
  {
    InputstreamInfo infoData(info);
    bool ret = static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
                   ->GetStream(streamid, infoData);
    if (ret && transfer_stream)
    {
      // Do this with given callback to prevent memory problems and leaks. This
      // then create on Kodi the needed class where then given back on demuxStream.
      *demuxStream = transfer_stream(instance->toKodi->kodiInstance, streamid, info);
    }
    return ret;
  }

  inline static void ADDON_EnableStream(const AddonInstance_InputStream* instance,
                                        int streamid,
                                        bool enable)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->EnableStream(streamid, enable);
  }

  inline static bool ADDON_OpenStream(const AddonInstance_InputStream* instance, int streamid)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->OpenStream(streamid);
  }

  inline static void ADDON_DemuxReset(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxReset();
  }

  inline static void ADDON_DemuxAbort(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxAbort();
  }

  inline static void ADDON_DemuxFlush(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxFlush();
  }

  inline static DEMUX_PACKET* ADDON_DemuxRead(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxRead();
  }

  inline static bool ADDON_DemuxSeekTime(const AddonInstance_InputStream* instance,
                                         double time,
                                         bool backwards,
                                         double* startpts)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->DemuxSeekTime(time, backwards, *startpts);
  }

  inline static void ADDON_DemuxSetSpeed(const AddonInstance_InputStream* instance, int speed)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxSetSpeed(speed);
  }

  inline static void ADDON_SetVideoResolution(const AddonInstance_InputStream* instance,
                                              unsigned int width,
                                              unsigned int height,
                                              unsigned int maxWidth,
                                              unsigned int maxHeight)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->SetVideoResolution(width, height);
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->SetVideoResolution(width, height, maxWidth, maxHeight);
  }

  // IDisplayTime
  inline static int ADDON_GetTotalTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTotalTime();
  }

  inline static int ADDON_GetTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTime();
  }

  // ITime
  inline static bool ADDON_GetTimes(const AddonInstance_InputStream* instance,
                                    INPUTSTREAM_TIMES* times)
  {
    InputstreamTimes cppTimes(times);
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTimes(cppTimes);
  }

  // IPosTime
  inline static bool ADDON_PosTime(const AddonInstance_InputStream* instance, int ms)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->PosTime(ms);
  }

  inline static int ADDON_GetChapter(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapter();
  }

  inline static int ADDON_GetChapterCount(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterCount();
  }

  inline static const char* ADDON_GetChapterName(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterName(ch);
  }

  inline static int64_t ADDON_GetChapterPos(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterPos(ch);
  }

  inline static bool ADDON_SeekChapter(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->SeekChapter(ch);
  }

  inline static int ADDON_ReadStream(const AddonInstance_InputStream* instance,
                                     uint8_t* buffer,
                                     unsigned int bufferSize)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->ReadStream(buffer, bufferSize);
  }

  inline static int64_t ADDON_SeekStream(const AddonInstance_InputStream* instance,
                                         int64_t position,
                                         int whence)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->SeekStream(position, whence);
  }

  inline static int64_t ADDON_PositionStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->PositionStream();
  }

  inline static int64_t ADDON_LengthStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->LengthStream();
  }

  inline static int ADDON_GetBlockSize(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetBlockSize();
  }

  inline static bool ADDON_IsRealTimeStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->IsRealTimeStream();
  }

  AddonInstance_InputStream* m_instanceData;
};
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
