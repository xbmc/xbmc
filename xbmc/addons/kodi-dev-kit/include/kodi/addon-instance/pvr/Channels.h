/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 2 - PVR channel
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVRChannel class PVRChannel
/// @ingroup cpp_kodi_addon_pvr_Defs_Channel
/// @brief **Channel data structure**\n
/// Representation of a TV or radio channel.
///
/// This is used to store all the necessary TV or radio channel data and can
/// either provide the necessary data from / to Kodi for the associated
/// functions or can also be used in the addon to store its data.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRChannel_Help
///
///@{
class PVRChannel : public CStructHdl<PVRChannel, PVR_CHANNEL>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRChannel()
  {
    memset(m_cStructure, 0, sizeof(PVR_CHANNEL));
    m_cStructure->iClientProviderUid = PVR_PROVIDER_INVALID_UID;
  }
  PVRChannel(const PVRChannel& channel) : CStructHdl(channel) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVRChannel_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Channel_PVRChannel
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Channel_PVRChannel :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Unique id** | `unsigned int` | @ref PVRChannel::SetUniqueId "SetUniqueId" | @ref PVRChannel::GetUniqueId "GetUniqueId" | *required to set*
  /// | **Is radio** | `bool` | @ref PVRChannel::SetIsRadio "SetIsRadio" | @ref PVRChannel::GetIsRadio "GetIsRadio" | *required to set*
  /// | **Channel number** | `unsigned int` | @ref PVRChannel::SetChannelNumber "SetChannelNumber" | @ref PVRChannel::GetChannelNumber "GetChannelNumber" | *optional*
  /// | **Sub channel number** | `unsigned int` | @ref PVRChannel::SetSubChannelNumber "SetSubChannelNumber" | @ref PVRChannel::GetSubChannelNumber "GetSubChannelNumber" | *optional*
  /// | **Channel name** | `std::string` | @ref PVRChannel::SetChannelName "SetChannelName" | @ref PVRChannel::GetChannelName "GetChannelName" | *optional*
  /// | **Mime type** | `std::string` | @ref PVRChannel::SetMimeType "SetMimeType" | @ref PVRChannel::GetMimeType "GetMimeType" | *optional*
  /// | **Encryption system** | `unsigned int` | @ref PVRChannel::SetEncryptionSystem "SetEncryptionSystem" | @ref PVRChannel::GetEncryptionSystem "GetEncryptionSystem" | *optional*
  /// | **Icon path** | `std::string` | @ref PVRChannel::SetIconPath "SetIconPath" | @ref PVRChannel::GetIconPath "GetIconPath" | *optional*
  /// | **Is hidden** | `bool` | @ref PVRChannel::SetIsHidden "SetIsHidden" | @ref PVRChannel::GetIsHidden "GetIsHidden" | *optional*
  /// | **Has archive** | `bool` | @ref PVRChannel::SetHasArchive "SetHasArchive" | @ref PVRChannel::GetHasArchive "GetHasArchive" | *optional*
  /// | **Order** | `int` | @ref PVRChannel::SetOrder "SetOrder" | @ref PVRChannel::GetOrder "GetOrder" | *optional*
  /// | **Client provider unique identifier** | `int` | @ref PVRChannel::SetClientProviderUid "SetClientProviderUid" | @ref PVRTimer::GetClientProviderUid "GetClientProviderUid" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Channel_PVRChannel
  ///@{

  /// @brief **required**\n
  /// Unique identifier for this channel.
  void SetUniqueId(unsigned int uniqueId) { m_cStructure->iUniqueId = uniqueId; }

  /// @brief To get with @ref SetUniqueId changed values.
  unsigned int GetUniqueId() const { return m_cStructure->iUniqueId; }

  /// @brief **required**\n
  /// **true** if this is a radio channel, **false** if it's a TV channel.
  void SetIsRadio(bool isRadio) { m_cStructure->bIsRadio = isRadio; }

  /// @brief To get with @ref SetIsRadio changed values.
  bool GetIsRadio() const { return m_cStructure->bIsRadio; }

  /// @brief **optional**\n
  /// Channel number of this channel on the backend.
  void SetChannelNumber(unsigned int channelNumber)
  {
    m_cStructure->iChannelNumber = channelNumber;
  }

  /// @brief To get with @ref SetChannelNumber changed values.
  unsigned int GetChannelNumber() const { return m_cStructure->iChannelNumber; }

  /// @brief **optional**\n
  /// Sub channel number of this channel on the backend (ATSC).
  void SetSubChannelNumber(unsigned int subChannelNumber)
  {
    m_cStructure->iSubChannelNumber = subChannelNumber;
  }

  /// @brief To get with @ref SetSubChannelNumber changed values.
  unsigned int GetSubChannelNumber() const { return m_cStructure->iSubChannelNumber; }

  /// @brief **optional**\n
  /// Channel name given to this channel.
  void SetChannelName(const std::string& channelName)
  {
    strncpy(m_cStructure->strChannelName, channelName.c_str(),
            sizeof(m_cStructure->strChannelName) - 1);
  }

  /// @brief To get with @ref SetChannelName changed values.
  std::string GetChannelName() const { return m_cStructure->strChannelName; }

  /// @brief **optional**\n
  /// Input format mime type.
  ///
  /// Available types can be found in https://www.iana.org/assignments/media-types/media-types.xhtml
  /// on "application" and "video" or leave empty if unknown.
  ///
  void SetMimeType(const std::string& inputFormat)
  {
    strncpy(m_cStructure->strMimeType, inputFormat.c_str(), sizeof(m_cStructure->strMimeType) - 1);
  }

  /// @brief To get with @ref SetMimeType changed values.
  std::string GetMimeType() const { return m_cStructure->strMimeType; }

  /// @brief **optional**\n
  /// The encryption ID or CaID of this channel (Conditional access systems).
  ///
  /// Lists about available ID's:
  /// - http://www.dvb.org/index.php?id=174
  /// - http://en.wikipedia.org/wiki/Conditional_access_system
  ///
  void SetEncryptionSystem(unsigned int encryptionSystem)
  {
    m_cStructure->iEncryptionSystem = encryptionSystem;
  }

  /// @brief To get with @ref SetEncryptionSystem changed values.
  unsigned int GetEncryptionSystem() const { return m_cStructure->iEncryptionSystem; }

  /// @brief **optional**\n
  /// Path to the channel icon (if present).
  void SetIconPath(const std::string& iconPath)
  {
    strncpy(m_cStructure->strIconPath, iconPath.c_str(), sizeof(m_cStructure->strIconPath) - 1);
  }

  /// @brief To get with @ref SetIconPath changed values.
  std::string GetIconPath() const { return m_cStructure->strIconPath; }

  /// @brief **optional**\n
  /// **true** if this channel is marked as hidden.
  void SetIsHidden(bool isHidden) { m_cStructure->bIsHidden = isHidden; }

  /// @brief To get with @ref GetIsRadio changed values.
  bool GetIsHidden() const { return m_cStructure->bIsHidden; }

  /// @brief **optional**\n
  /// **true** if this channel has a server-side back buffer.
  void SetHasArchive(bool hasArchive) { m_cStructure->bHasArchive = hasArchive; }

  /// @brief To get with @ref GetIsRadio changed values.
  bool GetHasArchive() const { return m_cStructure->bHasArchive; }

  /// @brief **optional**\n
  /// The value denoting the order of this channel in the 'All channels' group.
  void SetOrder(bool order) { m_cStructure->iOrder = order; }

  /// @brief To get with @ref SetOrder changed values.
  bool GetOrder() const { return m_cStructure->iOrder; }
  ///@}

  /// @brief **optional**\n
  /// Unique identifier of the provider this channel belongs to.
  ///
  /// @ref PVR_PROVIDER_INVALID_UID denotes that provider uid is not available.
  void SetClientProviderUid(int iClientProviderUid)
  {
    m_cStructure->iClientProviderUid = iClientProviderUid;
  }

  /// @brief To get with @ref SetClientProviderUid changed values
  int GetClientProviderUid() const { return m_cStructure->iClientProviderUid; }

