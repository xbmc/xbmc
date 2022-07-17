/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_general.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 1 - General PVR
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue class PVRTypeIntValue
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on type value**\n
/// Representation of a <b>`<int, std::string>`</b> event related value.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
///
///@{
class PVRTypeIntValue : public CStructHdl<PVRTypeIntValue, PVR_ATTRIBUTE_INT_VALUE>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRTypeIntValue(const PVRTypeIntValue& data) : CStructHdl(data) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Value** | `int` | @ref PVRTypeIntValue::SetValue "SetValue" | @ref PVRTypeIntValue::GetValue "GetValue"
  /// | **Description** | `std::string` | @ref PVRTypeIntValue::SetDescription "SetDescription" | @ref PVRTypeIntValue::GetDescription "GetDescription"
  ///
  /// @remark Further can there be used his class constructor to set values.

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue
  ///@{

  /// @brief Default class constructor.
  ///
  /// @note Values must be set afterwards.
  PVRTypeIntValue() = default;

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] value Type identification value
  /// @param[in] description Type description text
  PVRTypeIntValue(int value, const std::string& description)
  {
    SetValue(value);
    SetDescription(description);
  }

  /// @brief To set with the identification value.
  void SetValue(int value) { m_cStructure->iValue = value; }

  /// @brief To get with the identification value.
  int GetValue() const { return m_cStructure->iValue; }

  /// @brief To set with the description text of the value.
  void SetDescription(const std::string& description)
  {
    strncpy(m_cStructure->strDescription, description.c_str(),
            sizeof(m_cStructure->strDescription) - 1);
  }

  /// @brief To get with the description text of the value.
  std::string GetDescription() const { return m_cStructure->strDescription; }
  ///@}