private:
  PVRChannel(const PVR_CHANNEL* channel) : CStructHdl(channel) {}
  PVRChannel(PVR_CHANNEL* channel) : CStructHdl(channel) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVRChannelsResultSet class PVRChannelsResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_Channel_PVRChannel
/// @brief **PVR add-on channel transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetChannels().
///
///@{
class PVRChannelsResultSet
{
public:
  /*! \cond PRIVATE */
  PVRChannelsResultSet() = delete;
  PVRChannelsResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Channel_PVRChannelsResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] tag The to transferred data.
  void Add(const kodi::addon::PVRChannel& tag)
  {
    m_instance->toKodi->TransferChannelEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

  ///@}

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const PVR_HANDLE m_handle;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus class PVRSignalStatus
/// @ingroup cpp_kodi_addon_pvr_Defs_Channel
/// @brief **PVR Signal status information**\n
/// This class gives current status information from stream to Kodi.
///
/// Used to get information for user by call of @ref kodi::addon::CInstancePVRClient::GetSignalStatus()
/// to see current quality and source.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus_Help
///
///@{
class PVRSignalStatus : public CStructHdl<PVRSignalStatus, PVR_SIGNAL_STATUS>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRSignalStatus() = default;
  PVRSignalStatus(const PVRSignalStatus& type) : CStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Adapter name** | `std::string` | @ref PVRSignalStatus::SetAdapterName "SetAdapterName" | @ref PVRSignalStatus::GetAdapterName "GetAdapterName" | *optional*
  /// | **Adapter status** | `std::string` | @ref PVRSignalStatus::SetAdapterStatus "SetAdapterStatus" | @ref PVRSignalStatus::GetAdapterStatus "GetAdapterStatus" | *optional*
  /// | **Service name** | `std::string` | @ref PVRSignalStatus::SetServiceName "SetServiceName" | @ref PVRSignalStatus::GetServiceName "GetServiceName" | *optional*
  /// | **Provider name** | `std::string` | @ref PVRSignalStatus::SetProviderName "SetProviderName" | @ref PVRSignalStatus::GetProviderName "GetProviderName" | *optional*
  /// | **Mux name** | `std::string` | @ref PVRSignalStatus::SetMuxName "SetMuxName" | @ref PVRSignalStatus::GetMuxName "GetMuxName" | *optional*
  /// | **Signal/noise ratio** | `int` | @ref PVRSignalStatus::SetSNR "SetSNR" | @ref PVRSignalStatus::GetSNR "GetSNR" | *optional*
  /// | **Signal strength** | `int` | @ref PVRSignalStatus::SetSignal "SetSignal" | @ref PVRSignalStatus::GetSignal "GetSignal" | *optional*
  /// | **Bit error rate** | `long` | @ref PVRSignalStatus::SetBER "SetBER" | @ref PVRSignalStatus::GetBER "GetBER" | *optional*
  /// | **Uncorrected blocks** | `long` | @ref PVRSignalStatus::SetUNC "SetUNC" | @ref PVRSignalStatus::GetUNC "GetUNC" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus
  ///@{

  /// @brief **optional**\n
  /// Name of the adapter that's being used.
  void SetAdapterName(const std::string& adapterName)
  {
    strncpy(m_cStructure->strAdapterName, adapterName.c_str(),
            sizeof(m_cStructure->strAdapterName) - 1);
  }

  /// @brief To get with @ref SetAdapterName changed values.
  std::string GetAdapterName() const { return m_cStructure->strAdapterName; }

  /// @brief **optional**\n
  /// Status of the adapter that's being used.
  void SetAdapterStatus(const std::string& adapterStatus)
  {
    strncpy(m_cStructure->strAdapterStatus, adapterStatus.c_str(),
            sizeof(m_cStructure->strAdapterStatus) - 1);
  }

  /// @brief To get with @ref SetAdapterStatus changed values.
  std::string GetAdapterStatus() const { return m_cStructure->strAdapterStatus; }

  /// @brief **optional**\n
  /// Name of the current service.
  void SetServiceName(const std::string& serviceName)
  {
    strncpy(m_cStructure->strServiceName, serviceName.c_str(),
            sizeof(m_cStructure->strServiceName) - 1);
  }

  /// @brief To get with @ref SetServiceName changed values.
  std::string GetServiceName() const { return m_cStructure->strServiceName; }

  /// @brief **optional**\n
  /// Name of the current service's provider.
  void SetProviderName(const std::string& providerName)
  {
    strncpy(m_cStructure->strProviderName, providerName.c_str(),
            sizeof(m_cStructure->strProviderName) - 1);
  }

  /// @brief To get with @ref SetProviderName changed values.
  std::string GetProviderName() const { return m_cStructure->strProviderName; }

  /// @brief **optional**\n
  /// Name of the current mux.
  void SetMuxName(const std::string& muxName)
  {
    strncpy(m_cStructure->strMuxName, muxName.c_str(), sizeof(m_cStructure->strMuxName) - 1);
  }

  /// @brief To get with @ref SetMuxName changed values.
  std::string GetMuxName() const { return m_cStructure->strMuxName; }

  /// @brief **optional**\n
  /// Signal/noise ratio.
  ///
  /// @note 100% is 0xFFFF 65535
  void SetSNR(int snr) { m_cStructure->iSNR = snr; }

  /// @brief To get with @ref SetSNR changed values.
  int GetSNR() const { return m_cStructure->iSNR; }

  /// @brief **optional**\n
  /// Signal strength.
  ///
  /// @note 100% is 0xFFFF 65535
  void SetSignal(int signal) { m_cStructure->iSignal = signal; }

  /// @brief To get with @ref SetSignal changed values.
  int GetSignal() const { return m_cStructure->iSignal; }

  /// @brief **optional**\n
  /// Bit error rate.
  void SetBER(long ber) { m_cStructure->iBER = ber; }

  /// @brief To get with @ref SetBER changed values.
  long GetBER() const { return m_cStructure->iBER; }

  /// @brief **optional**\n
  /// Uncorrected blocks:
  void SetUNC(long unc) { m_cStructure->iUNC = unc; }

  /// @brief To get with @ref SetBER changed values.
  long GetUNC() const { return m_cStructure->iUNC; }
  ///@}

private:
  PVRSignalStatus(const PVR_SIGNAL_STATUS* type) : CStructHdl(type) {}
  PVRSignalStatus(PVR_SIGNAL_STATUS* type) : CStructHdl(type) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo class PVRDescrambleInfo
/// @ingroup cpp_kodi_addon_pvr_Defs_Channel
/// @brief **Data structure for descrample info**\n
/// Information data to give via this to Kodi.
///
/// As description see also here https://en.wikipedia.org/wiki/Conditional_access.
///
/// Used on @ref kodi::addon::CInstancePVRClient::GetDescrambleInfo().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo_Help
///
///@{
class PVRDescrambleInfo : public CStructHdl<PVRDescrambleInfo, PVR_DESCRAMBLE_INFO>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRDescrambleInfo()
  {
    m_cStructure->iPid = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iCaid = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iProvid = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iEcmTime = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iHops = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
  }
  PVRDescrambleInfo(const PVRDescrambleInfo& type) : CStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Packet identifier** | `int` | @ref PVRDescrambleInfo::SetPID "SetPID" | @ref PVRDescrambleInfo::GetPID "GetPID" | *optional*
  /// | **Conditional access identifier** | `int` | @ref PVRDescrambleInfo::SetCAID "SetCAID" | @ref PVRDescrambleInfo::GetCAID "GetCAID" | *optional*
  /// | **Provider-ID** | `int` | @ref PVRDescrambleInfo::SetProviderID "SetProviderID" | @ref PVRDescrambleInfo::GetProviderID "GetProviderID" | *optional*
  /// | **ECM time** | `int` | @ref PVRDescrambleInfo::SetECMTime "SetECMTime" | @ref PVRDescrambleInfo::GetECMTime "GetECMTime" | *optional*
  /// | **Hops** | `int` | @ref PVRDescrambleInfo::SetHops "SetHops" | @ref PVRDescrambleInfo::GetHops "GetHops" | *optional*
  /// | **Descramble card system** | `std::string` | @ref PVRDescrambleInfo::SetHops "SetHops" | @ref PVRDescrambleInfo::GetHops "GetHops" | *optional*
  /// | **Reader** | `std::string` | @ref PVRDescrambleInfo::SetReader "SetReader" | @ref PVRDescrambleInfo::GetReader "GetReader" | *optional*
  /// | **From** | `std::string` | @ref PVRDescrambleInfo::SetFrom "SetFrom" | @ref PVRDescrambleInfo::GetFrom "GetFrom" | *optional*
  /// | **Protocol** | `std::string` | @ref PVRDescrambleInfo::SetProtocol "SetProtocol" | @ref PVRDescrambleInfo::GetProtocol "GetProtocol" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo
  ///@{

  /// @brief **optional**\n
  /// Packet identifier.
  ///
  /// Each table or elementary stream in a transport stream is identified by
  /// a 13-bit packet identifier (PID).
  ///
  /// Is @ref PVR_DESCRAMBLE_INFO_NOT_AVAILABLE as default, if not available
  void SetPID(int pid) { m_cStructure->iPid = pid; }

  /// @brief To get with @ref SetPID changed values
  int GetPID() const { return m_cStructure->iPid; }

  /// @brief **optional**\n
  /// Conditional access identifier.
  ///
  /// Conditional access (abbreviated CA) or conditional access system (abbreviated CAS)
  /// is the protection of content by requiring certain criteria to be met before granting
  /// access to the content.
  ///
  /// Available CA system ID's listed here https://www.dvbservices.com/identifiers/ca_system_id.
  ///
  /// @ref PVR_DESCRAMBLE_INFO_NOT_AVAILABLE if not available.
  void SetCAID(int iCaid) { m_cStructure->iCaid = iCaid; }

  /// @brief To get with @ref SetCAID changed values.
  int GetCAID() const { return m_cStructure->iCaid; }

  /// @brief **optional**\n
  /// Provider-ID.
  ///
  /// Is @ref PVR_DESCRAMBLE_INFO_NOT_AVAILABLE as default, if not available.
  void SetProviderID(int provid) { m_cStructure->iProvid = provid; }

  /// @brief To get with @ref SetProviderID changed values
  int GetProviderID() const { return m_cStructure->iProvid; }

  /// @brief **optional**\n
  /// ECM time.
  ///
  /// Is @ref PVR_DESCRAMBLE_INFO_NOT_AVAILABLE as default, if not available.
  void SetECMTime(int ecmTime) { m_cStructure->iEcmTime = ecmTime; }

  /// @brief To get with @ref SetECMTime changed values.
  int GetECMTime() const { return m_cStructure->iEcmTime; }

  /// @brief **optional**\n
  /// Hops.
  ///
  /// Is @ref PVR_DESCRAMBLE_INFO_NOT_AVAILABLE as default, if not available.
  void SetHops(int hops) { m_cStructure->iHops = hops; }

  /// @brief To get with @ref SetHops changed values.
  int GetHops() const { return m_cStructure->iHops; }

  /// @brief **optional**\n
  /// Empty string if not available.
  void SetCardSystem(const std::string& cardSystem)
  {
    strncpy(m_cStructure->strCardSystem, cardSystem.c_str(),
            sizeof(m_cStructure->strCardSystem) - 1);
  }

  /// @brief To get with @ref SetCardSystem changed values.
  std::string GetCardSystem() const { return m_cStructure->strCardSystem; }

  /// @brief **optional**\n
  /// Empty string if not available.
  void SetReader(const std::string& reader)
  {
    strncpy(m_cStructure->strReader, reader.c_str(), sizeof(m_cStructure->strReader) - 1);
  }

  /// @brief To get with @ref SetReader changed values.
  std::string GetReader() const { return m_cStructure->strReader; }

  /// @brief **optional**\n
  /// Empty string if not available.
  void SetFrom(const std::string& from)
  {
    strncpy(m_cStructure->strFrom, from.c_str(), sizeof(m_cStructure->strFrom) - 1);
  }

  /// @brief To get with @ref SetFrom changed values.
  std::string GetFrom() const { return m_cStructure->strFrom; }

  /// @brief **optional**\n
  /// Empty string if not available.
  void SetProtocol(const std::string& protocol)
  {
    strncpy(m_cStructure->strProtocol, protocol.c_str(), sizeof(m_cStructure->strProtocol) - 1);
  }

  /// @brief To get with @ref SetProtocol changed values.
  std::string GetProtocol() const { return m_cStructure->strProtocol; }
  ///@}

private:
  PVRDescrambleInfo(const PVR_DESCRAMBLE_INFO* type) : CStructHdl(type) {}
  PVRDescrambleInfo(PVR_DESCRAMBLE_INFO* type) : CStructHdl(type) {}
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