private:
  PVRTypeIntValue(const PVR_ATTRIBUTE_INT_VALUE* data) : CStructHdl(data) {}
  PVRTypeIntValue(PVR_ATTRIBUTE_INT_VALUE* data) : CStructHdl(data) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRCapabilities class PVRCapabilities
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on capabilities**\n
/// This class is needed to tell Kodi which options are supported on the addon.
///
/// If a capability is set to **true**, then the corresponding methods from
/// @ref cpp_kodi_addon_pvr "kodi::addon::CInstancePVRClient" need to be
/// implemented.
///
/// As default them all set to **false**.
///
/// Used on @ref kodi::addon::CInstancePVRClient::GetCapabilities().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRCapabilities_Help
///
///@{
class PVRCapabilities
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  explicit PVRCapabilities() = delete;
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_PVRCapabilities_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_PVRCapabilities
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_PVRCapabilities :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Supports EPG** | `boolean` | @ref PVRCapabilities::SetSupportsEPG "SetSupportsEPG" | @ref PVRCapabilities::GetSupportsEPG "GetSupportsEPG"
  /// | **Supports EPG EDL** | `boolean` | @ref PVRCapabilities::SetSupportsEPGEdl "SetSupportsEPGEdl" | @ref PVRCapabilities::GetSupportsEPGEdl "GetSupportsEPGEdl"
  /// | **Supports TV** | `boolean` | @ref PVRCapabilities::SetSupportsTV "SetSupportsTV" | @ref PVRCapabilities::GetSupportsTV "GetSupportsTV"
  /// | **Supports radio** | `boolean` | @ref PVRCapabilities::SetSupportsRadio "SetSupportsRadio" | @ref PVRCapabilities::GetSupportsRadio "GetSupportsRadio"
  /// | **Supports recordings** | `boolean` | @ref PVRCapabilities::SetSupportsRecordings "SetSupportsRecordings" | @ref PVRCapabilities::GetSupportsRecordings "GetSupportsRecordings"
  /// | **Supports recordings undelete** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsUndelete "SetSupportsRecordingsUndelete" | @ref PVRCapabilities::GetSupportsRecordingsUndelete "SetSupportsRecordingsUndelete"
  /// | **Supports timers** | `boolean` | @ref PVRCapabilities::SetSupportsTimers "SetSupportsTimers" | @ref PVRCapabilities::GetSupportsTimers "GetSupportsTimers"
  /// | **Supports providers** | `boolean` | @ref PVRCapabilities::SetSupportsProviders "SetSupportsProviders" | @ref PVRCapabilities::GetSupportsProviders "GetSupportsProviders"
  /// | **Supports channel groups** | `boolean` | @ref PVRCapabilities::SetSupportsChannelGroups "SetSupportsChannelGroups" | @ref PVRCapabilities::GetSupportsChannelGroups "GetSupportsChannelGroups"
  /// | **Supports channel scan** | `boolean` | @ref PVRCapabilities::SetSupportsChannelScan "SetSupportsChannelScan" | @ref PVRCapabilities::GetSupportsChannelScan "GetSupportsChannelScan"
  /// | **Supports channel settings** | `boolean` | @ref PVRCapabilities::SetSupportsChannelSettings "SetSupportsChannelSettings" | @ref PVRCapabilities::GetSupportsChannelSettings "GetSupportsChannelSettings"
  /// | **Handles input stream** | `boolean` | @ref PVRCapabilities::SetHandlesInputStream "SetHandlesInputStream" | @ref PVRCapabilities::GetHandlesInputStream "GetHandlesInputStream"
  /// | **Handles demuxing** | `boolean` | @ref PVRCapabilities::SetHandlesDemuxing "SetHandlesDemuxing" | @ref PVRCapabilities::GetHandlesDemuxing "GetHandlesDemuxing"
  /// | **Supports recording play count** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingPlayCount "SetSupportsRecordingPlayCount" | @ref PVRCapabilities::GetSupportsRecordingPlayCount "GetSupportsRecordingPlayCount"
  /// | **Supports last played position** | `boolean` | @ref PVRCapabilities::SetSupportsLastPlayedPosition "SetSupportsLastPlayedPosition" | @ref PVRCapabilities::GetSupportsLastPlayedPosition "GetSupportsLastPlayedPosition"
  /// | **Supports recording EDL** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingEdl "SetSupportsRecordingEdl" | @ref PVRCapabilities::GetSupportsRecordingEdl "GetSupportsRecordingEdl"
  /// | **Supports recordings rename** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsRename "SetSupportsRecordingsRename" | @ref PVRCapabilities::GetSupportsRecordingsRename "GetSupportsRecordingsRename"
  /// | **Supports recordings lifetime change** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsLifetimeChange "SetSupportsRecordingsLifetimeChange" | @ref PVRCapabilities::GetSupportsRecordingsLifetimeChange "GetSupportsRecordingsLifetimeChange"
  /// | **Supports descramble info** | `boolean` | @ref PVRCapabilities::SetSupportsDescrambleInfo "SetSupportsDescrambleInfo" | @ref PVRCapabilities::GetSupportsDescrambleInfo "GetSupportsDescrambleInfo"
  /// | **Supports async EPG transfer** | `boolean` | @ref PVRCapabilities::SetSupportsAsyncEPGTransfer "SetSupportsAsyncEPGTransfer" | @ref PVRCapabilities::GetSupportsAsyncEPGTransfer "GetSupportsAsyncEPGTransfer"
  /// | **Supports recording size** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingSize "SetSupportsRecordingSize" | @ref PVRCapabilities::GetSupportsRecordingSize "GetSupportsRecordingSize"
  /// | **Supports recordings delete** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsDelete "SetSupportsRecordingsDelete" | @ref PVRCapabilities::GetSupportsRecordingsDelete "SetSupportsRecordingsDelete"
  /// | **Recordings lifetime values** | @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRCapabilities::SetRecordingsLifetimeValues "SetRecordingsLifetimeValues" | @ref PVRCapabilities::GetRecordingsLifetimeValues "GetRecordingsLifetimeValues"
  ///
  /// @warning This class can not be used outside of @ref kodi::addon::CInstancePVRClient::GetCapabilities()
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRCapabilities
  ///@{

  /// @brief Set **true** if the add-on provides EPG information.
  void SetSupportsEPG(bool supportsEPG) { m_capabilities->bSupportsEPG = supportsEPG; }

  /// @brief To get with @ref SetSupportsEPG changed values.
  bool GetSupportsEPG() const { return m_capabilities->bSupportsEPG; }

  /// @brief Set **true** if the backend supports retrieving an edit decision
  /// list for an EPG tag.
  void SetSupportsEPGEdl(bool supportsEPGEdl) { m_capabilities->bSupportsEPGEdl = supportsEPGEdl; }

  /// @brief To get with @ref SetSupportsEPGEdl changed values.
  bool GetSupportsEPGEdl() const { return m_capabilities->bSupportsEPGEdl; }

  /// @brief Set **true** if this add-on provides TV channels.
  void SetSupportsTV(bool supportsTV) { m_capabilities->bSupportsTV = supportsTV; }

  /// @brief To get with @ref SetSupportsTV changed values.
  bool GetSupportsTV() const { return m_capabilities->bSupportsTV; }

  /// @brief Set **true** if this add-on provides TV channels.
  void SetSupportsRadio(bool supportsRadio) { m_capabilities->bSupportsRadio = supportsRadio; }

  /// @brief To get with @ref SetSupportsRadio changed values.
  bool GetSupportsRadio() const { return m_capabilities->bSupportsRadio; }

  /// @brief **true** if this add-on supports playback of recordings stored on
  /// the backend.
  void SetSupportsRecordings(bool supportsRecordings)
  {
    m_capabilities->bSupportsRecordings = supportsRecordings;
  }

  /// @brief To get with @ref SetSupportsRecordings changed values.
  bool GetSupportsRecordings() const { return m_capabilities->bSupportsRecordings; }

  /// @brief Set **true** if this add-on supports undelete of recordings stored
  /// on the backend.
  void SetSupportsRecordingsUndelete(bool supportsRecordingsUndelete)
  {
    m_capabilities->bSupportsRecordingsUndelete = supportsRecordingsUndelete;
  }

  /// @brief To get with @ref SetSupportsRecordings changed values.
  bool GetSupportsRecordingsUndelete() const { return m_capabilities->bSupportsRecordingsUndelete; }

  /// @brief Set **true** if this add-on supports the creation and editing of
  /// timers.
  void SetSupportsTimers(bool supportsTimers) { m_capabilities->bSupportsTimers = supportsTimers; }

  /// @brief To get with @ref SetSupportsTimers changed values.
  bool GetSupportsTimers() const { return m_capabilities->bSupportsTimers; }

  /// @brief Set **true** if this add-on supports providers.
  ///
  /// It uses the following functions:
  /// - @ref kodi::addon::CInstancePVRClient::GetProvidersAmount()
  /// - @ref kodi::addon::CInstancePVRClient::GetProviders()
  void SetSupportsProviders(bool supportsProviders)
  {
    m_capabilities->bSupportsProviders = supportsProviders;
  }

  /// @brief To get with @ref SetSupportsProviders changed values.
  bool GetSupportsProviders() const { return m_capabilities->bSupportsProviders; }

  /// @brief Set **true** if this add-on supports channel groups.
  ///
  /// It use the following functions:
  /// - @ref kodi::addon::CInstancePVRClient::GetChannelGroupsAmount()
  /// - @ref kodi::addon::CInstancePVRClient::GetChannelGroups()
  /// - @ref kodi::addon::CInstancePVRClient::GetChannelGroupMembers()
  void SetSupportsChannelGroups(bool supportsChannelGroups)
  {
    m_capabilities->bSupportsChannelGroups = supportsChannelGroups;
  }

  /// @brief To get with @ref SetSupportsChannelGroups changed values.
  bool GetSupportsChannelGroups() const { return m_capabilities->bSupportsChannelGroups; }

  /// @brief Set **true** if this add-on support scanning for new channels on
  /// the backend.
  ///
  /// It use the following function:
  /// - @ref kodi::addon::CInstancePVRClient::OpenDialogChannelScan()
  void SetSupportsChannelScan(bool supportsChannelScan)
  {
    m_capabilities->bSupportsChannelScan = supportsChannelScan;
  }

  /// @brief To get with @ref SetSupportsChannelScan changed values.
  bool GetSupportsChannelScan() const { return m_capabilities->bSupportsChannelScan; }

  /// @brief Set **true** if this add-on supports channel edit.
  ///
  /// It use the following functions:
  /// - @ref kodi::addon::CInstancePVRClient::DeleteChannel()
  /// - @ref kodi::addon::CInstancePVRClient::RenameChannel()
  /// - @ref kodi::addon::CInstancePVRClient::OpenDialogChannelSettings()
  /// - @ref kodi::addon::CInstancePVRClient::OpenDialogChannelAdd()
  void SetSupportsChannelSettings(bool supportsChannelSettings)
  {
    m_capabilities->bSupportsChannelSettings = supportsChannelSettings;
  }

  /// @brief To get with @ref SetSupportsChannelSettings changed values.
  bool GetSupportsChannelSettings() const { return m_capabilities->bSupportsChannelSettings; }

  /// @brief Set **true** if this add-on provides an input stream. false if Kodi
  /// handles the stream.
  void SetHandlesInputStream(bool handlesInputStream)
  {
    m_capabilities->bHandlesInputStream = handlesInputStream;
  }

  /// @brief To get with @ref SetHandlesInputStream changed values.
  bool GetHandlesInputStream() const { return m_capabilities->bHandlesInputStream; }

  /// @brief Set **true** if this add-on demultiplexes packets.
  void SetHandlesDemuxing(bool handlesDemuxing)
  {
    m_capabilities->bHandlesDemuxing = handlesDemuxing;
  }

  /// @brief To get with @ref SetHandlesDemuxing changed values.
  bool GetHandlesDemuxing() const { return m_capabilities->bHandlesDemuxing; }

  /// @brief Set **true** if the backend supports play count for recordings.
  void SetSupportsRecordingPlayCount(bool supportsRecordingPlayCount)
  {
    m_capabilities->bSupportsRecordingPlayCount = supportsRecordingPlayCount;
  }

  /// @brief To get with @ref SetSupportsRecordingPlayCount changed values.
  bool GetSupportsRecordingPlayCount() const { return m_capabilities->bSupportsRecordingPlayCount; }

  /// @brief Set **true** if the backend supports store/retrieve of last played
  /// position for recordings.
  void SetSupportsLastPlayedPosition(bool supportsLastPlayedPosition)
  {
    m_capabilities->bSupportsLastPlayedPosition = supportsLastPlayedPosition;
  }

  /// @brief To get with @ref SetSupportsLastPlayedPosition changed values.
  bool GetSupportsLastPlayedPosition() const { return m_capabilities->bSupportsLastPlayedPosition; }

  /// @brief Set **true** if the backend supports retrieving an edit decision
  /// list for recordings.
  void SetSupportsRecordingEdl(bool supportsRecordingEdl)
  {
    m_capabilities->bSupportsRecordingEdl = supportsRecordingEdl;
  }

  /// @brief To get with @ref SetSupportsRecordingEdl changed values.
  bool GetSupportsRecordingEdl() const { return m_capabilities->bSupportsRecordingEdl; }

  /// @brief Set **true** if the backend supports renaming recordings.
  void SetSupportsRecordingsRename(bool supportsRecordingsRename)
  {
    m_capabilities->bSupportsRecordingsRename = supportsRecordingsRename;
  }

  /// @brief To get with @ref SetSupportsRecordingsRename changed values.
  bool GetSupportsRecordingsRename() const { return m_capabilities->bSupportsRecordingsRename; }

  /// @brief Set **true** if the backend supports changing lifetime for
  /// recordings.
  void SetSupportsRecordingsLifetimeChange(bool supportsRecordingsLifetimeChange)
  {
    m_capabilities->bSupportsRecordingsLifetimeChange = supportsRecordingsLifetimeChange;
  }

  /// @brief To get with @ref SetSupportsRecordingsLifetimeChange changed
  /// values.
  bool GetSupportsRecordingsLifetimeChange() const
  {
    return m_capabilities->bSupportsRecordingsLifetimeChange;
  }

  /// @brief Set **true** if the backend supports descramble information for
  /// playing channels.
  void SetSupportsDescrambleInfo(bool supportsDescrambleInfo)
  {
    m_capabilities->bSupportsDescrambleInfo = supportsDescrambleInfo;
  }

  /// @brief To get with @ref SetSupportsDescrambleInfo changed values.
  bool GetSupportsDescrambleInfo() const { return m_capabilities->bSupportsDescrambleInfo; }

  /// @brief Set **true** if this addon-on supports asynchronous transfer of epg
  /// events to Kodi using the callback function
  /// @ref kodi::addon::CInstancePVRClient::EpgEventStateChange().
  void SetSupportsAsyncEPGTransfer(bool supportsAsyncEPGTransfer)
  {
    m_capabilities->bSupportsAsyncEPGTransfer = supportsAsyncEPGTransfer;
  }

  /// @brief To get with @ref SetSupportsAsyncEPGTransfer changed values.
  bool GetSupportsAsyncEPGTransfer() const { return m_capabilities->bSupportsAsyncEPGTransfer; }

  /// @brief Set **true** if this addon-on supports retrieving size of recordings.
  void SetSupportsRecordingSize(bool supportsRecordingSize)
  {
    m_capabilities->bSupportsRecordingSize = supportsRecordingSize;
  }

  /// @brief To get with @ref SetSupportsRecordingSize changed values.
  bool GetSupportsRecordingSize() const { return m_capabilities->bSupportsRecordingSize; }

  /// @brief Set **true** if this add-on supports delete of recordings stored
  /// on the backend.
  void SetSupportsRecordingsDelete(bool supportsRecordingsDelete)
  {
    m_capabilities->bSupportsRecordingsDelete = supportsRecordingsDelete;
  }

  /// @brief To get with @ref SetSupportsRecordingsDelete changed values.
  bool GetSupportsRecordingsDelete() const { return m_capabilities->bSupportsRecordingsDelete; }

  /// @brief **optional**\n
  /// Set array containing the possible values for @ref PVRRecording::SetLifetime().
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetRecordingsLifetimeValues(const std::vector<PVRTypeIntValue>& recordingsLifetimeValues)
  {
    m_capabilities->iRecordingsLifetimesSize = 0;
    for (unsigned int i = 0; i < recordingsLifetimeValues.size() &&
                             i < sizeof(m_capabilities->recordingsLifetimeValues);
         ++i)
    {
      m_capabilities->recordingsLifetimeValues[i].iValue =
          recordingsLifetimeValues[i].GetCStructure()->iValue;
      strncpy(m_capabilities->recordingsLifetimeValues[i].strDescription,
              recordingsLifetimeValues[i].GetCStructure()->strDescription,
              sizeof(m_capabilities->recordingsLifetimeValues[i].strDescription) - 1);
      ++m_capabilities->iRecordingsLifetimesSize;
    }
  }

  /// @brief To get with @ref SetRecordingsLifetimeValues changed values.
  std::vector<PVRTypeIntValue> GetRecordingsLifetimeValues() const
  {
    std::vector<PVRTypeIntValue> recordingsLifetimeValues;
    for (unsigned int i = 0; i < m_capabilities->iRecordingsLifetimesSize; ++i)
      recordingsLifetimeValues.emplace_back(
          m_capabilities->recordingsLifetimeValues[i].iValue,
          m_capabilities->recordingsLifetimeValues[i].strDescription);
    return recordingsLifetimeValues;
  }
  ///@}

private:
  PVRCapabilities(PVR_ADDON_CAPABILITIES* capabilities) : m_capabilities(capabilities) {}

  PVR_ADDON_CAPABILITIES* m_capabilities;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty class PVRStreamProperty
/// @ingroup cpp_kodi_addon_pvr_Defs_General_Inputstream
/// @brief **PVR stream property value handler**\n
/// To set for Kodi wanted stream properties.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty_Help
///
///---------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// ...
///
/// PVR_ERROR CMyPVRInstance::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
///                                                      std::vector<kodi::addon::PVRStreamProperty>& properties)
/// {
///   ...
///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "inputstream.adaptive");
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// ...
/// ~~~~~~~~~~~~~
///
///
/// **Example 2:**
/// ~~~~~~~~~~~~~{.cpp}
/// ...
///
/// PVR_ERROR CMyPVRInstance::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
///                                                      std::vector<kodi::addon::PVRStreamProperty>& properties)
/// {
///   ...
///   kodi::addon::PVRStreamProperty property;
///   property.SetName(PVR_STREAM_PROPERTY_INPUTSTREAM);
///   property.SetValue("inputstream.adaptive");
///   properties.emplace_back(property);
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// ...
/// ~~~~~~~~~~~~~
///
///@{
class PVRStreamProperty : public CStructHdl<PVRStreamProperty, PVR_NAMED_VALUE>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRStreamProperty(const PVRStreamProperty& data) : CStructHdl(data) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Name** | `int` | @ref PVRStreamProperty::SetValue "SetName" | @ref PVRStreamProperty::GetName "GetName"
  /// | **Value** | `std::string` | @ref PVRStreamProperty::SetValue "SetValue" | @ref PVRStreamProperty::GetValue "GetValue"
  ///
  /// @remark Further can there be used his class constructor to set values.

  /// @addtogroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty
  ///@{

  /// @brief Default class constructor.
  ///
  /// @note Values must be set afterwards.
  PVRStreamProperty() = default;

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] name Type identification
  /// @param[in] value Type used property value
  PVRStreamProperty(const std::string& name, const std::string& value)
  {
    SetName(name);
    SetValue(value);
  }

  /// @brief To set with the identification name.
  void SetName(const std::string& name)
  {
    strncpy(m_cStructure->strName, name.c_str(), sizeof(m_cStructure->strName) - 1);
  }

  /// @brief To get with the identification name.
  std::string GetName() const { return m_cStructure->strName; }

  /// @brief To set with the used property value.
  void SetValue(const std::string& value)
  {
    strncpy(m_cStructure->strValue, value.c_str(), sizeof(m_cStructure->strValue) - 1);
  }

  /// @brief To get with the used property value.
  std::string GetValue() const { return m_cStructure->strValue; }
  ///@}

private:
  PVRStreamProperty(const PVR_NAMED_VALUE* data) : CStructHdl(data) {}
  PVRStreamProperty(PVR_NAMED_VALUE* data) : CStructHdl(data) {}
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
